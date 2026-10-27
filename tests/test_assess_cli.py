"""! @file
@brief Verify production assessment CLI/output with synthetic adapters and safe invalid selectors.
"""
import json
import subprocess
import sys
import tempfile
import unittest

BINARY, FIXTURE, OUTPUT, PLATFORM = sys.argv[1:5]
del sys.argv[1:5]


def invoke(binary, *args):
    """! @brief Capture a CLI result without logging native identities or inventories."""
    return subprocess.run([binary, *args], capture_output=True)


class AssessmentCLI(unittest.TestCase):
    """! @brief Assert command, schema, safety, compatibility and failure contracts end to end."""
    def test_help_and_usage(self):
        """! @brief Require explicit new-command help and reject ambiguous or duplicate syntax."""
        for command in ("assess", "capabilities"):
            result = invoke(BINARY, command, "--help")
            self.assertEqual(result.returncode, 0)
            self.assertIn(b"never authorize", result.stdout)
            for args in [(), ("",), ("a", "b"), ("--all", "a"), ("--json", "--json", "a"), ("--wat",), ("--help", "a")]:
                with self.subTest(command=command, args=args):
                    result = invoke(BINARY, command, *args)
                    self.assertEqual(result.returncode, 2)
                    self.assertEqual(result.stdout, b"")

    def test_existing_device_schema(self):
        """! @brief Preserve the complete schema-2 record within the new operation envelope."""
        old = json.loads(invoke(FIXTURE, "info", "stable", "--json").stdout)
        result = invoke(FIXTURE, "assess", "--json", "stable")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stderr, b"")
        data = json.loads(result.stdout.decode("utf-8", errors="strict"))
        self.assertEqual(data["device"], old["devices"][0])
        self.assertEqual(data["schema_version"], 2)
        self.assertEqual(data["mode"], "read-only")
        self.assertEqual(data["device"]["size_bytes"], 2**64 - 1)
        self.assertIn("\ufffd", data["device"]["model"])
        self.assertEqual(data["inspection_status"], "blocked")
        self.assertFalse(data["erase_authorized"])
        self.assertFalse(data["identity"]["guarantees_future_identity"])
        self.assertFalse(data["identity"]["atomic"])
        self.assertEqual(data["identity"]["valid_until"], "end_of_observation")
        self.assertTrue(any(f["code"] == "mounted_dependency" for f in data["findings"]))
        self.assertTrue(any(f["code"] == "snapshot_not_authorization" for f in data["findings"]))
        self.assertTrue(all(f["source"] and f["subject"] and f["message"] for f in data["findings"]))
        self.assertTrue(all(e["source"] and e["detail"] for e in data["observations"]))

    def test_capabilities_retains_safety(self):
        """! @brief Keep identical safety findings and method evidence in either operation."""
        a = json.loads(invoke(FIXTURE, "assess", "stable", "--json").stdout)
        b = json.loads(invoke(FIXTURE, "capabilities", "stable", "--json").stdout)
        self.assertEqual(b["operation"], "capabilities")
        self.assertEqual(b["assessment_kind"], "capability-evidence")
        for key in ("findings", "observations", "device", "identity", "capabilities", "erase_authorized"):
            self.assertEqual(a[key], b[key])
        self.assertEqual(len(b["capabilities"]), 6)
        for method in b["capabilities"]:
            for dimension in ("support", "os_access", "applicability", "passthrough"):
                self.assertEqual(method[dimension], "unknown")
            self.assertEqual(len(method["evidence"]), 4)

    def test_stability_and_option_termination(self):
        """! @brief Require deterministic output and treat tokens after -- as selectors."""
        a = invoke(FIXTURE, "assess", "--json", "stable")
        b = invoke(FIXTURE, "assess", "stable", "--json")
        self.assertEqual(a.stdout, b.stdout)
        self.assertEqual(a.stdout, invoke(FIXTURE, "assess", "--json", "stable").stdout)
        self.assertEqual(invoke(FIXTURE, "assess", "--json", "--", "--json").returncode, 0)
        self.assertEqual(invoke(FIXTURE, "assess", "--", "stable", "--json").returncode, 2)

    def test_native_failures_never_emit_success(self):
        """! @brief Preserve error classes and suppress stdout for changed, missing or failed targets."""
        for command in ("assess", "capabilities"):
            for selector, status in [("changed", 3), ("missing", 3), ("denied", 4), ("system-failure", 1), ("usage-failure", 1)]:
                with self.subTest(command=command, selector=selector):
                    result = invoke(FIXTURE, command, "--json", selector)
                    self.assertEqual(result.returncode, status)
                    self.assertEqual(result.stdout, b"")
                    self.assertTrue(result.stderr)

    def test_regular_file_rejected(self):
        """! @brief Reject ordinary files before any native device-data inspection."""
        with tempfile.NamedTemporaryFile(prefix="disksan-selector-") as target:
            for command in ("info", "assess", "capabilities"):
                result = invoke(BINARY, command, "--json", target.name)
                self.assertEqual(result.returncode, 3 if PLATFORM in ("Linux", "Darwin", "Windows") else 1)
                self.assertEqual(result.stdout, b"")

    def test_partial_identity(self):
        """! @brief Keep missing identity evidence partial without upgrading it to verification."""
        result = invoke(FIXTURE, "assess", "partial", "--json")
        self.assertEqual(result.returncode, 0)
        data = json.loads(result.stdout)
        self.assertEqual(data["identity"]["status"], "partially_verified")
        self.assertFalse(data["erase_authorized"])

    def test_terminal_escaping(self):
        """! @brief Prevent metadata and evidence from injecting terminal control sequences."""
        result = invoke(FIXTURE, "assess", "stable")
        self.assertEqual(result.returncode, 0)
        self.assertNotIn(b"\x1b", result.stdout)
        self.assertIn(b"Erase authorized: NO", result.stdout)
        self.assertIn(b"Observed evidence:", result.stdout)

    def test_stream_failure(self):
        """! @brief Require nonzero status when assessment output inherits a read-only stream."""
        with open(__file__, "rb") as target:
            result = subprocess.run([FIXTURE, "assess", "stable", "--json"], stdout=target, stderr=subprocess.PIPE)
        self.assertEqual(result.returncode, 1)

    def test_capability_states_and_conflicts(self):
        """! @brief Preserve observed method states while keeping access independent and authorization false."""
        for mode, expected in [("unknown", "unknown"), ("supported", "observed_supported"), ("unsupported", "observed_unsupported"), ("unavailable", "unavailable"), ("conflict", "unknown")]:
            data = json.loads(invoke(OUTPUT, mode).stdout)
            method = data["capabilities"][0]
            self.assertEqual(method["support"], expected)
            self.assertEqual(method["os_access"], "unknown")
            self.assertFalse(data["erase_authorized"])
            self.assertEqual(data["inspection_status"], "review_required")
            self.assertEqual(len(method["evidence"]), 2 if mode == "conflict" else 1)

    def test_poisoned_output(self):
        """! @brief Refuse unevaluated or failed evidence before writing a JSON prefix."""
        for mode in ("failed", "unevaluated"):
            result = invoke(OUTPUT, mode)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(result.stdout, b"")


if __name__ == "__main__":
    unittest.main()
