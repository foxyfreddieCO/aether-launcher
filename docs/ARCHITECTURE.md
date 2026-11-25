# Aether Launcher — Architecture & Auto-Update Design

## 1. Technology Stack Rationale

Aether Launcher is built as a **100% native Windows 11 desktop application** using **C++20 + Qt 6 (Qt Quick / QML + QtQuickEffects) + SDL3 + SQLite**, not a browser or Electron wrapper.

| Requirement | Technology Choice | Why It Excels |
| :--- | :--- | :--- |
| **Smooth Liquid Glass UI & 60–120fps Animations** | **Qt 6 Quick (QML) + `MultiEffect` GPU Shaders** | Compiles directly to native Direct3D 11/12 via Qt RHI on Windows 11. Hardware-accelerated backdrop blur, crossfading dynamic game artwork, translucent specular surfaces, and spring transitions run at 60–120fps with ~60–100 MB RAM overhead (vs. 500+ MB for Chromium/Electron). |
| **Native Windows 11 Integration** | **Win32 / COM / WASAPI / DWM / PDH C++ Backend** | Direct access to Windows 11 DWM (`DwmSetWindowAttribute`), WASAPI Core Audio (`IMMDeviceEnumerator`, `IAudioEndpointVolume`, `IPolicyConfig`), PDH 3D GPU utilization counters, `GetSystemTimes`, `GlobalMemoryStatusEx`, `ms-settings:` URI shortcuts, and `HKCU\...\Run` startup registration. |
| **Bluetooth & USB Controller Support** | **SDL3 (`SDL_Gamepad`)** | The exact gamepad subsystem used by Steam Big Picture. Supports Xbox, PlayStation DualSense / DualShock 4, Switch Pro, and generic controllers over Bluetooth & USB with hotplug detection, real wireless battery reporting (`SDL_GetGamepadPowerInfo`), dual-motor hardware rumble (`SDL_RumbleGamepad`), and automatic Xbox/PS glyph detection. |
| **Emulator & Game Launching** | **`QProcess` + Safe Template Argument Parser** | Launches standalone `.exe` games or any configured emulator (including dedicated configurable **Lichii** for Nintendo Switch 2) using token-expanded `argv` lists (`{rom}`, `{rom_dir}`, `{rom_name}`, `{title}`), detects process exit accurately, records playtime/last-played to SQLite, and restores launcher focus cleanly. |
| **Persistent Local Storage** | **SQLite (WAL mode) + Atomic JSON Settings** | Zero external database daemon. Strict data isolation guarantees `%LOCALAPPDATA%\AetherLauncher\` is never touched by installation or update swaps. |
| **Automatic Updates & Rollback** | **GitHub Actions CI + Standalone `AetherUpdater.exe`** | Verifies SHA-256 package integrity, backs up existing installation binaries, swaps atomically, verifies startup via `--update-healthcheck`, and automatically rolls back if the new build fails to start. |

---

## 2. Modular Source Layout

```
src/
├── core/
│   ├── AppPaths.h/.cpp             # Strict separation of installDir vs userDataDir + path security
│   ├── Models.h/.cpp               # GameEntry, EmulatorProfile, CollectionEntry, ThemeConfig, UpdateManifest
│   ├── Database.h/.cpp             # SQLite WAL database, schema migrations, play sessions, VACUUM INTO backups
│   └── SettingsManager.h/.cpp      # Themes, liquid glass tuning, audio, widgets, button mappings, display
├── library/
│   ├── MetadataService.h/.cpp      # Filename cleaner, local artwork discovery, custom art copy, SteamGridDB v2
│   └── LibraryManager.h/.cpp       # Game/collection CRUD, directory scanner, search/filter, backups, JSON export/import
├── emulators/
│   ├── EmulatorManager.h/.cpp      # Dedicated configurable Lichii profile + custom emulators + argument templates
│   └── GameLauncher.h/.cpp         # Native .exe & emulator launching, exit detection, playtime recording, errors
├── input/
│   └── ControllerManager.h/.cpp    # SDL3 Bluetooth/USB gamepad polling, rumble, battery, remapping, glyph switching
├── audio/
│   └── AudioManager.h/.cpp         # Procedural studio WAV SFX/ambient generator + custom sound pack & BGM player
├── system/
│   ├── SystemIntegration.h/.cpp    # Cross-platform system facade, clock, multi-monitor management
│   ├── SystemIntegration_win.cpp   # Windows 11 DWM, WASAPI audio/device switching, PDH GPU, CPU/RAM, Registry Run
│   └── SystemIntegration_mac.cpp   # macOS dev/test implementation (Mach CPU/RAM, CoreAudio/QMediaDevices)
├── updater/
│   ├── UpdateService.h/.cpp        # Manifest polling, semver compare, SHA-256 verification, updater handoff
│   └── updater_main.cpp            # Standalone AetherUpdater binary with backup, zip extraction, healthcheck & rollback
└── ui/qml/
    ├── Main.qml                    # Shell, dynamic background, nav chrome, global controller/keyboard routing, HUD, screensaver
    ├── components/                 # AmbientBackground, GlassPanel, FocusButton, GlyphBadge, GameCard
    ├── widgets/                    # WidgetBar (customizable clock, telemetry, volume, controller, Wi-Fi, BT, recent, stats)
    └── views/                      # HomeView, GameDetailView, EmulatorsView, LibraryManagerView, SettingsView, QuickMenuOverlay, LaunchOverlay
```

---

## 3. Data vs. Installation Separation (Zero Data Loss Guarantee)

| Location | Contents | Modified During Update? |
| :--- | :--- | :--- |
| **Install Directory** (e.g. `C:\Users\<You>\AppData\Local\Programs\AetherLauncher\`) | `AetherLauncher.exe`, `AetherUpdater.exe`, Qt 6 DLLs & QML plugins | **Yes** (backed up first to `updates/install_backups/v<ver>-<stamp>/`) |
| **User Data Directory** (`%LOCALAPPDATA%\AetherLauncher\`) | `database/library.sqlite`, `settings.json`, `artwork/covers/`, `artwork/backgrounds/`, `backups/`, `themes/`, `soundpacks/`, `cache/audio/` | **NEVER overwritten or deleted by updates** |

---

## 4. Automatic Development & Updating Workflow (ChatGPT / AI -> Windows 11)

Two supported pipelines work seamlessly with Aether Launcher's built-in `UpdateService` and `AetherUpdater.exe`:

### Option A (Recommended): GitHub Repository + GitHub Actions Auto-Release Pipeline
1. Host this source code in a GitHub repository (public or private).
2. Connect ChatGPT (via GitHub connector / Codex / Pull Request workflow) or your AI coding assistant to that repository.
3. Whenever you request a feature or fix and approve/merge the commit to `main`:
   - `.github/workflows/build-windows.yml` runs automatically on Microsoft's `windows-latest` MSVC runner.
   - Compiles `AetherLauncher.exe` and `AetherUpdater.exe`.
   - Runs the entire automated Catch2 unit test suite (`ctest`). If any test fails, the release is aborted automatically.
   - Packages `AetherLauncher-win64.zip`, computes its SHA-256 hash, generates `manifest.json`, and publishes a GitHub Release.
4. On your Windows 11 PC, configure **Settings → Updates & Rollback → Update Manifest Feed URL** once:
   ```
   https://github.com/<YOUR_USERNAME>/<YOUR_REPO>/releases/latest/download/manifest.json
   ```
5. On startup (or when clicking **Check for Updates**), Aether Launcher notifies you of the new version and release notes.
6. When you approve **Download & Install**:
   - Aether Launcher creates a fresh library/settings backup snapshot.
   - Downloads the zip package and verifies its **SHA-256 checksum**.
   - Hands off to `AetherUpdater.exe`, which backs up the current install directory, swaps binaries atomically, launches the new build with `--update-healthcheck`, and **automatically rolls back** if the new build fails to start.

### Option B: Local Windows 11 Development Companion (`tools/dev_companion.py`)
If you prefer building directly on your own Windows 11 PC without GitHub Actions:
1. Run on Windows 11:
   ```powershell
   python tools/dev_companion.py --repo C:\Dev\AetherLauncher --qt-dir C:\Qt\6.8.2\msvc2022_64
   ```
2. Set Aether Launcher's **Update Manifest Feed URL** to:
   ```
   http://127.0.0.1:47821/manifest.json
   ```
3. The companion watches your Git repo, pulls new commits, compiles, runs `ctest`, packages a SHA-256-verified zip, and serves it over loopback (`127.0.0.1`) only.
