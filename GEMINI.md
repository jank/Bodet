# Bodet / T&N Flip Clock Controller - Agent Context

## Project Overview
This project controls a restored T&N (Bodet) BT637 vintage flip clock using an ESP32 microcontroller. The original clock mechanism requires a polarized 12V impulse to advance the minute. This project replaces the original master clock signal with an ESP32-driven H-Bridge.

## Tech Stack
- **Framework:** Arduino
- **Platform:** PlatformIO
- **Language:** C++
- **Hardware:** ESP32 (NodeMCU/DevKit C), L298N H-Bridge, 12V Power Supply.

## Key Files
- **`src/main.cpp`**: Contains the core logic.
    - **`ClockDriver` Class**: Encapsulates motor control (H-Bridge) and pulse logic.
    - **`Logger` Class**: Handles logging with support for disabling via `#define`.
    - Connects to WiFi and NTP, with auto-reconnection in the main loop.
    - Manages Timezone (CET/CEST) implicitly via standard C library.
    - `last_displayed_time`: Tracks the physical time shown on the clock.
- **`platformio.ini`**: Configuration for `esp32dev` environment.
- **`Spare Tiles/`**: DXF design files for replacing missing clock tiles.

## Hardware Configuration
- **Motor Driver:** L298N Double H-Bridge.
- **Pinout:**
    - `enable_pin` (GPIO 21) -> ENA (H-Bridge)
    - `input1_pin` (GPIO 23) -> IN1 (H-Bridge)
    - `input2_pin` (GPIO 22) -> IN2 (H-Bridge)
- **Power:** 12V DC for the clock motor, regulated 5V for ESP32 (or via USB/onboard regulator).

## Logic Flow
1. **Setup:** Init serial (Logger), pins (ClockDriver), connect WiFi, sync NTP, set timezone.
2. **Main Loop:**
    - **WiFi Monitor:** Checks connectivity every loop. If lost, initiates a *non-blocking* background reconnect every 60s. The clock continues running on internal time.
    - **Time Check:** Get current system time (`now`) vs `last_displayed_time`.
    - **Catch Up:** If `now` is ahead, `clockDriver.advance()` and increment `last_displayed_time`.
    - **Wait:** If `now` is behind, do nothing.
    - **Pulse Duration:** Managed by `ClockDriver` (`PULSE_DURATION_MS`), enforcing motor safety.

## Current Status
- **Working:** Minute/Hour flipping, WiFi/NTP sync (non-blocking reconnect), DST handling, Motor protection, Class-based architecture.
- **Robustness:** Clock never stops; re-syncs automatically when WiFi returns.
- **Issues:** "Day of Month" mechanism (leap year handling) is not functional due to insufficient voltage/current at the secondary motor.
- **Mode:** Supports a `#define BARE_BONE_MODE` for testing hardware without WiFi.

## Instructions for Agents
- **Modifying Code:** Respect the pulse logic (`advanceMinuteTile`). Do not remove the delay without understanding the mechanical constraints (motor needs time to move).
- **WiFi Credentials:** Located in `src/main.cpp` (Placeholder `ssid`/`password` need to be set by the user or env vars).
- **Refactoring:** Keep the DST logic robust; mechanical clocks cannot "jump" instantly, they must physically flip 60 times to advance an hour.
