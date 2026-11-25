#include "AudioManager.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QtMath>

namespace Aether {

namespace {
constexpr int kSampleRate = 44100;
constexpr double kTwoPi = 6.28318530717958647692;

QVector<float> synthToneSweep(double f0, double f1, double durationSec, double gain, double attackSec = 0.005) {
    const int n = static_cast<int>(durationSec * kSampleRate);
    QVector<float> out(n);
    double phase = 0.0;
    for (int i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / kSampleRate;
        const double u = t / durationSec;
        const double freq = f0 + (f1 - f0) * u;
        phase += kTwoPi * freq / kSampleRate;
        const double envAttack = qMin(1.0, t / qMax(0.001, attackSec));
        const double envDecay = qPow(1.0 - u, 2.2);
        const double sig = 0.72 * qSin(phase) + 0.28 * qSin(phase * 2.0) * (1.0 - u);
        out[i] = static_cast<float>(sig * envAttack * envDecay * gain);
    }
    return out;
}

QVector<float> synthChord(const QVector<double> &freqs, double durationSec, double gain, double attackSec, double releasePow) {
    const int n = static_cast<int>(durationSec * kSampleRate);
    QVector<float> out(n, 0.0f);
    for (int i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / kSampleRate;
        const double u = t / durationSec;
        const double env = qMin(1.0, t / qMax(0.002, attackSec)) * qPow(1.0 - u, releasePow);
        double sum = 0.0;
        for (double f : freqs) {
            sum += qSin(kTwoPi * f * t);
        }
        out[i] = static_cast<float>((sum / qMax(1, static_cast<int>(freqs.size()))) * env * gain);
    }
    return out;
}

QVector<float> synthAmbientPad(double durationSec = 10.0) {
    // Seamless gentle console-style Cmaj9 ambient pad loop
    const int n = static_cast<int>(durationSec * kSampleRate);
    QVector<float> out(n, 0.0f);
    const QVector<double> freqs = {130.81, 196.00, 246.94, 293.66, 392.00};
    for (int i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / kSampleRate;
        const double lfo = 0.65 + 0.35 * qSin(kTwoPi * (1.0 / durationSec) * t);
        double sum = 0.0;
        for (int k = 0; k < freqs.size(); ++k) {
            const double weight = (k == 0) ? 0.35 : 0.16;
            const double shimmer = 0.8 + 0.2 * qSin(kTwoPi * ((k + 1.0) / durationSec) * t);
            sum += weight * shimmer * qSin(kTwoPi * freqs[k] * t);
        }
        // Smooth edge crossfade for seamless looping
        const double edge = qMin(1.0, qMin(t / 0.25, (durationSec - t) / 0.25));
        out[i] = static_cast<float>(sum * lfo * edge * 0.22);
    }
    return out;
}
} // namespace

AudioManager::AudioManager(const AppPaths &paths, SettingsManager &settings, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_settings(settings) {
    connect(&m_settings, &SettingsManager::settingsChanged, this, &AudioManager::applySettings);
}

AudioManager::~AudioManager() {
    if (m_bgmPlayer) {
        m_bgmPlayer->stop();
    }
}

void AudioManager::initialize() {
    if (m_initialized) return;
    m_initialized = true;

    m_paths.ensureDirectoriesExist();
    ensureDefaultSoundsGenerated();

    m_bgmOutput = new QAudioOutput(this);
    m_bgmPlayer = new QMediaPlayer(this);
    m_bgmPlayer->setAudioOutput(m_bgmOutput);
    m_bgmPlayer->setLoops(QMediaPlayer::Infinite);

    reloadSoundPack();
    applySettings();
}

void AudioManager::ensureDefaultSoundsGenerated() const {
    const QDir dir(m_paths.audioCacheDir());
    auto writeIfMissing = [&](const QString &name, const QVector<float> &samples) {
        const QString p = dir.filePath(name + QStringLiteral(".wav"));
        if (!QFileInfo::exists(p)) {
            writeWavPcm16(p, samples, kSampleRate);
        }
    };

    writeIfMissing(QStringLiteral("navigate"), synthToneSweep(640.0, 880.0, 0.055, 0.26, 0.003));
    writeIfMissing(QStringLiteral("confirm"),  synthChord({523.25, 659.25, 783.99, 1046.50}, 0.22, 0.34, 0.008, 2.0));
    writeIfMissing(QStringLiteral("back"),     synthToneSweep(520.0, 310.0, 0.14, 0.28, 0.006));
    writeIfMissing(QStringLiteral("launch"),   synthChord({261.63, 392.00, 523.25, 659.25, 783.99}, 0.72, 0.40, 0.06, 1.5));
    writeIfMissing(QStringLiteral("exit"),     synthChord({392.00, 523.25, 659.25}, 0.36, 0.30, 0.02, 1.8));
    writeIfMissing(QStringLiteral("error"),    synthToneSweep(220.0, 165.0, 0.22, 0.34, 0.005));
    writeIfMissing(QStringLiteral("ambient"),  synthAmbientPad(10.0));
}

QString AudioManager::resolveSoundPath(const QString &name) const {
    const QString packDir = m_settings.customSoundPackDir().trimmed();
    if (!packDir.isEmpty()) {
        const QString customWav = QDir(packDir).filePath(name + QStringLiteral(".wav"));
        if (QFileInfo::exists(customWav)) return customWav;
    }
    return QDir(m_paths.audioCacheDir()).filePath(name + QStringLiteral(".wav"));
}

void AudioManager::loadEffect(const QString &name) {
    QSoundEffect *eff = m_effects.value(name, nullptr);
    if (!eff) {
        eff = new QSoundEffect(this);
        m_effects.insert(name, eff);
    }
    const QString path = resolveSoundPath(name);
    if (QFileInfo::exists(path)) {
        eff->setSource(QUrl::fromLocalFile(path));
    }
}

void AudioManager::reloadSoundPack() {
    if (!m_initialized) return;
    const QStringList names = {
        QStringLiteral("navigate"),
        QStringLiteral("confirm"),
        QStringLiteral("back"),
        QStringLiteral("launch"),
        QStringLiteral("exit"),
        QStringLiteral("error")
    };
    for (const QString &name : names) {
        loadEffect(name);
    }
    applySettings();
}

void AudioManager::applySettings() {
    if (!m_initialized) return;

    const bool muted = m_settings.audioMuted();
    const double sfxVol = muted ? 0.0 : m_settings.sfxVolume();
    for (auto *eff : m_effects) {
        eff->setVolume(sfxVol);
    }

    if (m_bgmPlayer && m_bgmOutput) {
        const double bgmVol = muted ? 0.0 : m_settings.bgmVolume();
        m_bgmOutput->setVolume(static_cast<float>(bgmVol));

        QString bgmFile = m_settings.customBgmPath().trimmed();
        if (bgmFile.isEmpty() || !QFileInfo::exists(bgmFile)) {
            const QString packDir = m_settings.customSoundPackDir().trimmed();
            if (!packDir.isEmpty()) {
                for (const QString &ext : {QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("ogg"), QStringLiteral("flac")}) {
                    const QString cand = QDir(packDir).filePath(QStringLiteral("ambient.") + ext);
                    if (QFileInfo::exists(cand)) {
                        bgmFile = cand;
                        break;
                    }
                }
            }
        }
        if (bgmFile.isEmpty() || !QFileInfo::exists(bgmFile)) {
            bgmFile = QDir(m_paths.audioCacheDir()).filePath(QStringLiteral("ambient.wav"));
        }

        const QUrl desiredUrl = QUrl::fromLocalFile(bgmFile);
        if (m_bgmPlayer->source() != desiredUrl && QFileInfo::exists(bgmFile)) {
            m_bgmPlayer->setSource(desiredUrl);
        }

        if (m_settings.bgmEnabled() && !muted && !m_suspendedForGame) {
            if (m_bgmPlayer->playbackState() != QMediaPlayer::PlayingState) {
                m_bgmPlayer->play();
            }
        } else {
            if (m_bgmPlayer->playbackState() == QMediaPlayer::PlayingState) {
                m_bgmPlayer->pause();
            }
        }
    }
}

void AudioManager::playNavigate() {
    if (!m_initialized || !m_settings.sfxEnabled() || m_settings.audioMuted() || m_suspendedForGame) return;
    if (auto *e = m_effects.value(QStringLiteral("navigate"), nullptr)) e->play();
}

void AudioManager::playConfirm() {
    if (!m_initialized || !m_settings.sfxEnabled() || m_settings.audioMuted() || m_suspendedForGame) return;
    if (auto *e = m_effects.value(QStringLiteral("confirm"), nullptr)) e->play();
}

void AudioManager::playBack() {
    if (!m_initialized || !m_settings.sfxEnabled() || m_settings.audioMuted() || m_suspendedForGame) return;
    if (auto *e = m_effects.value(QStringLiteral("back"), nullptr)) e->play();
}

void AudioManager::playLaunch() {
    if (!m_initialized || !m_settings.sfxEnabled() || m_settings.audioMuted()) return;
    if (auto *e = m_effects.value(QStringLiteral("launch"), nullptr)) e->play();
}

void AudioManager::playExit() {
    if (!m_initialized || !m_settings.sfxEnabled() || m_settings.audioMuted()) return;
    if (auto *e = m_effects.value(QStringLiteral("exit"), nullptr)) e->play();
}

void AudioManager::playError() {
    if (!m_initialized || !m_settings.sfxEnabled() || m_settings.audioMuted()) return;
    if (auto *e = m_effects.value(QStringLiteral("error"), nullptr)) e->play();
}

void AudioManager::suspendForGame() {
    m_suspendedForGame = true;
    applySettings();
}

void AudioManager::resumeAfterGame() {
    m_suspendedForGame = false;
    applySettings();
}

bool AudioManager::writeWavPcm16(const QString &filePath, const QVector<float> &samples, int sampleRate) {
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    const quint32 numSamples = static_cast<quint32>(samples.size());
    const quint16 numChannels = 1;
    const quint16 bitsPerSample = 16;
    const quint32 byteRate = static_cast<quint32>(sampleRate) * numChannels * bitsPerSample / 8;
    const quint16 blockAlign = numChannels * bitsPerSample / 8;
    const quint32 dataSize = numSamples * blockAlign;
    const quint32 chunkSize = 36 + dataSize;

    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);

    out.writeRawData("RIFF", 4);
    out << chunkSize;
    out.writeRawData("WAVE", 4);
    out.writeRawData("fmt ", 4);
    out << static_cast<quint32>(16); // PCM
    out << static_cast<quint16>(1);  // AudioFormat = 1
    out << numChannels;
    out << static_cast<quint32>(sampleRate);
    out << byteRate;
    out << blockAlign;
    out << bitsPerSample;
    out.writeRawData("data", 4);
    out << dataSize;

    for (float s : samples) {
        const float clamped = qBound(-1.0f, s, 1.0f);
        out << static_cast<qint16>(clamped * 32767.0f);
    }
    return true;
}

} // namespace Aether
