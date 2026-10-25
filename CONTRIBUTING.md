# Contributing

DiskSan currently implements read-only discovery. Keep changes within that boundary unless a separate feature specification explicitly authorizes a new capability.

## Language and documentation

Write documentation, code comments, identifiers, diagnostics, and commit/PR descriptions in English. Follow the [coding standard](docs/CODING_STANDARD.md): every source file and function needs a meaningful `@brief`, and public APIs need parameter, return-value, ownership, and relevant failure contracts. Use escaped byte/Unicode literals for non-ASCII test data so multilingual behavior remains testable without adding non-English prose.

Update `docs/FILE_MANIFEST.txt` when adding or removing source files. Include source, tests, configuration, and Markdown documents. Exclude generated builds, packages, API HTML, local evidence, credentials, and machine-specific inventories.

## Verification

Run the documentation check and the native build/test suite before submitting changes:

```sh
python3 tools/check_documentation.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Use ASan/UBSan for changes affecting memory ownership, parsing, or native discovery. Run platform-specific tests where the platform is available; explicitly report untested platforms. A cross-build does not replace native acceptance. See [testing](docs/TESTING.md).

## Review expectations

Explain the observable behavior, why the change is needed, and what was verified. Preserve unknown/null values and evidence warnings. Do not deduplicate by serial number, infer physical media from a brand name, or treat a snapshot as erase authorization. Keep native APIs inside their adapters and maintain the read-only query allowlists.

Use focused commits and avoid unrelated refactoring. Preserve the repository's existing [Apache License 2.0](LICENSE) and attribution notices. Tagged releases and signed binary publication are separate from ordinary source contributions.
