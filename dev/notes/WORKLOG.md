# Work log

How the L-Space events were added, with the commands used, so the work can be repeated or checked.

## Method

Every event was added one at a time, so the program built and ran after each one:

1. Write the function above the `NEW_SKY_EVENTS_ABOVE` / `NEW_HILL_EVENTS_ABOVE` marker in `gluqlo.c`.
2. Add it to the table above `NEW_SKY_KINDS_ABOVE` / `NEW_HILL_KINDS_ABOVE`.
3. Bump `RARE_SKY_KINDS` / `HILL_EVENT_KINDS`. A `static_assert` fails the build if the count and table disagree.
4. Run `dev/tools/check_event.sh sky|hill KIND [seconds]`. It builds, runs the event on a virtual display
   (Xvfb, 1280x800 root, window at 128,16) with `-effect NAME:KIND -seed 3`, and saves a screenshot.
5. Look at the screenshot (or a contact sheet of ten) before moving on.

Rare sky events last only 4-7 s, so they are captured after 2 s; hill events last 30 s or more and are
captured after 10 s.

## Counting

`rare_sky` kinds 0-22 all draw one of only 3 classic objects, so they count as 3 visuals, not 23.

| After | Distinct visuals |
|---|---|
| Before the second session | 103 (3 + 42 sky + 31 hill + 27 others) |
| First ten new events | 113 |
| Events added after that (33 of the planned 100 so far) | 143 |
| Third session: 4 hill + 5 sky | 152 (the code count gives 155) |
| Fourth session: 24 hill + 28 sky in five batches | 207 |
| Fifth session: events added one at a time (see below) | 221 and counting |

The 27 "others": alien weather 5, long scene 5, world jump 4, fullscreen 4, foreground 4, and one each for
screen fade, flyby, background battle, fleet battle and dogfight.

## Events added in this session

Sky (kind number = 23 + index in `sky_event_kinds[]`, so `sky_event_65` is `rare_sky:65`):

- 65 paper planes, 66 soap bubbles, 67 ringed planet, 68 constellation
- 69 origami cranes, 70 sky lanterns, 71 owl, 72 leaf swirl, 73 murmuration
- 74 aurora curtain, 75 great comet, 76 pterosaurs, 77 butterflies, 78 zeppelin
- 79 saucer formation, 80 plasma ball, 81 red sprites, 82 space station, 83 binary star
- 84 hawk, 85 party balloons, 86 pollen, 87 mist dragon, 88 sky galleons

Hill (`hill_event:N`):

- 31 lighthouse, 32 cherry tree, 33 howling wolf, 34 pumpkins, 35 castle, 36 space elevator
- 37 airstrip, 38 hot springs, 39 night market, 40 lotus lights, 41 bonfire festival
- 42 waterfall, 43 scarecrows and crows, 44 pagoda, 45 ice skaters, 46 shepherd and sheep
- 47 stone bridge, 48 totem poles, 49 snowmen

Third session (after a disconnect; the pending dev/ files were committed first):

- Sky 89 jellyfish, 90 manta rays, 91 smoke rings, 92 hang gliders, 93 maple seeds
- Hill 50 sunflowers, 51 crystals, 52 glowing mushrooms, 53 tents (pitch, light up, campfire smoke)
- Manta rays were first too dark to see against the sky and were brightened.
- Maple seeds (93) are small and mostly sit at the screen edge in the first seconds; they do draw.

Fourth session (goal: over 200). Hill 54-77 and sky 94-121, five batches, each built and looked at before commit:

- Hill 54 fountain, 55 harvester, 56 cairns, 57 watermill, 58 marsh, 59 stargazers, 60 carousel,
  61 trebuchet, 62 orchard, 63 radio mast, 64 greenhouse, 65 swings, 66 beehives, 67 cableway, 68 ruins,
  69 geodome, 70 cabin, 71 wind chimes, 72 picnic, 73 wishing tree, 74 mine, 75 dragon eggs, 76 hammock,
  77 telegraph poles
- Sky 94 koi, 95 pinwheels, 96 contrails, 97 ribbon, 98 moth, 99 phoenix, 100 diamond dust, 101 solar
  sailer, 102 ringed gas giant, 103 dragonflies, 104 petals, 105 zodiac ring, 106 spiral galaxy,
  107 hummingbird, 108 umbrellas, 109 swans, 110 flying fish, 111 floating island, 112 lenticular cloud,
  113 dolphins, 114 pinwheel firework, 115 thunderbird, 116 crescent moon boat, 117 crystal sphere,
  118 music staff, 119 kite, 120 twin comets, 121 bead curtain
- Sky events are timed and move across the screen, so a screenshot taken at a fixed delay often misses them
  (diamond dust looked missing until the object was drawn directly). `dev/tools/draw_sky.sh FIRST LAST`
  draws sky kinds offscreen, mid-screen at a fixed size, and makes a contact sheet
  (`dev/captures/sheet_skyharness_*.png`). `dev/tools/batch_sheet.sh` does the same for a range of hill
  kinds on the virtual display.
- Fixes made after looking: cairns, contrails and moth brightened; diamond dust glints made larger then
  toned down; dolphins had the wrong heading and overlapped; thunderbird wings were too dark.
- Several new events are small, so on a real screen they are subtle. Not checked on a real screen.
- Honest count: the total is 207 distinct visuals by the counting rule above. By raw kind number it is
  122 sky + 78 hill + 27 others = 227, but 20 sky kinds (0-22) collapse to 3 classic visuals.

Fifth session (one event per commit, each looked at before committing):

- Sky 122 sea turtle, 123 prayer flags, 124 spinning top, 125 feather, 126 lantern-fish, 127 star mobile,
  128 cloud wave
- Hill 78 hamlet, 79 stepping stones, 80 seesaw, 81 laundry line, 82 well, 83 kiln, 84 rope bridge
- Lantern-fish were first dim and overlapping, so they were brightened and spaced out. The cloud wave was
  first a plain arch with stray spray dots (an unsigned-arithmetic slip in the offset); it was rewritten as a
  curling swell.
- The wording "well over a hundred" became "hundreds of" in the `gluqlo.c` header and `gluqlo.xml`.
- README has an openSUSE section; the package names are from memory and untested.

Screenshots are in `dev/captures/events/` (`rare_sky-N.jpg`, `hill_event-N.jpg`); contact sheets are in
`dev/captures/` (`sheet_*.jpg`).

## Findings

- Sky events are placed at 8-46 % of the screen height by the shared `render_rare_sky_event()`. The clock
  cards cover roughly 16-77 % and are translucent, so most sky events (old and new) sit partly or wholly
  behind the cards. Hill events draw on the near ridge below the cards and are always visible.
  The shared placement was not changed.
- The first screenshots of sky events looked blank because they were taken after 5 s, when many had already
  finished. Capturing at 2 s fixed that. Kinds 53 and 58 are small and dim but do draw.
- All checks were done on a virtual display only, never on a real screen, and only as still frames.

## Repository state notes

- The remote `l-space` branch had 16 commits that the local branch did not, so the work was pushed to
  `l-space-100` instead and the local branch renamed to match.
- `gh repo edit` for the About text failed with HTTP 403 (the Codespaces token has no admin rights), so the
  About text has to be set on GitHub by hand.
- `preview.png` is a single screenshot taken with
  `gluqlo -w 1024 -h 768 -ampm -effect fleet_battle -seed 3` after 24 s.

## Things to avoid (from earlier feedback)

Rocket ships, fireflies, "bouncing pixel" motes, ships shaped like mouse pointers or bottles, ships that are
too bright, and rain that falls sideways or rises.
