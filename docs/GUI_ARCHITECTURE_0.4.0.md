# DiskSan Desktop alpha - v0.4.0 architecture and build contract

## Decision

An **optional** Qt 6 Quick/QML desktop alpha demonstrates real disk inspection and simulation. It is NOT the future privileged erasure service. C17 portable core and CLI remain the single tested inspection authority. The desktop bridge is unprivileged and may consume only documented, bounded CLI JSON. Do not ship a fake GUI that simply opens a screenshot; include actual list, selection, assessment and simulation working against the compiled DiskSan CLI whenever Qt is available.

## Process boundary

```
Desktop UI (QML) -> C++ read-only ViewModel -> fixed QProcess invocation
                                                |
                                                v
                                 trusted DiskSan CLI binary
                                                |
                                                v
                               existing C17 read-only inspection
```

The bridge is responsible for schema validation, output limits, typed model conversion and clear errors. The QML layer must not construct CLI arguments, invoke arbitrary subprocesses, request elevation or issue raw native storage APIs. Version mismatch and unsupported JSON are errors, not fallback assumptions. The desktop never calls `capabilities --probe` automatically or through a convenience button in 0.4.0.

## Implemented file layout

```
desktop/
  CMakeLists.txt
  main.cpp
  backend/DeviceService.{h,cpp}
  backend/BoundedJson.{h,cpp}
  backend/CliContract.{h,cpp}
  qml/Main.qml
  qml/ReadOnlyLabel.qml
  qml/EvidenceList.qml
  tests/ ...
```

The application owns DeviceService until its single child has stopped. The QML engine owns the window and delegates. Service properties expose implicitly shared owned QVariant data; number tokens become decimal strings only after strict schema validation, preserving capacity through UINT64_MAX. No `double` conversion is used for integer identity or size. All work runs through the GUI event loop with asynchronous process signals; no synchronous process wait or nested event loop is added.

## Invocation contract

- CMake option: `DISKSAN_BUILD_DESKTOP=OFF` default. Build main `disksan` without Qt; desktop opt-in with `find_package(Qt6 ...)` and a version floor tested by the maintainer. Missing dependency with option ON is a clear configuration error. No mandatory Qt on CI default jobs.
- Resolve `disksan` via a known, verified absolute executable path (build target path in a build tree; configured installed path for packaged app). Do not use a PATH search. On supported OS, verify file exists and is executable and version compatible.
- Allowed argument arrays only: `--version`; `list --all --json`; `info --json -- <selector>`; `assess --json -- <selector>`; `capabilities --json -- <selector>`; `plan --json -- <selector>`; `simulate --json -- <selector>`. Options must appear before literal `--`; after `--` only the opaque selector is accepted. Whitelist commands via enum, never free-form strings. Prefer canonical option order verified by fixtures. Revisit argument order with the actual CLI parser rather than assuming.
- Pass selector as exactly one opaque argv element. It must be a selector returned from the current inventory (not a user-generated shell command). Defend against stale/replaced index and collisions; no trusting display text as identity.
- No shell, terminal interpreter, command-string assembly, `startCommand`, arbitrary external tools, network requests, updater, plugin loader, raw-device open, or elevated helper.
- Bound stdout/stderr separately (for example 2 MiB stdout, 32 KiB stderr), number of records, JSON nesting, decoding time and subprocess runtime. A per-request deadline may stop waiting but must NOT be described as a guarantee of device safety. Kill/cleanup stuck child; ignore late signals after request supersession or view destruction.
- Treat each subprocess stdout as untrusted: require exit code 0 AND complete valid UTF-8 JSON AND exact required envelope fields. Nonzero -> no success model even if partial JSON; malformed/truncated/too-large -> error. Missing nullable data remains unknown.
- Cancel/invalidate stale in-flight requests on Refresh, selection change and shutdown. Preserve a monotonically increasing request generation and selected native ID; never show stale assessments as belonging to newly selected disk.
- No persistent raw inventory or serial by default; export features not part of 0.4.0. Redact sensitive selectors from diagnostics (display only what is necessary on-screen for the user).

## Presentation semantics

Core data must always be visibly differentiated as: observed from live CLI, inferred policy review, hypothetical simulation, DEMO DATA fixture, unavailable, error, stale, and pending. Avoid green "safe" badges on disks lacking verified full physical/usage scope. `erase_authorized: false` is not a UI toggle; the UI has no destructive operation control.

## Qt dependency and licensing gate

Review the actually used Qt module licenses and required attributions, dynamic-link deployment and replacement requirements under LGPLv3 (or the maintainer's legitimate commercial license). Do not quietly bundle Qt libraries or assume Apache-2.0 of DiskSan changes Qt's obligations. Document GUI OS minimums for the *chosen Qt minor*; Qt 6.12 documentation currently lists macOS 14.4+ even though C CLI historically targets macOS 12.0. No unsupported compatibility claims.

References: https://doc.qt.io/qt-6/qprocess.html ; https://doc.qt.io/qt-6/supported-platforms.html ; https://doc.qt.io/qt-6/licensing.html .

## Concrete bridge limits and states

| Boundary | Implemented limit/behavior |
|---|---|
| Child lifecycle | One QProcess, at most three queued enum commands, 15-second deadline per child; cancellation closes read channels and kills asynchronously before replacement |
| Output | 2 MiB stdout; 32 KiB stderr; overflow discards results; raw stderr never enters visible messages |
| JSON | 16 levels, 32,768 nodes, 1,024 items per collection, 16 KiB decoded UTF-8 field, cooperative 500 ms parser deadline |
| Inventory | At most 256 devices; reject repeated native identity/path within a platform |
| Selector | Current validated inventory path, at most 4,096 UTF-8 bytes, no NUL; one argument after literal `--` |
| Acceptance | Exact compiled version, schema 2, read-only mode, known operation/fields/types, full payload and exit 0; reject any probe receipt |
| Authority | Require false authorization/execution/verification and fixed non-executable prerequisite/step states |
| Presentation | Disconnected, Loading, Ready, Empty, Error, Stale, Closing; `fresh` means current UI generation only |

Refresh clears the inventory and selection, then checks version and lists devices. Selection invalidates old results before info/assess/capabilities. Plan or simulation replaces all earlier selected responses and uses another CLI observation window. An identity or relevant state mismatch against the selected record discards the response. Generation matching prevents canceled or superseded callbacks from restoring old data. If a superseded child fails to start, the bridge posts the replacement to the next Qt event-loop turn so the old startup handler can release its process channels first. These checks do not create a live device lock or exclude all ABA changes.

The desktop launcher rejects every argument before constructing QGuiApplication, including Qt options that could change platform or plugin loading. The service constructor canonicalizes a known absolute executable location. Missing files can be installed at that same pinned location and retried; no settings field accepts another path. Each launch checks that the file is executable and still resolves to the pin. Binary/shared-library authenticity and the user's Qt environment are installation trust assumptions, not guarantees established by the version string. There is no supported relocatable GUI package in this release.

## Source-build and launch

The default CLI remains C17, CMake 3.20+, Python 3.9+ for tests and the native dependencies described in README. Opt-in desktop requires a C++17 compiler and Qt Core, Gui, Qml, Quick, QuickControls2; tests also require Qt Test. CMake accepts Qt 6.8+, but only 6.12.0 has current runtime evidence. Qt's own CMake/platform requirements also apply. An explicit ON with missing Qt is a configuration error.

On Linux, include the SDK's matching ICU runtime when selecting individual Qt archives; Qt 6.12.0's tested GCC SDK needs ICU 73, which may differ from the distribution's system ICU. Qt Gui also needs its platform OpenGL/EGL and related runtime dependencies even for the software/offscreen tests. Do not silently substitute ABI-incompatible library versions. The optional CI recipe includes the official ICU archive explicitly.

```sh
cmake -S . -B build-desktop -DCMAKE_BUILD_TYPE=Release \
  -DDISKSAN_BUILD_DESKTOP=ON -DCMAKE_PREFIX_PATH=/absolute/path/to/Qt/platform
cmake --build build-desktop --config Release
ctest --test-dir build-desktop -C Release -L desktop --output-on-failure -V
```

Linux executable: `build-desktop/desktop/disksan-desktop`. macOS executable: `build-desktop/desktop/disksan-desktop.app/Contents/MacOS/disksan-desktop`. The MSVC layout would be `build-desktop/desktop/Release/disksan-desktop.exe`, but Windows desktop execution is NOT_RUN. The CLI absolute target path is compiled into the app; keep that build and the Qt SDK available. Start as a normal user and click Refresh. There is no automatic startup discovery or elevation retry.

With BUILD_TESTING enabled, `disksan-desktop-fixture` is a separate executable pinned to the synthetic child. It permanently displays DEMO DATA. Production has no fixture switch or executable override. `test_desktop_ui` loads the same QML resources; screenshots are saved only by synthetic tests. `desktop_ui_native` uses real metadata without a probe and records no raw inventory.

| Desktop target | SDK/runtime evidence | Remaining scope |
|---|---|---|
| macOS arm64 | Qt 6.12.0; macOS 27.2 runtime; Cocoa and offscreen/software renderer executed | Deployment target 14.4, not tested on 14.4 or Intel; no signed/deployed bundle |
| Linux x86-64 | Qt 6.12.0 on Debian 13 with glibc 2.41 and ICU 73. GCC Release and Clang ASan/UBSan each passed 5/5 offscreen/software desktop groups, including native CLI inspection | Physical display, X11/Wayland and clean-machine deployment remain unverified |
| Windows x86-64 | NOT_RUN; no native Qt SDK/runner used | Native compiler, GUI, keyboard, deployment and runtime acceptance |
| Other Qt minors/architectures | NOT_RUN | CMake acceptance is not runtime validation |

Qt 6.12's documented macOS minimum is 14.4; desktop sets that target separately from the C CLI's 12.0. On the tested Linux SDK, ELF version records for Qt Core, Gui and Qml reference GLIBC 2.34 and GLIBCXX 3.4.29. These measurements cover selected linked libraries. Other plugins and deployment environments need their own dependency inventory and runtime test. See [Qt supported platforms](https://doc.qt.io/qt-6/supported-platforms.html).

## Dependency license review and distribution boundary

DiskSan-owned sources remain Apache-2.0. The current build dynamically links Qt; it does not redistribute SDK archives or invoke deployment tools. Module-level upstream terms reviewed on 2026-11-01:

| Component | Use | Upstream licensing options |
|---|---|---|
| [Core](https://doc.qt.io/qt-6/qtcore-index.html#licenses-and-attributions), [Gui](https://doc.qt.io/qt-6/qtgui-index.html#licenses-and-attributions) | QObject/processes/values/window and platform backend | Commercial, LGPLv3 or GPLv2 |
| [Qml](https://doc.qt.io/qt-6/qtqml-index.html#licenses-and-attributions), [Quick](https://doc.qt.io/qt-6/qtquick-index.html#licenses-and-attributions), [Quick Controls](https://doc.qt.io/qt-6/qtquickcontrols-index.html#license-and-attributions) | QML engine, visual items, Fusion controls and layouts | Commercial, LGPLv3 or GPLv2 |
| [Network](https://doc.qt.io/qt-6/qtnetwork-index.html#licenses-and-attributions), [OpenGL](https://doc.qt.io/qt-6/qtopengl-index.html#licenses-and-trademarks) | Actual linked Qt dependencies of Qml/Quick; DiskSan implements no network client or custom OpenGL operations | Commercial, LGPLv3 or GPLv2 |
| [Test](https://doc.qt.io/qt-6/qttest-index.html#licenses-and-attributions) | Test executables only | Commercial, LGPLv3 or GPLv2 |
| [SVG](https://doc.qt.io/qt-6/qtsvg-index.html#licenses-and-attributions) | Installed SDK plugin dependency; not a direct DiskSan target | Commercial, LGPLv3 or GPLv2 |
| [Shader Tools](https://doc.qt.io/qt-6/qtshadertools-index.html#licenses-and-attributions) | SDK/build support; no direct runtime API used | Runtime library: commercial, LGPLv3 or GPLv2; qsb tool: commercial or GPLv3 |

These alternatives do not relicense Qt or its third-party components as Apache-2.0. A later binary distribution needs an exact runtime/plugin inventory, corresponding Qt and third-party copyright/license notices, the applicable source or source-offer mechanism, and review of LGPLv3 modification/relinking, reverse-engineering and installation-information requirements (or a valid commercial license). Dynamic linking alone does not complete those obligations. Qt tools, documentation and some other modules have different terms; no blanket compatibility approval is claimed. Review the [Qt licensing guidance](https://doc.qt.io/qt-6/licensing.html) and [LGPLv3 text](https://www.gnu.org/licenses/lgpl-3.0.html) for the actual chosen distribution.

Dependency inspection of the macOS executable also confirms Qt's internal Qml/Quick support libraries and native graphics frameworks. The Linux SDK uses a separately supplied ICU runtime. These transitive libraries, platform plugins and their third-party notices belong in any future deployment inventory; the module table is not a substitute for a binary-distribution bill of materials.

CLI CPack archives deliberately install no GUI executable, Qt library, plugin or SDK. Desktop binary packaging, relocatable path selection, signing/notarization, clean-machine loading and redistribution approval remain explicit future release gates; source-build alpha is the 0.4.0 deliverable.
