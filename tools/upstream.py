#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Where niwa's platform files come from, and whether they have drifted.

niwa carries the FM-1 platform (hal/, loader/, usb.c, ota.c, ...) as a copy of its upstreams, not as a git merge.
tools/upstream.json lists every such file: which upstream it is a copy of, a hash of niwa's file, and a hash of
the upstream's file at the commit last looked at. docs/UPSTREAM.md says why and what to do with the answers.

  tools/upstream.py check [--no-fetch] [--strict] [--dir NAME=PATH ...]
      LOCAL: a file changed in niwa since the manifest was written (record it: update, with a note)
      MOVED: an upstream's file is not what it was at the recorded commit (look at the diff; adopt or skip)
      exit 1 on LOCAL, and on MOVED too with --strict
  tools/upstream.py update [--no-fetch] [--dir NAME=PATH ...]
      rewrite the hashes and commits from the files and the upstream checkouts as they are now. Run it after you
      have looked at what moved. "from", "adopt" and "note" are yours and are kept.

Upstream checkouts are shallow clones in build/upstream/ (--dir NAME=PATH uses a checkout you already have).
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tools" / "upstream.json"
CACHE = ROOT / "build" / "upstream"


def sha(b):
    return hashlib.sha256(b).hexdigest()[:16]


def git(*a, cwd):
    subprocess.run(["git", *a], cwd=cwd, check=True, capture_output=True, env={**os.environ, "GIT_LFS_SKIP_SMUDGE": "1"})


def checkout(name, up, dirs, fetch):
    if name in dirs:
        return Path(dirs[name])
    d = CACHE / name
    if d.exists():
        if fetch:
            git("fetch", "--depth", "1", "origin", "HEAD", cwd=d)
            git("reset", "--hard", "FETCH_HEAD", cwd=d)
        return d
    if not fetch:
        return None
    CACHE.mkdir(parents=True, exist_ok=True)
    git("clone", "--depth", "1", "https://github.com/" + up["repo"], str(d), cwd=ROOT)
    return d


def blob(d, path):
    p = d / path
    return sha(p.read_bytes()) if p.is_file() else None


def head(d):
    return subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=d, capture_output=True, text=True).stdout.strip()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=["check", "update"])
    ap.add_argument("--no-fetch", action="store_true", help="use only checkouts that are already there")
    ap.add_argument("--strict", action="store_true", help="check: a MOVED upstream file fails too")
    ap.add_argument("--dir", action="append", default=[], metavar="NAME=PATH")
    a = ap.parse_args()
    dirs = dict(x.split("=", 1) for x in a.dir)
    m = json.loads(MANIFEST.read_text())
    ups = {}
    for name, up in m["upstreams"].items():
        d = checkout(name, up, dirs, not a.no_fetch)
        if d is None:
            print(f"skip {name}: no checkout (run without --no-fetch, or give --dir {name}=PATH)")
        ups[name] = d
    bad = moved = 0
    for path, f in sorted(m["files"].items()):
        mine = blob(ROOT, path)
        if a.cmd == "update":
            f["sha256"] = mine
            for name, d in ups.items():
                if d is not None:
                    f.setdefault("upstream", {})[name] = blob(d, path)
            src = f.get("upstream", {}).get(f["from"])
            f["status"] = "same" if src is not None and src == mine else "local"
            continue
        if mine != f["sha256"]:
            print(f"LOCAL  {path}: changed since the manifest was written")
            bad += 1
        for name, d in ups.items():
            if d is None:
                continue
            now, was = blob(d, path), f.get("upstream", {}).get(name)
            if now != was:
                moved += 1
                print(f"MOVED  {path}: {name} {'is gone' if now is None else 'changed'}"
                      f"{' (niwa has that version already)' if now == mine else ''}"
                      f"{'  adopt: ' + f['adopt'] if f.get('adopt') else ''}")
    if a.cmd == "update":
        for name, d in ups.items():
            if d is not None:
                m["upstreams"][name]["commit"] = head(d)
        MANIFEST.write_text(json.dumps(m, indent=1, ensure_ascii=False) + "\n")
        print("manifest updated:", ", ".join(f"{n} {u.get('commit', '?')}" for n, u in m["upstreams"].items()))
        return 0
    print(f"{len(m['files'])} files; {bad} changed locally, {moved} upstream differences")
    return 1 if bad or (a.strict and moved) else 0


sys.exit(main())
