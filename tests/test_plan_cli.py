"""! @file
@brief Test inspection-plan CLI, exact additive JSON and failure paths using synthetic adapters and disposable files.
"""
import json
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY, FIXTURE, FAULT_FIXTURE = sys.argv[1:4]
del sys.argv[1:4]


def invoke(*args, env=None):
    """! @brief Run the production parser with synthetic native boundaries and a fatal protocol-call guard."""
    return subprocess.run([FIXTURE, *args], capture_output=True, env=env)


class PlanCLI(unittest.TestCase):
    """! @brief Distinguish completed inspections, unknown evidence and fatal failures without operational claims."""

    command = "plan"

    def test_positive_contract(self):
        """! @brief Even complete positive fixture evidence remains a non-executable schema-2 plan."""
        result = invoke(self.command, "complete", "--json")
        self.assertEqual(result.returncode, 0, result.stderr)
        body = json.loads(result.stdout)
        for name, value in {"schema_version": 2, "application": "DiskSan", "operation": self.command,
                            "mode": "read-only", "inspection_status": "inspection_only",
                            "evidence_origin": "synthetic_fixture"}.items():
            self.assertEqual(body[name], value)
        for name in ("execution_available", "erase_authorized", "verification_available"):
            self.assertIs(body[name], False)
        self.assertIs(body["simulation_only"], True)
        self.assertIs(body["privilege_unknown"], True)
        self.assertIs(body["identity"]["atomic"], False)
        self.assertIs(body["identity"]["guarantees_future_identity"], False)
        self.assertEqual(body["identity"]["valid_until"], "end_of_observation")
        self.assertEqual(body["device"]["size_bytes"], 2**64 - 1)
        self.assertEqual(len(body["method_evidence"]), 24)
        self.assertEqual(len(body["prerequisites"]), 10)
        for requirement in body["prerequisites"]:
            self.assertIs(requirement["modeled_only"], True)
            self.assertTrue(requirement["evidence_scope"])
        self.assertTrue(all(p["state"] == "not_implemented" for p in body["prerequisites"][5:]))
        info = json.loads(invoke("info", "complete", "--json").stdout)
        self.assertEqual(body["device"], info["devices"][0])
        self.assertNotIn("probe", body)
        if self.command == "plan":
            self.assertEqual(body["steps"], [])
            self.assertNotIn("simulation_result", body)
        else:
            self.assertEqual(body["simulation_result"], "non_executable_model_only")
            self.assertEqual(len(body["steps"]), 10)

    def test_options_and_literal_selectors(self):
        """! @brief Preserve legal option order and treat literal selectors as a single unexecuted argument."""
        baseline = invoke(self.command, "complete", "--json")
        for args in [("--json", "complete"), ("--json", "--", "complete")]:
            result = invoke(self.command, *args)
            self.assertEqual((result.returncode, result.stdout), (0, baseline.stdout))
        for selector in ("--probe", "--execute", "space in name", "$(false);`false`", "unicode-\u00e9", "line\n\x1b[31m"):
            result = invoke(self.command, "--json", "--", selector)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout)["device"]["path"], selector)

    def test_rejected_grammar(self):
        """! @brief Reject every execution/protocol/raw argument before discovery can occur."""
        cases = [(self.command,), (self.command, "--json"), (self.command, "never-discover", "extra"),
                 (self.command, "--json", "never-discover", "--json"), (self.command, "--", "never-discover", "--json")]
        for option in ("--probe", "--method", "--force", "--execute", "--yes", "--sudo", "--opcode", "--cns",
                       "--nsid", "--data-length", "--timeout", "--ioctl", "--command", "--sector", "--all"):
            cases.extend([(self.command, option, "never-discover"), (self.command, "never-discover", option)])
        for args in cases:
            with self.subTest(args=args):
                result = invoke(*args)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, b"")

    def test_hazards_and_unknowns(self):
        """! @brief Known consumers block while missing scope and identity remain explicit uncertainty."""
        states = {"mounted": "blocked", "system": "blocked", "swap": "blocked", "logical": "blocked",
                  "read-only": "blocked", "conflict": "blocked", "review": "review_required",
                  "unknown-capabilities": "review_required", "insufficient": "insufficient_evidence",
                  "unknown": "insufficient_evidence"}
        for selector, expected in states.items():
            result = invoke(self.command, "--json", selector)
            self.assertEqual(result.returncode, 0, result.stderr)
            body = json.loads(result.stdout)
            self.assertEqual(body["inspection_status"], expected, selector)
            self.assertIs(body["erase_authorized"], False)
        for selector, expected in [("unsupported", "observed_unsupported"), ("unavailable", "unavailable")]:
            body = json.loads(invoke(self.command, selector, "--json").stdout)
            self.assertIn(expected, [e["state"] for e in body["method_evidence"]])

    def test_failures_publish_no_success(self):
        """! @brief Preserve native exit classes and discard changed, lost or incomplete target windows."""
        for selector, code in [("missing", 3), ("denied", 4), ("system-failure", 1), ("usage-failure", 1),
                               ("cap-failure", 1), ("changed", 3), ("path-reused", 3), ("after-missing", 3), ("after-empty", 3)]:
            result = invoke(self.command, selector, "--json")
            self.assertEqual(result.returncode, code, result.stderr)
            self.assertEqual(result.stdout, b"")

    def test_no_environment_opt_in(self):
        """! @brief Environment text cannot enable probing or turn a plan into an operation."""
        expected = invoke(self.command, "complete", "--json")
        result = invoke(self.command, "complete", "--json", env=dict(os.environ, DISKSAN_PROBE="1", DISKSAN_EXECUTE="1"))
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, expected.stdout, b""))

    def test_protocol_tripwire(self):
        """! @brief Verify the fixture's protocol boundary is live while plan/simulation paths leave it untouched."""
        result = invoke("capabilities", "complete", "--probe", "--json")
        self.assertEqual(result.returncode, 99)
        self.assertEqual(result.stdout, b"")
        self.assertIn(b"Unexpected native protocol invocation", result.stderr)

    def test_allocation_failures(self):
        """! @brief Exhaust every core allocation in both snapshots and require no success payload and no leaked blocks on failure."""
        normal = invoke(self.command, "complete", "--json")
        baseline = subprocess.run([FAULT_FIXTURE, self.command, "complete", "--json"], capture_output=True,
                                  env=dict(os.environ, DISKSAN_TEST_ALLOC_POINT="1000"))
        self.assertEqual((baseline.returncode, baseline.stdout), (0, normal.stdout))
        match = re.search(rb"Fixture allocations: attempts=(\d+) live=0", baseline.stderr)
        self.assertIsNotNone(match)
        count = int(match[1]); self.assertGreater(count, 0); self.assertLess(count, 1000)
        for point in range(count):
            with self.subTest(point=point):
                result = subprocess.run([FAULT_FIXTURE, self.command, "complete", "--json"], capture_output=True,
                                        env=dict(os.environ, DISKSAN_TEST_ALLOC_POINT=str(point)))
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertEqual(result.stdout, b"")
                self.assertRegex(result.stderr, rb"Fixture allocations: attempts=\d+ live=0")

    def test_real_invalid_targets(self):
        """! @brief Exercise native selection rejection only on disposable ordinary files and directories."""
        if sys.platform not in ("darwin", "linux", "win32"):
            self.skipTest("No native adapter on this host")
        with tempfile.TemporaryDirectory(prefix="disksan-plan-") as temporary:
            root = Path(temporary); file = root / "ordinary-file"; file.write_bytes(b"fixture")
            for path in (root, file, root / "missing"):
                result = subprocess.run([BINARY, self.command, "--json", "--", str(path)], capture_output=True)
                self.assertEqual(result.returncode, 3, result.stderr)
                self.assertEqual(result.stdout, b"")
            self.assertEqual(file.read_bytes(), b"fixture")

    def test_terminal_and_output_failure(self):
        """! @brief Escape metadata controls, label synthetic models and propagate actual stream failure."""
        result = invoke(self.command, "--", "line\n\x1b[31m")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"SIMULATION ONLY - DOES NOT ERASE DATA", result.stdout)
        self.assertIn(b"DEMO DATA", result.stdout)
        self.assertNotIn(b"\x1b", result.stdout)
        self.assertIn(b"Execution available: NO", result.stdout)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "output"; path.write_bytes(b"unchanged")
            with path.open("rb") as sink:
                failed = subprocess.run([FIXTURE, self.command, "complete", "--json"], stdout=sink, stderr=subprocess.PIPE)
            self.assertEqual(failed.returncode, 1)
            self.assertEqual(path.read_bytes(), b"unchanged")


class SimulationCLI(PlanCLI):
    """! @brief Apply every parser, failure, scope and output contract to the non-executable simulator as well."""

    command = "simulate"

    def test_deterministic_transcript(self):
        """! @brief Require fixed bounded step order and stable bytes while deriving all states from the same inspection facts."""
        expected_codes = ["observe_target", "compare_identity", "review_usage", "review_topology", "review_method_evidence",
                          "privilege_boundary_unimplemented", "authorization_boundary_unimplemented", "exclusive_scope_unimplemented",
                          "operation_backend_unavailable", "completion_verifier_unavailable"]
        for selector in ("complete", "mounted", "system", "swap", "conflict", "logical", "review", "unknown-capabilities", "insufficient"):
            result = invoke(self.command, selector, "--json")
            self.assertEqual(result.returncode, 0, result.stderr)
            for _ in range(3):
                again = invoke(self.command, selector, "--json")
                self.assertEqual((again.returncode, again.stdout, again.stderr), (0, result.stdout, b""))
            body = json.loads(result.stdout)
            plan = json.loads(invoke("plan", selector, "--json").stdout)
            for key in plan:
                if key not in ("operation", "steps"):
                    self.assertEqual(body[key], plan[key], key)
            self.assertEqual([s["code"] for s in body["steps"]], expected_codes)
            for index, (step, prerequisite) in enumerate(zip(body["steps"], body["prerequisites"]), 1):
                self.assertEqual(step["index"], index)
                self.assertIs(step["modeled_only"], True)
                self.assertEqual(step["prerequisite"], prerequisite["code"])
                self.assertEqual(step["state"], prerequisite["state"])
                self.assertEqual(step["evidence_scope"], prerequisite["evidence_scope"])
                self.assertIn(step["state"], ("would_block", "unknown_requires_review", "not_implemented", "inspection_step_modeled"))
            self.assertTrue(all(s["state"] == "not_implemented" for s in body["steps"][5:]))
            if selector in ("mounted", "system", "swap", "conflict"):
                self.assertEqual(body["steps"][2]["state"], "would_block")
        text = invoke(self.command, "complete").stdout
        self.assertIn(b"non_executable_model_only", text)
        self.assertIn(b"no operation was attempted", text)


if __name__ == "__main__":
    unittest.main()
