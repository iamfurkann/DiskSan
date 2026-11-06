# DiskSan v0.4.0 - non-executable plan and simulator contract

## Semantic rules

`plan` is a description of observed device identity, risks and **hypothetical** prerequisites. `simulate` is a deterministic *model preview* of what checks would be required by a future engine. Neither opens a device for data I/O, issues any sanitize/format/trim/discard command, unmounts anything, calls `capabilities --probe`, elevates privilege or creates a reusable target authorization. A plan is not an artifact to feed into a future executable command.

Result levels are derived conservatively from the existing `ds_assessment` evidence:

- Observed `mounted=true`, `system=true`, `swap=true`, `read_only=true` (for hypothetical write feasibility), `holders=true`, `slaves=true` or logical target: `blocked`; known evidence remains blocking even when contradictory unknowns occur.
- Missing, contradictory, unknown or incomplete identity, generation, topology, host scope, usage or physical scope: `review_required` or `insufficient_evidence` (never `clear`).
- Even with no observed blockers, **execution_available=false** and **erase_authorized=false**. Future privilege checks and verified completion are not present.

Do not claim that a physically unmounted disk is idle or that no blocker was proven. Positive capability support is independent from application permission and operation applicability. Avoid selecting or recommending sanitization method automatically.

## Example synthetic schema-2 result (illustrative, not production fixture)

```json
{
  "schema_version": 2,
  "application": "DiskSan",
  "version": "0.4.0",
  "operation": "simulate",
  "mode": "read-only",
  "simulation_only": true,
  "execution_available": false,
  "erase_authorized": false,
  "identity": {
    "atomic": false,
    "guarantees_future_identity": false,
    "valid_until": "end_of_observation"
  },
  "inspection_status": "review_required",
  "device": { "device_role": "disk", "path": "/dev/EXAMPLE-NOT-REAL" },
  "findings": [{"code":"physical_scope_unverified","severity":"review"}],
  "prerequisites": [{"code":"independent_authorization_unimplemented","state":"not_implemented"}],
  "steps": [{"code":"preflight_scope","state":"unknown_requires_review","modeled_only":true}],
  "simulation_result": "non_executable_model_only"
}
```

In the real output `device` must be the full unchanged published schema-2 device object, not the shortened example. New arrays, string bounds, final policy taxonomy and terminal rendering must be documented in `docs/DEVICE_MODEL.md`; fixture sources are explicitly marked synthetic. `findings` are not a sanitization receipt and cannot contain success/completion tokens.

## Implemented transcript contract

The simulator builds a new plan from a fresh metadata-only observation window on
every invocation. Rendering reconstructs and compares the diagnosis before
writing; mutated or partial plan records cannot be published. It does not import
JSON, retain a session, wait for time to pass, choose a method or create a token.

The ten bounded steps have one-based `index`, `code`, `state`, `prerequisite`,
`evidence_scope`, `explanation` and `modeled_only:true`. Their fixed order is:

1. `observe_target`
2. `compare_identity`
3. `review_usage`
4. `review_topology`
5. `review_method_evidence`
6. `privilege_boundary_unimplemented`
7. `authorization_boundary_unimplemented`
8. `exclusive_scope_unimplemented`
9. `operation_backend_unavailable`
10. `completion_verifier_unavailable`

Step states are derived from the corresponding prerequisite in the same owned
plan: `inspection_step_modeled`, `unknown_requires_review`, `would_block`, or
`not_implemented`. The last five are always `not_implemented`. A blocker is a
hypothetical precondition failure, not a real operation rejection or completion.
The same snapshot and evidence produce identical output bytes. Live invocations
can legitimately differ because device observations are neither atomic nor
stable across time. `simulation_result` is always `non_executable_model_only`.

`tests/test_plan_cli.py` applies the same parser, error, exact JSON, terminal,
native invalid-target, stream and allocation contracts to both verbs. Its
synthetic native provider is linked to a fatal protocol tripwire, with a control
case proving that the tripwire is active. Dedicated allocation instrumentation
exhausts every core allocation across both snapshots and checks zero live blocks.
`test_plan` verifies pure policy truth tables and rejection of malformed output
records before writing any byte. None of these tests manipulates a real device.

## Required negative assertions

- No `plan`/`simulate` code path invokes `ds_native_protocol_probe` or other device-mutating functions. Default metadata-only assessment alone may use its existing read-only calls.
- Run selectors resembling shells, spaces, Unicode, leading dashes, bad path, path-reuse and missing target; no extra process execution, no success payload after failure.
- Any simulated system/root/active swap/mounted device is blocked, not "simulated success".
- No support/permission guesses from existing `--probe` findings or bus labels.
- Stable output ordering and codes; exact uint64; bounded/escaped text; JSON parseable only on successful output.
- Multiple consecutive inspections, failed allocation, output sink failure, stale identity and error propagation preserve cleanup and no permission to erase.
