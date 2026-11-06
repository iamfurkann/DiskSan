# DiskSan v0.4.0 - safety and authorization threat model (design only)

## Trust boundaries

1. Untrusted: native metadata, storage-device firmware responses, user-selected names, USB bridge behavior, proc/sysfs/registry output, CLI stdout/stderr when read by desktop, JSON fixture files, GUI user interactions.
2. Less trusted: device path/name/serial/capacity recorded earlier, non-atomic discovery snapshots, GUI cached selection, CI virtual disk layouts, Linux namespace mount observations, prior probe receipts.
3. Future privileged service (NOT PRESENT in 0.4.0): a separately reviewed operational trust boundary that must identify the live physical target and independent authorization. Do not implement a service stub that accepts an erase command or serialized "authorized" object.

## Top hazards and required treatment

- **Wrong target / TOCTOU / ABA**: paths and device numbers can be recycled, USB disks hotplug, controller and namespace identity change. Existing read-only before/after checks only support an observation. No saved plan can authorize a later action.
- **Wrong physical scope**: NVMe namespace vs shared controller, RAID/DM/multipath, APFS shared containers, storage spaces, bridge translation. Classifications and Identify support bits do not prove isolation of one media unit.
- **Live data dependence**: root/boot/EFI/swap/pagefile/hibernation, mounted filesystem, snapshots, holders, shared parent links, processes with raw handles. A hidden consumer is not proof of absence. A known hazard blocks; incomplete visibility requires review.
- **Untrusted capability facts**: controller may lie or return version-specific data; bridge passthrough not established; read-only Identify success is NOT a sanitize authorization or result.
- **Privilege escalation/confused deputy**: GUI cannot create authority by running as administrator/root. No elevated helper, shell, IPC erasure request, or device-handle passing in this version.
- **False assurance**: "simulation succeeded", green icons, exit 0, printed JSON or test fixture must not imply data removed. Explicit persistent simulation-only and no-certificate language.
- **Resource denial and malformed data**: bounded strings, arrays, JSON depth, output stream, Qt process time, native descriptors, allocation failures; results with truncation/errors are not positive assessments.
- **Sensitive information exposure**: model, serial, disk paths and telemetry; no automatic upload, logging of raw identify buffer or unrestricted diagnostic dump.

## Future authorization architecture (unimplemented)

A later dedicated operation service would require fresh protected-device detection, live physical controller/namespace scope, adequate privileges only after independently checked controls, multi-step explicit operator selection, machine-readable authorization request tied to current identity and ephemeral operation session, and fail-closed revalidation immediately before any irreversible command. This is conceptual and must not become an accepted token format now.

Another later service must separately design operation lifecycle, cancellation semantics (some sanitize commands cannot be cancelled), power-loss/reboot recovery, post-command verification and truthful evidence. No claim about NIST compliance, erasure certificates or physical media outcome may be generated from the current simulation.

### Assets, principals and present authority

The protected assets are existing user data, correct target attribution, the
integrity of inspection evidence and the operator's understanding of uncertainty.
The current principals are the ordinary user, the unprivileged CLI, the optional
desktop client, the operating system's observation APIs and untrusted storage
firmware. None of those principals receives a new destructive authority in 0.4.0.
A local process able to replace the installed executable or its shared libraries
already controls that user's software supply chain; a successful version string
alone is not executable authenticity. Desktop installation and binary ownership
remain separate trust assumptions, explicitly revisited before distribution.

The current C plan is a bounded value diagnosis. It copies source observations,
reconstructs findings, adds hypothetical prerequisites and retains no native
handle. It accepts no serialized plan input and emits no nonce, signature,
credential, executable command, method choice or sector range. Reusing this
diagnosis cannot bypass fresh native selection because no execution entry point
exists. An in-memory API invariant violation must fail, not silently truncate a
positive result.

### Boundary analysis and rejection conditions

| Boundary | Adversarial or mistaken input | Required present response | Remaining limitation |
| --- | --- | --- | --- |
| Firmware/kernel to inventory | False identity, misleading capacity, bridge translation | Preserve provenance and conflicts; never use serial/capacity as authority | A plausible observational response cannot prove truthful firmware |
| Selection to assessment | Reused path, changed generation/controller, ambiguous aliases | Existing closing selection fails without success JSON on observable mismatch | Matching endpoints cannot exclude all ABA events |
| Assessment to plan | Understated status, malformed bounds, forged derived labels, hidden logical role | Validate/copy raw observations, reconstruct findings and include snapshot hazards | Caller must provide the same observation-window snapshot; no import API exists |
| Capability evidence to prerequisites | Supported bit interpreted as executable permission | Keep each method/dimension, preserve unavailable/unsupported/unknown; select no method | Future method access and physical outcomes are not measured |
| CLI to desktop | Partial JSON, nonzero exit, oversized output, wrong schema/version or stale reply | Clear or invalidate visible results; bounded parsing and generation matching | A pinned executable still requires trustworthy installation ownership |
| UI selection to subprocess | Shell metacharacters, stale list index, arbitrary executable or verb | Only a current inventory selector as one argument, fixed verb enum and absolute program | A visual selection is not an atomic device lock |
| Simulation to operator | Fixture shown as live, green readiness cue, erase progress/certificate wording | Persistent model-only labeling; separate observed evidence and modeled stages | Operator training and physical target review remain outside simulation |

### Future separation of duties (architecture only)

Any later design must keep four responsibilities separate: observation of a
fresh target; independent authority to request a tightly scoped operation;
operation lifecycle and recovery; and evidence of the actual resulting media
state. The desktop is a requesting/presentation client, never the deciding
authority. This document intentionally defines no wire protocol, accepted token
layout, IPC endpoint, command dispatcher or executable service stub.

Before any future service exists, its review must establish protected-system
exclusion, cross-namespace consumers, whole physical scope, fresh identity bound
to the actual open handle, and an exclusive-access mechanism valid for that
platform. Denial or incomplete visibility must not trigger administrator retry,
an alternate device alias or automatic unmount. Confirmation text can record
human intent but cannot replace any of these controls.

Power loss, transport loss and process termination may leave a future operation
in an indeterminate or continuing state. A future cancel request must not be
reported as a stopped hardware operation without device-specific evidence.
Recovery and post-operation verification require their own specification and
hardware acceptance; the current simulator must never fabricate their outcome.

### Test linkage for the current implementation

`tests/test_plan.c` exercises positive hazards mixed with false/unknown evidence,
all capability dimensions, malformed counts/enums/strings, optimistic supplied
status, snapshot role/holder/read-only contradictions and copied evidence
ownership. The pure model has no native adapter dependency or heap allocation.
CLI forbidden-call and Qt process/parser/UX tests are separate milestone gates;
this architecture document is not evidence that those later gates have passed.

## Threat-model tests

Build negative fixtures for reused device path, changed native ID/gen/controller, stale GUI selection, path injection and external binary spoofing, root/mount/swap conflict, layered/unknown physical scope, capability-supported-but-not-accessible, partial JSON, subprocess failure, and simulation status spoofing. All such cases must fail closed or disclose unknown. Keep protocol fixture tests independent of any real device access.
