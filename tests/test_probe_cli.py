"""! @file
@brief Exercise production opt-in parsing and output exclusively with synthetic native adapters.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY, FIXTURE = sys.argv[1:3]
del sys.argv[1:3]


def invoke(*args, env=None):
    """! @brief Run the synthetic CLI and capture output without accessing any physical device."""
    return subprocess.run([FIXTURE, *args], capture_output=True, env=env)


class ProbeCLI(unittest.TestCase):
    """! @brief Validate explicit-only probing, stable defaults and honest independent evidence."""

    def test_option_order(self):
        """! @brief Accept one probe option before or after the selector, preserving output order."""
        expected = invoke("capabilities", "complete", "--probe", "--json")
        self.assertEqual(expected.returncode, 0, expected.stderr)
        for args in [
            ("--json", "--probe", "complete"), ("--probe", "complete", "--json"),
            ("--probe", "--json", "--", "complete"), ("complete", "--json", "--probe"),
            ("--probe", "--json", "--", "--probe"),
        ]:
            with self.subTest(args=args):
                result = invoke("capabilities", *args)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, expected.stdout)

    def test_forbidden_grammar(self):
        """! @brief Reject other commands, duplicates and raw-command options before discovery."""
        invalid = [
            ("list", "--probe"), ("info", "never-resolve", "--probe"),
            ("assess", "--probe", "never-resolve"), ("capabilities", "--probe"),
            ("capabilities", "never-resolve", "--probe", "--probe"),
            ("capabilities", "--probe", "--", "never-resolve", "--json"),
            ("capabilities", "--probe=true", "never-resolve"),
            ("capabilities", "--probe", "never-resolve", "extra"),
            ("capabilities", "--probe", "--json", "--json", "never-resolve"),
        ]
        for option in ("--opcode", "--cns", "--nsid", "--timeout", "--data-length", "--ioctl", "--command", "--feature", "--log-page"):
            invalid.append(("capabilities", "never-resolve", "--probe", option, "0x84"))
        for command in ("sanitize", "erase", "format", "discard", "reset", "set-feature", "firmware-download",
                        "firmware-commit", "security-send", "security-receive", "namespace-management", "self-test", "read", "write"):
            invalid.append((command, "never-resolve", "--probe"))
        for args in invalid:
            with self.subTest(args=args):
                result = invoke(*args)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, b"")

    def test_default_never_probes(self):
        """! @brief Require explicit CLI opt-in even when environment variables request probing."""
        environment = dict(os.environ, DISKSAN_PROBE="1", DISKSAN_AUTO_PROBE="true")
        for command in ("assess", "capabilities"):
            result = invoke(command, "no-probe", "--json", env=environment)
            self.assertEqual(result.returncode, 0, result.stderr)
            body = json.loads(result.stdout)
            self.assertNotIn("probe", body)
            for method in body["capabilities"]:
                for dimension in ("support", "os_access", "applicability", "passthrough"):
                    self.assertEqual(method[dimension], "unknown")
        for args in [("list", "--json"), ("info", "no-probe", "--json")]:
            self.assertEqual(invoke(*args, env=environment).returncode, 0)
        literal = invoke("capabilities", "--json", "--", "--probe")
        self.assertEqual(literal.returncode, 0)
        self.assertNotIn("probe", json.loads(literal.stdout))

    def test_complete_evidence(self):
        """! @brief Establish only documented support bits while preserving identity limits and false authorization."""
        result = invoke("capabilities", "complete", "--probe", "--json")
        self.assertEqual(result.returncode, 0)
        self.assertNotIn(b"PRIVATE-RAW-IDENTIFY", result.stdout + result.stderr)
        body = json.loads(result.stdout)
        self.assertEqual(body["schema_version"], 2)
        self.assertEqual(body["operation"], "capabilities")
        self.assertIs(body["erase_authorized"], False)
        self.assertIs(body["identity"]["atomic"], False)
        self.assertIs(body["identity"]["guarantees_future_identity"], False)
        self.assertEqual(body["identity"]["valid_until"], "end_of_observation")
        self.assertEqual(body["inspection_status"], "blocked")
        self.assertEqual(body["probe"], {"requested": True, "performed": True, "query": "nvme_identify_controller",
                                      "provider": "linux_nvme_identify_controller", "status": "complete", "reason": "none"})
        expected = ["observed_supported", "observed_supported", "observed_unsupported", "unknown", "unknown", "unknown"]
        self.assertEqual([m["support"] for m in body["capabilities"]], expected)
        for method in body["capabilities"]:
            self.assertEqual(len(method["evidence"]), 4)
            for dimension in ("os_access", "applicability", "passthrough"):
                self.assertEqual(method[dimension], "unknown")

    def test_unavailable_and_invalid(self):
        """! @brief Distinguish declined, denied, failed and malformed probes without inventing unsupported hardware."""
        for selector, status, performed in [
            ("unavailable", "unavailable", False), ("denied", "unavailable", False),
            ("query-denied", "unavailable", True), ("query-error", "unavailable", True),
            ("version", "unavailable", True), ("bad-reply", "invalid_response", True),
        ]:
            with self.subTest(selector=selector):
                result = invoke("capabilities", selector, "--probe", "--json")
                self.assertEqual(result.returncode, 0, result.stderr)
                body = json.loads(result.stdout)
                self.assertEqual(body["probe"]["status"], status)
                self.assertIs(body["probe"]["performed"], performed)
                self.assertTrue(all(m["support"] == "unknown" for m in body["capabilities"]))
                for i, method in enumerate(body["capabilities"]):
                    self.assertEqual(method["os_access"], "unavailable" if selector in ("denied", "query-denied") and i < 3 else "unknown")
                    self.assertEqual(method["applicability"], "unknown")
                    self.assertEqual(method["passthrough"], "unknown")

    def test_no_success_on_failure(self):
        """! @brief Drop all output on invalid selection, failed provider, changed identity or invalid receipt."""
        for selector, code in [("missing", 3), ("regular-file", 3), ("directory", 3), ("permission", 4),
                               ("probe-changed", 3), ("probe-fatal", 1), ("bad-receipt", 1),
                               ("bad-decoded", 1), ("changed", 3), ("after-missing", 3), ("after-empty", 3)]:
            result = invoke("capabilities", selector, "--probe", "--json")
            self.assertEqual(result.returncode, code)
            self.assertEqual(result.stdout, b"")

    def test_real_invalid_selectors(self):
        """! @brief Reject disposable files, directories and false aliases through the actual native CLI before querying."""
        if sys.platform not in ("linux", "darwin", "win32"):
            self.skipTest("No native selector adapter on this platform")
        with tempfile.TemporaryDirectory(prefix="disksan-probe-selector-") as directory:
            root = Path(directory)
            regular = root / "ordinary-file"
            regular.write_bytes(b"synthetic fixture; never device data")
            selectors = [regular, root, root / "missing"]
            if os.name != "nt":
                alias = root / "false-alias"
                alias.symlink_to(regular)
                broken = root / "broken-alias"
                broken.symlink_to(root / "missing")
                selectors.extend([alias, broken])
            for selector in selectors:
                result = subprocess.run([BINARY, "capabilities", "--json", "--probe", str(selector)], capture_output=True)
                self.assertEqual(result.returncode, 3, result.stderr)
                self.assertEqual(result.stdout, b"")
            self.assertEqual(regular.read_bytes(), b"synthetic fixture; never device data")

    def test_text_and_help(self):
        """! @brief Explain probe status in text and advertise explicit-only querying in production help."""
        result = invoke("capabilities", "complete", "--probe")
        self.assertEqual(result.returncode, 0)
        self.assertIn(b"Protocol probe: complete", result.stdout)
        self.assertIn(b"Erase authorized: NO", result.stdout)
        self.assertNotIn(b"no commands are issued", result.stdout)
        help_result = subprocess.run([BINARY, "capabilities", "--help"], capture_output=True)
        self.assertEqual(help_result.returncode, 0)
        self.assertIn(b"--probe", help_result.stdout)

    def test_output_failure(self):
        """! @brief Propagate output failure using a read-only disposable file as the child stdout handle."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "read-only-output"
            path.write_bytes(b"fixture")
            with path.open("rb") as sink:
                result = subprocess.run([FIXTURE, "capabilities", "complete", "--probe", "--json"], stdout=sink, stderr=subprocess.PIPE)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(path.read_bytes(), b"fixture")


if __name__ == "__main__":
    unittest.main()
