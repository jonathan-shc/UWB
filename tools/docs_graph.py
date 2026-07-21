#!/usr/bin/env python3
"""Make the architecture page's dependency graph legible.

The page generator emits the module import graph as one flat flowchart and a
zoomable shell around it. At this repo's size that renders as an unreadable
crop: dozens of modules, self-loop artifacts (a module importing its own
header), and a natural width several times the shell's, so the default 1:1
view shows two boxes and a tangle of splines.

This pass restructures the presentation, deriving everything from the page
itself so nothing is hand-curated to drift:

  * self-loop edges are dropped — at module level they are import artifacts,
    not information.
  * each module is assigned to its source directory, read from the page's own
    per-module headings, and the flat graph becomes clustered subgraphs.
  * a subsystem-level overview graph — one node per directory cluster, one
    arrow per aggregated dependency — goes above it. Small enough to be
    crisp at natural size, it answers the layering question at a glance.
  * every page with diagrams gets two script shims around the generator's
    nav.js: one tightens mermaid's layout spacing and bumps its font before
    the first render, the other clicks each diagram's own Fit control when
    the rendered graph overflows its shell, so big graphs open showing their
    whole shape instead of a random crop.

Idempotent for the same reason docs_media.py is: when the page generator is
not configured, the earlier passes run over a site/ kept from a previous
build, so a page may already carry the injections. Run from the repo root,
after docs_github.py and before the link pass.
"""

from __future__ import annotations

import html
import re
import sys
from pathlib import Path

SITE = Path("site")
ARCH = SITE / "architecture.html"

FIGURE_RE = re.compile(
    r'<figure class="graph-wrap"><div class="graph-shell">'
    r'<pre class="mermaid">(.*?)</pre></div></figure>',
    re.S,
)
PATH_RE = re.compile(r"(?:modules|ports)/[\w./-]+\.(?:cpp|c|h)")
NAV_ANCHOR = '<script defer src="nav.js"></script>'

# Runs between mermaid.min.js and nav.js (deferred scripts execute in document
# order), so nav.js's own initialize call flows through the shim and the first
# render already uses the tightened layout.
PRE_SHIM = """<style>.graph-wrap figcaption{margin:.7rem 0 0;font-size:.82rem;color:var(--muted);text-align:center}
.gv-over .graph-tools{display:none}</style>
<script defer id="gv-pre">
(function(){if(!window.mermaid)return;var orig=mermaid.initialize.bind(mermaid);
mermaid.initialize=function(c){c=c||{};c.flowchart=Object.assign({},c.flowchart,{nodeSpacing:26,rankSpacing:40,padding:8});
if(c.themeVariables)c.themeVariables.fontSize="15px";orig(c)};})();
</script>"""

# Waits for nav.js to render the diagrams and attach the zoom tools, then
# presses Fit once on any graph wider than its shell. Fail-soft: no diagrams,
# no tools, or no CDN and the interval just expires.
FIT_SHIM = """<script defer id="gv-fit">
(function(){var n=0,t=setInterval(function(){n++;
var wraps=document.querySelectorAll(".graph-wrap");var done=0;
wraps.forEach(function(w){var svg=w.querySelector("svg"),shell=w.querySelector(".graph-shell"),
b=w.querySelector(".graph-tools button[title='Fit to width']");
if(svg&&b){done++;if(!w.dataset.fitted){w.dataset.fitted=1;
if(svg.getBoundingClientRect().width>shell.clientWidth)b.click()}}});
if(wraps.length&&done===wraps.length)clearInterval(t);else if(n>60)clearInterval(t)},120);})();
</script>"""

OVER_CAPTION = (
    "Subsystem-level view of the module graph below: each box is a source "
    "directory, each arrow an import dependency between them."
)
DETAIL_CAPTION = (
    "Every module and its imports, grouped by directory. Opens fitted to the "
    "shell; zoom in to read the labels, or use the per-module sections below."
)


def parse_edges(mermaid: str) -> tuple[str, list[tuple[str, str]]]:
    lines = mermaid.strip().split("\n")
    pairs = []
    for line in lines[1:]:
        a, _, b = line.strip().partition(" --> ")
        if a and b and a != b:  # drops the self-loop artifacts
            pairs.append((a, b))
    return lines[0], pairs


def stem_dirs(page: str) -> dict[str, str]:
    """module name -> its source directory, from the page's file headings.

    A stem can exist both in modules/ and in a port's copy; the shared core
    is the one the import graph describes, so modules/ wins.
    """
    out: dict[str, str] = {}
    for p in PATH_RE.findall(page):
        d, f = p.rsplit("/", 1)
        stem = re.sub(r"\.(cpp|c|h)$", "", f)
        if stem not in out or (
            d.startswith("modules/") and not out[stem].startswith("modules/")
        ):
            out[stem] = d
    return out


def cluster_of(directory: str) -> str:
    d = re.sub(r"^modules/", "", directory)
    m = re.match(r"woz_uwb/src/([a-z_]+)", d)
    return "woz_uwb/" + m.group(1) if m else d.split("/")[0]


def figures(page: str, mermaid: str) -> bytes:
    header, pairs = parse_edges(mermaid)
    dirs = stem_dirs(page)
    clusters: dict[str, set[str]] = {}
    loose: set[str] = set()
    for a, b in pairs:
        for n in (a, b):
            if n in dirs:
                clusters.setdefault(cluster_of(dirs[n]), set()).add(n)
            else:
                loose.add(n)
    if loose:
        print(
            f"    {len(loose)} module(s) without a directory heading kept "
            f"ungrouped: {', '.join(sorted(loose))}"
        )

    def cluster_key(n: str) -> str | None:
        return cluster_of(dirs[n]) if n in dirs else None

    cedges = sorted(
        {
            (cluster_key(a), cluster_key(b))
            for a, b in pairs
            if cluster_key(a) and cluster_key(b) and cluster_key(a) != cluster_key(b)
        }
    )
    names = sorted(clusters)
    cid = {c: f"g{i}" for i, c in enumerate(names)}
    over = (
        [header]
        + [f'  {cid[c]}["{c}"]' for c in names]
        + [f"  {cid[a]} --> {cid[b]}" for a, b in cedges]
    )
    detail = [header]
    for i, c in enumerate(names):
        detail.append(f'  subgraph c{i}["{c}"]')
        detail += [f"    {n}" for n in sorted(clusters[c])]
        detail.append("  end")
    detail += [f"  {a} --> {b}" for a, b in pairs]

    esc = lambda t: html.escape("\n".join(t), quote=False)
    return (
        f'<figure class="graph-wrap gv-over"><div class="graph-shell">'
        f'<pre class="mermaid">{esc(over)}</pre></div>\n'
        f"<figcaption>{OVER_CAPTION}</figcaption></figure>\n"
        f'<figure class="graph-wrap"><div class="graph-shell">'
        f'<pre class="mermaid">{esc(detail)}</pre></div>\n'
        f"<figcaption>{DETAIL_CAPTION}</figcaption></figure>"
    ).encode()


def main() -> int:
    if not ARCH.is_file():
        print("    no rendered site — nothing to restructure")
        return 0

    raw = ARCH.read_bytes()
    if b'class="graph-wrap gv-over"' in raw:
        print("    architecture graphs already restructured")
    else:
        page = raw.decode()
        m = FIGURE_RE.search(page)
        if not m:
            print(
                "docs_graph: architecture page has no graph figure to "
                "restructure — generator layout changed?",
                file=sys.stderr,
            )
            return 1
        figs = figures(page, html.unescape(m.group(1)))
        raw = page[: m.start()].encode() + figs + page[m.end() :].encode()
        ARCH.write_bytes(raw)
        print("    architecture graph split into overview + clustered detail")

    shimmed = kept = 0
    anchor = NAV_ANCHOR.encode()
    for p in sorted(SITE.glob("*.html")):
        content = p.read_bytes()
        if b'class="mermaid"' not in content:
            continue
        if b'id="gv-pre"' in content:
            kept += 1
            continue
        if anchor not in content:
            continue
        content = content.replace(
            anchor, PRE_SHIM.encode() + anchor + FIT_SHIM.encode(), 1
        )
        p.write_bytes(content)
        shimmed += 1
    note = f" ({kept} already shimmed)" if kept else ""
    print(f"    layout + auto-fit shims on {shimmed} page(s){note}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
