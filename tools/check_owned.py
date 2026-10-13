#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""In a firmware built on niwa: are niwa's files (tools/niwa_owned.txt) still niwa's?

  tools/check_owned.py [--fetch]

Compares each of them with the version in the last niwa commit this repository has merged (the merge base with the git remote
`upstream/main`), so a niwa that has moved on since is not a difference. A file changed here is a change that belongs in niwa:
make it there, then `git fetch upstream && git merge upstream/main` (CLAUDE.md). Exit 1 on a difference. In niwa itself (no
`upstream` remote) it does nothing. --fetch: `git fetch upstream` first.
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def git(*a, check=True, raw=False):
    r = subprocess.run(["git", *a], cwd=ROOT, capture_output=True, check=check)
    if not raw:
        r.stdout = r.stdout.decode()
    return r


def main():
    if "upstream" not in git("remote").stdout.split():
        print("check_owned: no `upstream` remote: this is niwa itself (or add it: git remote add upstream <niwa's URL>)")
        return 0
    if "--fetch" in sys.argv:
        git("fetch", "upstream", "main")
    base = git("merge-base", "HEAD", "upstream/main", check=False)
    if base.returncode:
        print("check_owned: no common history with upstream/main (git fetch upstream; in CI: fetch-depth 0)")
        return 1
    base = base.stdout.strip()
    bad = []
    for line in (ROOT / "tools" / "niwa_owned.txt").read_text().splitlines():
        p = line.split("#")[0].strip()
        if not p:
            continue
        names = git("ls-tree", "-r", "--name-only", base, "--", p).stdout.split()
        names += [n for n in git("ls-files", "--", p).stdout.split() if n not in names]
        for n in names:
            was = git("show", f"{base}:{n}", check=False, raw=True)
            f = ROOT / n
            if was.returncode:                                  # not in niwa at that commit: a file added here
                bad.append(f"added   {n}")
            elif not f.is_file():
                bad.append(f"removed {n}")
            elif was.stdout != f.read_bytes():
                bad.append(f"changed {n}")
    for b in sorted(set(bad)):
        print("OWNED  " + b)
    print(f"check_owned: {len(set(bad))} of niwa's files differ from niwa {base[:7]}" +
          ("; make the change in niwa and merge it" if bad else ""))
    return 1 if bad else 0


sys.exit(main())
