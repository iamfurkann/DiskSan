"""! @file
@brief Test native macOS discovery using diskutil only as a read-only reference.
"""
import json
import os
from pathlib import Path
import plistlib
import subprocess
import sys
import tempfile
import unittest

BINARY = sys.argv.pop(1)


def invoke(*args):
    """! @brief Run native read-only discovery and retain output and exit status for assertions."""
    return subprocess.run([BINARY, *args], capture_output=True, text=True)


def reference_snapshot():
    """! @brief Read structured native topology; command or plist failure remains a hard error."""
    result = subprocess.run(["/usr/sbin/diskutil", "list", "-plist"], capture_output=True, check=True)
    return plistlib.loads(result.stdout)


def stable_snapshot():
    """! @brief Retry three measured reference topology changes and fail stable product mismatches."""
    for attempt in range(3):
        before = reference_snapshot()
        result = invoke("list", "--all", "--json")
        if result.returncode:
            raise AssertionError(result.stderr)
        data = json.loads(result.stdout)
        after = reference_snapshot()
        if before == after:
            return data, after
        print(f"Native topology changed across sample {attempt + 1}/3", file=sys.stderr)
    raise unittest.SkipTest("diskutil topology changed in all three bracketed samples")


class MacInventory(unittest.TestCase):
    """! @brief Compare live IOMedia snapshots with the native diskutil inventory."""
    @classmethod
    def setUpClass(cls):
        """! @brief Capture one native snapshot and a structured diskutil reference for the test class."""
        cls.data, cls.reference = stable_snapshot()

    def test_inventory_matches_diskutil(self):
        """! @brief Verify native capacity and APFS roles against diskutil plist records."""
        found = {Path(d["path"]).name: d for d in self.data["devices"]}
        self.assertEqual(set(found), set(self.reference["AllDisks"]))
        for whole in self.reference["AllDisksAndPartitions"]:
            d = found[whole["DeviceIdentifier"]]
            self.assertEqual(d["size_bytes"], whole["Size"])
            if whole.get("Content") == "Apple_APFS_Container":
                self.assertEqual(d["device_role"], "composite")
            for p in whole.get("Partitions", []):
                entry = found[p["DeviceIdentifier"]]
                self.assertEqual(entry["device_role"], "partition")
                self.assertEqual(entry["size_bytes"], p["Size"])
            for v in whole.get("APFSVolumes", []):
                self.assertIn(found[v["DeviceIdentifier"]]["device_role"], ("volume", "snapshot"))

    def test_native_identity_and_flags(self):
        """! @brief Verify unique registry identities, Unix numbers, and conservative APFS media labels."""
        identities = []
        for d in self.data["devices"]:
            self.assertEqual(d["platform"], "macos")
            self.assertIsNone(d["sysfs_path"])
            self.assertTrue(d["device_id"].startswith("iokit:"))
            identities.append(d["device_id"])
            number = os.stat(d["path"]).st_rdev
            self.assertEqual((d["major"], d["minor"]), (os.major(number), os.minor(number)))
            if d["device_role"] in ("volume", "snapshot", "composite"):
                self.assertEqual(d["media_type"], "unknown")
        self.assertEqual(len(identities), len(set(identities)))

    def test_default_filter(self):
        """! @brief Verify that the default list omits the platform roles designated as hidden."""
        r = invoke("list", "--json")
        self.assertEqual(r.returncode, 0, r.stderr)
        data = json.loads(r.stdout)
        self.assertTrue(all(d["device_role"] not in ("partition", "volume", "snapshot") for d in data["devices"]))
        self.assertEqual(data["shown"], len(data["devices"]))
        # Each response owns its counts; unrelated live snapshots need not have equal topology.
        self.assertGreaterEqual(data["omitted"], 0)

    def test_info_and_symlink(self):
        """! @brief Resolve representative native roles directly and through temporary block symlinks."""
        seen_roles = set()
        for d in self.data["devices"]:
            if d["device_role"] in seen_roles:
                continue
            seen_roles.add(d["device_role"])
            for args in [("info", d["path"], "--json"), ("info", "--json", d["path"])]:
                r = invoke(*args)
                self.assertEqual(r.returncode, 0, r.stderr)
                found = json.loads(r.stdout)["devices"]
                self.assertEqual(len(found), 1)
                self.assertEqual(found[0]["device_id"], d["device_id"])
            with tempfile.TemporaryDirectory(prefix="disksan-alias-") as tmp:
                alias = Path(tmp) / "device alias with spaces"
                alias.symlink_to(d["path"])
                r = invoke("info", str(alias), "--json")
                self.assertEqual(r.returncode, 0, r.stderr)
                self.assertEqual(json.loads(r.stdout)["devices"][0]["device_id"], d["device_id"])

    def test_invalid_selectors(self):
        """! @brief Reject ordinary paths and character nodes without emitting a successful JSON snapshot."""
        with tempfile.TemporaryDirectory(prefix="disksan-invalid-") as tmp:
            regular = Path(tmp) / "ordinary file"
            regular.write_text("fixture")
            broken = Path(tmp) / "broken"
            broken.symlink_to(Path(tmp) / "missing")
            for path in [regular, broken, Path(tmp) / "missing", Path(tmp), Path("/dev/null")]:
                r = invoke("info", str(path), "--json")
                self.assertEqual(r.returncode, 3, r.stderr)
                self.assertEqual(r.stdout, "")


if __name__ == "__main__":
    unittest.main()
