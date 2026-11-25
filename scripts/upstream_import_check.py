#!/usr/bin/env python3
"""Structural import lock; cannot detect arbitrary copied code or linkage."""
import json
import sys
from pathlib import Path

EXPECTED = {
    "schema_version": 1,
    "project_license_status": "PENDING_OWNER_DECISION",
    "external_import_status": "BLOCKED",
    "components": [],
}
RESERVED = ("third_party", "vendor", "external", "deps", ".gitmodules")


def unique_pairs(pairs):
    obj = {}
    for key, value in pairs:
        if key in obj:
            raise ValueError("Duplicate registry key")
        obj[key] = value
    return obj


def validate(root):
    registry = root / "docs/upstream/upstream-registry.json"
    if registry.is_symlink() or not registry.is_file():
        raise ValueError("Registry must be a regular repository file")
    data = json.loads(registry.read_text(encoding="utf-8"),
                      object_pairs_hook=unique_pairs)
    if (not isinstance(data, dict) or
            type(data.get("schema_version")) is not int or data != EXPECTED):
        raise ValueError("Import lock requires pending owner decision and no components")
    for name in RESERVED:
        path = root / name
        if path.is_symlink() or path.exists():
            raise ValueError("Reserved external source path blocked: " + name)


def main():
    try:
        validate(Path(__file__).resolve().parents[1])
    except (ValueError, OSError) as exc:
        print("Upstream import policy failed: " + str(exc), file=sys.stderr)
        return 1
    print("Upstream import lock verified; source/provenance review still required.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
