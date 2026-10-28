# Read-only assessment design (0.2.0 implementation)

This is the implementation contract, not a statement that every milestone has
passed. See `PROGRESS.md` for historical 0.2.0 evidence. The additive 0.3.0
provider path is documented in [architecture](ARCHITECTURE.md) and
[the protocol audit](PROTOCOL_QUERY_AUDIT_0.3.0.md); the policy below remains unchanged.

## Observation and policy boundary

The portable assessment model owns bounded, inline evidence strings. It makes no
OS calls and allocates no heap memory. Evidence is append-only and exact duplicates
are ignored. Invalid enums, oversized strings and exhausted capacity return an
error without appending the invalid evidence, and set a sticky failure flag.
Evaluation then fails with `not_evaluated`; callers must discard the assessment.
Silently dropping observations is forbidden.

Each observation records a typed dimension, true/false/unknown, source and detail.
Policy combines all observations. Conflicting true and false claims produce a
review finding; an observed hazard remains blocking even with contradictory or
unknown evidence. A negative hazard observation mixed with unknown evidence is
unknown. Absent observations are unknown. Thus empty mount lists are not global
proof of inactivity. Findings are emitted in fixed dimension/method order, while
evidence is sorted independently for deterministic output.

Role, mounts, swap, system dependencies, device read-only flags, holders and slaves
are distinct from completeness of identity, generation, topology, usage scope and
physical/controller scope. Namespace-wide visibility is a separate completeness
dimension. Every completed assessment includes a non-atomic snapshot limitation.
No policy state authorizes erasure; no result object contains a write handle,
authorization token, future validity interval, or callable destructive backend.

## Capability boundary

Method support, OS access, applicability and bridge passthrough are independent
observations. Supported/unsupported claims require a named evidence source; bus,
model, rotational and permission metadata cannot provide support evidence. The
initial provider deliberately performs no protocol commands and reports method
support as unknown, not unsupported. A provider that cannot inspect an API reports
unavailable and the reason. Contradictions preserve the original and competing
observations and require review. Protocol queries may be added only after the
exact request and access mask receive an explicit read-only audit.

## Target resolution and output contracts

Selection uses the existing native resolver. The assessment window begins with a
fresh selected record, collects usage/topology, then resolves again using the
original selector. Known changes in native ID, role, devnum, generation, capacity,
parent or controller fail with exit 3 and no success payload. Missing evidence
remains unknown. Repeated observations are not an atomic identity guarantee and
cannot detect every remove/reinsert or ABA event; Windows device numbers without
a generation are particularly limited.

`list` and `info` retain schema 2 and their existing meanings. `assess` and
`capabilities` use a separate schema-2 operation envelope, including snapshot
identity, evidence, findings and method observations. Both use the same
revalidation window. Exit 0 means inspection completed, including when policy is
blocked or review-required. Fatal observation/identity/output errors remain
nonzero. The envelope always states `erase_authorized: false` and
`guarantees_future_identity: false`. No saved result is valid for a future action.

## Current risk inventory and implementation order

1. Native discovery checks do not cover an entire assessment window: add explicit
   before/after comparison with deterministic race tests.
2. Linux proc metadata is namespace-local, graphs can change and swap files cannot
   be resolved by block-node names: parse bounded records, use identifiers and
   retain uncertainty about incomplete ancestry and file backing.
3. APFS sharing and Windows multi-disk extents exceed a single native parent link:
   collect available native metadata and report unresolved dimensions explicitly.
4. Existing topology lists cannot prove completeness: add independent completeness
   observations rather than interpreting an empty list as absence.
5. Physical hardware and native Windows access are not assumed. Cross-compilation,
   fixtures, native-host execution and controller verification remain separate gates.

Implement the portable model/policy first, then resolver and usage adapters,
capability provider, CLI/rendering, integration/fault checks and release handoff.
Source version is 0.2.0 now that the new surface is integrated. No candidate
release or tag exists merely because this design document exists.
