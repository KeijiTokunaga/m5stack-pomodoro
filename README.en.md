# M5Stack StopWatch Pomodoro

[![日本語](docs/language-tabs/ja.svg)](README.md) [![English](docs/language-tabs/en-active.svg)](README.en.md)

![Pomodoro UI: stopped in blue-gray, focus running in coral, break running in mint](docs/pomodoro-preview.png)

UI illustration, not a device photograph. Left: stopped. Center: focus running. Right: break running.

## What it does

A standalone Pomodoro timer for the **M5Stack StopWatch C152**, featuring a 466×466 round AMOLED display and ESP32-S3. No Wi-Fi, account, or cloud service is required.

- Focus for **25 minutes**, then take a **5-minute short break**. Every fourth completed focus session earns a **15-minute long break**.
- Each interval ends with sound and vibration. The next interval waits for you to start it manually.
- Tap anywhere inside the large central panel, including the countdown digits, to start or pause.
- **Blue-gray = stopped or paused; coral = focus running; mint = break running.** RUNNING / STOPPED labels also communicate the state without relying on color.
- The outer ring shows remaining time, four dots show cycle progress, and DONE is the lifetime count of completed focus sessions. Skipped sessions do not count.

## Controls

| Input | Action |
| --- | --- |
| Tap the large central timer panel | Start / pause |
| Tap SOUND at the bottom | Toggle sound; vibration stays enabled |
| Yellow button A, short press | Start / pause |
| Yellow button A, hold | Skip the current interval |
| Blue button B, short press | Toggle sound |
| Blue button B, hold | Reset the current interval and stop |

Progress and sound settings are saved on interaction, at interval completion, and every 60 seconds while running. After reboot, the timer restores the last saved remaining time **paused**. Powered-off time is not counted; sudden power loss can lose up to approximately 60 seconds of progress.

## Build and flash

Install Python and PlatformIO Core, then run from this repository:

```sh
python3 -m pip install platformio==6.1.19
pio run
pio device list
pio run -t upload --upload-port /dev/cu.usbmodem1301
```

Replace the port with your device's port (for example, `COM3` on Windows). If flashing cannot connect, keep USB connected, hold the power button for about two seconds until the green LED turns on, then release it. If the app does not start after flashing, briefly press the power button once.

The firmware enables 16 MB flash and OPI PSRAM using M5Stack's documented PlatformIO configuration. M5Unified and M5GFX dependencies are pinned to specific commits.

## Tests and diagnostics

```sh
mkdir -p build
c++ -std=c++11 -Wall -Wextra -Werror -I include test/timer_test.cpp -o build/timer_test
./build/timer_test
pio device monitor --port /dev/cu.usbmodem1301 --baud 115200
```

The timer tests cover countdown, pause/resume, the four-session cycle, skipping, reset, clock rollover, and saved-state restoration. GitHub Actions builds the firmware and runs these tests.

USB serial commands: `?` reports status, `a` starts/pauses, `r` resets, `n` skips, and `m` toggles sound. These use the same actions as the device controls.

## Official references

- [StopWatch specifications, download mode and PlatformIO configuration](https://docs.m5stack.com/en/core/StopWatch)
- [Buttons and M5Unified](https://docs.m5stack.com/en/arduino/stopwatch/button)
- [M5Unified](https://github.com/m5stack/M5Unified)
- [M5GFX](https://github.com/m5stack/M5GFX)
