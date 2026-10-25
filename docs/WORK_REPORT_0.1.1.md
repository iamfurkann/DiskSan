# DiskSan 0.1.1 validation report

## Scope and baseline

This is an unstaged reliability/release hardening change set based on main commit `55847dad8f32d81684526c3bb777ec8b682bd7f2`. The working tree was initially clean; remote main matched it and the owner's v0.1.0 prerelease was already published. No user changes were overwritten. Only dedicated DiskSan directories were used. No Git stage, commit, push, tag, release publication, repository-setting change, or workflow dispatch was performed.

The prior 0.1.0 source passed a fresh macOS Release baseline at 5/5 CTest groups and a Linux GCC Release baseline at 7/7. macOS native APIs initially failed inside the application sandbox; the explicitly permitted unsandboxed read-only run passed. Baseline logs are retained privately. Historical evidence remains in [WORK_REPORT.md](WORK_REPORT.md).

## Findings and implementation

| Finding | Correction | Regression evidence |
|---|---|---|
| Same native ID or dev_t silently discarded later metadata; contradictory IDs could disappear | Transactional native-identity/role reconciliation; explicit ambiguity failure; preserve aliases, relationships, warnings, and bounded source observations | Collision/namespace/Windows unknown-number fixtures, 38 merge allocation failure points, ownership and content assertions |
| Changed Linux opened identity only warned and allowed collection to continue | Stop the record on stale/disappeared handles; distinguish partial and total scans; include valid diskseq in native snapshot identity | Syscall wrappers, first/last record loss, reused number, six generation/dev_t fixtures |
| Windows descriptor offsets could reference the fixed header; path conversion conflated malformed/oversized metadata with allocation failure | Bounded descriptor/UTF-16 validation before conversion; check fixed response sizes and disk/partition identity; retain explicit skip warnings | 22 portable buffer assertions and Windows cross compilation; native API/handle acceptance remains pending |
| Contradictory-looking USB/NVMe flags discarded independent transport evidence | Preserve USB with independently supplied NVMe protocol evidence while leaving media/passthrough unverified | Updated classification fixture; no physical bridge validation claimed |
| A Linux executable changed during CPack install because of build RPATH removal | Build with the empty install RPATH from the start | Exact executable-byte comparison and relocated package version/help without LD_LIBRARY_PATH |
| Cross-platform checks lacked repository automation and explicit release controls | Four push/PR CI jobs, test labels, package allowlist, security policy, release handoff | Parsed YAML; local tests; hosted run not yet observed |

The merge rejects ambiguity rather than publishing competing records that could make info selectors appear unique. Missing native identity with a shared path/number does not establish sameness. This is intentionally conservative for multipath/unknown topology. Source precedence still keeps the first valid adapter observation; supplemental evidence exposes conflicts instead of inventing a universally trusted source priority.

## Verification matrix

| Environment | Executed evidence | Decision |
|---|---|---|
| macOS arm64, Darwin 27, AppleClang 21 | Release and ASan/UBSan, 8/8 CTest groups; live APFS/metadata, packaging, static analysis | PASS_WITH_LIMITATIONS |
| Debian 13.6 x86-64, GCC 14.2 | Release, 11/11 CTest groups; sysfs/lsblk/native info and packaging | PASS_WITH_LIMITATIONS |
| Debian 13.6 x86-64, Clang 19.1 | Debug ASan/UBSan with leak detection, 11/11 groups | PASS_WITH_LIMITATIONS |
| Windows x86-64, MinGW GCC 14 | Application and C test targets cross-compiled; native execution not performed | PASS_WITH_LIMITATIONS for compilation only |
| Native Windows / MSVC | Workflow and test commands prepared; no accessible Windows execution host | BLOCKED: NATIVE_WINDOWS_ACCEPTANCE_PENDING |
| GitHub Actions | Workflow syntax parsed locally; no hosted run on this change set | PENDING_OWNER_PUSH |

Each native run has one expected CLI skip for the unsupported-host-only case. The five Linux integration tests and five macOS integration tests ran without hardware/topology skips on the available hosts. All executed sanitizer checks completed without sanitizer findings. Test counts and exact reproduction commands are documented in [TESTING.md](TESTING.md). The final allocation suite includes relationship/sector/identifier conflict provenance in addition to the exhaustive merge failure sweep.

Static analysis covers six macOS/shared source files and eight Linux/shared files. It is additional evidence, not proof of absence of bugs. Doxygen rendering and the manifest/ASCII/API documentation gate are part of the final preparation checks. Portable Windows metadata fixtures exercise pure buffer logic; they do not simulate every SetupAPI/DeviceIoControl allocation/handle path.

## Read-only audit and transfer

Linux read-only tracing checks device opens, the three allowed ioctls, and absence of device data reads/writes. macOS uses IOKit/Disk Arbitration without raw opens; Windows retains GENERIC_READ or zero access and the existing query allowlist. Negative CLI forms continue returning usage errors without discovery. No disk erasure, formatting, mounting, partition changes, raw-data reads, or block-device writes were used in development or tests. Synthetic sysfs tests write ordinary temporary files only.

Source transfers use an explicit NUL-separated maintained-file manifest, no --delete, and per-file Mac md5/Linux md5sum comparisons. The verifier rejects mismatches, empty output, failed commands, and absent files. A new version-specific VM workspace holds all new source/build/evidence files; previously isolated DiskSan dependency binaries are reused without global installation. Private machine paths, transfer rows, and inventories remain outside the public tree.

## Distribution and remaining work

Version 0.1.1 comes from CMake; JSON schema remains 2 with additive evidence. Three architecture-specific archives and SHA256SUMS are prepared separately from source. Package checks allow only the tested executable, license, and public documentation, and do not publish a release. Windows remains a preview; macOS/Linux packages remain unsigned and not notarized. Old OS/runtime compatibility is not exhaustively tested.

No open observed P0 regression remains in the executed macOS/Linux checks. Native Windows acceptance and hosted CI are external release gates. P1 limitations include physical hotplug and USB/SATA/NVMe/SCSI hardware, real multipath/RAID/APFS topology, complete native API fault injection, and snapshot changes beyond observed generation/identity checks. Read-only capability/target-policy work belongs to 0.2.x; destructive backends require a separate future specification.

Suggested commit subject: `Harden discovery identity and release validation for v0.1.1`. Suggested tag/title are `v0.1.1` and `DiskSan v0.1.1 - Reliability & Release Hardening`. The owner reviews and creates the actual commit/tag, pushes, inspects CI, and publishes. No final commit identifier is fabricated for this unstaged tree.
