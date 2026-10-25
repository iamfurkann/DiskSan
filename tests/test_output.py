"""! @file
@brief Test portable output contracts using synthetic records only.
"""
import json
import subprocess
import sys
import unittest

BINARY = sys.argv.pop(1)


def run(mode="hostile", *args):
    """! @brief Run the fixture or native command and capture its output for contract assertions."""
    result = subprocess.run([BINARY, mode, *args], capture_output=True, check=True)
    assert result.stderr == b"", result.stderr
    return result.stdout.decode("utf-8", errors="strict").replace("\r\n", "\n")


class OutputContract(unittest.TestCase):
    """! @brief Verify schema 2 output, bounded display, Unicode handling, and failure propagation."""
    def device(self, mode="hostile"):
        """! @brief Decode the first synthetic device record for focused field assertions."""
        return json.loads(run(mode))["devices"][0]

    def test_envelope(self):
        """! @brief Verify the schema version, application identity, snapshot mode, and record counts."""
        data = json.loads(run())
        self.assertEqual(data["schema_version"], 2)
        self.assertEqual(data["application"], "DiskSan")
        self.assertEqual(data["mode"], "read-only")
        self.assertTrue(data["snapshot"])
        self.assertEqual((data["shown"], data["omitted"]), (1, 0))
        self.assertRegex(data["version"], r"^\d+\.\d+\.\d+$")

    def test_exact_u64(self):
        """! @brief Preserve the maximum uint64_t capacity as an exact JSON integer."""
        self.assertEqual(self.device()["size_bytes"], 2**64 - 1)

    def test_unknown_is_null(self):
        """! @brief Keep unavailable metadata and its evidence source null."""
        d = self.device("unknown")
        for name in ("size_bytes", "serial", "wwn", "eui", "nguid", "logical_sector_size", "read_only"):
            self.assertIsNone(d[name])
            self.assertIsNone(d["sources"][name])

    def test_zero_is_known(self):
        """! @brief Distinguish a sourced zero capacity from unknown metadata."""
        self.assertEqual(self.device("zero")["size_bytes"], 0)
        self.assertEqual(self.device("zero")["sources"]["size_bytes"], "fixture")

    def test_overflow_is_not_zero(self):
        """! @brief Reject overflowing capacity without manufacturing a known zero."""
        d = self.device("overflow")
        self.assertIsNone(d["size_bytes"])
        self.assertIn("invalid_numeric_metadata", d["warnings"])

    def test_utf8_and_control_escaping(self):
        """! @brief Preserve valid Unicode while escaping controls and replacing malformed bytes."""
        d = self.device()
        self.assertIn('quote" slash\\ newline\n tab\t ESC\x1b', d["model"])
        self.assertIn("Unicode: \u00e9 \U0001f600", d["model"])
        self.assertIn("\ufffd", d["model"])
        self.assertEqual(d["path"], "/dev/example\n\x1b[31m")
        self.assertNotIn("\u009b", run())

    def test_empty_inventory(self):
        """! @brief Serialize a valid empty snapshot with zero shown and omitted records."""
        data = json.loads(run("empty"))
        self.assertEqual(data["devices"], [])
        self.assertEqual((data["shown"], data["omitted"]), (0, 0))

    def test_virtual_only(self):
        """! @brief Keep virtio records virtual and avoid inferring physical media."""
        d = self.device("virtual")
        self.assertEqual(d["device_role"], "virtual")
        self.assertEqual(d["transport"], "virtio")
        self.assertEqual(d["media_type"], "unknown")

    def test_default_hides_partitions(self):
        """! @brief Verify that partitions require the explicit all-records filter."""
        data = json.loads(run("partition"))
        self.assertEqual((data["shown"], data["omitted"]), (0, 1))
        data = json.loads(run("partition", "json", "all"))
        self.assertEqual(data["devices"][0]["device_role"], "partition")

    def test_terminal_has_no_metadata_control_characters(self):
        """! @brief Prevent hostile metadata from injecting terminal controls or additional device rows."""
        value = run("hostile", "text")
        self.assertTrue(all(c == "\n" or 32 <= ord(c) < 127 for c in value))
        self.assertIn(r"\x1b", value)
        self.assertEqual(len([line for line in value.splitlines() if line.startswith("/dev/")]), 1)

    def test_long_table_field_bounded(self):
        """! @brief Bound displayed table fields while retaining complete accepted JSON metadata."""
        value = run("long", "text")
        row = next(line for line in value.splitlines() if line.startswith("/dev/"))
        self.assertLess(len(row), 220)
        self.assertIn("...", row)
        self.assertEqual(len(self.device("long")["model"]), 4096)

    def test_detail_safe(self):
        """! @brief Preserve exact capacity and escaped controls in detailed text output."""
        value = run("hostile", "text", "all", "detail")
        self.assertIn("Capacity (bytes):", value)
        self.assertIn("18446744073709551615", value)
        self.assertNotIn("\x1b", value)

    def test_sources_and_arrays(self):
        """! @brief Verify evidence labels, list types, and deterministic warning deduplication."""
        d = self.device()
        self.assertEqual(d["sources"]["model"], "fixture")
        for name in ("stable_paths", "holders", "slaves", "warnings"):
            self.assertIsInstance(d[name], list)
        self.assertEqual(d["warnings"], sorted(set(d["warnings"])))

    def test_determinism(self):
        """! @brief Require identical output for repeated serialization of the same fixture."""
        self.assertEqual(run(), run())

    def test_json_output_failure(self):
        """! @brief Propagate JSON output failure when stdout cannot accept writes."""
        with open(__file__, "rb") as target:
            r = subprocess.run([BINARY], stdout=target, stderr=subprocess.PIPE)
        self.assertEqual(r.returncode, 1)

    def test_utf8_sequence_boundaries(self):
        """! @brief Exercise truncated, overlong, surrogate, out-of-range, and boundary Unicode sequences."""
        samples = [b"\xc2", b"\xe2", b"\xe2\x82", b"\xf0", b"\xf0\x9f", b"\xf0\x9f\x98",
                   b"\xc0\x80", b"\xed\xa0\x80", b"\xf4\x90\x80\x80", bytes(range(1, 256)),
                   "\u0080\u009f\u07ff\u0800\uffff\U00010000\U0010ffff".encode()]
        for sample in samples:
            with self.subTest(sample=sample):
                r = subprocess.run([BINARY, "bytes"], input=sample, capture_output=True, check=True)
                value = json.loads(r.stdout.decode("utf-8", errors="strict"))
                self.assertIsInstance(value["devices"][0]["model"], str)


if __name__ == "__main__":
    unittest.main()
