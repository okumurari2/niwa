# Make it yours

niwa is meant to be built on: a firmware of your own starts as a copy of it. This is how that copy becomes your firmware, gets released, and gets into the
[FM-1 Firmware Hub](https://fm1.designburgapps.com) next to the others.

## 1. Copy

Make your repository from niwa. Either **Use this template** on GitHub, or, to keep niwa's history and take its updates later, clone niwa, push it to your new repository and keep niwa as a remote called `upstream` (see [UPSTREAM.md](UPSTREAM.md)). Then build once before changing anything, so you know your setup
works: `./build.sh` and `tests/run_host.sh` (see the [README](../README.md)).

## 2. Name it

- `niwa.c` is yours: rename it (`git mv`) and set the name in the two places that know it: `APP` at the top of `tools/build.py`
  (file names of the build and the package) and `APP` in `tests/run_host.sh`.
- At its top, before `#include "platform.c"`:

  ```c
  #define APP_NAME "MYSYNTH"        /* the rescue and the crash screen say this */
  ```

- Keep your changes to the platform files (`platform.c`, `hal/`, `loader/`) to none, and put yours in files of your own: then
  `git merge upstream/main` takes niwa's updates without conflicts.
- A record of your own: `SETTINGS_MAGIC` (four letters, as a number), so that your firmware never reads another's settings:
  `#define SETTINGS_MAGIC 0x........u` at the top of your app, before `#include "platform.c"`.
- Make your own logo (niwa's is `assets/logo.*`, once it is made: it marks niwa itself, not your firmware).

## 3. Your package identity

Every FM-1 package carries an identity like `FM-1_990`. The FM-1 reports the one it runs, and installers and the
hub use it to tell firmwares apart: two firmwares with the same identity look like one.

- niwa is `FM-1_95N` (N = the minor version). **Pick your own**: `ID_BASE` at the top of `tools/build.py` (`FM-1_96` for 96N).
- Keep the form `FM-1_9` + two digits if you can: the web installers then also finish an install that was cut
  off halfway (they resume only the update loaders they know, `FM-1_9NN`).
- Taken so far: `900` SLOOP, `901` MAKU (0.1), `91N` Felucca, `93N` sloopDX, `97N` zp12, `98N` (a Doom port), `95N` niwa, `99N` Dinghy,
  and others in the hub's list (each entry of its `catalog.json` has the pattern it matches; checked 2026-10-13: it mentions
  no `94N`, `95N` or `96N`, but the entries that name no pattern may use any). There is no registry: look at the catalog, and ask the hub if you are unsure.

## 4. Release

```sh
./build.sh --release 0.2        # -> build/niwa-0.2.fwsc, identity FM-1_952, the version "0.2" on the screen
```

On GitHub: a release with a tag (`v0.2`) and the `.fwsc` attached. Say what changed in its notes: the hub links
them.

## 5. Into the hub

The [FM-1 Firmware Hub](https://fm1.designburgapps.com) installs open-source FM-1 firmwares with one click,
and checks every night for new releases. To be listed:

- the source is public and GPL-3.0 (niwa is: keep it so);
- the releases are on GitHub, each with its `.fwsc`;
- a screenshot of your firmware's screen: `tests/run_host.sh` makes one (`build/host/*.png`), or a photo;
- you ask: the hub's *"Made an open-source firmware?"* card says how.

After that every release you make shows up there by itself.

## Your own fonts

To draw with a face niwa does not have (a big numeral, say), list it in `tools/app_fonts.txt`, one `--face` spec per line (the format and a sample: `tools/gen_aa_font.py`, `tests/app_fonts_sample.txt`). The build makes `build/gen/app_fonts.h` from it, `gfx.c` includes it, and the app draws with `&AF_<NAME>`. A big face costs flash: a 64 px face of 41 glyphs is about 60 KB.

## 6. Keep the credits

Your firmware stands on niwa, which stands on Dinghy (zvenson), SLOOP (isod89) and Felucca (Leo Kuroshita, Hügelton Instruments). The GPL asks you to
keep their copyright lines and to give your users your source; it also is just fair. Keep `LICENSING.md` and
the headers, and add yourself.

## Ideas to start from

- A second oscillator, detuned: two phases, one `inc` a little bigger.
- An envelope per key with the four knobs (`ENC_KNOB1..4`) as attack, decay, sustain, release.
- A one-pole low-pass: `y += (x - y) * k >> 15` per frame; `k` from a knob.
- A step sequencer on the white keys, `BTN_PLAY` to start, the tempo on `ENC_SELECT`.
- Look at how zp12 or sloopDX do it: same platform, much bigger apps.
