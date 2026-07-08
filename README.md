# Ballbot — Self-Balancing Truncated-Sphere Robot

A truncated-sphere robot that moves by shifting its center of gravity and using a reaction wheel, controlled remotely through a web interface on a phone. Built from scratch over a full academic year by a team of 6.

## Overview

The robot's shape is a truncated sphere. Movement isn't achieved with wheels or legs, but by:
- **Shifting its internal center of gravity** to lean and roll in a direction
- A **reaction/inertia wheel** to control orientation and stabilize the robot

The whole system is piloted remotely via a **web interface accessible from a phone**.

Team of 6, full academic year (design, build, and control from scratch).

## My Role

- Embedded control code (Arduino + ESP32)
- Motor selection and driving
- Piloting/control logic (web interface ↔ robot communication)
- **Closed-loop control (asservissement)** for balance and movement
- Full wiring and soldering

## Tech Stack

- **Hardware:** Arduino, ESP32
- **Software:** HTML (web control interface), embedded C/C++
- **Modeling/Simulation:** MATLAB (control system modeling and simulation, used to design and validate the closed-loop control before implementation)
- **Tools:** VS Code, Git
- **Skills:** Soldering, closed-loop control design

## Architecture

```
Phone (web interface, HTML)
        │  Wi-Fi
        ▼
     ESP32 ──► Arduino ──► Motors + Reaction wheel
        ▲
        └── Sensors (feedback for closed-loop control)
```

## Key Technical Points

- Designing a **closed-loop control system (asservissement)** to keep a spherical robot balanced and move it in a controlled direction using only a center-of-gravity shift + reaction wheel — no wheels/legs
- **Modeling and simulating the control system in MATLAB** before implementing it on hardware, to validate stability and tune parameters ahead of physical testing
- Wi-Fi communication between a phone-based web interface (ESP32) and the low-level motor control (Arduino)
- Mechanical design constraints of a truncated-sphere form factor

## Demo

<!-- Add photos/video of the robot moving + the web control interface -->

## Context

School project — ICAM, 3rd year engineering, group project (6 students, full year, built from scratch).
