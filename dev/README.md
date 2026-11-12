# Developer files

Nothing here is needed to build or run Gluqlo L-Space.

- `tools/check_event.sh sky|hill KIND [SECONDS]` – builds the program, runs one rare sky or hill event on a
  virtual display (Xvfb, needs ImageMagick's `import`) and saves a screenshot to `$OUT` (default
  `/tmp/kindtest/out`). Sky events are short, so the default capture time is 2 s; hill events use 10 s.
- `tools/run_kinds.sh DISPLAY_NUM EFFECT KIND...` – older helper: runs several kinds of one effect in a row.
- `design-sheets/` – the ship design sheets (`ship_designs*.png`) and the C++ programs that drew them.
- `captures/` – contact sheets of screenshots made while adding events, shrunk to keep the repository small.
  `scene_candidates.png` shows the scenes that were considered for `gluqlo.png`.

Contact sheets can be made with ImageMagick, for example
`montage rare_sky-65.png rare_sky-66.png -tile 2x -geometry 512x384+2+2 sheet.png`.
