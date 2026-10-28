# DiskSan v0.3.0 - maintainer release checklist

This is a pre-release checklist, NOT evidence of passing gates. The maintainer decides when to check items and publish.

## Candidate handoff context

Engineering evidence is recorded separately in [acceptance](ACCEPTANCE_0.3.0.md),
[progress](PROGRESS_0.3.0.md) and the [protocol matrix](PROTOCOL_TEST_MATRIX_0.3.0.md).
These checkboxes remain maintainer sign-off items. They are not an assertion that
engineering work is unperformed, and local test success does not tick publication
or physical-hardware acceptance on the maintainer's behalf.

The candidate has one enabled protocol provider (Linux LN-01); Windows and macOS
decline native probing. Current native Windows/MSVC execution, final committed
source CI and physical-controller acceptance are NOT_RUN. MinGW compilation
alone cannot close the Windows gate. The Windows property-query fixture gate is
conditional on enabling WN-01; no such provider is included in this candidate.
Any later enablement needs a new audit and native validation before distribution.

After creating and pushing the reviewed commit, inspect every hosted job for
that exact SHA, including skipped cases. Rebuild/check candidate archives if any
source or packaged documentation changes; generate SHA256SUMS last. Record the
accepted run URL here before publication. No final run URL or tag exists yet.

## Safety and requirements

- [ ] Compare current checkout to the published v0.2.0 baseline and preserve existing changes.
- [ ] Review all ACTIVE native protocol query entries against exact installed kernel/SDK behavior and primary documentation.
- [ ] Verify that neither CLI nor tests provide a generic pass-through interface or any storage mutation.
- [ ] Confirm `capabilities` defaults to metadata-only and real protocol probes require explicit `--probe`.
- [ ] Confirm all observed capability dimensions are independently scoped and unsupported/unknown/unavailable are differentiated.
- [ ] Confirm revalidation windows retain false erase authorization and future identity disclaimers.
- [ ] Confirm raw controller Identify/serial/host paths are not printed to CI or release logs by new code.

## Test gates

- [ ] Full available macOS Release and ASan/UBSan tests pass.
- [ ] Full Linux GCC Release and Clang ASan/UBSan tests pass.
- [ ] Windows MSVC build, deterministic query fixtures and available native integration tests pass.
- [ ] Negative forbidden-opcode/flag/error/cleanup/identity fixtures pass on intended toolchains.
- [ ] Physical NVMe probes are either explicitly tested on independently selected safe non-system devices OR marked NOT_RUN with public scope limits; no implicit live tests.
- [ ] Windows/macOS/ATA/SCSI unsupported paths are explicit instead of simulated support.
- [ ] `python3 tools/check_documentation.py`, `git diff --check`, and packaging smoke tests pass on actual final checkout.
- [ ] GitHub Actions jobs pass on the exact commit to be tagged; inspect skipped tests individually.

## Source and release materials

- [ ] `CMakeLists.txt` project version and `--version` report `0.3.0` on the final candidate.
- [ ] Update README, `docs/DEVICE_MODEL.md`, architecture/testing/security/roadmap, CHANGELOG and release notes to match *implemented* behavior rather than this plan.
- [ ] Ensure `docs/FILE_MANIFEST.txt` exactly matches current maintained files, with alphabetical ordering and no stale paths.
- [ ] Build final macOS/Linux/Windows artifacts from accepted source; include only allowlisted public files.
- [ ] Rebuild/check archive content and SHA-256 after *every* change affecting source or packaged docs.
- [ ] Check expected operating system/minimum runtime/platform constraints and optional signatures/notarization claims.
- [ ] Publish Release as `DiskSan v0.3.0 - Audited Read-only Capability Discovery`, tag `v0.3.0`, and mark pre-release if scope/hardware acceptance warrants it; publication remains a separate authorized maintainer action.
- [ ] Compare tag peeled SHA to the accepted CI SHA; check Release asset names, checksums and linked documentation.

## Handoff and known-issue disclosure

- [ ] State exactly which audited providers are active and which are unavailable, including tested OS/kernel/SDK hardware evidence.
- [ ] Explain that observed protocol support does NOT permit or guarantee actual sanitization, and the tool does not erase disks in v0.3.0.
- [ ] List all skipped/blocked native Windows/root/USB/RAID/APFS/ATA/SCSI cases without claiming success.
- [ ] Attach short reproducibility notes and the exact CI run URL; keep raw inventories and identifiers private.

A checkbox must only be ticked after the referenced evidence exists. Do not use the requested publication as authorization for commit/push/tag operations by Codex.
