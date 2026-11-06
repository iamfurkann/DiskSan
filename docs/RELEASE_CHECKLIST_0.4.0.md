# DiskSan v0.4.0 - maintainer release checklist

Status: maintainer review pending. The [engineering handoff](WORK_REPORT_0.4.0.md) records the uncommitted snapshot, local test results and package evidence. Check off release items against the exact commit and assets selected for publication.

## Feature/safety acceptance

- [ ] `plan` and `simulate` correctly report only hypothetical, non-executable steps, never claim successful erasure and always set `erase_authorized:false`.
- [ ] No new destructive/backdoor executable path, generic protocol command, write access, mount-changing call or automatically invoked `capabilities --probe` exists.
- [ ] Protected/system/mounted/layered/stale/unknown devices preserve blocking/review outcomes.
- [ ] v0.3.0 CLI/JSON/exit/error and native NVMe closed query behavior remain unchanged except documented additive fields/verbs.
- [ ] Documentation and implementation for future authorization explicitly remain design-only; no reusable authorization token or callable engine.
- [ ] CLI builds and package smoke pass with `DISKSAN_BUILD_DESKTOP=OFF`, independent of Qt.
- [ ] Optional Qt GUI has been *actually built and run* on every claimed desktop OS, or those platforms are declared preview/NOT_RUN.
- [ ] GUI process bridge has fixed allowed argv, trusted absolute binary location, JSON/size/deadline checks, stale-selection handling, and no execute/probe/elevation control.
- [ ] Visual error, loading, unknown, blocked, stale, no-device and demo-fixture states are reviewed; keyboard accessibility checked on supported GUI OS.

## Verification and distribution

- [ ] Every available native CLI test (Linux GCC/Clang sanitizer, Mac AppleClang, Windows MSVC) passes on the final source commit with individual skip audit.
- [ ] GUI tests, CMake optional-on/off, Qt version/OS support and licensing checks have concrete results; missing Qt is NOT_RUN, not PASS.
- [ ] Full source manifest, ASCII, API/Doxygen, CMake/CTest, `git diff --check`, package contents, release checksums and build provenance are current.
- [ ] Exact source SHA, tags and CI runs are recorded; signing/notarization only claimed if actually done.
- [ ] Documentation, README, CHANGELOG, release notes explain this is an inspection-only **pre-release** and distinguish GUI alpha from a deployable shipping app.
- [ ] Maintainer explicitly approves and performs any commit/push/tag/release/asset upload separately.

## User-facing release summary requirements

Explain: what 0.4.0 adds to a user; exact command examples; CLI and optional GUI availability; downloads/installation; real tests on named OS and toolchain; skipped hardware/GUI tests; that no data is erased; simulator is not a certificate or operation; Qt license/deployment limits. Do not publish stale "candidate" wording after publication.

## Maintainer handoff on main

Review the working tree and stage the maintained files from the repository root. Review newly added files in the staged diff as well. Build directories, SDKs and the four release assets live outside the source manifest.

```sh
git status --short --branch
git add --pathspec-from-file=docs/FILE_MANIFEST.txt
git diff --cached --check
git diff --cached --stat
git diff --cached
```

After reviewing the staged source and comparing it with the verified snapshot, commit it and push the branch. Record the printed commit ID.

```sh
git commit -m "feat: add read-only inspection plans, simulation, and desktop alpha"
git rev-parse HEAD
git push origin main
```

Check all hosted CLI jobs for that commit, including native MSVC and individual skips. Record the optional desktop workflow result if dispatched. Rebuild and revalidate affected assets if the reviewed source differs from the candidate. Once the release gates are accepted, enter the reviewed commit ID and tag it:

```sh
printf 'Reviewed full commit ID: '
IFS= read -r release_commit
git tag -a v0.4.0 "$release_commit" -m "DiskSan v0.4.0"
git push origin refs/tags/v0.4.0
(cd dist/v0.4.0 && shasum -a 256 -c SHA256SUMS)
```

On GitHub, draft a release for the existing `v0.4.0` tag, select pre-release, and use the text in `docs/RELEASE_NOTES_0.4.0.md`. Attach the three CLI archives and `SHA256SUMS` from `dist/v0.4.0/`. Verify the tag's commit, workflow results and attached asset digests before publishing the draft.
