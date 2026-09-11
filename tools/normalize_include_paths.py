#!/usr/bin/env python3
"""Normalize legacy include separators and local filename case."""

from __future__ import print_function

import argparse
import os
import re
from pathlib import Path

SOURCE_SUFFIXES = {".c", ".cpp", ".h", ".inl"}
INCLUDE = re.compile(r'^(\s*#\s*include\s*[<"])([^>"]+)([>"])', re.MULTILINE)


def case_resolve(root, relative):
    current = root
    for component in Path(relative).parts:
        if component == ".":
            continue
        if component == "..":
            current = current.parent
            continue
        if not current.is_dir():
            return None
        matches = [item for item in current.iterdir()
                   if item.name.lower() == component.lower()]
        if len(matches) != 1:
            return None
        current = matches[0]
    return current if current.is_file() else None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    repository = Path(__file__).resolve().parent.parent
    upstream = repository / "upstream"
    engine = (repository / ".." / "Genesis3D_Project").resolve()
    search_roots = [upstream]
    search_roots.extend(sorted({p.parent for p in engine.rglob("*") if p.is_file()}))
    changed = []

    for source in sorted(upstream.rglob("*")):
        if not source.is_file() or source.suffix.lower() not in SOURCE_SUFFIXES:
            continue
        original = source.read_text(encoding="utf-8", errors="surrogateescape")

        def replace(match):
            include = match.group(2).replace("\\", "/")
            candidates = []
            relative_match = case_resolve(source.parent, include)
            if relative_match is not None:
                candidates.append((relative_match, source.parent))
            for root in search_roots:
                found = case_resolve(root, include)
                if found is not None:
                    candidates.append((found, root))
            unique = {str(path.resolve()): (path, root) for path, root in candidates}
            if len(unique) != 1:
                return match.group(1) + include + match.group(3)
            path, root = next(iter(unique.values()))
            if include.startswith("."):
                normalized = os.path.relpath(str(path), str(source.parent))
            else:
                normalized = os.path.relpath(str(path), str(root))
            return match.group(1) + normalized.replace(os.sep, "/") + match.group(3)

        updated = INCLUDE.sub(replace, original)
        if updated != original:
            changed.append(str(source.relative_to(repository)))
            if args.apply:
                source.write_text(updated, encoding="utf-8", errors="surrogateescape")

    for path in changed:
        print(path)
    if changed and not args.apply:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
