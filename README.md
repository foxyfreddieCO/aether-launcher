# Aether Launcher — Windows 11 Liquid Glass Gaming Console Launcher

A native C++20 / Qt 6 / SDL3 Windows 11 gaming launcher inspired by PlayStation 5 and Steam Big Picture Mode, designed for fullscreen Bluetooth/USB controller navigation with a liquid glass aesthetic.

## Key Capabilities
- **Liquid Glass Console UI**: Hardware-accelerated Qt Quick (Direct3D 11/12 on Windows 11) with layered translucent panels, GPU backdrop blur (`MultiEffect`), ambient light orbs, crossfading per-game dynamic backgrounds, 5 built-in themes + custom theme saving, and live opacity/blur/animation controls.
- **Centralized Game Library**: Supports Nintendo Switch 2 via **Lichii**, configurable emulator profiles (Dolphin, PCSX2, RPCS3, Cemu, RetroArch, custom), and standalone Windows `.exe` games. Includes directory scanning, filename cleaning, local artwork discovery, optional SteamGridDB online artwork fetching, custom artwork overrides, Favorites, Collections, Search, and Platform filtering.
- **Dedicated Configurable Lichii Integration**: Pre-created configurable profile for Lichii (Nintendo Switch 2) where you select your local executable, game directories, file extensions, and launch arguments (`{rom}`, `{rom_dir}`, `{rom_name}`, `{title}`). Never bundles copyrighted ROMs, firmware, encryption keys, or proprietary emulator files.
- **Full SDL3 Controller Support**: Bluetooth and USB Xbox & PlayStation (DualSense / DualShock 4) controllers with hotplug detection, automatic Xbox/PS/Keyboard button glyph switching, real battery status, hardware rumble, analog stick & D-pad navigation, and interactive button remapping.
- **Windows 11 System Integration & Customizable Widgets**: Live CPU/GPU/RAM telemetry, WASAPI system volume & audio output switching, Bluetooth/Wi-Fi shortcuts (`ms-settings:`), multi-monitor placement, startup-with-Windows registry toggle, performance HUD overlay, and ambient idle screensaver.
- **Sound & Atmosphere**: Built-in synthesized studio WAV SFX & ambient music pad, custom sound pack folders, custom BGM track support, and per-channel volume/mute controls.
- **Safe Auto-Updating & Rollback**: Automated GitHub Actions Windows 11 CI/CD + optional local companion (`tools/dev_companion.py`) + standalone `AetherUpdater.exe` with SHA-256 verification, automatic binary backup, startup healthcheck verification, and automatic rollback on failure. User data is strictly isolated in `%LOCALAPPDATA%\AetherLauncher\`.

---

## Default Controller & Keyboard Controls

| Action | Xbox Controller | PlayStation Controller | Keyboard |
| :--- | :--- | :--- | :--- |
| **Confirm / Select** | A | Cross | Enter |
| **Back / Cancel** | B | Circle | Esc |
| **Game Page / Details** | X | Square | Click / Enter |
| **Favorite / Game Options** | Y | Triangle | — |
| **Switch Library Section** | LB / RB | L1 / R1 | Q / E |
| **Cycle Collections** | LT / RT | L2 / R2 | `[` / `]` |
| **Open Quick Settings** | Start | Options | Tab |
| **Toggle Fullscreen** | Quick Menu | Quick Menu | F11 |

All gamepad mappings can be customized in **Settings → Controller & Input**.

---

## Building & Installing on Windows 11

### Prerequisites
1. **Visual Studio 2022** (or Build Tools for Visual Studio 2022) with the **Desktop development with C++** workload.
2. **CMake 3.24+** (`winget install Kitware.CMake`).
3. **Qt 6.8+** (MSVC 2022 64-bit) with `Qt Multimedia` and `Qt Shader Tools` installed via the [Qt Online Installer](https://www.qt.io/download-qt-installer-oss) or `aqtinstall`.
   *(SDL3 and Catch2 are fetched automatically by CMake if not already installed locally.)*

### Build, Test & Package (PowerShell)
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/msvc2022_64"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure

# Deploy standalone Windows 11 folder
New-Item -ItemType Directory -Force -Path "$env:LOCALAPPDATA\Programs\AetherLauncher" | Out-Null
Copy-Item build\Release\AetherLauncher.exe "$env:LOCALAPPDATA\Programs\AetherLauncher\"
Copy-Item build\Release\AetherUpdater.exe "$env:LOCALAPPDATA\Programs\AetherLauncher\"
C:\Qt\6.8.2\msvc2022_64\bin\windeployqt.exe --release --qmldir src\ui\qml "$env:LOCALAPPDATA\Programs\AetherLauncher\AetherLauncher.exe"
```

Launch `AetherLauncher.exe` (pass `--windowed` if you want to start in windowed mode).

---

## Configuring Lichii (Nintendo Switch 2)
1. Open **Emulators** from the top navigation bar (or Quick Menu).
2. On the **Lichii — Dedicated Switch 2 Integration** card:
   - Click **Browse...** next to **Executable Path** and choose your local Lichii `.exe`.
   - Adjust **Launch Arguments** if your Lichii build uses specific CLI flags (default is `"{rom}"`; supports `{rom}`, `{rom_dir}`, `{rom_name}`, `{title}`).
   - Click **+ Add Game Folder** and select your Nintendo Switch 2 game directory.
   - Click **Scan Folders** to automatically import supported game files into your library.

---

## Automatic Updates via GitHub Actions or Local Companion
See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the complete architecture overview and step-by-step setup of both the **GitHub Actions auto-release pipeline** (`.github/workflows/build-windows.yml`) and the **Local Windows 11 Dev Companion** (`tools/dev_companion.py`).
