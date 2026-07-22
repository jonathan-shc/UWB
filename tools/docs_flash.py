#!/usr/bin/env python3
"""Publish the browser flasher: site/flash/ = the web-flasher/ page + firmware.

The page and its ESP Web Tools manifest are committed in web-flasher/; the
merged firmware image is not. GitHub release assets are served without CORS
headers (probed 2026-07-22: neither the github.com redirect nor its CDN
answers Access-Control-Allow-Origin), so the browser cannot fetch the image
from the release; it has to sit next to the page on the same origin. This
pass stages it at site-build time, preferring in order:

  1. web-flasher/openaliro-matter-lock.bin (gitignored): a local
     `idf.py merge-bin` output for bench runs, published with the committed
     manifest (version "dev").
  2. The latest release's loose assets (openaliro-matter-lock.bin +
     openaliro-matter-lock.manifest.json, uploaded by release.yml), fetched
     server side where CORS does not apply; the manifest arrives already
     version-stamped.
  3. Neither: skip the page entirely, loudly. An Install button whose
     firmware 404s is worse than no page, and before the first release this
     is the normal state of a fresh checkout.

Run from the repo root, after the link pass: the page is standalone and its
links are absolute, so it needs no rewriting. docs.sh drives it.
"""

from __future__ import annotations

import re
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path

SITE = Path("site")
SRC = Path("web-flasher")
FIRMWARE = "openaliro-matter-lock.bin"
RELEASE_MANIFEST = "openaliro-matter-lock.manifest.json"


def repo_slug() -> str:
    """owner/repo for the origin remote, or '' if none."""
    try:
        url = subprocess.run(
            ["git", "remote", "get-url", "origin"],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return ""
    m = re.search(r"[:/]([^/:]+)/([^/]+?)(?:\.git)?$", url)
    if not m:
        return ""
    return f"{m.group(1)}/{m.group(2)}"


def fetch(url: str) -> bytes:
    with urllib.request.urlopen(url, timeout=60) as resp:
        return resp.read()


def main() -> int:
    if not SITE.is_dir():
        print("docs_flash: site/ not found — run the generators first", file=sys.stderr)
        return 1

    dst = SITE / "flash"
    shutil.rmtree(dst, ignore_errors=True)

    local = SRC / FIRMWARE
    if local.is_file():
        dst.mkdir(parents=True)
        shutil.copyfile(local, dst / FIRMWARE)
        shutil.copyfile(SRC / "manifest.json", dst / "manifest.json")
        source = f"local {local} (manifest version dev)"
    else:
        slug = repo_slug()
        if not slug:
            print("    no origin remote and no local image — flash page skipped")
            return 0
        base = f"https://github.com/{slug}/releases/latest/download/"
        try:
            image = fetch(base + FIRMWARE)
            manifest = fetch(base + RELEASE_MANIFEST)
        except OSError as err:
            print(f"    no local image and no release asset ({err})")
            print("    flash page skipped — publish a release or drop a merged")
            print(f"    image at {local} and rerun")
            return 0
        dst.mkdir(parents=True)
        (dst / FIRMWARE).write_bytes(image)
        (dst / "manifest.json").write_bytes(manifest)
        source = f"latest release ({len(image)} bytes)"

    shutil.copyfile(SRC / "index.html", dst / "index.html")
    print(f"    {dst / 'index.html'} (firmware: {source})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
