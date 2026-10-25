"""! @file
@brief Define native Windows acceptance tests pending execution on a Windows host.
"""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY = sys.argv.pop(1)

def run(*args):
    """! @brief Run the fixture or native command and capture its output for contract assertions."""
    return subprocess.run([BINARY, *args], capture_output=True, text=True, encoding="utf-8")

class WindowsInventory(unittest.TestCase):
    """! @brief Verify native Windows identities, filtering, and invalid-selector handling."""
    def test_inventory(self):
        """! @brief Verify Windows native identities and round-trip each discovered path through info."""
        r = run("list", "--all", "--json")
        self.assertEqual(r.returncode, 0, r.stderr)
        data = json.loads(r.stdout)
        seen = set()
        for d in data["devices"]:
            self.assertEqual(d["platform"], "windows")
            for key in ("major", "minor", "sysfs_path"):
                self.assertIsNone(d[key])
            self.assertNotIn(d["device_id"], seen)
            seen.add(d["device_id"])
            info = run("info", d["path"], "--json")
            self.assertEqual(info.returncode, 0, info.stderr)
            self.assertEqual(json.loads(info.stdout)["devices"][0]["device_id"], d["device_id"])

    def test_default_filter(self):
        """! @brief Verify that the default list omits the platform roles designated as hidden."""
        r = run("list", "--json")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertTrue(all(d["device_role"] != "partition" for d in json.loads(r.stdout)["devices"]))

    def test_reject_ordinary_files(self):
        """! @brief Reject ordinary Windows files, directories, and nonexistent paths as devices."""
        with tempfile.TemporaryDirectory(prefix="disksan-") as tmp:
            file = Path(tmp) / "normal file.txt"
            file.write_text("test fixture")
            for target in (file, Path(tmp) / "absent", Path(tmp)):
                r = run("info", str(target), "--json")
                self.assertEqual(r.returncode, 3, r.stderr)
                self.assertEqual(r.stdout, "")

if __name__ == "__main__":
    unittest.main()
