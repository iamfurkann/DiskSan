# DiskSan v0.3.0 - observed published baseline for v0.4.0

Verification date: 2026-10-28 (Europe/Istanbul). Source: https://github.com/iamfurkann/DiskSan

## GitHub evidence

- Release: https://github.com/iamfurkann/DiskSan/releases/tag/v0.3.0, published 2026-10-28 15:19:44 UTC, marked pre-release.
- Annotated `v0.3.0` tag resolves to commit `02beaae641228864af161a17563a01d62850ad6b`, also the checked `main` HEAD at research time.
- Tag CI: https://github.com/iamfurkann/DiskSan/actions/runs/40714551432. The Linux GCC Release, Linux Clang ASan/UBSan, macOS AppleClang Release and Windows MSVC C17 Release jobs all completed with `success`. Windows native test output contains explicit skipped subtests. This is CI-runner evidence, not physical-disk coverage.
- Release assets: `disksan-0.3.0-linux-x86_64.tar.gz`, `disksan-0.3.0-macos-arm64.zip`, `disksan-0.3.0-windows-x86_64.zip`, and `SHA256SUMS`. GitHub supplied SHA-256 digests; archive-byte comparisons were not independently repeated during preparation of this kit.
- Annotated tag reports `unsigned`. Do not call it a cryptographically verified signed release.

## Implemented product behavior

- CMake project version 0.3.0; C17 core; CLI `list`, `info`, `assess`, `capabilities` with schema 2 JSON and documented exit codes.
- `capabilities <selector> --probe` is a separate explicit opt-in Linux NVMe Identify Controller query. The closed request uses opcode 0x06, CNS 1, NSID 0, 4096-byte response, and an audited read-only device path. Identity/namespace/controller association is rechecked; no generic pass-through is enabled.
- Three NVMe sanitize support bits can be inferred only from validated Identify data; authorization, access, applicability, bridge passthrough and completion remain unknown. Other methods and Windows/macOS protocol queries remain unavailable unless separately reviewed.
- No disk sanitization, format, discard, data-sector reading/writing, erasure certificate or GUI exists in this baseline.

## Verified / not verified

- CI builds and deterministic/synthetic test jobs passed on all four hosted environments. Local candidate evidence in the repository reports macOS 19/19 CTest groups in Release and sanitizer builds, Linux 24/24 groups for GCC/Clang sanitizer builds, plus a Windows MinGW cross-build; these are reported historical snapshot results, not newly rerun by this package.
- **NOT VERIFIED**: physical NVMe Identify execution on designated non-system hardware, physical SATA/USB/SCSI behavior, hotplug/ABA, complete RAID/APFS/multipath topology, all Windows physical system-volume dependencies, Linux native permission variability.
- The current release description still contains *candidate* and *publication pending* language after its actual publication; edit GitHub Release prose separately. Do not silently rewrite existing tag/archived acceptance documents to pretend their pre-publication timestamps were post-publication checks.

## Initial v0.4.0 compatibility freeze

Before coding, re-read the live checkout's `src/main.c`, `src/core/target.c`, `src/core/probe.c`, `src/output/assessment_output.c`, `include/disksan/assessment.h`, `include/disksan/protocol.h`, `CMakeLists.txt`, `docs/DEVICE_MODEL.md` and test targets. `docs/PROGRESS_0.3.0.md` contains a historical candidate ledger, not up-to-the-minute current validation. Do not modify v0.3.0's closed query allowlist as a side effect of planning or GUI work.
