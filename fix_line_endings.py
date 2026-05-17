#!/usr/bin/env python3
"""
Normalize line endings for the C3X text files that are easy for tools to mix.

Most files in this repo area intentionally use DOS/Windows CRLF endings. A few
files intentionally use Unix LF endings. This script keeps that policy explicit
instead of inferring it from whatever the file happens to contain today.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path


CRLF = "crlf"
LF = "lf"


# Keep paths relative to this script's directory and use "/" separators.
# Scope: files in the C3X root plus the Text and Trade Net X subfolders.
LINE_ENDING_POLICY = {
    ".gitignore": CRLF,
    "AGENTS.md": CRLF,
    "C3X.h": CRLF,
    "Civ3Conquests.h": CRLF,
    "INSTALL.bat": CRLF,
    "README.md": LF,
    "README.txt": CRLF,
    "RUN.bat": CRLF,
    "TEST_INJECTED_CODE_COMPILE.bat": CRLF,
    "assemble_release.bat": CRLF,
    "c3cre.py": CRLF,
    "changelog.txt": CRLF,
    "civ_prog_objects.csv": CRLF,
    "common.c": CRLF,
    "commonre.py": CRLF,
    "default.c3x_config.ini": CRLF,
    "default.districts_config.txt": LF,
    "default.districts_natural_wonders_config.txt": LF,
    "default.districts_wonders_config.txt": LF,
    "ep.c": CRLF,
    "fix_line_endings.py": CRLF,
    "forgdb.py": CRLF,
    "injected_code.c": CRLF,
    "trade_net_addresses.txt": CRLF,
    "Text/c3x-labels.txt": CRLF,
    "Text/c3x-script.txt": CRLF,
    "Trade Net X/BUILD.bat": CRLF,
    "Trade Net X/Civ3Conquests.hpp": CRLF,
    "Trade Net X/TRADE NET X INFO.txt": CRLF,
    "Trade Net X/TradeNetX.cpp": CRLF,
    "Trade Net X/convert_header_for_cpp.py": CRLF,
}


ENDING_BYTES = {
    CRLF: b"\r\n",
    LF: b"\n",
}


@dataclass(frozen=True)
class FileStatus:
    path: Path
    style: str
    exists: bool
    changed: bool
    crlf: int = 0
    lf: int = 0
    cr: int = 0


def count_endings(data: bytes) -> tuple[int, int, int]:
    crlf = 0
    lf = 0
    cr = 0
    i = 0
    while i < len(data):
        byte = data[i]
        if byte == 0x0D:
            if i + 1 < len(data) and data[i + 1] == 0x0A:
                crlf += 1
                i += 2
            else:
                cr += 1
                i += 1
        elif byte == 0x0A:
            lf += 1
            i += 1
        else:
            i += 1
    return crlf, lf, cr


def normalize_data(data: bytes, style: str) -> bytes:
    return re.sub(rb"\r\n|\r|\n", ENDING_BYTES[style], data)


def check_file(repo_root: Path, rel_path: str, style: str, write: bool) -> FileStatus:
    path = repo_root / Path(rel_path)
    if not path.exists():
        return FileStatus(path=path, style=style, exists=False, changed=False)

    data = path.read_bytes()
    normalized = normalize_data(data, style)
    changed = normalized != data
    crlf, lf, cr = count_endings(data)

    if changed and write:
        path.write_bytes(normalized)

    return FileStatus(
        path=path,
        style=style,
        exists=True,
        changed=changed,
        crlf=crlf,
        lf=lf,
        cr=cr,
    )


def format_counts(status: FileStatus) -> str:
    return f"CRLF={status.crlf} LF={status.lf} CR={status.cr}"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Fix mixed line endings in known C3X root/Text/Trade Net X text files."
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="do not write files; exit with status 1 if any file needs normalization",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="show what would be changed without writing files",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="also print files that already match the expected style",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parent
    write = not args.check and not args.dry_run

    statuses = [
        check_file(repo_root, rel_path, style, write)
        for rel_path, style in LINE_ENDING_POLICY.items()
    ]

    missing = [status for status in statuses if not status.exists]
    changed = [status for status in statuses if status.changed]
    unchanged = [status for status in statuses if status.exists and not status.changed]

    for status in changed:
        action = "fixed" if write else "would fix"
        print(
            f"{action}: {status.path.relative_to(repo_root).as_posix()} "
            f"->{status.style} ({format_counts(status)})"
        )

    if args.verbose:
        for status in unchanged:
            print(
                f"ok: {status.path.relative_to(repo_root).as_posix()} "
                f"->{status.style} ({format_counts(status)})"
            )

    for status in missing:
        print(
            f"missing: {status.path.relative_to(repo_root).as_posix()} "
            f"->{status.style}",
            file=sys.stderr,
        )

    if not changed and not missing:
        print("All configured files already use the expected line endings.")

    if args.check and (changed or missing):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
