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
    whole shape instead of a random crop — and makes the shells direct:
    drag pans, cmd/ctrl+scroll (and trackpad pinch) zooms around the
    cursor, and plain or shift+scroll stays native, so the shell scrolls
    vertically or horizontally like any scrollable pane.
  * the per-module sections lose their visual noise: headings show the file
    name with the directory as a small eyebrow above it instead of one long
    path, the "depends on" rows become compact base-name chips (full path
    on hover) instead of comma-separated full paths, and the blurbs drop
    the "@file <name> — " prefix that would repeat the heading above them.

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
.gv-over .graph-tools{display:none}
.graph-wrap:not(.gv-over) .graph-shell{cursor:grab}
.graph-wrap:not(.gv-over) .graph-shell.dragging{cursor:grabbing;user-select:none}
.arch-sec h2 .arch-dir{display:block;font-size:.7rem;font-weight:500;color:var(--faint);font-family:var(--mono);letter-spacing:.04em;margin:0 0 .25rem}</style>
<script defer id="gv-pre">
(function(){if(!window.mermaid)return;var orig=mermaid.initialize.bind(mermaid);
mermaid.initialize=function(c){c=c||{};c.flowchart=Object.assign({},c.flowchart,{nodeSpacing:26,rankSpacing:40,padding:8});
if(c.themeVariables)c.themeVariables.fontSize="15px";orig(c)};})();
</script>"""

# Waits for nav.js to render the diagrams and attach the zoom tools, then
# presses Fit once on any graph wider than its shell, and wires the shells
# for direct manipulation: drag pans, cmd/ctrl+scroll (which is also what a
# trackpad pinch sends) zooms around the cursor keeping the zoom buttons'
# state in sync; any other wheel event is left to the browser, which
# scrolls the shell natively (shift+scroll horizontal) and chains to the
# page at its edges. Fail-soft: no diagrams, no tools, or no CDN and the
# interval just expires.
FIT_SHIM = """<script defer id="gv-fit">
(function(){var n=0,t=setInterval(function(){n++;
var wraps=document.querySelectorAll(".graph-wrap");var done=0;
wraps.forEach(function(w){var svg=w.querySelector("svg"),shell=w.querySelector(".graph-shell"),
b=w.querySelector(".graph-tools button[title='Fit to width']");
if(svg&&b){done++;if(!w.dataset.fitted){w.dataset.fitted=1;
var h=w.dataset.gvHome&&svg.querySelector('[id*="-'+w.dataset.gvHome+'-"]');
if(h){var hr=h.getBoundingClientRect(),sr=shell.getBoundingClientRect();
shell.scrollLeft+=hr.left-sr.left-48;
shell.scrollTop+=hr.top-sr.top-(shell.clientHeight-hr.height)/2}
else if(svg.getBoundingClientRect().width>shell.clientWidth)b.click()}}});
if(wraps.length&&done===wraps.length)clearInterval(t);else if(n>60)clearInterval(t)},120);
document.querySelectorAll(".graph-wrap:not(.gv-over)").forEach(function(w){
var shell=w.querySelector(".graph-shell");if(!shell)return;
function setK(nk,cx,cy){var svg=shell.querySelector("svg");if(!svg)return;
var k=parseFloat(w.dataset.zoom||"1");nk=Math.max(.2,Math.min(2.5,nk));if(nk===k)return;
var r=shell.getBoundingClientRect();
var px=cx-r.left+shell.scrollLeft,py=cy-r.top+shell.scrollTop;
svg.style.width=svg.getBoundingClientRect().width/k*nk+"px";w.dataset.zoom=nk;
shell.scrollLeft=px*nk/k-(cx-r.left);shell.scrollTop=py*nk/k-(cy-r.top)}
shell.addEventListener("wheel",function(e){
if(!e.metaKey&&!e.ctrlKey)return;
e.preventDefault();var m=e.deltaMode===1?16:1;
setK(parseFloat(w.dataset.zoom||"1")*Math.pow(1.0015,-e.deltaY*m),e.clientX,e.clientY)},{passive:false});
var drag=null;
shell.addEventListener("mousedown",function(e){
if(e.button!==0||e.target.closest(".graph-tools"))return;
drag={x:e.clientX,y:e.clientY,l:shell.scrollLeft,t:shell.scrollTop};
shell.classList.add("dragging");e.preventDefault()});
addEventListener("mousemove",function(e){if(!drag)return;
shell.scrollLeft=drag.l-(e.clientX-drag.x);shell.scrollTop=drag.t-(e.clientY-drag.y)});
addEventListener("mouseup",function(){drag=null;shell.classList.remove("dragging")});
});})();
</script>"""

OVER_CAPTION = (
    "Subsystem-level view of the module graph below: each box is a source "
    "directory, each arrow an import dependency between them. The colors "
    "follow each directory through the detail graph and the sections below."
)
DETAIL_CAPTION = (
    "Every module and its imports, grouped and color-keyed by directory. "
    "Opens on the entry point; drag to pan, &#8984;&#8202;scroll or pinch "
    "to zoom, or use the per-module sections below."
)

# Categorical hues for the directory clusters, one slot per cluster in fixed
# (sorted-name) order. Light and dark are the same hues stepped per surface;
# the set passes the CVD/normal-vision gates on both of this site's surfaces,
# and every colored mark also carries its name as text, so color is never the
# only channel. Node text keeps the theme ink — the border and wash carry
# identity.
PALETTE = (
    ("#2a78d6", "#3987e5"),  # blue
    ("#eb6834", "#d95926"),  # orange
    ("#1baf7a", "#199e70"),  # aqua
    ("#eda100", "#c98500"),  # yellow
    ("#e87ba4", "#d55181"),  # magenta
    ("#008300", "#008300"),  # green
    ("#4a3aa7", "#9085e9"),  # violet
    ("#e34948", "#e66767"),  # red
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


def color_css(names: list[str], clusters: dict[str, set[str]]) -> str:
    """Theme-aware cluster colors: tinted node fills, hue strokes, dot vars.

    Selectors go by mermaid's DOM ids (flowchart-<node>-<n>), with !important
    to beat the svg's own id-prefixed stylesheet; the theme toggle scope must
    win over the OS preference in both directions.
    """
    light = ";".join(f"--gv{i}:{PALETTE[i % 8][0]}" for i in range(len(names)))
    dark = ";".join(f"--gv{i}:{PALETTE[i % 8][1]}" for i in range(len(names)))
    rules = [
        f":root{{{light}}}",
        f':root[data-theme="dark"]{{{dark}}}',
        f'@media (prefers-color-scheme:dark){{:root:not([data-theme="light"]){{{dark}}}}}',
        ".gv-dot{display:inline-block;width:.55em;height:.55em;"
        "border-radius:50%;background:var(--c);margin-right:.4em}",
    ]
    for i, c in enumerate(names):
        node_sel = ",".join(
            f'.graph-shell .node[id*="-{n}-"] rect' for n in sorted(clusters[c])
        )
        rules.append(
            f'{node_sel},.graph-shell .node[id*="-gvn{i}-"] rect'
            f"{{stroke:var(--gv{i})!important;"
            f"fill:color-mix(in srgb,var(--gv{i}) 10%,transparent)!important}}"
        )
        rules.append(
            f'.graph-shell .cluster[id*="gvc{i}"] rect'
            f"{{stroke:color-mix(in srgb,var(--gv{i}) 45%,transparent)!important;"
            f"fill:color-mix(in srgb,var(--gv{i}) 6%,transparent)!important}}"
        )
    return "<style>" + "\n".join(rules) + "</style>"


def figures(page: str, mermaid: str) -> tuple[str, dict[str, int]]:
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
    slots = {c: i for i, c in enumerate(names)}

    # the default view anchors on the graph's entry point: nothing imports
    # it, and among those the one that drives the most modules
    targets = {b for _, b in pairs}
    outdeg: dict[str, int] = {}
    for a, _ in pairs:
        outdeg[a] = outdeg.get(a, 0) + 1
    entries = sorted({n for p in pairs for n in p} - targets)
    home = max(entries, key=lambda n: outdeg.get(n, 0)) if entries else ""

    cid = {c: f"gvn{i}" for i, c in enumerate(names)}
    over = (
        [header]
        + [f'  {cid[c]}["{c}"]' for c in names]
        + [f"  {cid[a]} --> {cid[b]}" for a, b in cedges]
    )
    detail = [header]
    for i, c in enumerate(names):
        detail.append(f'  subgraph gvc{i}["{c}"]')
        detail += [f"    {n}" for n in sorted(clusters[c])]
        detail.append("  end")
    detail += [f"  {a} --> {b}" for a, b in pairs]

    esc = lambda t: html.escape("\n".join(t), quote=False)
    home_attr = f' data-gv-home="{home}"' if home else ""
    block = (
        f"{color_css(names, clusters)}\n"
        f'<figure class="graph-wrap gv-over"><div class="graph-shell">'
        f'<pre class="mermaid">{esc(over)}</pre></div>\n'
        f"<figcaption>{OVER_CAPTION}</figcaption></figure>\n"
        f'<figure class="graph-wrap"{home_attr}><div class="graph-shell">'
        f'<pre class="mermaid">{esc(detail)}</pre></div>\n'
        f"<figcaption>{DETAIL_CAPTION}</figcaption></figure>"
    )
    return block, slots


HEAD_RE = re.compile(
    r'<h2><a href="([^"]+)"><code>([\w./-]+/)([\w.-]+)</code></a></h2>'
)
CHIP_RE = re.compile(r'<a href="([^"]+)"><code>([\w./-]+)</code></a>(?:,\s*)?')
CHIPS_BLOCK_RE = re.compile(r'(<p class="chips">depends on )(.*?)(</p>)', re.S)
ATFILE_RE = re.compile(r"<p>@file [\w.-]+ — ")
ATFILE_BARE_RE = re.compile(r"<p>@file [\w.-]+</p>\n?")


def tidy_sections(page: str, slots: dict[str, int]) -> tuple[str, int, int]:
    """Short file-name headings with a directory eyebrow; base-name chips.

    Directories that form a cluster in the graph get its color dot in the
    eyebrow, correlating each section with the graphs above.
    """

    def head(m: re.Match) -> str:
        d = m.group(2).rstrip("/")
        i = slots.get(cluster_of(d))
        dot = (
            f'<i class="gv-dot" style="--c:var(--gv{i})"></i>'
            if i is not None
            else ""
        )
        return (
            f'<h2><span class="arch-dir">{dot}{d}</span>'
            f'<a href="{m.group(1)}"><code>{m.group(3)}</code></a></h2>'
        )

    def chip(m: re.Match) -> str:
        path = m.group(2)
        return f'<a href="{m.group(1)}" title="{path}"><code>{path.rsplit("/", 1)[-1]}</code></a> '

    def chips(m: re.Match) -> str:
        return m.group(1) + CHIP_RE.sub(chip, m.group(2)).rstrip() + m.group(3)

    page, heads = HEAD_RE.subn(head, page)
    page, blocks = CHIPS_BLOCK_RE.subn(chips, page)
    # with short headings, a blurb's "@file <name> — " prefix just repeats
    # the heading directly above it
    page = ATFILE_RE.sub("<p>", page)
    page = ATFILE_BARE_RE.sub("", page)
    return page, heads, blocks


def main() -> int:
    if not ARCH.is_file():
        print("    no rendered site — nothing to restructure")
        return 0

    page = ARCH.read_bytes().decode()
    dirty = False
    slots: dict[str, int] = {}
    if 'class="graph-wrap gv-over"' in page:
        print("    architecture graphs already restructured")
    else:
        m = FIGURE_RE.search(page)
        if not m:
            print(
                "docs_graph: architecture page has no graph figure to "
                "restructure — generator layout changed?",
                file=sys.stderr,
            )
            return 1
        figs, slots = figures(page, html.unescape(m.group(1)))
        page = page[: m.start()] + figs + page[m.end() :]
        dirty = True
        print("    architecture graph split into overview + clustered detail")

    if 'class="arch-dir"' in page:
        print("    module sections already tidied")
    else:
        page, heads, blocks = tidy_sections(page, slots)
        if not heads and not blocks:
            print(
                "docs_graph: architecture page has no module sections to "
                "tidy — generator layout changed?",
                file=sys.stderr,
            )
            return 1
        dirty = True
        print(
            f"    {heads} section heading(s) shortened, "
            f"{blocks} depends-on row(s) compacted"
        )
    if dirty:
        ARCH.write_bytes(page.encode())

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
