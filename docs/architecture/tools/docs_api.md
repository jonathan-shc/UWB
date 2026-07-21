<!-- generated documentation — edit the source, not this file -->
# `tools/docs_api.py`

Fill the reference pages the page generator leaves bare.

The generator documents code where it is defined: functions with bodies,
structs, inline helpers. A header that only *declares* things — prototypes,
macros, enums — renders as a hero line and a "used by" row, which reads as
an empty page even when every declaration in the file carries a doc comment.

This pass parses those headers straight from the working tree and appends
the missing declarations in the generator's own api-entry markup, so the
"On this page" rail and the search palette treat them like any other entry:

  * function prototypes (with their /** brief */ if present),
  * documented #defines, plus undocumented value-carrying ones — a pin map
    is worth listing even uncommented; include guards are not,
  * enum/struct/union declarations the page does not already show.

Anything the page already renders is skipped by anchor id, so running after
the generator adds only what it left out. New entries are also appended to
the search index in nav.js. Run from the repo root, after docs_graph.py and
before the link pass.

## API

### `clean_brief(raw: str) -> str`
`tools/docs_api.py:52`

Doc-comment text -> one inline-HTML sentence, generator style.

**called by** `parse_header`

### `classify(decl: str) -> tuple[str, str, str] | None`
`tools/docs_api.py:63`

A flattened `...;` declaration -> (kind, name, signature) or None.

**called by** `parse_header`

### `parse_header(text: str) -> list[tuple[int, str, str, str, str]]`
`tools/docs_api.py:79`

-> [(line, kind, name, signature, brief-html)] for every declaration.

**called by** `fill_page`  ·  **calls** `classify`, `clean_brief`

### `fill_page(page_path: Path) -> list[tuple[str, str, str, str]]`
`tools/docs_api.py:159`

-> [(kind, name, anchor-href)] appended to this page.

**called by** `main`  ·  **calls** `entry_html`, `parse_header`

### `index_rows(rows: list[tuple[str, str, str, str]]) -> int`
`tools/docs_api.py:187`

Append new entries to the search palette's index in nav.js.

**called by** `main`

<details><summary>Undocumented (2)</summary>

- `entry_html`
- `main`

</details>
