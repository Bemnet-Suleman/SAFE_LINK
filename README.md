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
- [Dashboard Status and Local Run](#dashboard-status-and-local-run)
- [System Architecture](#system-architecture)
- [Hardware](#hardware)
- [Firmware](#firmware)
- [Enclosure](#enclosure)
- [Tech Stack](#tech-stack)
- [Repository Structure](#repository-structure)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Roadmap](#roadmap)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)
- [Licensing note](#licensing-note)

---

## Overview

SAFE_LINK is an embedded systems project combining a wearable device, firmware, and an organizer-facing dashboard into a single group-safety platform. It targets youth camps, school excursions, hiking and scouting groups, outdoor training programs, and similar settings where a few adults are responsible for many participants across open or obstructed terrain.

The system is built around one principle: convert raw telemetry (location, motion, connectivity, battery, heart rate) into a small set of operational states — **SAFE, WARNING, CRITICAL, UNKNOWN** — and only ask for an organizer's attention when something changes for the worse.

## Project Status

SAFE_LINK has not yet reached a working prototype. Design work is complete across all workstreams; the next step is building and testing hardware.

| Workstream | Status |
|---|---|
| Documentation | ✔ Done |
| Organizer dashboard | ✔ Done (first phase build) |
| Schematic | ✔ Done |
| Firmware (`src/`) | ✔ Done |
| PCB layout | ✔ Done |
| 3D enclosure design | ✔ Done |
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

## Dashboard Status and Local Run

The dashboard is the primary operator interface for SAFE_LINK. It is currently in a pre-prototype but functional web-app state and is designed to present the live operational picture for a group or camp while hiding raw telemetry behind a cleaner exception-based view.

### Current dashboard scope

The working dashboard includes:

- Overview with summary cards and live operational health
- Live participant roster and filtering
- Tactical map view with location overlays
- Alert or incident queue with acknowledge/resolve actions
- Analytics and telemetry trend views
- Device network status and connectivity tracking
- Historical telemetry replay and export UI
- System settings for operation state and session handling
- A feature coverage panel covering all 11 project requirements

This is a working dashboard shell for a safety operation, not a field-validated deployment system. It is still being refined as the project moves from concept toward a prototype.

### Run the dashboard locally

#### Prerequisites

- Node.js 20+
- pnpm
- A Supabase project with URL and keys configured

#### 1) Install workspace dependencies

```bash
cd dashboard/Organizer-Center/SAFELINK
pnpm install
```

#### 2) Configure environment variables

For the backend API server:

```bash
export SUPABASE_URL="https://your-project.supabase.co"
export SUPABASE_SERVICE_ROLE_KEY="your-service-role-key"
export SUPABASE_ANON_KEY="your-anon-key"
```

For the frontend dashboard:

```bash
export VITE_SUPABASE_URL="https://your-project.supabase.co"
export VITE_SUPABASE_ANON_KEY="your-anon-key"
```

#### 3) Start the backend

```bash
cd dashboard/Organizer-Center/SAFELINK/artifacts/api-server
pnpm dev
```

#### 4) Start the frontend

```bash
cd dashboard/Organizer-Center/SAFELINK/artifacts/safelink
pnpm dev
```

Then open the dashboard in a browser at the Vite address shown in the terminal, usually:

```text
http://localhost:5173
```

### Overall project status

The current stack is:

- Frontend: Vite + React + TypeScript
- Backend: Express API server
- Database/auth: Supabase
- Core goal: show operational safety status for participants and groups instead of raw military-style command tooling

The prototype still needs extra validation around real-world telemetry, environment configuration, generated API client regeneration, and end-to-end workflow checks against a live Supabase project.

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

The reference schematic in `hardware/` adapts a design originally created for a **military wearable**, retargeted here for civilian group/camp-safety use. See the [Licensing note](#licensing-note) below for reused hardware designs.

**Project constraint:** the wearable was designed without SMD parts, which meant through-hole components and ready-made breakout modules (SIM800L, NEO-6M, MPU6050, MAX30102, TP4056). The first plan was a single-layer board; the final carrier PCB is a **two-layer, 90 × 60 mm** board because the single-layer routing needed too many wire jumpers. Several trade-offs between size, parts availability and routability follow from this.

**Current design files** (in `hardware/`):

- Schematic: `Schematic_SAFELINK_carrier_2026-10-11`, split into three sheets: **MCU**, **Power** and **Peripherals**.
- PCB layout: carrier board exports dated 2026-10-18 (two layers).

| Component | Part | Function |
|---|---|---|
| Microcontroller | ATmega328P-PU / PN (DIP-28 in a socket), 8 MHz HC-49S crystal | Core processing and firmware execution (runs at 3.3 V, so 8 MHz) |
| Cellular connectivity | SIM800L breakout (2G GSM/GPRS) with FPC antenna | Sends telemetry/alerts to the backend over the cell network |
| GPS | u-blox NEO-6M breakout (patch antenna) | Location fix, powered through a BC337/BC327 high-side switch |
| Motion sensing | MPU6050 on a GY-521 board | Movement classification, fall-pattern detection |
| Heart rate | MAX30102 breakout (PPG), on a cable to the skin-side window | Supporting physiological signal only |
| Battery charging | TP4056 module with protection (USB-C input) | Li-ion charge management |
| Voltage regulation | MCP1700-3302E (TO-92) | 3.3 V rail for the MCU and sensors |
| Power switch | Toggle switch rated 3 A or more | Main on/off |
| Emergency input | Tactile SOS button, 10 kΩ pull-up, 100 nF debounce | Manual emergency trigger (wakes the MCU from sleep) |
| Local feedback | 12 mm active buzzer, coin vibration motor | Audible and haptic alert, driven by BC337 transistors |
| Wearable integrity | Two-wire strap loop with 1 MΩ pull-up | Detects the strap being opened |
| Programming header | 2×3 ISP header (USBasp) | Firmware flashing — set the USBasp jumper to 3.3 V |

**Notes:**

- The SIM800L requires an active SIM card with GPRS data enabled and the correct APN for your carrier to communicate with the backend. It sits on the battery rail (3.4–4.4 V) with bulk capacitors beside its header.
- The SIM800L cannot reliably do modern TLS, so the device is intended to send signed payloads over plain HTTP to the project's own ingestion endpoint.
- Fuse settings for the ATmega328P (external 8 MHz crystal, brown-out 2.7 V): `lfuse=0xFF hfuse=0xD9 efuse=0xFD`. Check them with a fuse calculator before burning.

## Firmware

The firmware lives in `src/` and is a PlatformIO project (Arduino framework) for the ATmega328P at 8 MHz.

- `pins.h` — pin map for the carrier board.
- `main.cpp` — startup, SOS and IMU interrupts, battery reading, GPS power switch, buzzer and vibration helpers, and SIM800L initialisation.
- `platformio.ini` — board `pro8MHzatmega328`, USBasp upload, fuse values, library list.

| Signal | Pin | Signal | Pin |
|---|---|---|---|
| SIM800L UART | PD0 / PD1 | Buzzer | PB1 |
| SIM800L reset | PD4 | Vibration motor | PB0 |
| IMU interrupt | PD2 | Status LED | PB2 |
| SOS button | PD3 | Battery sense | PC0 |
| GPS serial in | PD5 | USB-present sense | PC1 |
| GPS power enable | PD6 | Strap loop | PC2 |
| Heart-rate interrupt | PD7 | I2C (IMU, HR) | PC4 / PC5 |

## Enclosure

The enclosure design is in `enclosure/`: a base, a lid and a printed SOS plunger for the 90 × 60 mm carrier board, closed with four M2.5 screws. Three versions are provided:

| Version | Use |
|---|---|
| A | Armband: strap lugs on the front and back walls |
| B | Backpack strap / belt: two strap bars under the floor and a lanyard tab |
| C | Demo bench: large lid window and rubber feet |

Case size is about 105 × 75 × 28.4 mm (38 mm with the GPS dome). `SAFE_LINK_Enclosure_Design.pdf` has renders and dimensioned drawings; the STL files for each part and a full assembly are included for printing (PETG or ASA recommended).

## Tech Stack

- **Firmware:** C/C++ (Arduino framework on PlatformIO, AVR toolchain)
- **Simulation:** Proteus
- **Schematic & PCB:** EasyEDA
- **Enclosure:** STL models and PDF drawings
- **Version control:** Git / GitHub
- **Monitoring:** Web dashboard

## Repository Structure

```
SAFE_LINK/
│── src/          # Firmware source (PlatformIO / Arduino framework)
│── docs/         # Documentation, diagrams, the project proposal
│── hardware/     # Schematics and PCB layouts (EasyEDA)
│── simulation/   # Proteus simulation files
│── dashboard/    # React web app dashboard
│── enclosure/    # 3D enclosure design files
│── tests/        # Test scripts and validation records
│── README.md
│── LICENSE
│── .gitignore
```

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- A USBasp programmer (set to 3.3 V) or another AVR ISP programmer
- Optional: `avrdude` for flashing a built `.hex` directly
- [Proteus](https://www.labcenter.com/) and [EasyEDA](https://easyeda.com/) (or its desktop client) for simulation and hardware design work

### Clone the repository

```bash
git clone https://github.com/<your-org-or-username>/SAFE_LINK.git
cd SAFE_LINK
```

### Build the firmware

```bash
pio run
```

`platformio.ini` must sit in the project root with the sources in `src/`. The target is an 8 MHz ATmega328P; 16 MHz is out of spec at 3.3 V.

### Flash to the microcontroller (via USBasp)

```bash
pio run -t upload
```

Or flash a built hex with `avrdude`:

```bash
avrdude -c usbasp -p m328p -U flash:w:.pio/build/safelink/firmware.hex
```

### Simulation and hardware design

Open the relevant project files in `simulation/` (Proteus) and `hardware/` (EasyEDA) directly in their respective tools.

## Development Workflow

Work is organized by branch, merged into `main` via pull request:

| Branch | Scope |
|---|---|
| `hardware-dev` | Schematic and PCB design (EasyEDA) |
| `firmware-dev` | Firmware (PlatformIO) |
| `dashboard-dev` | Organizer dashboard (web) |
| `simulation` | Proteus simulation files |
| `enclosure` | 3D enclosure design |
| `docs` | Documentation and reports |

## Roadmap

### Phase 0 — Concept & Design *(complete)*
- [x] Problem definition, feature specification, and system design proposal
- [x] ✔ Firmware architecture and initial drivers
- [x] ✔ Reference schematic and component selection
- [x] ✔ PCB layout
- [x] ✔ 3D enclosure design
- [x] ✔ Organizer dashboard
- [x] ✔ Project documentation
- [x] ✔ Dashboard design and development

### Phase 1 — First Hardware Prototype *(next)*
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

The full system design — problem statement, all 11 features analyzed in detail, dashboard design, architecture, alert logic, risks, and testing plan — lives in `docs/` as the project proposal. Start there for the reasoning behind any design decision referenced in this README. Hardware drawings are in `hardware/` and the enclosure drawings are in `enclosure/`.

## Contributing

1. Branch from `main` using the scope table above (e.g. `firmware-dev/uart-gps-parser`).
2. Open a pull request into `main` with a clear description of what changed and why.
3. At least one review is required before merge (see branch protection settings).

## License

This project is licensed under the MIT License. See [`LICENSE`](LICENSE) for details.

## Licensing note

MIT covers the software and firmware in this repository. Hardware designs adapted from third-party reference schematics (see [Hardware](#hardware)) may carry separate terms.
