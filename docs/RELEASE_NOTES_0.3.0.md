# DiskSan v0.3.0 - Audited Read-only Capability Discovery

Prepared candidate notes; publication remains a maintainer action.

DiskSan 0.3.0 adds explicitly requested NVMe controller capability discovery to
its C17 storage inspection CLI. On eligible Linux targets, it can read documented
controller support indicators and report their source while keeping unverified
access, applicability and passthrough states unknown. The interface runs in a
terminal and does not modify disk data.

## Highlights

- New `capabilities <selector> --probe` opt-in; ordinary `list`, `info`, `assess`
  and `capabilities` retain their metadata-only behavior.
- One audited Linux NVMe Identify Controller request, with fixed command fields,
  a 4096-byte response buffer, read-only access and no alternate command path.
- Direct namespace/controller association checks before opening, before querying
  and after the response, followed by the original selector's closing validation.
- Bounded, endian-safe decoding of reviewed NVMe 1.3.0, 1.4.0 and 2.0.0 layouts.
  SANICAP fields establish only NVMe block, crypto and overwrite support states.
- Additive schema-2 probe receipts distinguish declined, denied, failed, valid
  and malformed results without treating unavailable evidence as unsupported hardware.
- Expanded decoder, CLI, identity, native-request and resource-cleanup fixtures,
  including exact forbidden-command and read-only access checks.
- Explicit unavailable protocol providers on macOS and Windows; no invented
  ATA/SCSI support or USB bridge passthrough.

## Candidate downloads

| Platform | Asset | Protocol coverage |
| --- | --- | --- |
| macOS Apple Silicon (arm64) | `disksan-0.3.0-macos-arm64.zip` | Discovery/assessment; protocol query unavailable |
| Linux x86-64 | `disksan-0.3.0-linux-x86_64.tar.gz` | Opt-in fixed NVMe controller query; physical acceptance pending |
| Windows x86-64 | `disksan-0.3.0-windows-x86_64.zip` | Preview; protocol query unavailable, native 0.3.0 acceptance pending |
| Checksums | `SHA256SUMS` | SHA-256 digests for final candidate archives |

These names identify prepared distribution targets, not a claim of publication.
macOS targets 12.0; older supported releases were not individually tested. Linux
requires system `libudev.so.1` and glibc 2.34 or newer. Extract into a user-owned
directory and run `bin/disksan` or `bin/disksan.exe`.

## Usage

List devices normally, then deliberately select a non-system test namespace for
protocol acceptance:

```text
disksan list --all --json
disksan capabilities <selector> --json
disksan capabilities <selected-test-namespace> --probe --json
```

No environment variable or other subcommand enables probing. Raw opcodes, ioctls,
namespace IDs, timeouts and transfer lengths are not user-selectable. Access
denial does not trigger elevation or writable access.

## Validation

- macOS AppleClang Release and ASan/UBSan: 19/19 CTest groups passed in each build.
- Linux GCC Release and Clang ASan/UBSan: 24/24 groups passed in each build.
- 38 default CLI comparisons matched the 0.2.0 baseline after version normalization.
- 12,157 portable decoder assertions, 37,694 fixed-request assertions, and
  421 Linux lifecycle/failure cases exercised synthetic controller responses.
- Windows MinGW Release all-target cross-build passed; native 0.3.0 Windows
  execution remains pending. macOS and Windows compile the no-query adapter.
- Doxygen HTML passed with warnings treated as errors. Source manifest,
  English/ASCII documentation, API contracts, archive contents, tested binary
  bytes and checksums were checked; Mac/Linux relocated help/version passed.

Source snapshots, commands and remaining gates are recorded in
[the progress ledger](PROGRESS_0.3.0.md) and [acceptance matrix](ACCEPTANCE_0.3.0.md).
Synthetic tests verify request bytes and resource handling without querying
physical hardware. Historical 0.2.0 hosted CI does not validate this candidate;
final committed-source CI remains a release gate.

## Scope and known limitations

This release does not erase, sanitize, format, discard, modify partitions, access
storage user-data sectors, or generate erasure certificates. Advertised support
does not prove permission, scope, media outcomes or ability to execute a method.

Linux probing requires an independently observed generation and direct PCI NVMe
association. Bridges, fabrics, aggregate heads, observed layered targets and
incomplete mappings are declined. Unreviewed controller versions remain unknown.
The ioctl has no returned byte count; response-field checks cannot independently
prove truthful firmware content. Its timeout is a driver hint, not a guaranteed
wall-clock deadline.

Snapshots are non-atomic, expire at observation end and always retain
`erase_authorized: false`. Physical NVMe acceptance, hotplug, complex storage
topologies and native Windows validation remain separate open gates.

These packages are not presented as publisher-signed or notarized distributions.
Version 0.x is initial development and does not promise a stable public API.
License: Apache License 2.0.
