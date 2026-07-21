<!-- generated documentation — edit the source, not this file -->
# `tools/docs_cmds.py`

Render runnable command blocks as one copy chip per command.

A guide's bash block renders as a plain <pre>: the trailing `# comment` sits
in the same monospace run as the command, and the block-level copy button
copies comments and all. For a block of commands the reader wants the
opposite: each command on its own row, the comment visibly muted, and a Copy
button that yields exactly the command — the chip treatment the landing
page's quick start already uses. The chip CSS and the .js-copycmd handler
ship on every page, so the rewrite is markup only.

Only blocks that are unambiguously command sequences are touched: every
non-blank line must start with an allowlisted command (optionally prefixed
with VAR=value assignments). Device logs, pseudocode and C fragments never
match and render as before.

Run from the repo root, after docs_nav.py and before the link pass.

<details><summary>Undocumented (3)</summary>

- `chip`
- `rewrite`
- `main`

</details>
