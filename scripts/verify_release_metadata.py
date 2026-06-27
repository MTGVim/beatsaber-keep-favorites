#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        raise SystemExit(f"Missing JSON file: {path}")
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON in {path}: {exc}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qpm", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--expected-version")
    parser.add_argument("--expected-package-id", default="com.beatgames.beatsaber")
    parser.add_argument("--expected-package-version", required=True)
    parser.add_argument("--expected-modloader", default="Scotland2")
    args = parser.parse_args()

    qpm_path = Path(args.qpm)
    manifest_path = Path(args.manifest)
    qpm = load_json(qpm_path)
    manifest = load_json(manifest_path)

    qpm_info = qpm.get("info") or {}
    qpm_version = qpm_info.get("version")
    expected_version = args.expected_version or qpm_version

    errors: list[str] = []

    def check(label: str, actual, expected) -> None:
        if actual != expected:
            errors.append(f"{label}: expected {expected!r}, got {actual!r}")

    check("qpm.info.version", qpm_version, expected_version)
    check("manifest.version", manifest.get("version"), expected_version)
    check("manifest.name", manifest.get("name"), qpm_info.get("name"))
    check("manifest.id", manifest.get("id"), qpm_info.get("id"))
    check("manifest.modloader", manifest.get("modloader"), args.expected_modloader)
    check("manifest.packageId", manifest.get("packageId"), args.expected_package_id)
    check("manifest.packageVersion", manifest.get("packageVersion"), args.expected_package_version)

    author = (manifest.get("author") or "").strip()
    description = (manifest.get("description") or "").strip()

    if not author:
        errors.append("manifest.author must be a non-empty string")
    if not description:
        errors.append("manifest.description must be a non-empty string")
    if "dry-run helper" in description.lower():
        errors.append("manifest.description still contains stale 'dry-run helper' wording")
    if "porter" in manifest:
        errors.append("manifest must not contain the stale 'porter' field")

    if errors:
        print("Release metadata verification failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print("Release metadata verification passed")
    print(f"- version: {expected_version}")
    print(f"- packageId: {manifest.get('packageId')}")
    print(f"- packageVersion: {manifest.get('packageVersion')}")
    print(f"- author: {author}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
