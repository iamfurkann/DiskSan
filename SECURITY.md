# Security policy

DiskSan 0.1.x is an initial-development, read-only metadata discovery tool. It does not erase, sanitize, format, discard, modify partitions, or certify erasure. Observed identities and snapshots never authorize destructive operations.

## Supported versions

Security and reliability fixes target the latest published 0.1.x version. Version 0.1.1 is currently a prepared change set until its maintainer publishes a release. Older development builds should be replaced by the latest published version; no long-term support promise is made. Native Windows acceptance remains pending.

## Reporting a vulnerability

Check the repository's [Security page](https://github.com/iamfurkann/DiskSan/security). If GitHub offers **Report a vulnerability**, use that private reporting channel. Availability depends on repository settings; this document does not assert that the feature is enabled. If it is unavailable, open a minimal public issue requesting a private reporting channel, without exploit details, disk identifiers, personal paths, credentials, or raw inventories. Wait for the maintainer to establish a private channel before sharing sensitive details.

Include the version, operating system/compiler, minimal synthetic reproduction, expected and observed behavior, and whether the issue crosses the read-only boundary. Do not test reports by writing to disk devices. The project does not promise a response deadline, bounty, signing, or notarization.
