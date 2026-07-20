#!/usr/bin/env python3
"""Title the generated pages after the repository, not after the checkout directory.

The page generator takes the project name from the basename of the directory it
runs in, and offers no setting to override it. In a linked worktree that name is
the worktree directory's, not the repository's, which would put the wrong title
on every page and in the committed docs/ tree.

Deliberately no example checkout name here: this docstring is itself published,
and the rewrite below would substitute any literal it contained, leaving a
sentence that compares a name against itself.

This rewrites the checkout's name to the repository's wherever the generator
emitted it. The repository name comes from the common git directory, which every
worktree shares, so it is the same value from any checkout. When the two names
already agree this is a no-op, which is the case in the main checkout.

Run from the repo root, after the generators and before the link pass.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


def git(*args: str) -> str:
    try:
        return subprocess.run(
            ["git", *args], capture_output=True, text=True, check=True
        ).stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return ""


def main() -> int:
    checkout = Path.cwd().name
    common = git("rev-parse", "--path-format=absolute", "--git-common-dir")
    if not common:
        print("docs_title: not a git checkout — leaving titles alone", file=sys.stderr)
        return 0

    repo = Path(common).parent.name
    if repo == checkout:
        print(f"    titles already correct ({repo})")
        return 0

    targets = [Path("docs/README.md"), Path("docs/ARCHITECTURE.md")]
    targets += sorted(Path("site").rglob("*.html")) if Path("site").is_dir() else []

    # Standalone token only. A plain word boundary would also match inside
    # hyphenated names and URL paths (a `worktree-pro-docs` branch, say), so
    # require that neither side continues a word, a hyphen or a path.
    name = re.escape(checkout.encode())
    pattern = re.compile(rb"(?<![\w/-])" + name + rb"(?![\w/-])")
    edits = files = 0
    for path in targets:
        if not path.exists():
            continue
        raw = path.read_bytes()
        fixed, count = pattern.subn(repo.encode(), raw)
        if count:
            path.write_bytes(fixed)
            edits += count
            files += 1

    print(f"    retitled {checkout!r} -> {repo!r}: {edits} occurrence(s) in {files} file(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
