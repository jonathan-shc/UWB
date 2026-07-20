<!-- generated documentation — edit the source, not this file -->
# `tools/docs_title.py`

Title the generated pages after the repository, not after the checkout directory.

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

<details><summary>Undocumented (2)</summary>

- `git`
- `main`

</details>
