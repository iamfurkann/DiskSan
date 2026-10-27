# Device model and JSON schema 2

`list` and `info` use the same envelope. Successful `info` output contains one device. Schema 2 extends the unreleased Linux-only schema 1 draft with platform/native identity, nullable Unix numbers, and volume/snapshot roles.

## Envelope

`schema_version` is 2, `application` is `DiskSan`, `version` comes from the CMake project version, `mode` is `read-only`, and `snapshot` is true. `devices` contains records; `shown` counts included records and `omitted` counts filtered records. `warnings` contains sorted scan-level codes. Fatal diagnostics go to stderr. An output I/O failure may leave incomplete JSON and returns a nonzero exit status.

## Device fields

| Field | Type and meaning |
|---|---|
| `platform` | linux / macos / windows / unknown |
| `device_id` | Native snapshot identity, string/null; not permanent erase authorization |
| `path` | Native selector path, string/null |
| `sysfs_path` | Linux sysfs location; null on other platforms |
| `major`, `minor` | Unix device numbers; null on Windows |
| `model`, `vendor`, `serial`, `wwn`, `eui`, `nguid` | Reported metadata, string/null; uniqueness is not guaranteed |
| `parent`, `controller` | Observed native parent/controller relationship, string/null |
| `size_bytes` | Exact capacity in bytes, uint64/null |
| `logical_sector_size`, `physical_sector_size` | Positive integer/null, in bytes |
| `rotational`, `removable`, `read_only` | Reported boolean/null flags; `read_only` is a device flag, not application mode |
| `device_role` | disk / namespace / partition / virtual / composite / volume / snapshot / unknown |
| `transport` | nvme / ata / scsi / usb / virtio / mmc / unknown |
| `protocol` | nvme / ata / scsi / unknown |
| `media_type` | hdd / ssd / flash / unknown |
| `media_confidence` | heuristic / unknown |
| `classification_source` | platform_metadata, udev_ancestry_and_sysfs, iokit_and_diskarbitration, windows_setupapi, windows_storage_properties, or insufficient_evidence |
| `sources` | Evidence-source string/null for each of the 11 nullable text fields and 6 numeric/boolean fields |
| `stable_paths` | Linux by-id or Windows interface paths; permanence and uniqueness are not guaranteed |
| `holders`, `slaves` | Resolved Linux sysfs relationships; empty arrays on other platforms do not prove that no relationship exists |
| `warnings` | Sorted, deduplicated warning codes |

## Classification

Role precedence is snapshot, volume, partition, composite, virtual/virtio, namespace with sufficient NVMe evidence, disk, then unknown. The default list omits partition/volume/snapshot roles; `--all` includes them.

USB proves only the transport. SCSI access does not prove SAS cabling or the underlying native protocol. NVMe namespaces remain separate even when they share a controller or serial. An APFS container is composite, an APFS volume is a volume, and an APFS snapshot is a snapshot; none becomes a physical disk merely because it has a device node.

IOKit Medium Type may provide an SSD/HDD hint. Unknown transports such as Apple Fabric are not assumed to be NVMe. Windows keeps rotational metadata null and media unknown when no evidence exists. Brand or model names are not used to invent missing characteristics.

## Source precedence

Linux capacity/sector data prefers successful read-only ioctls, then sysfs. Model/vendor prefer sysfs, falling back to udev. Serial prefers sysfs/NVMe controller metadata, then `ID_SERIAL_SHORT`. WWN prefers `ID_WWN_WITH_EXTENSION`, then `ID_WWN`. Sysfs size is a count of 512-byte sectors converted with overflow checks.

macOS uses IOMedia size/flags and nearby Device/Protocol Characteristics. Disk Arbitration supplies missing model/vendor descriptions. Unreported physical sector sizes remain null. Native identity is the IOKit registry entry ID; the parent is the nearest observed IOMedia ancestor. Complete multi-device APFS/RAID topology is not claimed.

Windows uses SetupAPI interface identity, bounded STORAGE_DEVICE_DESCRIPTOR text, storage device numbers, and length/alignment queries. Read-access denial may fall back to zero-access queries. Unavailable capacity or sector data is never fabricated.

The first valid field value wins. Different later evidence produces `metadata_conflict`; bounded supplemental evidence retains the alternative source/value as described below. Serial is never a deduplication key. Unknown Unix numbers on Windows cannot merge unrelated records.

## Warnings and encoding

Representative codes include `metadata_conflict`, `classification_conflict`, `metadata_too_long`, `invalid_numeric_metadata`, `invalid_native_metadata`, `capacity_unavailable`, `sector_size_unavailable`, `physical_sector_size_unavailable`, `persistent_identity_unavailable`, and `identity_uniqueness_unverified`.

Linux adds context such as `devnode_permission_denied` and `ioctl_unsupported`; macOS may report `diskarbitration_description_unavailable` or `topology_is_registry_parent_only`; Windows may report `device_read_permission_denied`, `device_metadata_unavailable`, `device_query_failed`, or `read_only_flag_unavailable`. Consumers must retain unfamiliar warning codes because new codes may be added.

JSON preserves valid UTF-8 and replaces malformed bytes with U+FFFD. ASCII/C1 controls are escaped. Terminal output represents non-ASCII bytes and controls as `\xNN`. Metadata is limited to 4096 bytes; long identifiers are rejected instead of silently truncated into plausible identities. Tables additionally shorten displayed paths/models. JavaScript Number cannot represent every capacity above 2^53-1 exactly; consumers need suitable integer-aware JSON handling.

Snapshots are not atomic. Native ID, dev_t, serial, registry ID, and interface paths do not replace target revalidation for future destructive operations.

## 0.1.1 compatible additions

Schema remains 2. Existing field names/types, the read-only envelope, null semantics, and exit codes remain unchanged. Each device adds `evidence`, an array of `{field, source, value}` strings. Numeric/boolean observations use unsigned decimal strings (0/1 for booleans); primary numeric/boolean fields retain their original JSON types. Primary values and `sources` still identify the retained observation.

At most 64 deduplicated observations are retained per device, each component bounded by the 4096-byte metadata limit. A repeated source/value does not consume another slot. On overflow, `evidence_limit_reached` warns that supplemental observations were truncated; primary fields stay intact. Evidence is sorted by field/source/value. The array contains conflicting alternatives and corroborating observations from different sources; `metadata_conflict` distinguishes contradictory values. `duplicate_record_merged` identifies reconciliation separately from ordinary source disagreement. Detail output escapes evidence just like other metadata.

Matching native IDs can merge only with the same platform/role and consistent known device numbers. A known number/path collision with a different/missing native ID fails explicitly. A number missing from one otherwise matching record may be enriched with `identity_partially_verified`. Serial and controller are never identity keys. Records sort by path, then platform/native ID; supplemental arrays sort independently.

Linux `device_id` is `syspath#diskseq=N` when a valid generation is available, otherwise syspath with `identity_generation_unavailable`. This is a snapshot token, not a persistent device identity. Native ID strings were never a permanent identifier API. Windows keeps SetupAPI identity and null Unix numbers; macOS keeps registry identity and separate APFS roles.

USB plus independently observed native NVMe evidence yields transport `usb`, protocol `nvme`, role `disk`, and unknown media, with `usb_bridge_identity_and_passthrough_unverified`. A model name alone cannot set this evidence. USB with generic SCSI access still has unknown native protocol. Physical bridge passthrough was not validated by this change.

## 0.2.0 assessment envelope

`list` and `info` keep every existing field and type. `assess` and `capabilities`
use schema 2 with a separate operation envelope; they do not return `devices`,
`shown` or `omitted`. Both perform the same revalidation and policy assessment.
The `device` object uses exactly the inventory record schema above. Extra loop
backing links used internally for policy do not change that inventory object.

| Field | Contract |
|---|---|
| `operation` | `assess` or `capabilities` |
| `assessment_kind` | `target-preflight` or `capability-evidence` |
| `identity.status` | `verified_for_snapshot` only with positive identity consensus; otherwise `partially_verified` |
| `identity.guarantees_future_identity`, `identity.atomic` | Literal `false` |
| `identity.valid_until` | `end_of_observation`; diagnostic snapshots expire logically immediately |
| `inspection_status` | `blocked`, `review_required`, or `no_blockers_observed_in_snapshot`; none authorizes an action |
| `erase_authorized` | Literal `false`, including synthetic fully observed fixtures |
| `observations` | `{check, state, source, detail}`; state is the string `true`, `false` or `unknown` |
| `findings` | `{code, category, subject, severity, state, source, message}` |
| `capabilities` | Six method summaries and their independent source observations |

Finding severity is `block`, `review` or `info`. Categories include identity, role,
usage, topology, capability, platform_limit and observation_error. `subject` links
the finding to a check dimension or method, while `source` identifies derived
policy (`policy:observed_evidence`); original provenance is in observations and
method evidence. Stable codes include `mounted_dependency`, `system_dependency`,
`active_swap_dependency`, `logical_target`, `layered_backing`, `identity_unverified`,
`generation_unverified`, `topology_unverified`, `usage_unverified`, and
`snapshot_not_authorization`. Consumers must tolerate new codes and retain unknown
codes rather than silently discarding them.

Methods are `nvme_sanitize_block_erase`, `nvme_sanitize_crypto_erase`,
`nvme_sanitize_overwrite`, `ata_security_erase`, `ata_sanitize` and `scsi_sanitize`.
Each has `support`, `os_access`, `applicability`, `passthrough` and an `evidence`
array of `{dimension, state, source, detail}`. States are `observed_supported`,
`observed_unsupported`, `unknown`, and `unavailable`. These describe the respective
dimension only: support does not imply OS access or applicable scope. Conflicts
retain source observations and reduce summaries to unknown. Native 0.2.0 uses
`provider:metadata_only` and reports all four dimensions unknown for every method;
other states are exercised by synthetic fixtures, not hardware verification.

Evidence arrays are deterministic, exact duplicates are ignored, and conflicts
are retained. Each assessment permits 64 check and 64 method observations, with
source strings under 96 bytes and detail strings under 256 bytes. Invalid input
or overflow poisons evaluation; no success payload is emitted. Known identity,
generation or topology changes return exit 3; fatal collection/output errors
return nonzero. Output failure can leave partial bytes, so consumers must require
both exit 0 and complete valid JSON. Exit 0 with blocked/review findings means only
that inspection completed. Current native collectors always retain incomplete
usage/physical scope and never claim the fully observed synthetic policy state.
