<!--
SYNC IMPACT REPORT
==================
Version change: 1.2.0 → 1.3.0 (MINOR — Naming Conventions section revised again)
Ratified:       2026-04-19
Last Amended:   2026-04-21

Modified sections:
  - Naming Conventions: Revised to remove all `mav`/`Mav` prefixes from
    files, classes, and structs. Struct names now use `_t` suffix with
    snake_case members. Class names use bare PascalCase (e.g. `Bridge`,
    `Gcs`). File names use no prefix (e.g. `bridge.h`, `gcs.h`).
  - Migration Table: Updated all "Renamed to" targets to reflect no-prefix
    convention; `stMavEspParameters` → `parameters_t`.

Spec artifacts updated to match:
  ✅ specs/001-esp32-companion-computer/data-model.md  — struct/enum names + member names
  ✅ specs/001-esp32-companion-computer/plan.md        — file tree, migration table, module refs
  ✅ specs/001-esp32-companion-computer/contracts/http-api.md — struct type refs
  ✅ specs/001-esp32-companion-computer/research.md    — struct type refs

Templates checked (no naming-specific content — no updates required):
  ✅ .specify/templates/plan-template.md
  ✅ .specify/templates/spec-template.md
  ✅ .specify/templates/tasks-template.md

Deferred TODOs: none
-->

# ArduPilot ESP32 Webserver Constitution

## Core Principles

### I. Safety-First (NON-NEGOTIABLE)

No change to the MAVLink bridge, UART communication, or UDP forwarding logic may
be merged unless it has been verified to maintain transparent, lossless message
routing between the autopilot and ground control station. The bridge MUST remain
a passive relay — it MUST NOT alter, suppress, reorder, or inject MAVLink
messages without an explicit, documented, user-configurable reason. Any feature
that risks interrupting vehicle communication during flight MUST include a
hardware-tested verification step before merge.

**Rationale**: This firmware runs on a drone communication link. Failures are not
abstract — they can result in loss of vehicle control. Safety constraints
override convenience, schedule, and all other principles.

### II. Code Quality & Embedded Discipline

All C++ code MUST follow these non-negotiable embedded practices:

- Dynamic memory allocation (`new`, `malloc`, `String`) is PROHIBITED in any
  code path that executes at runtime after initialisation (setup loop excepted).
- Stack usage in ISR and RTOS task contexts MUST be bounded and documented.
- All public API functions MUST have a single, clearly documented return
  convention (error code, bool, or void — not mixed).
- Magic numbers MUST be replaced with named constants; all constants MUST be
  defined in the relevant header, not inline.

**Rationale**: ESP32-S3 has 512 KB SRAM (plus PSRAM). Heap fragmentation in
long-running embedded systems causes silent failures hours after deployment.

### III. Test Before Change

Every code change that modifies MAVLink parsing, HTTP endpoint behaviour,
parameter read/write, or PPM/PWM signal processing MUST be accompanied by a
verifiable test. Acceptable test forms in priority order:

1. Hardware-in-the-loop (HIL) test on physical ESP32-S3-CAM with a MAVLink
   simulator or real autopilot.
2. PlatformIO native unit test (`test/` directory) for pure-logic functions
   (parsing, CRC, parameter encoding).
3. Documented manual test procedure in the PR description covering the specific
   changed code path.

No new HTTP endpoint or MAVLink message handler MUST be shipped without at least
one acceptance scenario documented and executed.

**Rationale**: The hardware-dependent nature of this project makes automated
testing harder than typical software, but the safety stakes make testing more
important, not less.

### IV. Web UX Consistency

All web interface additions and changes MUST conform to:

- Consistent status indicator conventions: green = nominal, amber = degraded,
  red = fault — applied uniformly across all pages and widgets.
- All user-facing parameter names MUST match the MAVLink parameter IDs exactly
  (no aliases or renames in the UI layer).
- Response times: any web page or API endpoint that takes longer than 500 ms to
  respond MUST display a loading indicator; endpoints MUST NOT block the main
  loop for longer than 50 ms (use async or chunked responses).
- All web forms MUST validate input client-side and provide clear, plain-English
  error messages before submitting to the device.
- The web interface MUST remain functional when accessed from Mission Planner's
  embedded browser, QGroundControl, and a standard mobile browser.

**Rationale**: The web interface is used in the field, often on mobile devices
under time pressure. Inconsistency and unclear status indicators cause operator
error.

### V. Performance Budget

The following resource budgets are HARD LIMITS — exceeding them requires a
documented exception signed off before merge:

| Resource | Budget | Measurement method |
|---|---|---|
| MAVLink UART baud rate | 921600 baud sustained | Serial monitor, no packet loss at full rate |
| UDP forwarding latency | < 10 ms end-to-end | Wireshark capture during HIL test |
| Web server response time | < 500 ms for any endpoint | Browser DevTools network tab |
| Free heap at steady state | > 20 KB | `ESP.getFreeHeap()` logged every 60 s |
| Firmware binary size | < 1.5 MB | PlatformIO build output |
| WiFi AP connection time | < 5 s from power-on | Manual timing with stopwatch |

The MAVLink bridge loop MUST NOT be blocked by HTTP request handling. Web server
tasks MUST run at lower FreeRTOS priority than the MAVLink forwarding task.

**Rationale**: This device operates in real-time field conditions. Performance
regressions are not caught in the office — they manifest as dropped MAVLink
packets during flight.

## Naming Conventions

All identifiers introduced by this project MUST follow the rules below.
The migration table at the end of this section is authoritative.

### Files

- Source files use no project prefix: `<module>.h` / `<module>.cpp`.
  The legacy `mavesp8266_` prefix is removed entirely (e.g.
  `mavesp8266_gcs.h` → `gcs.h`).
- The root bridge module is named `bridge.h/.cpp` (was `mavesp8266.h/.cpp`).
- Header guards: `<MODULE>_H` matching the filename, e.g. `gcs.h`
  → `#ifndef GCS_H`.
- Exception: `main.cpp` retains its name (Arduino/PlatformIO entry-point
  requirement). Library files under `lib/` that are vendored third-party code
  are exempt from renaming.

### Classes

- Class names: `PascalCase`, no project prefix.
  The legacy verbose prefix `MavESP8266` is removed.
  Examples: `Bridge`, `Gcs`, `Vehicle`, `Component`, `Httpd`,
  `Parameters`, `Ppm`.

### Structs

- Struct names: `snake_case_t` suffix. Example: `parameters_t`,
  `telemetry_state_t`, `link_status_t`.
- Struct member variables: `snake_case` (no prefix). Example: `read_only`,
  `battery_voltage_mv`.

### Functions and Methods

- Public methods: `camelCase`. Example: `readMessage()`, `sendMessage()`,
  `handleMessage()`.
- Private methods: `_camelCase` (single leading underscore). Example:
  `_handleParamSet()`, `_sendRadioStatus()`.
- Free functions: `camelCase`.

### Variables

- Private member variables: `_snake_case`. Examples: `_udp_port`,
  `_last_status_time`.
- Local variables and function parameters: `snake_case`.

### Constants and Macros

- Compile-time constants (`#define`, `constexpr`, `enum` values):
  `UPPER_SNAKE_CASE`.
- Project-specific macros use a short module prefix where collision risk
  exists. Example: `WIFI_MODE_AP`, `VERSION_MAJOR`.
- MAVLink protocol constants (prefixed `MAV_`) come from the MAVLink library
  and are not renamed.

### Migration Table (existing non-conformant identifiers)

File renames, header guard updates, class renames, and struct renames MUST
be done in a dedicated refactor commit before functional changes.

| Current identifier | Renamed to | Location |
|---|---|---|
| `mavesp8266.h/.cpp` | `bridge.h/.cpp` | `src/` |
| `mavesp8266_component.h/.cpp` | `component.h/.cpp` | `src/` |
| `mavesp8266_gcs.h/.cpp` | `gcs.h/.cpp` | `src/` |
| `mavesp8266_vehicle.h/.cpp` | `vehicle.h/.cpp` | `src/` |
| `mavesp8266_parameters.h/.cpp` | `parameters.h/.cpp` | `src/` |
| `mavesp8266_httpd.h/.cpp` | `httpd.h/.cpp` | `src/` |
| `mavesp8266_ppm.h/.cpp` | `ppm.h/.cpp` | `src/` |
| `MAVESP8266_*` header guards | `*_H` | all renamed headers |
| `MAVESP8266_VERSION_*` macros | `VERSION_*` | `bridge.h` |
| `MavESP8266Bridge` | `Bridge` | `bridge.h` |
| `MavESP8266World` | `World` | `bridge.h` |
| `MavESP8266Component` | `Component` | `component.h` |
| `MavESP8266GCS` | `Gcs` | `gcs.h` |
| `MavESP8266Vehicle` | `Vehicle` | `vehicle.h` |
| `MavESP8266Parameters` | `Parameters` | `parameters.h` |
| `MavESP8266Httpd` | `Httpd` | `httpd.h` |
| `MavESP8266PPM` | `Ppm` | `ppm.h` |
| `MavESP8266Log` | `Log` | `bridge.h` |
| `stMavEspParameters` | `parameters_t` | `parameters.h` |
| `MAVESP_WIFI_MODE_*` | `WIFI_MODE_*` | `parameters.h` |

## Embedded Hardware Constraints

These constraints apply to ALL code targeting the ESP32-S3-CAM platform and
MUST be verified at each implementation step:

- **PSRAM**: Code MUST NOT assume PSRAM is available. Large buffers intended for
  PSRAM MUST use `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` with a fallback
  path if PSRAM is absent.
- **GPIO**: All GPIO pin assignments MUST reference the named constants in
  `variants/esp32s3-cam/pins_arduino.h` — no raw pin numbers in application
  code.
- **Watchdog**: The hardware watchdog MUST NOT be disabled. Long-running
  operations MUST call `yield()` or `esp_task_wdt_reset()` at appropriate
  intervals.
- **Flash partitions**: Any change to `partitions.csv` MUST be reviewed for
  impact on OTA update capability and SPIFFS/LittleFS storage.

## Development Workflow

Feature development MUST follow the Spec-Driven Development workflow:

1. **`/speckit.specify`** — define the feature in terms of user scenarios and
   acceptance criteria before any code is written.
2. **`/speckit.plan`** — produce a technical plan referencing the ESP32-S3-CAM
   hardware constraints and the performance budgets in Principle V.
3. **`/speckit.tasks`** — break into tasks ordered by dependency; safety-critical
   tasks (Principle I) MUST appear as blocking prerequisites.
4. **`/speckit.implement`** — implement with continuous reference to spec and
   plan; do not deviate without updating the spec first.

Each feature branch MUST have a corresponding Gitea issue. Branches MUST be
named `feature/###-short-description` where `###` matches the Gitea issue
number. PRs MUST link to their issue and MUST reference the spec artifact.

All implementation decisions that deviate from the plan MUST be documented in
the spec's `Notes` section before the PR is merged — not after.

## Governance

This constitution supersedes all prior ad-hoc coding practices and verbal
agreements for this project. In any conflict between this constitution and a
convenience or deadline, this constitution takes precedence.

**Amendment process**:

- PATCH (typos, clarifications): Any contributor may amend; bump patch version.
- MINOR (new principle or section): Requires documented rationale; bump minor
  version.
- MAJOR (removal or redefinition of existing principle): Requires explicit
  justification of why the old principle is no longer valid, plus a migration
  note for in-flight features; bump major version.

**Compliance review**: Every PR description MUST include a one-line statement
confirming which principles were checked (e.g., "Principles I, II, V verified").
PRs that omit this statement SHOULD be returned for revision before review.

**Runtime guidance**: Use `.github/copilot-instructions.md` for AI agent
runtime guidance and tooling configuration. Use this constitution for governing
principles that apply to all contributors and all agents equally.

**Version**: 1.3.0 | **Ratified**: 2026-04-19 | **Last Amended**: 2026-04-21
