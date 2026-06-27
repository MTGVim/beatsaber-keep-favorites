#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def extract_section(changelog_path: Path, version: str) -> str:
    text = changelog_path.read_text(encoding="utf-8")
    lines = text.splitlines()
    target = f"## [{version}]"
    start = None
    for i, line in enumerate(lines):
        if line.startswith(target):
            start = i + 1
            break
    if start is None:
        raise SystemExit(f"Could not find changelog section for version {version} in {changelog_path}")

    end = len(lines)
    for i in range(start, len(lines)):
        if lines[i].startswith("## ["):
            end = i
            break

    section = "\n".join(lines[start:end]).strip()
    if not section:
        raise SystemExit(f"Changelog section for version {version} is empty")
    return section


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--changelog", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--repo", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-url", required=True)
    parser.add_argument("--target-version", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--prev-tag", default="")
    args = parser.parse_args()

    section = extract_section(Path(args.changelog), args.version)
    compare_line = (
        f"- Compare: https://github.com/{args.repo}/compare/{args.prev_tag}...{args.tag}"
        if args.prev_tag
        else "- Compare: initial tagged release"
    )

    body = f"""# Keep Favorites {args.tag}

## Install
- Install `KeepFavorites.qmod` with a Quest qmod installer.
- Or manually copy `libKeepFavorites.so` into `/sdcard/ModData/com.beatgames.beatsaber/Modloader/mods/`.

## Target
- Platform: Meta Quest Beat Saber mod setup
- Beat Saber version: {args.target_version}

## Changes
{section}

## Build info
- Commit: {args.commit}
- Run: {args.run_url}
{compare_line}

## Assets
- KeepFavorites.qmod
- libKeepFavorites.so
"""
    Path(args.output).write_text(body.strip() + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
