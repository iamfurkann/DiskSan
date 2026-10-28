# v0.3.0 native protocol query allowlist and threat audit

Status: LN-01 APPROVED FOR PRODUCTION ACTIVATION after fixed-request, mapping and lifecycle fixtures passed GCC and Clang ASan/UBSan. No physical protocol execution has been performed. Other request rows remain unapproved. This allowlist authorizes only the exact explicit CLI pathway in code; it is not permission to select or probe hardware automatically.

## 1. Core rule

The native API call is NOT the unit of authorization: **the exact ioctl/API + request variant + input fields + device path + access flags + response length + error behavior is**. The fact that an ioctl is named `ADMIN`, `QUERY` or `PASS_THROUGH` does not establish non-mutating semantics. A fixed, independently reviewed request constructor is necessary. It may never accept arbitrary opcodes or caller-supplied control dwords.

Closed request enum in portable code: `DS_QUERY_NVME_IDENTIFY_CONTROLLER` initially. Each added request requires a separately audited constructor/test and this table update. Unknown IDs always fail *before opening any device*. User input may select an existing DiskSan native target, but may not choose opcode, CNS, NSID, log ID, vendor code, timeout, buffer length or transfer direction.

## 2. Platform allowlist

| ID | Platform, entrypoint | Exact request | Native access and target | Maximum reply | State |
| --- | --- | --- | --- | --- | --- |
| LN-01 | Linux, `NVME_IOCTL_ADMIN_CMD` only | NVMe Identify Controller: opcode 0x06, CNS 0x01, NSID 0; 4096 data-in; unrelated dwords, metadata and flags zero; 5000 ms timeout | Exactly O_RDONLY / O_CLOEXEC / O_NOFOLLOW / O_NONBLOCK; verified direct PCI NVMe controller character node associated with selected namespace | Fixed 4096-byte initialized buffer; no returned byte count in this ioctl | ACTIVE - synthetic request/mapping/lifecycle validation; physical execution NOT_RUN |
| WN-01 | Windows, `IOCTL_STORAGE_QUERY_PROPERTY` | Proposed fixed controller Identify property query only | Selected-device association and actual SDK/driver access must be validated | Bounded descriptor + 4096-byte reply, checked offsets | NOT APPROVED - native MSVC/API fixture validation unavailable; no live provider compiled |
| LN-02 | Linux, NVMe Identify Namespace CNS 0x00 | Only fixed NSID from independently verified namespace mapping | Separate exact audit required | 4096 bytes | NOT APPROVED for implementation until separately audited |
| LN-03 | Linux, NVMe Get Log Page | Only an individually documented fixed log-page query; no generic log-page selector | Separate exact audit, permissions and scope required | Protocol-specific fixed bounded length | NOT APPROVED until separately audited |
| A-01 | ATA IDENTIFY DEVICE | Must document exact OS native access primitive, CDB direction, allowed selector and response sizes | Platform-specific review | 512 bytes where applicable | NOT APPROVED |
| S-01 | SCSI INQUIRY / VPD | Must document CDB, EVPD/page, allocation length, data-in access, and translation limits | Platform-specific review | Fixed bounded page | NOT APPROVED |
| M-01 | macOS `IONVMeSMARTInterface::GetIdentifyData` candidate | SDK documents a 4K Identify request with namespace 0 for controller data | Selected IOMedia/controller binding, user-client access and response contract not validated | Header describes 4096 bytes; no independent returned length | NOT APPROVED - unavailable adapter compiled |

If either LN-01 or WN-01 cannot be established on a specific OS/driver, report `unavailable` without issuing anything, and log the relevant acceptance gate. Do not bypass with root, a writable handle, generic SG_IO, IOCTL_STORAGE_PROTOCOL_COMMAND, vendor passthrough, an external program or unreviewed kernel module.

## 3. Linux detailed review checklist, LN-01

Reference: `include/uapi/linux/nvme_ioctl.h` defines `struct nvme_passthru_cmd`, `NVME_IOCTL_ADMIN_CMD` and `NVME_IOCTL_ADMIN64_CMD`. Kernel `drivers/nvme/host/ioctl.c` checks whether admin requests can be issued without write access and defines a restricted set of unprivileged Identify CNS values; actual deployed kernels may differ.

Before enabling LN-01, record: exact tested kernel version, available header names, selected controller/namespace device type, mapping source, read-only `open` flags, ioctl command ID, Identify opcode/CNS/NSID and every control dword, data buffer pointer lifetime, timeout, return error interpretation and close behavior. Document the kernel's permission behavior for this exact request and route. Do not assume successful open means the ioctl is safe or permitted.

Request constructor tests MUST prove that all other admin opcodes and CNS variants cannot be generated, that the data transfer is device-to-host, that metadata and unused fields are zeroed, and that the program cannot open a device with O_RDWR/O_WRONLY or invoke any reset/io_uring/I/O submission ioctl. Negative tests capture the actual ioctl argument bytes with wrappers; parser-only tests are insufficient.

Controller Identify fields should be decoded from copied bytes using bounds and LE helpers; no untrusted in-memory packed struct reinterpretation. Test a full 4096-byte fixture, missing/short result, failure/permission and truncated copies. Do not log the raw 4096 bytes.

## 4. Windows detailed review checklist, WN-01

Microsoft documents `IOCTL_STORAGE_QUERY_PROPERTY` and `STORAGE_PROTOCOL_SPECIFIC_DATA` for NVMe Identify and Log Page information. Limit WN-01 to controller Identify; do NOT add generic protocol command `IOCTL_STORAGE_PROTOCOL_COMMAND`. Validate actual build SDK defines needed enums/structures and tested Windows version. The request uses a typed `STORAGE_PROPERTY_QUERY`, `StorageAdapterProtocolSpecificProperty`, `PropertyStandardQuery`, `ProtocolTypeNvme`, `NVMeDataTypeIdentify`, `ProtocolDataRequestValue=NVME_IDENTIFY_CNS_CONTROLLER`, fixed `ProtocolDataOffset` and `ProtocolDataLength`.

Validate `STORAGE_PROTOCOL_DATA_DESCRIPTOR.Version`, `.Size`, `STORAGE_PROTOCOL_SPECIFIC_DATA` location and field values, `ProtocolDataOffset+ProtocolDataLength` with overflow-safe arithmetic against bytes returned and allocated length, plus the expected protocol reply size. Re-query relevant native identity and controller association as required by the target window. Check `DeviceIoControl` return, errors, and all `CreateFileW`/`CloseHandle` paths.

Do not interpret general metadata access or a synthetically filled descriptor as confirmation that Windows firmware allows future NVMe commands. Hosted Windows runners may not have suitable NVMe targets. Track MSVC fixtures separately from physical-controller verification.

### Conditional-provider decision for this candidate

The [Microsoft NVMe documentation](https://learn.microsoft.com/en-us/windows/win32/fileio/working-with-nvme-devices)
defines the candidate property mechanism for Windows 10 and later. The relevant
enum/descriptor declarations also exist in the reviewed MinGW-w64 headers, but
that does not satisfy the specification's native MSVC plus fake-DeviceIoControl
acceptance gate. No current Windows SDK/runtime host is available for this work.
The historical 0.2.0 hosted runner cannot validate new code. Therefore WN-01 is
not implemented or conditionally enabled behind an unchecked build flag; the
production Windows provider is the explicit no-query unavailable adapter.

macOS SDK 27.0 includes `IOKit/storage/nvme/NVMeSMARTLibExternal.h` with
`IONVMeSMARTInterface::GetIdentifyData`, a 4096-byte buffer and namespace zero
for a controller request. Its typed layout documents NVMe 1.0c and reserves the
later version/SANICAP field regions. Header availability alone does not establish
selected IOMedia/controller association, user-client permission semantics or
modern response validity. No plugin/user client was instantiated and no device
queried during this review. M-01 remains unapproved; this is not a claim that
macOS has no Identify API. ATA/SCSI/USB interfaces remain unapproved as well.

`src/platform/protocol_unavailable.c` returns `provider: none`,
`status: unavailable`, `reason: platform_unavailable`, `performed: false` and
cleared numeric output without any native calls. The collector supplies all 24
dimensions as unknown. `tests/test_protocol_unavailable.c` exercises that exact
adapter on every build, including valid-looking NVMe records and forbidden kinds.

## 5. Permanently prohibited operations in v0.3.0

- Any NVMe Format, Sanitize, Security Send/Receive, Set Features, Firmware Commit/Download, Namespace Management, Device Self-test, resets, I/O reads of user data, and vendor-specific opcodes.
- Arbitrary NVMe ADMIN_CMD, ADMIN64, io_uring or SG_IO user input and raw IOCTL numbers supplied by CLI/config/files.
- ATA Security Erase, Sanitize, Set Features, SMART self-test initiations, download microcode, power state changes, any data-out pass-through.
- SCSI FORMAT UNIT, SANITIZE, WRITE SAME, UNMAP, SECURITY PROTOCOL OUT, START STOP UNIT, SEND DIAGNOSTIC or arbitrary CDB pass-through.
- Mount, unmount, fsck, format, discard/TRIM, partition/metadata modification, root/sudo fallback, automatic raw hardware queries during the test suite.

Block these through explicit allowlist tests and source review, not keyword searches alone. Log a failure in `PROGRESS_0.3.0.md` if a proposed native call cannot be shown non-mutating.

## 6. Approval and evidence record for each request

### LN-01 constructor boundary (M2; production activation pending)

- Source: `src/platform/linux/protocol_query.c`; only its fixed wrapper is compiled in the synthetic `linux_protocol_query` test at this milestone. Production has no caller yet.
- Exact request: `NVME_IOCTL_ADMIN_CMD` with a completely zeroed `struct nvme_passthru_cmd`, opcode 0x06, NSID 0, CDW10 0x01, data pointer to an owned 4096-byte initialized buffer, data_len 4096 and timeout_ms 5000. Flags, reserved words, metadata pointer/length, CDW2/3/11..15 and initial result are zero. No caller-supplied command fields, size, timeout or opcode exists.
- Unknown request IDs, null buffer and invalid descriptor fail with EINVAL before ioctl. Exactly one request is issued; errno, positive NVMe completion status and negative syscall failure are retained. No retry, fallback node or privilege change occurs.
- Kernel route reviewed: [Linux v6.12.101 ioctl.c](https://github.com/gregkh/linux/blob/v6.12.101/drivers/nvme/host/ioctl.c), `nvme_dev_ioctl` -> `nvme_user_cmd(ctrl, NULL, ...)`; `nvme_cmd_allowed` accepts this specific Identify CNS. Controller Identify transfers metadata toward the host. ADMIN64, I/O, reset, rescan and all other command variants remain excluded.
- The ioctl has no returned byte-count field. Its fixed request contract specifies 4096 bytes; completion status and the decoder's required fields are checked separately. A successful ioctl is not independent proof that every firmware response byte is truthful. The timeout is a driver request limit, not a guaranteed userspace wall-clock deadline.
- Independent wrapper fixture compares the complete 72-byte native command against separately built constants, checks the actual ioctl and initialized response bytes, rejects all other request IDs and captures denial/interruption/timeout/device-loss/status outcomes. Native descriptor access flags, mapping and handle lifecycle are M4 gates and are not established by this constructor fixture.
- Decision: CANDIDATE, not ACTIVE. Physical-device execution: NOT_RUN. No real controller was opened for these tests.

Use this template when a previously CANDIDATE request becomes implemented:

### LN-01 association and lifecycle (M4 activation record)

- `protocol_mapping.c` recreates a udev context and block object for each mapping.
  Only a selected native namespace with NVMe transport/protocol and no observed
  holders/slaves is eligible. The immediate parent must be an NVMe controller,
  its immediate parent PCI with driver `nvme`, and controller transport/state
  exactly `pcie`/`live`. Namespace heads, virtual/aggregate and bridge paths are
  declined. Observed PCI transport does not prove physical hardware.
- The selected sysfs path, native block number, canonical udev node and
  `syspath#diskseq` must match. Read only `diskseq`/`nsid` on the namespace and
  `cntlid`/`transport`/`state` on the controller. These are cached kernel metadata
  in the reviewed [v6.12.101 sysfs implementation](https://github.com/gregkh/linux/blob/v6.12.101/drivers/nvme/host/sysfs.c).
  In particular, do not read `nuse`, whose sysfs handler can issue another
  Identify query. The [PCI adapter](https://github.com/gregkh/linux/blob/v6.12.101/drivers/nvme/host/pci.c)
  defines driver `nvme` and transport `pcie`.
- The controller path is copied from that parent's udev devnode, never guessed
  from a namespace name. `lstat` requires a block namespace node and character
  controller node with matching native numbers. Open exactly once using
  `O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK`; fstat must match its type,
  devnum, filesystem device and inode. No alternate node or write access exists.
- Map immediately before open, again after open before querying, and after the
  query even on native failure. Node identity, generation, controller ID and
  namespace ID must agree. The decoded CNTLID must match; the selected NSID must
  fit the decoded maximum valid NSID. Missing initial evidence is unavailable;
  changed or lost established association is fatal exit 3 without output.
- Close the descriptor once on every opened path. A close error prevents success;
  Linux EINTR is not retried because that could close a reused descriptor. Only
  the fixed one-query helper is called. Ownership/allocation, malformed replies,
  denial, device loss and post-query changes are covered by synthetic fixtures.
- Verified source snapshot `m4-provider-reviewed-source`: all 116 maintained-file
  SHA-256 values matched after transfer. GCC 14.2 Release and Clang 19.1.7 Debug
  ASan/UBSan passed documentation, decoder, constructor and native lifecycle
  tests (4/4 each). The lifecycle test runs 421 cases, including 255 forbidden
  enum values; 279,111 assertions include per-byte zero-buffer checks. Actual
  production source is compiled into the fixture; only native boundaries are
  replaced. Numeric evidence is also tested through the production collector.
- Decision: ACTIVE for the exact Linux implementation. No ADMIN64, namespace
  Identify, Get Log Page, ATA, SCSI, bridge or vendor request is approved. Real
  hardware execution is NOT_RUN. Mapping checks cannot establish atomicity,
  detect every ABA event or guarantee future identity.

```
Request ID:
Exact call structure / OS kernel-SDK version:
Target type and selection/revalidation:
Fixed opcode, subfunction, flags, namespace, data direction and max byte count:
Access mask and privilege needs:
Possible side effects and reason the exact variant is observational:
Error / timeout / malformed response handling:
No unauthorized fallback:
Constructor tests (record SHA/command/results):
Native compiler/API tests:
Manually selected physical device test (or NOT_RUN), without raw identifiers:
Reviewer / date:
Decision: ACTIVE | NOT_APPROVED | BLOCKED
```

Document any new entries separately; do not silently widen an existing permit. Unavailable is an acceptable, truthful state.

## 7. Primary public references

### Reviewed decoder contract (M1; no native request)

- Pure implementation: `src/core/protocol.c`, contract `include/disksan/protocol.h`, deterministic fixtures `tests/test_protocol.c`.
- Accept exactly 4096 bytes and explicitly reviewed VER register values 1.3.0, 1.4.0 or 2.0.0. Other revisions remain unrecognized, not unsupported hardware. No text, serial, firmware string or raw reply is retained.
- Byte offsets: VID 0 (LE16), CNTLID 78 (LE16), VER 80 (LE32), CNTRLTYPE 111, SANICAP 328 (LE32), NN 516 (LE32). Validate a nonzero/non-FFFF vendor, nonzero/non-broadcast namespace limit and appropriate controller type. Native request/protocol/association validation is a separate prerequisite; byte decoding alone cannot prove scope.
- NVMe 1.3 uses only SANICAP bits 0/1/2; upper bits are reserved. For 1.4/2.0 reject reserved bits 3..28, reserved NODMMAS value 3, ancillary flags without support, and missing NODMMAS when support is advertised. Unrecognized or inconsistent responses supply no positive or negative support.
- Map CES/BES/OWS independently to crypto/block/overwrite support. All other 21 method/dimension entries remain unknown. Exactly 24 entries fit the existing 64-entry bound; mixing the metadata and probe providers is rejected rather than masking valid evidence with unconditional unknown placeholders.
- References: [NVMe 1.3d, Identify Controller](https://nvmexpress.org/wp-content/uploads/NVM-Express-1_3d-2019.03.20-Ratified.pdf), [NVMe 1.4c, Figure 251](https://nvmexpress.org/wp-content/uploads/NVM-Express-1_4c-2021.06.28-Ratified.pdf), [NVMe 2.0c, Figure 275](https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-2.0c-2022.10.04-Ratified.pdf). Only field meanings and bounds are implemented; no sanitize command or execution semantics are implemented.
- Pinned Linux research: [v6.12 ioctl implementation](https://github.com/torvalds/linux/blob/v6.12/drivers/nvme/host/ioctl.c) and [v6.12 UAPI](https://github.com/torvalds/linux/blob/v6.12/include/uapi/linux/nvme_ioctl.h). The available VM reports 6.12.101; exact installed-header/native fixture checks remain required before LN-01 activation.

- Linux UAPI: https://github.com/torvalds/linux/blob/master/include/uapi/linux/nvme_ioctl.h
- Linux driver permission checks: https://github.com/torvalds/linux/blob/master/drivers/nvme/host/ioctl.c
- Windows NVMe property queries: https://learn.microsoft.com/en-us/windows/win32/fileio/working-with-nvme-devices
- Windows property query API: https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_storage_query_property
- Windows general passthrough excluded from v0.3.0: https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_storage_protocol_command
