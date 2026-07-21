#!/usr/bin/env python3
"""Render a release FLASH.md into a self-contained FLASH.html.

The markdown file stays the single source of truth; this wraps its rendered
body in an embedded stylesheet (light + dark, no external assets) using the
same design language as the documentation site, so the bundle ships a guide
that reads like a page of the docs. The output is committed next to its
source, so regenerate after editing a FLASH.md:

    pip install markdown==3.8
    python3 scripts/flash_html.py release/*/FLASH.md

Output is deterministic (no timestamps): it only changes when the source does.
"""

import pathlib
import sys

import markdown

STYLE = """
:root{
  --ground:#f7f8fa; --surface:#ffffff; --raise:#eff1f5;
  --ink:#1d2129; --strong:#0e1116; --muted:#5c6470; --faint:#8b93a0;
  --line:rgba(18,26,38,.12); --hairline:rgba(18,26,38,.075);
  --accent:#0a69da; --accent-ink:#085ec2; --tint:rgba(10,105,218,.085);
  --warn:#9a6700; --warn-line:rgba(154,103,0,.4); --warn-tint:rgba(212,167,44,.12);
  --mono:ui-monospace,"SF Mono","JetBrains Mono","Cascadia Code",Menlo,Consolas,monospace;
  --sans:-apple-system,BlinkMacSystemFont,"SF Pro Text","Segoe UI",Roboto,Helvetica,Arial,sans-serif;
}
@media (prefers-color-scheme:dark){:root{
  --ground:#0b0c0f; --surface:#121419; --raise:#1a1d24;
  --ink:#d8dde5; --strong:#f4f6fa; --muted:#8f98a5; --faint:#5f6875;
  --line:rgba(226,235,248,.13); --hairline:rgba(226,235,248,.07);
  --accent:#3f97f5; --accent-ink:#7cb5f9; --tint:rgba(77,159,255,.12);
  --warn:#d9a84e; --warn-line:rgba(217,168,78,.45); --warn-tint:rgba(217,168,78,.1);
}}
*{box-sizing:border-box}
html{color-scheme:light dark}
html,body{margin:0;background:var(--ground)}
body{color:var(--ink);font-family:var(--sans);font-size:16px;line-height:1.72;
  -webkit-font-smoothing:antialiased;text-rendering:optimizeLegibility;
  padding:0 1.25rem}
::selection{background:var(--tint)}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px;border-radius:4px}
main{max-width:42.5rem;margin:0 auto;padding:2.6rem 0 7rem;
  animation:rise .45s cubic-bezier(.2,.7,.2,1) both}
@keyframes rise{from{opacity:0;transform:translateY(8px)}}
@media (prefers-reduced-motion:reduce){main{animation:none}}
.brand{display:flex;align-items:baseline;gap:.55rem;margin-bottom:2.2rem}
.brand b{font-family:var(--mono);font-weight:650;font-size:1rem;letter-spacing:-.02em;color:var(--strong)}
.brand .tag{font-size:.58rem;font-weight:650;letter-spacing:.14em;text-transform:uppercase;
  color:var(--faint);border:1px solid var(--line);border-radius:99px;padding:.14rem .45rem}
h1{font-size:1.85rem;font-weight:650;letter-spacing:-.021em;color:var(--strong);
  margin:.15rem 0 1.05rem;text-wrap:balance;line-height:1.18}
h1+p{color:var(--muted)}
h2{font-size:1.25rem;font-weight:650;letter-spacing:-.014em;margin:2.6rem 0 .9rem;color:var(--strong)}
h3{font-size:1.02rem;font-weight:650;margin:1.7rem 0 .45rem;color:var(--strong)}
p{margin:.9rem 0}
ul,ol{padding-left:1.35rem;margin:.9rem 0}
li{margin:.35rem 0}
li>p{margin:.35rem 0}
strong{color:var(--strong);font-weight:650}
a{color:var(--accent-ink);text-decoration:underline;
  text-decoration-color:color-mix(in srgb,var(--accent) 30%,transparent);text-underline-offset:3px}
a:hover{text-decoration-color:var(--accent)}
p code,li code,td code,h2 code{font-family:var(--mono);background:var(--raise);
  padding:.1em .36em;border-radius:5px;font-size:.84em;color:var(--ink)}
.prewrap{position:relative;margin:1.15rem 0}
pre{background:var(--raise);border:1px solid var(--hairline);border-radius:12px;
  padding:1rem 1.2rem;overflow-x:auto;font-family:var(--mono);font-size:.82rem;
  line-height:1.6;color:var(--ink);margin:0}
pre code{font-family:var(--mono)}
.copy{position:absolute;top:.55rem;right:.55rem;border:1px solid var(--line);background:var(--surface);
  color:var(--muted);border-radius:7px;font-family:var(--sans);font-size:.68rem;font-weight:550;
  padding:.26rem .6rem;opacity:0;cursor:pointer;transition:opacity .15s,color .15s,border-color .15s}
.prewrap:hover .copy,.copy:focus-visible{opacity:1}
.copy.done{color:var(--accent-ink);border-color:var(--accent);opacity:1}
.tablewrap{overflow-x:auto;margin:1.2rem 0}
table{border-collapse:collapse;font-size:.88rem;font-variant-numeric:tabular-nums;min-width:100%}
th,td{border:1px solid var(--hairline);padding:.45rem .75rem;text-align:left;vertical-align:top}
th{background:var(--raise);font-weight:600;color:var(--strong)}
blockquote{margin:1.25rem 0;padding:.15rem 1.1rem;background:var(--warn-tint);
  border:1px solid var(--warn-line);border-left:3px solid var(--warn);border-radius:0 12px 12px 0}
blockquote p{margin:.75rem 0}
blockquote strong{color:var(--warn)}
footer{margin-top:4rem;padding-top:1.1rem;border-top:1px solid var(--hairline);
  color:var(--faint);font-size:.8rem}
footer code{font-family:var(--mono);background:var(--raise);padding:.1em .36em;border-radius:5px}
"""

# Copy buttons on the command blocks, matching the docs site's behavior.
SCRIPT = """
document.querySelectorAll("pre").forEach(function(pre){
  var wrap=document.createElement("div");wrap.className="prewrap";
  pre.parentNode.insertBefore(wrap,pre);wrap.appendChild(pre);
  var btn=document.createElement("button");btn.className="copy";btn.type="button";
  btn.textContent="Copy";wrap.appendChild(btn);
  btn.addEventListener("click",function(){
    navigator.clipboard.writeText(pre.textContent.replace(/\\n$/,"")).then(function(){
      btn.textContent="Copied";btn.classList.add("done");
      setTimeout(function(){btn.textContent="Copy";btn.classList.remove("done");},1600);
    });
  });
});
"""

TEMPLATE = """<!doctype html>
<!-- Generated from FLASH.md by scripts/flash_html.py; edit the .md, not this. -->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<style>{style}</style>
</head>
<body>
<main>
<div class="brand"><b>openaliro</b><span class="tag">flash guide</span></div>
{body}
<footer>Rendered from the <code>FLASH.md</code> in this bundle; both carry the
same content. openaliro is evaluation firmware:
<a href="https://github.com/asxeem/openaliro">github.com/asxeem/openaliro</a></footer>
</main>
<script>{script}</script>
</body>
</html>
"""


def render(src: pathlib.Path) -> pathlib.Path:
    text = src.read_text(encoding="utf-8")
    title = next(
        (ln[2:].strip() for ln in text.splitlines() if ln.startswith("# ")),
        src.parent.name,
    )
    body = markdown.markdown(text, extensions=["tables", "fenced_code"])
    # Wide pin tables must scroll inside their own box on a phone.
    body = body.replace("<table>", '<div class="tablewrap"><table>')
    body = body.replace("</table>", "</table></div>")
    out = src.with_suffix(".html")
    out.write_text(
        TEMPLATE.format(title=title, style=STYLE, body=body, script=SCRIPT),
        encoding="utf-8",
    )
    return out


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    for arg in sys.argv[1:]:
        print(render(pathlib.Path(arg)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
