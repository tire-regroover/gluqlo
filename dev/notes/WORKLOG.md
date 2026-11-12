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
