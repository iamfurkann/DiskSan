# DiskSan Desktop alpha - interface specification

## Direction

Professional, neutral, data-dense desktop UI: not a novelty mockup and not a false "one-click secure erase" interface. Use Qt Quick Controls, clear typography, predictable navigation, accessible names, keyboard interactions, light/dark-theme awareness and truthful semantic states. Avoid arbitrary animations during device inventory or timeout; an amber unknown state must not look successful.

## Screen 1: Overview

Header: "DiskSan" + version + "Read-only inspection" persistent indicator.

Sidebar: "Devices", "Assessment", "Simulation", "About & limitations". Hide or disable navigation when no selected device rather than showing fabricated values. Main panel shows: a Refresh action, inventory loading/error/empty states, observed devices with stable distinguishers (OS, role, path, model, capacity when known), and a right-hand summary of selection. The device count reflects actual parsed CLI data, not a hardcoded mock number.

Device list rows show unknown capacity as "Unknown", not zero; show system dependency if actually observed, otherwise "Not assessed" or "Unknown". Do not color disks green simply because not mounted. Users can copy necessary data locally, but do not auto-log or transmit identity metadata.

## Screen 2: Assessment

Device header includes current path/role/model with missing-value placeholders. Sections:
- Identity: snapshot-only, non-atomic, validity ends with observation.
- Observed risks: system/root, mount, swap, holders/slaves, layers, physical-scope uncertainty.
- Capability evidence: six method families and independent support, OS access, applicability and passthrough states. "Supported" is not a readiness badge; normal desktop does not trigger probing.
- Evidence details: provenance, source and short explanation, displayed as untrusted text. No links or HTML executed from device metadata.

Blocked, review_required and unknown are not treated as OS permission failures. A process error is distinguishable from a valid assessment showing a blocked device.

## Screen 3: Simulation

Permanent text: "SIMULATION ONLY - NO STORAGE OPERATION WILL RUN". A user can click "Generate inspection plan" and "Preview simulated steps". Show source snapshot limitations, observed blockers and each hypothetical step with a stable reason. Never show progress bars implying actual erasure, estimated wipe duration, completion certificate or a destructive execute button. Use distinct fixture-watermark "DEMO DATA" in offline demos. Provisional plan uses no actual method/opcode/sector range.

## Screen 4: About and limitations

Explain: v0.4.0 cannot erase disks; Linux NVMe opt-in capability query exists in CLI but desktop does not initiate it. Assessment cannot establish global exclusivity, host-wide mount state or controller scope. Tests/claims are platform-specific. Include build/license info for Qt desktop separately from CLI.

## UX acceptance

1. Start with no CLI: show understandable error and settings/help, no synthetic disk.
2. Start with valid CLI, empty inventory: show zero items, not "all clear".
3. One selected disk, switch selection during slow assessment: prior result is discarded.
4. Malformed JSON and nonzero exit: show error, never previous success.
5. Unknown capacity/serial and malformed UTF-8 data: show bounded safe placeholders.
6. Simulate mounted system disk: simulated model cannot imply safe or executed erase.
7. Keyboard navigation, screen reader names and high-contrast labels tested when Qt runtime is available.
