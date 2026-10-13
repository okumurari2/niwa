# niwa のロゴ：Gemini 向けプロンプト

> 2026-10-13: 案(2)（波＋石＋新芽）で作成済み。`assets/logo.png`（原画）、`logo.svg` / `logo-mark.svg`（背景なし、potrace で起こした3色のパス）。この文書は、作り直すとき用に残す。

niwa（庭）のロゴを作るためのプロンプト。そのまま Gemini（画像生成）に貼る。
できた画像は `assets/logo.png`（1024 px 角）と `assets/logo.svg` に置く。置いたら `LICENSING.md`「The logo」を直す（作った人・ライセンス）。

## 方針（プロンプトの根拠）

- niwa = 庭。maku・t8lab・LOFI が育つ土台。「自走する・外れない・育つ」（[PHILOSOPHY.md](PHILOSOPHY.md)）を、庭の形で見せる。
  - **枯山水の砂紋が、1本のサイン波になっている**（音の土台。外れない、整っている）。
  - **石が1つ**（動かない土台。FM-1 の筐体の静けさ）。
  - 新芽は入れても1つまで（育つ）。入れすぎると「エコ・園芸」の絵になる。
- 小さく使える：FM-1 の画面（240 px）や GitHub のアイコン（16〜64 px）でつぶれないこと。
- 色は1色 + 黒か白：アプリのアクセント色の薄紫 `#B48CFF`（niwa.c の `C_ACC`）。
- 避けるもの：Dinghy のロゴ（designburg のロゴが波に乗る絵）に似せない、M-VAVE / FM-1 / maku / Felucca のロゴや名前を入れない。

## プロンプト（英語。そのまま貼る）

```
Design a logo mark for an open-source music-hardware firmware base called "niwa" (庭, Japanese for "garden").

Concept: a Japanese dry garden (karesansui) seen from above. Raked sand lines form ONE clean sine wave that runs across
the mark, because the project is a base for synthesizers. One smooth round stone sits on the wave, still and heavy. Optionally,
a single tiny sprout grows next to the stone, to suggest that things grow to their player's taste. Calm, self-contained,
nothing can go wrong in it.

Style: flat vector, geometric, minimal. Only 3 flat colors in total: a near-black background (#0B0B10), lilac (#B48CFF) for the
sine wave and the sprout, and off-white (#E8E8EC) for the stone. No gradients, no shadows, no 3D, no texture, no outlines
thinner than 4% of the image width. Thick, even strokes with round caps. Lots of empty space around the mark.

Format: square 1:1, 1024 x 1024, the mark centered, filling about 70% of the width. It must stay readable when shrunk to
32 x 32 pixels and in one color only. Plain background, no mockup, no device, no hands.

No text, no letters, no numbers, no watermark.

Do not use: a boat or sail, a signet or seal, a musical note, a keyboard, a speaker, a circuit board, a cherry blossom, bonsai
trees, bamboo, torii gates, or any existing brand logo.

Give me 4 variations: (1) wave + stone only, (2) wave + stone + sprout, (3) the same as (1) but the wave drawn as 3 parallel
raked lines, (4) the mark inside a circle. Show each on the dark background.
```

## できたあと

1. 4案から1つ選び、2〜3回「線をもう少し太く」「石を小さく」などの指示で整える。
2. 余白を詰めた正方形で書き出す（透明背景版も頼む：`Same, on a transparent background`）。
3. SVG が要る：色が3色の平面なので、Inkscape の「ビットマップのトレース」（色数 3）か、Gemini に `Redraw this as clean SVG paths` と頼む。16 px に縮めて確認する。
4. `assets/logo.png`、`assets/logo.svg` に置き、README の先頭に `<p align="center"><img src="assets/logo.svg" width="96" alt="niwa"></p>` を戻す。
5. `LICENSING.md`「The logo」に、作った人とライセンス（GPL に入れるか、ロゴだけ別にするか）を書く。
