"""! @file
@brief Enforce the maintained source manifest and English documentation conventions.
"""
import ast
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/FILE_MANIFEST.txt"
C_FUNCTION = re.compile(
    r"^(?:static\s+)?(?:[A-Za-z_]\w*\s+|\*\s*)+?"
    r"(?P<name>[A-Za-z_]\w*)\s*\((?P<args>[^;{}]*)\)\s*(?=[{;])",
    re.MULTILINE,
)
BRIEF = re.compile(r"@brief\s+[^\s*]")


def source_paths(root):
    """! @brief Discover maintained text files without traversing generated or private directories."""
    paths = set(root.glob("*.md"))
    paths.update(root / name for name in (".gitignore", "CMakeLists.txt", "Doxyfile", "LICENSE"))
    for directory in ("src", "include", "tests", "tools", "cmake", "docs"):
        for path in (root / directory).rglob("*"):
            if path.is_file() and "__pycache__" not in path.parts and path.name != ".DS_Store":
                paths.add(path)
    return sorted(path.relative_to(root).as_posix() for path in paths)


def preceding_brief(text, offset):
    """! @brief Find the immediately preceding Doxygen block for a conventional C declaration."""
    prefix = text[:offset].rstrip()
    if not prefix.endswith("*/"):
        return None
    start = prefix.rfind("/**")
    if start < 0:
        return None
    block = prefix[start:]
    if "*/" in block[:-2] or not BRIEF.search(block):
        return None
    return block


def check_c(name, text):
    """! @brief Check C file/function briefs and public header parameter and return contracts."""
    errors = []
    first = re.match(r"\s*/\*\*.*?\*/", text, re.DOTALL)
    if not first or "@file" not in first.group() or not BRIEF.search(first.group()):
        errors.append(f"{name}: missing file-level @file/@brief")
    functions = list(C_FUNCTION.finditer(text))
    for match in functions:
        line = text.count("\n", 0, match.start()) + 1
        location = f"{name}:{line}: {match['name']}"
        block = preceding_brief(text, match.start())
        if not block:
            errors.append(f"{location}: missing immediately preceding @brief")
            continue
        if name.startswith("include/"):
            for arg in match["args"].split(","):
                arg = arg.strip()
                if arg in ("", "void", "..."):
                    continue
                parameter = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^]]*\])?$", arg)
                if not parameter:
                    errors.append(f"{location}: unsupported parameter declaration {arg!r}")
                elif not re.search(r"@param(?:\[[^]]*\])?\s+" + parameter[1] + r"\b", block):
                    errors.append(f"{location}: missing @param for {parameter[1]}")
            result_type = match.group().split(match["name"], 1)[0].strip()
            if result_type != "void" and not re.search(r"@returns?\b|@retval\b", block):
                errors.append(f"{location}: missing @return contract")
    return errors, len(functions)


def check_python(name, text):
    """! @brief Inspect Python AST docstrings for module, class, function, and method briefs."""
    errors = []
    try:
        tree = ast.parse(text, filename=name)
    except SyntaxError as error:
        return [f"{name}: {error}"], 0
    module = ast.get_docstring(tree) or ""
    if "@file" not in module or not BRIEF.search(module):
        errors.append(f"{name}: missing module @file/@brief")
    nodes = [node for node in ast.walk(tree)
             if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef))]
    for node in nodes:
        if not BRIEF.search(ast.get_docstring(node) or ""):
            errors.append(f"{name}:{node.lineno}: {node.name}: missing @brief docstring")
    return errors, len(nodes)


def main():
    """! @brief Check manifest completeness and documentation, returning nonzero on violations."""
    names = MANIFEST.read_text(encoding="utf-8").splitlines()
    actual = source_paths(ROOT)
    errors = []
    if names != sorted(set(names)):
        errors.append("Source manifest must be sorted with no duplicate entries.")
    if set(names) != set(actual):
        errors.append(f"Manifest missing: {sorted(set(actual) - set(names))}")
        errors.append(f"Manifest stale: {sorted(set(names) - set(actual))}")
    c_count = python_count = 0
    for name in actual:
        path = ROOT / name
        if path.is_symlink():
            errors.append(f"{name}: source symlinks are not allowed")
            continue
        text = path.read_text(encoding="utf-8")
        for number, line in enumerate(text.splitlines(), 1):
            if not line.isascii():
                errors.append(f"{name}:{number}: use ASCII English prose or escaped Unicode test data")
        if path.suffix in (".c", ".h") or name.endswith(".h.in"):
            found, count = check_c(name, text)
            errors.extend(found)
            c_count += count
        elif path.suffix == ".py":
            found, count = check_python(name, text)
            errors.extend(found)
            python_count += count
        elif name in ("CMakeLists.txt", "Doxyfile") or path.suffix == ".cmake":
            header = "\n".join(text.splitlines()[:4])
            if "@file" not in header or not BRIEF.search(header):
                errors.append(f"{name}: missing configuration @file/@brief")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"Documentation OK: {len(actual)} files, {c_count} C declarations/definitions, "
          f"{python_count} Python classes/functions; manifest and ASCII policy passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
