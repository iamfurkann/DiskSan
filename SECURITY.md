# Security policy

DiskSan 0.4.0 is a read-only discovery, assessment, inspection-planning and non-executable simulation tool, with an optional desktop alpha. It does not erase, sanitize, format, discard, modify partitions, or certify erasure. Observed identities, plans, simulation steps and UI selections never authorize destructive operations.

## Supported versions

Security and reliability fixes target the latest maintained source and published release. Version 0.4.0 is under development; 0.3.0 is the published historical baseline recorded in `docs/BASELINE_0.3.0.md`. No long-term support promise is made. Current-version native Windows and desktop distribution acceptance remain separate gates.

## Reporting a vulnerability

Check the repository's [Security page](https://github.com/iamfurkann/DiskSan/security). If GitHub offers **Report a vulnerability**, use that private reporting channel. Availability depends on repository settings; this document does not assert that the feature is enabled. If it is unavailable, open a minimal public issue requesting a private reporting channel, without exploit details, disk identifiers, personal paths, credentials, or raw inventories. Wait for the maintainer to establish a private channel before sharing sensitive details.

Include the version, operating system/compiler, minimal synthetic reproduction, expected and observed behavior, and whether the issue crosses the read-only boundary. Do not test reports by writing to disk devices. The project does not promise a response deadline, bounty, signing, or notarization.

## Assessment trust boundary

Native metadata, strings, proc files and query responses are untrusted inputs.
A permission error, missing relationship or unknown capability never becomes
proof of inactivity, support or authorization. Observations are non-atomic;
identity verification expires at the end of collection. Saved JSON is diagnostic
only. Output may contain private device identifiers and paths; redact before
sharing. Linux proc/sysfs reads validate descriptor type and filesystem before
reading, and Windows volume handles request zero desired access. See the
[read-only query audit](docs/READ_ONLY_AUDIT_0.2.0.md) for exact limits and gaps.

## Protocol-query boundary in 0.3.0

Only explicit `capabilities --probe` can select the Linux fixed Identify Controller
provider. A protocol family or read-only descriptor alone is not a safety proof:
the exact request, constants, transfer direction, ancestry, length and lifecycle
are audited together in [the protocol allowlist](docs/PROTOCOL_QUERY_AUDIT_0.3.0.md).
Unknown request kinds are rejected before native access. There is no raw-command
interface, writable-handle retry, elevation or external-tool fallback.

The provider reads controller metadata, never storage user-data sectors. It
retains only bounded numeric fields and static diagnostic reasons. The existing
inventory can still contain private identifiers; redact complete CLI output
before sharing. Failed, unrecognized or unassociated replies establish no method
support. A successful Identify response does not establish sanitization access,
scope, media guarantees or authorization. Tests use synthetic native boundaries;
physical protocol acceptance requires explicit maintainer-selected hardware.

## Planning and desktop boundary in 0.4.0

The portable plan copies bounded evidence and retains no native handle. It accepts no saved-plan import and emits no executable method, command, token or authorization. A modeled step is not a submitted or completed hardware operation. The future service described in the [threat model](docs/THREAT_MODEL_0.4.0.md) is design-only.

The desktop launches only its configured absolute CLI, with fixed enumerated arguments and current inventory selectors. It requires the exact version, successful exit and validated bounded schema-2 response. Failed or superseded requests discard selected results; no shell, elevation or `--probe` route exists. Device text is plain text and raw stderr is not displayed. No inventory telemetry, export or persistent cache is implemented.

Executable pinning assumes trustworthy installation and shared-library ownership. A version response is not binary authenticity, and pathname checks cannot defeat an attacker who already controls the user's executable or Qt environment. Source-build alpha binaries are not signed deployment bundles. See the [desktop contract](docs/GUI_ARCHITECTURE_0.4.0.md) for resource limits and remaining release gates.
