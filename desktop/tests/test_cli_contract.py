"""! @file
@brief Mutate exact synthetic CLI outputs to test the desktop's schema boundary without native device access.
"""
import copy
import json
import os
import subprocess
import sys
import unittest

VALIDATOR, FIXTURE, DESKTOP = sys.argv[1:4]
del sys.argv[1:4]


def fixture(command):
    """! @brief Capture a production serializer through its explicitly synthetic adapter."""
    args = ["--version"] if command == "version" else [command, "--json"] + (["--all"] if command == "list" else ["complete"])
    return subprocess.run([FIXTURE, *args], capture_output=True, check=True).stdout


def validate(command, body, synthetic=True):
    """! @brief Feed bounded fixture bytes directly to the standalone Qt contract checker."""
    if not isinstance(body, bytes):
        body = json.dumps(body, ensure_ascii=True, separators=(",", ":")).encode()
    return subprocess.run([VALIDATOR, command, str(int(synthetic))], input=body, capture_output=True)


class Contract(unittest.TestCase):
    """! @brief Verify every operation, required field, invariant and stale-selection guard."""

    def test_argv_and_selection(self):
        """! @brief Exercise the enum allowlist and relevant identity comparisons without executing any child CLI."""
        subprocess.run([VALIDATOR, "--self-test"], check=True, capture_output=True)

    def test_desktop_rejects_qt_options(self):
        """! @brief Reject even Qt's own command-line options before initializing the window system."""
        for args in (["-platform", "offscreen"], ["-platformpluginpath", "."], ["--probe"]):
            with self.subTest(args=args):
                result = subprocess.run([DESKTOP, *args], capture_output=True, timeout=5,
                                        env={**os.environ, "QT_QPA_PLATFORM": "offscreen"})
                self.assertEqual(result.returncode, 2)
                self.assertIn(b"Launch without arguments", result.stderr)

    def test_real_serializer_contracts(self):
        """! @brief Accept actual synthetic serializer output and preserve capacities as exact display strings."""
        for command in ("version", "list", "info", "assess", "capabilities", "plan", "simulate"):
            with self.subTest(command=command):
                raw = fixture(command)
                result = validate(command, raw)
                self.assertEqual(result.returncode, 0, result.stderr)
                if command != "version":
                    body = json.loads(result.stdout)
                    device = body["devices"][0] if command in ("list", "info") else body["device"]
                    self.assertEqual(device["size_bytes"], "18446744073709551615")
                    self.assertIsInstance(device["read_only"], bool)
                    self.assertIsNone(device["serial"])
        self.assertEqual(validate("version", b"DiskSan v0.4.0\r\n").returncode, 0)
        self.assertEqual(validate("version", b"DiskSan v0.3.0\n").returncode, 1)

    def test_required_envelope_members(self):
        """! @brief Reject missing fields, unexpected fields and mismatched JSON types for every envelope."""
        for command in ("list", "info", "assess", "capabilities", "plan", "simulate"):
            body = json.loads(fixture(command))
            for key in body:
                with self.subTest(command=command, key=key):
                    changed = copy.deepcopy(body); del changed[key]
                    self.assertEqual(validate(command, changed).returncode, 1)
                    changed = copy.deepcopy(body); changed[key] = {} if isinstance(body[key], list) else []
                    self.assertEqual(validate(command, changed).returncode, 1)
            changed = copy.deepcopy(body); changed["execute"] = True
            self.assertEqual(validate(command, changed).returncode, 1)

    def test_device_types_and_inventory(self):
        """! @brief Require full schema-2 records and reject ambiguous duplicate paths or native identities."""
        body = json.loads(fixture("list"))
        for key, value in body["devices"][0].items():
            with self.subTest(key=key):
                changed = copy.deepcopy(body); del changed["devices"][0][key]
                self.assertEqual(validate("list", changed).returncode, 1)
                changed = copy.deepcopy(body); changed["devices"][0][key] = {} if isinstance(value, list) else []
                self.assertEqual(validate("list", changed).returncode, 1)
        changed = copy.deepcopy(body); changed["devices"] *= 2; changed["shown"] = 2
        self.assertEqual(validate("list", changed).returncode, 1)
        changed["devices"][1] = copy.deepcopy(changed["devices"][1]); changed["devices"][1]["path"] = "different"
        self.assertEqual(validate("list", changed).returncode, 1)
        changed = copy.deepcopy(body); changed["shown"] = 0
        self.assertEqual(validate("list", changed).returncode, 1)
        changed = copy.deepcopy(body); changed["devices"] = []; changed["shown"] = 0
        self.assertEqual(validate("list", changed).returncode, 0)
        self.assertEqual(validate("info", changed).returncode, 1)

    def test_authority_and_operation_claims(self):
        """! @brief Reject probe receipts, stale schema versions and fabricated permissions or completion claims."""
        for command in ("assess", "capabilities", "plan", "simulate"):
            body = json.loads(fixture(command))
            for key, value in [("erase_authorized", True), ("mode", "write"), ("schema_version", "2"),
                               ("version", "0.3.0"), ("operation", "erase"), ("probe", {"requested": True}),
                               ("inspection_status", "safe")]:
                changed = copy.deepcopy(body); changed[key] = value
                self.assertEqual(validate(command, changed).returncode, 1, (command, key))
            for key, value in [("atomic", True), ("guarantees_future_identity", True), ("valid_until", "forever")]:
                changed = copy.deepcopy(body); changed["identity"][key] = value
                self.assertEqual(validate(command, changed).returncode, 1)

    def test_plan_and_simulation_invariants(self):
        """! @brief Enforce fixed steps, unimplemented future requirements and unmistakable fixture provenance."""
        for command in ("plan", "simulate"):
            body = json.loads(fixture(command))
            self.assertEqual(validate(command, body, False).returncode, 1)
            native = copy.deepcopy(body); native["evidence_origin"] = "native_read_only_snapshot"
            self.assertEqual(validate(command, native, False).returncode, 0)
            for key in ("simulation_only", "execution_available", "verification_available", "privilege_unknown"):
                changed = copy.deepcopy(body); changed[key] = not body[key]
                self.assertEqual(validate(command, changed).returncode, 1)
            changed = copy.deepcopy(body); changed["prerequisites"][8]["state"] = "inspection_step_modeled"
            self.assertEqual(validate(command, changed).returncode, 1)
            changed = copy.deepcopy(body); changed["prerequisites"][0]["modeled_only"] = False
            self.assertEqual(validate(command, changed).returncode, 1)
        body = json.loads(fixture("simulate"))
        for key, value in [("state", "erase_complete"), ("index", 0), ("modeled_only", False), ("prerequisite", "execute")]:
            changed = copy.deepcopy(body); changed["steps"][0][key] = value
            self.assertEqual(validate("simulate", changed).returncode, 1)
        changed = copy.deepcopy(body); changed["steps"].reverse()
        self.assertEqual(validate("simulate", changed).returncode, 1)
        changed = copy.deepcopy(body); changed["simulation_result"] = "completed"
        self.assertEqual(validate("simulate", changed).returncode, 1)

    def test_evidence_types(self):
        """! @brief Preserve capability uncertainty while rejecting invalid dimensions, duplicate methods and malformed findings."""
        body = json.loads(fixture("assess"))
        for section, field, value in [("observations", "state", True), ("findings", "severity", "success"),
                                      ("capabilities", "support", "ready"), ("capabilities", "method", "arbitrary")]:
            changed = copy.deepcopy(body); changed[section][0][field] = value
            self.assertEqual(validate("assess", changed).returncode, 1)
        changed = copy.deepcopy(body); changed["capabilities"][1] = changed["capabilities"][0]
        self.assertEqual(validate("assess", changed).returncode, 1)
        changed = copy.deepcopy(body); changed["capabilities"][0]["evidence"][0]["dimension"] = "execute"
        self.assertEqual(validate("assess", changed).returncode, 1)


if __name__ == "__main__":
    unittest.main()
