# maku から何を持ってくるか

niwa の土台は Dinghy（Felucca / SLOOP のプラットフォームだけを残したもの、約 6,000 行）。
maku は Felucca 1.x の全体（エンジン・シーケンサー・ページ・メニュー）をそのまま抱え、その上に MAKU を足した派生で、ファイル数は約 240。
**MAKU の UI システムは、その中の薄い層にすぎない。** ここでは、その層と、maku で得た作法の実装だけを持ってくる範囲を決める。

調べた版：maku `73d9d1a`、t8lab `683ff21`（2026-10-13）。

## 1. 分かったこと

- MAKU の画面は `ui_maku.c`（242 行）だけで描かれ、Felucca の画面（`ui_draw.c` の「ページの上に4区画をかぶせる」、`ui_draw.c` 1327 行目）の上に乗っている。
- `ui_maku.c` が `gfx.c`（847 行）から使うのは、ごく一部：`cv_begin` `cv_blit` `cv_rect` `cv_line_t` `cv_text` `cv_text_in` `text_w`、フォント `AF_S` `AF_M` `AF_L`、`ux_mix`、トークン `T_*`。
  キーキャップ（`KC_*`）、アイコン（Fukiai）、角丸、`cv_alpha_hc` などは Felucca のページ・メニュー用で、MAKU の4区画は使わない。
- つまり ui 系の約 9,000 行（`ui.c` `ui_draw.c` `ui_graph.c` `ui_input.c` `ui_layer.c` `ui_menu.c` `ui_name.c` `ui_slice.c` `ui_events.c` `ui_tools.c`）の大半は Felucca のページ UI で、MAKU の様式には要らない。
  ただし `ui_input.c` には MAKU の操作（フォーカス、動詞の押下、`battery_notice`、`leds_eff`）が混ざっているので、関数単位で抜く。
- t8lab は maku の丸ごとのコピーに `t8/`（JS のエミュレーターとコア）と `t8_core.c` `t8_mode.c` `mode/mode.h` を足したもの。t8lab の画面（`t8/src/screen.js`）は `ui_maku.c` の写し。

## 2. 持ってくるもの

| 何 | maku の場所 | niwa での形 | 理由 |
| --- | --- | --- | --- |
| 4区画の描画の骨組み | `firmware/src/ui_maku.c` | トラック名・パラメータ名を外から渡す形に一般化（`mq_macro` `mq_gauge` `mq_top_left` の様式部分、署名で描き直し） | UI システムの本体 |
| 描画キャンバスの必要部分 | `firmware/src/gfx.c`：`cv_*` の基本、AA 文字（`text_run` `text_w` `ink_in` `cv_text_in` `text_fit`）、トークン `T_*` と `palette_set` | Dinghy の `gfx.c` を置き換え（`cv_begin` / `cv_blit` の形は同じ） | フォント3つ・トークン・インクで揃える、の実装 |
| AA フォントの生成 | `tools/gen_aa_font.py` `tools/aa_raster.py`、`assets/fonts/InterTight[wght].ttf`（OFL） | `tools/build.py` の生成段階に | Inter Tight 3 サイズ。L の文字集合の制限もここ |
| パレットの生成と検査 | `tools/gen_ui_palettes.py` | 同上。WCAG 検査でビルドを落とす | 追記のみ・コントラスト検査 |
| トラック色 | `ui_maku.c` `mk_col` `mk_ink` `mk_mix` | 色の表を CFW 側に出す | 唯一の例外色の扱い |
| `MANNER:fade-in` `MANNER:erase-dip` | `firmware/src/audio.c` の SOFT（`soft_fade_in` `soft_down_wait` `soft_up`）、`storage_hw.c` `st_erase` | Dinghy の `platform.c` の音声出口と `settings_save()` に入れる | Dinghy の `settings_save()` は消去で音が止まる（RULES.md「Save when silent」） |
| `MANNER:rec-preerase` | `storage.c` `st_blank` `st_tidy` | 保存が要る CFW のときに | いいねを持つ CFW で要る |
| `MANNER:battery` `MANNER:leds-auto` | `ui_input.c` `battery_notice` `leds_eff`、`ui_draw.c` `batt_level`（しきい値 531 / 561 / 591）、`panel.c` | 小さな関数として | 電池駆動の前提 |
| LED の起動スイープ | `firmware/hal/fm1_led_anim.h` | `hal/` に | 「スイープ中は触らない」の前提 |
| いいねの仕組み | `maku_fav.c`、`project.c` の保存（`OBJ_MAKUFAV`） | 「育つ」を持つ CFW 用の部品として（形だけ。中身は CFW ごと） | 16 件・押し出し・id で参照・壊れたら空から |
| テスト | `tests/text_spacing_test.py`、`tests/theme_test.c`、`maku_test.c` の「回しても全画面が塗られない」 | `tests/` に | 様式をテストで固定する |
| 文書 | `docs/UI_RULES.md`、decisions.md の教訓 | [UI.md](UI.md) [LESSONS.md](LESSONS.md) [PHILOSOPHY.md](PHILOSOPHY.md)（済） | — |

## 3. 持ってこないもの

| 何 | 理由 |
| --- | --- |
| Felucca のページ UI（`ui_graph.c` `ui_menu.c` `ui_name.c` `ui_slice.c` `ui_events.c` `ui_tools.c` `ui_layer.c` の大半、`menu_items.c`） | MAKU の様式は「ページを作らない」。設定メニューが要る CFW には、最小のものを別に作る |
| 音源一式（`eng_*.c` `fm6_core.c` `phys_*.c` `drum_voice.c` など） | CFW ごとの中身。LOFI が要るものだけ、そのとき個別に持ってくる。maku でも未使用の音源が約 28 KB 残っている（maku ABOUT） |
| シーケンサー・プロジェクト形式（`seq.c` `project.c` `song_chain.c` `favorites.c` `params.c` の保存形式） | 「書かない・戻らない」。いいねの保存だけを部品として抜く |
| `perform.c` の古い動詞（FREEZE TAPE SWEEP REPEAT …）とそのテスト | maku でも割り当てなしのまま残っている |
| Felucca 側のテストのうち、上の音源・シーケンサー用のもの | 持ってきた部品のテストだけを持つ |
| `t8/` の JS 一式 | t8lab 固有。エミュレーターの作り方の例として参照するだけ |
| `mode/mode.h`（draft 0） | 正本は maku のブランチ `mode-h-draft`。形が固まり、呼び出し元ができてから（t8lab `docs/MODES.md`）。niwa に正本を移すかは別に決める |

## 4. 予約済みの操作・領域（ぶつけない）

| 何 | 使っているもの |
| --- | --- |
| 電源投入時に OCT− | USB レスキュー（niwa の `platform.c`） |
| OCT− + OCT+ を 5 秒 | UBOOT（maku `main.c`）。パネル設定にも使用（t8lab MODES.md） |
| HOME + OCT− | maku の設定メニュー |
| フラッシュ `[0x4000, 0x97000)` | アプリ |
| フラッシュ `[0xE0000, 0xE5000)` | OTA ステージング |
| `0xE5000 / 0xE6000` | maku のいいね（公式への復帰で消える） |
| `0xFC000` 以降 | 使わない |

niwa の設定セクタ（`settings_load()` / `settings_save()`）の場所は [API.md](API.md)「Memory and flash」。

## 5. 進め方（案）

1. `gfx.c` を maku の必要部分で置き換え、フォントとパレットの生成をビルドに入れる。ホストモードで PNG を出し、Dinghy の画面が崩れないことを見る。
2. 4区画の骨組みを、トラック・パラメータの表を渡せば描ける形で入れる。`niwa.c` を4区画で描く例にする。
3. SOFT（fade-in / erase-dip）を `platform.c` に入れ、`settings_save()` を包む。
4. 電池通知・LED AUTO。
5. ここまで来たら LOFI を載せる。いいねの部品は LOFI が要るときに。

各段で `tests/run_host.sh` と `./build.sh`（ツールチェーンのある環境）を通し、decisions.md に1行足す。
