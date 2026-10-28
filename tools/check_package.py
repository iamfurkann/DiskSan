"""! @file
@brief Inspect an allowlisted runtime archive and smoke-test only its CLI help/version.
"""
import argparse
import hashlib
import os
from pathlib import Path, PurePosixPath
import stat
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def inspect(archive, binary, version, run):
    """! @brief Require exact public files and tested binary bytes before optional native execution."""
    executable = "bin/disksan.exe" if binary.suffix.lower() == ".exe" else "bin/disksan"
    modes = {}
    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as packed:
            assert packed.testzip() is None, "Archive CRC failed"
            entries = {}
            for item in packed.infolist():
                if item.is_dir():
                    continue
                assert item.filename not in entries, "Duplicate archive file"
                mode = item.external_attr >> 16
                assert stat.S_IFMT(mode) in (0, stat.S_IFREG), "Unexpected archive link or special file"
                entries[item.filename] = packed.read(item)
                modes[item.filename] = mode
    else:
        with tarfile.open(archive, "r:gz") as packed:
            entries = {}
            for member in packed.getmembers():
                assert member.isfile() or member.isdir(), "Unexpected archive link or special file"
                if member.isfile():
                    assert member.name not in entries, "Duplicate archive file"
                    entries[member.name] = packed.extractfile(member).read()
                    modes[member.name] = member.mode
    assert all(not PurePosixPath(name).is_absolute() and ".." not in PurePosixPath(name).parts
               for name in entries), "Unsafe archive path"
    roots = {PurePosixPath(name).parts[0] for name in entries}
    assert len(roots) == 1, "Archive must have one top-level directory"
    prefix = roots.pop() + "/"
    entries = {name.removeprefix(prefix): data for name, data in entries.items()}
    modes = {name.removeprefix(prefix): mode for name, mode in modes.items()}
    docs = ["README.md", "CHANGELOG.md", "CONTRIBUTING.md", "SECURITY.md", "LICENSE"]
    expected = {"share/doc/disksan/" + name: (ROOT / name).read_bytes() for name in docs}
    for path in (ROOT / "docs").rglob("*"):
        if path.is_file() and (path.suffix == ".md" or path.name == "FILE_MANIFEST.txt"):
            relative = path.relative_to(ROOT / "docs").as_posix()
            expected["share/doc/disksan/docs/" + relative] = path.read_bytes()
    expected[executable] = binary.read_bytes()
    missing = sorted(set(expected) - set(entries))
    extra = sorted(set(entries) - set(expected))
    changed = sorted(name for name in set(entries) & set(expected) if entries[name] != expected[name])
    assert not (missing or extra or changed), f"Archive mismatch: missing={missing}, extra={extra}, changed={changed}"
    if executable == "bin/disksan":
        assert modes[executable] & stat.S_IXUSR, "Packaged Unix binary is not executable"
    if run:
        with tempfile.TemporaryDirectory(prefix="disksan-package-") as temporary:
            target = Path(temporary) / Path(executable).name
            target.write_bytes(entries[executable])
            target.chmod(0o755)
            env = os.environ.copy()
            env.pop("LD_LIBRARY_PATH", None)
            result = subprocess.run([str(target), "--version"], env=env, capture_output=True, check=True, text=True)
            assert result.stdout.strip() == "DiskSan v" + version and not result.stderr
            result = subprocess.run([str(target), "--help"], env=env, capture_output=True, check=True, text=True)
            assert "read-only" in result.stdout and not result.stderr
    print(f"Package OK: {archive.name}; {len(entries)} files; native smoke={run}; SHA256={hashlib.sha256(archive.read_bytes()).hexdigest()}")


def main():
    """! @brief Build an isolated archive or inspect a supplied artifact with explicit execution policy."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path)
    parser.add_argument("--archive", type=Path)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--version", required=True)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--no-run", action="store_true")
    args = parser.parse_args()
    if args.archive:
        inspect(args.archive, args.binary, args.version, not args.no_run)
    else:
        assert args.build
        with tempfile.TemporaryDirectory(prefix="package-check-", dir=args.build) as temporary:
            subprocess.run(["cpack", "--config", str(args.build / "CPackConfig.cmake"), "-C", args.config, "-B", temporary], check=True)
            archives = list(Path(temporary).glob("*.zip")) + list(Path(temporary).glob("*.tar.gz"))
            assert len(archives) == 1
            inspect(archives[0], args.binary, args.version, not args.no_run)


if __name__ == "__main__":
    main()
