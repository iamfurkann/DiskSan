# DiskSan v0.2.0 - Read-only Target Assessment

Status: unpublished implementation-complete candidate with explicit external gates; acceptance is tracked in
[PROGRESS.md](PROGRESS.md). No commit, tag or GitHub Release is created by these
notes. Windows remains a preview until native MSVC/runtime acceptance passes.

## What changes

- `assess <selector>` reports observed usage/topology hazards, incomplete scope,
  before/after identity checks and explanatory findings.
- `capabilities <selector>` reports the same target assessment with informational
  method evidence. Six method families retain independently unknown support,
  OS access, applicability and passthrough because no protocol probe is enabled.
- Linux maps mount/swap identifiers and observed loop backing names into a bounded
  parent/holder/slave graph. macOS observes registry/volume/mount metadata. Windows
  queries volume extents through zero-access handles and validates mount paths.
- Stable schema-2 assessment JSON separates original observations, derived
  findings and method evidence, always with `erase_authorized: false`.
- Additional fixtures cover policy conflicts, identity changes, parser bounds,
  native faults, cleanup, invalid selectors, JSON/terminal escaping and output
  failures. Existing inventory-output tests remain intact.

## Compatibility and limits

`list`, `info`, `--all`, old device fields, schema 2 and exit codes are preserved.
New commands return a separate operation envelope containing one `device` object.
Exit 0 means inspection completed, even when findings are blocked or require
review. Output failures may leave partial JSON; require both exit 0 and a complete
payload. See [DEVICE_MODEL.md](DEVICE_MODEL.md) for the exact contract.

Snapshots are non-atomic, logically expire at collection end, and never authorize
future erasure. Unknown scope is expected. APFS sharing, loop-held backing inode,
mount namespaces, hidden consumers, pagefiles, controller/namespace scope and
Windows storage layers cannot be exhaustively established. No physical erase
capability, NIST outcome or future destructive suitability is certified.

## Validation and release gates

Executed evidence is versioned by source snapshot in [PROGRESS.md](PROGRESS.md):
macOS Release/ASan/UBSan, Linux native GCC and cross-compilation are separate from
native Windows, physical hardware and hosted CI. Passing portable fixtures or
MinGW compilation cannot close those external gates. Review the
[query allowlist](READ_ONLY_AUDIT_0.2.0.md) before accepting the candidate.

Before publication, the maintainer should review the final source, run hosted CI
on the actual committed revision, complete or explicitly retain the Windows
preview gate, and regenerate each native package plus SHA256SUMS from final
sources. Package/document changes invalidate earlier smoke-archive checksums.

## Manual acceptance

From a normal terminal, build and run the test commands in [TESTING.md](TESTING.md).
Run `list --all --json`, then `info`, `assess --json` and `capabilities --json` with
a selector actually shown in that list. Require consistent current identity,
explicit scope limitations, unknown native method states, and false authorization.
A known root/system target should report its observable dependency as blocking.
An ordinary temporary file must be rejected with exit 3 and no success payload.
Do not disconnect, mount, format or modify a real device to exercise a failure.

Native Windows: use a normal PowerShell terminal, build with MSVC C17, run the
full CTest suite, then compare listed identities/capacities with Disk Management.
Preserve permission warnings and null/unknown values. See
[DISTRIBUTION.md](DISTRIBUTION.md) for extraction, package names and owner steps.

This release contains no erase/sanitize/format/discard backend, block-data read or
write, mount mutation, elevation flow, erasure certificate or signing claim.
