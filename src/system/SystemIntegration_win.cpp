#include "SystemIntegration.h"

#ifdef Q_OS_WIN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <pdh.h>
#include <pdhmsg.h>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

// Standard Windows undocumented IPolicyConfig interface for switching default audio output device
interface DECLSPEC_UUID("f8679f50-850a-41cf-9c72-430f290290c8") IPolicyConfig : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, WAVEFORMATEX **) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, WAVEFORMATEX **) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, WAVEFORMATEX *, WAVEFORMATEX *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, PINT64, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, struct DeviceShareMode *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, struct DeviceShareMode *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY &, PROPVARIANT *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY &, PROPVARIANT *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR wszDeviceId, ERole role) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
};

static const CLSID CLSID_CPolicyConfigClient =
    {0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};
static const IID IID_IPolicyConfig =
    {0xf8679f50, 0x850a, 0x41cf, {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8}};

namespace Aether {

struct SystemIntegration::PlatformPrivate {
    bool comInitialized = false;
    ULONGLONG prevIdle = 0;
    ULONGLONG prevKernel = 0;
    ULONGLONG prevUser = 0;
    bool hasPrevCpu = false;

    PDH_HQUERY gpuQuery = nullptr;
    PDH_HCOUNTER gpuCounter = nullptr;
    bool pdhReady = false;
};

namespace {
ULONGLONG fileTimeToU64(const FILETIME &ft) {
    return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

IAudioEndpointVolume *getDefaultEndpointVolume() {
    IMMDeviceEnumerator *enumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&enumerator)))) {
        return nullptr;
    }
    IMMDevice *device = nullptr;
    IAudioEndpointVolume *vol = nullptr;
    if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device)) && device) {
        device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&vol));
        device->Release();
    }
    enumerator->Release();
    return vol;
}
} // namespace

void SystemIntegration::platformInit() {
    m_priv = new PlatformPrivate();
    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    m_priv->comInitialized = SUCCEEDED(hr);

    // Initialize Windows 11 PDH GPU 3D Engine utilization counter
    if (PdhOpenQueryW(nullptr, 0, &m_priv->gpuQuery) == ERROR_SUCCESS) {
        if (PdhAddEnglishCounterW(m_priv->gpuQuery, L"\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &m_priv->gpuCounter) == ERROR_SUCCESS) {
            PdhCollectQueryData(m_priv->gpuQuery);
            m_priv->pdhReady = true;
        }
    }
}

void SystemIntegration::platformShutdown() {
    if (m_priv) {
        if (m_priv->gpuQuery) {
            PdhCloseQuery(m_priv->gpuQuery);
        }
        if (m_priv->comInitialized) {
            CoUninitialize();
        }
        delete m_priv;
        m_priv = nullptr;
    }
}

SystemTelemetry SystemIntegration::platformReadTelemetry() {
    SystemTelemetry t;

    // CPU via GetSystemTimes
    FILETIME idleTime{}, kernelTime{}, userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        const ULONGLONG idle = fileTimeToU64(idleTime);
        const ULONGLONG kernel = fileTimeToU64(kernelTime);
        const ULONGLONG user = fileTimeToU64(userTime);
        if (m_priv && m_priv->hasPrevCpu) {
            const ULONGLONG dIdle = idle - m_priv->prevIdle;
            const ULONGLONG dKernel = kernel - m_priv->prevKernel;
            const ULONGLONG dUser = user - m_priv->prevUser;
            const ULONGLONG total = dKernel + dUser;
            if (total > 0) {
                t.cpuUsagePercent = qBound(0.0, (1.0 - static_cast<double>(dIdle) / static_cast<double>(total)) * 100.0, 100.0);
            }
        }
        if (m_priv) {
            m_priv->prevIdle = idle;
            m_priv->prevKernel = kernel;
            m_priv->prevUser = user;
            m_priv->hasPrevCpu = true;
        }
    }

    // RAM via GlobalMemoryStatusEx
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        const double totalGb = static_cast<double>(mem.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
        const double availGb = static_cast<double>(mem.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
        t.ramTotalGb = totalGb;
        t.ramUsedGb = qMax(0.0, totalGb - availGb);
        t.ramUsagePercent = static_cast<double>(mem.dwMemoryLoad);
    }

    // GPU via PDH 3D Engine counter
    if (m_priv && m_priv->pdhReady && PdhCollectQueryData(m_priv->gpuQuery) == ERROR_SUCCESS) {
        DWORD bufSize = 0;
        DWORD itemCount = 0;
        if (PdhGetFormattedCounterArrayW(m_priv->gpuCounter, PDH_FMT_DOUBLE, &bufSize, &itemCount, nullptr) == PDH_MORE_DATA && bufSize > 0) {
            QByteArray buffer(static_cast<int>(bufSize), '\0');
            auto *items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W *>(buffer.data());
            if (PdhGetFormattedCounterArrayW(m_priv->gpuCounter, PDH_FMT_DOUBLE, &bufSize, &itemCount, items) == ERROR_SUCCESS) {
                double sum = 0.0;
                for (DWORD i = 0; i < itemCount; ++i) {
                    if (items[i].FmtValue.CStatus == ERROR_SUCCESS) {
                        sum += items[i].FmtValue.doubleValue;
                    }
                }
                t.gpuAvailable = true;
                t.gpuUsagePercent = qBound(0.0, sum, 100.0);
            }
        }
    }

    // Temperature: only report if real sensor accessible (never fabricate)
    t.temperatureAvailable = false;
    t.temperatureCelsius = -1.0;

    return t;
}

int SystemIntegration::platformGetVolumePercent() const {
    IAudioEndpointVolume *vol = getDefaultEndpointVolume();
    if (!vol) return -1;
    float scalar = 0.0f;
    const HRESULT hr = vol->GetMasterVolumeLevelScalar(&scalar);
    vol->Release();
    return SUCCEEDED(hr) ? qRound(scalar * 100.0f) : -1;
}

bool SystemIntegration::platformSetVolumePercent(int percent) {
    IAudioEndpointVolume *vol = getDefaultEndpointVolume();
    if (!vol) return false;
    const float scalar = static_cast<float>(qBound(0, percent, 100)) / 100.0f;
    const HRESULT hr = vol->SetMasterVolumeLevelScalar(scalar, nullptr);
    vol->Release();
    return SUCCEEDED(hr);
}

bool SystemIntegration::platformGetMuted() const {
    IAudioEndpointVolume *vol = getDefaultEndpointVolume();
    if (!vol) return false;
    BOOL muted = FALSE;
    vol->GetMute(&muted);
    vol->Release();
    return muted == TRUE;
}

bool SystemIntegration::platformSetMuted(bool muted) {
    IAudioEndpointVolume *vol = getDefaultEndpointVolume();
    if (!vol) return false;
    const HRESULT hr = vol->SetMute(muted ? TRUE : FALSE, nullptr);
    vol->Release();
    return SUCCEEDED(hr);
}

QList<AudioOutputDevice> SystemIntegration::platformListAudioOutputs() const {
    QList<AudioOutputDevice> out;
    IMMDeviceEnumerator *enumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&enumerator)))) {
        return out;
    }

    QString defaultId;
    IMMDevice *defDev = nullptr;
    if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defDev)) && defDev) {
        LPWSTR rawId = nullptr;
        if (SUCCEEDED(defDev->GetId(&rawId)) && rawId) {
            defaultId = QString::fromWCharArray(rawId);
            CoTaskMemFree(rawId);
        }
        defDev->Release();
    }

    IMMDeviceCollection *collection = nullptr;
    if (SUCCEEDED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection)) && collection) {
        UINT count = 0;
        collection->GetCount(&count);
        for (UINT i = 0; i < count; ++i) {
            IMMDevice *dev = nullptr;
            if (SUCCEEDED(collection->Item(i, &dev)) && dev) {
                AudioOutputDevice item;
                LPWSTR rawId = nullptr;
                if (SUCCEEDED(dev->GetId(&rawId)) && rawId) {
                    item.id = QString::fromWCharArray(rawId);
                    CoTaskMemFree(rawId);
                }
                IPropertyStore *props = nullptr;
                if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &props)) && props) {
                    PROPVARIANT varName;
                    PropVariantInit(&varName);
                    if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &varName)) && varName.vt == VT_LPWSTR) {
                        item.name = QString::fromWCharArray(varName.pwszVal);
                    }
                    PropVariantClear(&varName);
                    props->Release();
                }
                item.isDefault = (!defaultId.isEmpty() && item.id == defaultId);
                if (!item.name.isEmpty()) out.append(item);
                dev->Release();
            }
        }
        collection->Release();
    }
    enumerator->Release();
    return out;
}

bool SystemIntegration::platformSelectAudioOutput(const QString &deviceId) {
    if (deviceId.isEmpty()) return false;
    IPolicyConfig *policy = nullptr;
    if (FAILED(CoCreateInstance(CLSID_CPolicyConfigClient, nullptr, CLSCTX_ALL,
                                IID_IPolicyConfig, reinterpret_cast<void **>(&policy))) || !policy) {
        return openSoundSettings();
    }
    const std::wstring wid = deviceId.toStdWString();
    const HRESULT hr1 = policy->SetDefaultEndpoint(wid.c_str(), eConsole);
    const HRESULT hr2 = policy->SetDefaultEndpoint(wid.c_str(), eMultimedia);
    const HRESULT hr3 = policy->SetDefaultEndpoint(wid.c_str(), eCommunications);
    policy->Release();
    return SUCCEEDED(hr1) || SUCCEEDED(hr2) || SUCCEEDED(hr3);
}

bool SystemIntegration::platformOpenBluetoothSettings() {
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", L"ms-settings:bluetooth", nullptr, nullptr, SW_SHOWNORMAL)) > 32;
}

bool SystemIntegration::platformOpenWifiSettings() {
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", L"ms-settings:network-wifi", nullptr, nullptr, SW_SHOWNORMAL)) > 32;
}

bool SystemIntegration::platformOpenSoundSettings() {
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", L"ms-settings:sound", nullptr, nullptr, SW_SHOWNORMAL)) > 32;
}

bool SystemIntegration::platformSetStartupWithWindows(bool enabled) {
    QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    if (enabled) {
        const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        reg.setValue(QStringLiteral("AetherLauncher"), QStringLiteral("\"%1\"").arg(exe));
    } else {
        reg.remove(QStringLiteral("AetherLauncher"));
    }
    reg.sync();
    return reg.status() == QSettings::NoError;
}

void SystemIntegration::platformApplyWindowStyle(QWindow *window) {
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) return;

    // Enable Windows 11 Dark Immersive Titlebar & Rounded Corners
    const BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));

    const DWORD backdrop = 3 /* DWMSBT_TRANSIENTWINDOW / Acrylic */;
    DwmSetWindowAttribute(hwnd, 38 /* DWMWA_SYSTEMBACKDROP_TYPE */, &backdrop, sizeof(backdrop));
}

} // namespace Aether

#endif // Q_OS_WIN
