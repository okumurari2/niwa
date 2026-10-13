# Upstream: where the platform comes from

niwa's platform (`firmware/hal/`, `firmware/loader/`, `usb.c`, `ota.c`, `lcd.c`, `libc.c`, `uac_fir.h`, `bootguard.h`,
`crt0.S`, `app.ld`, and some of `tools/`) is not niwa's own work. It is a copy, and people keep working on the originals.

```
upstreams:  SLOOP (isod89)   Felucca (hugelton)   felucca-bsp (hugelton)
                  \               |                    :
                   \__ copied, with a record of where from __   (felucca-bsp: later, see below)
                                  |
                                niwa      <- tools/upstream.py says what moved
                                  |
                            LOFI and every other firmware: a git remote called `upstream` that points at niwa
```

A firmware built on niwa takes niwa's updates with an ordinary `git merge upstream/main`. It does not need to know the
upstreams; niwa takes their changes in, tests them together, and passes them on. `tools/upstream.py` is niwa's tool: a firmware
built on niwa does not run it (its own `tools/build.py` differs from niwa's by design). A firmware's own changes stay out of
niwa's files: what it needs there is a change to make in niwa. [LOFI](https://github.com/okumurari2/lofi) is the first.
`CLAUDE.md` tells whoever works in such a firmware (a person or Claude) to make the change in niwa, and `tools/check_owned.py` (run in
its CI) fails when a niwa file was changed there instead: the list is `tools/niwa_owned.txt`.

## Why a recorded copy, and not a git merge

- Felucca is a whole application (engines, sequencer, editor). niwa keeps about thirty files of it. A merge would
  bring back everything niwa left out.
- The upstreams do not agree with each other: SLOOP and Felucca both hardened the update loader, in different ways.
  A merge cannot choose; a person has to, once, with a device in hand.
- A copy with a record shows what moved, and leaves the decision where it belongs.

## The record

`tools/upstream.json` lists every platform file in niwa: the upstream it is a copy of, a hash of niwa's file, a hash of the
upstream's file at the commit last looked at, whether the two are the same (`status`), and what was decided:

| `adopt` | means |
| --- | --- |
| `""` | nothing to decide: the same as the upstream, or deliberately different (the note says why) |
| `skip` | looked at, not taken (comments, features niwa has no use for) |
| `later` | worth taking when a firmware needs it and it can be tested on a device |

```sh
python3 tools/upstream.py check           # LOCAL: niwa's file changed, not recorded.  MOVED: an upstream's file changed
python3 tools/upstream.py update          # after you have looked: rewrite the hashes and commits
```

`check` needs the network (shallow clones in `build/upstream/`); `--no-fetch` uses what is there. `LOCAL` makes it fail, so
it can run in CI to catch a platform file changed without a note. `MOVED` only informs (`--strict` fails on it too):
run it now and then, and when an upstream publishes a release.

## Where niwa stands (2026-10-13)

Looked at: SLOOP `8fad1a1` (2.6), Felucca `9f6a885` (1.5.6), felucca-bsp `b899449`.

- **niwa = SLOOP, nearly.** 29 of 36 recorded files are byte for byte SLOOP's. The other seven are niwa's on purpose, or about
  to be replaced: `fm1_flash.h` (niwa has no store of its own yet), `fm1_irq.h` (a comment), `recovery.c` (the name),
  `usb.c`, `gfx.c`, `tools/build.py`, `tools/fm1_install.py`.
- **Felucca 1.5.6 is ahead of SLOOP in places**: `app.ld` (RAM / POOL split), `lcd.c` and `fm1_lcd_hw.h` (SCREEN OFF),
  `uac_fir.h`, `fm1_input.h`, `usb.c`, `ota.c`, the loader. Marked `later` in the record, each with what it is. Many other
  differences are comments only (`skip`).
- **Nothing was taken from Felucca 1.5.x in this step**, on purpose: the loader and USB are what can brick a unit, niwa's platform was
  tested on a device as Dinghy, and the 1.5 changes cannot be tested here. They are listed so that nobody has to find them again.
- Not examined in detail: the differences in `usb.c`, `uac_fir.h`, `fm1_input.h`, `ldr_core.c` against Felucca. The record says so.
- One change goes the other way: niwa's `usb.c` ends a cut-off SysEx reply with an F7 on the next send (`sx_open`). SLOOP has
  no such fix. It is worth offering to SLOOP.

## felucca-bsp

The platform part of Felucca, being split out by Leo Kuroshita: `chip/`, `loader/`, `boards/fm1/`. As of `b899449`:

- it **cannot be built on its own** yet and says "do not build a release on this yet";
- its licences are **provisional** (MPL-2.0 intended, but `chip/usb.c` stays GPL-3.0-only, so a firmware that includes it is GPL-3.0 as a whole);
- file names and layout may still change; it already has experimental parts (a second core, expanded flash, BLE) that have not run on a device.

So niwa does not take from it yet. The plan: when it builds and its licences are final, add it to the record as a third upstream
and move `hal/`, `loader/`, `usb.c`, `ota.c` over to it, one directory at a time, each with a device test. Until then it is only
read.

## When something moved

1. `tools/upstream.py check`, read each MOVED line and its `adopt` note.
2. `diff` the file against the upstream's (`build/upstream/<name>/<path>`).
3. Take it, or write why not in the note. Build (`./build.sh`), run `tests/run_host.sh`, and flash a unit before a release.
4. `tools/upstream.py update`, commit the manifest with the change.
