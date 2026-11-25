#pragma once

#include "core/AppPaths.h"
#include "core/SettingsManager.h"

#include <QAudioOutput>
#include <QHash>
#include <QMediaPlayer>
#include <QObject>
#include <QSoundEffect>
#include <QString>

namespace Aether {

class AudioManager : public QObject {
    Q_OBJECT
public:
    explicit AudioManager(const AppPaths &paths, SettingsManager &settings, QObject *parent = nullptr);
    ~AudioManager() override;

    void initialize();

    Q_INVOKABLE void playNavigate();
    Q_INVOKABLE void playConfirm();
    Q_INVOKABLE void playBack();
    Q_INVOKABLE void playLaunch();
    Q_INVOKABLE void playExit();
    Q_INVOKABLE void playError();
    Q_INVOKABLE void reloadSoundPack();

    void suspendForGame();
    void resumeAfterGame();

private:
    void ensureDefaultSoundsGenerated() const;
    QString resolveSoundPath(const QString &name) const;
    void loadEffect(const QString &name);
    void applySettings();

    static bool writeWavPcm16(const QString &filePath, const QVector<float> &samples, int sampleRate = 44100);

    AppPaths m_paths;
    SettingsManager &m_settings;
    QHash<QString, QSoundEffect *> m_effects;
    QMediaPlayer *m_bgmPlayer = nullptr;
    QAudioOutput *m_bgmOutput = nullptr;
    bool m_suspendedForGame = false;
    bool m_initialized = false;
};

} // namespace Aether
