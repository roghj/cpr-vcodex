from __future__ import annotations

import argparse
import re
from pathlib import Path


TAG_RE = re.compile(r"^(\d+\.\d+\.\d+\.\d+)-(?:cpr-vcodex|Roghj)$")
REPO = "roghj/cpr-vcodex"
BREAK_RE = re.compile(r"\s*<br\s*/?>\s*", re.IGNORECASE)


def version_from_tag(tag: str) -> str:
    match = TAG_RE.fullmatch(tag)
    if not match:
        raise ValueError(f"Unsupported CPR-vCodex release tag: {tag}")
    return match.group(1)


def extract_changes(changelog: str, version: str) -> str:
    row_re = re.compile(rf"^\|\s*`{re.escape(version)}`\s*\|\s*(.*?)\s*\|\s*$", re.MULTILINE)
    match = row_re.search(changelog)
    if not match:
        raise ValueError(f"CHANGELOG.md has no row for {version}")

    changes = BREAK_RE.sub("\n", match.group(1).strip())
    changes = "\n".join(line.strip() for line in changes.splitlines() if line.strip())
    if not changes or changes == "-":
        raise ValueError(f"CHANGELOG.md has no release notes for {version}")
    return changes


def render_release_notes(changelog_path: Path, tag: str) -> str:
    version = version_from_tag(tag)
    changelog = changelog_path.read_text(encoding="utf-8")
    if tag.endswith("-Roghj"):
        # Roghj releases use a row named after the full tag (`1.6.5.7-Roghj`);
        # without one, publish a short generic note instead of failing.
        try:
            changes = extract_changes(changelog, tag)
        except ValueError:
            changes = f"Versão Roghj baseada no CPR-vCodex {version}."
    else:
        changes = extract_changes(changelog, version)
    return (
        f"## Changes\n\n{changes}\n\n"
        f"[Full changelog](https://github.com/{REPO}/blob/{tag}/CHANGELOG.md)\n"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate GitHub release notes from the matching changelog row.")
    parser.add_argument("--tag", required=True)
    parser.add_argument("--changelog", type=Path, default=Path("CHANGELOG.md"))
    args = parser.parse_args()

    try:
        print(render_release_notes(args.changelog, args.tag), end="")
        return 0
    except (OSError, ValueError) as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    raise SystemExit(main())
