# SAFE_LINK

**A wearable-based safety and accountability system for organized outdoor groups.**

![Status](https://img.shields.io/badge/status-pre--prototype-yellow)
![License](https://img.shields.io/badge/license-MIT-blue)
![Platform](https://img.shields.io/badge/platform-ATmega328P-green)

SAFE_LINK gives a small organizing team (camp staff, trip leaders, coaches) the ability to responsibly oversee a group far larger than manual headcounts and radios allow. Each participant wears a compact device; organizers watch a dashboard that surfaces **exceptions**, not raw sensor data.

> ⚠️ **Project status: pre-prototype.** No hardware unit has been built and tested end-to-end yet. Nothing described here has been field-validated. This is not a substitute for standard supervision or emergency procedures.

---

## Table of Contents
- [Overview](#overview)
- [Project Status](#project-status)
- [Key Features](#key-features)
- [System Architecture](#system-architecture)
- [Hardware](#hardware)
- [Tech Stack](#tech-stack)
- [Repository Structure](#repository-structure)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Roadmap](#roadmap)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

SAFE_LINK is an embedded systems project combining a wearable device, firmware, and an organizer-facing dashboard into a single group-safety platform. It targets youth camps, school excursions, hiking and scouting groups, outdoor training programs, and similar settings where a few adults are responsible for many participants across open or obstructed terrain.

The system is built around one principle: convert raw telemetry (location, motion, connectivity, battery, heart rate) into a small set of operational states — **SAFE, WARNING, CRITICAL, UNKNOWN** — and only ask for an organizer's attention when something changes for the worse.

## Project Status

SAFE_LINK has not yet reached a working prototype. The following workstreams are active **in parallel**:

| Workstream | Status |
|---|---|
| Organizer dashboard | 🔄 In progress |
| Firmware | 🔄 In progress |
| Schematic | 🔄 In progress |
| PCB layout | 🔄 In progress |
| 3D enclosure design | 🔄 In progress |
| Documentation | 🔄 In progress |
| Testing / prototyping | 🔄 In progress |

See [Roadmap](#roadmap) for how these workstreams lead to a finished product, not just a working prototype.

## Key Features

| # | Feature | Purpose |
|---|---|---|
| 1 | Participant identification | Ties every packet and alert to a specific person |
| 2 | SOS button | Manual emergency trigger, retried until acknowledged |
| 3 | Location & last-known-location | Live position, with a clearly labeled fallback when GPS is unavailable |
| 4 | Movement / fall detection | Classifies MOVING / STATIONARY / possible fall from IMU data |
| 5 | Heart rate (supporting signal only) | Never used as a medical diagnosis or proof of status |
| 6 | Battery monitoring | Flags low/critical battery before a device goes dark |
| 7 | Check-in / attendance | Roster-aware presence check at defined checkpoints |
| 8 | Connection status | Distinguishes "confirmed safe" from "unknown" — never conflates the two |
| 9 | Geofencing | Alerts when a participant leaves a defined safe zone |
| 10 | Group separation detection | Flags a participant drifting from their group's centroid |
| 11 | Wearable integrity / removal detection | Detects when a device comes off |

Full rationale, dashboard behavior, and technical implementation for each feature is in the project proposal — see [Documentation](#documentation).

## System Architecture

```
WEARABLE (sensors + MCU)
	↓
CELLULAR / GNSS LINK (SIM800L + NEO-6M)
	↓
BACKEND SERVER
	↓
ORGANIZER DASHBOARD
```

## Hardware

The reference schematic (`hardware/`) adapts a design originally created for a **military wearable**, retargeted here for civilian group/camp-safety use. See [Setting Up This Project](#setting-up-this-project) below (in the chat response) for a note on licensing reused hardware designs.

| Component | Part | Function |
|---|---|---|
| Microcontroller | ATmega328P-PN | Core processing and firmware execution |
| Cellular connectivity | SIM800L (GSM/GPRS) | Sends telemetry/alerts to the backend over the cell network |
| GPS | u-blox NEO-6M (via onboard header) | Location fix |
| Motion sensing | MPU6050 (6-axis accelerometer + gyroscope) | Movement classification, fall-pattern detection |
| Heart rate | MAX30102 (PPG) | Supporting physiological signal only |
| Battery charging | TP4056 (USB-C input) | Li-ion charge management |
| Voltage regulation | MCP1700-1802E | Regulated supply rail |
| Emergency input | Tactile SOS button + 1 kΩ pull resistor | Manual emergency trigger |
| Local feedback | Piezo buzzer | Audible alert |
| Antennas | 2× MMCX connectors | External GSM and GPS antennas |
| Programming header | USBasp / ISP | Firmware flashing |

**Note:** the SIM800L requires an active SIM card with SMS and/or GPRS data enabled, and correct APN configuration for your carrier, to communicate with the backend.

## Tech Stack

- **Firmware:** C, AVR-GCC toolchain
- **Simulation:** Proteus
- **Schematic & PCB:** EasyEDA
- **Enclosure:** 3D design software (CAD, TBD)
- **Version control:** Git / GitHub
- **Monitoring:** Web dashboard

## Repository Structure

```
SAFE_LINK/
│── src/          # Firmware source (C, AVR-GCC)
│── docs/         # Documentation, diagrams, the project proposal
│── hardware/     # Schematics and PCB layouts (EasyEDA)
│── simulation/   # Proteus simulation files
│── dashboard/   # React web app dashboard
│── enclosure/    # 3D enclosure design files
│── tests/        # Test scripts and validation records
│── README.md
│── LICENSE
│── .gitignore
```

## Getting Started

### Prerequisites

```bash
sudo apt install gcc-avr avr-libc avrdude
```

You'll also need [Proteus](https://www.labcenter.com/) and [EasyEDA](https://easyeda.com/) (or its desktop client) for simulation and hardware design work.

### Clone the repository

```bash
git clone https://github.com/<your-org-or-username>/SAFE_LINK.git
cd SAFE_LINK/src
```

### Build the firmware

```bash
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -o main.elf main.c
avr-objcopy -O ihex -R .eeprom main.elf main.hex
```

> Adjust `-DF_CPU` to match your board's actual clock source. As firmware grows to include GSM, GPS, IMU, and HR drivers, moving to a `Makefile` is strongly recommended over a single manual command.

### Flash to the microcontroller (via USBasp)

```bash
avrdude -c usbasp -p m328p -U flash:w:main.hex
```

### Simulation and hardware design

Open the relevant project files in `simulation/` (Proteus) and `hardware/` (EasyEDA) directly in their respective tools.

## Development Workflow

Work is organized by branch, merged into `main` via pull request:

| Branch | Scope |
|---|---|
| `hardware-dev` | Schematic and PCB design (EasyEDA) |
| `firmware-dev` | Firmware (AVR-GCC) |
| `dashboard-dev` | Organizer dashboard (web) |
| `simulation` | Proteus simulation files |
| `enclosure` | 3D enclosure design |
| `docs` | Documentation and reports |

## Roadmap

### Phase 0 — Concept & Design *(current)*
- [x] Problem definition, feature specification, and system design proposal
- [ ] 🔄 Firmware architecture and initial drivers
- [ ] 🔄 Reference schematic and component selection
- [ ] 🔄 PCB layout
- [ ] 🔄 3D enclosure design
- [ ] 🔄 Organizer dashboard
- [ ] 🔄 Project documentation
- [ ] 🔄 Dashboard design and development

### Phase 1 — First Hardware Prototype
- [ ] Bring up each subsystem individually (GPS fix, GSM link, IMU, HR sensor, charging)
- [ ] Assemble first integrated prototype unit
- [ ] Firmware MVP: sensor sampling, core alert logic, GSM reporting
- [ ] Minimal dashboard able to receive and display device status

### Phase 2 — Validation & Testing
- [ ] Bench and integration testing per subsystem
- [ ] Controlled trials: simulated SOS, fall, geofence, and separation events
- [ ] Communication range/reliability testing at a real venue
- [ ] Organizer usability testing of the dashboard

### Phase 3 — Pilot Deployment
- [ ] Small supervised pilot at an actual camp/trip, standard manual procedures kept in parallel
- [ ] Iterate on alert thresholds using pilot data

### Phase 4 — Production Readiness
- [ ] Design-for-manufacture pass on PCB and enclosure
- [ ] Finalize BOM and sourcing at production quantities
- [ ] Regulatory/compliance review (radio and safety standards, as applicable)
- [ ] Small-batch manufacturing run

### Phase 5 — Product Release
- [ ] Distribution to target organizations
- [ ] Ongoing support and maintenance process

## Documentation

The full system design — problem statement, all 11 features analyzed in detail, dashboard design, architecture, alert logic, risks, and testing plan — lives in `docs/` as the project proposal. Start there for the reasoning behind any design decision referenced in this README.

## Contributing

1. Branch from `main` using the scope table above (e.g. `firmware-dev/uart-gps-parser`).
2. Open a pull request into `main` with a clear description of what changed and why.
3. At least one review is required before merge (see branch protection settings).

## License

This project is licensed under the MIT License. See [`LICENSE`](LICENSE) for details.

*Note: MIT covers the software/firmware in this repository. Hardware designs adapted from third-party reference schematics may carry separate terms — see the licensing note in* [Setting Up This Project](#setting-up-this-project)*.*