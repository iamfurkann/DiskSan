# Testing

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure -V
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DDISKSAN_SANITIZERS=ON
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure -V
python3 tools/check_documentation.py
```

GCC/Clang use Wall, Wextra, Wpedantic, Wconversion, Wshadow, and Werror. MSVC uses C17 and W4/WX; native validation is pending. Python 3.9 or newer runs the test scripts. See [CI](CI.md) for runner definitions and separate portable/fault/integration/packaging gates.

## Existing test groups retained in 0.2.0

| CTest group | Coverage |
|---|---|
| documentation | Manifest, ASCII source policy, file/function briefs, public API contracts, Python docstrings, workflow file briefs |
| core | 236 assertions across 24 classification fixtures, parsing, normalization, native identity, distinct namespaces, and absent Unix numbers |
| identity_faults | 3224 assertions, 38 separate failing merge allocation points, unchanged ownership on failure, alias/relationship/source merge, collisions, bounded evidence, zero live core allocations |
| windows_metadata | 39 portable synthetic assertions for descriptor sizes/offsets/terminators and UTF-16/MULTI_SZ boundaries; no native Windows API execution |
| output_contract | 18 tests: v0.1.0 schema-2 golden compatibility, additive evidence, exact uint64/null, deterministic output, terminal controls, exact Unicode replacement/preservation |
| cli | 5 tests, including 17 invalid forms and an inherited read-only stdout file handle; unsupported-host case skips on native hosts |
| block_info_errors - Linux | 167 assertions: closed ioctl/open allowlist, permission/query failures, changed identity, disappearance, invalid sectors, and closure |
| discovery_errors - Linux | 85 assertions across 10 libudev/enumeration fixtures including first/last record loss, partial success, total failure, and reused device-number ambiguity |
| linux_metadata_faults | 78 assertions across 6 ordinary-file sysfs fixtures: valid/missing/invalid/changed diskseq, changed dev_t, and probe disappearance |
| linux_integration | 5 live sysfs/lsblk, partition/by-id/info, and invalid-selector tests |
| macos_integration | 5 diskutil/native identity/APFS/info/filter/invalid-selector tests with bracketed reference topology checks |
| windows_integration | 3 prepared real Windows identity/info/filter/file-rejection tests; runtime acceptance pending |
| packaging_smoke | Fresh CPack archive, exact public allowlist and executable bytes, relocated version/help; Linux removes custom library paths |

Fault wrappers never open actual block data. Core allocation substitution is limited to a separate test object, after native CRT declarations, and leaves production allocation unchanged.

## Evidence and scope

The [0.1.0 report](WORK_REPORT.md) preserves historical results. See [0.1.1 validation](WORK_REPORT_0.1.1.md) for baseline results; [current progress](PROGRESS.md) records 0.2.0 compiler/build results, skips, limitations, and release gates. A checked-in workflow is not a successful hosted run. Windows buffer fixtures and MinGW cross-compilation are not native Windows execution.

Mac tests use read-only diskutil only as a reference; production uses registry/Disk Arbitration APIs. Three measured reference topology changes may skip the comparison class; a stable mismatch, invalid JSON, or API failure fails. Other integration assertions retain their failure semantics. Linux missing device/by-id prerequisites are explicit skips. CLI output-error tests pass an inherited read-only file handle on each OS; they use no Unix-only preexec or invalid-fd assumption, but Windows execution still needs confirmation.

Physical USB/SATA/NVMe/SCSI, physical hotplug, multi-device APFS/RAID/multipath, complete IOKit/SetupAPI handle/allocation failure injection, and older OS versions are not covered by the local synthetic matrix. These remain explicit open work. No blanket skip/continue-on-error hides failures.

## Reproduction and transfer

Tests never invoke sudo, mount, format, or erasure. They may write ordinary fixture files within a unique temporary directory. Set TMPDIR inside an isolated workspace if desired. Linux dependencies can be extracted into a dedicated project directory; CI dependency installation affects only its disposable runner.

Transfer source through an explicit NUL-separated manifest, without --delete. Compare Mac md5 and Linux md5sum for every selected file before building, rejecting empty hashes, failed commands, missing files, and mismatches. These hashes establish equality, not authenticity. Package SHA-256 sums cover final archive bytes. Raw inventory and machine-specific logs remain private and are not archive assets.

Optional `doxygen Doxyfile` renders API documentation with warnings as errors. The Python documentation gate checks this repository's conventional C declarations and Python AST docstrings, not arbitrary C syntax or natural-language correctness. Workflow YAML needs its own syntax check. Review remains necessary.

## New assessment tests

| CTest group | Coverage |
|---|---|
| assessment_policy | 1,291 assertions: hazards, all three-state combinations, conflicts, roles, independent capability dimensions, poison-on-overflow and deterministic output |
| target_window | 617 assertions: stale/changed identity, generation, scope, discovery/usage errors, closing-selector reuse and cleanup |
| usage_parsers | 622 assertions: mountinfo/swaps numbers, escapes, NUL/control bytes, oversized/malformed records and empty outputs on failure |
| usage_graph | 181 assertions: reverse/shared-parent relationships, duplicate IDs/devnums, unresolved edges, changed membership, bounded graph |
| assessment_cli | 11 Python tests: help/options, exit classes, schema, unchanged device object, UTF-8/uint64, no success on failure, output stream failure, literal false authorization and capability conflicts |
| assessment_native | 2 read-only live tests: bounded target sample, alias selection and independently observable root dependency; missing prerequisites are explicit SKIP |
| linux_usage_faults | 22 synthetic scenarios: allowlisted paths/flags, proc/sysfs types, permissions, allocation/stream cleanup, malformed data, stat replacement and loop backing changes |
| windows_usage_faults | 16 prepared native-compiled synthetic cases: zero-access/IOCTL allowlist, malformed extent/UTF-16/MULTI_SZ responses, changed/lost closing query and handle cleanup; MinGW build PASS, execution NOT_RUN |
| macos_usage_faults | 474 assertions across 15 synthetic scenarios: CF ownership, missing/wrong descriptions, URL conversion, bounded mount counts/strings, calloc failure, node errors and changed identity |

New native assessment tests require no administrator elevation. Windows root
reference is an explicit SKIP pending owner's native acceptance; other Windows
assessment tests remain runnable from the source checkout. Do not treat a passing
CTest group with skipped prerequisites as the missing hardware/API evidence.

Linux test wrappers permit only the exact metadata paths and read-only flags;
macOS substitutions compile in a separate object, leaving production APIs intact.
The current 26,147-assertion Linux count includes byte-by-byte fixture construction and is
not a count of independent scenarios. Package tests are disposable smoke artifacts;
regenerate final release assets after documentation changes.
