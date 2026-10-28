# DiskSan development roadmap toward v1.0.0

Status: PLANNING, not an implementation license or binding deadline. The public repository currently has a published v0.2.0 pre-release. In Semantic Versioning, v0.x denotes initial development and does not guarantee stable APIs. Version advancement is contingent on evidence and scope, not calendar dates.

## Versioned objectives

| Version | Proposed deliverable | Release gate |
| --- | --- | --- |
| v0.3.0 | Opt-in audited read-only native capability discovery; begin with Linux NVMe Identify Controller, optionally Windows property-query; explicit unsupported adapters | Closed request allowlist, synthetic security/decoding tests, correct independent capability semantics, preserved CLI/schema, available native CI |
| v0.4.0 | Inspect-only operation planning, threat/target authorization architecture, simulation and non-destructive dry runs | Independent plan review, stable target/identity semantics, no implicit execution |
| v0.5.0 | First narrowly scoped real NVMe sanitization prototype **only on expendable, explicitly authorized test hardware** | Independent safety review, positive target authorization, recovery design, power-loss/error cases, operator separation, no general release without real-device evidence |
| v0.6.0 | Broaden protocol/OS coverage conditionally (ATA/SCSI/native Windows when verified); reduce unobservable topology risks | Per-protocol and OS hardware matrix; unsupported combinations explicitly remain unavailable |
| v0.7.0 | Completion/verification of erase outcomes, failure recovery and machine-readable execution evidence | No false success after command submission; interrupted-operation and incomplete verification handling |
| v0.8.0 | Security hardening, reproducible packaging, signing and platform compatibility | External review, physical-device regression, stable error and artifact contracts |
| v0.9.0 | Feature freeze and release candidate for documented supported combinations | No known critical safety defects, user acceptance and incident/recovery rehearsals |
| v1.0.0 | First stable general release with a deliberately constrained, documented set of verified sanitization methods/platforms | All release criteria below, maintainer sign-off; unsupported hardware clearly excluded |

The tentative numbering can change when evidence changes. Do not ship unverified ATA/SCSI/bridge or destructive operations merely to satisfy a version table. An initial stable v1.0.0 MAY support a narrow set of devices/platforms rather than claiming every device type; it must still be safe, correct and usable for the advertised cases.

## Required v1.0.0 acceptance gates

1. **Safety**: explicit authorization and scoped target identification immediately before any irreversible action; protected system/boot/shared/virtual targets rejected according to documented policy; no confused deputy or stale-selection pathway; deterministic behavior for device removal/replacement.
2. **Protocol correctness**: per-family audited commands with exact privileges, implementation-specific scope, fixed I/O bounds, controller/namespace linkage, no unsupported fallback and real approved hardware evidence.
3. **Operation lifecycle**: cancellation/power loss/reboot/timeouts/hotplug cases handled as documented, with state recovery and no success claim without verified completion. Device-side cryptographic erase semantics are never guessed from nominal support.
4. **Verification and reporting**: distinguish commanded, accepted, in progress, completed, independently verified, partially verified, unverified, failed and interrupted outcomes. Published certificates or compliance assertions require separate reviewed methodology; avoid premature NIST conformity claims.
5. **Platform support**: repeatable dedicated real-hardware tests for every advertised OS/protocol/transport combination; no inference from CI virtual disks, emulators or cross compilation. Unsupported platforms/bridges are clearly excluded.
6. **Quality**: compiler-warning gates, sanitizer/fault tests, negative authorization tests, documented on-disk/device behavior, structured result contract, evidence review and dependency/security review.
7. **Distribution and user experience**: binaries/checksums tied to reviewed source and CI; appropriate signatures/notarization where practical; installation and safe dry-run documentation; operational warnings and support boundaries.
8. **Maintainer governance**: independently reviewed release notes and known limitations, threat-model review, explicit final publication authorization and post-release issue handling.

## Risk-based ordering

The riskiest work is not generating a protocol opcode; it is proving that the *correct physical target*, *correct scope*, *correct permissions*, and *correct completion status* are maintained under real-world faults. Keep native read-only discovery in v0.3.0 and simulation/planning in v0.4.0 before implementing any irreversible backend. Avoid combining a new experimental erase path with untested cross-platform UI changes.

## Release terminology

- `IMPLEMENTATION_COMPLETE`: the defined version's reachable requirements are implemented with observed test results.
- `RELEASE_CANDIDATE`: the maintainer has reviewed a proposed distributable revision and its explicit limitations.
- `STABLE v1.0.0`: the entire advertised supported configuration matrix meets the production acceptance gates, not merely a passing hosted test workflow.

No calendar date is promised for v1.0.0. The earliest defensible milestone is a post-v0.9.0 acceptance decision, conditional on hardware access, safety review and test evidence.
