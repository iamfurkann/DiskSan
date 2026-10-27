# Security policy

DiskSan 0.2.0 is a read-only metadata discovery and target-assessment tool. It does not erase, sanitize, format, discard, modify partitions, or certify erasure. Observed identities and snapshots never authorize destructive operations.

## Supported versions

Security and reliability fixes target the latest maintained source and published release. Version 0.2.0 is an unpublished development change set until its maintainer publishes it. Older development builds should be replaced by the latest published version; no long-term support promise is made. Native Windows acceptance remains pending.

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
