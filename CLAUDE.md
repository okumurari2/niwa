# CLAUDE.md

niwa は FM-1 用ファームの共通基盤。LOFI など、niwa から作ったプロジェクトが `upstream` として取り込む。

## niwa から作ったプロジェクトで作業するとき（LOFI など）

- **niwa のファイルは、そのプロジェクトでは変えない。** 一覧は `tools/niwa_owned.txt`、確認は `python3 tools/check_owned.py`
  （プラットフォーム `platform.c` `hal/` `loader/`、画面 `gfx.c` `quad.h`、`soft.h` `battery.h` `fifths.h`、共通のツールとテスト、
  共通の文書）。
- **niwa のファイルに変更が要るときは、niwa 側で直す。** 手順: ① niwa を clone（または既存の clone）してブランチを切る
  ② 直す ③ `sh tests/run_tests.sh` と `./build.sh` を通す ④ `docs/decisions.md` に1行（理由つき）⑤ niwa に入れる
  ⑥ プロジェクト側で `git fetch upstream && git merge upstream/main`。プロジェクトの作業を止めて待つ必要はない:
  直しを niwa に入れ、同じ内容をプロジェクトに merge すればよい。
- niwa に入れる変更は、そのプロジェクトの名前や仕様に依存させない（`LOFI` などと書かない。差し込み口を足す: `APP`、
  `ID_BASE`、`SETTINGS_MAGIC` のように、プロジェクト側が上書きする形にする）。
- プロジェクトが自分で持つ場所: `firmware/src/<APP>.c` と、その追加ファイル、`tools/build.py` の `APP` / `ID_BASE`、
  `tests/run_host.sh` の `APP`、`README.md`、`LICENSING.md`、`docs/decisions.md`、ロゴ。
- 直し方が分からない、または niwa の方針（`docs/PHILOSOPHY.md`、`docs/UI.md`）に反しそうなときは、変えずにユーザーに聞く。

## niwa 自身で作業するとき

- 共通基盤に入れるのは、2つ以上のプロジェクトが使うもの、または基盤の安全に関わるもの。1つのプロジェクトにしか要らないものは
  そのプロジェクトに置く（`docs/PHILOSOPHY.md` の判断軸）。
- プラットフォームのファイル（`hal/` `loader/` `usb.c` `ota.c` など）を変えたら `tools/upstream.py check` で記録を確かめ、
  `update` で直す（`docs/UPSTREAM.md`）。
- 変更したら `sh tests/run_tests.sh`（ホスト）と、ツールチェーンがあれば `./build.sh`（実機向け）を通す
  （ホストでは実機側のコードがコンパイルされない: `docs/LESSONS.md` 21）。
- 実機で確かめていないことは、確かめていないと書く。
