#!/usr/bin/env bash
# docs.sh — build the documentation site into site/.
#
# Two generators write into the same output directory, in this order:
#
#   1. the subsystem tree + guides + search shell   -> site/*.html
#   2. doxygen (docs/Doxyfile)                      -> site/api/
#
# then a link pass rewrites cross-document links so the published site has no
# dead ends, and the freshness gate confirms the committed docs/ tree matches
# the source. Run it through `make docs`.
#
# Nothing here needs the NCS toolchain or hardware.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$REPO_ROOT"

# A linked worktree builds the same site as the main checkout: tools/docs_title.py
# repairs the one thing that differs (the generator titles pages after the
# checkout directory). The freshness gate is the exception — it regenerates
# internally and would see the repaired titles as drift — so it is skipped here
# and left to the main checkout and to CI, which both run on a plain clone.
IN_WORKTREE=0
if [ "$(git rev-parse --git-dir)" != "$(git rev-parse --git-common-dir)" ]; then
	IN_WORKTREE=1
fi

# The subsystem tree, the guide rendering and the site shell come from a page
# generator that lives outside this repo. Point PAGE_GEN at an executable that
# takes one argument, `build` or `check`, and maps it onto that tool; the default
# path is gitignored so the hook stays out of tree. Without it, `make docs` still
# builds the reference tree and the link pass over the committed docs/.
PAGE_GEN="${PAGE_GEN:-$REPO_ROOT/tools/docs_generate.local}"
SKIPPED_GEN=0

for tool in doxygen dot; do
	command -v "$tool" >/dev/null 2>&1 || {
		echo "docs.sh: '$tool' not found (brew install doxygen graphviz)" >&2
		exit 1
	}
done

# Doxygen runs first: docs/reference.md links into site/api/, and the page
# generator's dead-link gate resolves that link against the rendered site.
echo "==> reference tree (doxygen)"
doxygen docs/Doxyfile

echo "==> subsystem tree, guides and site shell"
if [ -x "$PAGE_GEN" ]; then
	"$PAGE_GEN" build
else
	echo "    not configured on this machine — this is normal and not an error."
	echo "    The docs/ tree is committed, so only maintainers regenerating it"
	echo "    need the generator. Building the reference tree over it instead."
	SKIPPED_GEN=1
fi

echo "==> titles"
python3 tools/docs_title.py

echo "==> link pass"
python3 tools/docs_links.py

if [ "$SKIPPED_GEN" -eq 1 ]; then
	echo "==> freshness gate (skipped: no page generator configured)"
elif [ "$IN_WORKTREE" -eq 1 ]; then
	echo "==> freshness gate (skipped: linked worktree — CI and the main checkout run it)"
else
	echo "==> freshness gate"
	"$PAGE_GEN" check
fi

echo
if [ "$SKIPPED_GEN" -eq 1 ]; then
	echo "reference tree only — open it with: open site/api/index.html"
else
	echo "site/index.html is ready — open it with: open site/index.html"
fi
