# Gluqlo code review notes

Review of `gluqlo.c` (~2450 lines) done before adding and fixing effects.

## How it works

- Two flip cards drawn with SDL 1.2, SDL_gfx and SDL_ttf.
  - `render_digits` does the flip with `zoomSurface`.
  - `render_clock` is the per-frame driver.
  - A timer pushes an ambient frame every 100 ms (`AMBIENT_FRAME_INTERVAL`).
  - A minute change runs a 260 ms flip (`render_animation`).
- `render_backdrop` builds a cached base (sky, nebula, clouds, three ridge layers).
  Each frame it blits the cache, then draws the dynamic layers.
- Draw order in `render_backdrop`:
  1. Season and weather tint.
  2. `render_fullscreen_event`, `render_weather_effects`, `render_alien_weather`, `render_world_jump`.
  3. Sun, stars, birds.
  4. Fleet battle, background battle, flyby, dogfight, `render_rare_sky_event`, `render_long_scene`,
     `render_hill_event`.
  5. Autumn leaves, rain, snow, warm overlay.
- In `render_clock`, after the cards: card-local rain and snow, `render_foreground_event`, `render_rare_screen_fade`.
- Scheduled effects are pure functions of scene time (`scene_time()` = SDL ticks + a per-launch offset):
  pick a slot with `ticks / cycle`, hash the slot into a seed, gate on `seed % N`, fade in and out.
  No state, drawn straight to the screen.

## Fixed in the second pass

- Effects no longer overlap: `GROUP_TAKEOVER` (world_jump > alien_weather > long_scene > fullscreen >
  hill_event) and `GROUP_SKY` (fleet_battle > background_battle > rare_sky > dogfight > battle_flyby).
  The lower-priority effect skips the whole slot, so nothing pops in half way through.
- `accept` predicates (`rare_sky_accept`, `fullscreen_accept`) make "quiet" slots really empty, so they
  no longer block other effects. Frequencies are unchanged.
- `render_rare_sky_event` is split: one `sky_event_NN()` per kind, a `sky_event_kinds[]` table and a
  `static_assert` that keeps `RARE_SKY_KINDS` in step with it.
- Each launch starts at a different point of the season/weather/effect schedule (`scene_offset`).
  `-seed N` makes that reproducible; `-effect` starts at 0. `fleet_battle` no longer seeds from `time()`.
- Aurora is one helper (`draw_aurora`), used by the weather and the fullscreen event.
  Ground ridge heights come from `ridge_height()`.
- Overlays (`foreground`, `screen_fade`) force the backdrop to be redrawn (once per flip, then reused),
  and card-local rain/snow only draws on cards redrawn that frame, so alpha no longer piles up.
- Argument parsing checks that values exist and are valid; `-r` no longer crashes. Bad options exit 1.
- Makefile builds with `-O2` (it had no optimisation) and uses `CXX`/`-x c++` instead of `CC=g++`.

## Ships, weapons and battles

- Ships are polygon designs (`ship_designs[]`, half outlines mirrored) painted in four bands of light by
  `draw_ship_design()`; they are drawn opaque and blended back at `SHIP_OPACITY`. Three roles:
  small (fighters), mid and big (fleet battle only). `ship_teams[]` holds the dark colours; engines glow.
  Each design has a main weapon and, for mid/big ships, turret mounts (drawn as barrels that differ by
  weapon).
- Nine weapons (`WEAPON_*`, `weapon_specs[]`, `draw_weapon_shot()`): pulse, beam, plasma, mass driver,
  flak, missile, swarm, torpedo, rail. A shot is drawn from its age, so it is stateless. `draw_explosion()`
  draws every blast; a destroyed ship gets the big one with fragments and secondary blasts.
- `fleet_battle` places ships with `fleet_pose()`; each gun fires on its own schedule (`fleet_fire()`);
  the losing fleet loses all but its last ship one at a time (`fleet_death()`).
- `dogfight` (one pair) and `background_battle` (2-3 pairs) share `render_dogfight()`: chaser and leader fly
  looping routes, tilt along their heading, the chaser fires its weapon, one volley hits and on the last
  pass the leader is destroyed. `battle_flyby` is the old simple pass with pulse bolts.
- `hill_event` has 95 kinds (`hill_event_kinds[]`; 0-36 listed here, 37-77 are in the work log): village, caravan, turbines, lanterns, campfire,
  bloom, herd, volcano, beacons, fireflies, kites, fireworks, train, rocket, abduction, geysers, stones,
  then ferris wheel, observatory, windmills, light tree, meteorite, dust devils, lasers, tesla towers,
  balloons, giant, clock tower, drones, bats, gate, then lighthouse, cherry tree, wolf, pumpkins, castle,
  space elevator.
- `rare_sky` has 139 kinds (23 classic + `sky_event_23..138`; the newer ones are in the work log); kinds 43-50 are hot air balloons, geese,
  meteor shower, triangle lights, skywriting loop, sky whale, storm cell and eclipse.
- Unique visuals in total (kinds 0-22 of `rare_sky` are only 3 distinct classic visuals, so count those
  as 3): 5 alien weather + 4 world jump + 5 long scene + 119 rare sky + 4 fullscreen + 4 foreground +
  95 hill + 5 single-kind effects (screen fade, flyby, background battle, fleet battle, dogfight) = 241.
  Every kind was smoke-tested with `-effect NAME:KIND` (see `dev/tools/check_event.sh`), and sky kinds
  were also drawn mid-screen at full size with `dev/tools/draw_sky.sh`. The plant growth is
  `draw_hill_garden()`, used by `long_scene` kind 0.
- Spaceship events are rare: gates are fleet_battle 6, background_battle 9, battle_flyby 6, dogfight 4.
  Measured over 48 h of scene time, starts per hour went from 5.0 / 8.3 / 16.8 / 17.0 to
  2.5 / 4.1 / 7.7 / 9.5 (about half). Blocking by higher-priority effects means gates are not simply
  doubled; re-measure after changing any gate.
- Rain angle: a rain streak leans the way the drop drifts (`0.16 + wind * 0.22` of its length). Alien
  weather particles all fall down and lean right (direction 0.25, 1.0) for every kind, so nothing falls
  sideways or rises.
- Test harnesses: include `gluqlo.c` with `-Dmain=...` and call the draw functions offscreen.
  `dev/` holds the event check script, the ship design sheets and contact sheets of screenshots.
- To add a sky or hill event: put the function above the `NEW_SKY_EVENTS_ABOVE` / `NEW_HILL_EVENTS_ABOVE`
  marker comment, add it to the table above `NEW_SKY_KINDS_ABOVE` / `NEW_HILL_KINDS_ABOVE`, and bump
  `RARE_SKY_KINDS` / `HILL_EVENT_KINDS` (a `static_assert` fails the build if they disagree).
- Everything was checked on a virtual display (Xvfb) and offscreen only, not on a real screen.

## Still open

- The bodies of the moved `sky_event_NN()`, `render_world_jump()` and `render_long_scene()` keep the
  extra indentation they had when they were branches of one big function.
- Performance: every 100 ms frame redraws all dynamic layers with SDL_gfx. `GLUQLO_PROFILE=1` logs slow frames.
- `long_scene` can still run alongside rare sky events and battles (different groups).

## Effect table

Scheduled effects are described by an `EffectSpec` (name, cycle, gate, duration, fades, group, priority,
accept) and share `effect_window()`, which returns the slot seed, the phase inside the active window and
the fade factor. See the comment above `EffectSpec` in `gluqlo.c` for how to add one.

Testing options:

    gluqlo -effect list            # print effect names and kind ranges
    gluqlo -effect NAME            # show only that effect, repeating, other scheduled effects off
    gluqlo -effect NAME:KIND       # additionally force its sub-kind (where it has one)
    gluqlo -seed N                 # start the scenery at a reproducible point
