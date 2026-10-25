"""! @file
@brief Test real Linux discovery without sudo, mounts, or disk writes.
"""
import glob
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest

BINARY = sys.argv.pop(1)


def invoke(*args):
    """! @brief Run native read-only discovery and retain output and exit status for assertions."""
    return subprocess.run([BINARY, *args], capture_output=True, text=True, check=False)


class LinuxInventory(unittest.TestCase):
    """! @brief Compare live Linux snapshots with sysfs, block aliases, and lsblk."""
    def test_real_inventory_and_sysfs(self):
        """! @brief Compare device numbers, capacity, partition roles, and filtering with sysfs."""
        result = invoke("list", "--all", "--json")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        data = json.loads(result.stdout)
        numbers = set()
        for d in data["devices"]:
            with self.subTest(device=d["path"]):
                pair = (d["major"], d["minor"])
                self.assertNotIn(pair, numbers)
                numbers.add(pair)
                root = Path(d["sysfs_path"])
                self.assertEqual((root / "dev").read_text().strip(), f"{pair[0]}:{pair[1]}")
                sectors = int((root / "size").read_text())
                if "metadata_conflict" not in d["warnings"]:
                    self.assertEqual(d["size_bytes"], sectors * 512)
                if (root / "partition").exists():
                    self.assertEqual(d["device_role"], "partition")
                if d["transport"] == "virtio":
                    self.assertIn(d["device_role"], ("virtual", "partition"))
                    self.assertEqual(d["media_type"], "unknown")
        baseline = json.loads(invoke("list", "--json").stdout)
        self.assertTrue(all(d["device_role"] != "partition" for d in baseline["devices"]))

    def test_invalid_targets(self):
        """! @brief Reject regular files, directories, broken links, and character-device selectors."""
        with tempfile.TemporaryDirectory(prefix="disksan-test-") as tmp:
            regular = Path(tmp) / "regular file"
            regular.write_text("test fixture; not a disk")
            broken = Path(tmp) / "broken"
            broken.symlink_to(Path(tmp) / "absent")
            for path in (regular, broken, Path(tmp) / "absent", Path(tmp), Path("/dev/null")):
                r = invoke("info", str(path), "--json")
                self.assertEqual(r.returncode, 3, r.stderr)
                self.assertEqual(r.stdout, "")
                self.assertTrue(r.stderr)

    def test_real_info_and_aliases(self):
        """! @brief Resolve each available block node and a temporary symlink to the same identity."""
        data = json.loads(invoke("list", "--all", "--json").stdout)
        tested = 0
        for d in data["devices"]:
            path = d["path"]
            if not path or not os.path.exists(path) or not stat.S_ISBLK(os.stat(path).st_mode):
                continue
            r = invoke("info", path, "--json")
            self.assertEqual(r.returncode, 0, r.stderr)
            found = json.loads(r.stdout)["devices"][0]
            self.assertEqual((found["major"], found["minor"]), (d["major"], d["minor"]))
            self.assertEqual(found["device_role"], d["device_role"])
            with tempfile.TemporaryDirectory(prefix="disksan-alias-") as tmp:
                alias = Path(tmp) / "alias with spaces"
                alias.symlink_to(path)
                via_alias = invoke("info", str(alias), "--json")
                self.assertEqual(via_alias.returncode, 0, via_alias.stderr)
                alias_d = json.loads(via_alias.stdout)["devices"][0]
                self.assertEqual((alias_d["major"], alias_d["minor"]), (d["major"], d["minor"]))
            tested += 1
        if tested == 0:
            self.skipTest("No accessible block device nodes; live info remains unverified")

    def test_by_id(self):
        """! @brief Resolve available by-id aliases and verify their Unix device numbers."""
        links = [p for p in glob.glob("/dev/disk/by-id/*") if os.path.exists(p) and stat.S_ISBLK(os.stat(p).st_mode)]
        if not links:
            self.skipTest("No valid by-id links on this Linux host")
        for link in links:
            r = invoke("info", link, "--json")
            self.assertEqual(r.returncode, 0, r.stderr)
            d = json.loads(r.stdout)["devices"][0]
            number = os.stat(link).st_rdev
            self.assertEqual((d["major"], d["minor"]), (os.major(number), os.minor(number)))

    def test_lsblk_comparison(self):
        """! @brief Compare discovered capacities with structured lsblk output when available."""
        if not shutil.which("lsblk"):
            self.skipTest("lsblk not installed")
        reference = subprocess.run(["lsblk", "--json", "--bytes", "--list", "--all", "--output", "PATH,MAJ:MIN,SIZE"], capture_output=True, text=True, check=True)
        expected = {d["maj:min"]: d for d in json.loads(reference.stdout)["blockdevices"]}
        r = invoke("list", "--all", "--json")
        self.assertEqual(r.returncode, 0, r.stderr)
        for d in json.loads(r.stdout)["devices"]:
            key = f"{d['major']}:{d['minor']}"
            if key in expected and expected[key]["size"] is not None:
                self.assertEqual(d["size_bytes"], int(expected[key]["size"]))


if __name__ == "__main__":
    unittest.main()
