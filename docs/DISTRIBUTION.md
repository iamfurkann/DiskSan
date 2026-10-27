# Platform packages

The same source version produces three native archives. Each contains the platform CLI, public Markdown documentation, and the source manifest. Build logs, local evidence, and raw machine inventories are excluded from binary archives.

| Target | Filename | Status |
|---|---|---|
| macOS arm64 | disksan-0.2.0-macos-arm64.zip | Candidate validated on a real Apple Silicon Mac |
| Linux x86-64 | disksan-0.2.0-linux-x86_64.tar.gz | Candidate validated on Debian 13 |
| Windows x86-64 | disksan-0.2.0-windows-x86_64.zip | Preview awaiting native Windows acceptance |

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

Do not run Windows test executables on the cross-build host. Native acceptance remains a separate step.

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
.\bin\disksan.exe info '\\.\PhysicalDrive0' --json | ConvertFrom-Json
```

Use a path shown by the list command. Compare capacity with Disk Management or Get-Disk, retaining permission warnings and null fields. Run `assess` and `capabilities` for the same listed selector, require literal false `erase_authorized`, preserve unknown method states, and check that an ordinary file is rejected by all three target commands. These are metadata queries only; no erase/format command exists.

To run tests from a native Windows source checkout:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Release policy

The canonical repository is [iamfurkann/DiskSan](https://github.com/iamfurkann/DiskSan). A future tagged release can expose three download assets plus SHA256SUMS. Local archives and source commits do not create such a release automatically. Do not describe the Windows preview as runtime-validated before native acceptance passes.

Packages include the repository's [Apache License 2.0](../LICENSE). Windows/macOS signing and macOS notarization remain release decisions. Current local CLI archives are not presented as signed/notarized distribution packages. See [testing](TESTING.md) and the [roadmap](ROADMAP.md) for validation limits.

## 0.2.0 release handoff

The prepared title is `DiskSan v0.2.0 - Read-only Target Assessment`; the proposed tag is `v0.2.0`. Neither is created by preparation. Review the [release notes](RELEASE_NOTES_0.2.0.md), [validation report](PROGRESS.md), and CI results on the actual committed revision before publishing. No final commit hash is assumed for an unstaged change set.

The `packaging_smoke` test invokes CPack into a fresh directory, checks the exact runtime/documentation allowlist and tested binary bytes, then runs the relocated executable with `--version`/`--help`. On Linux it removes LD_LIBRARY_PATH and requires an executable without a private build RPATH. The Windows cross archive is inspected with `--no-run`; only native Windows execution can close its preview gate. New artifacts include SECURITY.md and current public release/CI documentation. Source, logs, raw inventories, local paths, credentials, and build files are not runtime assets.

Generate SHA256SUMS only after the final archives are produced. If any packaged documentation or executable changes, rebuild that platform package and regenerate checksums. Validate the macOS executable bit, Linux runtime requirements, and Windows PE architecture/imports. Archives are unsigned and not notarized; old-OS compatibility has not been separately verified. Keep the three platform archives plus SHA256SUMS together in the release assets; GitHub-generated source archives are separate.
