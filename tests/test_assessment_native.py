"""! @file
@brief Exercise native assessment metadata with existing devices and disposable selector aliases only.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY = sys.argv.pop(1)
PLATFORM = sys.argv.pop(1)


def invoke(*args):
    """! @brief Capture a metadata-only command without exposing device inventories in routine logs."""
    return subprocess.run([BINARY, *args], capture_output=True, text=True, timeout=60)


class NativeAssessment(unittest.TestCase):
    """! @brief Check actual native assessment structure, alias selection and known system dependencies."""

    def assert_assessment(self, result):
        """! @brief Require explicit observational limits and unknown capabilities in a successful inspection."""
        self.assertEqual(result.returncode, 0, result.stderr)
        data = json.loads(result.stdout)
        self.assertEqual(data["schema_version"], 2)
        self.assertIs(data["erase_authorized"], False)
        self.assertIs(data["identity"]["guarantees_future_identity"], False)
        self.assertIs(data["identity"]["atomic"], False)
        self.assertEqual(data["identity"]["valid_until"], "end_of_observation")
        self.assertIn(data["inspection_status"], ("blocked", "review_required"))
        for method in data["capabilities"]:
            for dimension in ("support", "os_access", "applicability", "passthrough"):
                self.assertEqual(method[dimension], "unknown")
        return data

    def test_accessible_targets(self):
        """! @brief Inspect a bounded live inventory sample without opening device data or changing mounts."""
        inventory = invoke("list", "--all", "--json")
        self.assertEqual(inventory.returncode, 0, inventory.stderr)
        devices = json.loads(inventory.stdout)["devices"]
        tested = 0
        for device in devices:
            path = device["path"]
            if not path or (PLATFORM != "Windows" and not os.path.exists(path)):
                continue
            for command in ("assess", "capabilities"):
                data = self.assert_assessment(invoke(command, path, "--json"))
                self.assertEqual(data["device"]["device_id"], device["device_id"])
                self.assertEqual(data["device"]["device_role"], device["device_role"])
            if PLATFORM != "Windows":
                with tempfile.TemporaryDirectory(prefix="disksan-assess-alias-") as tmp:
                    alias = Path(tmp) / "selector with spaces"
                    alias.symlink_to(path)
                    data = self.assert_assessment(invoke("assess", "--json", "--", str(alias)))
                    self.assertEqual(data["device"]["device_id"], device["device_id"])
            tested += 1
            if tested == 4:
                break
        if not tested:
            self.skipTest("No accessible device selectors; native assessment is not validated")

    def test_root_dependency(self):
        """! @brief Require a blocking system observation for an independently observable root device."""
        if PLATFORM == "Windows":
            self.skipTest("Windows system-volume reference requires owner native acceptance")
        if PLATFORM == "Darwin":
            import plistlib
            reference = subprocess.run(["/usr/sbin/diskutil", "info", "-plist", "/"], capture_output=True, check=True)
            root = plistlib.loads(reference.stdout)
            selector = root.get("DeviceNode")
        else:
            root_number = os.stat("/").st_dev
            inventory = invoke("list", "--all", "--json")
            self.assertEqual(inventory.returncode, 0, inventory.stderr)
            selector = next((d["path"] for d in json.loads(inventory.stdout)["devices"]
                             if (d["major"], d["minor"]) == (os.major(root_number), os.minor(root_number))), None)
        if not selector or not os.path.exists(selector):
            self.skipTest("Root backing has no accessible discovered node in this namespace")
        data = self.assert_assessment(invoke("assess", selector, "--json"))
        self.assertEqual(data["inspection_status"], "blocked")
        self.assertTrue(any(o["check"] == "system" and o["state"] == "true" for o in data["observations"]))


if __name__ == "__main__":
    unittest.main()
