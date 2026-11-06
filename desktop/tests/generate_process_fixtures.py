"""! @file
@brief Generate build-local process fixtures exclusively from the synthetic CLI serializer.
"""
import copy
import json
from pathlib import Path
import subprocess
import sys


def main():
    """! @brief Save exact synthetic version and schema documents without inspecting any native device."""
    binary, directory = sys.argv[1:3]
    target = Path(directory); target.mkdir(parents=True, exist_ok=True)
    for command in ("version", "list", "info", "assess", "capabilities", "plan", "simulate"):
        args = ["--version"] if command == "version" else [command, "--json"] + (["--all"] if command == "list" else ["complete"])
        raw = subprocess.run([binary, *args], check=True, capture_output=True).stdout
        (target / (command + ".json")).write_bytes(raw)
        if command == "list":
            body = json.loads(raw)
            for selector, identity in (("second", "synthetic#diskseq=8"), ("--probe", "synthetic#diskseq=9")):
                extra = copy.deepcopy(body["devices"][0]); extra["path"] = selector; extra["device_id"] = identity
                body["devices"].append(extra)
            body["shown"] = len(body["devices"])
            (target / "list.json").write_text(json.dumps(body), encoding="ascii")
            body["devices"] = []; body["shown"] = 0
            (target / "empty.json").write_text(json.dumps(body), encoding="ascii")


if __name__ == "__main__":
    main()
