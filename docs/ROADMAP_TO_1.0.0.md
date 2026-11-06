# DiskSan - evidence-based roadmap toward v1.0.0

Status: PLANNING. The observed published baseline as of 2026-10-28 is v0.3.0; it is a pre-release. The version table is a proposed sequence, not a schedule or authorization for destructive engineering.

| Version | Goal | Key gates |
|---|---|---|
| v0.3.0 (published) | Explicitly opt-in, audited Linux NVMe Identify Controller capability discovery | Tagged CI passed; real physical NVMe acceptance is still open |
| v0.4.0 (next) | Non-executable operation planning and simulation, threat architecture, optional Qt Quick desktop prototype | No erase path; truthful unknowns; real GUI read-only use where Qt is available |
| v0.5.0 | Separate independently reviewed experimental NVMe sanitization lifecycle on owned expendable hardware only if authorized | Operator authorization design, identified physical scope, recovery and negative tests; no general-device release without evidence |
| v0.6.0 | Expand verified protocol/OS combinations conditionally; DiskSan Desktop Alpha | Actual platform/hardware matrix and usable read-only UI; unsupported devices remain excluded |
| v0.7.0 | Truthful completion states, independent verification and interruption recovery | Submission is not success; power loss, reboot, hotplug and failure drills |
| v0.8.0 | Hardening, distribution, signing/notarization where practical; Desktop Beta | External review, physical regression, Qt licensing/deployment, security/UX tests |
| v0.9.0 | Feature freeze and scoped release candidate | No known critical safety flaws, demonstrated selected platform/method matrix |
| v1.0.0 (planned stable) | Stable CLI + desktop for explicitly supported, independently validated combinations | Production authorization/verification, repeatable hardware/OS acceptance and maintainer sign-off |

## Hard requirements for stable v1.0.0

1. Target scope and correctness: independent physical media/controller identity, protected system volumes, independent operator authorization and immediate pre-execution fail-closed checks on every advertised combination.
2. Method correctness: validated protocol commands with reviewed privileges, response bounds, transport semantics and real hardware evidence; no unsupported bridge assumptions.
3. Lifecycle and recovery: interrupted operations, failure, reboot, power loss, timeout and hotplug are handled without fabricated completion or unsafe retries.
4. Verification/report: distinguish requested, accepted, in progress, completed, verified, failed and unknown; results are not NIST/certification claims unless independently reviewed.
5. OS coverage: test each advertised Linux/macOS/Windows + transport + protocol + device class; narrow supported matrix is acceptable if clearly documented.
6. Quality: native compiler, sanitizer/fault/authorization negative suites, security review, reproducible auditable source-to-binary packages, transparent limits and trusted release distribution.
7. Usability: standalone CLI retains automation surface; desktop provides accurate, accessible device inspection and operator flows, with a separately audited privileged boundary if real operation controls are ever introduced.

If the GUI or physical hardware acceptance is incomplete by v0.9.0, defer stable release rather than declaring v1.0.0 by date. Desktop and CLI may have different OS minimums because Qt versions vary. A stable version may initially support only a small, explicitly documented set of devices/methods.
