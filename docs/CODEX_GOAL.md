# Codex Goal - DiskSan v0.2.0

> Copy the **single-line `/goal` command** below into a Codex thread with Goals support. The Goal is a **thread-level objective**: placing this file in the repository does **not** automatically activate it. `AGENTS.md` provides standing repo instructions; this file provides the completion target.

## Before activating

1. Place `AGENTS.md` at the DiskSan repository root and install the accompanying `docs/` files according to `docs/CODEX_WORKFLOW.md`.
2. Open the actual DiskSan project in Codex. Ensure the working directory includes `AGENTS.md` and the version specification.
3. Inspect the current checkout. Do not assume the baseline SHA is still current. If there are unrelated user changes, preserve them.
4. Check `codex --version` when using the CLI. Goals require a Codex build supporting the feature (documented from CLI 0.128.0). The app or hosted environment may expose it differently. If `/goal` is unavailable, use the ordinary prompt below instead.

## Ready-to-paste `/goal`

```text
/goal Complete DiskSan v0.2.0 as a safe, evidence-based, read-only target assessment release candidate according to docs/specs/v0.2.0.md, using the actual current repository state and AGENTS.md. Establish and record a tested baseline; implement portable policy/findings, identity revalidation, conservative usage/topology checks, read-only capability evidence and a documented CLI/JSON surface; maintain backward compatibility with v0.1.1. Work through verifiable milestones, run relevant tests and sanitizers, update docs/PROGRESS.md after evidenced milestones, resolve failures, and continue with the next independent task without routine approval requests. A capability observation, successful inspection, or saved snapshot must never authorize a future erase. No erase/sanitize/format/discard/mount-modifying/device-data-writing code or real-device destructive tests. Preserve existing user changes. Never commit, push, tag, publish a GitHub release, alter repository settings, or report unrun CI/native Windows/hardware checks as passed. Completion requires implemented code, passing accessible acceptance/regression tests, an audited read-only boundary, accurate documentation/manifests, and a final evidence-based handoff. If external platform access or a genuine blocker remains, finish independent reachable work and report precisely what is BLOCKED/NOT_RUN and which maintainer action would unblock it; do not invent a result.
```

## If your Codex interface does not support Goals

Paste the following as an ordinary initial prompt. It does not create a persistent Goal, so use `docs/PROGRESS.md` for handoffs.

```text
Implement DiskSan v0.2.0 in the currently open repository. Read AGENTS.md and docs/specs/v0.2.0.md; consult docs/BASELINE_0.1.1.md as historical evidence, but inspect the live working tree before assuming facts. Follow the milestone structure in docs/PROGRESS.md. Carry out the authorized engineering work, not just a plan: design, implement, test, fix regressions, update documentation, and produce a factual handoff. Work independently within the safe read-only boundary; preserve existing behavior and user changes. Do not run destructive disk operations. Do not commit, push, tag or publish. Record each verified milestone and distinguish PASS/FAIL/BLOCKED/NOT_RUN. Complete all reachable work and identify external acceptance gates accurately.
```

## Goal controls (where supported)

```text
/goal          # View current goal
/goal pause    # Pause an active goal
/goal resume   # Resume a paused goal
/goal clear    # Clear the goal from the thread
```

The Goal belongs to the **current Codex thread**, not to the GitHub repository and not to every new chat. It does not guarantee infinite execution: budgets, interruptions and genuine blockers can stop progress. A fresh thread should receive a new `/goal` or ordinary prompt, after reading `docs/PROGRESS.md`.

## Progress and completion semantics

- **PASS**: the specific criterion was observed and checked with a concrete command/fixture/log.
- **FAIL**: an attempted criterion failed; record the failure and fix or explain.
- **NOT_RUN**: no test execution evidence exists.
- **BLOCKED**: the criterion depends on inaccessible hardware, permissions, a missing toolchain or maintainer action.
- **SKIP**: a test explicitly skipped and why; a skip is not a pass.
- **IMPLEMENTATION_COMPLETE**: every reachable requirement is implemented and verified without an open critical risk; remaining external gates are reported, not falsified.
- **READY_TO_PUBLISH**: separate maintainer determination after required external gates. The Goal may finish with `IMPLEMENTATION_COMPLETE` and a `NOT_READY_TO_PUBLISH` handoff.

## Sources and rationale

- OpenAI, **Using Goals in Codex**: <https://developers.openai.com/cookbook/examples/codex/using_goals_in_codex>
- OpenAI, **Rethinking skills and prompts for GPT-6 Astra**: <https://developers.openai.com/blog/rethinking-skills-and-prompts-for-gpt-6-astra>
