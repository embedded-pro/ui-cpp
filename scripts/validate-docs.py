#!/usr/bin/env python3
"""Validate this repository's documentation invariants.

The sibling toolboxes validate per-algorithm doc pages against a template with
sections like "Mathematical Theory" and "Complexity Analysis". This repository
has no algorithms; its docs are an interface contract and a tier register, so it
checks the two rules it actually states instead:

1. Every doc/*.md path referenced from source or documentation exists. AGENTS.md,
   CLAUDE.md, Geometry.hpp and Input.hpp all pointed at doc/canvas.md and
   doc/portability.md for two phases while doc/ was empty.

2. Every backend directory under ui/backend/ is named in doc/portability.md.
   AGENTS.md requires a Tier 3 component to carry a one-line justification there;
   nothing enforced it.

Exit codes:
    0 — all checks pass
    1 — one or more checks failed
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DOC_ROOT = ROOT / "doc"
BACKEND_ROOT = ROOT / "ui" / "backend"

SEARCH_SUFFIXES = {".md", ".hpp", ".cpp", ".cmake", ".txt", ".yml", ".yaml"}
SKIP_DIRS = {".git", "build", "_deps", "node_modules", "megalinter-reports"}

DOC_REFERENCE = re.compile(r"doc/([A-Za-z0-9_./-]+\.md)")


def source_files() -> list[pathlib.Path]:
    return [
        path
        for path in ROOT.rglob("*")
        if path.is_file()
        and path.suffix in SEARCH_SUFFIXES
        and not any(part in SKIP_DIRS for part in path.relative_to(ROOT).parts)
    ]


def check_referenced_docs_exist() -> list[str]:
    failures: list[str] = []

    for path in source_files():
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue

        for reference in sorted(set(DOC_REFERENCE.findall(text))):
            if not (DOC_ROOT / reference).is_file():
                failures.append(
                    f"{path.relative_to(ROOT)} references doc/{reference}, which does not exist"
                )

    return failures


def check_backends_are_registered() -> list[str]:
    portability = DOC_ROOT / "portability.md"

    if not portability.is_file():
        return ["doc/portability.md does not exist"]

    text = portability.read_text(encoding="utf-8")

    return [
        f"ui/backend/{backend.name} is not mentioned in doc/portability.md "
        f"(AGENTS.md requires a tier entry for it)"
        for backend in sorted(BACKEND_ROOT.iterdir())
        if backend.is_dir() and backend.name not in text
    ]


def main() -> int:
    if not DOC_ROOT.is_dir():
        print(f"ERROR: doc directory not found at {DOC_ROOT}", file=sys.stderr)
        return 1

    failures = check_referenced_docs_exist() + check_backends_are_registered()

    if failures:
        print(f"FAIL: {len(failures)} documentation problem(s):\n")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("PASS: every referenced doc exists and every backend is registered.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
