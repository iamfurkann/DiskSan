# DiskSan v0.3.0 - protocol safety and validation matrix

Status: Available software, documentation and candidate distribution checks PASS. All physical protocol tests, native Windows/MSVC and final committed-source CI are NOT_RUN. Exact snapshots and logs are recorded in [the progress ledger](PROGRESS_0.3.0.md).

## Request-constructor tests (non-negotiable)

| Fixture | Required observable behavior | Status |
| --- | --- | --- |
| Default `capabilities` with any valid selector | Exactly zero native protocol queries; output remains 0.2.0 contract | PASS: old fixtures + 38 baseline comparisons |
| `assess`, `list`, `info` with `--probe` | Usage exit 2, zero protocol queries and no valid-success payload | PASS: CLI grammar fixtures |
| `capabilities --probe` on ordinary file/dir/invalid selector | Exit invalid-target/usage as appropriate; no native query | PASS: actual CLI disposable targets on Mac/GCC/Clang |
| Unknown/prohibited request enum or user-controlled opcode | Reject before opening native handle | PASS: decoder gate + 255 native forbidden enum cases |
| Linux LN-01 fixed constructor | Only Identify opcode 0x06, CNS 0x01, fixed 4096-byte data-in, zero unused fields, bounded timeout | PASS: exact bytes and flags captured on GCC/Clang |
| Forbidden NVMe sanitize/format/feature/reset/I/O code path | No production execution path; fake ioctl captures zero forbidden invocations | PASS: fixed constructor + independent ioctl allowlist wrapper |
| Linux open permission denial | No O_RDWR/O_WRONLY/sudo retry; honest unavailable + cleanup | PASS: native fault cases; no retry |
| Windows WN-01 fixed property query | Only permitted `IOCTL_STORAGE_QUERY_PROPERTY` Identify Controller request; zeroed bounded descriptor, correct access mask | NOT_RUN / NOT_APPROVED: WN-01 is not implemented |
| Windows general pass-through attempt | `IOCTL_STORAGE_PROTOCOL_COMMAND`, SCSI pass-through and writable handles absent/rejected | PASS source boundary: actual Windows build selects no-query adapter; no new native API |
| Controller/namespace mismatch or target disappearance | Discard partial assessment; invalid-target nonzero and no success JSON | PASS: native mapping and target-window faults |
| USB/virtual/multipath/unknown mapping | No raw bridge command; unknown/unavailable evidence | PASS synthetic eligibility and mapping gates; physical NOT_RUN |

## Response decoder / evidence tests

| Fixture | Required observable behavior | Status |
| --- | --- | --- |
| Valid NVMe Identify Controller bytes | Correct little-endian documented command-family flags; evidence cites exact field | PASS: all support combinations and version layouts |
| All truncated payload lengths | No out-of-bounds read, `supported` never inferred; fail or unavailable | PASS: every length 0..4095 |
| Non-NVMe/wrong descriptor/version/size | No capability evidence upgrade; bounded error and cleanup | PASS for enabled Linux envelope; Windows descriptor NOT_RUN |
| Unaligned returned buffer and corrupt offset arithmetic | Decoder remains safe; no struct cast based on untrusted alignment | PASS: byte decoding; Linux has no driver-provided offsets |
| Contradictory source observations | Both sources retained; derived capability unknown/review | PASS: unchanged policy conflicts + mixed-provider rejection |
| Support bit without proven os access | Support may be observed; other dimensions remain independent | PASS: 24 dimensions checked |
| Permission denied / not found / unsupported API | os_access unavailable or explicitly unknown; not hardware unsupported | PASS: native error injection and normalized evidence |
| Successful Identify with USB bridge | No inferred passthrough/scope guarantee; restrict or leave unavailable | PASS: bridge inputs declined without query; no passthrough claim |
| Observation limit exhaustion | Fatal/poison result; no quietly truncated positive findings | PASS: poisoned bounded result, no partial serialization |
| Output I/O failure | Nonzero exit, no claimed valid complete JSON | PASS: real output failure and fixture stream errors |
| Scrubbing/log redaction | No raw Identify, serials, full inventories or untrusted text in shared CI artifacts | PASS source review + raw marker absent from fixture output |

## Environment acceptance table

| Environment | Required evidence | State |
| --- | --- | --- |
| macOS arm64 AppleClang Release | Full baseline + changed CLI tests; native unavailable semantics if no approved API | PASS: 19/19 |
| macOS arm64 AppleClang ASan/UBSan | Full sanitizer regression | PASS: 19/19 |
| Linux x86-64 GCC Release | Full CTest plus request-constructor synthetic tests | PASS: 24/24 + latest Python CLI 9/9 |
| Linux x86-64 Clang ASan/UBSan | Full fault/decoder/sanitizer suite | PASS: 24/24 + latest Python CLI 9/9 |
| Windows x86-64 MSVC hosted | Native build, synthetic `DeviceIoControl` fixtures, full CTest | NOT_RUN: current native Windows host/SDK unavailable; no property provider enabled |
| Linux manually selected physical NVMe device | Deliberate opt-in real Identify, controller mapping and non-mutating trace; not a boot/system disk | NOT_RUN: no maintainer-selected device or live invocation |
| Windows owner-selected physical NVMe device | Deliberate opt-in property query, real data validation, driver/API scope | NOT_RUN: no enabled provider or selected device |
| ATA/SCSI/macOS physical provider | Only if an individual audit row has become ACTIVE; otherwise BLOCKED/NOT_APPROVED | NOT_APPROVED: no native query enabled |
| Final committed source CI | All configured jobs on exact reviewed SHA | NOT_RUN: no authorized final commit/push |
| Release assets | Regenerated after final edits, checked file/bytes/SHA-256 and relocated smoke | PASS candidate scope; Windows runtime NOT_RUN; final hashes in SHA256SUMS |

### Notes

Real device tests must require a maintainer-supplied selector; no CI runner or standard CTest command may probe physical firmware by default. Keep individual failure evidence private and publish only redacted summaries. `SKIP`, `NOT_RUN` and `BLOCKED` are distinct from `PASS`. A passing Windows runner without real NVMe data is not protocol-hardware acceptance.
