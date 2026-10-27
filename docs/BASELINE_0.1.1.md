# DiskSan baseline - v0.1.1 to v0.2.0

> Evidence snapshot: GitHub `iamfurkann/DiskSan`, inspected 2026-10-26. This is a reference baseline, not proof of a tagged v0.1.1 release or of successful hosted CI. Inspect the local checkout again before editing.

## Source of truth and version state

- Repository: <https://github.com/iamfurkann/DiskSan>
- Observed `main` HEAD: `58d07c9a2f00f9a21c4e6c6a7ac96d9febe332bf` (fix binary stdin for Windows output fixtures).
- Previous initial implementation: `55847dad8f32d81684526c3bb777ec8b682bd7f2`.
- `CMakeLists.txt` declares version `0.1.1`, and `README.md`, `CHANGELOG.md`, `docs/ROADMAP.md`, and `docs/RELEASE_NOTES_0.1.1.md` describe that source version.
- At inspection time, GitHub listed a published `v0.1.0` prerelease, but the `v0.1.1` tag and GitHub Release endpoints returned 404. Treat the v0.1.1 release state as **unconfirmed/unpublished on GitHub** until the maintainer or fresh remote evidence establishes otherwise. Do not create tags or releases without permission.
- The project's Apache-2.0 license is already present.

## Existing functionality: preserve it

- Read-only C17 discovery/identification CLI for Linux, macOS, and Windows; `list`, `list --all`, `info <selector>`, `--json`, `--help`, and `--version`.
- Linux: libudev/sysfs and **only** `BLKGETSIZE64`, `BLKSSZGET`, `BLKPBSZGET` on `O_RDONLY` block descriptors in the current data probe; stale/replaced node handling and `diskseq` checks.
- macOS: IOKit and Disk Arbitration; APFS container, volume, and snapshot roles; no block data opens.
- Windows: SetupAPI and bounded storage queries, read-or-zero-access handles; Win32 UTF-16 CLI conversion. MSVC/native Windows acceptance is reported as outstanding in the versioned documents.
- Shared device model preserves `unknown` and JSON `null`; roles, transport, protocol, media, source provenance, warnings, and topology evidence are distinct.
- JSON schema version **2**, including additive `evidence` observations from v0.1.1, exact `uint64` capacity, nullable Unix numbers on Windows. Do not silently break established keys or exit statuses (0 success; 1 system/output; 2 usage; 3 invalid/changed selector; 4 permission).
- Discovery snapshots are *not atomic*, *not durable authorization*, and *not confirmation of hardware command support*.

## Existing v0.1.1 hardening: do not reimplement

- Transactional record merge: matching platform/native identity/role with consistent numbers; contradictory identity produces a distinct ambiguity error rather than dropping evidence; duplicate metadata/aliases/warnings preserved.
- Bounded source observations; `metadata_conflict`, merge-related warnings, `identity_partially_verified`, generation-unavailable warnings.
- Linux collector split into `src/platform/linux/discovery.c`, `device_info.c`, `native.h`.
- Windows portable metadata helpers and tests; allocator-failure tests around reconciliation.
- GitHub Actions workflow `.github/workflows/ci.yml`; `SECURITY.md`, `docs/CI.md`, documentation-manifest checker, `tools/check_package.py`.

## Existing local verification, as reported by maintainers

- `docs/WORK_REPORT_0.1.1.md` reports Linux GCC Release and Clang ASan/UBSan **11/11 CTest groups**, macOS Release and sanitizer **8/8**, 38 transactional-merge allocation-failure points, and successful MinGW cross compilation.
- These are *repository claims*, not results independently reproduced while preparing this kit.
- Native Windows/MSVC execution, real USB/SATA/NVMe/SCSI/bridge/hotplug matrix, broad multipath/APFS/RAID topology, and hosted CI evidence remain subject to fresh verification.

## Known project files to inspect on demand

| Focus | Existing paths |
| --- | --- |
| Build/test selection | `CMakeLists.txt`, `.github/workflows/ci.yml`, `docs/CI.md` |
| Device model & ownership | `include/disksan/device.h`, `src/core/device.c`, `src/core/classification.c` |
| CLI/JSON | `src/main.c`, `src/output/output.c`, `docs/DEVICE_MODEL.md` |
| Linux | `src/platform/linux/{discovery.c,device_info.c,block_info.c,native.h}` |
| macOS | `src/platform/macos/discovery.c` |
| Windows | `src/platform/windows/{discovery.c,metadata.c,metadata.h}` |
| Tests | `tests/`, `tools/check_documentation.py`, `tools/check_package.py` |
| Safety/distribution | `SECURITY.md`, `docs/ARCHITECTURE.md`, `docs/TESTING.md`, `docs/DISTRIBUTION.md`, `docs/ROADMAP.md` |

## Baseline acceptance gate before implementation

1. Confirm current checkout, branch, HEAD, worktree state, version, available toolchains, and existing user changes. Never overwrite unrelated modifications.
2. Run available existing baseline build/tests before modifying behavior. Record command, platform, result, and any skips in `docs/PROGRESS.md`.
3. If HEAD differs from the snapshot here, use the actual checkout and reconcile changed API/logic before following the v0.2.0 specification. Do not reset history simply to match this snapshot.
4. Never infer unobserved physical-device support or that a CI workflow has run merely because its YAML exists.
