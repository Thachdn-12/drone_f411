from pathlib import Path

readme_content = """# Drone Firmware Project

## Overview
Custom quadcopter flight controller firmware using:

- STM32F411 Black Pill
- MPU6050 IMU
- BLHeli_S ESC
- ESP32 WiFi module

The project follows a hybrid realtime + event-driven architecture for building a modular and scalable drone firmware system.

---

## Main Features

- MPU6050 sensor reading
- PID stabilization loop
- ESC PWM control
- ESP32 WiFi communication
- Event queue + dispatcher
- Realtime scheduling

---

## Hardware Stack

- STM32F411
- MPU6050
- BLHeli_S 30A ESC
- 2205 2300KV Brushless Motors
- 3S LiPo Battery
- UBEC 5V

---

## Firmware Architecture

```text
Application Layer
    ↓
Service Layer
    ↓
Driver Layer
    ↓
Hardware



