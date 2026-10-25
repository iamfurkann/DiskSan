# Continuous integration

The workflow runs on push and pull_request. Default permissions are `contents: read`, checkout credentials are not persisted, and no secrets, workflow_dispatch, pull_request_target, or publication steps are used. Dependencies are installed only on disposable GitHub runners; the tests themselves never require elevation.

| Job | Runner | Compiler/mode | Gates |
|---|---|---|---|
| Linux GCC Release | ubuntu-24.04 x64 | GCC, C17, Werror | portable, faults, native, packaging |
| Linux Clang ASan UBSan | ubuntu-24.04 x64 | Clang Debug, ASan/UBSan | portable, faults, native |
| macOS AppleClang Release | macos-15 arm64 | AppleClang, C17, Werror | portable, faults, native, packaging |
| Windows MSVC C17 Release | windows-2022 x64 | Visual Studio 17 2022, W4/WX | portable/faults, native, packaging |

The OS labels and Visual Studio image were checked against the official [runner catalog](https://github.com/actions/runner-images) and [Windows 2022 software list](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md). Runner patch versions change over time. `actions/checkout@v4` is an official maintained major reference; dependency installation uses the runner distribution repositories.

CTest labels make deterministic, native, and packaging results visible separately. The Unix fault step also includes portable allocation/buffer tests; duplicate execution is intentional for an explicit fault gate. A failed step remains blocking. No raw inventories or diagnostics are uploaded as artifacts. Archive checks run version/help only. Native checks can expose hardware-specific assertions in job logs; avoid adding identifiers to test diagnostics unnecessarily.

The macOS comparison brackets discovery with two diskutil plist snapshots. A stable reference mismatch fails. Only three consecutive measured reference topology changes can skip that integration class, with a reason in the log. API access failure is a hard failure, not a skip. CLI unsupported-host coverage skips when a native adapter exists. Linux missing by-id/device prerequisites and Windows unavailable device prerequisites are explicit test skips; inspect them before asserting hardware coverage.

Workflow YAML was parsed locally; local CTest results are reported separately. No hosted run for this unstaged change set has been observed. After the maintainer pushes, inspect all four jobs on the exact commit and record the run URL. A successful Windows compile does not replace real-device acceptance on the owner's Windows system. Branch protection, settings, secrets, or manual dispatch are not changed by this workflow preparation.
