# Platform packages

The same source version produces three native archives. Each contains the platform CLI, public Markdown documentation, and the source manifest. Build logs, local evidence, and raw machine inventories are excluded from binary archives.

| Target | Filename | Status |
|---|---|---|
| macOS arm64 | disksan-0.4.0-macos-arm64.zip | CLI candidate; final 0.4.0 package gate tracked separately |
| Linux x86-64 | disksan-0.4.0-linux-x86_64.tar.gz | CLI candidate; final 0.4.0 package gate tracked separately |
| Windows x86-64 | disksan-0.4.0-windows-x86_64.zip | Preview awaiting native Windows acceptance |

CPack derives the version from PROJECT_VERSION and architecture from the actual CMake target (including explicit macOS architectures and MSVC x64). The Linux binary requires system libudev.so.1 and glibc 2.34 or newer; Debian 13 is the tested environment, not a guarantee for every distribution. The Mac archive contains arm64 only. macOS 12.0 is the deployment target; older supported OS versions have not been individually tested. Windows ARM64 is not covered.

## Create packages

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
ctest --test-dir build-release -C Release --output-on-failure
cpack --config build-release/CPackConfig.cmake -C Release -B dist
```

macOS ZIP and Linux tar.gz archives contain `bin/disksan`; Windows ZIP contains `bin/disksan.exe`. Public documents are under `share/doc/disksan`. Extract into a user-owned directory and run the binary there. Mac ZIP files can be extracted with Finder or `ditto -x -k`, preserving executable permissions. No system-wide installation or privilege escalation is required for extraction.

MinGW cross-build, with the toolchain installed or available on PATH:

```sh
cmake -S . -B build-windows -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-mingw.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-windows
cpack --config build-windows/CPackConfig.cmake -C Release -B dist
```

Do not run Windows test executables on the cross-build host. Native acceptance remains a separate step. This build compiles the explicit unavailable protocol adapter; it does not enable Windows Identify querying.

## Verify packages

Create SHA-256 checksums after producing the final archives. Verify archive contents, extract into a fresh directory, and run `--version`, `list --all --json`, and `info` for a path that actually appears in the inventory. A Linux package must resolve system libraries without a build-directory RPATH or a custom LD_LIBRARY_PATH. A cross-built Windows ZIP can be checked for CRC integrity and PE imports, but these checks do not prove runtime behavior.

On macOS, the delivered checksum file can be checked from the archive directory:

```sh
shasum -a 256 -c SHA256SUMS
```

## Native Windows acceptance

After extracting the preview, run in a normal PowerShell terminal:

```powershell
.\bin\disksan.exe --version
.\bin\disksan.exe --help
.\bin\disksan.exe list
.\bin\disksan.exe list --all --json | ConvertFrom-Json
$device = Read-Host 'Enter the exact path of a device shown above'
.\bin\disksan.exe info --json -- $device | ConvertFrom-Json
.\bin\disksan.exe plan --json -- $device | ConvertFrom-Json
.\bin\disksan.exe simulate --json -- $device | ConvertFrom-Json
```

Use a path shown by the list command. Compare capacity with Disk Management or Get-Disk, retaining permission warnings and null fields. Run `assess` and `capabilities` for the same listed selector, require literal false `erase_authorized`, preserve unknown method states, and check that an ordinary file is rejected by the target commands. For `plan` and `simulate`, check the process exit and require schema 2, `simulation_only: true`, `erase_authorized: false` and `execution_available: false`. Only simulation contains the ten modeled steps and `non_executable_model_only` result. These are metadata queries only; no erase/format command exists.

To run native MSVC tests and inspect individual skips from a Windows source checkout:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DDISKSAN_WERROR=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure -V
```

## Release policy

The canonical repository is [iamfurkann/DiskSan](https://github.com/iamfurkann/DiskSan). A future tagged release can expose three download assets plus SHA256SUMS. Local archives and source commits do not create such a release automatically. Do not describe the Windows preview as runtime-validated before native acceptance passes.

Packages include the repository's [Apache License 2.0](../LICENSE). Windows/macOS signing and macOS notarization remain release decisions. Current local CLI archives are not presented as signed/notarized distribution packages. See [testing](TESTING.md) and the [roadmap](ROADMAP.md) for validation limits.

## Historical 0.3.0 release handoff

The historical prepared title was `DiskSan v0.3.0 - Audited Read-only Capability Discovery`, with proposed tag `v0.3.0`. The maintainer subsequently published that release; see `BASELINE_0.3.0.md`. Preparation itself did not publish it. Review the [release notes](RELEASE_NOTES_0.3.0.md), [validation report](PROGRESS.md), and CI results on the actual committed revision before publishing. No final commit hash is assumed for an unstaged change set.

The `packaging_smoke` test invokes CPack into a fresh directory, checks the exact runtime/documentation allowlist and tested binary bytes, then runs the relocated executable with `--version`/`--help`. On Linux it removes LD_LIBRARY_PATH and requires an executable without a private build RPATH. The Windows cross archive is inspected with `--no-run`; only native Windows execution can close its preview gate. New artifacts include SECURITY.md and current public release/CI documentation. Source, logs, raw inventories, local paths, credentials, and build files are not runtime assets.

Generate SHA256SUMS only after the final archives are produced. If any packaged documentation or executable changes, rebuild that platform package and regenerate checksums. Validate the macOS executable bit, Linux runtime requirements, and Windows PE architecture/imports. Archives are unsigned and not notarized; old-OS compatibility has not been separately verified. Keep the three platform archives plus SHA256SUMS together in the release assets; GitHub-generated source archives are separate.

## Candidate provider coverage

The Linux archive enables only the fixed audited NVMe Identify Controller path
behind explicit `capabilities --probe`. macOS and Windows packages include the
same CLI/receipt contract with an unavailable protocol provider. No archive
provides ATA/SCSI/USB pass-through, a destructive backend or automatic probing.
Physical query acceptance is NOT_RUN unless separately recorded for deliberately
selected non-system hardware. See [current evidence](PROGRESS_0.4.0.md).

Candidate assets are generated locally. Their existence does not mean the dirty
working tree was committed, tagged, signed or published. Final hosted CI must
match the maintainer's reviewed commit; release hashes cover the final archive
bytes and are regenerated after documentation changes.

## 0.4.0 CLI and desktop separation

The 0.4.0 CLI adds inspection-only `plan` and `simulate`; successful output is not an erasure result. The archive layout and dependencies remain CLI-only. CPack installs only `disksan` and the public documents even when `DISKSAN_BUILD_DESKTOP=ON`. The exact-content package test rejects any extra GUI executable, Qt runtime, plugin or SDK file.

The Qt Quick alpha is a source-build deliverable using the matching build-tree CLI. Its macOS minimum is 14.4 for the tested Qt 6.12.0 SDK; it does not inherit the C CLI's 12.0 deployment target. No Windows GUI runtime, relocatable GUI bundle, Qt redistribution, signing or notarization is claimed. See [desktop build/licensing](GUI_ARCHITECTURE_0.4.0.md) for exact platform evidence and requirements. Separate GUI download assets are not part of this candidate.

Regenerate candidate CLI assets only after final source and public documentation settle. Record the uncommitted source snapshot hashes and base SHA during preparation, then replace that provenance with the maintainer-reviewed commit and matching hosted CI at release. Use the [0.4.0 checklist](RELEASE_CHECKLIST_0.4.0.md); neither source changes nor local packages authorize commit, tag, upload or publication.
