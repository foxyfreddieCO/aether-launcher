#include "ControllerManager.h"

#include <SDL3/SDL.h>

#include <QVariantMap>
#include <cmath>

namespace Aether {

namespace {
constexpr float kStickDeadzone = 0.45f;
constexpr float kTriggerThreshold = 0.55f;
constexpr qint64 kRepeatDelayMs = 340;
constexpr qint64 kRepeatIntervalMs = 125;

QString sdlButtonToLogicalId(uint8_t button) {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH:          return QStringLiteral("south");
    case SDL_GAMEPAD_BUTTON_EAST:           return QStringLiteral("east");
    case SDL_GAMEPAD_BUTTON_WEST:           return QStringLiteral("west");
    case SDL_GAMEPAD_BUTTON_NORTH:          return QStringLiteral("north");
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  return QStringLiteral("left_shoulder");
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return QStringLiteral("right_shoulder");
    case SDL_GAMEPAD_BUTTON_START:          return QStringLiteral("start");
    case SDL_GAMEPAD_BUTTON_BACK:           return QStringLiteral("back");
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:     return QStringLiteral("left_stick");
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    return QStringLiteral("right_stick");
    default:                                return {};
    }
}

QString familyFromType(SDL_GamepadType type) {
    switch (type) {
    case SDL_GAMEPAD_TYPE_PS3:
    case SDL_GAMEPAD_TYPE_PS4:
    case SDL_GAMEPAD_TYPE_PS5:
        return QStringLiteral("playstation");
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
        return QStringLiteral("nintendo");
    case SDL_GAMEPAD_TYPE_XBOX360:
    case SDL_GAMEPAD_TYPE_XBOXONE:
        return QStringLiteral("xbox");
    default:
        return QStringLiteral("xbox");
    }
}

QString powerStateToString(SDL_PowerState state) {
    switch (state) {
    case SDL_POWERSTATE_ON_BATTERY: return QStringLiteral("battery");
    case SDL_POWERSTATE_CHARGING:   return QStringLiteral("charging");
    case SDL_POWERSTATE_CHARGED:    return QStringLiteral("charged");
    case SDL_POWERSTATE_NO_BATTERY: return QStringLiteral("wired");
    default:                        return QStringLiteral("unknown");
    }
}
} // namespace

ControllerManager::ControllerManager(SettingsManager &settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings) {
    m_pollTimer.setInterval(16); // ~60 Hz responsive gamepad polling
    connect(&m_pollTimer, &QTimer::timeout, this, &ControllerManager::pollEvents);

    m_batteryTimer.setInterval(10000); // Update battery every 10s
    connect(&m_batteryTimer, &QTimer::timeout, this, &ControllerManager::refreshBatteryStatus);
}

ControllerManager::~ControllerManager() {
    shutdown();
}

bool ControllerManager::initialize() {
    if (m_sdlInitialized) return true;

    SDL_SetHint(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "0");

    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        return false;
    }

    m_sdlInitialized = true;

    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if (ids) {
        for (int i = 0; i < count; ++i) {
            openGamepad(ids[i]);
        }
        SDL_free(ids);
    }

    m_pollTimer.start();
    m_batteryTimer.start();
    return true;
}

void ControllerManager::shutdown() {
    m_pollTimer.stop();
    m_batteryTimer.stop();

    for (auto *pad : m_gamepads) {
        if (pad) SDL_CloseGamepad(pad);
    }
    m_gamepads.clear();
    m_primaryId = 0;

    if (m_sdlInitialized) {
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        m_sdlInitialized = false;
    }
}

void ControllerManager::setInputSuspended(bool suspended) {
    if (m_inputSuspended == suspended) return;
    m_inputSuspended = suspended;
    if (suspended) {
        m_heldDirection.clear();
        m_dpadMask = 0;
        m_stickX = 0.0f;
        m_stickY = 0.0f;
    }
    emit inputSuspendedChanged();
}

void ControllerManager::openGamepad(uint32_t instanceId) {
    if (m_gamepads.contains(instanceId)) return;
    SDL_Gamepad *pad = SDL_OpenGamepad(instanceId);
    if (!pad) return;

    m_gamepads.insert(instanceId, pad);
    if (m_primaryId == 0) {
        m_primaryId = instanceId;
    }
    const char *rawName = SDL_GetGamepadName(pad);
    const QString name = rawName ? QString::fromUtf8(rawName) : QStringLiteral("Controller");

    updatePrimaryInfo();
    setInputMode(QStringLiteral("controller"));
    emit controllerConnected(name);
}

void ControllerManager::closeGamepad(uint32_t instanceId) {
    auto it = m_gamepads.find(instanceId);
    if (it == m_gamepads.end()) return;

    const char *rawName = it.value() ? SDL_GetGamepadName(it.value()) : nullptr;
    const QString name = rawName ? QString::fromUtf8(rawName) : QStringLiteral("Controller");

    if (it.value()) SDL_CloseGamepad(it.value());
    m_gamepads.erase(it);

    if (m_primaryId == instanceId) {
        m_primaryId = m_gamepads.isEmpty() ? 0 : m_gamepads.constBegin().key();
    }
    updatePrimaryInfo();
    emit controllerDisconnected(name);
}

void ControllerManager::updatePrimaryInfo() {
    if (m_primaryId == 0 || !m_gamepads.contains(m_primaryId)) {
        m_primaryName.clear();
        m_controllerFamily = QStringLiteral("xbox");
        m_batteryPercent = -1;
        m_batteryState = QStringLiteral("unknown");
        emit controllerStateChanged();
        return;
    }

    SDL_Gamepad *pad = m_gamepads.value(m_primaryId);
    const char *rawName = SDL_GetGamepadName(pad);
    m_primaryName = rawName ? QString::fromUtf8(rawName) : QStringLiteral("Gamepad");
    m_controllerFamily = familyFromType(SDL_GetGamepadType(pad));

    int percent = -1;
    const SDL_PowerState pstate = SDL_GetGamepadPowerInfo(pad, &percent);
    m_batteryPercent = percent;
    m_batteryState = powerStateToString(pstate);

    emit controllerStateChanged();
}

void ControllerManager::refreshBatteryStatus() {
    if (m_primaryId != 0 && m_gamepads.contains(m_primaryId)) {
        updatePrimaryInfo();
    }
}

QVariantList ControllerManager::connectedControllersVariant() const {
    QVariantList list;
    for (auto it = m_gamepads.constBegin(); it != m_gamepads.constEnd(); ++it) {
        SDL_Gamepad *pad = it.value();
        int percent = -1;
        const SDL_PowerState ps = SDL_GetGamepadPowerInfo(pad, &percent);
        const char *name = SDL_GetGamepadName(pad);
        list.append(QVariantMap{
            {QStringLiteral("id"), static_cast<qulonglong>(it.key())},
            {QStringLiteral("name"), name ? QString::fromUtf8(name) : QStringLiteral("Gamepad")},
            {QStringLiteral("family"), familyFromType(SDL_GetGamepadType(pad))},
            {QStringLiteral("batteryPercent"), percent},
            {QStringLiteral("batteryState"), powerStateToString(ps)},
            {QStringLiteral("isPrimary"), it.key() == m_primaryId}
        });
    }
    return list;
}

void ControllerManager::setInputMode(const QString &mode) {
    if (m_inputMode == mode) return;
    m_inputMode = mode;
    emit inputModeChanged();
}

void ControllerManager::notifyKeyboardMouseActivity() {
    setInputMode(QStringLiteral("keyboard"));
    emit userActivity();
}

void ControllerManager::rumble(double lowFreq, double highFreq, int durationMs) {
    if (!m_settings.rumbleEnabled() || m_primaryId == 0 || !m_gamepads.contains(m_primaryId)) {
        return;
    }
    SDL_Gamepad *pad = m_gamepads.value(m_primaryId);
    const auto lo = static_cast<Uint16>(qBound(0.0, lowFreq, 1.0) * 65535.0);
    const auto hi = static_cast<Uint16>(qBound(0.0, highFreq, 1.0) * 65535.0);
    SDL_RumbleGamepad(pad, lo, hi, static_cast<Uint32>(qBound(10, durationMs, 2000)));
}

void ControllerManager::testRumble() {
    rumble(0.55, 0.70, 220);
}

void ControllerManager::beginRemap(const QString &actionName) {
    m_remappingAction = actionName;
    emit remappingChanged();
}

void ControllerManager::cancelRemap() {
    if (m_remappingAction.isEmpty()) return;
    m_remappingAction.clear();
    emit remappingChanged();
}

QString ControllerManager::glyphLabelForAction(const QString &actionName) const {
    if (m_inputMode == QLatin1String("keyboard")) {
        if (actionName == QLatin1String("confirm"))        return QStringLiteral("Enter");
        if (actionName == QLatin1String("back"))           return QStringLiteral("Esc");
        if (actionName == QLatin1String("info"))           return QStringLiteral("I");
        if (actionName == QLatin1String("options"))        return QStringLiteral("O");
        if (actionName == QLatin1String("prevCategory"))   return QStringLiteral("Q");
        if (actionName == QLatin1String("nextCategory"))   return QStringLiteral("E");
        if (actionName == QLatin1String("prevCollection")) return QStringLiteral("[");
        if (actionName == QLatin1String("nextCollection")) return QStringLiteral("]");
        if (actionName == QLatin1String("quickMenu"))      return QStringLiteral("Tab");
        return actionName;
    }

    const QString btn = m_settings.buttonMappings().value(actionName).toString();
    const bool isPS = (m_controllerFamily == QLatin1String("playstation"));

    if (btn == QLatin1String("south"))          return isPS ? QStringLiteral("Cross")    : QStringLiteral("A");
    if (btn == QLatin1String("east"))           return isPS ? QStringLiteral("Circle")   : QStringLiteral("B");
    if (btn == QLatin1String("west"))           return isPS ? QStringLiteral("Square")   : QStringLiteral("X");
    if (btn == QLatin1String("north"))          return isPS ? QStringLiteral("Triangle") : QStringLiteral("Y");
    if (btn == QLatin1String("left_shoulder"))  return isPS ? QStringLiteral("L1")       : QStringLiteral("LB");
    if (btn == QLatin1String("right_shoulder")) return isPS ? QStringLiteral("R1")       : QStringLiteral("RB");
    if (btn == QLatin1String("left_trigger"))   return isPS ? QStringLiteral("L2")       : QStringLiteral("LT");
    if (btn == QLatin1String("right_trigger"))  return isPS ? QStringLiteral("R2")       : QStringLiteral("RT");
    if (btn == QLatin1String("start"))          return isPS ? QStringLiteral("Options")  : QStringLiteral("Start");
    if (btn == QLatin1String("back"))           return isPS ? QStringLiteral("Create")   : QStringLiteral("View");
    return btn.toUpper();
}

void ControllerManager::handlePhysicalButton(const QString &buttonId) {
    if (buttonId.isEmpty()) return;

    setInputMode(QStringLiteral("controller"));
    emit userActivity();

    if (!m_remappingAction.isEmpty()) {
        m_settings.setButtonMapping(m_remappingAction, buttonId);
        m_remappingAction.clear();
        rumble(0.3, 0.5, 90);
        emit remappingChanged();
        return;
    }

    const QVariantMap map = m_settings.buttonMappings();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (it.value().toString() == buttonId) {
            if (it.key() == QLatin1String("confirm")) {
                rumble(0.12, 0.22, 35);
            }
            emit actionTriggered(it.key());
            return;
        }
    }
}

void ControllerManager::processDirectionalHold(const QString &direction) {
    if (direction.isEmpty()) {
        m_heldDirection.clear();
        m_holdRepeatStarted = false;
        return;
    }

    setInputMode(QStringLiteral("controller"));
    emit userActivity();

    if (direction != m_heldDirection) {
        m_heldDirection = direction;
        m_holdRepeatStarted = false;
        m_holdTimer.restart();
        emit navigate(direction);
        return;
    }

    const qint64 elapsed = m_holdTimer.elapsed();
    if (!m_holdRepeatStarted && elapsed >= kRepeatDelayMs) {
        m_holdRepeatStarted = true;
        m_holdTimer.restart();
        emit navigate(direction);
    } else if (m_holdRepeatStarted && elapsed >= kRepeatIntervalMs) {
        m_holdTimer.restart();
        emit navigate(direction);
    }
}

void ControllerManager::pollEvents() {
    if (!m_sdlInitialized) return;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
            openGamepad(event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            closeGamepad(event.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
            if (m_inputSuspended) break;
            m_primaryId = event.gbutton.which;
            const uint8_t b = event.gbutton.button;
            if (b == SDL_GAMEPAD_BUTTON_DPAD_UP)    { m_dpadMask |= 1; break; }
            if (b == SDL_GAMEPAD_BUTTON_DPAD_DOWN)  { m_dpadMask |= 2; break; }
            if (b == SDL_GAMEPAD_BUTTON_DPAD_LEFT)  { m_dpadMask |= 4; break; }
            if (b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) { m_dpadMask |= 8; break; }
            handlePhysicalButton(sdlButtonToLogicalId(b));
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_UP: {
            const uint8_t b = event.gbutton.button;
            if (b == SDL_GAMEPAD_BUTTON_DPAD_UP)    m_dpadMask &= ~1;
            if (b == SDL_GAMEPAD_BUTTON_DPAD_DOWN)  m_dpadMask &= ~2;
            if (b == SDL_GAMEPAD_BUTTON_DPAD_LEFT)  m_dpadMask &= ~4;
            if (b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) m_dpadMask &= ~8;
            break;
        }
        case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
            if (m_inputSuspended) break;
            const float norm = static_cast<float>(event.gaxis.value) / 32767.0f;
            if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
                m_stickX = norm;
            } else if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
                m_stickY = norm;
            } else if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER) {
                const bool down = norm >= kTriggerThreshold;
                if (down && !m_leftTriggerDown) {
                    m_leftTriggerDown = true;
                    handlePhysicalButton(QStringLiteral("left_trigger"));
                } else if (!down) {
                    m_leftTriggerDown = false;
                }
            } else if (event.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
                const bool down = norm >= kTriggerThreshold;
                if (down && !m_rightTriggerDown) {
                    m_rightTriggerDown = true;
                    handlePhysicalButton(QStringLiteral("right_trigger"));
                } else if (!down) {
                    m_rightTriggerDown = false;
                }
            }
            break;
        }
        default:
            break;
        }
    }

    if (m_inputSuspended) return;

    QString dir;
    if (m_dpadMask & 1)      dir = QStringLiteral("up");
    else if (m_dpadMask & 2) dir = QStringLiteral("down");
    else if (m_dpadMask & 4) dir = QStringLiteral("left");
    else if (m_dpadMask & 8) dir = QStringLiteral("right");
    else if (std::fabs(m_stickX) >= kStickDeadzone || std::fabs(m_stickY) >= kStickDeadzone) {
        if (std::fabs(m_stickX) >= std::fabs(m_stickY)) {
            dir = (m_stickX < 0.0f) ? QStringLiteral("left") : QStringLiteral("right");
        } else {
            dir = (m_stickY < 0.0f) ? QStringLiteral("up") : QStringLiteral("down");
        }
    }
    processDirectionalHold(dir);
}

} // namespace Aether
