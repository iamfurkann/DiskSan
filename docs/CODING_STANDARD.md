# Coding and documentation standard

## Language and scope

All maintained source, test, build-configuration, documentation, and contributor-facing prose is English. Keep repository text ASCII; represent non-ASCII test inputs with explicit byte or Unicode escapes. Runtime metadata may contain any valid UTF-8 and must retain the encoding behavior documented in the device model.

Use C17, four-space indentation, explicit ownership, bounded conversions, and existing `ds_` public names. Keep a conventional function signature at file scope so lightweight documentation checks can inspect it. Do not add platform headers or system-specific types to the portable core.

## File and function briefs

Start every C source/header with a Doxygen file block:

```c
/**
 * @file
 * @brief Describe the module's responsibility and boundary in one sentence.
 */
```

Document every function, including static helpers, entry points, and test wrappers, immediately before its declaration or definition. Explain its purpose and important constraints instead of restating the name. Public headers are the authoritative API contract; implementations carry a concise local brief.

```c
/**
 * @brief Parse an unsigned decimal value without accepting overflow.
 * @param[in] text Null-terminated input; null is rejected.
 * @param[out] value Receives the parsed value only on success.
 * @return True for valid input; false for invalid input or overflow.
 */
bool ds_parse_u64(const char *text, uint64_t *value);
```

For public APIs, document every parameter with direction, nullability, and ownership where relevant. Document return values for non-void functions. Use `@note`, `@pre`, or `@warning` when lifetime, state, thread, platform, or safety constraints need emphasis. Never describe an operation as atomic, permanent, or safe for erasure without evidence.

Document public structs, fields, constants, and nontrivial macros. Field comments use `/**< @brief ... */`. Include guards do not need briefs. In-body comments explain decisions, invariants, or non-obvious failure handling; obvious assignments do not need narration.

## Python and build files

Python modules use a Doxygen-style module docstring with `@file` and `@brief`. Every class, function, and method, including tests, has a meaningful docstring beginning with `@brief` (optionally prefixed by `!` for Doxygen).

```python
"""! @file
@brief Verify the portable output contract without opening devices.
"""


def invoke(binary):
    """! @brief Run a read-only command and return its captured result."""
```

CMake files begin with `# @file` and `# @brief` comments. Explain platform selection, hardening, test registration, and packaging decisions at their relevant sections. Doxyfile configuration comments follow the same file-brief convention.

## Ownership and failure handling

Initialize records before use. Release all owned memory on every path. Borrowed evidence labels must outlive their records. Inventory insertion consumes a record only on success, including deduplication; the caller retains ownership on failure.

Unknown values stay unknown. Preserve a known zero separately from absence. Keep the first valid evidence and report conflicts. Check conversions, descriptor offsets, buffer lengths, and allocation failures. Propagate stream failures to a nonzero command status.

## Device-access boundary

Version 0.2.0 performs metadata queries only. Preserve Linux O_RDONLY and the three-ioctl allowlist. macOS uses registry/description APIs without raw opens. Windows requests read or zero access, never GENERIC_WRITE. Tests must not require root or write device data. New native queries require review of their documented access and side effects.

## Enforcement and reference generation

```sh
python3 tools/check_documentation.py
ctest --test-dir build --output-on-failure
# Optional, with Doxygen installed:
doxygen Doxyfile
```

The `documentation` CTest gate verifies the source manifest, ASCII policy, file/function briefs, public header parameters/returns, and Python docstrings. It is a focused checker for this repository's conventional declarations, not a full C parser or a proof that prose is English or correct. Review remains necessary. Generated API HTML is written under `build-api/html` and must not be committed.

The syntax follows the official [Doxygen comment-block guide](https://www.doxygen.nl/manual/docblocks.html) and [special commands](https://www.doxygen.nl/manual/commands.html).

## Failure seams and CI

Keep test substitution outside production behavior. The allocation test compiles a separate core object with a forced include after CRT declarations; production allocation is unchanged. Linux link wrappers substitute only documented boundaries. Portable Windows buffer validation uses ordinary byte fixtures and must not be reported as native handle testing. Every new helper, wrapper, and test still needs `@brief`.

Inventory merge must be transactional: failed allocation or ambiguous identity preserves both inputs. Direct metadata operations may retain partial diagnostic evidence before a fatal allocation error; callers always clean up the record and must not publish the failed result. Update tests when changing this contract.

Workflow YAML needs file/brief comments, explicit least-privilege permissions, and no secret-bearing pull-request execution. The documentation checker includes `.github` and JSON fixtures in the source manifest. Workflow syntax is validated separately; ASCII validation alone is not a YAML or security check.
