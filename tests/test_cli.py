"""! @file
@brief Test portable CLI usage, version reporting, and stream errors.
"""
import os
import re
import subprocess
import sys
import unittest

BINARY, PLATFORM = sys.argv[1:3]
del sys.argv[1:3]


def call(*args, **kwargs):
    """! @brief Run the CLI with captured output and caller-supplied subprocess options."""
    return subprocess.run([BINARY, *args], capture_output=True, **kwargs)


class CLI(unittest.TestCase):
    """! @brief Verify public command-line syntax, exit statuses, and output failures."""
    def test_help(self):
        """! @brief Verify that help succeeds, describes read-only behavior, and leaves stderr empty."""
        r = call("--help")
        self.assertEqual(r.returncode, 0)
        self.assertIn(b"read-only", r.stdout)
        self.assertEqual(r.stderr, b"")

    def test_version(self):
        """! @brief Verify semantic version output and a clean success status."""
        r = call("--version")
        self.assertEqual(r.returncode, 0)
        self.assertRegex(r.stdout, rb"^DiskSan v\d+\.\d+\.\d+\r?\n$")
        self.assertEqual(r.stderr, b"")

    def test_usage_errors(self):
        """! @brief Reject unsupported commands, missing selectors, extra arguments, and duplicate flags."""
        cases = [(), ("unknown",), ("erase",), ("sanitize",), ("format",),
                 ("info",), ("info", ""), ("info", "a", "b"),
                 ("list", "a"), ("list", "--wat"), ("--json", "list"),
                 ("list", "--json", "--json"), ("info", "--all", "a"),
                 ("--version", "extra"), ("--help", "extra"),
                 ("info", "a", "--json", "--json"), ("list", "--all", "--all")]
        for args in cases:
            with self.subTest(args=args):
                r = call(*args)
                self.assertEqual(r.returncode, 2)
                self.assertEqual(r.stdout, b"")
                self.assertIn(b"Usage:", r.stderr)

    def test_unsupported_host_is_explicit(self):
        """! @brief Verify explicit discovery failure on hosts without a native adapter."""
        if PLATFORM in ("Linux", "Darwin", "Windows"):
            self.skipTest("Native backend is present")
        for args in [("list",), ("list", "--json"), ("list", "--all"),
                     ("info", "--json", "/dev/example"), ("info", "/dev/example", "--json"),
                     ("info", "--", "-selector")]:
            with self.subTest(args=args):
                r = call(*args)
                self.assertEqual(r.returncode, 1)
                self.assertEqual(r.stdout, b"")
                self.assertIn(b"requires Linux, macOS or Windows", r.stderr)

    def test_output_failure(self):
        # A read-only regular file descriptor cannot accept stdout writes.
        """! @brief Verify that a read-only stdout descriptor produces an output-error status."""
        with open(__file__, "rb") as target:
            r = subprocess.run([BINARY, "--version"], stdout=target, stderr=subprocess.PIPE)
        self.assertEqual(r.returncode, 1)


if __name__ == "__main__":
    unittest.main()
