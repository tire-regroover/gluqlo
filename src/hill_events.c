/*
 * The garden: plants grow on the near ridge, one after another. There are five
 * kinds (round trees, pines, cypresses, flowering bushes and giant glowing
 * mushrooms) in one of three colour themes (lush, autumn, blossom), with grass
 * sprouting between them. Saplings shoot up, canopies fill out, trees sway in
 * the wind, and late in the event the bushes flower and the trees bear fruit.
 */
static void draw_hill_garden(RenderTarget *surface, Uint32 ticks, int scale, Uint32 seed,
                             double progress, double fade) {
  static const Uint8 themes[3][3][3] = {
      {{44, 84, 60}, {96, 154, 88}, {245, 238, 200}},   // lush
      {{118, 58, 30}, {214, 130, 52}, {232, 196, 76}},  // autumn
      {{58, 96, 72}, {116, 170, 116}, {248, 152, 196}}  // blossom
  };
  const Uint8 *dark = themes[seed % 3][0];
  const Uint8 *light = themes[seed % 3][1];
  const Uint8 *flower = themes[seed % 3][2];
  int width = surface->w;
  Uint8 alpha = (Uint8)(215 * fade);
  scale = scale * 17 / 10;  // plants are drawn larger than the other hill things

  // Grass first, so the plants stand in it.
  for (int blade = 0; blade < 96; blade++) {
    unsigned int h = fx_hash(seed, 500 + blade);
    double x = width * (blade + 0.5) / 96.0 + (int)(h % 9) - 4;
    double sprout = smooth_step(0.02, 0.3, progress - (h % 11) * 0.012);
    double length = scale * (0.010 + (h >> 8) % 8 * 0.002) * sprout;
    if (length < 1.0) continue;
    double sway = sin(ticks / 650.0 + blade * 0.8) * length * 0.35;
    double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
    thickLineRGBA(surface, (int)x, (int)base, (int)(x + sway), (int)(base - length), 1,
                  (Uint8)(dark[0] + 20), (Uint8)(dark[1] + 24), (Uint8)(dark[2] + 14),
                  (Uint8)(alpha * 0.8));
  }

  const int plants = 11;
  for (int plant = 0; plant < plants; plant++) {
    unsigned int h = fx_hash(seed, plant);
    double r = (h % 100) / 100.0;
    double x = width * (0.05 + 0.9 * plant / (plants - 1.0)) + (int)((h >> 8) % 36) - 18;
    double start = 0.03 + plant * 0.028;
    double growth = smooth_step(start, start + 0.45, progress);
    double canopy = smooth_step(start + 0.28, start + 0.65, progress);
    if (growth <= 0.0) continue;
    double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
    double sway = sin(ticks / 900.0 + plant * 1.3) * scale * 0.004 * canopy;
    // A fixed mix of the five kinds, rotated by the seed, so every garden has
    // some of each.
    static const int species_plan[11] = {0, 1, 3, 0, 2, 1, 4, 0, 3, 1, 2};
    int species = species_plan[(plant + seed) % 11];
    int bx = (int)x, by = (int)base;

    if (species == 0) {
      // Round tree: a trunk and a crown of overlapping blobs, lit from above.
      double th = scale * (0.030 + 0.014 * r) * growth;
      int tx = (int)(x + sway * 0.3), ty = (int)(base - th);
      thickLineRGBA(surface, bx, by, tx, ty, 2 + (int)(growth * 2), 54, 40, 34, alpha);
      double crown = scale * (0.022 + 0.008 * r) * canopy;
      for (int blob = 0; blob < 5; blob++) {
        double angle = blob * 1.2566 + r * 3.0;
        int cx = (int)(x + sway + cos(angle) * crown * 0.75);
        int cy = (int)(base - th - crown * 0.4 + sin(angle) * crown * 0.5);
        filledCircleRGBA(surface, cx, cy, (int)(crown * 0.62) + 1, dark[0], dark[1], dark[2],
                         alpha);
      }
      for (int blob = 0; blob < 3; blob++) {
        double angle = blob * 2.1 + r * 3.0 + 0.6;
        int cx = (int)(x + sway + cos(angle) * crown * 0.5 - crown * 0.15);
        int cy = (int)(base - th - crown * 0.75 + sin(angle) * crown * 0.3);
        filledCircleRGBA(surface, cx, cy, (int)(crown * 0.4) + 1, light[0], light[1], light[2],
                         (Uint8)(alpha * 0.85));
      }
      double fruit = smooth_step(0.72, 0.9, progress);
      if (fruit > 0.0 && canopy > 0.9) {
        for (int f = 0; f < 5; f++) {
          double angle = f * 1.7 + r * 5.0;
          int fx = (int)(x + sway + cos(angle) * crown * 0.8);
          int fy = (int)(base - th - crown * 0.4 + sin(angle) * crown * 0.5);
          filledCircleRGBA(surface, fx, fy, 2, flower[0], flower[1], flower[2],
                           (Uint8)(alpha * fruit));
        }
      }
    } else if (species == 1) {
      // Pine: stacked tiers that widen toward the ground.
      double th = scale * (0.045 + 0.016 * r) * growth;
      thickLineRGBA(surface, bx, by, bx, (int)(base - th * 0.25), 2, 54, 40, 34, alpha);
      for (int tier = 0; tier < 4; tier++) {
        double tier_base = base - th * (0.12 + tier * 0.2);
        double tier_width = scale * 0.017 * (1.0 - tier * 0.2) * (0.3 + 0.7 * canopy);
        double tier_top = tier_base - th * 0.3;
        int shift = (int)(sway * (tier + 1) * 0.35);
        filledTrigonRGBA(surface, (int)(x - tier_width) + shift / 2, (int)tier_base,
                         (int)(x + tier_width) + shift / 2, (int)tier_base, bx + shift,
                         (int)tier_top, dark[0], dark[1], dark[2], alpha);
        filledTrigonRGBA(surface, (int)(x - tier_width * 0.5) + shift / 2, (int)tier_base - 1,
                         bx + shift / 2, (int)tier_base - 1, bx + shift, (int)tier_top, light[0],
                         light[1], light[2], (Uint8)(alpha * 0.6));
      }
    } else if (species == 2) {
      // Cypress: a tall slim flame of leaves.
      double ch = scale * (0.05 + 0.018 * r) * growth;
      double cw = scale * 0.0085 * (0.4 + 0.6 * canopy);
      filledEllipseRGBA(surface, (int)(x + sway * 0.6), (int)(base - ch * 0.5), (int)cw + 1,
                        (int)(ch * 0.5) + 1, dark[0], dark[1], dark[2], alpha);
      filledEllipseRGBA(surface, (int)(x + sway * 0.6 - cw * 0.3), (int)(base - ch * 0.55),
                        (int)(cw * 0.45) + 1, (int)(ch * 0.4) + 1, light[0], light[1], light[2],
                        (Uint8)(alpha * 0.7));
    } else if (species == 3) {
      // Bush that comes into flower late in the event.
      double bush = scale * 0.018 * growth;
      for (int blob = 0; blob < 3; blob++) {
        filledCircleRGBA(surface, (int)(x + (blob - 1) * bush * 0.8 + sway * 0.4),
                         (int)(base - bush * 0.6), (int)(bush * (blob == 1 ? 0.8 : 0.6)) + 1,
                         dark[0], dark[1], dark[2], alpha);
      }
      double bloom = smooth_step(0.5 + start * 0.4, 0.78 + start * 0.4, progress);
      for (int f = 0; f < 9; f++) {
        unsigned int fh = fx_hash(h, f);
        int fx = (int)(x + ((int)(fh % 100) - 50) * bush * 0.022 + sway * 0.4);
        int fy = (int)(base - bush * (0.4 + (fh >> 8) % 60 / 100.0));
        if ((fh >> 16) % 100 > bloom * 100.0) continue;
        filledCircleRGBA(surface, fx, fy, 2, flower[0], flower[1], flower[2], alpha);
        pixelRGBA(surface, fx, fy, 255, 255, 235, alpha);
      }
    } else {
      // Giant mushroom with a softly glowing cap.
      double mh = scale * (0.026 + 0.010 * r) * growth;
      double cap = scale * 0.016 * (0.3 + 0.7 * canopy);
      boxRGBA(surface, bx - 2, (int)(base - mh), bx + 2, by, 150, 142, 130, alpha);
      double pulse = 0.65 + 0.35 * sin(ticks / 420.0 + plant);
      filledCircleRGBA(surface, bx, (int)(base - mh), (int)(cap * 1.8) + 1, light[0], light[1],
                       light[2], (Uint8)(26 * fade * pulse * canopy));
      filledEllipseRGBA(surface, bx, (int)(base - mh), (int)cap + 1, (int)(cap * 0.7) + 1,
                        dark[0] + 40, dark[1] + 10, dark[2] + 20, alpha);
      boxRGBA(surface, bx - (int)cap - 1, (int)(base - mh), bx + (int)cap + 1, (int)(base - mh) + 2,
              30, 24, 30, alpha);
      for (int spot = 0; spot < 3; spot++) {
        filledCircleRGBA(surface, bx + (int)((spot - 1) * cap * 0.55),
                         (int)(base - mh - cap * 0.3 - (spot == 1 ? cap * 0.15 : 0)),
                         1 + (int)(cap * 0.12), 190, 255, 230,
                         (Uint8)(235 * fade * pulse * canopy));
      }
    }
  }
}


static void render_long_scene(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_LONG_SCENE, ticks, &window)) return;
  Uint32 seed = window.seed;
  double progress = window.progress;
  double fade = window.fade;
  int kind = effect_kind(&EFFECT_LONG_SCENE, seed % 7);

  if (kind == 0) {
    draw_hill_garden(surface, ticks, scale, seed, progress, fade);
  } else if (kind == 1) {
    int flood_y = surface->h * (82 - (int)(progress * 9)) / 100;
    boxRGBA(surface, 0, flood_y, surface->w - 1, surface->h - 1, 28, 79, 116, (Uint8)(58 * fade));
    for (int band = 0; band < 5; band++) {
      int previous_x = 0;
      int previous_y = flood_y + band * scale / 38;
      for (int segment = 1; segment <= 20; segment++) {
        int next_x = surface->w * segment / 20;
        int next_y = flood_y + band * scale / 38 +
                     (int)(sin(segment * 0.8 + ticks / (500.0 + band * 80)) * scale * 0.012);
        thickLineRGBA(surface, previous_x, previous_y, next_x, next_y, 1 + band % 2, 115, 183, 208,
                      (Uint8)(70 * fade));
        previous_x = next_x;
        previous_y = next_y;
      }
    }
  } else if (kind == 2) {
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 135, 57, 27, (Uint8)(17 * fade));
    for (int fire = 0; fire < 12; fire++) {
      int fire_x = surface->w * (8 + fire * 7) / 100;
      double nx = (double)fire_x / surface->w;
      double ridge = ridge_height(RIDGE_MIDDLE, nx);
      int fire_y = (int)(surface->h * ridge);
      int flicker = (int)(fabs(sin(ticks / (170.0 + fire * 11) + fire)) * scale * 0.025);
      thickLineRGBA(surface, fire_x, fire_y, fire_x + 2, fire_y - flicker, 2, 238, 112, 55,
                    (Uint8)(120 * fade));
      filledCircleRGBA(surface, fire_x + 4, fire_y - flicker, 2, 255, 192, 106,
                       (Uint8)(145 * fade));
    }
  } else if (kind == 3) {
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 36, 57, 78, (Uint8)(24 * fade));
    for (int band = 0; band < 6; band++) {
      int previous_x = 0;
      int previous_y = surface->h * (17 + band * 10) / 100;
      for (int segment = 1; segment <= 18; segment++) {
        int next_x = surface->w * segment / 18;
        int next_y = surface->h * (17 + band * 10) / 100 +
                     (int)(sin(segment * 0.6 + ticks / 1400.0 + band) * scale * 0.05);
        thickLineRGBA(surface, previous_x, previous_y, next_x, next_y, 2, 78, 162, 163,
                      (Uint8)(28 * fade));
        previous_x = next_x;
        previous_y = next_y;
      }
    }
  } else if (kind == 4) {
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 17, 20, 32, (Uint8)(34 * fade));
    int figure_x = surface->w * (25 + seed % 50) / 100;
    int figure_y = surface->h * 83 / 100;
    int figure_height = scale / 10;
    filledTrigonRGBA(surface, figure_x, figure_y - figure_height, figure_x - figure_height / 3,
                     figure_y, figure_x + figure_height / 3, figure_y, 12, 16, 25,
                     (Uint8)(150 * fade));
    filledCircleRGBA(surface, figure_x, figure_y - figure_height, figure_height / 6, 18, 22, 30,
                     (Uint8)(160 * fade));
  } else if (kind == 5) {
    // Bioluminescent tide: glowing cyan-emerald surf swells up the near ridge
    // with drifting sea-foam motes and pulsing wave crests.
    int tide_y = surface->h * (85 - (int)(sin(progress * M_PI) * 11)) / 100;
    boxRGBA(surface, 0, tide_y, surface->w - 1, surface->h - 1, 14, 58, 72, (Uint8)(64 * fade));
    for (int band = 0; band < 4; band++) {
      int prev_x = 0;
      int prev_y = tide_y + band * scale / 34;
      for (int seg = 1; seg <= 22; seg++) {
        int next_x = surface->w * seg / 22;
        int next_y = tide_y + band * scale / 34 +
                     (int)(sin(seg * 0.75 + ticks / (420.0 + band * 90)) * scale * 0.011);
        thickLineRGBA(surface, prev_x, prev_y, next_x, next_y, 2, 70, 228, 210,
                      (Uint8)((95 - band * 16) * fade));
        prev_x = next_x;
        prev_y = next_y;
      }
    }
    static const Uint8 bio[3] = {80, 245, 215};
    for (int i = 0; i < 24; i++) {
      unsigned int h = fx_hash(seed, 700 + i);
      double fx = fmod((h % 1000) / 1000.0 + progress * (0.2 + (h % 5) * 0.04), 1.0) * surface->w;
      double fy = tide_y + (h % 100) / 100.0 * (surface->h - tide_y);
      double pulse = 0.5 + 0.5 * sin(ticks / 340.0 + i * 1.3);
      weapon_glow(surface, fx, fy, 1.5 + (h % 3) * 0.6, bio, (85.0 + 95.0 * pulse) * fade);
    }
  } else {
    // Slow skyward migration: a convoy of silent mid-ships and glowing
    // lanterns drifts across the entire sky over the long scene.
    int dir = (seed & 1) ? 1 : -1;
    int len = scale / 24;
    if (len < 16) len = 16;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * 0.85 * fade);
    for (int s = 0; s < 5; s++) {
      double offset = s * 0.14;
      double u = (progress * 1.4 - offset);
      if (u < -0.1 || u > 1.1) continue;
      double sx = (dir > 0) ? -len + u * (surface->w + 2.0 * len)
                            : surface->w + len - u * (surface->w + 2.0 * len);
      double sy =
          surface->h * (0.14 + (s % 3) * 0.06) + sin(ticks / 1400.0 + s * 1.7) * scale * 0.014;
      draw_ship_role(surface, (int)sx, (int)sy, (s == 2) ? SHIP_MID : SHIP_SMALL, seed + s * 19,
                     (s == 2) ? (int)(len * 1.5) : len, dir, s % 2, opacity);
    }
  }
}


/*
 * Hill events
 *
 * Things that happen on the hills. Each is a small function taking a HillEvent.
 * They stand on the near ridge (the one below the clock cards, so they are
 * seen), except the volcano, which sits on the middle ridge at one edge of the
 * screen. `t` is seconds since the event began, `progress` runs 0..1 over it,
 * and `fade` fades the whole thing in and out. To add one: write hill_xxx(),
 * append it to hill_event_kinds[] and bump HILL_EVENT_KINDS.
 */
typedef struct {
  RenderTarget *surface;
  Uint32 ticks;
  int scale;
  Uint32 seed;
  double t;
  double progress;
  double fade;
  int direction;  // 1: things go left to right, -1: right to left
} HillEvent;

// A repeatable random number in 0..1 for the event.
static double hill_random(const HillEvent *event, int a, int b) {
  return (fx_hash(event->seed + a * 7919u, b) % 1000) / 1000.0;
}


// A small walking figure with a lantern on a pole.
static void hill_walker(RenderTarget *surface, double x, double ground, double height,
                        int direction, double step, bool lantern, double fade, double t,
                        int index) {
  Uint8 alpha = weapon_alpha(235.0 * fade);
  double hip = ground - height * 0.45;
  double swing = sin(step) * height * 0.22;
  int width = height > 22 ? 3 : 2;
  thickLineRGBA(surface, (int)x, (int)hip, (int)(x + swing), (int)ground, width, 12, 15, 22, alpha);
  thickLineRGBA(surface, (int)x, (int)hip, (int)(x - swing), (int)ground, width, 12, 15, 22, alpha);
  thickLineRGBA(surface, (int)x, (int)hip, (int)(x + direction * height * 0.05),
                (int)(ground - height * 0.8), width + 1, 14, 18, 26, alpha);
  filledCircleRGBA(surface, (int)(x + direction * height * 0.07), (int)(ground - height * 0.92),
                   (int)(height * 0.1) + 1, 14, 18, 26, alpha);
  if (lantern) {
    double hand_x = x + direction * height * 0.2, hand_y = ground - height * 0.58;
    double lamp_x = x + direction * height * 0.42, lamp_y = ground - height * 0.5;
    thickLineRGBA(surface, (int)hand_x, (int)hand_y, (int)lamp_x, (int)(lamp_y - height * 0.1), 1,
                  12, 15, 22, alpha);
    static const Uint8 warm[3] = {255, 190, 100};
    weapon_glow(surface, lamp_x, lamp_y, height * 0.11 * (0.9 + 0.1 * sin(t * 9.0 + index)), warm,
                235.0 * fade);
    ellipseRGBA(surface, (int)lamp_x, (int)ground, (int)(height * 0.8), (int)(height * 0.09), 255,
                190, 100, weapon_alpha(40.0 * fade));
  }
}


// A village of small houses: windows light up one by one, chimneys smoke, and a
// church tower stands in the middle.
static void hill_village(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int houses = 8;
  for (int house = 0; house < houses; house++) {
    double r = hill_random(event, house, 1), r2 = hill_random(event, house, 2);
    double x = surface->w * (0.06 + 0.88 * (house + 0.5 + (r - 0.5) * 0.5) / houses);
    double base = ground_y(surface, RIDGE_NEAR, x) + event->scale * 0.004;
    double width = event->scale * (0.04 + 0.016 * r2);
    double height = width * (0.75 + 0.5 * r);
    bool tower = house == 3;
    if (tower) {
      width *= 0.7;
      height *= 2.4;
    }
    double arrive = smooth_step(0.0, 0.1, event->progress - house * 0.012);
    if (arrive <= 0.0) continue;
    Uint8 alpha = weapon_alpha(235.0 * event->fade * arrive);
    int x0 = (int)(x - width / 2), x1 = (int)(x + width / 2);
    int y1 = (int)base, y0 = (int)(base - height);
    boxRGBA(surface, x0, y0, x1, y1, 16, 20, 30, alpha);
    int roof = (int)(width * (tower ? 1.2 : 0.55));
    filledTrigonRGBA(surface, x0 - 2, y0, x1 + 2, y0, (int)x, y0 - roof, 10, 12, 20, alpha);
    int windows = tower ? 2 : 2 + (int)(r * 2.0);
    for (int k = 0; k < windows; k++) {
      double lit = smooth_step(0.15 + house * 0.045 + k * 0.03, 0.2 + house * 0.045 + k * 0.03,
                               event->progress) *
                   (1.0 - smooth_step(0.9, 0.97, event->progress));
      if (lit <= 0.0) continue;
      double flicker = 0.88 + 0.12 * sin(event->t * 7.0 + house * 3.1 + k);
      int wx = x0 + (int)(width * (k + 0.5) / windows) - 1;
      int wy = y0 + (int)(height * (0.3 + 0.25 * (k % 2)));
      filledCircleRGBA(surface, wx + 1, wy + 2, 6, 255, 190, 100,
                       weapon_alpha(32.0 * lit * event->fade));
      boxRGBA(surface, wx, wy, wx + 2, wy + 3, 255, 214, 130,
              weapon_alpha(240.0 * lit * flicker * event->fade));
    }
    if (!tower && house % 2 == 0) {
      int chimney_x = x1 - 4, chimney_y = y0 - roof / 2 - 3;
      boxRGBA(surface, chimney_x, chimney_y, chimney_x + 2, chimney_y + 5, 12, 15, 24, alpha);
      for (int puff = 0; puff < 5; puff++) {
        double age = fmod(event->t / 2.2 + puff * 0.2 + house * 0.13, 1.0);
        filledCircleRGBA(surface, (int)(chimney_x + sin(age * 5.0 + house) * 3.0 + age * 8.0),
                         (int)(chimney_y - age * event->scale * 0.07), (int)(1.5 + age * 3.5), 150,
                         152, 162, weapon_alpha(85.0 * (1.0 - age) * event->fade));
      }
    }
  }
}


// A line of travellers with lanterns walks along the ridge.
static void hill_caravan(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int walkers = 7;
  double spacing = event->scale * 0.05;
  double margin = event->scale * 0.1;
  double travel = surface->w + 2.0 * margin + walkers * spacing;
  double lead = event->progress * travel - margin;
  for (int walker = 0; walker < walkers; walker++) {
    double progress_x = lead - walker * spacing;
    double x = event->direction > 0 ? progress_x : surface->w - progress_x;
    if (x < -20 || x > surface->w + 20) continue;
    double ground = ground_y(surface, RIDGE_NEAR, x) + 1.0;
    double height = event->scale * (walker == 3 ? 0.046 : 0.04);
    if (walker == walkers - 1) height *= 0.7;
    hill_walker(surface, x, ground, height, event->direction, event->t * 6.0 + walker * 1.9,
                walker % 2 == 0, event->fade, event->t, walker);
  }
}


// Wind turbines rise on the ridge and begin to turn, with red lights blinking
// on top.
static void hill_turbines(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int turbines = 5;
  Uint8 alpha = weapon_alpha(235.0 * event->fade);
  for (int i = 0; i < turbines; i++) {
    double x = event->surface->w * (0.07 + 0.86 * i / (turbines - 1.0));
    double rise = smooth_step(0.02 + i * 0.03, 0.14 + i * 0.03, event->progress);
    if (rise <= 0.0) continue;
    double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
    double tower = event->scale * (0.1 + 0.016 * (i % 2)) * rise;
    int top_x = (int)x, top_y = (int)(base - tower);
    thickLineRGBA(surface, (int)x, (int)base, top_x, top_y, 3, 20, 26, 38, alpha);
    thickLineRGBA(surface, (int)x + 1, (int)base, top_x + 1, top_y, 1, 52, 62, 82,
                  (Uint8)(alpha * 0.8));
    double blade = tower * 0.5;
    double spin = event->t * (1.1 + 0.18 * i) * (0.4 + 0.6 * rise) + i;
    for (int b = 0; b < 3; b++) {
      double angle = spin + b * 2.0944;
      int tip_x = top_x + (int)(cos(angle) * blade), tip_y = top_y + (int)(sin(angle) * blade);
      thickLineRGBA(surface, top_x, top_y, tip_x, tip_y, 3, 28, 34, 48, alpha);
      thickLineRGBA(surface, top_x, top_y, tip_x, tip_y, 1, 90, 104, 128, (Uint8)(alpha * 0.8));
    }
    filledCircleRGBA(surface, top_x, top_y, 3, 40, 46, 62, alpha);
    if (fmod(event->t + i * 0.37, 1.6) < 0.35 && rise > 0.9) {
      static const Uint8 red[3] = {255, 60, 50};
      weapon_glow(surface, top_x, top_y - 4, 3.0, red, 235.0 * event->fade);
    }
  }
}


// Paper lanterns float up from the hills, swaying as they climb and fading near
// the top.
static void hill_lanterns(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int lanterns = 28;
  for (int i = 0; i < lanterns; i++) {
    double rx = hill_random(event, i, 1), ph = hill_random(event, i, 2),
           rs = hill_random(event, i, 3);
    double cycle = 13.0 + rs * 8.0;
    double u = fmod(event->t / cycle + ph, 1.0);
    double x0 = surface->w * (0.03 + 0.94 * rx);
    double x = x0 + sin(event->t * 0.8 + i) * event->scale * 0.012 +
               u * event->scale * 0.06 * event->direction;
    double y = ground_y(surface, RIDGE_NEAR, x0) - u * surface->h * 0.62;
    double envelope = smooth_step(0.0, 0.1, u) * (1.0 - smooth_step(0.7, 1.0, u));
    double size = event->scale * (0.013 + 0.006 * rs);
    double flicker = 0.85 + 0.15 * sin(event->t * 8.0 + i * 2.0);
    Uint8 a = weapon_alpha(255.0 * envelope * event->fade);
    filledCircleRGBA(surface, (int)x, (int)y, (int)(size * 3.4), 255, 140, 60,
                     weapon_alpha(22.0 * envelope * event->fade * flicker));
    filledCircleRGBA(surface, (int)x, (int)y, (int)(size * 2.2), 255, 160, 70,
                     weapon_alpha(40.0 * envelope * event->fade * flicker));
    boxRGBA(surface, (int)(x - size * 0.5), (int)(y - size * 0.65), (int)(x + size * 0.5),
            (int)(y + size * 0.65), 255, (Uint8)(150 + 40 * flicker), 70, a);
    boxRGBA(surface, (int)(x - size * 0.3), (int)(y - size * 0.4), (int)(x + size * 0.3),
            (int)(y + size * 0.4), 255, 224, 150,
            weapon_alpha(220.0 * envelope * event->fade * flicker));
    lineRGBA(surface, (int)(x - size * 0.5), (int)(y - size * 0.65), (int)(x + size * 0.5),
             (int)(y - size * 0.65), 90, 40, 20, a);
    lineRGBA(surface, (int)(x - size * 0.5), (int)(y + size * 0.65), (int)(x + size * 0.5),
             (int)(y + size * 0.65), 90, 40, 20, a);
  }
}


// A tent and a campfire at the edge of the screen; two figures sit by the fire
// and sparks and smoke rise from it.
static void hill_campfire(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double x = event->direction > 0 ? surface->w * 0.075 : surface->w * 0.925;
  double ground = ground_y(surface, RIDGE_NEAR, x) + 2.0;
  double s = event->scale * 1.4;
  double build =
      smooth_step(0.0, 0.1, event->progress) * (1.0 - smooth_step(0.9, 1.0, event->progress));
  Uint8 alpha = weapon_alpha(235.0 * event->fade);
  // The tent stands on the far side of the fire.
  double tent_x = x - event->direction * s * 0.06;
  double tent_width = s * 0.035;
  filledTrigonRGBA(surface, (int)(tent_x - tent_width), (int)ground, (int)(tent_x + tent_width),
                   (int)ground, (int)tent_x, (int)(ground - s * 0.045), 22, 28, 40, alpha);
  filledTrigonRGBA(surface, (int)tent_x, (int)ground, (int)(tent_x + event->direction * s * 0.012),
                   (int)ground, (int)tent_x, (int)(ground - s * 0.03), 8, 10, 16, alpha);
  // The glow of the fire on the ground and the tent.
  filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.09), (int)(s * 0.014), 255, 150, 60,
                    weapon_alpha(34.0 * event->fade * build));
  double flare = 0.85 + 0.15 * sin(event->t * 11.0) + 0.1 * sin(event->t * 17.0);
  filledCircleRGBA(surface, (int)x, (int)(ground - s * 0.015), (int)(s * 0.05 * flare), 255, 140,
                   50, weapon_alpha(26.0 * event->fade * build));
  thickLineRGBA(surface, (int)(x - s * 0.012), (int)ground, (int)(x + s * 0.012),
                (int)(ground - s * 0.004), 3, 44, 30, 24, alpha);
  thickLineRGBA(surface, (int)(x + s * 0.012), (int)ground, (int)(x - s * 0.012),
                (int)(ground - s * 0.004), 3, 54, 36, 26, alpha);
  for (int tongue = 0; tongue < 5; tongue++) {
    double sway = sin(event->t * (9.0 + tongue * 2.0) + tongue) * s * 0.003;
    double h = s * (0.018 + 0.006 * (tongue % 3)) * build *
               (0.75 + 0.25 * sin(event->t * 13.0 + tongue * 2.3));
    double w = s * 0.007;
    double fx = x + (tongue - 2) * s * 0.004;
    filledTrigonRGBA(surface, (int)(fx - w), (int)(ground - 2), (int)(fx + w), (int)(ground - 2),
                     (int)(fx + sway), (int)(ground - h), 250, 120, 40,
                     weapon_alpha(215.0 * event->fade));
    filledTrigonRGBA(surface, (int)(fx - w * 0.55), (int)(ground - 2), (int)(fx + w * 0.55),
                     (int)(ground - 2), (int)(fx + sway), (int)(ground - h * 0.75), 255, 205, 110,
                     weapon_alpha(225.0 * event->fade));
  }
  for (int spark = 0; spark < 12; spark++) {
    double u = fmod(event->t / 1.7 + spark * 0.083, 1.0);
    double sx =
        x + sin(spark * 7.0 + event->t * 2.5) * s * 0.014 * u + event->direction * u * s * 0.01;
    double sy = ground - s * 0.02 - u * s * 0.13;
    filledCircleRGBA(surface, (int)sx, (int)sy, spark % 3 == 0 ? 2 : 1, 255, (Uint8)(210 - 90 * u),
                     100, weapon_alpha(240.0 * (1.0 - u) * event->fade * build));
  }
  for (int puff = 0; puff < 6; puff++) {
    double u = fmod(event->t / 4.5 + puff / 6.0, 1.0);
    filledCircleRGBA(surface,
                     (int)(x + sin(u * 4.0 + puff) * s * 0.01 + u * s * 0.03 * event->direction),
                     (int)(ground - s * 0.03 - u * s * 0.17), (int)(s * (0.006 + 0.014 * u)), 120,
                     120, 134, weapon_alpha(70.0 * (1.0 - u) * event->fade * build));
  }
  // Two figures sit by the fire, facing it.
  for (int person = 0; person < 2; person++) {
    double px = x + (person == 0 ? -s * 0.028 : s * 0.034);
    int facing = px < x ? 1 : -1;
    filledEllipseRGBA(surface, (int)px, (int)(ground - s * 0.008), (int)(s * 0.01),
                      (int)(s * 0.009), 14, 18, 26, alpha);
    filledCircleRGBA(surface, (int)(px + facing * s * 0.002), (int)(ground - s * 0.024),
                     (int)(s * 0.005) + 1, 14, 18, 26, alpha);
    filledCircleRGBA(surface, (int)(px + facing * s * 0.005), (int)(ground - s * 0.022), 1, 255,
                     170, 90, weapon_alpha(200.0 * event->fade * build));
  }
}


// Glowing plants along the ridge: waves of light pass through them, with a few
// that are much bigger than the rest.
static void hill_bloom(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int plants = 34;
  static const Uint8 colors[3][3] = {{90, 235, 215}, {175, 125, 255}, {255, 125, 205}};
  for (int i = 0; i < plants; i++) {
    double r = hill_random(event, i, 1);
    double x = event->surface->w * (0.02 + 0.96 * (i + 0.5 + (r - 0.5) * 0.6) / plants);
    double grow = smooth_step(0.0 + i * 0.004, 0.18 + i * 0.004, event->progress) *
                  (1.0 - smooth_step(0.93, 1.0, event->progress));
    if (grow <= 0.0) continue;
    double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
    double tall = event->scale * (0.018 + 0.03 * r * r) * grow;
    double wave = 0.5 + 0.5 * sin(x * 0.018 * event->direction - event->t * 3.3);
    double lean = sin(event->t * 0.9 + i) * tall * 0.12;
    const Uint8 *c = colors[i % 3];
    int top_x = (int)(x + lean), top_y = (int)(base - tall);
    filledEllipseRGBA(surface, (int)x, (int)base, (int)(tall * 0.7) + 2, 2, c[0], c[1], c[2],
                      weapon_alpha(40.0 * wave * event->fade * grow));
    thickLineRGBA(surface, (int)x, (int)base, top_x, top_y, 1, 30, 70, 64,
                  weapon_alpha(220.0 * event->fade));
    thickLineRGBA(surface, (int)x, (int)(base - tall * 0.4), (int)(x - tall * 0.25),
                  (int)(base - tall * 0.55), 1, 30, 70, 64, weapon_alpha(200.0 * event->fade));
    double bulb = (2.0 + tall * 0.12) * (0.8 + 0.4 * wave);
    weapon_glow(surface, top_x, top_y, bulb, c, (110.0 + 140.0 * wave) * event->fade * grow);
    if (r > 0.7) {
      // The big ones unfold a ring of petals.
      for (int petal = 0; petal < 6; petal++) {
        double angle = petal * 1.0472 + event->t * 0.2;
        lineRGBA(surface, top_x, top_y, (int)(top_x + cos(angle) * bulb * 3.2),
                 (int)(top_y + sin(angle) * bulb * 2.0), c[0], c[1], c[2],
                 weapon_alpha((90.0 + 120.0 * wave) * event->fade * grow));
      }
    }
  }
}


// A herd of four-legged animals walks along the ridge, a calf at the back.
static void hill_herd(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int animals = 6;
  double spacing = event->scale * 0.115;
  double margin = event->scale * 0.1;
  double travel = surface->w + 2.0 * margin + animals * spacing;
  double lead = event->progress * travel - margin;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int animal = 0; animal < animals; animal++) {
    double progress_x = lead - animal * spacing - hill_random(event, animal, 1) * spacing * 0.4;
    double x = event->direction > 0 ? progress_x : surface->w - progress_x;
    if (x < -40 || x > surface->w + 40) continue;
    double size = event->scale * (animal == animals - 1 ? 0.03 : 0.052);
    double ground = ground_y(surface, RIDGE_NEAR, x) + 1.0;
    int d = event->direction;
    double gait = event->t * 3.4 + animal * 0.9;
    double bob = fabs(sin(gait)) * size * 0.04;
    double body_y = ground - size * 0.7 - bob;
    // Far legs are a shade lighter than the body, so the legs read as separate.
    for (int leg = 0; leg < 4; leg++) {
      double hip_x = x + (leg < 2 ? 0.38 : -0.38) * size;
      double swing = sin(gait + leg * 1.6) * size * 0.2;
      bool far_leg = leg % 2 == 1;
      thickLineRGBA(surface, (int)hip_x, (int)body_y, (int)(hip_x + swing * d), (int)ground,
                    far_leg ? 2 : 3, far_leg ? 22 : 12, far_leg ? 26 : 15, far_leg ? 36 : 22,
                    alpha);
    }
    filledEllipseRGBA(surface, (int)x, (int)body_y, (int)(size * 0.55), (int)(size * 0.26), 12, 15,
                      22, alpha);
    double neck_x = x + d * size * 0.45, neck_y = body_y - size * 0.1;
    double head_x = x + d * size * 0.78,
           head_y = body_y - size * 0.5 + sin(gait * 0.5) * size * 0.04;
    thickLineRGBA(surface, (int)neck_x, (int)neck_y, (int)head_x, (int)head_y,
                  (int)(size * 0.2) + 1, 12, 15, 22, alpha);
    filledEllipseRGBA(surface, (int)(head_x + d * size * 0.08), (int)head_y, (int)(size * 0.13) + 1,
                      (int)(size * 0.08) + 1, 12, 15, 22, alpha);
    thickLineRGBA(surface, (int)(x - d * size * 0.5), (int)(body_y - size * 0.08),
                  (int)(x - d * size * 0.62), (int)(body_y + size * 0.05), 2, 12, 15, 22, alpha);
    if (animal % 2 == 0 && animal != animals - 1) {
      // Antlers: a main beam with two tines.
      int ax = (int)head_x, ay = (int)(head_y - size * 0.05);
      int tx = (int)(head_x - d * size * 0.1), ty = (int)(head_y - size * 0.5);
      thickLineRGBA(surface, ax, ay, tx, ty, 1, 30, 32, 40, alpha);
      thickLineRGBA(surface, (int)((ax + tx) / 2), (int)((ay + ty) / 2), (int)(tx + d * size * 0.2),
                    (int)(ty + size * 0.1), 1, 30, 32, 40, alpha);
      thickLineRGBA(surface, tx, ty, (int)(tx + d * size * 0.12), (int)(ty - size * 0.14), 1, 30,
                    32, 40, alpha);
    }
  }
}


// A volcano on the middle ridge at one edge of the screen. It grows, its crater
// begins to glow, and it erupts: lava is thrown in arcs, lava runs down the
// cone and an ash cloud rises and drifts.
static void hill_volcano(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = event->direction > 0 ? surface->w * 0.075 : surface->w * 0.925;
  double base = ground_y(surface, RIDGE_NEAR, x) + s * 0.012;
  double rise = smooth_step(0.0, 0.12, event->progress);
  double erupt = smooth_step(0.14, 0.3, event->progress) *
                 (1.0 - smooth_step(0.86, 0.98, event->progress)) *
                 (0.7 + 0.3 * sin(event->t * 0.8));
  double cone_width = s * 0.24, cone_height = s * 0.1 * rise;
  double crater_y = base - cone_height;
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  int dir = event->direction;
  int left = (int)(x - cone_width / 2), right = (int)(x + cone_width / 2);
  int crater_left = (int)(x - cone_width * 0.07), crater_right = (int)(x + cone_width * 0.07);
  // The ash cloud goes behind the cone.
  for (int puff = 0; puff < 14; puff++) {
    double u = fmod(event->t / 7.0 + puff / 14.0, 1.0);
    double px = x + sin(u * 3.0 + puff) * s * 0.02 + dir * u * s * 0.14;
    double py = crater_y - u * surface->h * 0.45;
    double radius = s * (0.018 + 0.055 * u);
    double heat = 1.0 - smooth_step(0.0, 0.35, u);
    filledCircleRGBA(surface, (int)px, (int)py, (int)radius, (Uint8)(44 + 150 * heat),
                     (Uint8)(38 + 50 * heat), (Uint8)(48 + 10 * heat),
                     weapon_alpha(110.0 * (1.0 - u) * erupt * event->fade));
  }
  filledTrigonRGBA(surface, left, (int)base + 4, right, (int)base + 4, crater_left, (int)crater_y,
                   26, 20, 28, alpha);
  filledTrigonRGBA(surface, crater_left, (int)crater_y, crater_right, (int)crater_y, right,
                   (int)base + 4, 26, 20, 28, alpha);
  filledTrigonRGBA(surface, crater_left, (int)crater_y, crater_right, (int)crater_y, (int)x,
                   (int)(crater_y + s * 0.012), 255, 120, 50,
                   weapon_alpha(255.0 * erupt * event->fade));
  filledCircleRGBA(surface, (int)x, (int)crater_y, (int)(s * 0.06 * (0.5 + erupt)), 255, 110, 50,
                   weapon_alpha(34.0 * erupt * event->fade));
  filledCircleRGBA(surface, (int)x, (int)crater_y, (int)(s * 0.032 * (0.5 + erupt)), 255, 140, 60,
                   weapon_alpha(80.0 * erupt * event->fade));
  filledCircleRGBA(surface, (int)x, (int)crater_y, (int)(s * 0.013 * (0.5 + erupt)) + 1, 255, 224,
                   160, weapon_alpha(210.0 * erupt * event->fade));
  // Lava runs down the cone in glowing streams.
  for (int stream = 0; stream < 3; stream++) {
    double side = (stream - 1) * 0.75;
    double length = (0.4 + 0.2 * stream) * erupt;
    double sx = x + side * cone_width * 0.04, sy = crater_y;
    for (int step = 1; step <= 8; step++) {
      double f = step / 8.0 * length;
      double nx = x + side * cone_width * (0.04 + 0.4 * f) + sin(step * 1.3 + stream) * s * 0.004;
      double ny = crater_y + (base - crater_y) * f;
      thickLineRGBA(surface, (int)sx, (int)sy, (int)nx, (int)ny, 2, 255, (Uint8)(130 + 40 * stream),
                    50, weapon_alpha(210.0 * (1.0 - f) * event->fade));
      sx = nx;
      sy = ny;
    }
  }
  // Lava thrown from the crater in arcs.
  for (int blob = 0; blob < 24; blob++) {
    double r = hill_random(event, blob, 1), r2 = hill_random(event, blob, 2);
    double u = fmod(event->t * 0.55 + blob * 0.0417 + r2, 1.0);
    double vx = (r - 0.5) * cone_width * 1.3 + dir * cone_width * 0.1;
    double peak = s * (0.08 + 0.16 * r2);
    double bx = x + vx * u, by = crater_y - peak * 4.0 * u * (1.0 - u);
    int radius = 2 + blob % 3;
    filledCircleRGBA(surface, (int)bx, (int)by, radius + 2, 255, 130, 50,
                     weapon_alpha(50.0 * (1.0 - 0.5 * u) * erupt * event->fade));
    filledCircleRGBA(surface, (int)bx, (int)by, radius, 255, (Uint8)(215 - 110 * u), 80,
                     weapon_alpha(255.0 * (1.0 - 0.5 * u) * erupt * event->fade));
  }
  if (erupt > 0.05) {
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 255, 90, 40,
            weapon_alpha(9.0 * erupt * event->fade));
  }
}


// A chain of signal towers on the ridge lights up one after another, each
// sending a searchlight beam into the sky that sweeps slowly from side to side.
static void hill_beacons(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int towers = 6;
  double s = event->scale;
  for (int i = 0; i < towers; i++) {
    int order = event->direction > 0 ? i : towers - 1 - i;
    double x = surface->w * (0.07 + 0.86 * i / (towers - 1.0));
    double base = ground_y(surface, RIDGE_NEAR, x) + 2.0;
    double height = s * 0.058;
    int top_x = (int)x, top_y = (int)(base - height);
    double lit = smooth_step(0.08 + order * 0.07, 0.12 + order * 0.07, event->progress) *
                 (1.0 - smooth_step(0.88, 0.97, event->progress));
    Uint8 alpha = weapon_alpha(235.0 * event->fade);
    if (lit > 0.0) {
      double aim = -1.5708 + sin(event->t * 0.5 + i * 0.9) * 0.55;
      double half = 0.045;
      double reach = surface->h * 0.95;
      double ax = top_x + cos(aim - half) * reach, ay = top_y + sin(aim - half) * reach;
      double bx = top_x + cos(aim + half) * reach, by = top_y + sin(aim + half) * reach;
      filledTrigonRGBA(surface, top_x, top_y, (int)ax, (int)ay, (int)bx, (int)by, 235, 228, 190,
                       weapon_alpha(24.0 * lit * event->fade));
      filledTrigonRGBA(surface, top_x, top_y, (int)(top_x + cos(aim - half * 0.4) * reach),
                       (int)(top_y + sin(aim - half * 0.4) * reach),
                       (int)(top_x + cos(aim + half * 0.4) * reach),
                       (int)(top_y + sin(aim + half * 0.4) * reach), 255, 250, 220,
                       weapon_alpha(26.0 * lit * event->fade));
    }
    filledTrigonRGBA(surface, (int)(x - s * 0.008), (int)base, (int)(x + s * 0.008), (int)base,
                     top_x, top_y, 20, 24, 36, alpha);
    boxRGBA(surface, top_x - 4, top_y - 3, top_x + 4, top_y, 16, 20, 30, alpha);
    static const Uint8 flame[3] = {255, 170, 80};
    weapon_glow(surface, top_x, top_y - 5, s * 0.009 * (0.9 + 0.1 * sin(event->t * 12.0 + i)),
                flame, 235.0 * lit * event->fade);
  }
}


// Fireflies drift over the ridge and blink on and off at their own pace.
static void hill_fireflies(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int flies = 70;
  static const Uint8 glow[3] = {200, 255, 120};
  double s = event->scale;
  double build =
      smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.88, 1.0, event->progress));
  for (int i = 0; i < flies; i++) {
    double r1 = hill_random(event, i, 1), r2 = hill_random(event, i, 2),
           r3 = hill_random(event, i, 3);
    double home_x = surface->w * (0.02 + 0.96 * r1);
    double home_y = ground_y(surface, RIDGE_NEAR, home_x) - 6.0 - r2 * surface->h * 0.2;
    double x = home_x + sin(event->t * (0.4 + r3 * 0.5) + i * 1.7) * s * 0.03;
    double y = home_y + cos(event->t * (0.5 + r2 * 0.4) + i) * s * 0.02;
    double blink = 0.5 + 0.5 * sin(event->t * (1.2 + r3 * 1.5) + i * 2.3);
    blink = blink * blink * blink;
    if (blink < 0.03) continue;
    weapon_glow(surface, x, y, 1.0 + s * 0.0025, glow, 235.0 * blink * build * event->fade);
  }
}


// Kites on strings: small figures stand on the ridge and let kites climb, sway
// and trail ribbons.
static void hill_kites(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int kites = 4;
  double s = event->scale;
  int d = event->direction;
  static const Uint8 colors[4][3] = {
      {230, 80, 70}, {245, 190, 70}, {80, 170, 230}, {150, 220, 110}};
  static const double shape_x[4] = {0.0, 0.8, 0.0, -0.8};
  static const double shape_y[4] = {-1.2, 0.0, 1.6, 0.0};
  for (int i = 0; i < kites; i++) {
    double anchor_x = surface->w * (0.12 + 0.76 * (i + 0.5) / kites);
    double ground = ground_y(surface, RIDGE_NEAR, anchor_x) + 1.0;
    double height = s * 0.034;
    hill_walker(surface, anchor_x, ground, height, d, 0.0, false, event->fade, event->t, i);
    double rise = smooth_step(0.04 + i * 0.04, 0.2 + i * 0.04, event->progress);
    double hand_x = anchor_x + d * height * 0.2, hand_y = ground - height * 0.58;
    double target_x =
        anchor_x + d * s * (0.05 + 0.03 * (i % 2)) + sin(event->t * 0.7 + i * 2.0) * s * 0.03;
    double target_y =
        ground - surface->h * (0.26 + 0.08 * (i % 3)) + cos(event->t * 0.9 + i) * s * 0.015;
    double kx = hand_x + (target_x - hand_x) * rise, ky = hand_y + (target_y - hand_y) * rise;
    Uint8 line_alpha = weapon_alpha(170.0 * event->fade);
    double last_x = hand_x, last_y = hand_y;
    for (int k = 1; k <= 10; k++) {
      double u = k / 10.0;
      double lx = hand_x + (kx - hand_x) * u;
      double ly = hand_y + (ky - hand_y) * u + sin(3.14159265 * u) * s * 0.012 * rise;
      lineRGBA(surface, (int)last_x, (int)last_y, (int)lx, (int)ly, 210, 210, 200, line_alpha);
      last_x = lx;
      last_y = ly;
    }
    double ksz = s * 0.02;
    double tilt = sin(event->t * 1.3 + i) * 0.3;
    double ca = cos(tilt), sa = sin(tilt);
    int px[4], py[4];
    for (int p = 0; p < 4; p++) {
      px[p] = (int)(kx + (shape_x[p] * ca - shape_y[p] * sa) * ksz);
      py[p] = (int)(ky + (shape_x[p] * sa + shape_y[p] * ca) * ksz);
    }
    const Uint8 *c = colors[i % 4];
    Uint8 alpha = weapon_alpha(235.0 * event->fade * rise);
    filledTrigonRGBA(surface, px[0], py[0], px[3], py[3], px[2], py[2], (Uint8)(c[0] * 0.7),
                     (Uint8)(c[1] * 0.7), (Uint8)(c[2] * 0.7), alpha);
    filledTrigonRGBA(surface, px[0], py[0], px[1], py[1], px[2], py[2], c[0], c[1], c[2], alpha);
    lineRGBA(surface, px[0], py[0], px[2], py[2], 60, 50, 50, alpha);
    lineRGBA(surface, px[1], py[1], px[3], py[3], 60, 50, 50, alpha);
    double tail_x = px[2], tail_y = py[2];
    for (int k = 1; k <= 7; k++) {
      double bx = px[2] + sin(event->t * 3.0 + k * 0.9 + i) * ksz * 0.35 * (k / 7.0 + 0.3);
      double by = py[2] + k * ksz * 0.55;
      lineRGBA(surface, (int)tail_x, (int)tail_y, (int)bx, (int)by, 210, 210, 200, line_alpha);
      const Uint8 *bow = colors[(i + k) % 4];
      filledCircleRGBA(surface, (int)bx, (int)by, k % 2 ? 2 : 1, bow[0], bow[1], bow[2], alpha);
      tail_x = bx;
      tail_y = by;
    }
  }
}


// Fireworks: shells climb from the ridge, burst into rings of sparks that fall
// and fade.
static void hill_fireworks(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int shells = 6;
  static const Uint8 palette[5][3] = {
      {255, 90, 80}, {255, 205, 90}, {110, 200, 255}, {170, 255, 140}, {230, 130, 255}};
  double s = event->scale;
  for (int b = 0; b < shells; b++) {
    double phase = event->t / 4.6 + b * 0.37;
    int cycle = (int)floor(phase);
    double u = phase - cycle;
    unsigned int h = fx_hash(event->seed + b * 131u, cycle);
    double bx = surface->w * (0.1 + 0.8 * (h & 255) / 255.0);
    double top = surface->h * (0.14 + 0.28 * ((h >> 8) & 255) / 255.0);
    double ground = ground_y(surface, RIDGE_NEAR, bx);
    const Uint8 *c = palette[(h >> 16) % 5];
    const double launch = 0.22;
    if (u < launch) {
      double k = u / launch;
      for (int j = 0; j < 6; j++) {
        double kk = k - j * 0.05;
        if (kk < 0.0) break;
        double yy = ground + (top - ground) * (1.0 - (1.0 - kk) * (1.0 - kk));
        filledCircleRGBA(surface, (int)(bx + sin(kk * 30.0) * 1.5), (int)yy, j == 0 ? 2 : 1, 255,
                         220, 150, weapon_alpha(230.0 * (1.0 - j / 6.0) * event->fade));
      }
      continue;
    }
    double age = (u - launch) / (1.0 - launch);
    double radius = s * (0.1 + 0.07 * ((h >> 24) & 255) / 255.0);
    if (age < 0.1) {
      static const Uint8 white[3] = {255, 250, 235};
      weapon_glow(surface, bx, top, s * 0.03 * (1.0 - age * 8.0), white, 235.0 * event->fade);
    }
    double ease = 1.0 - pow(1.0 - age, 3.0);
    double ease_back = 1.0 - pow(1.0 - age * 0.93, 3.0);
    double fall = s * 0.12 * age * age;
    double fade = pow(1.0 - age, 1.5) * event->fade;
    const int sparks = 40;
    for (int j = 0; j < sparks; j++) {
      if (age > 0.5 && ((int)(event->t * 20.0) + j) % 3 == 0) continue;
      double a = j * 6.2831853 / sparks + (h & 7);
      double speed = j % 2 ? 0.7 : 1.0;
      int sx = (int)(bx + cos(a) * radius * speed * ease);
      int sy = (int)(top + sin(a) * radius * speed * ease + fall);
      filledCircleRGBA(surface, sx, sy, age < 0.5 ? 2 : 1, c[0], c[1], c[2],
                       weapon_alpha(255.0 * fade));
      filledCircleRGBA(surface, (int)(bx + cos(a) * radius * speed * ease_back),
                       (int)(top + sin(a) * radius * speed * ease_back + fall * 0.8), 1, c[0], c[1],
                       c[2], weapon_alpha(110.0 * fade));
    }
  }
}


// A train with lit windows follows the ridge, its engine puffing smoke and
// shining a headlight ahead.
static void hill_train(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int cars = 6;
  double s = event->scale;
  int d = event->direction;
  double car_length = s * 0.07, gap = s * 0.008, margin = s * 0.1;
  double travel = surface->w + 2.0 * margin + (cars + 1) * (car_length + gap);
  double lead = event->progress * travel - margin;
  double wheel = s * 0.005;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int car = 0; car <= cars; car++) {
    bool engine = car == 0;
    double front_px = lead - car * (car_length + gap);
    double rear_px = front_px - car_length * (engine ? 0.85 : 1.0);
    double fx = d > 0 ? front_px : surface->w - front_px;
    double rx = d > 0 ? rear_px : surface->w - rear_px;
    if ((fx < -30 && rx < -30) || (fx > surface->w + 30 && rx > surface->w + 30)) continue;
    double fy = ground_y(surface, RIDGE_NEAR, fx) + 1.0;
    double ry = ground_y(surface, RIDGE_NEAR, rx) + 1.0;
    double height = s * (engine ? 0.034 : 0.028);
    int ax = (int)rx, ay = (int)(ry - wheel);
    int bx = (int)fx, by = (int)(fy - wheel);
    int cx = bx, cy = (int)(fy - wheel - height);
    int dx = ax, dy = (int)(ry - wheel - height);
    Uint8 r = engine ? 20 : 26, g = engine ? 24 : 32, b = engine ? 34 : 46;
    filledTrigonRGBA(surface, ax, ay, bx, by, cx, cy, r, g, b, alpha);
    filledTrigonRGBA(surface, ax, ay, cx, cy, dx, dy, r, g, b, alpha);
    int windows = engine ? 1 : 4;
    for (int k = 0; k < windows; k++) {
      double u = engine ? 0.3 : (k + 0.5) / 4.0;
      int wx = (int)(rx + (fx - rx) * u);
      int wy = (int)(ry + (fy - ry) * u - wheel - height * 0.6);
      double flicker = 0.9 + 0.1 * sin(event->t * 5.0 + car * 2.0 + k);
      boxRGBA(surface, wx - 2, wy - 2, wx + 2, wy + 1, 255, 214, 130,
              weapon_alpha(240.0 * flicker * event->fade));
    }
    for (int k = 0; k < 3; k++) {
      double u = 0.15 + 0.35 * k;
      filledCircleRGBA(surface, (int)(rx + (fx - rx) * u), (int)(ry + (fy - ry) * u - wheel * 0.6),
                       (int)(wheel * 0.8) + 1, 8, 10, 16, alpha);
    }
    if (engine) {
      double chx = rx + (fx - rx) * 0.75, chy = ry + (fy - ry) * 0.75 - wheel - height;
      boxRGBA(surface, (int)chx - 2, (int)(chy - height * 0.4), (int)chx + 2, (int)chy, 14, 18, 26,
              alpha);
      for (int puff = 0; puff < 8; puff++) {
        double age = fmod(event->t / 1.6 + puff / 8.0, 1.0);
        filledCircleRGBA(surface, (int)(chx - d * age * s * 0.12 + sin(age * 6.0 + puff) * 2.0),
                         (int)(chy - height * 0.4 - age * s * 0.07), (int)(2.0 + age * 5.0), 150,
                         152, 162, weapon_alpha(105.0 * (1.0 - age) * event->fade));
      }
      static const Uint8 lamp[3] = {255, 235, 170};
      double lx = fx + d * 3.0, ly = fy - wheel - height * 0.5;
      filledTrigonRGBA(surface, (int)lx, (int)ly, (int)(lx + d * s * 0.16), (int)(ly - s * 0.02),
                       (int)(lx + d * s * 0.16), (int)(ly + s * 0.025), 255, 235, 170,
                       weapon_alpha(26.0 * event->fade));
      weapon_glow(surface, lx, ly, s * 0.006, lamp, 235.0 * event->fade);
    }
  }
}


// A rocket on a gantry vents steam, ignites and climbs out of the top of the
// screen on a column of smoke.
static void hill_rocket(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double x = surface->w * (d > 0 ? 0.2 : 0.8);
  double ground = ground_y(surface, RIDGE_NEAR, x) + 2.0;
  double rocket_h = s * 0.075, rocket_w = s * 0.011;
  const double liftoff = 0.34;
  double flight = (event->progress - liftoff) / 0.4;
  if (flight < 0.0) flight = 0.0;
  if (flight > 1.0) flight = 1.0;
  double climb = surface->h + rocket_h * 2.0;
  double alt = flight * flight * climb;
  double drift = flight * flight * s * 0.05 * d;
  double ignite = smooth_step(liftoff - 0.08, liftoff, event->progress) *
                  (1.0 - smooth_step(liftoff + 0.2, liftoff + 0.34, event->progress));
  double dissipate = 1.0 - smooth_step(0.74, 0.95, event->progress);
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  // The gantry and pad, with a blinking light on top.
  double tower_x = x - d * rocket_w * 3.0;
  boxRGBA(surface, (int)(tower_x - 2), (int)(ground - rocket_h * 0.95), (int)(tower_x + 2),
          (int)ground, 18, 22, 32, alpha);
  for (int arm = 0; arm < 3; arm++) {
    double ay = ground - rocket_h * (0.3 + 0.25 * arm);
    thickLineRGBA(surface, (int)tower_x, (int)ay, (int)(x - d * rocket_w * 0.6), (int)ay, 1, 18, 22,
                  32, event->progress < liftoff ? alpha : (Uint8)0);
  }
  boxRGBA(surface, (int)(x - rocket_w * 2.5), (int)(ground - 2), (int)(x + rocket_w * 2.5),
          (int)(ground + 1), 30, 34, 46, alpha);
  if (fmod(event->t, 1.2) < 0.4) {
    static const Uint8 red[3] = {255, 60, 50};
    weapon_glow(surface, tower_x, ground - rocket_h * 0.95 - 3.0, 2.5, red, 235.0 * event->fade);
  }
  // Steam and fire flare at the pad.
  filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.07), (int)(s * 0.012), 255, 150, 60,
                    weapon_alpha(50.0 * ignite * event->fade));
  for (int puff = 0; puff < 12; puff++) {
    double u = fmod(event->t / 2.5 + puff / 12.0, 1.0);
    double side = (puff % 2 ? 1.0 : -1.0) * u * s * 0.1;
    filledCircleRGBA(surface, (int)(x + side), (int)(ground - u * s * 0.015),
                     (int)(s * (0.008 + 0.02 * u)), 225, 230, 240,
                     weapon_alpha(90.0 * (1.0 - u) * ignite * event->fade));
  }
  // The smoke column hangs where the rocket has been.
  const int trail = 24;
  for (int k = 0; k < trail && flight > 0.0; k++) {
    double past = flight - k * 0.018;
    if (past <= 0.0) break;
    double px = x + past * past * s * 0.05 * d + sin(k * 1.7) * 3.0;
    double py = ground - past * past * climb;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(s * (0.008 + 0.018 * k / trail)), 190, 190,
                     200,
                     weapon_alpha(120.0 * (1.0 - (double)k / trail) * dissipate * event->fade));
  }
  if (flight >= 1.0) return;
  // The rocket.
  double rx = x + drift, base = ground - alt - 2.0;
  int top_y = (int)(base - rocket_h * 0.78);
  boxRGBA(surface, (int)(rx - rocket_w / 2), top_y, (int)(rx + rocket_w / 2), (int)base, 205, 210,
          220, alpha);
  boxRGBA(surface, (int)(rx - rocket_w / 2), (int)(base - rocket_h * 0.5), (int)(rx + rocket_w / 2),
          (int)(base - rocket_h * 0.42), 200, 60, 55, alpha);
  filledTrigonRGBA(surface, (int)(rx - rocket_w / 2), top_y, (int)(rx + rocket_w / 2), top_y,
                   (int)rx, (int)(base - rocket_h), 200, 60, 55, alpha);
  filledTrigonRGBA(surface, (int)(rx - rocket_w / 2), (int)base, (int)(rx - rocket_w * 1.1),
                   (int)(base + 2), (int)(rx - rocket_w / 2), (int)(base - rocket_h * 0.28), 150,
                   45, 42, alpha);
  filledTrigonRGBA(surface, (int)(rx + rocket_w / 2), (int)base, (int)(rx + rocket_w * 1.1),
                   (int)(base + 2), (int)(rx + rocket_w / 2), (int)(base - rocket_h * 0.28), 150,
                   45, 42, alpha);
  filledCircleRGBA(surface, (int)rx, (int)(base - rocket_h * 0.62), (int)(rocket_w * 0.22) + 1, 90,
                   160, 210, alpha);
  double flame = smooth_step(liftoff - 0.05, liftoff + 0.02, event->progress);
  if (flame > 0.0) {
    double flicker = 0.75 + 0.25 * sin(event->t * 40.0);
    double length = rocket_h * (0.25 + 0.5 * flight) * flame * flicker;
    filledTrigonRGBA(surface, (int)(rx - rocket_w * 0.45), (int)base, (int)(rx + rocket_w * 0.45),
                     (int)base, (int)rx, (int)(base + length), 255, 140, 40,
                     weapon_alpha(225.0 * event->fade));
    filledTrigonRGBA(surface, (int)(rx - rocket_w * 0.25), (int)base, (int)(rx + rocket_w * 0.25),
                     (int)base, (int)rx, (int)(base + length * 0.65), 255, 225, 140,
                     weapon_alpha(235.0 * event->fade));
    static const Uint8 hot[3] = {255, 170, 80};
    weapon_glow(surface, rx, base + 2.0, rocket_w * 1.4 * flame, hot, 200.0 * event->fade);
  }
}


// A flying saucer glides in, hovers and beams up a cow, then leaves.
static void hill_abduction(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double target_x = surface->w * (0.3 + 0.4 * hill_random(event, 1, 1));
  double ground = ground_y(surface, RIDGE_NEAR, target_x) + 2.0;
  double arrive = smooth_step(0.0, 0.16, event->progress);
  double leave = smooth_step(0.84, 1.0, event->progress);
  double saucer_x =
      target_x - d * surface->w * 0.6 * (1.0 - arrive) + d * surface->w * 0.7 * leave * leave;
  double saucer_y = surface->h * 0.3 + sin(event->t * 1.1) * s * 0.006 - leave * surface->h * 0.2;
  double a = s * 0.05, b = s * 0.012;
  double beam =
      smooth_step(0.2, 0.26, event->progress) * (1.0 - smooth_step(0.74, 0.8, event->progress));
  double bottom = saucer_y + b;
  double pulse = 0.8 + 0.2 * sin(event->t * 9.0);
  if (beam > 0.0) {
    double near_w = s * 0.012, far_w = s * 0.055;
    filledTrigonRGBA(surface, (int)(saucer_x - near_w), (int)bottom, (int)(saucer_x + near_w),
                     (int)bottom, (int)(saucer_x - far_w), (int)ground, 170, 255, 190,
                     weapon_alpha(34.0 * beam * pulse * event->fade));
    filledTrigonRGBA(surface, (int)(saucer_x + near_w), (int)bottom, (int)(saucer_x + far_w),
                     (int)ground, (int)(saucer_x - far_w), (int)ground, 170, 255, 190,
                     weapon_alpha(34.0 * beam * pulse * event->fade));
    for (int k = 0; k < 5; k++) {
      double u = fmod(event->t * 0.6 + k * 0.2, 1.0);
      double ring_w = far_w + (near_w - far_w) * u;
      ellipseRGBA(surface, (int)saucer_x, (int)(ground + (bottom - ground) * u), (int)ring_w,
                  (int)(ring_w * 0.15) + 1, 200, 255, 210,
                  weapon_alpha(110.0 * beam * (1.0 - u) * event->fade));
    }
  }
  // The cow stands on the ground, then rises in the beam and shrinks.
  double lift = smooth_step(0.32, 0.72, event->progress);
  if (lift < 0.97) {
    double size = s * 0.04 * (1.0 - 0.5 * lift);
    double cow_x = target_x + sin(event->t * 2.0) * s * 0.004 * lift;
    double cow_y = ground - size * 0.7 - lift * (ground - bottom - size);
    double shade = lift * lift;
    Uint8 cr = (Uint8)(14 + 150 * shade), cg = (Uint8)(18 + 190 * shade),
          cb = (Uint8)(26 + 150 * shade);
    Uint8 alpha = weapon_alpha(238.0 * event->fade);
    for (int leg = 0; leg < 4; leg++) {
      double hip_x = cow_x + (leg < 2 ? 0.3 : -0.3) * size;
      double sway = sin(event->t * 3.0 + leg) * size * 0.12 * lift;
      thickLineRGBA(surface, (int)hip_x, (int)cow_y, (int)(hip_x + sway), (int)(cow_y + size * 0.6),
                    2, cr, cg, cb, alpha);
    }
    filledEllipseRGBA(surface, (int)cow_x, (int)cow_y, (int)(size * 0.5), (int)(size * 0.25), cr,
                      cg, cb, alpha);
    filledEllipseRGBA(surface, (int)(cow_x + d * size * 0.58), (int)(cow_y - size * 0.1),
                      (int)(size * 0.15), (int)(size * 0.11), cr, cg, cb, alpha);
  }
  // The saucer.
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  filledEllipseRGBA(surface, (int)saucer_x, (int)(saucer_y - b * 0.7), (int)(a * 0.4),
                    (int)(b * 1.5), 150, 215, 230, weapon_alpha(210.0 * event->fade));
  filledEllipseRGBA(surface, (int)saucer_x, (int)saucer_y, (int)a, (int)b, 120, 132, 150, alpha);
  filledEllipseRGBA(surface, (int)saucer_x, (int)(saucer_y + b * 0.4), (int)(a * 0.7),
                    (int)(b * 0.5), 84, 94, 112, alpha);
  static const Uint8 light[3] = {255, 240, 160};
  for (int j = 0; j < 7; j++) {
    double angle = 0.4 + j * (3.14159265 - 0.8) / 6.0;
    if (((int)(event->t * 6.0) + j) % 3 == 0) {
      weapon_glow(surface, saucer_x + cos(angle) * a * 0.85 * (d > 0 ? 1 : -1),
                  saucer_y + sin(angle) * b * 0.55, 2.0, light, 235.0 * event->fade);
    }
  }
}


// Geysers erupt one after another: a water column, arcing drops and steam.
static void hill_geysers(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int geysers = 4;
  double s = event->scale;
  for (int i = 0; i < geysers; i++) {
    double x = surface->w * (0.14 + 0.72 * (i + 0.5 * hill_random(event, i, 1)) / geysers);
    double ground = ground_y(surface, RIDGE_NEAR, x) + 2.0;
    double period = 6.0 + 2.0 * hill_random(event, i, 2);
    double local = fmod(event->t / period + hill_random(event, i, 3), 1.0);
    double erupt = smooth_step(0.0, 0.04, local) * (1.0 - smooth_step(0.22, 0.3, local));
    double height = s * (0.1 + 0.07 * hill_random(event, i, 4)) * erupt;
    filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.03), (int)(s * 0.006), 70, 60, 64,
                      weapon_alpha(235.0 * event->fade));
    filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.018), (int)(s * 0.003), 110, 190,
                      210, weapon_alpha(220.0 * event->fade));
    if (erupt > 0.02) {
      double wobble = sin(event->t * 14.0 + i) * s * 0.002;
      thickLineRGBA(surface, (int)x, (int)ground, (int)(x + wobble), (int)(ground - height * 0.85),
                    3, 200, 235, 250, weapon_alpha(170.0 * erupt * event->fade));
      for (int j = 0; j < 22; j++) {
        double r = hill_random(event, i * 40 + j, 5), r2 = hill_random(event, i * 40 + j, 6);
        double u = fmod(event->t * 1.3 + j / 22.0, 1.0);
        double dx = (r - 0.5) * s * 0.07 * u;
        double dy = height * 4.0 * u * (1.0 - u) * (0.6 + 0.4 * r2);
        filledCircleRGBA(surface, (int)(x + dx), (int)(ground - dy), 1 + j % 2, 200, 235, 250,
                         weapon_alpha(230.0 * erupt * event->fade));
      }
    }
    for (int puff = 0; puff < 8; puff++) {
      double u = fmod(event->t / 3.5 + puff / 8.0 + i * 0.2, 1.0);
      double px = x + event->direction * u * s * 0.05 + sin(u * 5.0 + puff) * s * 0.01;
      double py = ground - height * 0.5 - u * s * 0.12;
      filledCircleRGBA(surface, (int)px, (int)py, (int)(s * (0.008 + 0.02 * u)), 225, 230, 240,
                       weapon_alpha(80.0 * (1.0 - u) * (0.3 + 0.7 * erupt) * event->fade));
    }
  }
}


// A circle of standing stones: runes wake up on them one by one, then a pillar
// of light rises from the middle with motes spiralling up it.
static void hill_stones(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int stones = 7;
  double s = event->scale;
  double span = s * 0.5;
  double center_x = surface->w * 0.5;
  double center_ground = ground_y(surface, RIDGE_NEAR, center_x) + 2.0;
  static const Uint8 rune[3] = {110, 230, 255};
  double pillar =
      smooth_step(0.5, 0.62, event->progress) * (1.0 - smooth_step(0.84, 0.94, event->progress));
  if (pillar > 0.0) {
    double width = s * 0.03 * (0.8 + 0.2 * sin(event->t * 3.0));
    boxRGBA(surface, (int)(center_x - width), 0, (int)(center_x + width), (int)center_ground,
            rune[0], rune[1], rune[2], weapon_alpha(24.0 * pillar * event->fade));
    boxRGBA(surface, (int)(center_x - width * 0.4), 0, (int)(center_x + width * 0.4),
            (int)center_ground, 220, 250, 255, weapon_alpha(46.0 * pillar * event->fade));
    for (int j = 0; j < 18; j++) {
      double u = fmod(event->t * 0.25 + j / 18.0, 1.0);
      double angle = u * 12.0 + j;
      filledCircleRGBA(surface, (int)(center_x + cos(angle) * width * 0.9),
                       (int)(center_ground - u * surface->h * 0.8), 2, rune[0], rune[1], rune[2],
                       weapon_alpha(220.0 * pillar * (1.0 - u * 0.5) * event->fade));
    }
  }
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int i = 0; i < stones; i++) {
    double x = center_x - span / 2.0 + span * i / (stones - 1.0);
    double ground = ground_y(surface, RIDGE_NEAR, x) + 2.0;
    double height = s * (0.045 + 0.015 * hill_random(event, i, 1));
    double width = height * 0.35;
    int order = event->direction > 0 ? i : stones - 1 - i;
    filledTrigonRGBA(surface, (int)(x - width / 2), (int)ground, (int)(x + width / 2), (int)ground,
                     (int)(x + width * 0.4), (int)(ground - height), 30, 34, 46, alpha);
    filledTrigonRGBA(surface, (int)(x - width / 2), (int)ground, (int)(x + width * 0.4),
                     (int)(ground - height), (int)(x - width * 0.4), (int)(ground - height), 30, 34,
                     46, alpha);
    double wake = smooth_step(0.1 + order * 0.06, 0.16 + order * 0.06, event->progress) *
                  (1.0 - smooth_step(0.9, 0.98, event->progress));
    if (wake <= 0.0) continue;
    double glow = wake * (0.8 + 0.2 * sin(event->t * 4.0 + i));
    double mid = ground - height * 0.55;
    lineRGBA(surface, (int)(x - width * 0.2), (int)(mid - height * 0.15), (int)(x + width * 0.2),
             (int)(mid - height * 0.15), rune[0], rune[1], rune[2],
             weapon_alpha(235.0 * glow * event->fade));
    lineRGBA(surface, (int)x, (int)(mid - height * 0.15), (int)x, (int)(mid + height * 0.15),
             rune[0], rune[1], rune[2], weapon_alpha(235.0 * glow * event->fade));
    circleRGBA(surface, (int)x, (int)(mid + height * 0.05), (int)(width * 0.18) + 1, rune[0],
               rune[1], rune[2], weapon_alpha(235.0 * glow * event->fade));
    weapon_glow(surface, x, mid, height * 0.12, rune, 150.0 * glow * event->fade);
    if (pillar > 0.0) {
      lineRGBA(surface, (int)x, (int)(ground - height), (int)center_x, (int)(center_ground - 2.0),
               rune[0], rune[1], rune[2], weapon_alpha(90.0 * pillar * event->fade));
    }
  }
}


#define SCENE_PI 3.14159265359
#define SCENE_TAU 6.28318530718

// Screen row of the near ridge under pixel column x, for things standing on it.
static double hill_ground(const HillEvent *event, double x) {
  return ground_y(event->surface, RIDGE_NEAR, x) + 2.0;
}


// A column near one side of the screen (so it is not hidden by the cards):
// `fraction` of the width from the side things come from.
static double hill_side(const HillEvent *event, double fraction) {
  return event->surface->w * (event->direction > 0 ? fraction : 1.0 - fraction);
}


// A lit Ferris wheel rises on the ridge and turns, its gondolas and rim lights
// blinking.
static void hill_ferris(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = hill_side(event, 0.1);
  double ground = hill_ground(event, cx);
  double radius = s * 0.12 * smooth_step(0.0, 0.12, event->progress);
  if (radius < 3.0) return;
  double hub_y = ground - radius - s * 0.012;
  double lit =
      smooth_step(0.1, 0.2, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  Uint8 alpha = weapon_alpha(235.0 * event->fade);
  static const Uint8 colors[4][3] = {
      {255, 120, 110}, {120, 220, 255}, {255, 220, 110}, {160, 255, 150}};
  thickLineRGBA(surface, (int)cx, (int)hub_y, (int)(cx - radius * 0.6), (int)ground, 3, 18, 22, 32,
                alpha);
  thickLineRGBA(surface, (int)cx, (int)hub_y, (int)(cx + radius * 0.6), (int)ground, 3, 18, 22, 32,
                alpha);
  const int spokes = 12;
  double spin = event->t * 0.22 * event->direction;
  int gondola = (int)(s * 0.007) + 2;
  for (int k = 0; k < spokes; k++) {
    double angle = spin + k * SCENE_TAU / spokes;
    double tip_x = cx + cos(angle) * radius, tip_y = hub_y + sin(angle) * radius;
    double next_x = cx + cos(angle + SCENE_TAU / spokes) * radius;
    double next_y = hub_y + sin(angle + SCENE_TAU / spokes) * radius;
    thickLineRGBA(surface, (int)cx, (int)hub_y, (int)tip_x, (int)tip_y, 1, 40, 46, 64, alpha);
    thickLineRGBA(surface, (int)tip_x, (int)tip_y, (int)next_x, (int)next_y, 2, 34, 40, 58, alpha);
    boxRGBA(surface, (int)tip_x - gondola, (int)tip_y, (int)tip_x + gondola,
            (int)tip_y + gondola * 2, 26, 30, 44, alpha);
    boxRGBA(surface, (int)tip_x - 1, (int)tip_y + 1, (int)tip_x + 1, (int)tip_y + gondola + 1, 255,
            214, 130, weapon_alpha(230.0 * lit * event->fade));
    double blink = 0.55 + 0.45 * sin(event->t * 5.0 + k * 1.7);
    weapon_glow(surface, tip_x, tip_y - 1.0, 2.2, colors[k % 4], 230.0 * lit * blink * event->fade);
  }
  filledCircleRGBA(surface, (int)cx, (int)hub_y, 4, 44, 50, 70, alpha);
  boxRGBA(surface, (int)(cx - radius * 0.7), (int)(ground - 2), (int)(cx + radius * 0.7),
          (int)(ground + 2), 22, 26, 38, alpha);
}


// A small observatory: the dome slit opens and a laser guide star beam climbs
// into the sky.
static void hill_observatory(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = hill_side(event, 0.13);
  double ground = hill_ground(event, cx);
  double r = s * 0.05, wall = s * 0.035;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double open =
      smooth_step(0.06, 0.14, event->progress) * (1.0 - smooth_step(0.9, 0.96, event->progress));
  double dome_y = ground - wall;
  boxRGBA(surface, (int)(cx - r), (int)dome_y, (int)(cx + r), (int)ground, 22, 26, 38, alpha);
  for (int k = 0; k < 3; k++) {
    int wx = (int)(cx - r * 0.6 + k * r * 0.6);
    boxRGBA(surface, wx - 1, (int)(dome_y + wall * 0.3), wx + 1, (int)(dome_y + wall * 0.3) + 5,
            255, 214, 130, weapon_alpha(220.0 * open * event->fade));
  }
  filledPieRGBA(surface, (int)cx, (int)dome_y, (int)r, 180, 360, 34, 40, 58, alpha);
  double top = dome_y - r;
  double slit = r * 0.24 * open;
  if (slit > 0.5) {
    boxRGBA(surface, (int)(cx - slit), (int)top, (int)(cx + slit), (int)(top + r * 0.55), 8, 10, 16,
            alpha);
  }
  if (open <= 0.3) return;
  static const Uint8 green[3] = {120, 255, 170};
  double tilt = event->direction * (0.12 + 0.18 * sin(event->t * 0.25));
  double length = s * 1.3;
  double ex = cx + sin(tilt) * length, ey = top - cos(tilt) * length;
  double flicker = (0.85 + 0.15 * sin(event->t * 13.0)) * open * event->fade;
  thickLineRGBA(surface, (int)cx, (int)top, (int)ex, (int)ey, 7, green[0], green[1], green[2],
                weapon_alpha(16.0 * flicker));
  thickLineRGBA(surface, (int)cx, (int)top, (int)ex, (int)ey, 3, green[0], green[1], green[2],
                weapon_alpha(50.0 * flicker));
  thickLineRGBA(surface, (int)cx, (int)top, (int)ex, (int)ey, 1, 220, 255, 235,
                weapon_alpha(200.0 * flicker));
  weapon_glow(surface, cx, top, 4.0, green, 200.0 * open * event->fade);
  double star_x = cx + sin(tilt) * s * 0.7, star_y = top - cos(tilt) * s * 0.7;
  double twinkle = 3.0 + 1.5 * sin(event->t * 3.0);
  weapon_glow(surface, star_x, star_y, twinkle, green, 220.0 * open * event->fade);
  lineRGBA(surface, (int)(star_x - twinkle * 3.0), (int)star_y, (int)(star_x + twinkle * 3.0),
           (int)star_y, 220, 255, 235, weapon_alpha(150.0 * open * event->fade));
  lineRGBA(surface, (int)star_x, (int)(star_y - twinkle * 3.0), (int)star_x,
           (int)(star_y + twinkle * 3.0), 220, 255, 235, weapon_alpha(150.0 * open * event->fade));
}


// Old windmills grow on the ridge and their lattice sails start to turn.
static void hill_windmills(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int mills = 3;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double lit =
      smooth_step(0.15, 0.25, event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
  for (int i = 0; i < mills; i++) {
    double x = surface->w * (0.08 + 0.84 * i / (mills - 1.0));
    double rise = smooth_step(0.02 + i * 0.05, 0.12 + i * 0.05, event->progress);
    if (rise <= 0.0) continue;
    double base = hill_ground(event, x);
    double height = s * (0.09 + 0.015 * (i % 2)) * rise;
    double wide = height * 0.2, narrow = height * 0.11;
    double top = base - height;
    filledTrigonRGBA(surface, (int)(x - wide), (int)base, (int)(x + wide), (int)base,
                     (int)(x - narrow), (int)top, 24, 28, 40, alpha);
    filledTrigonRGBA(surface, (int)(x + wide), (int)base, (int)(x + narrow), (int)top,
                     (int)(x - narrow), (int)top, 24, 28, 40, alpha);
    filledTrigonRGBA(surface, (int)(x - narrow - 2), (int)top, (int)(x + narrow + 2), (int)top,
                     (int)x, (int)(top - height * 0.18), 14, 16, 26, alpha);
    filledCircleRGBA(surface, (int)x, (int)(base - height * 0.22), 6, 255, 190, 100,
                     weapon_alpha(30.0 * lit * event->fade));
    boxRGBA(surface, (int)x - 2, (int)(base - height * 0.28), (int)x + 2,
            (int)(base - height * 0.16), 255, 214, 130, weapon_alpha(235.0 * lit * event->fade));
    double hub_x = x, hub_y = top - height * 0.02;
    double spin = event->t * (0.45 + 0.1 * i) * rise + i * 1.1;
    double arm = height * 0.75;
    for (int k = 0; k < 4; k++) {
      double a = spin + k * SCENE_PI / 2.0;
      double ux = cos(a), uy = sin(a);
      double tip_x = hub_x + ux * arm, tip_y = hub_y + uy * arm;
      double px = -uy, py = ux, wd = arm * 0.2;
      double ax = hub_x + ux * arm * 0.3, ay = hub_y + uy * arm * 0.3;
      filledTrigonRGBA(surface, (int)ax, (int)ay, (int)tip_x, (int)tip_y, (int)(tip_x + px * wd),
                       (int)(tip_y + py * wd), 62, 68, 90, weapon_alpha(190.0 * event->fade));
      filledTrigonRGBA(surface, (int)ax, (int)ay, (int)(ax + px * wd), (int)(ay + py * wd),
                       (int)(tip_x + px * wd), (int)(tip_y + py * wd), 62, 68, 90,
                       weapon_alpha(190.0 * event->fade));
      thickLineRGBA(surface, (int)hub_x, (int)hub_y, (int)tip_x, (int)tip_y, 2, 20, 24, 36, alpha);
    }
    filledCircleRGBA(surface, (int)hub_x, (int)hub_y, 3, 44, 50, 70, alpha);
  }
}


typedef struct {
  const HillEvent *event;
  double grow;  // 0..1 how much of the tree exists
  double lit;   // 0..1 how bright its lights are
  int max_depth;
  int salt;
} HillTree;

static void hill_tree_branch(const HillTree *tree, double x, double y, double angle, double length,
                             int depth, int id) {
  const HillEvent *event = tree->event;
  double reveal = tree->grow * (tree->max_depth + 1) - depth;
  if (reveal <= 0.0) return;
  if (reveal > 1.0) reveal = 1.0;
  double a = angle + sin(event->t * 0.7 + depth * 0.5 + id * 0.37) * 0.018 * (depth + 1);
  double x1 = x + sin(a) * length * reveal, y1 = y - cos(a) * length * reveal;
  int width = tree->max_depth - depth + 1;
  if (width > 7) width = 7;
  thickLineRGBA(event->surface, (int)x, (int)y, (int)x1, (int)y1, width, 28, 22, 30,
                weapon_alpha(238.0 * event->fade));
  if (depth >= tree->max_depth - 1 && tree->lit > 0.0) {
    static const Uint8 tint[3][3] = {{255, 200, 110}, {255, 150, 170}, {150, 220, 255}};
    double twinkle = 0.6 + 0.4 * sin(event->t * 2.5 + id * 1.9);
    weapon_glow(event->surface, x1, y1, depth == tree->max_depth ? 3.0 : 2.0, tint[id % 3],
                230.0 * tree->lit * twinkle * reveal * event->fade);
  }
  if (reveal < 1.0 || depth >= tree->max_depth) return;
  double r1 = hill_random(event, id + tree->salt, 1), r2 = hill_random(event, id + tree->salt, 2);
  hill_tree_branch(tree, x1, y1, a - 0.35 - 0.3 * r1, length * (0.72 + 0.08 * r2), depth + 1,
                   id * 2);
  hill_tree_branch(tree, x1, y1, a + 0.35 + 0.3 * r2, length * (0.72 + 0.08 * r1), depth + 1,
                   id * 2 + 1);
}


// Two bare trees at the sides of the screen grow branch by branch and then
// light up with coloured lamps.
static void hill_lighttree(const HillEvent *event) {
  for (int i = 0; i < 2; i++) {
    double x = event->surface->w * (i ? 0.92 : 0.08);
    double ground = hill_ground(event, x);
    HillTree tree;
    tree.event = event;
    tree.grow = smooth_step(0.02 + i * 0.06, 0.4 + i * 0.06, event->progress);
    tree.lit =
        smooth_step(0.35, 0.55, event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
    tree.max_depth = 6;
    tree.salt = i * 1000;
    if (tree.lit > 0.0) {
      ellipseRGBA(event->surface, (int)x, (int)ground, (int)(event->scale * 0.05),
                  (int)(event->scale * 0.008), 255, 200, 110,
                  weapon_alpha(70.0 * tree.lit * event->fade));
    }
    hill_tree_branch(&tree, x, ground, 0.0, event->scale * 0.075, 0, 1);
  }
}


// One meteorite hitting the near ridge: streak, flash, shock ring, debris, dust
// and a crater that cools.
static void hill_impact(const HillEvent *event, int id, double start, double x, int from) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double duration = event->progress > 0.001 ? event->t / event->progress : 47.0;
  double age = (event->progress - start) * duration;
  if (age < 0.0) return;
  double ground = hill_ground(event, x);
  const double flight = 1.5;
  static const Uint8 hot[3] = {255, 170, 90};
  if (age < flight) {
    double u = age / flight;
    double sx = x - from * s * 0.5, sy = ground - s * 0.8;
    double hx = sx + (x - sx) * u, hy = sy + (ground - sy) * u;
    double tx = hx - (x - sx) * 0.22, ty = hy - (ground - sy) * 0.22;
    thickLineRGBA(surface, (int)tx, (int)ty, (int)hx, (int)hy, 5, 255, 150, 70,
                  weapon_alpha(40.0 * event->fade));
    thickLineRGBA(surface, (int)tx, (int)ty, (int)hx, (int)hy, 3, 255, 180, 100,
                  weapon_alpha(120.0 * event->fade));
    thickLineRGBA(surface, (int)tx, (int)ty, (int)hx, (int)hy, 1, 255, 250, 235,
                  weapon_alpha(230.0 * event->fade));
    weapon_glow(surface, hx, hy, 4.0 + u * 3.0, hot, 235.0 * event->fade);
    return;
  }
  double q = age - flight;
  double glow = exp(-q / 6.0);
  filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.03), (int)(s * 0.007), 10, 8, 10,
                    weapon_alpha(230.0 * event->fade));
  filledEllipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.022), (int)(s * 0.005), 255, 110, 50,
                    weapon_alpha(200.0 * glow * event->fade));
  if (q < 1.4) {
    double flash = 1.0 - q / 1.4;
    weapon_glow(surface, x, ground - s * 0.01, s * 0.05 * flash + 2.0, hot,
                240.0 * flash * event->fade);
  }
  if (q < 3.5) {
    double w = q / 3.5;
    for (int ring = 0; ring < 2; ring++) {
      double rw = w - ring * 0.1;
      if (rw <= 0.0) continue;
      ellipseRGBA(surface, (int)x, (int)ground, (int)(s * 0.02 + rw * s * 0.22),
                  (int)(s * 0.004 + rw * s * 0.03), 255, 220, 170,
                  weapon_alpha(160.0 * (1.0 - rw) * event->fade));
    }
  }
  for (int puff = 0; puff < 7; puff++) {
    double u = q / 8.0 - puff * 0.05;
    if (u <= 0.0 || u >= 1.0) continue;
    double px = x + sin(puff * 2.3) * s * 0.03 * u + from * u * s * 0.03;
    double py = ground - u * s * 0.14 - (puff % 3) * s * 0.01;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(s * (0.012 + 0.03 * u)), 120, 105, 95,
                     weapon_alpha(90.0 * (1.0 - u) * event->fade));
  }
  const double gravity = s * 1.2;
  for (int j = 0; j < 10; j++) {
    double r1 = hill_random(event, id * 20 + j, 1), r2 = hill_random(event, id * 20 + j, 2);
    double vx = (r1 - 0.5) * s * 0.5, vy = s * (0.25 + 0.35 * r2);
    double land = 2.0 * vy / gravity;
    double tt = q < land ? q : land;
    double px = x + vx * tt, py = ground - (vy * tt - 0.5 * gravity * tt * tt);
    if (q < land) {
      filledCircleRGBA(surface, (int)px, (int)py, 1 + j % 2, 255, 150 + j * 5, 80,
                       weapon_alpha(235.0 * event->fade));
    } else {
      filledCircleRGBA(surface, (int)px, (int)py, 1 + j % 2, 50, 42, 44,
                       weapon_alpha(230.0 * event->fade));
      filledCircleRGBA(surface, (int)px, (int)py, 2 + j % 2, 255, 120, 50,
                       weapon_alpha(70.0 * glow * event->fade));
    }
  }
}


// Three meteorites crash into the ridge one after another.
static void hill_meteorite(const HillEvent *event) {
  hill_impact(event, 1, 0.1, event->surface->w * 0.12, 1);
  hill_impact(event, 2, 0.4, event->surface->w * 0.88, -1);
  hill_impact(event, 3, 0.68, event->surface->w * 0.22, 1);
}


// Dust devils: whirling funnels of sand wander along the ridge.
static void hill_dustdevils(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int devils = 3, slices = 16;
  for (int i = 0; i < devils; i++) {
    double u = fmod(event->progress * 1.3 + i / (double)devils, 1.0);
    double edge = smooth_step(0.0, 0.08, u) * (1.0 - smooth_step(0.92, 1.0, u));
    double x = surface->w * (0.05 + 0.9 * (event->direction > 0 ? u : 1.0 - u));
    double base = hill_ground(event, x);
    double height = s * (0.1 + 0.04 * (i % 2));
    for (int k = 0; k < slices; k++) {
      double v = (k + 0.5) / slices;
      double width = s * (0.005 + 0.04 * v);
      double sway = sin(event->t * 2.5 - v * 5.0 + i) * s * 0.012 * v;
      double shade = 0.6 + 0.4 * cos(k * 1.7 - event->t * 10.0);
      filledEllipseRGBA(surface, (int)(x + sway), (int)(base - v * height), (int)width,
                        (int)(width * 0.25) + 1, 190, 168, 135,
                        weapon_alpha(150.0 * (1.0 - 0.4 * v) * shade * edge * event->fade));
    }
    for (int j = 0; j < 10; j++) {
      double v = fmod(event->t * 0.15 + j / 10.0 + i * 0.3, 1.0);
      double angle = event->t * (6.0 + i) + j * 2.1;
      double width = s * (0.005 + 0.04 * v);
      double sway = sin(event->t * 2.5 - v * 5.0 + i) * s * 0.012 * v;
      double bx = x + sway + cos(angle) * width * 1.1;
      double by = base - v * height + sin(angle) * width * 0.25;
      lineRGBA(surface, (int)bx, (int)by, (int)(bx - sin(angle) * 4.0), (int)(by + cos(angle)), 60,
               50, 44, weapon_alpha(200.0 * edge * event->fade));
    }
    ellipseRGBA(surface, (int)x, (int)base, (int)(s * 0.03), (int)(s * 0.004), 170, 150, 120,
                weapon_alpha(60.0 * edge * event->fade));
  }
}


// A laser show: emitters along the ridge sweep fans of coloured beams across
// the sky.
static void hill_lasers(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  static const Uint8 colors[4][3] = {
      {255, 70, 200}, {70, 230, 255}, {120, 255, 120}, {255, 200, 70}};
  const int emitters = 5;
  double show =
      smooth_step(0.06, 0.12, event->progress) * (1.0 - smooth_step(0.9, 0.96, event->progress));
  for (int i = 0; i < emitters; i++) {
    double x = surface->w * (0.08 + 0.84 * i / (emitters - 1.0));
    double ground = hill_ground(event, x);
    boxRGBA(surface, (int)x - 3, (int)ground - 4, (int)x + 3, (int)ground + 1, 20, 24, 36,
            weapon_alpha(238.0 * event->fade));
    if (show <= 0.0) continue;
    const Uint8 *c = colors[i % 4];
    for (int b = 0; b < 3; b++) {
      double angle = (b - 1) * 0.5 + sin(event->t * (0.5 + 0.17 * i) + i * 1.3 + b) * 0.35;
      double length = s * 1.1;
      double ex = x + sin(angle) * length, ey = ground - cos(angle) * length;
      double flicker = (0.85 + 0.15 * sin(event->t * 17.0 + i * 3.0 + b)) * show * event->fade;
      thickLineRGBA(surface, (int)x, (int)ground, (int)ex, (int)ey, 7, c[0], c[1], c[2],
                    weapon_alpha(14.0 * flicker));
      thickLineRGBA(surface, (int)x, (int)ground, (int)ex, (int)ey, 3, c[0], c[1], c[2],
                    weapon_alpha(45.0 * flicker));
      thickLineRGBA(surface, (int)x, (int)ground, (int)ex, (int)ey, 1, 255, 255, 255,
                    weapon_alpha(150.0 * flicker));
    }
    weapon_glow(surface, x, ground - 4.0, 4.0, c, 200.0 * show * event->fade);
  }
}


// A jagged arc of lightning between two points, drawn as a glow and a white
// core.
static void hill_arc(RenderTarget *surface, double x0, double y0, double x1, double y1,
                     int segments, double jitter, unsigned int seed, const Uint8 *color,
                     double alpha) {
  double px = x0, py = y0;
  for (int k = 1; k <= segments; k++) {
    double u = (double)k / segments;
    double nx = x0 + (x1 - x0) * u, ny = y0 + (y1 - y0) * u;
    if (k < segments) {
      nx += ((fx_hash(seed, k) % 2001) / 1000.0 - 1.0) * jitter;
      ny += ((fx_hash(seed, k + 64) % 2001) / 1000.0 - 1.0) * jitter;
    }
    thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 3, color[0], color[1], color[2],
                  weapon_alpha(alpha * 0.25));
    thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 1, 235, 240, 255,
                  weapon_alpha(alpha));
    px = nx;
    py = ny;
  }
}


// Tesla towers charge up and throw lightning between their spheres and down to
// the ground.
static void hill_tesla(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int towers = 4;
  static const Uint8 blue[3] = {140, 180, 255};
  double tx[4], ty[4], rise[4];
  double charge =
      smooth_step(0.1, 0.2, event->progress) * (1.0 - smooth_step(0.9, 0.96, event->progress));
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int i = 0; i < towers; i++) {
    double x = surface->w * (0.07 + 0.86 * i / (towers - 1.0));
    rise[i] = smooth_step(0.0 + i * 0.02, 0.1 + i * 0.02, event->progress);
    double base = hill_ground(event, x);
    double height = s * (0.12 + 0.02 * (i % 2)) * rise[i];
    double top = base - height;
    double sphere = s * 0.014;
    double wide = s * 0.012;
    tx[i] = x;
    ty[i] = top - sphere;
    if (rise[i] <= 0.0) continue;
    filledTrigonRGBA(surface, (int)(x - wide), (int)base, (int)(x + wide), (int)base, (int)x,
                     (int)top, 20, 24, 36, alpha);
    for (int bar = 1; bar < 4; bar++) {
      double f = bar / 4.0;
      lineRGBA(surface, (int)(x - wide * (1.0 - f)), (int)(base - height * f),
               (int)(x + wide * (1.0 - f)), (int)(base - height * f), 60, 68, 92, alpha);
    }
    filledCircleRGBA(surface, (int)x, (int)ty[i], (int)sphere, 70, 78, 100, alpha);
    weapon_glow(surface, x, ty[i], sphere * 0.9, blue,
                200.0 * charge * (0.7 + 0.3 * sin(event->t * 9.0 + i)) * event->fade);
  }
  if (charge <= 0.1) return;
  unsigned int burst = (unsigned int)(event->t * 1.5);
  unsigned int tick = (unsigned int)(event->t * 14.0);
  for (int i = 0; i < towers; i++) {
    if (rise[i] < 0.95) continue;
    if (i < towers - 1 && rise[i + 1] >= 0.95 && fx_hash(event->seed, burst * 8 + i) % 3 != 0) {
      hill_arc(surface, tx[i], ty[i], tx[i + 1], ty[i + 1], 9, s * 0.012,
               fx_hash(event->seed, tick * 8 + i), blue, 230.0 * charge * event->fade);
    }
    if (fx_hash(event->seed, burst * 8 + 4 + i) % 4 == 0) {
      double ex = tx[i] + (hill_random(event, i, 9) - 0.5) * s * 0.2;
      hill_arc(surface, tx[i], ty[i], ex, hill_ground(event, ex), 8, s * 0.01,
               fx_hash(event->seed, tick * 8 + 4 + i), blue, 200.0 * charge * event->fade);
    }
  }
}


// Hot air balloons are inflated on the ridge, their burners roar and they lift
// off one by one.
static void hill_balloons(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  static const Uint8 palette[4][2][3] = {{{150, 70, 70}, {225, 195, 150}},
                                         {{60, 110, 150}, {215, 215, 195}},
                                         {{90, 130, 80}, {225, 185, 110}},
                                         {{130, 90, 150}, {215, 205, 175}}};
  const int count = 4;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int i = 0; i < count; i++) {
    double x0 = hill_side(event, 0.07 + 0.08 * i);
    double ground = hill_ground(event, x0);
    double inflate = smooth_step(0.02 + 0.03 * i, 0.14 + 0.03 * i, event->progress);
    if (inflate <= 0.0) continue;
    double launch = 0.3 + 0.1 * i;
    double rise = smooth_step(launch, launch + 0.4, event->progress);
    double bx = x0 + event->direction * rise * rise * surface->w * 0.2 +
                sin(event->t * 0.6 + i * 1.7) * s * 0.01 * rise;
    double by = ground - rise * s * 0.95;
    double r = s * (0.042 + 0.006 * (i % 2)) * inflate;
    double rope = s * 0.03 * inflate;
    double ex = bx + sin(event->t * 0.5 + i) * r * 0.05, ey = by - rope - r * 1.15;
    lineRGBA(surface, (int)bx - 3, (int)by, (int)(ex - r * 0.45), (int)(ey + r), 30, 26, 24, alpha);
    lineRGBA(surface, (int)bx + 3, (int)by, (int)(ex + r * 0.45), (int)(ey + r), 30, 26, 24, alpha);
    filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.88) + 1, (int)(r * 1.18) + 1, 20, 22,
                      30, alpha);
    const Uint8 *a = palette[i % 4][0], *b = palette[i % 4][1];
    filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.85), (int)(r * 1.15), a[0], a[1], a[2],
                      alpha);
    filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.55), (int)(r * 1.15), b[0], b[1], b[2],
                      alpha);
    filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.25), (int)(r * 1.15), a[0], a[1], a[2],
                      alpha);
    boxRGBA(surface, (int)bx - 4, (int)by, (int)bx + 4, (int)by + 5, 70, 50, 40, alpha);
    double flare = fmod(event->t + i * 0.8, 2.6);
    if (flare < 0.6 && inflate > 0.5) {
      static const Uint8 warm[3] = {255, 190, 110};
      double f = 1.0 - flare / 0.6;
      filledTrigonRGBA(surface, (int)bx - 2, (int)(by - rope * 0.9), (int)bx + 2,
                       (int)(by - rope * 0.9), (int)bx, (int)(by - rope * 0.9 - r * 0.5 * f), 255,
                       160, 70, weapon_alpha(220.0 * f * event->fade));
      filledEllipseRGBA(surface, (int)ex, (int)ey, (int)(r * 0.6), (int)(r * 0.9), 255, 190, 110,
                        weapon_alpha(45.0 * f * event->fade));
      weapon_glow(surface, bx, by - rope * 0.95, r * 0.12, warm, 230.0 * f * event->fade);
    }
  }
}


// A giant walking figure strides slowly along the middle ridge, its eye and
// chest glowing.
static void hill_giant(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double H = s * 0.26;
  double margin = H * 0.5;
  double span = surface->w + 2.0 * margin;
  double x =
      d > 0 ? -margin + event->progress * span : surface->w + margin - event->progress * span;
  double ground = ground_y(surface, RIDGE_MIDDLE, x) + 2.0;
  double gait = event->t * SCENE_TAU * 0.2;
  double thigh = H * 0.24, shin = H * 0.24, swing = H * 0.17;
  double hip_y = ground - (thigh + shin) * 0.93 - fabs(cos(gait)) * H * 0.015;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  filledEllipseRGBA(surface, (int)x, (int)ground, (int)(H * 0.3), (int)(H * 0.03), 8, 10, 16,
                    weapon_alpha(110.0 * event->fade));
  for (int leg = 0; leg < 2; leg++) {
    double ph = gait + leg * SCENE_PI;
    double foot_x = x + sin(ph) * swing, foot_y = ground - fmax(0.0, cos(ph)) * H * 0.05;
    double knee_x = (x + foot_x) / 2.0 + d * H * 0.05, knee_y = (hip_y + foot_y) / 2.0 - H * 0.01;
    Uint8 shade = leg ? 44 : 32;
    thickLineRGBA(surface, (int)x, (int)hip_y, (int)knee_x, (int)knee_y, 9, shade, shade + 4,
                  shade + 14, alpha);
    thickLineRGBA(surface, (int)knee_x, (int)knee_y, (int)foot_x, (int)foot_y, 7, shade, shade + 4,
                  shade + 14, alpha);
    thickLineRGBA(surface, (int)foot_x, (int)foot_y, (int)(foot_x + d * H * 0.07), (int)foot_y, 5,
                  shade, shade + 4, shade + 14, alpha);
  }
  double shoulder_x = x + d * H * 0.03, shoulder_y = hip_y - H * 0.33;
  thickLineRGBA(surface, (int)x, (int)hip_y, (int)shoulder_x, (int)shoulder_y, 16, 40, 46, 62,
                alpha);
  thickLineRGBA(surface, (int)(x + d * 4), (int)hip_y, (int)(shoulder_x + d * 4), (int)shoulder_y,
                2, 60, 70, 92, weapon_alpha(200.0 * event->fade));
  for (int arm = 0; arm < 2; arm++) {
    double ph = gait + arm * SCENE_PI + SCENE_PI;
    double hand_x = shoulder_x + sin(ph) * H * 0.13,
           hand_y = shoulder_y + H * 0.3 - fmax(0.0, cos(ph)) * H * 0.03;
    thickLineRGBA(surface, (int)shoulder_x, (int)shoulder_y, (int)hand_x, (int)hand_y, 6,
                  arm ? 48 : 34, arm ? 54 : 40, arm ? 72 : 56, alpha);
  }
  double head_x = shoulder_x + d * H * 0.03, head_y = shoulder_y - H * 0.09;
  filledCircleRGBA(surface, (int)head_x, (int)head_y, (int)(H * 0.065), 42, 48, 66, alpha);
  static const Uint8 eye[3] = {255, 130, 70}, core[3] = {110, 220, 255};
  weapon_glow(surface, head_x + d * H * 0.03, head_y, 2.5, eye, 230.0 * event->fade);
  weapon_glow(surface, x + d * H * 0.02, (hip_y + shoulder_y) / 2.0, H * 0.025, core,
              200.0 * event->fade * (0.7 + 0.3 * sin(event->t * 2.0)));
}


// A clock tower rises and its lit face spins fast; every few seconds the bell
// sends out rings.
static void hill_clocktower(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = hill_side(event, 0.1);
  double ground = hill_ground(event, cx);
  double w = s * 0.035, h = s * 0.22 * smooth_step(0.0, 0.1, event->progress);
  if (h < 4.0) return;
  double top = ground - h;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double lit =
      smooth_step(0.1, 0.18, event->progress) * (1.0 - smooth_step(0.92, 0.97, event->progress));
  boxRGBA(surface, (int)(cx - w), (int)top, (int)(cx + w), (int)ground, 26, 30, 44, alpha);
  boxRGBA(surface, (int)(cx - w), (int)top, (int)(cx - w * 0.6), (int)ground, 40, 46, 64, alpha);
  filledTrigonRGBA(surface, (int)(cx - w - 3), (int)top, (int)(cx + w + 3), (int)top, (int)cx,
                   (int)(top - w * 1.7), 16, 18, 28, alpha);
  double face_y = top + w * 1.15, face_r = w * 0.78;
  filledCircleRGBA(surface, (int)cx, (int)face_y, (int)face_r + 2, 40, 46, 64, alpha);
  filledCircleRGBA(surface, (int)cx, (int)face_y, (int)face_r, (Uint8)(30 + 225 * lit),
                   (Uint8)(34 + 198 * lit), (Uint8)(46 + 124 * lit), alpha);
  for (int tick = 0; tick < 12; tick++) {
    double a = tick * SCENE_TAU / 12.0;
    lineRGBA(surface, (int)(cx + cos(a) * face_r * 0.8), (int)(face_y + sin(a) * face_r * 0.8),
             (int)(cx + cos(a) * face_r * 0.95), (int)(face_y + sin(a) * face_r * 0.95), 40, 32, 28,
             alpha);
  }
  double minute = event->t * 0.6 - SCENE_PI / 2.0, hour = event->t * 0.05 - SCENE_PI / 2.0;
  thickLineRGBA(surface, (int)cx, (int)face_y, (int)(cx + cos(minute) * face_r * 0.82),
                (int)(face_y + sin(minute) * face_r * 0.82), 1, 30, 24, 22, alpha);
  thickLineRGBA(surface, (int)cx, (int)face_y, (int)(cx + cos(hour) * face_r * 0.5),
                (int)(face_y + sin(hour) * face_r * 0.5), 2, 30, 24, 22, alpha);
  for (int k = 0; k < 3; k++) {
    double wy = face_y + face_r + w * (0.7 + 0.7 * k);
    if (wy + 6 > ground) break;
    boxRGBA(surface, (int)cx - 2, (int)wy, (int)cx + 2, (int)wy + 7, 255, 214, 130,
            weapon_alpha(220.0 * lit * event->fade));
  }
  double local = fmod(event->t, 8.0);
  for (int ring = 0; ring < 3; ring++) {
    double age = local - ring * 0.8;
    if (age <= 0.0 || age >= 2.2) continue;
    double u = age / 2.2;
    int radius = (int)(face_r + u * s * 0.12);
    Uint8 a = weapon_alpha(150.0 * (1.0 - u) * lit * event->fade);
    circleRGBA(surface, (int)cx, (int)face_y, radius, 255, 220, 150, a);
    circleRGBA(surface, (int)cx, (int)face_y, radius + 1, 255, 220, 150, a);
  }
}


// Point `u` (0..1) of a drone formation shape, in -1..1 units.
static void drone_shape(int shape, double u, double *px, double *py) {
  switch (shape) {
    case 0:
      *px = cos(u * SCENE_TAU);
      *py = sin(u * SCENE_TAU);
      break;
    case 1: {
      double f = u * 10.0;
      int k = (int)f % 10;
      double frac = f - floor(f);
      double a0 = -SCENE_PI / 2.0 + k * SCENE_PI / 5.0, a1 = a0 + SCENE_PI / 5.0;
      double r0 = k % 2 == 0 ? 1.0 : 0.42, r1 = k % 2 == 0 ? 0.42 : 1.0;
      *px = cos(a0) * r0 * (1.0 - frac) + cos(a1) * r1 * frac;
      *py = sin(a0) * r0 * (1.0 - frac) + sin(a1) * r1 * frac;
      break;
    }
    case 2: {
      double a = u * SCENE_TAU;
      *px = 16.0 * pow(sin(a), 3.0) / 17.0;
      *py = -(13.0 * cos(a) - 5.0 * cos(2.0 * a) - 2.0 * cos(3.0 * a) - cos(4.0 * a)) / 17.0;
      break;
    }
    default:
      *px = -1.0 + 2.0 * u;
      *py = 0.35 * sin(u * SCENE_TAU * 2.0);
      break;
  }
}


// A swarm of drone lights lifts off the ridge and forms a ring, a star, a heart
// and a wave.
static void hill_drones(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  const int drones = 48;
  static const Uint8 palette[4][3] = {
      {255, 200, 110}, {120, 220, 255}, {255, 120, 160}, {150, 255, 160}};
  double cx = surface->w * (0.5 + 0.3 * sin(event->progress * SCENE_TAU * 0.8));
  double cy = surface->h * 0.1, radius = event->scale * 0.075;
  double st = event->t / 6.5;
  int a = (int)st % 4, b = (a + 1) % 4;
  double blend = smooth_step(0.75, 1.0, st - floor(st));
  const Uint8 *color = palette[blend > 0.5 ? b : a];
  for (int i = 0; i < drones; i++) {
    double u = (double)i / drones;
    double ax, ay, bx, by;
    drone_shape(a, u, &ax, &ay);
    drone_shape(b, u, &bx, &by);
    double target_x = cx + (ax + (bx - ax) * blend) * radius;
    double target_y = cy + (ay + (by - ay) * blend) * radius;
    double r = hill_random(event, i, 1);
    double gx = surface->w * (0.08 + 0.84 * (i + 0.5) / drones);
    double gy = ground_y(surface, RIDGE_NEAR, gx);
    double up = smooth_step(0.02 + 0.08 * r, 0.16 + 0.08 * r, event->progress) *
                (1.0 - smooth_step(0.84 + 0.08 * r, 0.94 + 0.05 * r, event->progress));
    if (up <= 0.0) continue;
    weapon_glow(surface, gx + (target_x - gx) * up, gy + (target_y - gy) * up, 2.2, color,
                235.0 * event->fade * (0.75 + 0.25 * sin(event->t * 3.0 + i)));
  }
}


// A cave mouth in a mound: a stream of bats pours out and fans over the sky.
static void hill_bats(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = hill_side(event, 0.12);
  double ground = hill_ground(event, cx);
  double mound_w = s * 0.07, mound_h = s * 0.045;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  filledEllipseRGBA(surface, (int)cx, (int)(ground + 2), (int)mound_w, (int)mound_h, 18, 22, 32,
                    alpha);
  filledEllipseRGBA(surface, (int)(cx + d * mound_w * 0.2), (int)(ground - mound_h * 0.1),
                    (int)(mound_w * 0.28), (int)(mound_h * 0.5), 4, 5, 8, alpha);
  const int bats = 44;
  double active = smooth_step(0.05, 0.25, event->progress);
  double away = 1.0 - smooth_step(0.85, 0.95, event->progress);
  for (int i = 0; i < bats; i++) {
    if (i / (double)bats > active) continue;
    double r1 = hill_random(event, i, 1), r2 = hill_random(event, i, 2),
           r3 = hill_random(event, i, 3);
    double u = fmod(event->t * (0.08 + 0.04 * r3) + r1, 1.0);
    double bx =
        cx + d * (mound_w * 0.1 + u * s * (0.35 + 0.6 * r2)) + sin(u * 9.0 + i) * s * 0.02 * u;
    double by =
        ground - mound_h * 0.3 - u * s * (0.25 + 0.5 * r1) + sin(u * 13.0 + i * 2.1) * s * 0.012;
    double vis = smooth_step(0.0, 0.05, u) * (1.0 - smooth_step(0.7, 1.0, u)) * away * event->fade;
    double size = s * (0.011 + 0.006 * r3) * (0.5 + 0.9 * u);
    double flap = sin(event->t * 13.0 + i * 1.7);
    Uint8 a = weapon_alpha(240.0 * vis);
    for (int side = -1; side <= 1; side += 2) {
      double wx = bx + side * size * 1.4, wy = by - flap * size * 0.9;
      thickLineRGBA(surface, (int)bx, (int)by, (int)wx, (int)wy, 2, 8, 10, 16, a);
      thickLineRGBA(surface, (int)wx, (int)wy, (int)(bx + side * size * 2.0),
                    (int)(by - flap * size * 0.5 + size * 0.3), 1, 8, 10, 16, a);
    }
    filledCircleRGBA(surface, (int)bx, (int)by, (int)(size * 0.35) + 1, 8, 10, 16, a);
  }
}


// A stone gate wakes: the arch fills with a swirling light and travellers step
// out of it.
static void hill_gate(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = hill_side(event, 0.12);
  double ground = hill_ground(event, cx);
  double height = s * 0.12, half = s * 0.045, post = s * 0.015;
  double open =
      smooth_step(0.06, 0.16, event->progress) * (1.0 - smooth_step(0.9, 0.96, event->progress));
  double mid_y = ground - height * 0.5;
  static const Uint8 layers[5][3] = {
      {80, 50, 190}, {110, 90, 230}, {120, 150, 250}, {170, 210, 255}, {240, 250, 255}};
  if (open > 0.0) {
    for (int k = 0; k < 5; k++) {
      double f = 1.0 - k * 0.17;
      filledEllipseRGBA(surface, (int)cx, (int)mid_y, (int)(half * f), (int)(height * 0.5 * f),
                        layers[k][0], layers[k][1], layers[k][2],
                        weapon_alpha(120.0 * open * event->fade));
    }
    for (int j = 0; j < 8; j++) {
      double a = event->t * 1.6 + j * 0.78;
      double rad = 0.15 + 0.85 * fmod(j * 0.37 + event->t * 0.2, 1.0);
      weapon_glow(surface, cx + cos(a) * half * 0.9 * rad, mid_y + sin(a) * height * 0.45 * rad,
                  1.5, layers[3], 200.0 * open * event->fade);
    }
  }
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  boxRGBA(surface, (int)(cx - half - post), (int)(ground - height), (int)(cx - half), (int)ground,
          28, 32, 44, alpha);
  boxRGBA(surface, (int)(cx + half), (int)(ground - height), (int)(cx + half + post), (int)ground,
          28, 32, 44, alpha);
  boxRGBA(surface, (int)(cx - half - post * 1.3), (int)(ground - height - post),
          (int)(cx + half + post * 1.3), (int)(ground - height), 34, 38, 52, alpha);
  weapon_glow(surface, cx, ground - height - post * 0.5, 2.0, layers[3],
              200.0 * open * event->fade);
  for (int k = 0; k < 5; k++) {
    double u = (event->progress - (0.2 + 0.12 * k)) / 0.4;
    if (u <= 0.0 || u >= 1.0) continue;
    double x = cx + d * (half * 0.2 + u * s * 0.7);
    double vis = smooth_step(0.0, 0.08, u) * (1.0 - smooth_step(0.85, 1.0, u));
    hill_walker(surface, x, hill_ground(event, x), s * 0.036, d, event->t * 5.0 + k * 2.0,
                k % 2 == 0, event->fade * vis, event->t, k);
  }
}


// A striped lighthouse rises on the ridge; its lamp turns, sweeping two beams
// across the sky.
static void hill_lighthouse(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = hill_side(event, 0.1);
  double ground = hill_ground(event, cx);
  double h = s * 0.16 * smooth_step(0.0, 0.1, event->progress), w = s * 0.022;
  if (h < 4.0) return;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double lit =
      smooth_step(0.1, 0.18, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  double lamp_y = ground - h - w * 0.5;
  static const Uint8 warm[3] = {255, 235, 170};
  double a = event->t * 1.1;
  for (int band = 0; band < 6; band++) {
    double y0 = ground - h * band / 6.0, y1 = ground - h * (band + 1) / 6.0;
    double w0 = w * (1.0 - 0.45 * band / 6.0), w1 = w * (1.0 - 0.45 * (band + 1) / 6.0);
    Uint8 r = band % 2 ? 150 : 120, g = band % 2 ? 156 : 48, b = band % 2 ? 172 : 52;
    filledTrigonRGBA(surface, (int)(cx - w0), (int)y0, (int)(cx + w0), (int)y0, (int)(cx - w1),
                     (int)y1, r, g, b, alpha);
    filledTrigonRGBA(surface, (int)(cx + w0), (int)y0, (int)(cx + w1), (int)y1, (int)(cx - w1),
                     (int)y1, r, g, b, alpha);
  }
  boxRGBA(surface, (int)(cx - w * 0.75), (int)(lamp_y - w * 0.2), (int)(cx + w * 0.75),
          (int)(lamp_y + w * 0.5), 30, 34, 48, alpha);
  filledTrigonRGBA(surface, (int)(cx - w * 0.9), (int)(lamp_y - w * 0.2), (int)(cx + w * 0.9),
                   (int)(lamp_y - w * 0.2), (int)cx, (int)(lamp_y - w * 1.0), 120, 48, 52, alpha);
  if (lit <= 0.0) return;
  double sweep = cos(a), reach = s * 0.9 * fabs(sweep);
  for (int side = -1; side <= 1; side += 2) {
    double ex = cx + side * (sweep >= 0.0 ? 1 : -1) * reach;
    filledTrigonRGBA(surface, (int)cx, (int)lamp_y, (int)ex, (int)(lamp_y - s * 0.02), (int)ex,
                     (int)(lamp_y + s * 0.02), 255, 245, 200,
                     weapon_alpha(40.0 * lit * fabs(sweep) * event->fade));
  }
  weapon_glow(surface, cx, lamp_y + w * 0.15, w * (0.4 + 0.8 * pow(fabs(sin(a)), 6.0)), warm,
              235.0 * lit * event->fade);
}


// A cherry tree grows and comes into blossom, then lets its petals fall in a
// slow drift.
static void hill_cherry(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = hill_side(event, 0.13);
  double ground = hill_ground(event, cx);
  double grow = smooth_step(0.0, 0.18, event->progress);
  double bloom =
      smooth_step(0.12, 0.4, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  double trunk = s * 0.075 * grow;
  if (trunk < 3.0) return;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double top_x = cx + d * s * 0.01, top_y = ground - trunk;
  thickLineRGBA(surface, (int)cx, (int)ground, (int)top_x, (int)top_y, 5, 52, 38, 40, alpha);
  for (int b = -1; b <= 1; b += 2) {
    thickLineRGBA(surface, (int)top_x, (int)(top_y + trunk * 0.25), (int)(top_x + b * s * 0.04),
                  (int)(top_y - trunk * 0.1), 3, 52, 38, 40, alpha);
  }
  for (int blob = 0; blob < 11; blob++) {
    double ang = blob * 0.57, rad = s * (0.018 + 0.02 * ((blob * 7) % 5) / 5.0);
    double bx = top_x + cos(ang * 2.1) * s * 0.045,
           by = top_y - trunk * 0.1 + sin(ang * 1.3) * s * 0.025;
    filledCircleRGBA(surface, (int)bx, (int)by, (int)(rad * bloom) + 1,
                     (Uint8)(214 + blob % 3 * 12), (Uint8)(130 + blob % 4 * 14),
                     (Uint8)(165 + blob % 3 * 10), weapon_alpha(200.0 * bloom * event->fade));
  }
  for (int petal = 0; petal < 34; petal++) {
    double r = hill_random(event, petal, 1), r2 = hill_random(event, petal, 2);
    double u = fmod(event->t * (0.07 + 0.05 * r2) + r, 1.0);
    double px = top_x + (r2 - 0.5) * s * 0.12 + u * d * s * (0.1 + 0.1 * r) +
                sin(event->t * 1.4 + petal) * s * 0.012;
    double py = top_y - s * 0.02 + u * (ground - top_y + s * 0.02);
    double vis = smooth_step(0.0, 0.1, u) * (1.0 - smooth_step(0.85, 1.0, u)) * bloom * event->fade;
    filledEllipseRGBA(surface, (int)px, (int)py, 2, 1, 255, 190, 214, weapon_alpha(230.0 * vis));
  }
}


// A wolf on a rock howls at a rising moon; rings of sound spread from its
// muzzle.
static void hill_wolf(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = hill_side(event, 0.14);
  double ground = hill_ground(event, cx);
  double rise = smooth_step(0.0, 0.3, event->progress);
  double moon_x = cx + d * s * 0.16, moon_y = ground - s * 0.1 - rise * s * 0.22, moon_r = s * 0.05;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  filledCircleRGBA(surface, (int)moon_x, (int)moon_y, (int)(moon_r * 1.7), 190, 205, 235,
                   weapon_alpha(26.0 * event->fade));
  filledCircleRGBA(surface, (int)moon_x, (int)moon_y, (int)moon_r, 226, 232, 240,
                   weapon_alpha(215.0 * event->fade));
  filledCircleRGBA(surface, (int)(moon_x - moon_r * 0.3), (int)(moon_y + moon_r * 0.1),
                   (int)(moon_r * 0.22), 196, 204, 216, weapon_alpha(200.0 * event->fade));
  double wy = ground - s * 0.03;
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(s * 0.05), (int)(s * 0.035), 18, 22, 32,
                    alpha);
  double howl = smooth_step(0.0, 0.3, fmod(event->t, 6.0)) *
                (1.0 - smooth_step(3.6, 4.2, fmod(event->t, 6.0)));
  double neck_x = cx + d * s * 0.016, neck_y = wy - s * 0.016;
  double head_x = neck_x + d * s * (0.012 + 0.012 * howl),
         head_y = neck_y - s * (0.012 + 0.016 * howl);
  double nose_x = head_x + d * s * (0.014 - 0.002 * howl),
         nose_y = head_y - s * (0.004 + 0.012 * howl);
  filledEllipseRGBA(surface, (int)cx, (int)(wy - s * 0.008), (int)(s * 0.022), (int)(s * 0.01), 8,
                    10, 16, alpha);
  for (int leg = -1; leg <= 1; leg += 2) {
    thickLineRGBA(surface, (int)(cx + leg * s * 0.014), (int)(wy - s * 0.004),
                  (int)(cx + leg * s * 0.016), (int)(wy + s * 0.012), 2, 8, 10, 16, alpha);
  }
  thickLineRGBA(surface, (int)(cx - d * s * 0.02), (int)(wy - s * 0.01), (int)(cx - d * s * 0.036),
                (int)(wy + s * 0.006), 3, 8, 10, 16, alpha);
  thickLineRGBA(surface, (int)(cx + d * s * 0.012), (int)(wy - s * 0.008), (int)head_x, (int)head_y,
                5, 8, 10, 16, alpha);
  thickLineRGBA(surface, (int)head_x, (int)head_y, (int)nose_x, (int)nose_y, 4, 8, 10, 16, alpha);
  filledTrigonRGBA(surface, (int)head_x, (int)(head_y - s * 0.002), (int)(head_x - d * s * 0.004),
                   (int)(head_y - s * 0.012), (int)(head_x + d * s * 0.002),
                   (int)(head_y - s * 0.002), 8, 10, 16, alpha);
  for (int ring = 0; ring < 3; ring++) {
    double age = fmod(event->t, 6.0) - 0.4 - ring * 0.6;
    if (age <= 0.0 || age >= 2.2) continue;
    double u = age / 2.2;
    ellipseRGBA(surface, (int)nose_x, (int)nose_y, (int)(s * 0.01 + u * s * 0.07),
                (int)(s * 0.007 + u * s * 0.05), 200, 215, 240,
                weapon_alpha(110.0 * (1.0 - u) * howl * event->fade));
  }
}


// A pumpkin patch: round pumpkins swell out of the ground and their carved
// faces light up.
static void hill_pumpkins(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int count = 9;
  for (int i = 0; i < count; i++) {
    double r = hill_random(event, i, 1);
    double x = surface->w * (0.06 + 0.88 * (i + 0.5 + (r - 0.5) * 0.5) / count);
    double grow = smooth_step(0.02 + i * 0.03, 0.14 + i * 0.03, event->progress);
    if (grow <= 0.0) continue;
    double ground = hill_ground(event, x);
    double rad = s * (0.017 + 0.012 * r) * grow;
    double lit = smooth_step(0.2 + i * 0.03, 0.28 + i * 0.03, event->progress) *
                 (1.0 - smooth_step(0.9, 0.97, event->progress));
    double flicker = 0.8 + 0.2 * sin(event->t * 9.0 + i * 2.3);
    int cy = (int)(ground - rad * 0.8);
    Uint8 alpha = weapon_alpha(238.0 * event->fade);
    filledEllipseRGBA(surface, (int)x, cy, (int)rad + 1, (int)(rad * 0.82) + 1, 118, 54, 20, alpha);
    filledEllipseRGBA(surface, (int)(x - rad * 0.5), cy, (int)(rad * 0.5) + 1, (int)(rad * 0.8) + 1,
                      138, 66, 24, alpha);
    filledEllipseRGBA(surface, (int)(x + rad * 0.5), cy, (int)(rad * 0.5) + 1, (int)(rad * 0.8) + 1,
                      108, 48, 18, alpha);
    thickLineRGBA(surface, (int)x, (int)(cy - rad * 0.8), (int)(x + rad * 0.15),
                  (int)(cy - rad * 1.2), 2, 52, 62, 30, alpha);
    if (lit <= 0.0 || rad < 5.0) continue;
    Uint8 glow = weapon_alpha(235.0 * lit * flicker * event->fade);
    filledCircleRGBA(surface, (int)x, cy, (int)(rad * 1.8), 255, 150, 50,
                     weapon_alpha(26.0 * lit * flicker * event->fade));
    filledTrigonRGBA(surface, (int)(x - rad * 0.55), (int)(cy - rad * 0.1), (int)(x - rad * 0.15),
                     (int)(cy - rad * 0.1), (int)(x - rad * 0.35), (int)(cy - rad * 0.4), 255, 205,
                     90, glow);
    filledTrigonRGBA(surface, (int)(x + rad * 0.15), (int)(cy - rad * 0.1), (int)(x + rad * 0.55),
                     (int)(cy - rad * 0.1), (int)(x + rad * 0.35), (int)(cy - rad * 0.4), 255, 205,
                     90, glow);
    lineRGBA(surface, (int)(x - rad * 0.5), (int)(cy + rad * 0.25), (int)(x - rad * 0.2),
             (int)(cy + rad * 0.4), 255, 205, 90, glow);
    lineRGBA(surface, (int)(x - rad * 0.2), (int)(cy + rad * 0.4), (int)(x + rad * 0.2),
             (int)(cy + rad * 0.4), 255, 205, 90, glow);
    lineRGBA(surface, (int)(x + rad * 0.2), (int)(cy + rad * 0.4), (int)(x + rad * 0.5),
             (int)(cy + rad * 0.25), 255, 205, 90, glow);
  }
}


// A small castle with crenellated towers, lit windows, flickering gate torches
// and waving flags.
static void hill_castle(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = hill_side(event, 0.14);
  double ground = hill_ground(event, cx) + 2.0;
  double build = smooth_step(0.0, 0.14, event->progress);
  double lit =
      smooth_step(0.14, 0.24, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  if (build < 0.05) return;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double keep_w = s * 0.05, keep_h = s * 0.09 * build, tower_w = s * 0.026,
         tower_h = s * 0.12 * build;
  boxRGBA(surface, (int)(cx - keep_w), (int)(ground - keep_h), (int)(cx + keep_w), (int)ground, 26,
          30, 44, alpha);
  for (int side = -1; side <= 1; side += 2) {
    double tx = cx + side * (keep_w + tower_w * 0.6);
    boxRGBA(surface, (int)(tx - tower_w), (int)(ground - tower_h), (int)(tx + tower_w), (int)ground,
            32, 36, 52, alpha);
    filledTrigonRGBA(surface, (int)(tx - tower_w * 1.2), (int)(ground - tower_h),
                     (int)(tx + tower_w * 1.2), (int)(ground - tower_h), (int)tx,
                     (int)(ground - tower_h - tower_w * 2.2), 70, 34, 44, alpha);
    double flag_top = ground - tower_h - tower_w * 2.2;
    thickLineRGBA(surface, (int)tx, (int)flag_top, (int)tx, (int)(flag_top - s * 0.02), 1, 20, 22,
                  30, alpha);
    double wave = sin(event->t * 4.0 + side) * s * 0.004;
    filledTrigonRGBA(surface, (int)tx, (int)(flag_top - s * 0.02), (int)tx,
                     (int)(flag_top - s * 0.008), (int)(tx + d * s * 0.016),
                     (int)(flag_top - s * 0.014 + wave), 200, 60, 70, alpha);
    boxRGBA(surface, (int)(tx - 2), (int)(ground - tower_h * 0.6), (int)(tx + 2),
            (int)(ground - tower_h * 0.6 + 6), 255, 214, 130,
            weapon_alpha(235.0 * lit * event->fade));
  }
  for (int m = 0; m < 6; m++) {
    double mx = cx - keep_w + (2.0 * keep_w) * (m + 0.5) / 6.0;
    boxRGBA(surface, (int)(mx - 3), (int)(ground - keep_h - 4), (int)(mx + 3),
            (int)(ground - keep_h), 26, 30, 44, alpha);
  }
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(keep_w * 0.22), (int)(keep_h * 0.3), 6, 8,
                    14, alpha);
  for (int win = 0; win < 3; win++) {
    boxRGBA(surface, (int)(cx - keep_w * 0.7 + win * keep_w * 0.7) - 1,
            (int)(ground - keep_h * 0.7), (int)(cx - keep_w * 0.7 + win * keep_w * 0.7) + 2,
            (int)(ground - keep_h * 0.7) + 6, 255, 214, 130,
            weapon_alpha(235.0 * lit * (0.85 + 0.15 * sin(event->t * 6.0 + win)) * event->fade));
  }
  for (int side = -1; side <= 1; side += 2) {
    double fx = cx + side * keep_w * 0.45, fy = ground - keep_h * 0.28;
    double flame = 0.8 + 0.2 * sin(event->t * 12.0 + side * 3.0);
    thickLineRGBA(surface, (int)fx, (int)(fy + 4), (int)fx, (int)(fy + 10), 2, 40, 30, 28, alpha);
    filledTrigonRGBA(surface, (int)(fx - 3), (int)(fy + 4), (int)(fx + 3), (int)(fy + 4), (int)fx,
                     (int)(fy - 5 * flame), 255, 150, 50, weapon_alpha(230.0 * lit * event->fade));
    filledCircleRGBA(surface, (int)fx, (int)fy, 10, 255, 150, 60,
                     weapon_alpha(26.0 * lit * flame * event->fade));
  }
}


// A space elevator: a thin cable climbs from a base station to the top of the
// sky, with lit climbers running along it.
static void hill_spaceelevator(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = hill_side(event, 0.17);
  double ground = hill_ground(event, cx);
  double reach = smooth_step(0.02, 0.2, event->progress);
  double top = ground - (ground + s * 0.05) * reach;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 cyan[3] = {150, 235, 255};
  thickLineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)top, 3, 120, 200, 230,
                weapon_alpha(24.0 * event->fade));
  lineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)top, 215, 240, 255,
           weapon_alpha(200.0 * event->fade));
  filledTrigonRGBA(surface, (int)(cx - s * 0.03), (int)(ground + 2), (int)(cx + s * 0.03),
                   (int)(ground + 2), (int)cx, (int)(ground - s * 0.025), 28, 34, 48, alpha);
  boxRGBA(surface, (int)(cx - s * 0.04), (int)(ground - 2), (int)(cx + s * 0.04), (int)(ground + 3),
          22, 26, 38, alpha);
  if (fmod(event->t, 1.4) < 0.4) {
    static const Uint8 red[3] = {255, 70, 60};
    weapon_glow(surface, cx, ground - s * 0.028, 2.5, red, 235.0 * event->fade);
  }
  for (int car = 0; car < 4; car++) {
    double u = fmod(event->t * (0.045 + 0.012 * car) + car * 0.27, 1.0);
    if (car % 2) u = 1.0 - u;
    double y = ground - 8.0 - u * (ground - top - 16.0);
    if (reach < 0.98 && y < top) continue;
    boxRGBA(surface, (int)(cx - 3), (int)(y - 6), (int)(cx + 3), (int)(y + 6), 58, 66, 90, alpha);
    boxRGBA(surface, (int)(cx - 1), (int)(y - 4), (int)(cx + 1), (int)(y - 1), 255, 224, 150,
            alpha);
    weapon_glow(surface, cx, y + 7.0, 2.0, cyan, 200.0 * event->fade);
  }
}


// A small airstrip: runway lights come on in a row and a little plane lands,
// taxis and lifts off again.
static void hill_airstrip(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  static const Uint8 white[3] = {255, 245, 210};
  const int lights = 12;
  double lit =
      smooth_step(0.04, 0.14, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  double x0 = surface->w * 0.12, x1 = surface->w * 0.88;
  for (int i = 0; i < lights; i++) {
    double x = x0 + (x1 - x0) * i / (lights - 1.0);
    double on = smooth_step(i / (double)lights * 0.5, i / (double)lights * 0.5 + 0.1, lit);
    weapon_glow(surface, x, hill_ground(event, x) - 1.0, 2.0, white,
                220.0 * on * event->fade * (0.75 + 0.25 * sin(event->t * 4.0 + i)));
  }
  double u = (event->progress - 0.2) / 0.6;
  if (u <= 0.0 || u >= 1.0) return;
  double run = d > 0 ? u : 1.0 - u;
  double px = x0 + (x1 - x0) * run;
  double air = u < 0.5 ? pow(1.0 - u * 2.0, 2.0) : pow((u - 0.5) * 2.0, 2.0) * 0.0;
  double py = hill_ground(event, px) - s * 0.012 - air * s * 0.18;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  filledEllipseRGBA(surface, (int)px, (int)py, (int)(s * 0.02), (int)(s * 0.006), 30, 34, 48,
                    alpha);
  thickLineRGBA(surface, (int)(px - d * s * 0.02), (int)py, (int)(px - d * s * 0.03),
                (int)(py - s * 0.01), 2, 30, 34, 48, alpha);
  thickLineRGBA(surface, (int)px, (int)(py - s * 0.004), (int)px, (int)(py + s * 0.004), 3, 60, 66,
                86, alpha);
  weapon_glow(surface, px, py - s * 0.007, 2.0, white, 200.0 * event->fade);
}


// Hot springs: pools of glowing water on the ridge breathe slow clouds of
// steam.
static void hill_hotsprings(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int pools = 4;
  double warm =
      smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  for (int i = 0; i < pools; i++) {
    double x = surface->w * (0.12 + 0.76 * (i + 0.5 * hill_random(event, i, 1)) / pools);
    double ground = hill_ground(event, x);
    double w = s * (0.03 + 0.015 * hill_random(event, i, 2));
    filledEllipseRGBA(surface, (int)x, (int)ground, (int)(w * 1.2), (int)(w * 0.2) + 1, 30, 36, 44,
                      weapon_alpha(235.0 * event->fade));
    filledEllipseRGBA(
        surface, (int)x, (int)ground, (int)w, (int)(w * 0.14) + 1, 80, 190, 210,
        weapon_alpha(190.0 * warm * event->fade * (0.8 + 0.2 * sin(event->t * 1.5 + i))));
    for (int puff = 0; puff < 9; puff++) {
      double u = fmod(event->t / 5.0 + puff / 9.0 + i * 0.17, 1.0);
      double px = x + sin(u * 4.0 + puff + i) * w * 0.5 + event->direction * u * s * 0.03;
      double py = ground - u * s * 0.16;
      filledCircleRGBA(surface, (int)px, (int)py, (int)(s * (0.006 + 0.016 * u)), 215, 225, 235,
                       weapon_alpha(70.0 * (1.0 - u) * warm * event->fade));
    }
  }
}


// A night market: stalls under striped awnings, strings of bulbs between poles,
// and a few shoppers.
static void hill_market(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int stalls = 6;
  double lit =
      smooth_step(0.05, 0.16, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  Uint8 alpha = weapon_alpha(236.0 * event->fade);
  static const Uint8 bulbs[4][3] = {
      {255, 210, 120}, {255, 140, 120}, {140, 230, 255}, {190, 255, 150}};
  double prev_x = 0, prev_y = 0;
  for (int i = 0; i < stalls; i++) {
    double x = surface->w * (0.12 + 0.76 * (i + 0.5) / stalls);
    double ground = hill_ground(event, x);
    double rise = smooth_step(0.0 + i * 0.015, 0.1 + i * 0.015, event->progress);
    double w = s * 0.036, h = s * 0.034 * rise;
    boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)ground, 24, 28, 40, alpha);
    for (int stripe = 0; stripe < 4; stripe++) {
      double sx = x - w + stripe * w * 0.5;
      filledTrigonRGBA(surface, (int)sx, (int)(ground - h), (int)(sx + w * 0.5), (int)(ground - h),
                       (int)(sx + w * 0.25), (int)(ground - h - s * 0.012), stripe % 2 ? 210 : 150,
                       stripe % 2 ? 200 : 54, stripe % 2 ? 190 : 60, alpha);
    }
    filledCircleRGBA(surface, (int)x, (int)(ground - h * 0.45), 8, 255, 190, 100,
                     weapon_alpha(34.0 * lit * event->fade));
    boxRGBA(surface, (int)(x - w * 0.5), (int)(ground - h * 0.6), (int)(x + w * 0.5),
            (int)(ground - h * 0.3), 255, 214, 130,
            weapon_alpha(220.0 * lit * event->fade * (0.88 + 0.12 * sin(event->t * 5.0 + i))));
    if (i > 0) {
      for (int k = 1; k < 6; k++) {
        double u = k / 6.0;
        double bx = prev_x + (x - prev_x) * u,
               by = prev_y + (ground - h - prev_y) * u + sin(3.14159 * u) * s * 0.012;
        weapon_glow(surface, bx, by, 1.6, bulbs[(i + k) % 4], 220.0 * lit * event->fade);
      }
    }
    prev_x = x;
    prev_y = ground - h - s * 0.012;
  }
}


// A pond of floating lotus lights: little flames drift across dark water and
// ripples spread around them.
static void hill_lotus(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * 0.5, ground = hill_ground(event, cx) + s * 0.008;
  double rw = s * 0.28, rh = s * 0.014;
  double fill =
      smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.93, 0.98, event->progress));
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(rw * fill), (int)(rh * fill) + 1, 16, 26,
                    40, weapon_alpha(220.0 * event->fade));
  ellipseRGBA(surface, (int)cx, (int)ground, (int)(rw * fill), (int)(rh * fill) + 1, 90, 130, 160,
              weapon_alpha(110.0 * event->fade));
  static const Uint8 pink[3] = {255, 170, 200};
  for (int i = 0; i < 16; i++) {
    double r1 = hill_random(event, i, 1), r2 = hill_random(event, i, 2);
    double u = fmod(event->t * (0.012 + 0.01 * r2) + r1, 1.0);
    double lx = cx - rw * 0.9 + rw * 1.8 * u;
    double ly = ground + (r2 - 0.5) * rh * 1.2;
    double vis = smooth_step(0.0, 0.1, u) * (1.0 - smooth_step(0.9, 1.0, u)) * fill;
    double ring = fmod(event->t * 0.6 + r1 * 3.0, 1.0);
    ellipseRGBA(surface, (int)lx, (int)ly, (int)(3 + ring * 10), (int)(1 + ring * 3), 190, 210, 235,
                weapon_alpha(90.0 * (1.0 - ring) * vis * event->fade));
    weapon_glow(surface, lx, ly - 2.0, 2.0, pink,
                230.0 * vis * event->fade * (0.8 + 0.2 * sin(event->t * 6.0 + i)));
  }
}


// A bonfire festival: a big fire in the middle, a ring of dancers circling it,
// sparks rising.
static void hill_bonfire(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * 0.5, ground = hill_ground(event, cx);
  double build =
      smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.9, 0.97, event->progress));
  Uint8 alpha = weapon_alpha(236.0 * event->fade);
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(s * 0.16), (int)(s * 0.02), 255, 150, 60,
                    weapon_alpha(30.0 * build * event->fade));
  double flare = 0.85 + 0.15 * sin(event->t * 9.0);
  for (int tongue = 0; tongue < 6; tongue++) {
    double h = s * (0.04 + 0.012 * (tongue % 3)) * build *
               (0.7 + 0.3 * sin(event->t * (8.0 + tongue) + tongue)) * flare;
    double fx = cx + (tongue - 2.5) * s * 0.006;
    filledTrigonRGBA(surface, (int)(fx - s * 0.01), (int)ground, (int)(fx + s * 0.01), (int)ground,
                     (int)(fx + sin(event->t * 5.0 + tongue) * 3.0), (int)(ground - h), 250, 120,
                     40, weapon_alpha(215.0 * event->fade));
    filledTrigonRGBA(surface, (int)(fx - s * 0.005), (int)ground, (int)(fx + s * 0.005),
                     (int)ground, (int)fx, (int)(ground - h * 0.7), 255, 220, 130,
                     weapon_alpha(225.0 * event->fade));
  }
  for (int spark = 0; spark < 14; spark++) {
    double u = fmod(event->t / 2.2 + spark * 0.071, 1.0);
    filledCircleRGBA(surface, (int)(cx + sin(spark * 5.0 + event->t) * s * 0.03 * u),
                     (int)(ground - s * 0.04 - u * s * 0.18), 1, 255, 210, 110,
                     weapon_alpha(240.0 * (1.0 - u) * build * event->fade));
  }
  for (int dancer = 0; dancer < 8; dancer++) {
    double a = event->t * 0.7 + dancer * SCENE_TAU / 8.0;
    double px = cx + cos(a) * s * 0.11, depth = sin(a);
    double py = ground + depth * s * 0.012;
    double arms = sin(event->t * 6.0 + dancer) * s * 0.008;
    double hgt = s * 0.03 * build;
    Uint8 shade = (Uint8)(14 + 12 * (depth + 1.0));
    thickLineRGBA(surface, (int)px, (int)py, (int)px, (int)(py - hgt * 0.6), 3, shade, shade + 4,
                  shade + 12, alpha);
    thickLineRGBA(surface, (int)(px - hgt * 0.4), (int)(py - hgt * 0.5 + arms),
                  (int)(px + hgt * 0.4), (int)(py - hgt * 0.5 - arms), 1, shade, shade + 4,
                  shade + 12, alpha);
    filledCircleRGBA(surface, (int)px, (int)(py - hgt * 0.75), (int)(hgt * 0.14) + 1, shade,
                     shade + 4, shade + 12, alpha);
  }
}


// A waterfall spills down the near ridge, with mist at the bottom and a pale
// stream running away.
static void hill_waterfall(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = hill_side(event, 0.16), ground = hill_ground(event, cx);
  double flow =
      smooth_step(0.0, 0.1, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  double h = s * 0.11, w = s * 0.014;
  Uint8 alpha = weapon_alpha(236.0 * event->fade);
  filledTrigonRGBA(surface, (int)(cx - s * 0.05), (int)(ground + 2), (int)(cx + s * 0.05),
                   (int)(ground + 2), (int)cx, (int)(ground - h * 1.15), 24, 28, 40, alpha);
  for (int strand = 0; strand < 5; strand++) {
    double sx = cx + (strand - 2) * w * 0.4;
    for (int seg = 0; seg < 10; seg++) {
      double u = fmod(event->t * (0.9 + 0.1 * strand) + seg / 10.0 + strand * 0.13, 1.0);
      double py = ground - h + u * h;
      thickLineRGBA(surface, (int)sx, (int)py, (int)sx, (int)(py + h * 0.06), 2, 190, 225, 245,
                    weapon_alpha(170.0 * flow * (0.4 + 0.6 * u) * event->fade));
    }
  }
  for (int puff = 0; puff < 8; puff++) {
    double u = fmod(event->t / 3.0 + puff / 8.0, 1.0);
    filledCircleRGBA(surface, (int)(cx + sin(puff * 2.3 + u * 4.0) * w * 2.0),
                     (int)(ground - u * s * 0.06), (int)(s * (0.01 + 0.02 * u)), 215, 228, 240,
                     weapon_alpha(70.0 * (1.0 - u) * flow * event->fade));
  }
  for (int k = 1; k < 12; k++) {
    double u = k / 12.0, rx = cx + event->direction * u * s * 0.3;
    thickLineRGBA(surface, (int)rx, (int)(hill_ground(event, rx) + 1.0), (int)(rx + 5),
                  (int)(hill_ground(event, rx + 5) + 1.0), 2, 110, 180, 210,
                  weapon_alpha(120.0 * (1.0 - u) * flow * event->fade));
  }
}


// Scarecrows stand in a field; crows land on them, flap off, and settle again.
static void hill_scarecrows(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int count = 4;
  Uint8 alpha = weapon_alpha(236.0 * event->fade);
  for (int i = 0; i < count; i++) {
    double x = surface->w * (0.1 + 0.8 * (i + 0.5) / count);
    double ground = hill_ground(event, x);
    double rise = smooth_step(0.0 + i * 0.03, 0.1 + i * 0.03, event->progress);
    double h = s * 0.05 * rise;
    thickLineRGBA(surface, (int)x, (int)ground, (int)x, (int)(ground - h), 2, 54, 40, 34, alpha);
    thickLineRGBA(surface, (int)(x - h * 0.4), (int)(ground - h * 0.75), (int)(x + h * 0.4),
                  (int)(ground - h * 0.75), 2, 54, 40, 34, alpha);
    filledCircleRGBA(surface, (int)x, (int)(ground - h * 1.0), (int)(h * 0.1) + 1, 150, 120, 80,
                     alpha);
    filledTrigonRGBA(surface, (int)(x - h * 0.16), (int)(ground - h * 1.04), (int)(x + h * 0.16),
                     (int)(ground - h * 1.04), (int)x, (int)(ground - h * 1.3), 60, 44, 36, alpha);
    for (int c = 0; c < 2; c++) {
      double cycle = 7.0 + c * 2.3 + i;
      double local = fmod(event->t + i * 3.1 + c * 1.7, cycle) / cycle;
      double bx, by;
      if (local < 0.5) {
        bx = x + (c ? 1 : -1) * h * 0.4;
        by = ground - h * 0.8;
      } else {
        double f = (local - 0.5) / 0.5;
        bx = x + (c ? 1 : -1) * (h * 0.4 + f * s * 0.3 * event->direction);
        by = ground - h * 0.8 - f * s * 0.2;
        double flap = sin(event->t * 20.0 + c) * s * 0.006;
        thickLineRGBA(surface, (int)(bx - s * 0.008), (int)(by - flap), (int)bx, (int)by, 1, 8, 10,
                      16, weapon_alpha(235.0 * (1.0 - f) * event->fade));
        thickLineRGBA(surface, (int)bx, (int)by, (int)(bx + s * 0.008), (int)(by - flap), 1, 8, 10,
                      16, weapon_alpha(235.0 * (1.0 - f) * event->fade));
      }
      if (local < 0.5)
        filledEllipseRGBA(surface, (int)bx, (int)by, (int)(s * 0.006) + 1, (int)(s * 0.004) + 1, 8,
                          10, 16, alpha);
    }
  }
}


// A tiered pagoda rises on the ridge, floor by floor, its eaves hung with
// swaying lanterns.
static void hill_pagoda(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = hill_side(event, 0.13), ground = hill_ground(event, cx);
  const int tiers = 5;
  double lit =
      smooth_step(0.3, 0.42, event->progress) * (1.0 - smooth_step(0.92, 0.98, event->progress));
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double tier_h = s * 0.024;
  for (int t = 0; t < tiers; t++) {
    double rise = smooth_step(0.0 + t * 0.05, 0.08 + t * 0.05, event->progress);
    if (rise <= 0.0) break;
    double w = s * (0.04 - t * 0.006), y0 = ground - t * tier_h * 1.4, h = tier_h * rise;
    boxRGBA(surface, (int)(cx - w * 0.6), (int)(y0 - h), (int)(cx + w * 0.6), (int)y0, 70, 36, 40,
            alpha);
    filledTrigonRGBA(surface, (int)(cx - w * 1.2), (int)(y0 - h), (int)(cx + w * 1.2),
                     (int)(y0 - h), (int)cx, (int)(y0 - h - tier_h * 0.7 * rise), 28, 32, 44,
                     alpha);
    boxRGBA(surface, (int)(cx - 2), (int)(y0 - h * 0.7), (int)(cx + 2), (int)(y0 - h * 0.25), 255,
            214, 130, weapon_alpha(230.0 * lit * event->fade));
    for (int side = -1; side <= 1; side += 2) {
      double lx = cx + side * w * 1.1, sway = sin(event->t * 1.8 + t + side) * 1.5;
      static const Uint8 red[3] = {255, 120, 90};
      weapon_glow(surface, lx + sway, y0 - h + 4.0, 1.8, red, 220.0 * lit * event->fade);
    }
  }
  double top = ground - tiers * tier_h * 1.4;
  thickLineRGBA(surface, (int)cx, (int)(top + tier_h * 0.5), (int)cx, (int)(top - tier_h * 0.8), 1,
                28, 32, 44,
                weapon_alpha(235.0 * event->fade * smooth_step(0.2, 0.3, event->progress)));
}


// A frozen pond: figures glide in loops, leaving pale curved tracks on the ice.
static void hill_skaters(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * 0.5, ground = hill_ground(event, cx) + s * 0.006;
  double rw = s * 0.26, rh = s * 0.016;
  double freeze =
      smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.93, 0.98, event->progress));
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(rw * freeze), (int)(rh * freeze) + 1, 150,
                    190, 215, weapon_alpha(90.0 * event->fade));
  ellipseRGBA(surface, (int)cx, (int)ground, (int)(rw * freeze), (int)(rh * freeze) + 1, 200, 230,
              245, weapon_alpha(150.0 * event->fade));
  Uint8 alpha = weapon_alpha(236.0 * event->fade * freeze);
  for (int i = 0; i < 3; i++) {
    double a = event->t * (0.5 + 0.15 * i) + i * 2.1;
    double px = cx + cos(a) * rw * 0.7, py = ground + sin(a) * rh * 0.6;
    for (int k = 1; k <= 12; k++) {
      double ta = a - k * 0.07;
      filledCircleRGBA(surface, (int)(cx + cos(ta) * rw * 0.7), (int)(ground + sin(ta) * rh * 0.6),
                       1, 230, 242, 250,
                       weapon_alpha(120.0 * (1.0 - k / 13.0) * freeze * event->fade));
    }
    double lean = cos(a) * s * 0.006;
    thickLineRGBA(surface, (int)px, (int)py, (int)(px + lean), (int)(py - s * 0.024), 3, 18, 22, 32,
                  alpha);
    filledCircleRGBA(surface, (int)(px + lean), (int)(py - s * 0.03), (int)(s * 0.005) + 1, 18, 22,
                     32, alpha);
    thickLineRGBA(surface, (int)(px - s * 0.006), (int)py, (int)(px + s * 0.006), (int)py, 1, 220,
                  235, 245, alpha);
  }
}


// A flock of sheep is walked along the ridge by a shepherd with a lantern and a
// small dog.
static void hill_shepherd(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double margin = s * 0.1, travel = surface->w + 2.0 * margin + s * 0.4;
  double lead = event->progress * travel - margin;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int i = 0; i < 9; i++) {
    double off = s * 0.04 * (i % 3) + hill_random(event, i, 1) * s * 0.05 + (i / 3) * s * 0.045;
    double px = d > 0 ? lead - off : surface->w - lead + off;
    if (px < -30 || px > surface->w + 30) continue;
    double ground = hill_ground(event, px);
    double bob = fabs(sin(event->t * 3.0 + i)) * s * 0.002;
    double sz = s * 0.02;
    for (int leg = 0; leg < 2; leg++) {
      double lx = px + (leg ? 0.4 : -0.4) * sz;
      thickLineRGBA(surface, (int)lx, (int)(ground - sz * 0.4),
                    (int)(lx + sin(event->t * 6.0 + leg + i) * sz * 0.15), (int)ground, 1, 24, 28,
                    38, alpha);
    }
    filledEllipseRGBA(surface, (int)px, (int)(ground - sz * 0.65 - bob), (int)(sz * 0.7),
                      (int)(sz * 0.45), 150, 156, 170, alpha);
    filledCircleRGBA(surface, (int)(px + d * sz * 0.7), (int)(ground - sz * 0.65 - bob),
                     (int)(sz * 0.22) + 1, 30, 34, 46, alpha);
  }
  double sx = d > 0 ? lead + s * 0.05 : surface->w - lead - s * 0.05;
  double sground = hill_ground(event, sx);
  hill_walker(surface, sx, sground, s * 0.042, d, event->t * 5.0, true, event->fade, event->t, 0);
  double dx = sx - d * s * 0.05 + sin(event->t * 3.0) * s * 0.02;
  filledEllipseRGBA(surface, (int)dx, (int)(hill_ground(event, dx) - s * 0.008), (int)(s * 0.01),
                    (int)(s * 0.006), 14, 18, 26, alpha);
}


// An arched stone bridge over a stream; a lantern-carrying traveller crosses
// and the water glints beneath.
static void hill_bridge(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = surface->w * 0.5, span = s * 0.2;
  double ground = hill_ground(event, cx);
  double rise =
      smooth_step(0.0, 0.12, event->progress) * (1.0 - smooth_step(0.94, 0.98, event->progress));
  Uint8 alpha = weapon_alpha(236.0 * event->fade);
  filledEllipseRGBA(surface, (int)cx, (int)(ground + s * 0.006), (int)(span * 1.2 * rise),
                    (int)(s * 0.008 * rise) + 1, 40, 80, 110, weapon_alpha(200.0 * event->fade));
  for (int k = 0; k < 14; k++) {
    double gx = cx - span * 1.1 + fmod(k * 37.0 + event->t * 6.0, span * 2.2);
    pixelRGBA(surface, (int)gx, (int)(ground + s * 0.006 + sin(k * 3.0) * 2.0), 190, 225, 245,
              weapon_alpha(160.0 * rise * event->fade));
  }
  for (int i = 0; i < 24; i++) {
    double u0 = -1.0 + 2.0 * i / 24.0, u1 = -1.0 + 2.0 * (i + 1) / 24.0;
    double y0 = ground - s * 0.03 * rise * (1.0 - u0 * u0),
           y1 = ground - s * 0.03 * rise * (1.0 - u1 * u1);
    thickLineRGBA(surface, (int)(cx + u0 * span), (int)y0, (int)(cx + u1 * span), (int)y1, 4, 52,
                  56, 72, alpha);
  }
  for (int i = 0; i <= 8; i++) {
    double u = -1.0 + 2.0 * i / 8.0;
    lineRGBA(surface, (int)(cx + u * span), (int)(ground - s * 0.03 * rise * (1.0 - u * u)),
             (int)(cx + u * span), (int)(ground - s * 0.03 * rise * (1.0 - u * u) - s * 0.008), 30,
             34, 48, alpha);
  }
  double u = (event->progress - 0.25) / 0.5;
  if (u > 0.0 && u < 1.0) {
    double px = cx + d * (-1.2 + 2.4 * u) * span;
    double nu = (px - cx) / span;
    double py = ground - s * 0.03 * (1.0 - fmin(1.0, nu * nu));
    hill_walker(surface, px, py, s * 0.036, d, event->t * 5.0, true, event->fade, event->t, 0);
  }
}


// Carved totem poles stand in a row; their eyes open one after another and
// glow.
static void hill_totems(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int poles = 5;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 eye[3] = {255, 190, 90};
  for (int i = 0; i < poles; i++) {
    int order = event->direction > 0 ? i : poles - 1 - i;
    double x = surface->w * (0.1 + 0.8 * (i + 0.5) / poles);
    double ground = hill_ground(event, x);
    double rise = smooth_step(0.0 + i * 0.03, 0.1 + i * 0.03, event->progress);
    double h = s * (0.07 + 0.015 * (i % 3)) * rise, w = s * 0.012;
    double awake = smooth_step(0.2 + order * 0.08, 0.28 + order * 0.08, event->progress) *
                   (1.0 - smooth_step(0.9, 0.97, event->progress));
    boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)ground, 54, 40, 40, alpha);
    boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x - w * 0.4), (int)ground, 74, 54, 50,
            alpha);
    for (int face = 0; face < 3; face++) {
      double fy = ground - h * (0.2 + 0.28 * face);
      boxRGBA(surface, (int)(x - w), (int)(fy - w * 0.15), (int)(x + w), (int)(fy + w * 0.15), 30,
              22, 24, alpha);
      for (int side = -1; side <= 1; side += 2) {
        weapon_glow(surface, x + side * w * 0.45, fy - w * 0.5, 1.4, eye,
                    235.0 * awake * event->fade * (0.8 + 0.2 * sin(event->t * 3.0 + i + face)));
      }
    }
    filledTrigonRGBA(surface, (int)(x - w * 1.3), (int)(ground - h), (int)(x + w * 1.3),
                     (int)(ground - h), (int)x, (int)(ground - h - w * 1.6), 100, 44, 46, alpha);
  }
}


// Snowmen build themselves ball by ball, then wave their twig arms as snow
// drifts down around them.
static void hill_snowmen(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int count = 4;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int i = 0; i < count; i++) {
    double x = surface->w * (0.12 + 0.76 * (i + 0.5) / count);
    double ground = hill_ground(event, x);
    double r = s * (0.016 + 0.003 * (i % 2));
    double b1 = smooth_step(0.02 + i * 0.05, 0.08 + i * 0.05, event->progress);
    double b2 = smooth_step(0.08 + i * 0.05, 0.14 + i * 0.05, event->progress);
    double b3 = smooth_step(0.14 + i * 0.05, 0.2 + i * 0.05, event->progress);
    filledCircleRGBA(surface, (int)x, (int)(ground - r * b1), (int)(r * b1) + 1, 224, 234, 244,
                     alpha);
    filledCircleRGBA(surface, (int)x, (int)(ground - r * 2.0 - r * 0.7 * b2),
                     (int)(r * 0.75 * b2) + 1, 232, 240, 248, alpha);
    double hy = ground - r * 2.0 - r * 1.4 - r * 0.5 * b3;
    filledCircleRGBA(surface, (int)x, (int)hy, (int)(r * 0.55 * b3) + 1, 238, 244, 250, alpha);
    if (b3 < 0.9) continue;
    filledTrigonRGBA(surface, (int)(x + 1), (int)hy, (int)(x + r * 0.9), (int)(hy + 1), (int)x,
                     (int)(hy + 2), 230, 120, 50, alpha);
    pixelRGBA(surface, (int)(x - r * 0.2), (int)(hy - r * 0.15), 20, 24, 32, alpha);
    pixelRGBA(surface, (int)(x + r * 0.2), (int)(hy - r * 0.15), 20, 24, 32, alpha);
    double wave = sin(event->t * 4.0 + i * 1.3) * r * 0.5;
    double sy = ground - r * 2.8;
    thickLineRGBA(surface, (int)(x - r * 0.6), (int)sy, (int)(x - r * 1.5),
                  (int)(sy - r * 0.2 + wave), 1, 70, 50, 40, alpha);
    thickLineRGBA(surface, (int)(x + r * 0.6), (int)sy, (int)(x + r * 1.5),
                  (int)(sy - r * 0.2 - wave), 1, 70, 50, 40, alpha);
  }
  for (int f = 0; f < 36; f++) {
    double r1 = hill_random(event, f, 1), r2 = hill_random(event, f, 2);
    double u = fmod(event->t * (0.12 + 0.08 * r2) + r1, 1.0);
    double fx = surface->w * r1 + sin(event->t + f) * s * 0.01,
           fy = surface->h * 0.55 + u * (surface->h * 0.4);
    filledCircleRGBA(surface, (int)fx, (int)fy, 1, 225, 235, 245,
                     weapon_alpha(200.0 * (1.0 - u * 0.4) * event->fade));
  }
}


// A row of sunflowers grows up and every head slowly turns to follow the sun
// across the sky.
static void hill_sunflowers(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int count = 9;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int i = 0; i < count; i++) {
    double x = surface->w * (0.08 + 0.84 * (i + 0.5) / count) +
               (hill_random(event, i, 1) - 0.5) * s * 0.02;
    double ground = hill_ground(event, x);
    double grow = smooth_step(0.0 + i * 0.015, 0.12 + i * 0.015, event->progress);
    double h = s * (0.055 + 0.02 * hill_random(event, i, 2)) * grow;
    double face = (event->progress - 0.5) * event->direction * 0.9;
    double sway = sin(event->t * 1.3 + i) * h * 0.03;
    double hx = x + sway + sin(face) * h * 0.12, hy = ground - h;
    double r = s * 0.011 * grow;
    thickLineRGBA(surface, (int)x, (int)ground, (int)hx, (int)hy, 2, 40, 86, 44, alpha);
    filledEllipseRGBA(surface, (int)(x + (hx - x) * 0.4 - r * 1.2), (int)(ground - h * 0.4),
                      (int)(r * 1.1) + 1, (int)(r * 0.5) + 1, 52, 112, 56, alpha);
    for (int petal = 0; petal < 12; petal++) {
      double a = petal * 0.5236 + 0.26;
      double px = hx + cos(a) * r * 1.5, py = hy + sin(a) * r * 1.5 * (0.55 + 0.45 * cos(face));
      filledEllipseRGBA(surface, (int)px, (int)py, (int)(r * 0.5) + 1, (int)(r * 0.5) + 1, 244, 196,
                        40, alpha);
    }
    filledEllipseRGBA(surface, (int)(hx + sin(face) * r * 0.3), (int)hy, (int)(r * 0.95) + 1,
                      (int)(r * 0.95 * (0.6 + 0.4 * cos(face))) + 1, 84, 54, 30, alpha);
  }
}


// Crystals push up out of the hillside, glowing in slow shifting colours, and
// sparkle at their tips.
static void hill_crystals(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int clusters = 6;
  for (int c = 0; c < clusters; c++) {
    double cx = surface->w * (0.1 + 0.8 * (c + 0.5) / clusters);
    double ground = hill_ground(event, cx);
    for (int k = 0; k < 5; k++) {
      double start = 0.03 + c * 0.04 + k * 0.01;
      double grow = smooth_step(start, start + 0.1, event->progress);
      if (grow <= 0.0) continue;
      double lean = (k - 2) * 0.28;
      double h = s * (0.03 + 0.04 * hill_random(event, c * 5 + k, 3)) * grow;
      double w = s * 0.007;
      double bx = cx + (k - 2) * s * 0.008;
      double tx = bx + sin(lean) * h, ty = ground - cos(lean) * h;
      double hue = event->t * 0.35 + c * 0.9 + k * 0.4;
      Uint8 r = (Uint8)(120 + 100 * sin(hue)), g = (Uint8)(150 + 90 * sin(hue + 2.1)),
            b = (Uint8)(200 + 50 * sin(hue + 4.2));
      Uint8 body = weapon_alpha(170.0 * event->fade);
      filledTrigonRGBA(surface, (int)(bx - w), (int)ground, (int)(bx + w), (int)ground, (int)tx,
                       (int)ty, r, g, b, body);
      filledTrigonRGBA(surface, (int)(bx - w * 0.2), (int)ground, (int)(bx + w), (int)ground,
                       (int)tx, (int)ty, 235, 240, 255, weapon_alpha(70.0 * event->fade));
      Uint8 tint[3] = {r, g, b};
      weapon_glow(surface, bx, ground - h * 0.4, w * 3.0, tint, 90.0 * grow * event->fade);
      double twinkle = sin(event->t * 3.0 + c * 2.0 + k * 1.7);
      if (twinkle > 0.85 && grow > 0.9) {
        Uint8 white[3] = {255, 255, 255};
        weapon_glow(surface, tx, ty, 2.5, white, 230.0 * event->fade * (twinkle - 0.85) / 0.15);
      }
    }
  }
}


// Giant glowing mushrooms bloom on the hill and puff drifting spores into the
// air.
static void hill_mushrooms(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int count = 7;
  Uint8 alpha = weapon_alpha(235.0 * event->fade);
  for (int i = 0; i < count; i++) {
    double x = surface->w * (0.1 + 0.8 * (i + 0.5) / count);
    double ground = hill_ground(event, x);
    double grow = smooth_step(0.02 + i * 0.03, 0.14 + i * 0.03, event->progress);
    double h = s * (0.03 + 0.03 * hill_random(event, i, 4)) * grow;
    double cap = h * 0.9;
    double pulse = 0.7 + 0.3 * sin(event->t * 1.6 + i * 1.1);
    Uint8 glow[3] = {(Uint8)(110 + 90 * (i % 2)), (Uint8)(200 - 60 * (i % 3)), 255};
    boxRGBA(surface, (int)(x - h * 0.12), (int)(ground - h), (int)(x + h * 0.12), (int)ground, 214,
            206, 222, alpha);
    weapon_glow(surface, x, ground - h, cap * 0.6, glow, 120.0 * pulse * grow * event->fade);
    filledEllipseRGBA(surface, (int)x, (int)(ground - h), (int)cap + 1, (int)(cap * 0.55) + 1,
                      glow[0] / 2 + 40, glow[1] / 2 + 30, glow[2] / 2 + 40, alpha);
    for (int dot = 0; dot < 4; dot++) {
      double dx = (dot - 1.5) * cap * 0.4;
      filledCircleRGBA(
          surface, (int)(x + dx), (int)(ground - h - cap * 0.2 * (1.0 - fabs(dx) / cap)),
          (int)(cap * 0.12) + 1, 235, 245, 255, weapon_alpha(210.0 * pulse * event->fade));
    }
    for (int spore = 0; spore < 5; spore++) {
      double u = fmod(event->t * (0.08 + 0.04 * hill_random(event, i * 5 + spore, 5)) +
                          hill_random(event, i * 5 + spore, 6),
                      1.0);
      double sx = x + sin(event->t * 0.8 + spore + i) * cap * (0.5 + u),
             sy = ground - h - u * s * 0.12;
      filledCircleRGBA(surface, (int)sx, (int)sy, 1, glow[0], glow[1], glow[2],
                       weapon_alpha(200.0 * (1.0 - u) * grow * event->fade));
    }
  }
}


// Three tents pitch themselves around a camp; each lights up from inside and a
// plume of smoke rises between them.
static void hill_tents(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  static const Uint8 cloth[3][3] = {{150, 78, 60}, {70, 100, 140}, {96, 120, 78}};
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 lamp[3] = {255, 205, 120};
  double mid = surface->w * (0.35 + 0.3 * hill_random(event, 0, 7));
  for (int i = 0; i < 3; i++) {
    double x = mid + (i - 1) * s * 0.17;
    double ground = hill_ground(event, x);
    double up = smooth_step(0.02 + i * 0.05, 0.1 + i * 0.05, event->progress);
    double w = s * 0.045 * up, h = s * 0.05 * up;
    filledTrigonRGBA(surface, (int)(x - w), (int)ground, (int)(x + w), (int)ground, (int)x,
                     (int)(ground - h), cloth[i][0], cloth[i][1], cloth[i][2], alpha);
    filledTrigonRGBA(surface, (int)(x - w * 0.25), (int)ground, (int)(x + w * 0.25), (int)ground,
                     (int)x, (int)(ground - h * 0.7), 20, 18, 24, alpha);
    double lit = smooth_step(0.2 + i * 0.06, 0.3 + i * 0.06, event->progress) *
                 (0.85 + 0.15 * sin(event->t * 5.0 + i));
    filledTrigonRGBA(surface, (int)(x - w * 0.25), (int)ground, (int)(x + w * 0.25), (int)ground,
                     (int)x, (int)(ground - h * 0.7), lamp[0], lamp[1], lamp[2],
                     weapon_alpha(235.0 * lit * event->fade));
    weapon_glow(surface, x, ground - h * 0.3, w * 0.3, lamp, 110.0 * lit * event->fade);
    thickLineRGBA(surface, (int)x, (int)(ground - h), (int)x, (int)(ground - h - s * 0.01), 1, 60,
                  50, 44, alpha);
  }
  double fx = mid, fg = hill_ground(event, fx);
  double fire = smooth_step(0.15, 0.25, event->progress);
  weapon_glow(surface, fx, fg - s * 0.006, 2.5, lamp,
              200.0 * fire * event->fade * (0.8 + 0.2 * sin(event->t * 9.0)));
  for (int p = 0; p < 12; p++) {
    double u = fmod(event->t * 0.2 + p / 12.0, 1.0);
    double px = fx + sin(event->t * 1.4 + p * 0.8) * s * 0.01 * u * 3.0, py = fg - u * s * 0.17;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(2 + u * s * 0.008), 130, 132, 142,
                     weapon_alpha(110.0 * (1.0 - u) * fire * event->fade));
  }
}


// A tiered fountain on the hill throws jets of water in slow shifting colours
// that fall back into the basin.
static void hill_fountain(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 7));
  double ground = hill_ground(event, cx);
  double rise = smooth_step(0.0, 0.1, event->progress);
  Uint8 alpha = weapon_alpha(235.0 * event->fade);
  double bw = s * 0.07, bh = s * 0.014;
  filledEllipseRGBA(surface, (int)cx, (int)(ground - bh * 0.4), (int)bw, (int)bh + 1, 70, 76, 92,
                    alpha);
  filledEllipseRGBA(surface, (int)cx, (int)(ground - bh * 0.7), (int)(bw * 0.88),
                    (int)(bh * 0.7) + 1, 40, 86, 120, alpha);
  boxRGBA(surface, (int)(cx - s * 0.006), (int)(ground - s * 0.05 * rise), (int)(cx + s * 0.006),
          (int)(ground - bh), 84, 90, 106, alpha);
  filledEllipseRGBA(surface, (int)cx, (int)(ground - s * 0.05 * rise), (int)(s * 0.02),
                    (int)(s * 0.006) + 1, 84, 90, 106, alpha);
  double power = smooth_step(0.1, 0.2, event->progress);
  for (int jet = -4; jet <= 4; jet++) {
    double vx = jet * 0.075 * s, vy = (0.2 - 0.014 * abs(jet)) * s;
    double hue = event->t * 0.4 + jet * 0.35;
    Uint8 r = (Uint8)(150 + 90 * sin(hue)), g = (Uint8)(190 + 60 * sin(hue + 2.1)),
          b = (Uint8)(235 + 20 * sin(hue + 4.2));
    for (int d = 0; d < 14; d++) {
      double u = fmod(event->t * 0.55 + d / 14.0 + jet * 0.03, 1.0) * 0.8;
      double px = cx + vx * u,
             py = ground - s * 0.05 * rise - (vy * u - 0.5 * s * 0.5 * u * u) * power;
      if (py > ground - bh * 0.5) continue;
      filledCircleRGBA(surface, (int)px, (int)py, 1 + (d % 3 == 0), r, g, b,
                       weapon_alpha(210.0 * (1.0 - u * 0.6) * power * event->fade));
    }
  }
  for (int ring = 0; ring < 3; ring++) {
    double u = fmod(event->t * 0.4 + ring / 3.0, 1.0);
    ellipseRGBA(surface, (int)cx, (int)(ground - bh * 0.7), (int)(bw * 0.8 * u) + 1,
                (int)(bh * 0.6 * u) + 1, 210, 230, 250,
                weapon_alpha(150.0 * (1.0 - u) * power * event->fade));
  }
}


// A harvester crawls along the ridge in the dark, its headlights lighting the
// crop and dust trailing behind.
static void hill_harvester(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double u = smooth_step(0.0, 1.0, event->progress);
  double x = d > 0 ? surface->w * (-0.1 + 1.2 * u) : surface->w * (1.1 - 1.2 * u);
  double ground = hill_ground(event, x);
  double tilt = (hill_ground(event, x + 6.0) - hill_ground(event, x - 6.0)) / 12.0;
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double w = s * 0.07, h = s * 0.03;
  static const Uint8 lamp[3] = {255, 238, 170};
  double wheel = s * 0.012;
  double base = ground - wheel;
  filledTrigonRGBA(surface, (int)(x + d * w * 0.5), (int)(base - h * 0.5), (int)(x + d * w * 3.2),
                   (int)(base + s * 0.012 + tilt * w * 3), (int)(x + d * w * 3.2),
                   (int)(base - s * 0.04 + tilt * w * 3), 255, 238, 170,
                   weapon_alpha(36.0 * event->fade));
  boxRGBA(surface, (int)(x - w * 0.5), (int)(base - h * 0.9), (int)(x + w * 0.5), (int)base, 176,
          60, 40, alpha);
  boxRGBA(surface, (int)(x + d * w * 0.05), (int)(base - h * 1.6), (int)(x + d * w * 0.4),
          (int)(base - h * 0.9), 60, 70, 86, alpha);
  boxRGBA(surface, (int)(x + d * w * 0.1), (int)(base - h * 1.5), (int)(x + d * w * 0.34),
          (int)(base - h * 1.0), 255, 214, 120, weapon_alpha(200.0 * event->fade));
  boxRGBA(surface, (int)(x + d * w * 0.5), (int)(base - h * 0.45), (int)(x + d * w * 0.8),
          (int)(base + wheel * 0.3), 150, 160, 70, alpha);
  for (int i = 0; i < 3; i++) {
    double wx = x + (i - 1) * w * 0.38;
    filledCircleRGBA(surface, (int)wx, (int)(ground - wheel * 0.9), (int)wheel, 20, 22, 28, alpha);
    double a = event->t * 6.0 * d + i;
    lineRGBA(surface, (int)wx, (int)(ground - wheel * 0.9), (int)(wx + cos(a) * wheel * 0.8),
             (int)(ground - wheel * 0.9 + sin(a) * wheel * 0.8), 120, 124, 130, alpha);
  }
  weapon_glow(surface, x + d * w * 0.62, base - h * 0.6, 2.2, lamp, 230.0 * event->fade);
  for (int p = 0; p < 14; p++) {
    double age = fmod(event->t * 0.8 + p / 14.0, 1.0);
    double px = x - d * (w * 0.6 + age * s * 0.12),
           py = ground - age * s * 0.03 - hill_random(event, p, 1) * s * 0.01;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(2 + age * s * 0.01), 150, 130, 100,
                     weapon_alpha(90.0 * (1.0 - age) * event->fade));
  }
}


// Small stone cairns stack themselves stone by stone; the top one of each
// lights up when it is done.
static void hill_cairns(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int count = 6;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 rune[3] = {130, 220, 255};
  for (int i = 0; i < count; i++) {
    double x =
        surface->w * (0.1 + 0.8 * (i + 0.5) / count) + (hill_random(event, i, 1) - 0.5) * s * 0.03;
    double ground = hill_ground(event, x);
    double y = ground;
    int stones = 4 + i % 3;
    double r = s * 0.014;
    for (int k = 0; k < stones; k++) {
      double drop =
          smooth_step(0.02 + i * 0.03 + k * 0.05, 0.06 + i * 0.03 + k * 0.05, event->progress);
      if (drop <= 0.0) break;
      double rx = r * (1.0 - 0.14 * k), ry = rx * 0.5;
      double sx = x + (hill_random(event, i * 9 + k, 2) - 0.5) * r * 0.5;
      double sy = y - ry - (1.0 - drop) * s * 0.05;
      Uint8 shade = (Uint8)(130 + 14 * (k % 3));
      filledEllipseRGBA(surface, (int)sx, (int)sy, (int)rx + 1, (int)ry + 1, shade, shade - 4,
                        shade + 6, alpha);
      filledEllipseRGBA(surface, (int)(sx - rx * 0.2), (int)(sy - ry * 0.3), (int)(rx * 0.6) + 1,
                        (int)(ry * 0.4) + 1, shade + 36, shade + 32, shade + 40,
                        weapon_alpha(150.0 * event->fade));
      if (drop >= 1.0) y -= ry * 1.7;
      if (k == stones - 1 && drop >= 1.0) {
        double lit = smooth_step(0.5, 0.6, event->progress) * (0.7 + 0.3 * sin(event->t * 2.0 + i));
        weapon_glow(surface, sx, sy - ry - 3.0, 2.0, rune, 200.0 * lit * event->fade);
      }
    }
  }
}


// A little watermill beside a stream: the wheel turns, its paddles drip, and
// the window is lit.
static void hill_watermill(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.4 + 0.2 * hill_random(event, 0, 8));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double up = smooth_step(0.0, 0.12, event->progress);
  double bw = s * 0.055, bh = s * 0.05 * up;
  static const Uint8 lamp[3] = {255, 205, 120};
  boxRGBA(surface, (int)(cx - bw), (int)(ground - bh), (int)(cx + bw), (int)ground, 112, 84, 64,
          alpha);
  filledTrigonRGBA(surface, (int)(cx - bw * 1.2), (int)(ground - bh), (int)(cx + bw * 1.2),
                   (int)(ground - bh), (int)cx, (int)(ground - bh - s * 0.035 * up), 74, 50, 46,
                   alpha);
  boxRGBA(surface, (int)(cx - bw * 0.5), (int)(ground - bh * 0.7), (int)(cx - bw * 0.1),
          (int)(ground - bh * 0.35), lamp[0], lamp[1], lamp[2],
          weapon_alpha(220.0 * smooth_step(0.15, 0.25, event->progress) * event->fade));
  double wx = cx + bw * 1.15, wr = s * 0.038 * up, wy = ground - wr * 0.8;
  filledEllipseRGBA(surface, (int)(cx + bw * 0.6), (int)(ground + 1), (int)(bw * 1.5),
                    (int)(s * 0.006) + 1, 60, 100, 150, weapon_alpha(200.0 * event->fade));
  for (int k = 0; k < 8; k++) {
    double a = event->t * 0.8 + k * 0.7854;
    double ex = wx + cos(a) * wr, ey = wy + sin(a) * wr;
    thickLineRGBA(surface, (int)wx, (int)wy, (int)ex, (int)ey, 1, 90, 66, 52, alpha);
    thickLineRGBA(surface, (int)(ex - sin(a) * 3), (int)(ey + cos(a) * 3), (int)(ex + sin(a) * 3),
                  (int)(ey - cos(a) * 3), 2, 130, 98, 74, alpha);
    if (ey > wy + wr * 0.2 && ey < wy + wr * 0.9) {
      filledCircleRGBA(surface, (int)ex, (int)(ey + 4 + 2 * sin(event->t * 9.0 + k)), 1, 170, 210,
                       245, weapon_alpha(180.0 * event->fade));
    }
  }
  circleRGBA(surface, (int)wx, (int)wy, (int)wr, 110, 84, 66, alpha);
  filledCircleRGBA(surface, (int)wx, (int)wy, 2, 60, 46, 40, alpha);
  for (int seg = 0; seg < 10; seg++) {
    double u = fmod(event->t * 0.3 + seg / 10.0, 1.0);
    double px = cx - bw * 0.2 + u * s * 0.3 * event->direction,
           py = ground + 1 + sin(event->t * 3.0 + seg) * 1.0;
    filledCircleRGBA(surface, (int)px, (int)py, 1, 190, 220, 245,
                     weapon_alpha(160.0 * (1.0 - u) * event->fade));
  }
}


// A reedy pond: cattails sway, ripples spread from rising fish, and two frog
// eyes glint at the edge.
static void hill_marsh(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.35 + 0.3 * hill_random(event, 0, 9));
  double ground = hill_ground(event, cx);
  double pw = s * 0.2, ph = s * 0.025;
  Uint8 alpha = weapon_alpha(235.0 * event->fade);
  static const Uint8 eye[3] = {220, 255, 140};
  double fill = smooth_step(0.0, 0.1, event->progress);
  filledEllipseRGBA(surface, (int)cx, (int)(ground - ph * 0.3), (int)(pw * fill) + 1,
                    (int)(ph * fill) + 1, 26, 44, 66, weapon_alpha(225.0 * event->fade));
  ellipseRGBA(surface, (int)cx, (int)(ground - ph * 0.3), (int)(pw * fill) + 1,
              (int)(ph * fill) + 1, 90, 120, 150, weapon_alpha(120.0 * event->fade));
  for (int k = 0; k < 4; k++) {
    double u = fmod(event->t * 0.22 + k * 0.25 + hill_random(event, k, 3), 1.0);
    double rx = cx + (hill_random(event, k, 4) - 0.5) * pw * 1.2,
           ry = ground - ph * 0.3 + (hill_random(event, k, 5) - 0.5) * ph;
    ellipseRGBA(surface, (int)rx, (int)ry, (int)(u * s * 0.04) + 1, (int)(u * s * 0.012) + 1, 190,
                215, 240, weapon_alpha(170.0 * (1.0 - u) * fill * event->fade));
  }
  for (int r = 0; r < 26; r++) {
    double side = r < 13 ? -1.0 : 1.0;
    double rx = cx + side * (pw * (0.78 + 0.28 * hill_random(event, r, 6))) +
                (hill_random(event, r, 7) - 0.5) * s * 0.03;
    double base = hill_ground(event, rx) + 1.0;
    double h = s * (0.035 + 0.04 * hill_random(event, r, 8)) *
               smooth_step(0.02 + r * 0.004, 0.14 + r * 0.004, event->progress);
    double sway = sin(event->t * 1.1 + r * 0.7) * h * 0.08;
    thickLineRGBA(surface, (int)rx, (int)base, (int)(rx + sway), (int)(base - h), 1, 52, 86, 52,
                  alpha);
    if (r % 2 == 0)
      thickLineRGBA(surface, (int)(rx + sway), (int)(base - h), (int)(rx + sway),
                    (int)(base - h - s * 0.012), 3, 100, 62, 40, alpha);
  }
  double blink = fmod(event->t, 4.0) > 3.6 ? 0.0 : 1.0;
  double ex = cx + pw * 0.55, ey = ground - ph * 0.9;
  weapon_glow(surface, ex, ey, 1.2, eye, 200.0 * blink * fill * event->fade);
  weapon_glow(surface, ex + 5, ey, 1.2, eye, 200.0 * blink * fill * event->fade);
}


// Three stargazers lie on the hill beside a telescope while shooting stars
// streak across the sky above them.
static void hill_stargazers(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 10));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double fin = smooth_step(0.0, 0.08, event->progress);
  double tx = cx + s * 0.05, th = s * 0.04 * fin;
  thickLineRGBA(surface, (int)tx, (int)(ground - th), (int)(tx - s * 0.015), (int)ground, 1, 120,
                120, 130, alpha);
  thickLineRGBA(surface, (int)tx, (int)(ground - th), (int)(tx + s * 0.015), (int)ground, 1, 120,
                120, 130, alpha);
  thickLineRGBA(surface, (int)tx, (int)(ground - th), (int)(tx - event->direction * s * 0.03),
                (int)(ground - th - s * 0.03), 4, 170, 176, 190, alpha);
  for (int p = 0; p < 3; p++) {
    double px = cx - p * s * 0.028, py = ground - s * 0.003;
    filledEllipseRGBA(surface, (int)px, (int)py, (int)(s * 0.014) + 1, (int)(s * 0.005) + 1,
                      190 - p * 30, 110 + p * 40, 100 + p * 50, alpha);
    filledCircleRGBA(surface, (int)(px + s * 0.015), (int)(py - s * 0.002), (int)(s * 0.005) + 1,
                     232, 196, 164, alpha);
  }
  int slot = (int)(event->t / 2.2);
  double age = event->t - slot * 2.2;
  if (age < 0.8) {
    double u = age / 0.8;
    double sx0 = surface->w * (0.1 + 0.8 * hill_random(event, slot, 11)),
           sy0 = surface->h * (0.04 + 0.12 * hill_random(event, slot, 12));
    double len = s * 0.3, dx = event->direction * len * u, dy = len * 0.35 * u;
    for (int k = 0; k < 14; k++) {
      double f = k / 14.0;
      double px = sx0 + dx - event->direction * len * 0.5 * f * (u > 0.5 ? 1.0 : u * 2.0),
             py = sy0 + dy - len * 0.175 * f * (u > 0.5 ? 1.0 : u * 2.0);
      filledCircleRGBA(surface, (int)px, (int)py, k < 3 ? 2 : 1, 235, 240, 255,
                       weapon_alpha(230.0 * (1.0 - f) * (1.0 - u * 0.7) * fin * event->fade));
    }
    weapon_glow(surface, sx0 + dx, sy0 + dy, 1.8, (const Uint8[]){220, 235, 255},
                230.0 * (1.0 - u) * event->fade);
  }
}


// A lit carousel turns slowly, its painted horses rising and falling and its
// rim bulbs chasing round.
static void hill_carousel(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 13));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  double up = smooth_step(0.0, 0.12, event->progress);
  double r = s * 0.06, h = s * 0.055 * up;
  filledEllipseRGBA(surface, (int)cx, (int)(ground - 2), (int)r, (int)(r * 0.18) + 1, 110, 90, 90,
                    alpha);
  for (int k = 0; k < 6; k++) {
    double a = event->t * 0.7 + k * 1.0472;
    double px = cx + cos(a) * r * 0.8, depth = sin(a);
    double py = ground - 2 + depth * r * 0.12;
    double bob = sin(event->t * 2.4 + k * 1.3) * h * 0.08;
    thickLineRGBA(surface, (int)px, (int)py, (int)px, (int)(py - h * 0.85 + bob), 1, 220, 200, 120,
                  alpha);
    static const Uint8 horse[3][3] = {{240, 240, 236}, {200, 120, 70}, {150, 190, 230}};
    const Uint8 *c = horse[k % 3];
    filledEllipseRGBA(surface, (int)px, (int)(py - h * 0.5 + bob), (int)(r * 0.14) + 1,
                      (int)(r * 0.08) + 1, c[0], c[1], c[2], alpha);
    filledCircleRGBA(surface, (int)(px + cos(a + 1.57) * r * 0.11), (int)(py - h * 0.62 + bob),
                     (int)(r * 0.05) + 1, c[0], c[1], c[2], alpha);
  }
  thickLineRGBA(surface, (int)cx, (int)(ground - 2), (int)cx, (int)(ground - h), 3, 200, 170, 90,
                alpha);
  filledTrigonRGBA(surface, (int)(cx - r * 1.05), (int)(ground - h), (int)(cx + r * 1.05),
                   (int)(ground - h), (int)cx, (int)(ground - h - s * 0.03 * up), 190, 60, 70,
                   alpha);
  for (int b = 0; b < 12; b++) {
    double bx = cx - r * 1.05 + b * r * 2.1 / 11.0;
    double on = 0.5 + 0.5 * sin(event->t * 6.0 - b * 0.9);
    weapon_glow(surface, bx, ground - h, 1.0, (const Uint8[]){255, 225, 150},
                230.0 * on * up * event->fade);
  }
}


// A trebuchet hurls glowing stones in high arcs that burst into sparks where
// they land.
static void hill_trebuchet(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double bx = d > 0 ? surface->w * 0.22 : surface->w * 0.78;
  double ground = hill_ground(event, bx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.1, event->progress);
  double h = s * 0.05 * up;
  thickLineRGBA(surface, (int)(bx - s * 0.03), (int)ground, (int)bx, (int)(ground - h), 3, 92, 66,
                50, alpha);
  thickLineRGBA(surface, (int)(bx + s * 0.03), (int)ground, (int)bx, (int)(ground - h), 3, 92, 66,
                50, alpha);
  double cycle = 3.6;
  double u = fmod(event->t, cycle) / cycle;
  double arm = -0.9 + 2.4 * smooth_step(0.0, 0.35, u);
  double ax = bx + d * sin(arm) * s * 0.045, ay = ground - h - cos(arm) * s * 0.045;
  double bxx = bx - d * sin(arm) * s * 0.02, byy = ground - h + cos(arm) * s * 0.02;
  thickLineRGBA(surface, (int)bxx, (int)byy, (int)ax, (int)ay, 3, 130, 96, 70, alpha);
  filledCircleRGBA(surface, (int)bxx, (int)(byy + 3), (int)(s * 0.008) + 1, 70, 70, 80, alpha);
  static const Uint8 fire[3] = {255, 170, 80};
  double range = d * s * 0.5;
  if (u > 0.28 && u < 0.85) {
    double f = (u - 0.28) / 0.57;
    double px = bx + range * f, py = ground - h - 4.0 * f * (1.0 - f) * s * 0.17;
    for (int k = 0; k < 8; k++) {
      double ff = f - k * 0.015;
      if (ff < 0) break;
      weapon_glow(surface, bx + range * ff, ground - h - 4.0 * ff * (1.0 - ff) * s * 0.17, 1.0,
                  fire, 120.0 * (1.0 - k / 8.0) * event->fade);
    }
    weapon_glow(surface, px, py, 2.0, fire, 240.0 * event->fade);
  }
  if (u >= 0.85) {
    double f = (u - 0.85) / 0.15, lx = bx + range;
    double lg = hill_ground(event, lx);
    for (int k = 0; k < 10; k++) {
      double a = -3.14159 * (0.1 + 0.8 * k / 9.0);
      filledCircleRGBA(surface, (int)(lx + cos(a) * f * s * 0.04),
                       (int)(lg + sin(a) * f * s * 0.05 + f * f * s * 0.02), 1, 255, 200, 110,
                       weapon_alpha(220.0 * (1.0 - f) * event->fade));
    }
  }
}


// An orchard: apple trees fill with fruit and a few apples fall with a little
// glint.
static void hill_orchard(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int trees = 5;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  for (int i = 0; i < trees; i++) {
    double x = surface->w * (0.12 + 0.76 * (i + 0.5) / trees);
    double ground = hill_ground(event, x);
    double grow = smooth_step(0.0 + i * 0.03, 0.12 + i * 0.03, event->progress);
    double h = s * 0.045 * grow, cr = s * 0.032 * grow;
    thickLineRGBA(surface, (int)x, (int)ground, (int)x, (int)(ground - h), 3, 80, 56, 44, alpha);
    for (int blob = 0; blob < 4; blob++) {
      filledCircleRGBA(surface, (int)(x + (blob - 1.5) * cr * 0.5),
                       (int)(ground - h - cr * 0.3 * (blob == 1 || blob == 2)),
                       (int)(cr * 0.75) + 1, 42 + blob * 6, 100 + blob * 8, 54, alpha);
    }
    double fruit = smooth_step(0.3 + i * 0.03, 0.5 + i * 0.03, event->progress);
    for (int a = 0; a < 7; a++) {
      double ax = x + (hill_random(event, i * 7 + a, 14) - 0.5) * cr * 1.8,
             ay = ground - h - (hill_random(event, i * 7 + a, 15) - 0.3) * cr * 0.9;
      if (a < 7 * fruit) filledCircleRGBA(surface, (int)ax, (int)ay, 2, 220, 60, 56, alpha);
      if (a == 0 && fruit > 0.9) {
        double f = fmod(event->t * 0.35 + i * 0.2, 1.0);
        double fy = ay + f * f * (ground - ay);
        filledCircleRGBA(surface, (int)ax, (int)fy, 2, 230, 70, 60,
                         weapon_alpha(235.0 * event->fade));
        if (f > 0.9)
          weapon_glow(surface, ax, ground - 1, 1.2, (const Uint8[]){255, 210, 200},
                      160.0 * event->fade);
      }
    }
  }
}


// A tall radio mast blinks its red lights and sends rings of signal rippling up
// into the sky.
static void hill_radiomast(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.5 * hill_random(event, 0, 16));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double h = s * 0.22 * smooth_step(0.0, 0.14, event->progress);
  double w = s * 0.012;
  thickLineRGBA(surface, (int)(cx - w), (int)ground, (int)cx, (int)(ground - h), 1, 160, 164, 172,
                alpha);
  thickLineRGBA(surface, (int)(cx + w), (int)ground, (int)cx, (int)(ground - h), 1, 160, 164, 172,
                alpha);
  for (int k = 1; k < 8; k++) {
    double f = k / 8.0;
    double hw = w * (1.0 - f);
    lineRGBA(surface, (int)(cx - hw), (int)(ground - h * f), (int)(cx + hw),
             (int)(ground - h * f * 0.96), 150, 154, 162, alpha);
  }
  double blink = fmod(event->t, 1.4) < 0.5 ? 1.0 : 0.15;
  weapon_glow(surface, cx, ground - h, 1.4, (const Uint8[]){255, 70, 60},
              230.0 * blink * event->fade);
  weapon_glow(surface, cx, ground - h * 0.5, 1.0, (const Uint8[]){255, 70, 60},
              200.0 * (1.2 - blink) * event->fade);
  double on = smooth_step(0.15, 0.25, event->progress);
  for (int ring = 0; ring < 4; ring++) {
    double u = fmod(event->t * 0.28 + ring / 4.0, 1.0);
    ellipseRGBA(surface, (int)cx, (int)(ground - h), (int)(u * s * 0.25) + 1,
                (int)(u * s * 0.1) + 1, 150, 220, 255,
                weapon_alpha(200.0 * (1.0 - u) * on * event->fade));
  }
}


// A glass greenhouse glows from within, its plants swaying and a little steam
// venting from the roof.
static void hill_greenhouse(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 17));
  double ground = hill_ground(event, cx);
  double up = smooth_step(0.0, 0.1, event->progress);
  double w = s * 0.07, h = s * 0.045 * up;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 lamp[3] = {190, 255, 170};
  double lit = smooth_step(0.12, 0.22, event->progress);
  boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 70, 130, 100,
          weapon_alpha(120.0 * event->fade));
  filledTrigonRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)(ground - h),
                   (int)cx, (int)(ground - h - s * 0.03 * up), 90, 150, 120,
                   weapon_alpha(130.0 * event->fade));
  boxRGBA(surface, (int)(cx - w * 0.9), (int)(ground - h * 0.9), (int)(cx + w * 0.9),
          (int)(ground - 1), 150, 255, 150, weapon_alpha(70.0 * lit * event->fade));
  for (int k = 0; k <= 6; k++) {
    double fx = cx - w + k * w / 3.0;
    lineRGBA(surface, (int)fx, (int)(ground - h), (int)fx, (int)ground, 200, 230, 215, alpha);
  }
  rectangleRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 200, 230,
                215, alpha);
  for (int p = 0; p < 9; p++) {
    double px = cx - w * 0.85 + p * w * 1.7 / 8.0,
           ph = h * (0.3 + 0.4 * hill_random(event, p, 18)) * lit;
    double sway = sin(event->t * 1.4 + p) * ph * 0.12;
    thickLineRGBA(surface, (int)px, (int)(ground - 1), (int)(px + sway), (int)(ground - ph), 1, 70,
                  170, 80, alpha);
    if (p % 3 == 0)
      filledCircleRGBA(surface, (int)(px + sway), (int)(ground - ph), 2, 255, 120, 150,
                       weapon_alpha(230.0 * lit * event->fade));
  }
  weapon_glow(surface, cx, ground - h * 0.8, w * 0.5, lamp, 100.0 * lit * event->fade);
  for (int p = 0; p < 6; p++) {
    double u = fmod(event->t * 0.2 + p / 6.0, 1.0);
    filledCircleRGBA(surface, (int)(cx + sin(event->t + p) * s * 0.01),
                     (int)(ground - h - s * 0.03 - u * s * 0.08), (int)(2 + u * 4), 220, 235, 235,
                     weapon_alpha(100.0 * (1.0 - u) * lit * event->fade));
  }
}


// A swing set: two swings sway out of step, a child's silhouette on one of
// them.
static void hill_swings(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.5 * hill_random(event, 0, 19));
  double ground = hill_ground(event, cx);
  double up = smooth_step(0.0, 0.08, event->progress);
  double w = s * 0.05, h = s * 0.05 * up;
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  thickLineRGBA(surface, (int)(cx - w), (int)ground, (int)(cx - w * 0.7), (int)(ground - h), 2, 150,
                90, 80, alpha);
  thickLineRGBA(surface, (int)(cx - w * 0.4), (int)ground, (int)(cx - w * 0.7), (int)(ground - h),
                2, 150, 90, 80, alpha);
  thickLineRGBA(surface, (int)(cx + w), (int)ground, (int)(cx + w * 0.7), (int)(ground - h), 2, 150,
                90, 80, alpha);
  thickLineRGBA(surface, (int)(cx + w * 0.4), (int)ground, (int)(cx + w * 0.7), (int)(ground - h),
                2, 150, 90, 80, alpha);
  thickLineRGBA(surface, (int)(cx - w * 0.7), (int)(ground - h), (int)(cx + w * 0.7),
                (int)(ground - h), 3, 170, 100, 90, alpha);
  for (int k = 0; k < 2; k++) {
    double px = cx + (k - 0.5) * w * 0.7;
    double a = sin(event->t * 1.7 + k * 1.8) * (0.4 + 0.3 * smooth_step(0.1, 0.4, event->progress));
    double len = h * 0.85;
    double sx = px + sin(a) * len, sy = ground - h + cos(a) * len;
    lineRGBA(surface, (int)(px - 3), (int)(ground - h), (int)(sx - 3), (int)sy, 220, 210, 190,
             alpha);
    lineRGBA(surface, (int)(px + 3), (int)(ground - h), (int)(sx + 3), (int)sy, 220, 210, 190,
             alpha);
    thickLineRGBA(surface, (int)(sx - 4), (int)sy, (int)(sx + 4), (int)sy, 2, 60, 80, 120, alpha);
    if (k == 0) {
      thickLineRGBA(surface, (int)sx, (int)sy, (int)(sx + sin(a) * 2), (int)(sy - h * 0.25), 3, 20,
                    24, 36, alpha);
      filledCircleRGBA(surface, (int)(sx + sin(a) * 3), (int)(sy - h * 0.32), 3, 20, 24, 36, alpha);
    }
  }
}


// Beehives in a flowery meadow: glowing bees weave figure-eights around the
// boxes.
static void hill_beehives(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  const int count = 4;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 bee[3] = {255, 215, 80};
  for (int i = 0; i < count; i++) {
    double x = surface->w * (0.22 + 0.56 * (i + 0.5) / count);
    double ground = hill_ground(event, x);
    double up = smooth_step(0.0 + i * 0.04, 0.1 + i * 0.04, event->progress);
    double w = s * 0.016, h = s * 0.02 * up;
    boxRGBA(surface, (int)(x - w * 0.8), (int)(ground - h - w), (int)(x - w * 0.6), (int)ground, 90,
            70, 60, alpha);
    boxRGBA(surface, (int)(x + w * 0.6), (int)(ground - h - w), (int)(x + w * 0.8), (int)ground, 90,
            70, 60, alpha);
    boxRGBA(surface, (int)(x - w), (int)(ground - h * 1.8), (int)(x + w), (int)(ground - h),
            (i % 2) ? 230 : 240, 226, 200, alpha);
    boxRGBA(surface, (int)(x - w * 1.1), (int)(ground - h * 2.0), (int)(x + w * 1.1),
            (int)(ground - h * 1.8), 120, 90, 80, alpha);
    boxRGBA(surface, (int)(x - w * 0.3), (int)(ground - h * 1.3), (int)(x + w * 0.3),
            (int)(ground - h * 1.2), 30, 24, 24, alpha);
    for (int b = 0; b < 6; b++) {
      double a = event->t * (1.6 + 0.3 * (b % 3)) + b * 1.05 + i;
      double bx = x + sin(a) * s * 0.03, by = ground - h * 1.5 + sin(a * 2.0) * s * 0.012 - b * 1.0;
      weapon_glow(surface, bx, by, 0.8, bee, 190.0 * up * event->fade);
    }
  }
  for (int f = 0; f < 26; f++) {
    double fx = surface->w * (0.15 + 0.7 * hill_random(event, f, 20)),
           fy = hill_ground(event, fx) - 1.0;
    double grow = smooth_step(0.1 + f * 0.01, 0.25 + f * 0.01, event->progress);
    thickLineRGBA(surface, (int)fx, (int)fy, (int)fx, (int)(fy - s * 0.012 * grow), 1, 60, 140, 70,
                  alpha);
    filledCircleRGBA(surface, (int)fx, (int)(fy - s * 0.012 * grow), 2, f % 2 ? 250 : 240,
                     f % 3 ? 140 : 230, f % 2 ? 190 : 100,
                     weapon_alpha(235.0 * grow * event->fade));
  }
}


// A cableway: two pylons hold a long cable and lit gondolas glide up and down
// it past each other.
static void hill_cableway(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x1 = surface->w * 0.18, x2 = surface->w * 0.82;
  double g1 = hill_ground(event, x1), g2 = hill_ground(event, x2);
  double up = smooth_step(0.0, 0.12, event->progress);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double t1 = g1 - s * 0.12 * up, t2 = g2 - s * 0.2 * up;
  thickLineRGBA(surface, (int)x1, (int)g1, (int)x1, (int)t1, 2, 140, 144, 154, alpha);
  thickLineRGBA(surface, (int)x2, (int)g2, (int)x2, (int)t2, 2, 140, 144, 154, alpha);
  lineRGBA(surface, (int)x1, (int)t1, (int)x2, (int)t2, 190, 194, 204, alpha);
  lineRGBA(surface, (int)x1, (int)(t1 + 3), (int)x2, (int)(t2 + 3), 150, 154, 164, alpha);
  double move = smooth_step(0.15, 0.25, event->progress);
  for (int g = 0; g < 4; g++) {
    double u = fmod(event->t * 0.06 + g * 0.25, 1.0);
    if (g % 2) u = 1.0 - u;
    double gx = x1 + (x2 - x1) * u, gy = t1 + (t2 - t1) * u;
    thickLineRGBA(surface, (int)gx, (int)gy, (int)gx, (int)(gy + s * 0.012), 1, 150, 154, 164,
                  alpha);
    boxRGBA(surface, (int)(gx - s * 0.009), (int)(gy + s * 0.012), (int)(gx + s * 0.009),
            (int)(gy + s * 0.026), 200, 70, 60, weapon_alpha(235.0 * move * event->fade));
    boxRGBA(surface, (int)(gx - s * 0.006), (int)(gy + s * 0.015), (int)(gx + s * 0.006),
            (int)(gy + s * 0.021), 255, 224, 150, weapon_alpha(235.0 * move * event->fade));
  }
}


// Old stone ruins: pillars light up with runes one after another and a few
// fragments float above them.
static void hill_ruins(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 21));
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 rune[3] = {120, 230, 255};
  for (int i = 0; i < 6; i++) {
    double x = cx + (i - 2.5) * s * 0.05;
    double ground = hill_ground(event, x);
    double rise = smooth_step(0.0 + i * 0.03, 0.1 + i * 0.03, event->progress);
    double h = s * (0.03 + 0.025 * (i % 3) + 0.01 * hill_random(event, i, 22)) * rise;
    boxRGBA(surface, (int)(x - s * 0.009), (int)(ground - h), (int)(x + s * 0.009), (int)ground,
            110, 108, 114, alpha);
    boxRGBA(surface, (int)(x - s * 0.009), (int)(ground - h), (int)(x - s * 0.004), (int)ground,
            138, 136, 142, alpha);
    if (i % 3 != 2)
      boxRGBA(surface, (int)(x - s * 0.012), (int)(ground - h - s * 0.005), (int)(x + s * 0.012),
              (int)(ground - h), 126, 124, 130, alpha);
    double pulse = sin(event->t * 2.2 - i * 0.9);
    double on = smooth_step(0.2, 0.3, event->progress) * (pulse > 0 ? pulse : 0.0);
    weapon_glow(surface, x, ground - h * 0.55, 1.1, rune, 230.0 * on * event->fade);
    weapon_glow(surface, x, ground - h * 0.3, 0.8, rune, 200.0 * on * event->fade);
  }
  for (int k = 0; k < 4; k++) {
    double a = event->t * 0.6 + k * 1.6;
    double fx = cx + sin(a) * s * 0.07,
           fy = hill_ground(event, cx) - s * (0.12 + 0.02 * sin(a * 1.3 + k));
    filledEllipseRGBA(surface, (int)fx, (int)fy, (int)(s * 0.008) + 1, (int)(s * 0.005) + 1, 128,
                      126, 134,
                      weapon_alpha(235.0 * smooth_step(0.25, 0.35, event->progress) * event->fade));
  }
}


// A geodesic dome whose panels light up in drifting patterns of colour.
static void hill_geodome(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 23));
  double ground = hill_ground(event, cx);
  double r = s * 0.06 * smooth_step(0.0, 0.1, event->progress);
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  if (r < 2) return;
  filledPieRGBA(surface, (int)cx, (int)ground, (int)r, 180, 360, 70, 80, 100,
                weapon_alpha(120.0 * event->fade));
  for (int ring = 1; ring <= 3; ring++) {
    double rr = r * ring / 3.0;
    int segs = 4 + ring * 3;
    for (int k = 0; k < segs; k++) {
      double a0 = 3.14159 + 3.14159 * k / segs, a1 = 3.14159 + 3.14159 * (k + 1) / segs;
      double am = (a0 + a1) * 0.5, rm = rr - r / 6.0;
      lineRGBA(surface, (int)(cx + cos(a0) * rr), (int)(ground + sin(a0) * rr),
               (int)(cx + cos(a1) * rr), (int)(ground + sin(a1) * rr), 180, 190, 210, alpha);
      lineRGBA(surface, (int)(cx + cos(a0) * rr), (int)(ground + sin(a0) * rr),
               (int)(cx + cos(a0) * (rr - r / 3.0)), (int)(ground + sin(a0) * (rr - r / 3.0)), 180,
               190, 210, alpha);
      double hue = event->t * 0.6 + k * 0.7 + ring;
      double lit = 0.5 + 0.5 * sin(hue * 1.7);
      filledCircleRGBA(
          surface, (int)(cx + cos(am) * rm), (int)(ground + sin(am) * rm), (int)(r * 0.05) + 1,
          (Uint8)(150 + 100 * sin(hue)), (Uint8)(170 + 80 * sin(hue + 2.1)),
          (Uint8)(210 + 40 * sin(hue + 4.2)),
          weapon_alpha(230.0 * lit * smooth_step(0.15, 0.25, event->progress) * event->fade));
    }
  }
  lineRGBA(surface, (int)(cx - r), (int)ground, (int)(cx + r), (int)ground, 180, 190, 210, alpha);
}


// A log cabin: its windows light up one by one and smoke curls from the
// chimney.
static void hill_cabin(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 24));
  double ground = hill_ground(event, cx);
  double up = smooth_step(0.0, 0.1, event->progress);
  double w = s * 0.045, h = s * 0.035 * up;
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  static const Uint8 lamp[3] = {255, 200, 110};
  boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 104, 70, 52,
          alpha);
  for (int log = 1; log < 4; log++)
    lineRGBA(surface, (int)(cx - w), (int)(ground - h * log / 4.0), (int)(cx + w),
             (int)(ground - h * log / 4.0), 70, 46, 36, alpha);
  filledTrigonRGBA(surface, (int)(cx - w * 1.2), (int)(ground - h), (int)(cx + w * 1.2),
                   (int)(ground - h), (int)cx, (int)(ground - h - s * 0.03 * up), 72, 50, 54,
                   alpha);
  boxRGBA(surface, (int)(cx + w * 0.4), (int)(ground - h - s * 0.03 * up), (int)(cx + w * 0.65),
          (int)(ground - h - s * 0.008), 90, 80, 84, alpha);
  for (int win = 0; win < 2; win++) {
    double on = smooth_step(0.15 + win * 0.12, 0.22 + win * 0.12, event->progress) *
                (0.9 + 0.1 * sin(event->t * 7.0 + win));
    double wx = cx - w * 0.55 + win * w * 0.9;
    boxRGBA(surface, (int)(wx - w * 0.16), (int)(ground - h * 0.75), (int)(wx + w * 0.16),
            (int)(ground - h * 0.4), lamp[0], lamp[1], lamp[2],
            weapon_alpha(235.0 * on * event->fade));
    weapon_glow(surface, wx, ground - h * 0.58, w * 0.25, lamp, 70.0 * on * event->fade);
  }
  boxRGBA(surface, (int)(cx + w * 0.55), (int)(ground - h * 0.6), (int)(cx + w * 0.85), (int)ground,
          58, 38, 32, alpha);
  for (int p = 0; p < 12; p++) {
    double u = fmod(event->t * 0.18 + p / 12.0, 1.0);
    double sx = cx + w * 0.52 + sin(event->t * 1.2 + p) * s * 0.008 * u * 3.0 +
                u * s * 0.03 * event->direction;
    double sy = ground - h - s * 0.035 - u * s * 0.14;
    filledCircleRGBA(
        surface, (int)sx, (int)sy, (int)(2 + u * s * 0.008), 150, 152, 160,
        weapon_alpha(120.0 * (1.0 - u) * smooth_step(0.1, 0.2, event->progress) * event->fade));
  }
}


// Wind chimes hang from three poles; the tubes swing and a faint ring spreads
// when they strike.
static void hill_windchimes(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  for (int pole = 0; pole < 3; pole++) {
    double px = surface->w * (0.25 + 0.25 * pole) + (hill_random(event, pole, 25) - 0.5) * s * 0.04;
    double ground = hill_ground(event, px);
    double h = s * 0.07 * smooth_step(0.0 + pole * 0.04, 0.1 + pole * 0.04, event->progress);
    thickLineRGBA(surface, (int)px, (int)ground, (int)px, (int)(ground - h), 2, 110, 84, 66, alpha);
    thickLineRGBA(surface, (int)(px - s * 0.02), (int)(ground - h), (int)(px + s * 0.02),
                  (int)(ground - h), 2, 130, 100, 78, alpha);
    for (int tube = 0; tube < 5; tube++) {
      double tx = px - s * 0.016 + tube * s * 0.008;
      double swing = sin(event->t * (1.6 + 0.2 * tube) + pole + tube * 0.7) * 0.18;
      double len = s * (0.016 + 0.007 * ((tube + pole) % 3));
      double ex = tx + sin(swing) * len, ey = ground - h + cos(swing) * len;
      thickLineRGBA(surface, (int)tx, (int)(ground - h), (int)ex, (int)ey, 2, 210, 215, 225, alpha);
      if (fabs(swing) > 0.17) {
        double r = (fabs(swing) - 0.17) / 0.01 * s * 0.01;
        ellipseRGBA(surface, (int)ex, (int)ey, (int)(4 + r), (int)(2 + r * 0.5), 235, 245, 255,
                    weapon_alpha(130.0 * event->fade));
      }
    }
  }
}


// A picnic blanket with a basket and two friends; a lantern at the middle
// lights their faces.
static void hill_picnic(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 26));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double fin = smooth_step(0.0, 0.1, event->progress);
  static const Uint8 lamp[3] = {255, 205, 120};
  double w = s * 0.05 * fin, flutter = sin(event->t * 3.0) * 1.0;
  for (int k = 0; k < 6; k++) {
    for (int j = 0; j < 2; j++) {
      Uint8 c = ((k + j) % 2) ? 200 : 120;
      boxRGBA(surface, (int)(cx - w + k * w / 3.0),
              (int)(ground - s * 0.006 * (j + 1) + (k == 5 ? flutter : 0)),
              (int)(cx - w + (k + 1) * w / 3.0), (int)(ground - s * 0.006 * j), c, (Uint8)(c / 2),
              (Uint8)(c / 3), alpha);
    }
  }
  boxRGBA(surface, (int)(cx + w * 0.4), (int)(ground - s * 0.018), (int)(cx + w * 0.7),
          (int)(ground - s * 0.006), 140, 100, 70, alpha);
  for (int p = 0; p < 2; p++) {
    double px = cx + (p ? 1.0 : -1.0) * w * 0.65;
    filledTrigonRGBA(surface, (int)(px - s * 0.008), (int)(ground - s * 0.006),
                     (int)(px + s * 0.008), (int)(ground - s * 0.006), (int)px,
                     (int)(ground - s * 0.026), p ? 60 : 150, 70, p ? 150 : 70, alpha);
    filledCircleRGBA(surface, (int)px, (int)(ground - s * 0.03), (int)(s * 0.005) + 1, 228, 190,
                     160, alpha);
  }
  weapon_glow(surface, cx, ground - s * 0.012, 2.2, lamp,
              220.0 * smooth_step(0.1, 0.2, event->progress) * event->fade *
                  (0.85 + 0.15 * sin(event->t * 6.0)));
}


// A wishing tree: ribbons and glowing paper tags hang from its branches and
// flutter in a breeze.
static void hill_wishtree(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 27));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double grow = smooth_step(0.0, 0.12, event->progress);
  double h = s * 0.09 * grow;
  thickLineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)(ground - h), 4, 76, 56, 48, alpha);
  for (int b = 0; b < 6; b++) {
    double side = b % 2 ? 1.0 : -1.0;
    double bx = cx + side * s * 0.05 * (0.6 + 0.4 * (b / 2) / 2.0),
           by = ground - h * (0.55 + 0.1 * (b / 2));
    thickLineRGBA(surface, (int)cx, (int)(ground - h * (0.45 + 0.1 * (b / 2))), (int)bx, (int)by, 2,
                  84, 62, 52, alpha);
    for (int r = 0; r < 4; r++) {
      double rx = cx + (bx - cx) * (0.4 + 0.2 * r);
      double ry = (ground - h * (0.45 + 0.1 * (b / 2))) +
                  (by - (ground - h * (0.45 + 0.1 * (b / 2)))) * (0.4 + 0.2 * r);
      double len = s * (0.02 + 0.01 * hill_random(event, b * 4 + r, 28)) *
                   smooth_step(0.1 + b * 0.03, 0.25 + b * 0.03, event->progress);
      double sway = sin(event->t * 1.8 + b + r * 0.6) * len * 0.3;
      double hue = b * 0.9 + r;
      Uint8 cr = (Uint8)(190 + 60 * sin(hue)), cg = (Uint8)(150 + 80 * sin(hue + 2.1)),
            cb = (Uint8)(180 + 70 * sin(hue + 4.2));
      thickLineRGBA(surface, (int)rx, (int)ry, (int)(rx + sway), (int)(ry + len), 1, cr, cg, cb,
                    alpha);
      double glow = 0.5 + 0.5 * sin(event->t * 2.0 + b * 1.3 + r);
      boxRGBA(surface, (int)(rx + sway - 1), (int)(ry + len), (int)(rx + sway + 2),
              (int)(ry + len + 4), 255, 240, 200,
              weapon_alpha((130.0 + 120.0 * glow) * event->fade));
    }
  }
}


// A mine mouth in the hillside: loaded ore carts roll out of the dark, their
// lamps sweeping the rails.
static void hill_mine(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 29));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.1, event->progress);
  double r = s * 0.035 * up;
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(r * 1.5), (int)r, 60, 56, 62, alpha);
  filledEllipseRGBA(surface, (int)cx, (int)(ground + 1), (int)r, (int)(r * 0.75), 10, 10, 14,
                    alpha);
  thickLineRGBA(surface, (int)(cx - r), (int)(ground - r * 0.9), (int)(cx - r), (int)ground, 2, 110,
                76, 56, alpha);
  thickLineRGBA(surface, (int)(cx + r), (int)(ground - r * 0.9), (int)(cx + r), (int)ground, 2, 110,
                76, 56, alpha);
  lineRGBA(surface, (int)cx, (int)(ground - 1), (int)(cx + d * s * 0.3), (int)(ground + 2), 130,
           130, 140, alpha);
  static const Uint8 ore[3] = {120, 240, 255};
  for (int c = 0; c < 3; c++) {
    double u = fmod(event->t * 0.07 + c / 3.0, 1.0);
    double x = cx + d * s * 0.04 + d * u * s * 0.28, y = ground + u * 2.0 - s * 0.003;
    boxRGBA(surface, (int)(x - s * 0.009), (int)(y - s * 0.013), (int)(x + s * 0.009), (int)y, 100,
            84, 70, weapon_alpha(235.0 * smooth_step(0.0, 0.1, u) * event->fade));
    filledCircleRGBA(surface, (int)(x - 3), (int)(y - s * 0.014), 2, 140, 220, 255,
                     weapon_alpha(235.0 * smooth_step(0.0, 0.1, u) * event->fade));
    weapon_glow(
        surface, x + 2, y - s * 0.016, 1.0, ore,
        170.0 * (0.6 + 0.4 * sin(event->t * 5.0 + c)) * smooth_step(0.0, 0.1, u) * event->fade);
    filledCircleRGBA(surface, (int)(x - s * 0.006), (int)y, 2, 30, 30, 36,
                     weapon_alpha(235.0 * event->fade));
    filledCircleRGBA(surface, (int)(x + s * 0.006), (int)y, 2, 30, 30, 36,
                     weapon_alpha(235.0 * event->fade));
  }
}


// Large glowing eggs lie in a nest; they crack in turn and small bright
// wyrmlings climb out and flutter up.
static void hill_eggs(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 30));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  for (int i = 0; i < 5; i++) {
    double x = cx + (i - 2) * s * 0.035;
    double y = ground - s * 0.012 + (i % 2) * 2.0;
    double pulse = 0.5 + 0.5 * sin(event->t * 1.5 + i);
    double hatch = smooth_step(0.35 + i * 0.07, 0.45 + i * 0.07, event->progress);
    Uint8 tint[3] = {(Uint8)(120 + 80 * (i % 3) / 2), 220, (Uint8)(190 + 40 * (i % 2))};
    filledEllipseRGBA(surface, (int)x, (int)y, (int)(s * 0.012), (int)(s * 0.017), 96, 92, 110,
                      alpha);
    filledEllipseRGBA(surface, (int)x, (int)(y - 1), (int)(s * 0.009), (int)(s * 0.013), tint[0],
                      tint[1], tint[2],
                      weapon_alpha((110.0 + 100.0 * pulse) * (1.0 - hatch * 0.6) * event->fade));
    if (hatch > 0.0) {
      lineRGBA(surface, (int)(x - s * 0.01), (int)y, (int)x, (int)(y - s * 0.006), 20, 20, 28,
               alpha);
      lineRGBA(surface, (int)x, (int)(y - s * 0.006), (int)(x + s * 0.01), (int)(y - s * 0.012), 20,
               20, 28, alpha);
      double fly = smooth_step(0.0, 1.0, fmod(event->t * 0.15 + i * 0.2, 1.0)) * hatch;
      double wx = x + sin(event->t * 2.0 + i) * s * 0.02 * fly, wy = y - s * 0.02 - fly * s * 0.1;
      double flap = sin(event->t * 14.0 + i) * s * 0.006;
      thickLineRGBA(surface, (int)(wx - s * 0.007), (int)(wy - flap), (int)wx, (int)wy, 1, tint[0],
                    tint[1], tint[2], weapon_alpha(235.0 * (1.0 - fly * 0.5) * event->fade));
      thickLineRGBA(surface, (int)(wx + s * 0.007), (int)(wy - flap), (int)wx, (int)wy, 1, tint[0],
                    tint[1], tint[2], weapon_alpha(235.0 * (1.0 - fly * 0.5) * event->fade));
      weapon_glow(surface, wx, wy, 1.2, tint, 200.0 * (1.0 - fly * 0.5) * event->fade);
    }
  }
  for (int n = 0; n < 12; n++) {
    double a = n * 0.52;
    filledCircleRGBA(surface, (int)(cx + cos(a) * s * 0.1), (int)(ground + sin(a) * 1.5), 2, 90, 70,
                     50, alpha);
  }
}


// Two trees hold a hammock that sways gently, a sleeper in it and a lantern
// glowing on a branch.
static void hill_hammock(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 31));
  double span = s * 0.09;
  double g1 = hill_ground(event, cx - span), g2 = hill_ground(event, cx + span);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double grow = smooth_step(0.0, 0.1, event->progress);
  double h = s * 0.06 * grow;
  static const Uint8 lamp[3] = {255, 205, 120};
  for (int side = -1; side <= 1; side += 2) {
    double tx = cx + side * span, tg = side < 0 ? g1 : g2;
    thickLineRGBA(surface, (int)tx, (int)tg, (int)tx, (int)(tg - h), 3, 84, 62, 50, alpha);
    filledCircleRGBA(surface, (int)tx, (int)(tg - h - s * 0.012), (int)(s * 0.022 * grow) + 1, 48,
                     104, 62, alpha);
  }
  double sway = sin(event->t * 0.9) * s * 0.004;
  double px = cx - span, py = g1 - h * 0.55, qx = cx + span, qy = g2 - h * 0.55;
  double prevx = px, prevy = py;
  for (int i = 1; i <= 16; i++) {
    double f = i / 16.0;
    double hx = px + (qx - px) * f + sway * sin(f * 3.14159) * 2.0;
    double hy = py + (qy - py) * f + sin(f * 3.14159) * s * 0.02;
    thickLineRGBA(surface, (int)prevx, (int)prevy, (int)hx, (int)hy, 2, 210, 120, 100, alpha);
    prevx = hx;
    prevy = hy;
  }
  double mx = cx + sway * 2.0, my = (py + qy) / 2 + s * 0.02;
  filledEllipseRGBA(surface, (int)mx, (int)(my - s * 0.005), (int)(s * 0.016), (int)(s * 0.005) + 1,
                    24, 28, 40, alpha);
  filledCircleRGBA(surface, (int)(mx - s * 0.016), (int)(my - s * 0.008), (int)(s * 0.005) + 1, 24,
                   28, 40, alpha);
  weapon_glow(surface, cx - span * 0.9, g1 - h * 0.85, 2.0, lamp,
              210.0 * smooth_step(0.1, 0.2, event->progress) * event->fade *
                  (0.9 + 0.1 * sin(event->t * 4.0)));
}


// A line of telegraph poles runs along the ridge; pulses of light race down the
// wires between them.
static void hill_telegraph(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  const int poles = 7;
  double px[poles], py[poles];
  for (int i = 0; i < poles; i++) {
    px[i] = surface->w * (0.1 + 0.8 * i / (poles - 1));
    double g = hill_ground(event, px[i]);
    double h = s * 0.07 * smooth_step(0.0 + i * 0.03, 0.1 + i * 0.03, event->progress);
    py[i] = g - h;
    thickLineRGBA(surface, (int)px[i], (int)g, (int)px[i], (int)py[i], 2, 100, 78, 62, alpha);
    thickLineRGBA(surface, (int)(px[i] - s * 0.012), (int)(py[i] + 2), (int)(px[i] + s * 0.012),
                  (int)(py[i] + 2), 2, 120, 94, 74, alpha);
  }
  static const Uint8 pulse[3] = {150, 235, 255};
  for (int i = 0; i + 1 < poles; i++) {
    for (int w = -1; w <= 1; w += 2) {
      double ax = px[i] + w * s * 0.01, ay = py[i] + 2, bx = px[i + 1] + w * s * 0.01,
             by = py[i + 1] + 2;
      double prevx = ax, prevy = ay;
      for (int k = 1; k <= 10; k++) {
        double f = k / 10.0;
        double nx = ax + (bx - ax) * f, ny = ay + (by - ay) * f + sin(f * 3.14159) * s * 0.012;
        lineRGBA(surface, (int)prevx, (int)prevy, (int)nx, (int)ny, 150, 150, 160, alpha);
        prevx = nx;
        prevy = ny;
      }
      double u = fmod(event->t * 0.5 + i * 0.37 + (w > 0 ? 0.0 : 0.5), 1.0) * 1.0;
      double dx = event->direction > 0 ? u : 1.0 - u;
      weapon_glow(surface, ax + (bx - ax) * dx, ay + (by - ay) * dx + sin(dx * 3.14159) * s * 0.012,
                  1.0, pulse, 230.0 * smooth_step(0.2, 0.3, event->progress) * event->fade);
    }
  }
}


// A chimney-lit village of tiny houses on a hillside: windows blink on at
// random, and a church bell swings.
static void hill_hamlet(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  static const Uint8 lamp[3] = {255, 205, 120};
  double start = surface->w * (0.2 + 0.2 * hill_random(event, 0, 40));
  for (int i = 0; i < 7; i++) {
    double x = start + i * s * 0.07;
    double ground = hill_ground(event, x);
    double up = smooth_step(0.0 + i * 0.03, 0.08 + i * 0.03, event->progress);
    double w = s * (0.016 + 0.006 * (i % 3)), h = s * (0.02 + 0.008 * ((i + 1) % 3)) * up;
    boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)ground, 74 + i * 6, 62, 70,
            alpha);
    filledTrigonRGBA(surface, (int)(x - w * 1.25), (int)(ground - h), (int)(x + w * 1.25),
                     (int)(ground - h), (int)x, (int)(ground - h - w * 1.1), 120, 60, 56, alpha);
    double blink = fmod(event->t * 0.15 + hill_random(event, i, 41), 1.0);
    double on = smooth_step(0.2, 0.3, event->progress) * (blink < 0.7 ? 1.0 : 0.0);
    boxRGBA(surface, (int)(x - w * 0.3), (int)(ground - h * 0.7), (int)(x + w * 0.3),
            (int)(ground - h * 0.35), lamp[0], lamp[1], lamp[2],
            weapon_alpha(235.0 * on * event->fade));
    if (i % 3 == 1) {
      for (int p = 0; p < 5; p++) {
        double u = fmod(event->t * 0.2 + p / 5.0 + i * 0.1, 1.0);
        filledCircleRGBA(
            surface,
            (int)(x + w * 0.6 + sin(event->t + p) * 2.0 * u + u * event->direction * s * 0.02),
            (int)(ground - h - w - u * s * 0.06), (int)(1 + u * 3), 150, 152, 160,
            weapon_alpha(110.0 * (1.0 - u) * up * event->fade));
      }
    }
  }
  double cx = start + 7 * s * 0.07, cg = hill_ground(event, cx);
  double ch = s * 0.07 * smooth_step(0.1, 0.2, event->progress);
  boxRGBA(surface, (int)(cx - s * 0.01), (int)(cg - ch), (int)(cx + s * 0.01), (int)cg, 96, 90, 100,
          alpha);
  filledTrigonRGBA(surface, (int)(cx - s * 0.013), (int)(cg - ch), (int)(cx + s * 0.013),
                   (int)(cg - ch), (int)cx, (int)(cg - ch - s * 0.02), 110, 70, 66, alpha);
  double swing = sin(event->t * 2.2) * s * 0.004;
  filledCircleRGBA(surface, (int)(cx + swing), (int)(cg - ch + s * 0.01), (int)(s * 0.003) + 1, 230,
                   190, 90, weapon_alpha(235.0 * event->fade));
}


// A row of glowing stepping stones lights up across a dark pond, one after
// another, as a traveller hops along them.
static void hill_stepstones(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * 0.5;
  double ground = hill_ground(event, cx);
  double pw = s * 0.26, ph = s * 0.03;
  static const Uint8 glow[3] = {130, 220, 255};
  double fill = smooth_step(0.0, 0.1, event->progress);
  filledEllipseRGBA(surface, (int)cx, (int)(ground - ph * 0.3), (int)(pw * fill) + 1,
                    (int)(ph * fill) + 1, 22, 40, 62, weapon_alpha(225.0 * event->fade));
  const int stones = 8;
  for (int i = 0; i < stones; i++) {
    double f = (double)i / (stones - 1);
    double x = cx + (f - 0.5) * pw * 1.6 * event->direction;
    double y = ground - ph * 0.3 + sin(f * 6.28) * ph * 0.3;
    double on = smooth_step(0.1 + i * 0.05, 0.16 + i * 0.05, event->progress);
    filledEllipseRGBA(surface, (int)x, (int)y, (int)(s * 0.012) + 1, (int)(s * 0.004) + 1, 110, 116,
                      130, weapon_alpha(235.0 * on * event->fade));
    weapon_glow(surface, x, y, s * 0.004, glow,
                150.0 * on * event->fade * (0.7 + 0.3 * sin(event->t * 2.0 + i)));
    double rip = fmod(event->t * 0.5 + i * 0.13, 1.0);
    ellipseRGBA(surface, (int)x, (int)y, (int)(s * 0.01 + rip * s * 0.02),
                (int)(s * 0.003 + rip * s * 0.006), 170, 210, 240,
                weapon_alpha(110.0 * (1.0 - rip) * on * event->fade));
  }
  double hop = fmod(event->t * 0.18, 1.0);
  double hf = hop * (stones - 1);
  int k = (int)hf;
  double u = hf - k;
  double f0 = (double)k / (stones - 1), f1 = (double)(k + 1 < stones ? k + 1 : k) / (stones - 1);
  double x0 = cx + (f0 - 0.5) * pw * 1.6 * event->direction,
         x1 = cx + (f1 - 0.5) * pw * 1.6 * event->direction;
  double px = x0 + (x1 - x0) * u, py = ground - ph * 0.3 - sin(u * 3.14159) * s * 0.02 - s * 0.012;
  hill_walker(surface, px, py + s * 0.012, s * 0.03, event->direction, event->t * 5.0, false,
              event->fade * smooth_step(0.3, 0.4, event->progress), event->t, 0);
}


// A seesaw rocks on the hill with two small figures; a gentle ripple of dust
// puffs at each landing.
static void hill_seesaw(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 42));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double fin = smooth_step(0.0, 0.08, event->progress);
  double half = s * 0.05;
  double tilt = sin(event->t * 1.8) * 0.28;
  double piv_y = ground - s * 0.012;
  filledTrigonRGBA(surface, (int)(cx - s * 0.01), (int)ground, (int)(cx + s * 0.01), (int)ground,
                   (int)cx, (int)piv_y, 130, 100, 84, alpha);
  for (int side = -1; side <= 1; side += 2) {
    double ex = cx + side * cos(tilt) * half * fin, ey = piv_y - side * sin(tilt) * half * fin;
    thickLineRGBA(surface, (int)cx, (int)piv_y, (int)ex, (int)ey, 3, 190, 140, 90, alpha);
    double hy = ey - s * 0.004;
    thickLineRGBA(surface, (int)ex, (int)hy, (int)ex, (int)(hy - s * 0.014), 3, side < 0 ? 40 : 150,
                  60, side < 0 ? 140 : 60, alpha);
    filledCircleRGBA(surface, (int)ex, (int)(hy - s * 0.019), (int)(s * 0.004) + 1, 226, 190, 160,
                     alpha);
    bool landing = (side * sin(tilt) < -0.25);
    if (landing) {
      double q = (fabs(sin(tilt)) - 0.25) / 0.03;
      if (q > 1.0) q = 1.0;
      for (int p = 0; p < 4; p++) {
        filledCircleRGBA(surface, (int)(ex + (p - 1.5) * s * 0.006 * q),
                         (int)(ground - p % 2 * 2 - q * s * 0.006), 2, 150, 130, 100,
                         weapon_alpha(110.0 * (1.0 - q * 0.5) * fin * event->fade));
      }
    }
  }
}


// A line of laundry strung between two poles: sheets and shirts billow in the
// wind and a lantern glows beneath.
static void hill_laundry(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 43));
  double span = s * 0.1;
  double g1 = hill_ground(event, cx - span), g2 = hill_ground(event, cx + span);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.08, event->progress);
  double h = s * 0.055 * up;
  static const Uint8 lamp[3] = {255, 205, 120};
  thickLineRGBA(surface, (int)(cx - span), (int)g1, (int)(cx - span), (int)(g1 - h), 2, 100, 78, 64,
                alpha);
  thickLineRGBA(surface, (int)(cx + span), (int)g2, (int)(cx + span), (int)(g2 - h), 2, 100, 78, 64,
                alpha);
  double ax = cx - span, ay = g1 - h, bx = cx + span, by = g2 - h;
  double prevx = ax, prevy = ay;
  for (int i = 1; i <= 14; i++) {
    double f = i / 14.0;
    double nx = ax + (bx - ax) * f, ny = ay + (by - ay) * f + sin(f * 3.14159) * s * 0.01;
    lineRGBA(surface, (int)prevx, (int)prevy, (int)nx, (int)ny, 180, 180, 188, alpha);
    prevx = nx;
    prevy = ny;
  }
  double hang = smooth_step(0.1, 0.2, event->progress);
  for (int k = 0; k < 6; k++) {
    double f = (k + 1) / 7.0;
    double px = ax + (bx - ax) * f, py = ay + (by - ay) * f + sin(f * 3.14159) * s * 0.01;
    double billow = sin(event->t * 2.2 + k * 1.1) * s * 0.006;
    double w = s * (0.011 + 0.005 * (k % 2)), l = s * (0.022 + 0.01 * (k % 3)) * hang;
    static const Uint8 col[3][3] = {{235, 235, 235}, {200, 120, 110}, {120, 160, 210}};
    const Uint8 *c = col[k % 3];
    filledTrigonRGBA(surface, (int)(px - w), (int)py, (int)(px + w), (int)py, (int)(px + billow),
                     (int)(py + l), c[0], c[1], c[2], alpha);
    filledTrigonRGBA(surface, (int)(px - w), (int)py, (int)(px + billow), (int)(py + l),
                     (int)(px - w + billow * 1.5), (int)(py + l), c[0], c[1], c[2], alpha);
  }
  weapon_glow(surface, cx, (g1 + g2) * 0.5 - s * 0.006, 2.0, lamp,
              190.0 * smooth_step(0.15, 0.25, event->progress) * event->fade *
                  (0.85 + 0.15 * sin(event->t * 5.0)));
}


// A gentle wooden water-wheel pump by a well: a bucket rises on a rope, tips,
// and a silver trickle runs off down the hill.
static void hill_well(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 44));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.1, event->progress);
  double w = s * 0.022, h = s * 0.02 * up;
  boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 118, 112, 120,
          alpha);
  boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)(ground - h + 2), 150, 144,
          152, alpha);
  double post = s * 0.045 * up;
  thickLineRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx - w), (int)(ground - h - post),
                3, 100, 74, 58, alpha);
  thickLineRGBA(surface, (int)(cx + w), (int)(ground - h), (int)(cx + w), (int)(ground - h - post),
                3, 100, 74, 58, alpha);
  filledTrigonRGBA(surface, (int)(cx - w * 1.5), (int)(ground - h - post), (int)(cx + w * 1.5),
                   (int)(ground - h - post), (int)cx, (int)(ground - h - post - s * 0.02), 120, 70,
                   60, alpha);
  double cycle = fmod(event->t * 0.25, 1.0);
  double lift = cycle < 0.6 ? cycle / 0.6 : 1.0;
  double by = ground - h - post * 0.15 - lift * post * 0.65;
  thickLineRGBA(surface, (int)cx, (int)(ground - h - post), (int)cx, (int)by, 1, 200, 190, 170,
                alpha);
  boxRGBA(surface, (int)(cx - s * 0.006), (int)by, (int)(cx + s * 0.006), (int)(by + s * 0.009),
          140, 100, 70, alpha);
  if (cycle >= 0.6) {
    double pour = (cycle - 0.6) / 0.4;
    for (int d = 0; d < 8; d++) {
      double u = fmod(pour * 2.0 + d / 8.0, 1.0);
      filledCircleRGBA(
          surface, (int)(cx + event->direction * (s * 0.008 + u * s * 0.02)),
          (int)(by + s * 0.01 + u * u * (ground - by)), 1, 190, 225, 250,
          weapon_alpha(220.0 * (1.0 - u * 0.4) * smooth_step(0.0, 0.2, pour) * event->fade));
    }
  }
  for (int k = 0; k < 14; k++) {
    double u = fmod(event->t * 0.4 + k / 14.0, 1.0);
    double sx = cx + event->direction * (w + u * s * 0.2), sy = hill_ground(event, sx) - 1.0;
    filledCircleRGBA(
        surface, (int)sx, (int)sy, 1, 180, 215, 245,
        weapon_alpha(180.0 * (1.0 - u) * smooth_step(0.1, 0.2, event->progress) * event->fade));
  }
}


// A kiln on the hillside glows hot; potters carry glowing pots out one by one
// and set them in a row to cool.
static void hill_kiln(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = surface->w * (d > 0 ? 0.25 : 0.75);
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.1, event->progress);
  double r = s * 0.03 * up;
  static const Uint8 fire[3] = {255, 150, 60};
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)r, (int)(r * 1.1), 120, 80, 64, alpha);
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(r * 0.8), (int)(r * 0.9), 96, 64, 52,
                    alpha);
  boxRGBA(surface, (int)(cx - r * 0.3), (int)(ground - r * 1.1 - s * 0.012), (int)(cx + r * 0.3),
          (int)(ground - r * 0.9), 104, 72, 58, alpha);
  double heat = smooth_step(0.08, 0.18, event->progress) * (0.85 + 0.15 * sin(event->t * 6.0));
  filledEllipseRGBA(surface, (int)cx, (int)(ground - r * 0.1), (int)(r * 0.35) + 1,
                    (int)(r * 0.4) + 1, 255, 190, 90, weapon_alpha(235.0 * heat * event->fade));
  weapon_glow(surface, cx, ground - r * 0.1, r * 0.4, fire, 120.0 * heat * event->fade);
  for (int p = 0; p < 10; p++) {
    double u = fmod(event->t * 0.2 + p / 10.0, 1.0);
    filledCircleRGBA(surface, (int)(cx + sin(event->t + p) * s * 0.006 * u * 3.0),
                     (int)(ground - r * 1.1 - s * 0.012 - u * s * 0.12), (int)(2 + u * 5), 130, 130,
                     140, weapon_alpha(100.0 * (1.0 - u) * heat * event->fade));
  }
  const int pots = 6;
  for (int i = 0; i < pots; i++) {
    double ready = 0.25 + i * 0.1;
    double carry = smooth_step(ready, ready + 0.08, event->progress);
    double rest = surface->w * (d > 0 ? 0.42 + i * 0.07 : 0.58 - i * 0.07);
    double px = cx + d * r * 1.2 + (rest - (cx + d * r * 1.2)) * carry;
    double pg = hill_ground(event, px);
    if (event->progress < ready - 0.02) continue;
    double cool = smooth_step(ready + 0.06, ready + 0.3, event->progress);
    double pr = s * 0.007;
    Uint8 cr = (Uint8)(255 - 150 * cool), cg = (Uint8)(150 - 50 * cool),
          cb = (Uint8)(60 + 30 * cool);
    filledEllipseRGBA(surface, (int)px, (int)(pg - pr * 1.2), (int)pr + 1, (int)(pr * 1.2) + 1, cr,
                      cg, cb, alpha);
    boxRGBA(surface, (int)(px - pr * 0.5), (int)(pg - pr * 2.6), (int)(px + pr * 0.5),
            (int)(pg - pr * 2.2), cr, cg, cb, alpha);
    if (cool < 0.7)
      weapon_glow(surface, px, pg - pr * 1.2, pr, fire, 120.0 * (1.0 - cool) * event->fade);
  }
}


// A rope bridge sags between two rises; lanterns hang along it and sway as a
// traveller crosses.
static void hill_ropebridge(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x1 = surface->w * 0.2, x2 = surface->w * 0.8;
  double g1 = hill_ground(event, x1), g2 = hill_ground(event, x2);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.12, event->progress);
  static const Uint8 lamp[3] = {255, 205, 120};
  thickLineRGBA(surface, (int)x1, (int)g1, (int)x1, (int)(g1 - s * 0.05 * up), 3, 100, 76, 60,
                alpha);
  thickLineRGBA(surface, (int)x2, (int)g2, (int)x2, (int)(g2 - s * 0.05 * up), 3, 100, 76, 60,
                alpha);
  const int n = 28;
  double prevx = x1, prevy = g1 - s * 0.03 * up, prevy2 = g1 - s * 0.06 * up;
  for (int i = 1; i <= n; i++) {
    double f = (double)i / n;
    double nx = x1 + (x2 - x1) * f;
    double sag = sin(f * 3.14159) * s * 0.05 * up;
    double deck =
        g1 + (g2 - g1) * f - s * 0.03 * up + sag + sin(event->t * 1.3 - f * 8.0) * s * 0.0015;
    double rail = deck - s * 0.03 * up;
    thickLineRGBA(surface, (int)prevx, (int)prevy, (int)nx, (int)deck, 2, 130, 98, 76, alpha);
    lineRGBA(surface, (int)prevx, (int)prevy2, (int)nx, (int)rail, 170, 150, 120, alpha);
    if (i % 3 == 0)
      lineRGBA(surface, (int)nx, (int)deck, (int)nx, (int)rail, 150, 126, 100,
               (Uint8)(alpha * 0.7));
    if (i % 7 == 0) {
      double sw = sin(event->t * 1.8 + i) * s * 0.003;
      weapon_glow(surface, nx + sw, rail + s * 0.008, 1.6, lamp,
                  200.0 * smooth_step(0.15, 0.25, event->progress) * event->fade);
    }
    prevx = nx;
    prevy = deck;
    prevy2 = rail;
  }
  double cross = fmod(event->t * 0.04, 1.0);
  double f = event->direction > 0 ? cross : 1.0 - cross;
  double wx = x1 + (x2 - x1) * f;
  double wy = g1 + (g2 - g1) * f - s * 0.03 * up + sin(f * 3.14159) * s * 0.05 * up;
  hill_walker(surface, wx, wy, s * 0.03, event->direction, event->t * 5.0, true,
              event->fade * smooth_step(0.2, 0.3, event->progress), event->t, 0);
}


// A pottery-wheel-sized bell tower chimes: bells of different sizes swing in
// turn and each ring sends a pulse of light.
static void hill_belltower(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 45));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.12, event->progress);
  double w = s * 0.034, h = s * 0.062 * up;
  static const Uint8 glow[3] = {255, 225, 150};
  boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx + w), (int)ground, 108, 98, 110,
          alpha);
  boxRGBA(surface, (int)(cx - w), (int)(ground - h), (int)(cx - w * 0.4), (int)ground, 132, 122,
          134, alpha);
  boxRGBA(surface, (int)(cx - w * 1.15), (int)(ground - h - s * 0.012), (int)(cx + w * 1.15),
          (int)(ground - h), 92, 84, 96, alpha);
  filledTrigonRGBA(surface, (int)(cx - w * 1.15), (int)(ground - h - s * 0.012),
                   (int)(cx + w * 1.15), (int)(ground - h - s * 0.012), (int)cx,
                   (int)(ground - h - s * 0.05), 120, 64, 60, alpha);
  boxRGBA(surface, (int)(cx - w * 0.6), (int)(ground - h * 0.55), (int)(cx + w * 0.6),
          (int)(ground - h * 0.35), 20, 18, 26, alpha);
  for (int b = 0; b < 3; b++) {
    double bx = cx + (b - 1) * w * 0.7, top = ground - h - s * 0.002;
    double phase = event->t * 1.2 - b * 0.9;
    double swing = sin(phase * 3.0) * 0.5 * smooth_step(0.15, 0.25, event->progress);
    double len = s * (0.012 + 0.003 * (2 - b));
    double ex = bx + sin(swing) * len, ey = top + cos(swing) * len;
    lineRGBA(surface, (int)bx, (int)top, (int)ex, (int)(ey - s * 0.004), 170, 160, 140, alpha);
    filledTrigonRGBA(surface, (int)(ex - s * 0.009), (int)(ey + s * 0.008), (int)(ex + s * 0.009),
                     (int)(ey + s * 0.008), (int)ex, (int)(ey - s * 0.007), 240, 196, 90, alpha);
    if (fabs(swing) > 0.45) {
      double q = (fabs(swing) - 0.45) / 0.05;
      if (q > 1.0) q = 1.0;
      ellipseRGBA(surface, (int)ex, (int)ey, (int)(s * 0.01 + q * s * 0.03),
                  (int)(s * 0.01 + q * s * 0.03), 255, 230, 160,
                  weapon_alpha(150.0 * (1.0 - q * 0.5) * event->fade));
      weapon_glow(surface, ex, ey, 1.2, glow, 170.0 * event->fade);
    }
  }
}


// A hedge maze of dark walls; a tiny lantern wanders its paths and, at the end,
// the centre blooms with light.
static void hill_maze(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 46));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(235.0 * event->fade);
  double w = s * 0.11, h = s * 0.022;
  double grow = smooth_step(0.0, 0.12, event->progress);
  static const Uint8 lamp[3] = {255, 215, 130};
  static const Uint8 bloom[3] = {255, 170, 200};
  filledEllipseRGBA(surface, (int)cx, (int)(ground - 1), (int)(w * 1.1), (int)(h * 0.9) + 1, 24, 40,
                    30, weapon_alpha(200.0 * grow * event->fade));
  for (int ring = 0; ring < 3; ring++) {
    double rw = w * (1.0 - ring * 0.3), rh = h * (1.0 - ring * 0.3);
    for (int k = 0; k < 16; k++) {
      double a0 = k * 0.3927 + ring * 0.2, a1 = a0 + 0.3927;
      bool gap = ((k + ring * 5) % 7 == 0);
      if (gap) continue;
      double x0 = cx + cos(a0) * rw, y0 = ground - 1 + sin(a0) * rh;
      double x1 = cx + cos(a1) * rw, y1 = ground - 1 + sin(a1) * rh;
      double top = s * 0.022 * grow;
      Uint8 g = (Uint8)(50 + ring * 10);
      filledTrigonRGBA(surface, (int)x0, (int)y0, (int)x1, (int)y1, (int)x0, (int)(y0 - top), 34, g,
                       44, alpha);
      filledTrigonRGBA(surface, (int)x1, (int)y1, (int)x1, (int)(y1 - top), (int)x0,
                       (int)(y0 - top), 40, (Uint8)(g + 14), 50, alpha);
    }
  }
  double trip = fmod(event->t * 0.1, 1.0);
  double a = trip * 6.2832 * 2.4, r = 1.0 - 0.9 * trip;
  double lx = cx + cos(a) * w * 0.85 * r, ly = ground - 1 + sin(a) * h * 0.85 * r - s * 0.006;
  weapon_glow(surface, lx, ly - s * 0.008, 2.6, lamp,
              235.0 * smooth_step(0.15, 0.25, event->progress) * event->fade);
  double end = smooth_step(0.8, 0.95, trip);
  for (int p = 0; p < 8; p++) {
    double pa = p * 0.785 + event->t;
    weapon_glow(surface, cx + cos(pa) * s * 0.012 * end,
                ground - s * 0.012 + sin(pa) * s * 0.004 * end, 1.0, bloom,
                190.0 * end * event->fade);
  }
}


// A fisherman sits on a rock beside a dark pool, casting a line; a fish rises,
// rings spread and the float bobs.
static void hill_fisher(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 47));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double fin = smooth_step(0.0, 0.1, event->progress);
  double pw = s * 0.12, ph = s * 0.02;
  filledEllipseRGBA(surface, (int)(cx + d * pw * 0.5), (int)(ground - ph * 0.3),
                    (int)(pw * fin) + 1, (int)(ph * fin) + 1, 22, 40, 62,
                    weapon_alpha(225.0 * event->fade));
  double rx = cx - d * s * 0.02;
  filledEllipseRGBA(surface, (int)rx, (int)(ground - s * 0.005), (int)(s * 0.016),
                    (int)(s * 0.008) + 1, 96, 92, 100, alpha);
  double hx = rx, hy = ground - s * 0.012;
  thickLineRGBA(surface, (int)hx, (int)hy, (int)hx, (int)(hy - s * 0.016), 3, 120, 70, 60, alpha);
  filledCircleRGBA(surface, (int)hx, (int)(hy - s * 0.022), (int)(s * 0.005) + 1, 226, 190, 160,
                   alpha);
  filledTrigonRGBA(surface, (int)(hx - d * s * 0.01), (int)(hy - s * 0.026),
                   (int)(hx + d * s * 0.01), (int)(hy - s * 0.026), (int)hx, (int)(hy - s * 0.036),
                   150, 120, 80, alpha);
  double tipx = hx + d * s * 0.07, tipy = hy - s * 0.07;
  thickLineRGBA(surface, (int)(hx + d * s * 0.006), (int)(hy - s * 0.012), (int)tipx, (int)tipy, 1,
                150, 110, 80, alpha);
  double fx = cx + d * pw * 0.55, fy = ground - ph * 0.3;
  double bite = fmod(event->t * 0.2, 1.0);
  double bob = bite > 0.8 ? sin((bite - 0.8) * 50.0) * s * 0.003 : sin(event->t * 1.5) * s * 0.0008;
  lineRGBA(surface, (int)tipx, (int)tipy, (int)fx, (int)(fy + bob), 210, 210, 220,
           (Uint8)(alpha * 0.8));
  filledCircleRGBA(surface, (int)fx, (int)(fy + bob), 2, 235, 80, 70, alpha);
  for (int r = 0; r < 3; r++) {
    double u = fmod(event->t * 0.4 + r / 3.0, 1.0);
    ellipseRGBA(surface, (int)fx, (int)fy, (int)(3 + u * s * 0.025), (int)(1 + u * s * 0.008), 190,
                215, 240, weapon_alpha(150.0 * (1.0 - u) * fin * event->fade));
  }
  if (bite > 0.8) {
    double q = (bite - 0.8) / 0.2;
    double jx = fx + d * q * s * 0.012, jy = fy - sin(q * 3.14159) * s * 0.03;
    filledEllipseRGBA(surface, (int)jx, (int)jy, (int)(s * 0.006) + 1, (int)(s * 0.003) + 1, 200,
                      215, 235, weapon_alpha(230.0 * event->fade));
  }
}


// A lone cactus-like crystal tree blooms with tiny lanterns, each one lighting
// up and floating gently away.
static void hill_lanterntree(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 48));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double grow = smooth_step(0.0, 0.12, event->progress);
  double h = s * 0.08 * grow;
  thickLineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)(ground - h), 5, 120, 100, 88, alpha);
  for (int b = 0; b < 4; b++) {
    double side = b % 2 ? 1.0 : -1.0;
    double by = ground - h * (0.4 + 0.15 * b);
    double ex = cx + side * s * (0.025 + 0.006 * b) * grow, ey = by - s * 0.02 * grow;
    thickLineRGBA(surface, (int)cx, (int)by, (int)ex, (int)ey, 3, 124, 104, 92, alpha);
  }
  for (int l = 0; l < 16; l++) {
    double u = fmod(event->t * 0.07 + hill_random(event, l, 49), 1.0);
    double born = smooth_step(0.0, 0.15, u), fade = 1.0 - smooth_step(0.75, 1.0, u);
    double side = (hill_random(event, l, 50) - 0.5) * 2.0;
    double lx = cx + side * s * 0.03 + sin(event->t * 0.5 + l) * s * 0.01 * u +
                u * event->direction * s * 0.06;
    double ly = ground - h * (0.7 + 0.3 * hill_random(event, l, 51)) - u * s * 0.18;
    Uint8 c[3] = {(Uint8)(255 - 40 * (l % 3)), (Uint8)(190 + 20 * (l % 2)),
                  (Uint8)(110 + 40 * (l % 4))};
    weapon_glow(surface, lx, ly, 3.2, c,
                235.0 * born * fade * smooth_step(0.1, 0.2, event->progress) * event->fade);
  }
}


// A blacksmith's forge: the anvil rings, a hammer rises and falls, and a fan of
// orange sparks leaps from each strike.
static void hill_forge(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int d = event->direction;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 52));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.1, event->progress);
  static const Uint8 fire[3] = {255, 160, 70};
  double w = s * 0.03 * up, h = s * 0.03 * up;
  boxRGBA(surface, (int)(cx - d * s * 0.04 - w), (int)(ground - h), (int)(cx - d * s * 0.04 + w),
          (int)ground, 90, 70, 66, alpha);
  boxRGBA(surface, (int)(cx - d * s * 0.04 - w * 0.5), (int)(ground - h * 0.7),
          (int)(cx - d * s * 0.04 + w * 0.5), (int)(ground - h * 0.2), 255, 170, 80,
          weapon_alpha(235.0 * smooth_step(0.1, 0.2, event->progress) *
                       (0.85 + 0.15 * sin(event->t * 8.0)) * event->fade));
  weapon_glow(surface, cx - d * s * 0.04, ground - h * 0.45, w * 0.7, fire,
              100.0 * smooth_step(0.1, 0.2, event->progress) * event->fade);
  double ax = cx + d * s * 0.02, ay = ground - s * 0.012;
  boxRGBA(surface, (int)(ax - s * 0.018), (int)(ay - s * 0.009), (int)(ax + s * 0.018), (int)ay,
          120, 124, 138, alpha);
  boxRGBA(surface, (int)(ax - s * 0.009), (int)ay, (int)(ax + s * 0.009), (int)ground, 100, 104,
          116, alpha);
  double beat = fmod(event->t * 1.1, 1.0);
  double swing = beat < 0.7 ? smooth_step(0.0, 0.7, beat) : 1.0 - smooth_step(0.7, 0.78, beat);
  double handx = ax - d * s * 0.05, handy = ground - s * 0.03;
  double tipx = ax - d * s * 0.004, tipy = ay - s * 0.008 - (1.0 - swing) * s * 0.05;
  thickLineRGBA(surface, (int)handx, (int)ground, (int)handx, (int)(handy - s * 0.004), 4, 150, 120,
                110, alpha);
  thickLineRGBA(surface, (int)handx, (int)handy, (int)tipx, (int)tipy, 3, 190, 150, 110, alpha);
  filledCircleRGBA(surface, (int)tipx, (int)tipy, (int)(s * 0.004) + 1, 150, 154, 168, alpha);
  filledCircleRGBA(surface, (int)handx, (int)(handy - s * 0.012), (int)(s * 0.006) + 1, 232, 196,
                   166, alpha);
  if (beat >= 0.7 && beat < 1.0) {
    double q = (beat - 0.7) / 0.3;
    for (int p = 0; p < 12; p++) {
      double a = -3.14159 * (0.1 + 0.8 * p / 11.0);
      double r = q * s * (0.025 + 0.02 * hill_random(event, p, 53));
      filledCircleRGBA(surface, (int)(ax + cos(a) * r), (int)(ay + sin(a) * r + q * q * s * 0.02),
                       1, 255, 200, 110, weapon_alpha(230.0 * (1.0 - q) * event->fade));
    }
    weapon_glow(surface, ax, ay - s * 0.004, 1.6, fire, 200.0 * (1.0 - q) * event->fade);
  }
}


// A great bonsai-like tree on a rock holds a nest of glowing fireflies-free
// "star seeds" that drift up one by one into the sky.
static void hill_seedtree(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 54));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double grow = smooth_step(0.0, 0.12, event->progress);
  static const Uint8 seed_c[3] = {200, 230, 255};
  filledEllipseRGBA(surface, (int)cx, (int)ground, (int)(s * 0.03), (int)(s * 0.009) + 1, 96, 92,
                    104, alpha);
  double h = s * 0.055 * grow;
  thickLineRGBA(surface, (int)cx, (int)ground, (int)(cx + s * 0.006), (int)(ground - h * 0.6), 4,
                120, 92, 76, alpha);
  thickLineRGBA(surface, (int)(cx + s * 0.006), (int)(ground - h * 0.6), (int)(cx - s * 0.012),
                (int)(ground - h), 3, 120, 92, 76, alpha);
  for (int c = 0; c < 5; c++) {
    double a = c * 1.25;
    filledCircleRGBA(surface, (int)(cx - s * 0.012 + cos(a) * s * 0.02 * grow),
                     (int)(ground - h + sin(a) * s * 0.012 * grow), (int)(s * 0.014 * grow) + 1, 54,
                     116, 70, alpha);
  }
  for (int k = 0; k < 14; k++) {
    double u = fmod(event->t * 0.06 + hill_random(event, k, 55), 1.0);
    double born = smooth_step(0.0, 0.1, u), fade = 1.0 - smooth_step(0.8, 1.0, u);
    double sx = cx - s * 0.012 + sin(event->t * 0.7 + k) * s * 0.02 * u +
                (hill_random(event, k, 56) - 0.5) * s * 0.03;
    double sy = ground - h - u * s * 0.22;
    weapon_glow(surface, sx, sy, 2.4, seed_c,
                230.0 * born * fade * smooth_step(0.1, 0.2, event->progress) * event->fade);
    if (u > 0.3)
      weapon_glow(surface, sx + sin(u * 20.0) * 1.5, sy + 6, 0.8, seed_c,
                  90.0 * born * fade * event->fade);
  }
}


// A stone circle on the hilltop: each standing stone takes a turn glowing with
// a vertical beam, then all pulse together.
static void hill_stonecircle(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 57));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  const int n = 8;
  double rx = s * 0.12, ry = s * 0.018;
  static const Uint8 beam[3] = {150, 235, 255};
  double all = smooth_step(0.6, 0.7, event->progress) * (0.6 + 0.4 * sin(event->t * 3.0));
  for (int i = 0; i < n; i++) {
    double a = i * 6.2832 / n + 0.3;
    double sx = cx + cos(a) * rx, sy = ground - 1 + sin(a) * ry;
    double up = smooth_step(0.0 + i * 0.02, 0.08 + i * 0.02, event->progress);
    double h = s * (0.035 + 0.012 * (i % 3)) * up, w = s * 0.007;
    boxRGBA(surface, (int)(sx - w), (int)(sy - h), (int)(sx + w), (int)sy, 96, 94, 104, alpha);
    boxRGBA(surface, (int)(sx - w), (int)(sy - h), (int)(sx - w * 0.2), (int)sy, 122, 120, 130,
            alpha);
    double turn = fmod(event->t * 0.5, (double)n);
    double lit = (turn >= i && turn < i + 1) ? 1.0 - fabs(turn - i - 0.5) * 2.0 : 0.0;
    lit = fmax(lit * smooth_step(0.15, 0.25, event->progress), all);
    if (lit > 0.02) {
      for (int k = 0; k < 14; k++) {
        double f = k / 14.0;
        double bw = 3.0 * (1.0 - f) + 1.0;
        boxRGBA(surface, (int)(sx - bw), (int)(sy - h - (f + 0.07) * s * 0.2), (int)(sx + bw),
                (int)(sy - h - f * s * 0.2), beam[0], beam[1], beam[2],
                weapon_alpha(150.0 * lit * (1.0 - f) * event->fade));
      }
      weapon_glow(surface, sx, sy - h, 3.0, beam, 235.0 * lit * event->fade);
    }
  }
}


// A sandcastle by a tide pool: turrets rise grain by grain, a tiny flag waves,
// and a wave washes in and trims one tower.
static void hill_sandcastle(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 58));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double build = smooth_step(0.0, 0.3, event->progress);
  double wave = fmod(event->t * 0.12, 1.0);
  double wash = wave > 0.7 ? sin((wave - 0.7) / 0.3 * 3.14159) : 0.0;
  for (int i = 0; i < 3; i++) {
    double tx = cx + (i - 1) * s * 0.03;
    double h = s * (0.022 + (i == 1 ? 0.018 : 0.0)) * build;
    double w = s * 0.011;
    boxRGBA(surface, (int)(tx - w), (int)(ground - h), (int)(tx + w), (int)ground, 214, 188, 140,
            alpha);
    boxRGBA(surface, (int)(tx - w), (int)(ground - h), (int)(tx - w * 0.3), (int)ground, 232, 206,
            160, alpha);
    for (int c = -1; c <= 1; c += 2)
      boxRGBA(surface, (int)(tx + c * w * 0.7 - 1), (int)(ground - h - s * 0.004),
              (int)(tx + c * w * 0.7 + 1), (int)(ground - h), 214, 188, 140, alpha);
    boxRGBA(surface, (int)(tx - 1), (int)(ground - h * 0.6), (int)(tx + 1),
            (int)(ground - h * 0.35), 70, 58, 48, alpha);
  }
  double mx = cx, my = ground - s * 0.04 * build;
  lineRGBA(surface, (int)mx, (int)my, (int)mx, (int)(my - s * 0.015), 160, 150, 140, alpha);
  double flap = sin(event->t * 6.0) * s * 0.002;
  filledTrigonRGBA(surface, (int)mx, (int)(my - s * 0.015), (int)mx, (int)(my - s * 0.009),
                   (int)(mx + event->direction * s * 0.01 + flap), (int)(my - s * 0.012), 230, 80,
                   80, alpha);
  filledEllipseRGBA(surface, (int)(cx + event->direction * s * 0.07), (int)(ground - 1),
                    (int)(s * 0.035), (int)(s * 0.006) + 1, 40, 80, 120,
                    weapon_alpha(210.0 * event->fade));
  if (wash > 0.0) {
    filledEllipseRGBA(surface, (int)(cx + event->direction * s * 0.03 * (1.0 - wash)),
                      (int)(ground - 1), (int)(s * (0.05 + 0.06 * wash)),
                      (int)(s * 0.008 * wash) + 1, 150, 200, 235,
                      weapon_alpha(170.0 * wash * event->fade));
    for (int f = 0; f < 8; f++)
      filledCircleRGBA(surface, (int)(cx + (f - 3.5) * s * 0.012), (int)(ground - 2 - wash * 2.0),
                       1, 240, 248, 255, weapon_alpha(220.0 * wash * event->fade));
  }
}


// A music box on a stump opens: a tiny dancer turns on its lid and notes float
// up from it in glowing wisps.
static void hill_musicbox(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 59));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  double up = smooth_step(0.0, 0.1, event->progress);
  static const Uint8 note[3] = {255, 225, 150};
  double sh = s * 0.03 * up;
  filledEllipseRGBA(surface, (int)cx, (int)(ground - sh), (int)(s * 0.032), (int)(s * 0.009) + 1,
                    150, 118, 100, alpha);
  boxRGBA(surface, (int)(cx - s * 0.028), (int)(ground - sh), (int)(cx + s * 0.028), (int)ground,
          128, 98, 82, alpha);
  double bh = s * 0.024 * up;
  boxRGBA(surface, (int)(cx - s * 0.026), (int)(ground - sh - bh), (int)(cx + s * 0.026),
          (int)(ground - sh), 190, 70, 84, alpha);
  boxRGBA(surface, (int)(cx - s * 0.026), (int)(ground - sh - bh - 3), (int)(cx + s * 0.026),
          (int)(ground - sh - bh), 225, 190, 100, alpha);
  double open = smooth_step(0.1, 0.2, event->progress);
  if (open > 0.0) {
    double dx = cx, dy = ground - sh - bh - s * 0.012 * open;
    double spin = event->t * 3.0;
    thickLineRGBA(surface, (int)dx, (int)(dy + s * 0.012), (int)(dx + cos(spin) * s * 0.004),
                  (int)dy, 2, 245, 235, 245, alpha);
    filledCircleRGBA(surface, (int)(dx + cos(spin) * s * 0.003), (int)(dy - s * 0.004),
                     (int)(s * 0.003) + 1, 245, 225, 210, alpha);
    double skirt = fabs(cos(spin)) * s * 0.008 + s * 0.002;
    filledTrigonRGBA(surface, (int)(dx - skirt), (int)(dy + s * 0.008), (int)(dx + skirt),
                     (int)(dy + s * 0.008), (int)dx, (int)dy, 240, 160, 190, alpha);
    for (int n = 0; n < 8; n++) {
      double u = fmod(event->t * 0.15 + n / 8.0, 1.0);
      double nx = dx + sin(event->t * 1.2 + n * 1.4) * s * 0.02 * (0.3 + u), ny = dy - u * s * 0.16;
      weapon_glow(surface, nx, ny, 2.8, note, 235.0 * (1.0 - u) * open * event->fade);
      thickLineRGBA(surface, (int)(nx + 2), (int)ny, (int)(nx + 2), (int)(ny - 6), 1, note[0],
                    note[1], note[2], weapon_alpha(200.0 * (1.0 - u) * open * event->fade));
    }
  }
}


// A tall spiral staircase of light winds up from a hilltop dais into the sky,
// steps lighting one after another.
static void hill_lightstairs(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.3 + 0.4 * hill_random(event, 0, 60));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(240.0 * event->fade);
  static const Uint8 step_c[3] = {190, 225, 255};
  filledEllipseRGBA(surface, (int)cx, (int)(ground - 1), (int)(s * 0.07), (int)(s * 0.012) + 1, 140,
                    138, 152, alpha);
  const int steps = 40;
  double reach = smooth_step(0.05, 0.6, event->progress) * steps;
  for (int i = 0; i < steps; i++) {
    if (i > reach) break;
    double f = (double)i / steps;
    double a = i * 0.42 + event->t * 0.3;
    double sx = cx + cos(a) * s * 0.06, sy = ground - 4 - f * s * 0.34 + sin(a) * s * 0.014;
    bool front = sin(a) > 0;
    double pulse = 0.55 + 0.45 * sin(event->t * 3.0 - i * 0.5);
    double fade = 1.0 - f * 0.6;
    weapon_glow(surface, sx, sy, front ? 3.4 : 2.0, step_c,
                235.0 * pulse * fade * (front ? 1.0 : 0.55) * event->fade);
    if (i % 4 == 0)
      lineRGBA(surface, (int)cx, (int)sy, (int)sx, (int)sy, 170, 205, 240,
               weapon_alpha(110.0 * fade * event->fade));
  }
  thickLineRGBA(surface, (int)cx, (int)(ground - 2), (int)cx,
                (int)(ground - 4 - s * 0.34 * smooth_step(0.05, 0.6, event->progress)), 2, 150, 190,
                235, weapon_alpha(120.0 * event->fade));
}


// A hilltop brass armillary sphere: three tilted concentric rings spin around
// a glowing sun-core while tiny planet beads orbit along the ecliptic.
static void hill_armillary(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 0, 61));
  double ground = hill_ground(event, cx);
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 brass[3] = {245, 200, 110};
  static const Uint8 sun_c[3] = {255, 230, 155};
  static const Uint8 star_c[3] = {175, 225, 255};
  // Stone plinth & cradle arc
  filledTrigonRGBA(surface, (int)(cx - s * 0.032), (int)(ground + 2), (int)(cx + s * 0.032),
                   (int)(ground + 2), (int)cx, (int)(ground - s * 0.036), 42, 46, 58, alpha);
  boxRGBA(surface, (int)(cx - s * 0.036), (int)(ground - 3), (int)(cx + s * 0.036),
          (int)(ground + 2), 54, 58, 72, alpha);
  double cy = ground - s * 0.095;
  double r = s * 0.062;
  thickLineRGBA(surface, (int)cx, (int)(ground - s * 0.032), (int)cx, (int)(cy + r * 0.55), 3, 165,
                130, 68, alpha);
  // Tilted meridian stand
  circleRGBA(surface, (int)cx, (int)cy, (int)(r * 1.04), 175, 138, 72,
             weapon_alpha(180.0 * event->fade));
  // Three rotating elliptical rings at distinct inclinations
  for (int ring = 0; ring < 3; ring++) {
    double tilt = -0.45 + ring * 0.55;
    double spin = event->t * (0.45 + ring * 0.28) * (ring == 1 ? -1 : 1);
    double rx = r * (0.96 - ring * 0.12);
    double ry = rx * (0.26 + 0.55 * fabs(cos(spin)));
    int segs = 32;
    for (int i = 0; i < segs; i++) {
      double a0 = (2.0 * M_PI * i) / segs;
      double a1 = (2.0 * M_PI * (i + 1)) / segs;
      double x0 = cos(a0) * rx, y0 = sin(a0) * ry;
      double x1 = cos(a1) * rx, y1 = sin(a1) * ry;
      double px0 = cx + x0 * cos(tilt) - y0 * sin(tilt);
      double py0 = cy + x0 * sin(tilt) + y0 * cos(tilt);
      double px1 = cx + x1 * cos(tilt) - y1 * sin(tilt);
      double py1 = cy + x1 * sin(tilt) + y1 * cos(tilt);
      bool front = sin(a0) > 0;
      Uint8 ra = weapon_alpha((front ? 225.0 : 105.0) * event->fade);
      thickLineRGBA(surface, (int)px0, (int)py0, (int)px1, (int)py1, front ? 2 : 1,
                    brass[0] - ring * 18, brass[1] - ring * 15, brass[2] + ring * 18, ra);
    }
    // Orbiting planet bead on each ring
    double pa = event->t * (0.9 + ring * 0.4) + ring * 2.1;
    double bx = cos(pa) * rx, by = sin(pa) * ry;
    double px = cx + bx * cos(tilt) - by * sin(tilt);
    double py = cy + bx * sin(tilt) + by * cos(tilt);
    weapon_glow(surface, px, py, 2.4, ring == 2 ? star_c : brass, 220.0 * event->fade);
  }
  // Glowing sun-core at the center
  double pulse = 0.85 + 0.15 * sin(event->t * 2.8);
  weapon_glow(surface, cx, cy, s * 0.011 * pulse, sun_c, 240.0 * event->fade);
  filledCircleRGBA(surface, (int)cx, (int)cy, (int)(s * 0.007) + 1, 255, 245, 200, alpha);
}


// 2001 / Lovecraftian cyclopean monolith: an impossibly sharp, pitch-black
// slab rises silently from the ridge; subtle geometric fractures pulse with
// cold lunar cyan while nearby dust motes drift upward in defiance of gravity.
static void hill_monolith(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 0, 62));
  double ground = hill_ground(event, cx);
  double rise = smooth_step(0.05, 0.35, event->progress);
  double h = s * 0.19 * rise;
  double w = s * 0.036;
  Uint8 alpha = weapon_alpha(248.0 * event->fade);
  static const Uint8 rune_c[3] = {120, 210, 235};
  if (h < 3.0) return;
  // Ground fracture glow at the base
  filledEllipseRGBA(surface, (int)cx, (int)(ground + 1), (int)(w * 1.4), (int)(s * 0.009) + 1, 70,
                    140, 175, weapon_alpha(55.0 * rise * event->fade));
  // Pitch-black 1:4:9 slab silhouette with a razor-thin cold highlight edge
  double top = ground - h;
  boxRGBA(surface, (int)(cx - w * 0.5), (int)top, (int)(cx + w * 0.5), (int)(ground + 2), 6, 7, 12,
          alpha);
  lineRGBA(surface, (int)(cx - w * 0.5), (int)top, (int)(cx - w * 0.5), (int)ground, 110, 175, 205,
           weapon_alpha(150.0 * event->fade));
  lineRGBA(surface, (int)(cx - w * 0.5), (int)top, (int)(cx + w * 0.5), (int)top, 140, 205, 230,
           weapon_alpha(175.0 * event->fade));
  // Subtle non-Euclidean geometric seams pulsing along the face
  double hum = 0.45 + 0.55 * sin(event->t * 1.6);
  for (int i = 1; i <= 3; i++) {
    double sy = top + h * (i * 0.23);
    double inset = w * 0.22;
    lineRGBA(surface, (int)(cx - inset), (int)sy, (int)(cx + inset), (int)sy, rune_c[0], rune_c[1],
             rune_c[2], weapon_alpha(130.0 * hum * rise * event->fade));
  }
  // Vertical hairline & zenith pinpoint
  lineRGBA(surface, (int)cx, (int)(top + h * 0.14), (int)cx, (int)(top + h * 0.82), rune_c[0],
           rune_c[1], rune_c[2], weapon_alpha(95.0 * hum * rise * event->fade));
  weapon_glow(surface, cx, top - s * 0.012, 2.2, rune_c, 180.0 * hum * rise * event->fade);
  // Gravity-defying dust motes rising around the monolith
  for (int m = 0; m < 14; m++) {
    double u = fmod(event->t * 0.14 + hill_random(event, m, 3), 1.0);
    double mx =
        cx + (hill_random(event, m, 4) - 0.5) * w * 3.4 + sin(event->t * 1.1 + m) * s * 0.006;
    double my = ground - u * (h * 1.25);
    weapon_glow(surface, mx, my, 1.3, rune_c, 165.0 * sin(u * M_PI) * rise * event->fade);
  }
}


// Lovecraftian abyssal horizon: vast, silent tendrils rise slowly from behind
// the ridge like the silhouette of something submerged in deep fog, with faint
// cold-phosphorescent eyes opening and closing near the crest.
static void hill_abyssal_reach(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 0, 63));
  double ground = hill_ground(event, cx);
  double reach = sin(smooth_step(0.04, 0.96, event->progress) * M_PI);
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 abyss_eye[3] = {155, 225, 200};
  // Low mist pooling along the ridge
  for (int m = 0; m < 5; m++) {
    double mx = cx + (m - 2) * s * 0.055;
    filledEllipseRGBA(surface, (int)mx, (int)(ground + 2), (int)(s * 0.065), (int)(s * 0.014) + 1,
                      22, 32, 42, weapon_alpha(55.0 * reach * event->fade));
  }
  // Five slow, tapering silhouettes rising and swaying from behind the hill
  for (int t = 0; t < 5; t++) {
    double base_x = cx + (t - 2) * s * 0.045;
    double len = s * (0.11 + 0.055 * hill_random(event, t, 1)) * reach;
    double base_w = s * (0.013 - fabs(t - 2) * 0.0018);
    double px = base_x, py = ground + 3;
    const int segs = 10;
    for (int i = 1; i <= segs; i++) {
      double u = (double)i / segs;
      double sway = sin(event->t * (0.65 + t * 0.09) + t * 1.4 - u * 2.2) * s * 0.022 * u;
      double nx = base_x + sway + (t - 2) * s * 0.008 * u;
      double ny = ground + 3 - len * u;
      int thick = (int)(base_w * (1.0 - u * 0.85)) + 1;
      thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, thick, 8, 11, 18, alpha);
      px = nx;
      py = ny;
    }
  }
  // Faint, slow-blinking eyes low along the ridge mist
  for (int e = 0; e < 3; e++) {
    double blink = sin(event->t * 0.7 + e * 2.3);
    if (blink <= 0.15) continue;
    double ex = cx + (e - 1) * s * 0.038;
    double ey = ground - s * (0.018 + 0.008 * (e % 2));
    weapon_glow(surface, ex, ey, 1.8, abyss_eye, 160.0 * blink * reach * event->fade);
  }
}


// "Hyperion" (Dan Simmons): the Tree of Thorns rises in dark chrome silhouette
// on the ridge, its metallic branches glinting with cold crimson sparks while
// faint time-tide motes drift around its canopy.
static void hill_shrikes_tree(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 0, 64));
  double ground = hill_ground(event, cx);
  double rise = smooth_step(0.05, 0.35, event->progress);
  double h = s * 0.18 * rise;
  Uint8 alpha = weapon_alpha(244.0 * event->fade);
  static const Uint8 thorn_red[3] = {225, 65, 75};
  static const Uint8 chrome_c[3] = {185, 200, 220};
  if (h < 3.0) return;
  // Gnarled metallic trunk
  thickLineRGBA(surface, (int)cx, (int)(ground + 2), (int)cx, (int)(ground - h * 0.55), 3, 28, 32,
                42, alpha);
  lineRGBA(surface, (int)(cx - 1), (int)ground, (int)(cx - 1), (int)(ground - h * 0.55),
           chrome_c[0], chrome_c[1], chrome_c[2], weapon_alpha(110.0 * event->fade));
  // Jagged chrome branches & thorn-spines
  for (int b = 0; b < 7; b++) {
    double frac = 0.35 + (b / 6.0) * 0.55;
    double by = ground - h * frac;
    int side = (b % 2 == 0) ? 1 : -1;
    double blen = h * (0.38 - frac * 0.18);
    double bx = cx + side * blen;
    double btip_y = by - blen * 0.48;
    thickLineRGBA(surface, (int)cx, (int)by, (int)bx, (int)btip_y, 2, 26, 30, 40, alpha);
    lineRGBA(surface, (int)cx, (int)by, (int)bx, (int)btip_y, chrome_c[0], chrome_c[1], chrome_c[2],
             weapon_alpha(125.0 * event->fade));
    // Cold crimson glint at branch tips
    double glint = 0.35 + 0.65 * fabs(sin(event->t * 1.4 + b * 1.7));
    weapon_glow(surface, bx, btip_y, 1.8, thorn_red, 175.0 * glint * rise * event->fade);
  }
  // Crown spire
  thickLineRGBA(surface, (int)cx, (int)(ground - h * 0.55), (int)cx, (int)(ground - h), 2, 30, 34,
                46, alpha);
  weapon_glow(surface, cx, ground - h, 2.1, chrome_c, 150.0 * rise * event->fade);
}


// "Annihilation" (Jeff VanderMeer): a weathered, pale coastal watchtower
// stands on the ridge; instead of a normal lantern, a strange spiral of
// living bioluminescent script winds slowly up its walls and breathes out
// into the night air.
static void hill_pale_lighthouse(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.26 + 0.48 * hill_random(event, 0, 65));
  double ground = hill_ground(event, cx);
  double h = s * 0.165;
  double bw = s * 0.022, tw = s * 0.013;
  Uint8 alpha = weapon_alpha(238.0 * event->fade);
  static const Uint8 bio_c[3] = {130, 240, 175};
  // Tapered pale-stone tower silhouette
  filledTrigonRGBA(surface, (int)(cx - bw), (int)(ground + 2), (int)(cx + bw), (int)(ground + 2),
                   (int)(cx - tw), (int)(ground - h), 48, 56, 60, alpha);
  filledTrigonRGBA(surface, (int)(cx + bw), (int)(ground + 2), (int)(cx + tw), (int)(ground - h),
                   (int)(cx - tw), (int)(ground - h), 48, 56, 60, alpha);
  boxRGBA(surface, (int)(cx - tw * 1.15), (int)(ground - h - s * 0.014), (int)(cx + tw * 1.15),
          (int)(ground - h), 32, 38, 44, alpha);
  // Spiraling bioluminescent script ascending the tower
  for (int i = 0; i < 12; i++) {
    double u = (double)i / 11.0;
    double phase_u = fmod(event->t * 0.25 + u, 1.0);
    double w = bw * (1.0 - u) + tw * u;
    double sx = cx + sin(u * 10.0 + event->t * 0.6) * w * 0.85;
    double sy = ground - u * h * 0.92;
    weapon_glow(surface, sx, sy, 1.7, bio_c, (110.0 + 115.0 * sin(phase_u * M_PI)) * event->fade);
  }
  // Eerie, unfocused green-gold lantern pulse
  double pulse = 0.55 + 0.45 * sin(event->t * 1.3);
  weapon_glow(surface, cx, ground - h - s * 0.008, s * 0.016 * pulse, bio_c, 190.0 * event->fade);
}


// Weird maritime horror (Hodgson / Mieville): the dark, tilted hulk of an
// ancient stranded galleon rests high on the ridge, its broken masts draped
// in tattered rigging while cold St. Elmo's fire flickers at the yardarms.
static void hill_derelict_galleon(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 0, 66));
  double ground = hill_ground(event, cx);
  double hw = s * 0.068, hh = s * 0.024;
  Uint8 alpha = weapon_alpha(242.0 * event->fade);
  static const Uint8 elmo[3] = {125, 205, 245};
  // Tilted hull silhouette
  filledTrigonRGBA(surface, (int)(cx - hw), (int)(ground - hh * 0.5), (int)(cx + hw * 0.9),
                   (int)(ground - hh * 0.8), (int)(cx - hw * 0.45), (int)(ground + 3), 16, 18, 26,
                   alpha);
  filledTrigonRGBA(surface, (int)(cx + hw * 0.9), (int)(ground - hh * 0.8), (int)(cx + hw * 0.55),
                   (int)(ground + 3), (int)(cx - hw * 0.45), (int)(ground + 3), 16, 18, 26, alpha);
  // Two canted, broken masts and yardarms
  for (int m = 0; m < 2; m++) {
    double mx = cx + (m == 0 ? -hw * 0.2 : hw * 0.28);
    double my0 = ground - hh * 0.5;
    double mh = s * (m == 0 ? 0.11 : 0.085);
    double lean = s * (m == 0 ? -0.016 : -0.011);
    double top_x = mx + lean, top_y = my0 - mh;
    thickLineRGBA(surface, (int)mx, (int)my0, (int)top_x, (int)top_y, 2, 22, 25, 34, alpha);
    double y_y = my0 - mh * 0.65;
    double y_x = mx + lean * 0.65;
    double yw = s * (m == 0 ? 0.032 : 0.022);
    lineRGBA(surface, (int)(y_x - yw), (int)(y_y + yw * 0.15), (int)(y_x + yw),
             (int)(y_y - yw * 0.15), 26, 30, 40, alpha);
    // Faint St. Elmo's fire at the masthead & yardarm tips
    double flick = 0.4 + 0.6 * fabs(sin(event->t * 2.4 + m * 1.9));
    weapon_glow(surface, top_x, top_y, 1.7, elmo, 165.0 * flick * event->fade);
    weapon_glow(surface, y_x - yw, y_y + yw * 0.15, 1.2, elmo, 115.0 * flick * event->fade);
  }
}


// Weird fiction threshold (Blackwood / Machen): an ancient, crooked stone
// archway stands alone on the ridge; inside its frame, a different, deeper
// starfield of pale violet-blue stars gleams where the hill should be.
static void hill_threshold_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 0, 67));
  double ground = hill_ground(event, cx);
  double aw = s * 0.038, ah = s * 0.088;
  Uint8 alpha = weapon_alpha(242.0 * event->fade);
  static const Uint8 rift_c[3] = {165, 150, 240};
  // Deep void interior inside the arch
  boxRGBA(surface, (int)(cx - aw * 0.65), (int)(ground - ah * 0.85), (int)(cx + aw * 0.65),
          (int)(ground + 2), 5, 4, 14, alpha);
  filledEllipseRGBA(surface, (int)cx, (int)(ground - ah * 0.85), (int)(aw * 0.65), (int)(aw * 0.45),
                    5, 4, 14, alpha);
  // Crooked megalithic pillars and lintel
  thickLineRGBA(surface, (int)(cx - aw * 0.82), (int)(ground + 2), (int)(cx - aw * 0.72),
                (int)(ground - ah * 0.88), (int)(s * 0.012) + 2, 34, 36, 46, alpha);
  thickLineRGBA(surface, (int)(cx + aw * 0.82), (int)(ground + 2), (int)(cx + aw * 0.72),
                (int)(ground - ah * 0.88), (int)(s * 0.012) + 2, 34, 36, 46, alpha);
  thickLineRGBA(surface, (int)(cx - aw * 0.96), (int)(ground - ah * 0.88), (int)(cx + aw * 0.96),
                (int)(ground - ah * 0.94), (int)(s * 0.014) + 2, 38, 40, 52, alpha);
  // Alien stars gleaming through the doorway
  double hum = 0.65 + 0.35 * sin(event->t * 1.4);
  for (int i = 0; i < 9; i++) {
    double sx = cx + (hill_random(event, i, 10) - 0.5) * aw * 1.05;
    double sy = ground - (0.15 + 0.78 * hill_random(event, i, 11)) * ah;
    double tw = 0.5 + 0.5 * sin(event->t * 2.2 + i * 1.7);
    weapon_glow(surface, sx, sy, 1.3, rift_c, 195.0 * tw * hum * event->fade);
  }
}


// "The Crystal World" (J.G. Ballard): a grove of petrified, vitrified trees
// crystallizes along the ridge, catching starlight in sharp prismatic facets
// of pale ice-cyan and amber-rose.
static void hill_glass_forest(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.26 + 0.48 * hill_random(event, 0, 68));
  double spread = s * 0.22;
  double grow = smooth_step(0.04, 0.45, event->progress);
  static const Uint8 ice_c[3] = {170, 235, 250};
  static const Uint8 rose_c[3] = {235, 185, 225};
  for (int t = 0; t < 7; t++) {
    double tx = cx + (t - 3) * (spread / 6.0) + (hill_random(event, t, 1) - 0.5) * s * 0.015;
    double ground = hill_ground(event, tx);
    double h = s * (0.055 + 0.065 * hill_random(event, t, 2)) * grow;
    if (h < 2.0) continue;
    Uint8 ta = weapon_alpha(190.0 * event->fade);
    thickLineRGBA(surface, (int)tx, (int)(ground + 2), (int)tx, (int)(ground - h), 2, 130, 185, 210,
                  ta);
    for (int b = 0; b < 3; b++) {
      double by = ground - h * (0.45 + b * 0.2);
      int dir = ((t + b) % 2 == 0) ? 1 : -1;
      double bx = tx + dir * h * (0.36 - b * 0.08);
      double tip_y = by - h * 0.26;
      lineRGBA(surface, (int)tx, (int)by, (int)bx, (int)tip_y, 175, 225, 245, ta);
      double glint = 0.25 + 0.75 * fabs(sin(event->t * 1.5 + t * 1.3 + b));
      weapon_glow(surface, bx, tip_y, 1.4, (t + b) % 3 == 0 ? rose_c : ice_c,
                  155.0 * glint * grow * event->fade);
    }
  }
}


// A sunken tidal bell tower rises from the ridge mist, its bronze bell
// swinging in slow arcs and sending golden acoustic shock-rings outward.
static void hill_tidal_bell(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 0, 103));
  double ground = hill_ground(event, cx);
  double h = s * 0.135;
  double w = s * 0.036;
  Uint8 stone_a = weapon_alpha(215.0 * event->fade);
  // Tapered stone spire
  filledTrigonRGBA(surface, (int)(cx - w * 0.6), (int)(ground + 2), (int)(cx + w * 0.6),
                   (int)(ground + 2), (int)cx, (int)(ground - h), 34, 42, 54, stone_a);
  // Open belfry arch near the top
  double bell_y = ground - h * 0.72;
  filledCircleRGBA(surface, (int)cx, (int)bell_y, (int)(w * 0.28), 14, 18, 28, stone_a);
  // Swinging bronze bell
  double swing = sin(event->t * 1.9) * w * 0.22;
  filledCircleRGBA(surface, (int)(cx + swing), (int)(bell_y + 2), (int)(w * 0.16) + 1, 212, 168, 86,
                   weapon_alpha(230.0 * event->fade));
  // Expanding acoustic rings on each peak swing
  double toll = fmod(event->t * 0.6048, 1.0);
  double rr = s * (0.02 + toll * 0.14);
  circleRGBA(surface, (int)cx, (int)bell_y, (int)rr, 245, 205, 118,
             weapon_alpha(145.0 * (1.0 - toll) * event->fade));
}


// "The Martian Chronicles" (Ray Bradbury): a slender sand-skiff with a
// translucent brass-ribbed sail glides silently along the ridge crest,
// trailed by pale dust motes and a warm prow lantern.
static void hill_canal_skiff(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double x = dir > 0 ? surface->w * (0.08 + 0.84 * event->progress)
                     : surface->w * (0.92 - 0.84 * event->progress);
  double ground = hill_ground(event, x);
  double hull = s * 0.055;
  double mast = s * 0.068;
  Uint8 hull_a = weapon_alpha(220.0 * event->fade);
  // Crescent skiff hull skimming the ridge
  thickLineRGBA(surface, (int)(x - hull * 0.55), (int)(ground - 3), (int)(x + hull * 0.55),
                (int)(ground - 3), 3, 28, 34, 44, hull_a);
  // Mast and billowing amber-tinted sail
  lineRGBA(surface, (int)x, (int)(ground - 3), (int)x, (int)(ground - mast), 176, 138, 86, hull_a);
  filledTrigonRGBA(surface, (int)x, (int)(ground - mast), (int)x, (int)(ground - mast * 0.25),
                   (int)(x + dir * hull * 0.52), (int)(ground - mast * 0.42), 188, 154, 102,
                   weapon_alpha(110.0 * event->fade));
  // Prow lantern
  static const Uint8 lantern[3] = {250, 205, 115};
  weapon_glow(surface, x + dir * hull * 0.55, ground - 4, 1.8, lantern, 185.0 * event->fade);
}


// Star-crystal observatory: a stepped basalt dome on the ridge opens its
// brass shutter to project a soft indigo-cyan starlight beam upward, with
// motes drifting inside the beam.
static void hill_prism_observatory(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.28 + 0.44 * hill_random(event, 1, 2));
  double ground = hill_ground(event, x);
  double r = s * 0.032;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  // Dome silhouette
  filledCircleRGBA(surface, (int)x, (int)(ground - r * 0.45), (int)r, 18, 22, 32, hull_a);
  boxRGBA(surface, (int)(x - r * 1.15), (int)(ground - r * 0.45), (int)(x + r * 1.15),
          (int)(ground + 2), 14, 18, 26, hull_a);
  // Sweeping indigo-cyan starlight beam
  double tilt = -0.35 + 0.25 * sin(event->t * 0.45);
  double bx = x + sin(tilt) * s * 0.19;
  double by = ground - r * 1.2 - cos(tilt) * s * 0.19;
  filledTrigonRGBA(surface, (int)(x - 3), (int)(ground - r * 1.1), (int)(x + 3),
                   (int)(ground - r * 1.1), (int)bx, (int)by, 125, 205, 250,
                   weapon_alpha(48.0 * event->fade));
  static const Uint8 slit[3] = {150, 225, 255};
  weapon_glow(surface, x, ground - r * 1.15, 2.2, slit, 175.0 * event->fade);
}


// Aurora windmill: a tall, slender ridge-top collector tower with three
// bioluminescent emerald-tipped blades that trace faint glowing arcs as they
// turn.
static void hill_aurora_turbine(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 3, 7));
  double ground = hill_ground(event, x);
  double h = s * 0.095;
  double blade = s * 0.048;
  double hub_y = ground - h;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)x, (int)(ground + 2), (int)x, (int)hub_y, 3, 18, 22, 32, hull_a);
  static const Uint8 tip_c[3] = {95, 240, 185};
  double rot = event->t * 1.35;
  for (int b = 0; b < 3; b++) {
    double a = rot + b * (2.0 * M_PI / 3.0);
    double tx = x + cos(a) * blade;
    double ty = hub_y + sin(a) * blade;
    thickLineRGBA(surface, (int)x, (int)hub_y, (int)tx, (int)ty, 2, 36, 46, 62, hull_a);
    weapon_glow(surface, tx, ty, 1.4, tip_c, 165.0 * event->fade);
  }
  weapon_glow(surface, x, hub_y, 1.8, tip_c, 185.0 * event->fade);
}


// Solar mirror array: three ridge-mounted heliostat dishes that track slowly
// together and bounce faint golden starlight beams to a central receiver spire.
static void hill_solar_mirror(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 4, 9));
  double spire_ground = hill_ground(event, cx);
  double spire_h = s * 0.085;
  double rec_y = spire_ground - spire_h;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)cx, (int)(spire_ground + 2), (int)cx, (int)rec_y, 3, 18, 22, 32,
                hull_a);
  static const Uint8 rec_c[3] = {255, 210, 120};
  weapon_glow(surface, cx, rec_y, 2.6, rec_c, 195.0 * event->fade);
  double tilt = 0.25 * sin(event->t * 0.35);
  for (int i = -1; i <= 1; i++) {
    if (i == 0) continue;
    double mx = cx + i * s * 0.075;
    double mg = hill_ground(event, mx);
    double my = mg - s * 0.024;
    lineRGBA(surface, (int)mx, (int)(mg + 2), (int)mx, (int)my, 20, 25, 36, hull_a);
    double span = s * 0.022;
    double dx = cos(tilt) * span;
    double dy = sin(tilt) * span;
    thickLineRGBA(surface, (int)(mx - dx), (int)(my - dy), (int)(mx + dx), (int)(my + dy), 2, 165,
                  195, 225, weapon_alpha(210.0 * event->fade));
    lineRGBA(surface, (int)mx, (int)my, (int)cx, (int)rec_y, 255, 215, 130,
             weapon_alpha(65.0 * event->fade));
  }
}


// Ether fountain: a tiered stone basin on the ridge sending up twin arcs of
// softly glowing lavender-blue luminous droplets.
static void hill_ether_fountain(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.25 + 0.50 * hill_random(event, 7, 13));
  double ground = hill_ground(event, x);
  double w = s * 0.032;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  boxRGBA(surface, (int)(x - w), (int)(ground - s * 0.010), (int)(x + w), (int)(ground + 2), 20, 24,
          34, hull_a);
  boxRGBA(surface, (int)(x - w * 0.35), (int)(ground - s * 0.025), (int)(x + w * 0.35),
          (int)(ground - s * 0.010), 26, 32, 44, hull_a);
  static const Uint8 drop_c[3] = {165, 195, 255};
  for (int i = 0; i < 14; i++) {
    double u = fmod(event->t * 0.72 + i / 14.0, 1.0);
    double side = (i % 2 == 0) ? -1.0 : 1.0;
    double spread = (0.4 + 0.5 * ((i / 2) % 3)) * w * 0.9;
    double px = x + side * spread * u;
    double py = ground - s * 0.025 - (4.0 * u * (1.0 - u)) * s * 0.055;
    double a = (1.0 - u * 0.65) * 185.0 * event->fade;
    weapon_glow(surface, px, py, 1.1, drop_c, a);
  }
}


// Signal pyre: a tall stone brazier on the ridge crest flaring with warm
// amber flames and rising wind-blown embers.
static void hill_signal_pyre(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 5, 11));
  double ground = hill_ground(event, x);
  double h = s * 0.048;
  double w = s * 0.016;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)(ground + 2), 20, 22, 30,
          hull_a);
  boxRGBA(surface, (int)(x - w * 1.35), (int)(ground - h - s * 0.010), (int)(x + w * 1.35),
          (int)(ground - h), 28, 30, 40, hull_a);
  static const Uint8 flame_c[3] = {255, 155, 65};
  double pulse = 0.85 + 0.15 * sin(event->t * 7.5);
  weapon_glow(surface, x, ground - h - s * 0.018, s * 0.016 * pulse, flame_c, 195.0 * event->fade);
  for (int i = 0; i < 8; i++) {
    double u = fmod(event->t * 0.65 + i / 8.0, 1.0);
    double ex = x + sin(event->t * 3.0 + i * 1.7) * w * 1.4;
    double ey = ground - h - s * 0.016 - u * s * 0.065;
    weapon_glow(surface, ex, ey, 1.0, flame_c, (1.0 - u) * 175.0 * event->fade);
  }
}


// Tide-lantern pier: a dark wooden jetty jutting along the ridge hollow with
// three hanging sea-green glass lanterns swaying gently in the night breeze.
static void hill_tide_lantern(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 9, 17));
  double ground = hill_ground(event, x);
  double deck_y = ground - s * 0.022;
  double span = s * 0.095;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)(x - span), (int)deck_y, (int)(x + span), (int)deck_y, 2, 22, 26, 36,
                hull_a);
  static const Uint8 lan_c[3] = {120, 235, 200};
  for (int i = -1; i <= 1; i++) {
    double px = x + i * span * 0.72;
    double pg = hill_ground(event, px);
    double top_y = deck_y - s * 0.032;
    lineRGBA(surface, (int)px, (int)(pg + 2), (int)px, (int)top_y, 20, 24, 34, hull_a);
    double sway = sin(event->t * 1.4 + i * 1.1) * s * 0.005;
    double lx = px + s * 0.012 + sway;
    double ly = top_y + s * 0.014;
    lineRGBA(surface, (int)px, (int)top_y, (int)lx, (int)ly, 65, 80, 95,
             weapon_alpha(180.0 * event->fade));
    weapon_glow(surface, lx, ly, 1.6, lan_c, 195.0 * event->fade);
  }
}


// Frost obelisk: a slender tapered ice-stone spire on the ridge crest with
// pale glacial runes pulsing softly up its spine.
static void hill_frost_obelisk(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.20 + 0.60 * hill_random(event, 11, 19));
  double ground = hill_ground(event, x);
  double h = s * 0.088;
  double bw = s * 0.015;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledTrigonRGBA(surface, (int)(x - bw), (int)(ground + 2), (int)(x + bw), (int)(ground + 2),
                   (int)x, (int)(ground - h), 20, 26, 38, hull_a);
  static const Uint8 rune_c[3] = {155, 225, 255};
  for (int i = 0; i < 4; i++) {
    double u = 0.20 + i * 0.18;
    double ry = ground - h * u;
    double pulse = 0.45 + 0.55 * sin(event->t * 2.2 - i * 0.9);
    weapon_glow(surface, x, ry, 1.3, rune_c, 185.0 * pulse * event->fade);
  }
}


// Wind harp: twin curved wooden posts on the ridge strung with vertical silver
// chords that shimmer in waves when the wind passes through them.
static void hill_wind_harp(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 13, 23));
  double ground = hill_ground(event, x);
  double w = s * 0.042;
  double h = s * 0.068;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)(x - w), (int)(ground + 2), (int)(x - w * 0.65), (int)(ground - h), 2,
                24, 28, 38, hull_a);
  thickLineRGBA(surface, (int)(x + w), (int)(ground + 2), (int)(x + w * 0.65), (int)(ground - h), 2,
                24, 28, 38, hull_a);
  thickLineRGBA(surface, (int)(x - w * 0.65), (int)(ground - h), (int)(x + w * 0.65),
                (int)(ground - h), 2, 24, 28, 38, hull_a);
  static const Uint8 chord_c[3] = {195, 220, 255};
  for (int i = -2; i <= 2; i++) {
    double cx = x + i * (w * 0.25);
    double shimmer = 0.45 + 0.55 * sin(event->t * 4.5 + i * 0.8);
    lineRGBA(surface, (int)cx, (int)ground, (int)cx, (int)(ground - h), chord_c[0], chord_c[1],
             chord_c[2], weapon_alpha(150.0 * shimmer * event->fade));
  }
}


// Lantern bridge: an arched stone footbridge spanning a dip on the ridge
// with warm amber lanterns perched along its parapet.
static void hill_lantern_bridge(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 15, 29));
  double span = s * 0.11;
  double arch_h = s * 0.032;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  static const Uint8 lan_c[3] = {255, 190, 95};
  double prev_x = x - span, prev_y = hill_ground(event, prev_x);
  for (int i = 1; i <= 12; i++) {
    double u = -1.0 + 2.0 * i / 12.0;
    double px = x + u * span;
    double py = hill_ground(event, px) - (1.0 - u * u) * arch_h;
    thickLineRGBA(surface, (int)prev_x, (int)prev_y, (int)px, (int)py, 3, 22, 26, 36, hull_a);
    if (i % 3 == 0 && i < 12) {
      double sway = sin(event->t * 1.8 + i) * s * 0.002;
      lineRGBA(surface, (int)px, (int)py, (int)px, (int)(py - s * 0.014), 28, 34, 46, hull_a);
      weapon_glow(surface, px + sway, py - s * 0.016, 1.4, lan_c, 190.0 * event->fade);
    }
    prev_x = px;
    prev_y = py;
  }
}


// Sundial gnomon: a tall angled stone pointer on the ridge crest with a
// faint starlight glint travelling along its tip.
static void hill_sun_dial(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 17, 31));
  double ground = hill_ground(event, x);
  double h = s * 0.072;
  double bw = s * 0.022;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledTrigonRGBA(surface, (int)(x - bw), (int)(ground + 2), (int)(x + bw * 0.6),
                   (int)(ground + 2), (int)(x + bw * 1.5), (int)(ground - h), 20, 24, 34, hull_a);
  static const Uint8 glint_c[3] = {220, 235, 255};
  double pulse = 0.55 + 0.45 * sin(event->t * 1.6);
  weapon_glow(surface, x + bw * 1.5, ground - h, 1.8, glint_c, 185.0 * pulse * event->fade);
}


// Glacier arch: a jagged translucent ice arch on the ridge glowing softly
// with pale cyan internal light.
static void hill_glacier_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 19, 37));
  double ground = hill_ground(event, x);
  double span = s * 0.065;
  double h = s * 0.062;
  Uint8 ice_a = weapon_alpha(195.0 * event->fade);
  filledTrigonRGBA(surface, (int)(x - span), (int)(ground + 2), (int)(x - span * 0.45),
                   (int)(ground + 2), (int)(x - span * 0.25), (int)(ground - h * 0.85), 55, 95, 125,
                   ice_a);
  filledTrigonRGBA(surface, (int)(x + span), (int)(ground + 2), (int)(x + span * 0.45),
                   (int)(ground + 2), (int)(x + span * 0.25), (int)(ground - h * 0.85), 55, 95, 125,
                   ice_a);
  thickLineRGBA(surface, (int)(x - span * 0.25), (int)(ground - h * 0.85), (int)x,
                (int)(ground - h), 4, 85, 145, 185, ice_a);
  thickLineRGBA(surface, (int)x, (int)(ground - h), (int)(x + span * 0.25),
                (int)(ground - h * 0.85), 4, 85, 145, 185, ice_a);
  static const Uint8 core_c[3] = {150, 235, 255};
  weapon_glow(surface, x, ground - h * 0.92, 2.2, core_c, 165.0 * event->fade);
}


// Beacon windmill: a silhouette ridge windmill whose slowly turning sails
// carry warm amber marker lamps at their tips.
static void hill_beacon_windmill(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 21, 41));
  double ground = hill_ground(event, x);
  double h = s * 0.075;
  double bw = s * 0.016;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledTrigonRGBA(surface, (int)(x - bw), (int)(ground + 2), (int)(x + bw), (int)(ground + 2),
                   (int)x, (int)(ground - h * 1.15), 18, 22, 32, hull_a);
  double hub_y = ground - h;
  double arm = s * 0.048;
  double rot = event->t * 0.55;
  static const Uint8 lamp_c[3] = {255, 185, 95};
  for (int i = 0; i < 4; i++) {
    double a = rot + i * (M_PI * 0.5);
    double tx = x + cos(a) * arm;
    double ty = hub_y + sin(a) * arm;
    lineRGBA(surface, (int)x, (int)hub_y, (int)tx, (int)ty, 34, 42, 58, hull_a);
    weapon_glow(surface, tx, ty, 1.2, lamp_c, 180.0 * event->fade);
  }
}


// Star basin: a shallow stone reflecting pool on the ridge crest with
// twinkling pale-blue starlight ripples across its surface.
static void hill_star_basin(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 23, 43));
  double ground = hill_ground(event, x);
  double w = s * 0.068;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  boxRGBA(surface, (int)(x - w), (int)(ground - s * 0.008), (int)(x + w), (int)(ground + 2), 20, 24,
          34, hull_a);
  lineRGBA(surface, (int)(x - w * 0.88), (int)(ground - s * 0.008), (int)(x + w * 0.88),
           (int)(ground - s * 0.008), 115, 165, 225, weapon_alpha(180.0 * event->fade));
  static const Uint8 star_c[3] = {175, 225, 255};
  for (int i = 0; i < 6; i++) {
    double u = -0.75 + 0.3 * i;
    double sx = x + u * w;
    double sy = ground - s * 0.011 - fabs(sin(event->t * 1.8 + i * 1.3)) * s * 0.012;
    double tw = 0.4 + 0.6 * sin(event->t * 3.2 + i * 1.7);
    weapon_glow(surface, sx, sy, 1.0, star_c, 165.0 * tw * event->fade);
  }
}


// Signal semaphores: twin tall ridge masts with slowly rotating mechanical
// arms tipped with alternating ruby and cyan signal lamps.
static void hill_signal_semaphores(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.26 + 0.48 * hill_random(event, 25, 47));
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  static const Uint8 lamp_c[2][3] = {{255, 110, 120}, {110, 225, 255}};
  for (int m = 0; m < 2; m++) {
    double mx = cx + (m == 0 ? -1 : 1) * s * 0.055;
    double mg = hill_ground(event, mx);
    double top = mg - s * 0.068;
    thickLineRGBA(surface, (int)mx, (int)(mg + 2), (int)mx, (int)top, 2, 22, 26, 36, hull_a);
    double ang = sin(event->t * 0.9 + m * 1.6) * 1.1 - M_PI * 0.5;
    double ax = mx + cos(ang) * s * 0.032;
    double ay = top + sin(ang) * s * 0.032;
    thickLineRGBA(surface, (int)mx, (int)top, (int)ax, (int)ay, 2, 42, 50, 66, hull_a);
    weapon_glow(surface, ax, ay, 1.4, lamp_c[m], 195.0 * event->fade);
  }
}


// Crystal geode: a split dark basalt outcrop on the ridge revealing a softly
// glowing cluster of amethyst crystal shards inside.
static void hill_crystal_geode(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 27, 53));
  double ground = hill_ground(event, x);
  double r = s * 0.036;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledTrigonRGBA(surface, (int)(x - r), (int)(ground + 2), (int)(x - r * 0.2), (int)(ground + 2),
                   (int)(x - r * 0.7), (int)(ground - r * 0.9), 20, 22, 32, hull_a);
  filledTrigonRGBA(surface, (int)(x + r), (int)(ground + 2), (int)(x + r * 0.2), (int)(ground + 2),
                   (int)(x + r * 0.7), (int)(ground - r * 0.9), 20, 22, 32, hull_a);
  static const Uint8 gem_c[3] = {205, 130, 255};
  for (int i = -1; i <= 1; i++) {
    double gx = x + i * r * 0.24;
    double gh = r * (i == 0 ? 1.15 : 0.85);
    filledTrigonRGBA(surface, (int)(gx - r * 0.14), (int)(ground + 1), (int)(gx + r * 0.14),
                     (int)(ground + 1), (int)gx, (int)(ground - gh), 145, 85, 210,
                     weapon_alpha(205.0 * event->fade));
    weapon_glow(surface, gx, ground - gh * 0.7, 1.3, gem_c, 160.0 * event->fade);
  }
}


// Hanging gardens: tiered dark stone terraces on the ridge with trailing
// luminescent emerald-gold vines swaying in the night air.
static void hill_hanging_gardens(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 29, 59));
  double ground = hill_ground(event, x);
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  static const Uint8 vine_c[3] = {130, 235, 165};
  for (int t = 0; t < 2; t++) {
    double tw = s * (0.055 - t * 0.016);
    double ty = ground - s * (0.022 + t * 0.022);
    boxRGBA(surface, (int)(x - tw), (int)ty, (int)(x + tw), (int)(ty + s * 0.016), 20, 24, 34,
            hull_a);
    for (int v = -2; v <= 2; v++) {
      double vx = x + v * tw * 0.38;
      double sway = sin(event->t * 1.6 + v + t) * s * 0.004;
      double vy = ty + s * 0.024;
      lineRGBA(surface, (int)vx, (int)(ty + s * 0.006), (int)(vx + sway), (int)vy, vine_c[0],
               vine_c[1], vine_c[2], weapon_alpha(155.0 * event->fade));
      weapon_glow(surface, vx + sway, vy, 1.0, vine_c, 150.0 * event->fade);
    }
  }
}


// Echo spire: a slender obsidian spire on the ridge emitting rising
// concentric pale-cyan acoustic rings.
static void hill_echo_spire(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 31, 61));
  double ground = hill_ground(event, x);
  double h = s * 0.082;
  double bw = s * 0.012;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledTrigonRGBA(surface, (int)(x - bw), (int)(ground + 2), (int)(x + bw), (int)(ground + 2),
                   (int)x, (int)(ground - h), 18, 20, 30, hull_a);
  static const Uint8 ring_c[3] = {135, 225, 255};
  for (int i = 0; i < 3; i++) {
    double u = fmod(event->t * 0.5 + i / 3.0, 1.0);
    double ry = ground - h - u * s * 0.045;
    double rx = s * (0.008 + u * 0.032);
    ellipseRGBA(surface, (int)x, (int)ry, (int)rx, (int)(rx * 0.35), ring_c[0], ring_c[1],
                ring_c[2], weapon_alpha((1.0 - u) * 185.0 * event->fade));
  }
}


// Amber kiln: a domed brick kiln on the ridge glowing warmly from its
// arched mouth with rising sparks.
static void hill_amber_kiln(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 33, 67));
  double ground = hill_ground(event, x);
  double r = s * 0.032;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledCircleRGBA(surface, (int)x, (int)(ground - r * 0.35), (int)r, 24, 22, 28, hull_a);
  boxRGBA(surface, (int)(x - r), (int)(ground - r * 0.35), (int)(x + r), (int)(ground + 2), 24, 22,
          28, hull_a);
  static const Uint8 fire_c[3] = {255, 165, 75};
  weapon_glow(surface, x, ground - r * 0.25, r * 0.45, fire_c, 195.0 * event->fade);
  for (int i = 0; i < 6; i++) {
    double u = fmod(event->t * 0.6 + i / 6.0, 1.0);
    double sx = x + sin(event->t * 2.5 + i * 1.9) * r * 0.6;
    double sy = ground - r * 1.3 - u * s * 0.05;
    weapon_glow(surface, sx, sy, 1.0, fire_c, (1.0 - u) * 165.0 * event->fade);
  }
}


// Starlight mill: a tall mill tower on the ridge with a glowing blue crystal
// wheel turning slowly.
static void hill_starlight_mill(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 35, 71));
  double ground = hill_ground(event, x);
  double h = s * 0.072;
  double bw = s * 0.018;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledTrigonRGBA(surface, (int)(x - bw), (int)(ground + 2), (int)(x + bw), (int)(ground + 2),
                   (int)x, (int)(ground - h * 1.15), 20, 24, 36, hull_a);
  double hy = ground - h * 0.85;
  double r = s * 0.028;
  static const Uint8 crys_c[3] = {135, 210, 255};
  circleRGBA(surface, (int)x, (int)hy, (int)r, crys_c[0], crys_c[1], crys_c[2],
             weapon_alpha(135.0 * event->fade));
  double rot = event->t * 0.7;
  for (int i = 0; i < 6; i++) {
    double a = rot + i * (M_PI / 3.0);
    double px = x + cos(a) * r;
    double py = hy + sin(a) * r;
    lineRGBA(surface, (int)x, (int)hy, (int)px, (int)py, crys_c[0], crys_c[1], crys_c[2],
             weapon_alpha(120.0 * event->fade));
    weapon_glow(surface, px, py, 1.1, crys_c, 175.0 * event->fade);
  }
}


// Reed beacons: a cluster of slender marsh reeds along the ridge hollow
// tipped with gently swaying golden-green wisps.
static void hill_reed_beacons(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 37, 73));
  Uint8 hull_a = weapon_alpha(220.0 * event->fade);
  static const Uint8 wisp_c[3] = {195, 245, 135};
  for (int i = -3; i <= 3; i++) {
    double rx = cx + i * s * 0.014;
    double rg = hill_ground(event, rx);
    double rh = s * (0.036 + (i % 2 == 0 ? 0.012 : 0.0));
    double sway = sin(event->t * 1.9 + i * 0.7) * s * 0.006;
    lineRGBA(surface, (int)rx, (int)(rg + 2), (int)(rx + sway), (int)(rg - rh), 26, 36, 40, hull_a);
    weapon_glow(surface, rx + sway, rg - rh, 1.1, wisp_c, 175.0 * event->fade);
  }
}


// Star-gate pylon: a tall split-ring stone arch on the ridge with a faint
// shimmering violet-blue portal haze inside.
static void hill_stargate_pylon(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 39, 79));
  double ground = hill_ground(event, x);
  double r = s * 0.038;
  double cy = ground - r * 0.9;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  circleRGBA(surface, (int)x, (int)cy, (int)r, 26, 30, 44, hull_a);
  circleRGBA(surface, (int)x, (int)cy, (int)(r - 1), 36, 42, 60, hull_a);
  static const Uint8 portal_c[3] = {155, 165, 255};
  double pulse = 0.75 + 0.25 * sin(event->t * 2.4);
  weapon_glow(surface, x, cy, r * 0.65 * pulse, portal_c, 140.0 * event->fade);
}


// Tide chimes: a row of five slender bronze chimes hanging from a ridge beam
// that swing gently and glint with pale aqua highlights.
static void hill_tide_chimes(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 41, 83));
  double ground = hill_ground(event, x);
  double w = s * 0.048;
  double top = ground - s * 0.056;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  lineRGBA(surface, (int)(x - w), (int)(ground + 2), (int)(x - w), (int)top, 22, 26, 38, hull_a);
  lineRGBA(surface, (int)(x + w), (int)(ground + 2), (int)(x + w), (int)top, 22, 26, 38, hull_a);
  thickLineRGBA(surface, (int)(x - w), (int)top, (int)(x + w), (int)top, 2, 26, 30, 44, hull_a);
  static const Uint8 chime_c[3] = {145, 235, 225};
  for (int i = -2; i <= 2; i++) {
    double cx = x + i * w * 0.35;
    double len = s * (0.022 + (2 - abs(i)) * 0.005);
    double sway = sin(event->t * 2.0 + i * 0.7) * s * 0.004;
    lineRGBA(surface, (int)cx, (int)top, (int)(cx + sway), (int)(top + len), chime_c[0], chime_c[1],
             chime_c[2], weapon_alpha(185.0 * event->fade));
  }
}


// Ember cairn: a stacked stone cairn on the ridge with warm orange-gold
// ember light glowing between its upper stones.
static void hill_ember_cairn(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 43, 89));
  double ground = hill_ground(event, x);
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  static const Uint8 ember_c[3] = {255, 165, 80};
  double y = ground;
  for (int i = 0; i < 4; i++) {
    double rw = s * (0.024 - i * 0.004);
    double rh = s * 0.011;
    boxRGBA(surface, (int)(x - rw), (int)(y - rh), (int)(x + rw), (int)y, 22, 24, 34, hull_a);
    y -= rh * 1.1;
  }
  double pulse = 0.7 + 0.3 * sin(event->t * 3.1);
  weapon_glow(surface, x, y + s * 0.012, 1.8, ember_c, 195.0 * pulse * event->fade);
}


// Mist crane: a tall silhouette crane perched on the ridge crest, slowly
// dipping its beak toward a small pale-silver pool.
static void hill_mist_crane(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 45, 97));
  double ground = hill_ground(event, x);
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  double leg_h = s * 0.032;
  lineRGBA(surface, (int)x, (int)(ground + 2), (int)x, (int)(ground - leg_h), 20, 24, 36, hull_a);
  filledEllipseRGBA(surface, (int)(x - s * 0.008), (int)(ground - leg_h), (int)(s * 0.016),
                    (int)(s * 0.008), 24, 28, 42, hull_a);
  double dip = sin(event->t * 1.1) * s * 0.008;
  double hx = x + s * 0.018;
  double hy = ground - leg_h - s * 0.020 + dip;
  thickLineRGBA(surface, (int)x, (int)(ground - leg_h), (int)hx, (int)hy, 2, 26, 32, 46, hull_a);
  lineRGBA(surface, (int)hx, (int)hy, (int)(hx + s * 0.012), (int)(hy + s * 0.004), 165, 195, 225,
           weapon_alpha(185.0 * event->fade));
}


// Moonlit observatory dome: a rounded silhouette dome on the ridge with a
// slit opening and a faint cyan-white telescope beam pointing skyward.
static void hill_moon_dome(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 47, 101));
  double ground = hill_ground(event, x);
  double r = s * 0.036;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledCircleRGBA(surface, (int)x, (int)ground, (int)r, 22, 26, 38, hull_a);
  double ang = -M_PI * 0.38 + sin(event->t * 0.35) * 0.15;
  double tx = x + cos(ang) * r * 1.4;
  double ty = ground + sin(ang) * r * 1.4;
  thickLineRGBA(surface, (int)x, (int)(ground - r * 0.4), (int)tx, (int)ty, 3, 34, 42, 58, hull_a);
  lineRGBA(surface, (int)tx, (int)ty, (int)(tx + cos(ang) * s * 0.08),
           (int)(ty + sin(ang) * s * 0.08), 165, 225, 255, weapon_alpha(75.0 * event->fade));
  static const Uint8 slit_c[3] = {155, 220, 255};
  weapon_glow(surface, tx, ty, 1.2, slit_c, 185.0 * event->fade);
}


// Star-lantern archway: a tall timber torii-like gate on the ridge with a
// single bright crimson-gold paper lantern swinging from its crossbeam.
static void hill_lantern_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 49, 103));
  double ground = hill_ground(event, x);
  double w = s * 0.034;
  double h = s * 0.068;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)(x - w), (int)(ground + 2), (int)(x - w * 0.85), (int)(ground - h), 2,
                22, 24, 34, hull_a);
  thickLineRGBA(surface, (int)(x + w), (int)(ground + 2), (int)(x + w * 0.85), (int)(ground - h), 2,
                22, 24, 34, hull_a);
  thickLineRGBA(surface, (int)(x - w * 1.25), (int)(ground - h), (int)(x + w * 1.25),
                (int)(ground - h), 3, 24, 26, 38, hull_a);
  double sway = sin(event->t * 1.7) * s * 0.005;
  double lx = x + sway;
  double ly = ground - h + s * 0.018;
  lineRGBA(surface, (int)x, (int)(ground - h), (int)lx, (int)ly, 65, 75, 90, hull_a);
  static const Uint8 lan_c[3] = {255, 145, 95};
  weapon_glow(surface, lx, ly, 1.8, lan_c, 205.0 * event->fade);
}


// Bioluminescent grove: three slender dark willow silhouettes along the
// ridge with softly glowing aqua-green drooping leaf tips.
static void hill_bio_grove(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 51, 107));
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  static const Uint8 bio_c[3] = {115, 245, 195};
  for (int t = -1; t <= 1; t++) {
    double tx = cx + t * s * 0.042;
    double tg = hill_ground(event, tx);
    double th = s * (t == 0 ? 0.062 : 0.048);
    thickLineRGBA(surface, (int)tx, (int)(tg + 2), (int)tx, (int)(tg - th), 2, 18, 22, 32, hull_a);
    for (int b = -1; b <= 1; b++) {
      double bx = tx + b * s * 0.016;
      double by = tg - th + s * 0.018;
      lineRGBA(surface, (int)tx, (int)(tg - th), (int)bx, (int)by, 24, 32, 42, hull_a);
      double pulse = 0.55 + 0.45 * sin(event->t * 2.1 + t + b);
      weapon_glow(surface, bx, by, 1.1, bio_c, 165.0 * pulse * event->fade);
    }
  }
}


// Beacon buoy: a tall skeletal mast on the ridge crest with a rhythmic
// amber-white warning strobe flashing at its summit.
static void hill_beacon_mast(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 53, 109));
  double ground = hill_ground(event, x);
  double h = s * 0.088;
  double bw = s * 0.014;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  lineRGBA(surface, (int)(x - bw), (int)(ground + 2), (int)x, (int)(ground - h), 28, 34, 48,
           hull_a);
  lineRGBA(surface, (int)(x + bw), (int)(ground + 2), (int)x, (int)(ground - h), 28, 34, 48,
           hull_a);
  static const Uint8 strobe_c[3] = {255, 195, 110};
  double flash = pow(0.5 + 0.5 * sin(event->t * 3.8), 3.0);
  weapon_glow(surface, x, ground - h, 1.5 + flash * 1.5, strobe_c,
              (85.0 + 145.0 * flash) * event->fade);
}


// Tide clock: a slender stone gauge tower on the ridge with a glowing
// aqua level column rising and falling slowly inside its window.
static void hill_tide_clock(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 55, 113));
  double ground = hill_ground(event, x);
  double h = s * 0.072;
  double w = s * 0.012;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)(ground + 2), 20, 24, 34,
          hull_a);
  double level = 0.25 + 0.65 * (0.5 + 0.5 * sin(event->t * 0.8));
  static const Uint8 aqua_c[3] = {115, 235, 225};
  thickLineRGBA(surface, (int)x, (int)(ground - s * 0.008), (int)x, (int)(ground - h * level), 3,
                aqua_c[0], aqua_c[1], aqua_c[2], weapon_alpha(185.0 * event->fade));
  weapon_glow(surface, x, ground - h * level, 1.2, aqua_c, 185.0 * event->fade);
}


// Fire wheel: a festival Catherine wheel mounted on a ridge post, spinning
// slowly with warm amber sparks around its rim.
static void hill_fire_wheel(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 57, 127));
  double ground = hill_ground(event, x);
  double cy = ground - s * 0.052;
  double r = s * 0.028;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)x, (int)(ground + 2), (int)x, (int)cy, 2, 22, 26, 36, hull_a);
  circleRGBA(surface, (int)x, (int)cy, (int)r, 42, 46, 58, hull_a);
  static const Uint8 spark_c[3] = {255, 175, 85};
  double rot = event->t * 1.4;
  for (int i = 0; i < 6; i++) {
    double a = rot + i * (M_PI / 3.0);
    double sx = x + cos(a) * r;
    double sy = cy + sin(a) * r;
    weapon_glow(surface, sx, sy, 1.1, spark_c, 185.0 * event->fade);
  }
}


// Rune pillar: a weathered standing monolith on the ridge with four
// emerald runes pulsing sequentially up its face.
static void hill_rune_pillar(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 59, 131));
  double ground = hill_ground(event, x);
  double h = s * 0.076;
  double w = s * 0.011;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)(ground + 2), 18, 22, 30,
          hull_a);
  static const Uint8 rune_c[3] = {115, 245, 165};
  for (int i = 0; i < 4; i++) {
    double ry = ground - h * (0.22 + i * 0.20);
    double pulse = 0.35 + 0.65 * (0.5 + 0.5 * sin(event->t * 2.4 - i * 1.1));
    weapon_glow(surface, x, ry, 1.2, rune_c, 185.0 * pulse * event->fade);
  }
}


// Star antenna: a parabolic dish mounted on the ridge nodding slowly toward
// the zenith with a faint cyan receiver glint.
static void hill_star_antenna(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 61, 137));
  double ground = hill_ground(event, x);
  double hy = ground - s * 0.036;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)(x - s * 0.014), (int)(ground + 2), (int)x, (int)hy, 2, 22, 26, 38,
                hull_a);
  thickLineRGBA(surface, (int)(x + s * 0.014), (int)(ground + 2), (int)x, (int)hy, 2, 22, 26, 38,
                hull_a);
  double tilt = -0.35 + 0.25 * sin(event->t * 0.45);
  double span = s * 0.032;
  double dx = cos(tilt) * span;
  double dy = sin(tilt) * span;
  thickLineRGBA(surface, (int)(x - dx), (int)(hy - dy), (int)(x + dx), (int)(hy + dy), 3, 42, 52,
                72, hull_a);
  double fx = x + sin(tilt) * s * 0.020;
  double fy = hy - cos(tilt) * s * 0.020;
  lineRGBA(surface, (int)x, (int)hy, (int)fx, (int)fy, 65, 80, 105, hull_a);
  static const Uint8 glint_c[3] = {145, 225, 255};
  weapon_glow(surface, fx, fy, 1.3, glint_c, 185.0 * event->fade);
}


// Lantern pergola: a low dark-wood ridge pergola hung with three swaying
// warm coral-gold paper lanterns.
static void hill_lantern_pergola(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 63, 139));
  double ground = hill_ground(event, x);
  double w = s * 0.055;
  double h = s * 0.048;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)(x - w), (int)(ground + 2), (int)(x - w), (int)(ground - h), 2, 22,
                24, 34, hull_a);
  thickLineRGBA(surface, (int)(x + w), (int)(ground + 2), (int)(x + w), (int)(ground - h), 2, 22,
                24, 34, hull_a);
  thickLineRGBA(surface, (int)(x - w * 1.18), (int)(ground - h), (int)(x + w * 1.18),
                (int)(ground - h), 3, 26, 28, 40, hull_a);
  static const Uint8 lan_c[3] = {255, 165, 105};
  for (int i = -1; i <= 1; i++) {
    double lx = x + i * w * 0.62 + sin(event->t * 1.7 + i) * s * 0.003;
    double ly = ground - h + s * 0.014;
    lineRGBA(surface, (int)(x + i * w * 0.62), (int)(ground - h), (int)lx, (int)ly, 60, 70, 85,
             hull_a);
    weapon_glow(surface, lx, ly, 1.5, lan_c, 195.0 * event->fade);
  }
}


// Star-well: a circular stone well curb on the ridge with a faint rising
// column of pale silver-blue starlight motes.
static void hill_star_well(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 65, 149));
  double ground = hill_ground(event, x);
  double w = s * 0.028;
  double h = s * 0.018;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  boxRGBA(surface, (int)(x - w), (int)(ground - h), (int)(x + w), (int)(ground + 2), 22, 26, 36,
          hull_a);
  static const Uint8 mote_c[3] = {175, 225, 255};
  for (int i = 0; i < 7; i++) {
    double u = fmod(event->t * 0.45 + i / 7.0, 1.0);
    double mx = x + sin(event->t * 2.0 + i * 1.5) * w * 0.65;
    double my = ground - h - u * s * 0.068;
    weapon_glow(surface, mx, my, 1.1, mote_c, (1.0 - u) * 185.0 * event->fade);
  }
}


// Bell arch: a slender stone belfry arch on the ridge with a gently
// swinging bronze bell glinting in the moonlight.
static void hill_bell_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 67, 151));
  double ground = hill_ground(event, x);
  double w = s * 0.024;
  double h = s * 0.068;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)(x - w), (int)(ground + 2), (int)(x - w * 0.6), (int)(ground - h), 3,
                22, 26, 36, hull_a);
  thickLineRGBA(surface, (int)(x + w), (int)(ground + 2), (int)(x + w * 0.6), (int)(ground - h), 3,
                22, 26, 36, hull_a);
  thickLineRGBA(surface, (int)(x - w * 0.7), (int)(ground - h), (int)(x + w * 0.7),
                (int)(ground - h), 3, 26, 30, 42, hull_a);
  double swing = sin(event->t * 2.2) * s * 0.008;
  double bx = x + swing;
  double by = ground - h + s * 0.020;
  lineRGBA(surface, (int)x, (int)(ground - h), (int)bx, (int)by, 60, 70, 85, hull_a);
  static const Uint8 bell_c[3] = {245, 210, 140};
  filledCircleRGBA(surface, (int)bx, (int)by, (int)(s * 0.007) + 1, bell_c[0], bell_c[1], bell_c[2],
                   weapon_alpha(205.0 * event->fade));
}


// Aurora reeds: a row of crystalline spire-reeds along the ridge that
// shimmer in sequence with pale green-violet aurora light.
static void hill_aurora_reeds(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 69, 157));
  static const Uint8 c0[3] = {115, 245, 195};
  static const Uint8 c1[3] = {185, 145, 255};
  for (int i = -4; i <= 4; i++) {
    double rx = cx + i * s * 0.012;
    double rg = hill_ground(event, rx);
    double rh = s * (0.030 + (i % 2 == 0 ? 0.014 : 0.0));
    double wave = 0.4 + 0.6 * (0.5 + 0.5 * sin(event->t * 2.8 - i * 0.6));
    const Uint8 *c = (i % 2 == 0) ? c0 : c1;
    lineRGBA(surface, (int)rx, (int)(rg + 2), (int)rx, (int)(rg - rh), c[0], c[1], c[2],
             weapon_alpha(165.0 * wave * event->fade));
    weapon_glow(surface, rx, rg - rh, 1.0, c, 170.0 * wave * event->fade);
  }
}


// Star-gate obelisk: twin angled stone monoliths on the ridge with a
// shimmering pale-gold energy arc sparking between their tips.
static void hill_twin_monoliths(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 71, 163));
  double ground = hill_ground(event, x);
  double sep = s * 0.036;
  double h = s * 0.072;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  filledTrigonRGBA(surface, (int)(x - sep - s * 0.014), (int)(ground + 2),
                   (int)(x - sep + s * 0.010), (int)(ground + 2), (int)(x - sep * 0.65),
                   (int)(ground - h), 20, 24, 34, hull_a);
  filledTrigonRGBA(surface, (int)(x + sep + s * 0.014), (int)(ground + 2),
                   (int)(x + sep - s * 0.010), (int)(ground + 2), (int)(x + sep * 0.65),
                   (int)(ground - h), 20, 24, 34, hull_a);
  static const Uint8 arc_c[3] = {255, 215, 135};
  double mid_y = ground - h - sin(event->t * 3.5) * s * 0.010;
  lineRGBA(surface, (int)(x - sep * 0.65), (int)(ground - h), (int)x, (int)mid_y, arc_c[0],
           arc_c[1], arc_c[2], weapon_alpha(175.0 * event->fade));
  lineRGBA(surface, (int)x, (int)mid_y, (int)(x + sep * 0.65), (int)(ground - h), arc_c[0],
           arc_c[1], arc_c[2], weapon_alpha(175.0 * event->fade));
  weapon_glow(surface, x, mid_y, 1.3, arc_c, 195.0 * event->fade);
}


// Paper boat stream: three tiny lit paper boats gliding slowly along the
// ridge hollow with warm candle-amber reflections.
static void hill_paper_boats(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 73, 167));
  static const Uint8 candle_c[3] = {255, 195, 110};
  for (int i = -1; i <= 1; i++) {
    double bx = cx + i * s * 0.045 + fmod(event->t * s * 0.008, s * 0.04);
    double by = hill_ground(event, bx) - s * 0.006;
    filledTrigonRGBA(surface, (int)(bx - s * 0.012), (int)by, (int)(bx + s * 0.012), (int)by,
                     (int)bx, (int)(by - s * 0.014), 225, 225, 235,
                     weapon_alpha(185.0 * event->fade));
    weapon_glow(surface, bx, by - s * 0.006, 1.1, candle_c, 185.0 * event->fade);
  }
}


// Glow-worm cave: a dark stone archway tucked into the ridge with clusters
// of pale-blue bioluminescent dots twinkling inside.
static void hill_glow_cave(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 75, 173));
  double ground = hill_ground(event, x);
  double r = s * 0.036;
  filledCircleRGBA(surface, (int)x, (int)ground, (int)r, 12, 16, 24,
                   weapon_alpha(235.0 * event->fade));
  static const Uint8 worm_c[3] = {125, 225, 255};
  for (int i = 0; i < 8; i++) {
    double wx = x + (-0.65 + 0.18 * i) * r;
    double wy = ground - (0.25 + 0.45 * ((i * 3) % 4) / 3.0) * r;
    double tw = 0.45 + 0.55 * sin(event->t * 2.4 + i * 1.3);
    weapon_glow(surface, wx, wy, 0.9, worm_c, 185.0 * tw * event->fade);
  }
}


// Starlight pendulum: a tall gallows-frame on the ridge with a glowing
// silver-blue bob swinging in a slow hypnotic arc.
static void hill_pendulum(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.22 + 0.56 * hill_random(event, 77, 179));
  double ground = hill_ground(event, x);
  double h = s * 0.075;
  double w = s * 0.028;
  Uint8 hull_a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)(x - w), (int)(ground + 2), (int)(x - w * 0.5), (int)(ground - h), 2,
                22, 26, 36, hull_a);
  thickLineRGBA(surface, (int)(x + w), (int)(ground + 2), (int)(x + w * 0.5), (int)(ground - h), 2,
                22, 26, 36, hull_a);
  thickLineRGBA(surface, (int)(x - w * 0.6), (int)(ground - h), (int)(x + w * 0.6),
                (int)(ground - h), 2, 26, 30, 42, hull_a);
  double ang = sin(event->t * 1.4) * 0.52;
  double len = h * 0.72;
  double bx = x + sin(ang) * len;
  double by = ground - h + cos(ang) * len;
  lineRGBA(surface, (int)x, (int)(ground - h), (int)bx, (int)by, 95, 115, 145, hull_a);
  static const Uint8 bob_c[3] = {165, 225, 255};
  weapon_glow(surface, bx, by, 1.5, bob_c, 205.0 * event->fade);
}


// Peregrine falcon stoop: dives steeply from high sky onto the ridge and
// pulls up sharply with faint wind-shear streaks.
static void hill_falcon_stoop(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double p = event->progress;
  double tx = surface->w * (0.28 + 0.44 * hill_random(event, 79, 181));
  double ground = hill_ground(event, tx);
  double start_x = tx - dir * s * 0.42;
  double end_x = tx + dir * s * 0.45;
  double bx = start_x + (end_x - start_x) * p;
  double dive = sin(p * M_PI);
  double by = ground - s * 0.04 - (1.0 - dive * dive) * s * 0.32;
  double pitch = dir * ((p < 0.5) ? 0.65 * (1.0 - p * 2.0) : -0.45 * (p - 0.5) * 2.0);
  double flap = (p > 0.35 && p < 0.65) ? -0.45 : sin(event->t * 14.0) * 0.65;
  Uint8 alpha = weapon_alpha(230.0 * event->fade);
  draw_sleek_bird(surface, bx, by, s * 0.030, dir, flap, pitch, 24, 30, 44, alpha, true);
  if (p > 0.25 && p < 0.65) {
    lineRGBA(surface, (int)(bx - dir * s * 0.02), (int)(by - s * 0.02), (int)(bx - dir * s * 0.06),
             (int)(by - s * 0.06), 160, 195, 235, weapon_alpha(70.0 * event->fade));
  }
}


// Silent barn owl gliding low over the ridge contour, dipping into hollows.
static void hill_owl_glide(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double p = event->progress;
  double bx = (dir > 0) ? surface->w * (0.08 + 0.84 * p) : surface->w * (0.92 - 0.84 * p);
  double by = hill_ground(event, bx) - s * 0.055 - sin(p * M_PI * 3.0) * s * 0.018;
  double flap = sin(event->t * 3.8) * 0.72;
  draw_sleek_bird(surface, bx, by, s * 0.036, dir, flap, cos(p * M_PI * 3.0) * -0.14, 185, 195, 215,
                  weapon_alpha(205.0 * event->fade), true);
}


// Roost flight: a flock of ravens lifts off from a ridge sentinel tree and
// arcs away into the night sky.
static void hill_roost_flight(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double tx = surface->w * (0.25 + 0.50 * hill_random(event, 81, 191));
  double ground = hill_ground(event, tx);
  Uint8 hull_a = weapon_alpha(225.0 * event->fade);
  thickLineRGBA(surface, (int)tx, (int)(ground + 2), (int)tx, (int)(ground - s * 0.055), 3, 16, 20,
                28, hull_a);
  lineRGBA(surface, (int)tx, (int)(ground - s * 0.035), (int)(tx - s * 0.018),
           (int)(ground - s * 0.052), 16, 20, 28, hull_a);
  lineRGBA(surface, (int)tx, (int)(ground - s * 0.040), (int)(tx + s * 0.020),
           (int)(ground - s * 0.058), 16, 20, 28, hull_a);
  for (int i = 0; i < 7; i++) {
    double delay = i * 0.07;
    double bp = (event->progress - delay) / (1.0 - delay);
    if (bp <= 0.0) continue;
    int dir = (i % 2 == 0) ? event->direction : -event->direction;
    double bx = tx + dir * bp * s * (0.22 + 0.04 * i);
    double by = ground - s * 0.052 - pow(bp, 1.15) * s * (0.14 + 0.02 * (i % 3));
    double flap = sin(event->t * 9.5 + i * 1.4) * 0.85;
    draw_sleek_bird(surface, bx, by, s * 0.018, dir, flap, -0.25, 22, 26, 36,
                    weapon_alpha(210.0 * event->fade * (1.0 - bp * 0.4)), false);
  }
}


// Night heron: tall slender silhouette on the ridge that launches into slow,
// majestic wingbeats.
static void hill_heron_stalk(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double tx = surface->w * (0.25 + 0.50 * hill_random(event, 83, 193));
  double ground = hill_ground(event, tx);
  double p = event->progress;
  Uint8 alpha = weapon_alpha(220.0 * event->fade);
  if (p < 0.30) {
    double h = s * 0.032;
    lineRGBA(surface, (int)tx, (int)ground, (int)tx, (int)(ground - h * 0.55), 24, 30, 42, alpha);
    thickLineRGBA(surface, (int)(tx - dir * s * 0.008), (int)(ground - h * 0.55),
                  (int)(tx + dir * s * 0.008), (int)(ground - h * 0.65), 2, 24, 30, 42, alpha);
    lineRGBA(surface, (int)(tx + dir * s * 0.008), (int)(ground - h * 0.65),
             (int)(tx + dir * s * 0.010), (int)(ground - h * 0.95), 24, 30, 42, alpha);
    lineRGBA(surface, (int)(tx + dir * s * 0.010), (int)(ground - h * 0.95),
             (int)(tx + dir * s * 0.022), (int)(ground - h * 0.92), 24, 30, 42, alpha);
  } else {
    double fp = (p - 0.30) / 0.70;
    double bx = tx + dir * fp * s * 0.36;
    double by = ground - s * 0.03 - sin(fp * M_PI * 0.5) * s * 0.14;
    double flap = sin(event->t * 4.2) * 0.9;
    draw_sleek_bird(surface, bx, by, s * 0.038, dir, flap, -0.20, 26, 34, 48, alpha, true);
  }
}


// Swallow skim: agile swallows darting in sweeping arcs just above the ridge.
static void hill_swallow_skim(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  for (int i = 0; i < 4; i++) {
    int dir = (i % 2 == 0) ? 1 : -1;
    double phase = fmod(event->progress * 1.4 + i * 0.23, 1.0);
    double bx = (dir > 0) ? surface->w * (0.12 + 0.76 * phase) : surface->w * (0.88 - 0.76 * phase);
    double by = hill_ground(event, bx) - s * 0.022 - fabs(sin(phase * M_PI * 4.0 + i)) * s * 0.028;
    double pitch = cos(phase * M_PI * 4.0 + i) * -0.35;
    double flap = sin(event->t * 14.0 + i * 2.1) * 0.9;
    draw_sleek_bird(surface, bx, by, s * 0.017, dir, flap, pitch, 28, 36, 52,
                    weapon_alpha(205.0 * event->fade), true);
  }
}


// Eagle soar: broad-winged eagle circling lazily on thermal updrafts above
// the hill crest.
static void hill_eagle_soar(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 85, 197));
  double cy = hill_ground(event, cx) - s * 0.16;
  double a = event->t * 0.85;
  double bx = cx + cos(a) * s * 0.11;
  double by = cy + sin(a) * s * 0.032;
  int dir = (-sin(a) >= 0.0) ? 1 : -1;
  double flap = 0.18 + sin(event->t * 2.1) * 0.14;
  draw_sleek_bird(surface, bx, by, s * 0.044, dir, flap, cos(a) * 0.12, 26, 32, 46,
                  weapon_alpha(220.0 * event->fade), true);
}


// Crane dance: two tall cranes silhouetted on the ridge crest, bowing and
// leaping with moonlit wings.
static void hill_crane_dance(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 87, 199));
  Uint8 alpha = weapon_alpha(225.0 * event->fade);
  for (int i = 0; i < 2; i++) {
    int dir = (i == 0) ? 1 : -1;
    double x = cx + (i == 0 ? -s * 0.024 : s * 0.024);
    double ground = hill_ground(event, x);
    double leap = fabs(sin(event->t * 2.6 + i * 1.5)) * s * 0.026;
    double flap = sin(event->t * 5.2 + i * 1.5) * 0.85;
    draw_sleek_bird(surface, x, ground - s * 0.028 - leap, s * 0.030, dir, flap, -0.18, 200, 212,
                    232, alpha, true);
    lineRGBA(surface, (int)x, (int)(ground - leap), (int)x, (int)(ground - s * 0.026 - leap), 185,
             198, 220, alpha);
  }
}


// Nightjar hawk: buoyant erratic banked turns over the ridge dusk.
static void hill_nightjar_hawk(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double p = event->progress;
  double bx = (dir > 0) ? surface->w * (0.14 + 0.72 * p) : surface->w * (0.86 - 0.72 * p);
  double by = hill_ground(event, bx) - s * 0.075 + sin(event->t * 3.1) * s * 0.025;
  double flap = sin(event->t * 8.5) * 0.88;
  draw_sleek_bird(surface, bx, by, s * 0.024, dir, flap, cos(event->t * 3.1) * 0.25, 36, 44, 62,
                  weapon_alpha(215.0 * event->fade), true);
}


// Raven spire: three ravens wheeling around a craggy stone pinnacle.
static void hill_raven_spire(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double x = surface->w * (0.24 + 0.52 * hill_random(event, 89, 211));
  double ground = hill_ground(event, x);
  double h = s * 0.072;
  filledTrigonRGBA(surface, (int)(x - s * 0.015), (int)(ground + 2), (int)(x + s * 0.015),
                   (int)(ground + 2), (int)x, (int)(ground - h), 18, 22, 32,
                   weapon_alpha(235.0 * event->fade));
  for (int i = 0; i < 3; i++) {
    double a = event->t * 1.5 + i * (2.0 * M_PI / 3.0);
    double bx = x + cos(a) * s * 0.055;
    double by = ground - h * 0.88 + sin(a) * s * 0.016;
    int dir = (-sin(a) >= 0.0) ? 1 : -1;
    double flap = sin(event->t * 7.2 + i * 1.9) * 0.75;
    draw_sleek_bird(surface, bx, by, s * 0.020, dir, flap, 0.08, 22, 26, 36,
                    weapon_alpha(215.0 * event->fade), false);
  }
}


// Kingfisher dart: sleek bird with faint cyan-blue sheen hovering briefly
// above a hollow before darting across.
static void hill_kingfisher_dive(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double p = event->progress;
  double cx = surface->w * (0.28 + 0.44 * hill_random(event, 91, 223));
  double ground = hill_ground(event, cx);
  double bx = (p < 0.45) ? cx : cx + dir * ((p - 0.45) / 0.55) * s * 0.42;
  double by = ground - s * 0.065 +
              ((p < 0.45) ? sin(event->t * 6.0) * s * 0.004 : sin((p - 0.45) * M_PI) * s * 0.032);
  double flap = (p < 0.45) ? sin(event->t * 22.0) * 0.95 : -0.25;
  draw_sleek_bird(surface, bx, by, s * 0.019, dir, flap, (p < 0.45) ? -0.15 : 0.25, 45, 130, 175,
                  weapon_alpha(225.0 * event->fade), true);
}


// Vulture kettle: five broad-winged raptors spiraling steadily upward from
// the far ridge.
static void hill_vulture_kettle(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 93, 227));
  double base_y = hill_ground(event, cx) - s * 0.05;
  for (int i = 0; i < 5; i++) {
    double a = event->t * 0.95 + i * 1.25;
    double r = s * (0.045 + 0.01 * (i % 3));
    double bx = cx + cos(a) * r;
    double by = base_y - event->progress * s * 0.16 - i * s * 0.018 + sin(a) * s * 0.014;
    int dir = (-sin(a) >= 0.0) ? 1 : -1;
    draw_sleek_bird(surface, bx, by, s * 0.028, dir, 0.22, 0.05, 24, 28, 38,
                    weapon_alpha(200.0 * event->fade), true);
  }
}


// Loon skitter: a pair of night birds skimming fast and low across the ridge
// saddle, leaving subtle pale wake ripples behind.
static void hill_loon_skitter(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  for (int i = 0; i < 2; i++) {
    double p = event->progress;
    double bx = (dir > 0) ? surface->w * (0.10 + 0.80 * p) - i * s * 0.04
                          : surface->w * (0.90 - 0.80 * p) + i * s * 0.04;
    double by = hill_ground(event, bx) - s * 0.014;
    double flap = sin(event->t * 11.0 + i * 1.1) * 0.85;
    draw_sleek_bird(surface, bx, by, s * 0.022, dir, flap, 0.04, 30, 38, 54,
                    weapon_alpha(215.0 * event->fade), true);
    lineRGBA(surface, (int)(bx - dir * s * 0.015), (int)(by + s * 0.008),
             (int)(bx - dir * s * 0.045), (int)(by + s * 0.010), 140, 175, 215,
             weapon_alpha(95.0 * event->fade));
  }
}


// Starling wave: an undulating murmuration ribbon of 24 small birds sweeping
// over the hill line like living smoke.
static void hill_starling_wave(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double head_x = (dir > 0) ? surface->w * (0.05 + 0.90 * event->progress)
                            : surface->w * (0.95 - 0.90 * event->progress);
  for (int i = 0; i < 24; i++) {
    double u = i / 24.0;
    double bx = head_x - dir * u * s * 0.24 + sin(event->t * 2.5 + i) * s * 0.01;
    double by = hill_ground(event, bx) - s * 0.065 -
                sin(u * M_PI * 2.0 + event->t * 2.4) * s * 0.026 + ((i % 5) - 2) * s * 0.007;
    double flap = sin(event->t * 15.0 + i * 0.9) * 0.9;
    draw_sleek_bird(surface, bx, by, s * 0.010, dir, flap, 0.0, 22, 28, 40,
                    weapon_alpha(190.0 * event->fade), false);
  }
}


// Phoenix crest: an ember-tipped raptor sweeping low over the ridge, shedding
// a brief trail of cooling golden sparks.
static void hill_phoenix_crest(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double p = event->progress;
  double bx = (dir > 0) ? surface->w * (0.10 + 0.80 * p) : surface->w * (0.90 - 0.80 * p);
  double by = hill_ground(event, bx) - s * 0.065 - sin(p * M_PI) * s * 0.045;
  double flap = sin(event->t * 6.5) * 0.82;
  static const Uint8 ember_c[3] = {255, 175, 85};
  for (int i = 1; i <= 6; i++) {
    double sx = bx - dir * i * s * 0.016;
    double sy = by + i * s * 0.003 + sin(event->t * 4.0 + i) * s * 0.004;
    weapon_glow(surface, sx, sy, 1.1 - i * 0.12, ember_c, (170.0 - i * 22.0) * event->fade);
  }
  draw_sleek_bird(surface, bx, by, s * 0.034, dir, flap, -0.14, 245, 155, 75,
                  weapon_alpha(230.0 * event->fade), true);
}


// Gull ridge: coastal gulls riding the ridge updraft in a steady hover,
// banking slightly against the wind.
static void hill_gull_ridge(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 95, 229));
  for (int i = 0; i < 4; i++) {
    double bx = cx + (i - 1.5) * s * 0.042 + sin(event->t * 1.1 + i * 1.4) * s * 0.012;
    double by = hill_ground(event, bx) - s * (0.065 + 0.014 * (i % 2)) +
                cos(event->t * 1.4 + i) * s * 0.008;
    double flap = 0.25 + sin(event->t * 3.2 + i * 1.7) * 0.35;
    draw_sleek_bird(surface, bx, by, s * 0.023, dir, flap, sin(event->t * 1.5 + i) * 0.12, 195, 208,
                    225, weapon_alpha(205.0 * event->fade), true);
  }
}


// NEW_HILL_EVENTS_ABOVE (new hill_xxx() functions go above this line)

static void (*const hill_event_kinds[])(const HillEvent *) = {
    hill_village,
    hill_caravan,
    hill_turbines,
    hill_lanterns,
    hill_campfire,
    hill_bloom,
    hill_herd,
    hill_volcano,
    hill_beacons,
    hill_fireflies,
    hill_kites,
    hill_fireworks,
    hill_train,
    hill_rocket,
    hill_abduction,
    hill_geysers,
    hill_stones,
    hill_ferris,
    hill_observatory,
    hill_windmills,
    hill_lighttree,
    hill_meteorite,
    hill_dustdevils,
    hill_lasers,
    hill_tesla,
    hill_balloons,
    hill_giant,
    hill_clocktower,
    hill_drones,
    hill_bats,
    hill_gate,
    hill_lighthouse,
    hill_cherry,
    hill_wolf,
    hill_pumpkins,
    hill_castle,
    hill_spaceelevator,
    hill_airstrip,
    hill_hotsprings,
    hill_market,
    hill_lotus,
    hill_bonfire,
    hill_waterfall,
    hill_scarecrows,
    hill_pagoda,
    hill_skaters,
    hill_shepherd,
    hill_bridge,
    hill_totems,
    hill_snowmen,
    hill_sunflowers,
    hill_crystals,
    hill_mushrooms,
    hill_tents,
    hill_fountain,
    hill_harvester,
    hill_cairns,
    hill_watermill,
    hill_marsh,
    hill_stargazers,
    hill_carousel,
    hill_trebuchet,
    hill_orchard,
    hill_radiomast,
    hill_greenhouse,
    hill_swings,
    hill_beehives,
    hill_cableway,
    hill_ruins,
    hill_geodome,
    hill_cabin,
    hill_windchimes,
    hill_picnic,
    hill_wishtree,
    hill_mine,
    hill_eggs,
    hill_hammock,
    hill_telegraph,
    hill_hamlet,
    hill_stepstones,
    hill_seesaw,
    hill_laundry,
    hill_well,
    hill_kiln,
    hill_ropebridge,
    hill_belltower,
    hill_maze,
    hill_fisher,
    hill_lanterntree,
    hill_forge,
    hill_seedtree,
    hill_stonecircle,
    hill_sandcastle,
    hill_musicbox,
    hill_lightstairs,
    hill_armillary,
    hill_monolith,
    hill_abyssal_reach,
    hill_shrikes_tree,
    hill_pale_lighthouse,
    hill_derelict_galleon,
    hill_threshold_arch,
    hill_glass_forest,
    hill_tidal_bell,
    hill_canal_skiff,
    hill_prism_observatory,
    hill_aurora_turbine,
    hill_solar_mirror,
    hill_ether_fountain,
    hill_signal_pyre,
    hill_tide_lantern,
    hill_frost_obelisk,
    hill_wind_harp,
    hill_lantern_bridge,
    hill_sun_dial,
    hill_glacier_arch,
    hill_beacon_windmill,
    hill_star_basin,
    hill_signal_semaphores,
    hill_crystal_geode,
    hill_hanging_gardens,
    hill_echo_spire,
    hill_amber_kiln,
    hill_starlight_mill,
    hill_reed_beacons,
    hill_stargate_pylon,
    hill_tide_chimes,
    hill_ember_cairn,
    hill_mist_crane,
    hill_moon_dome,
    hill_lantern_arch,
    hill_bio_grove,
    hill_beacon_mast,
    hill_tide_clock,
    hill_fire_wheel,
    hill_rune_pillar,
    hill_star_antenna,
    hill_lantern_pergola,
    hill_star_well,
    hill_bell_arch,
    hill_aurora_reeds,
    hill_twin_monoliths,
    hill_paper_boats,
    hill_glow_cave,
    hill_pendulum,
    hill_falcon_stoop,
    hill_owl_glide,
    hill_roost_flight,
    hill_heron_stalk,
    hill_swallow_skim,
    hill_eagle_soar,
    hill_crane_dance,
    hill_nightjar_hawk,
    hill_raven_spire,
    hill_kingfisher_dive,
    hill_vulture_kettle,
    hill_loon_skitter,
    hill_starling_wave,
    hill_phoenix_crest,
    hill_gull_ridge,
    // NEW_HILL_KINDS_ABOVE
};
static_assert(sizeof(hill_event_kinds) / sizeof(hill_event_kinds[0]) == HILL_EVENT_KINDS,
              "HILL_EVENT_KINDS does not match hill_event_kinds[]");

static void render_hill_event(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_HILL_EVENT, ticks, &window)) return;
  HillEvent event;
  event.surface = surface;
  event.ticks = ticks;
  event.scale = scale;
  event.seed = window.seed;
  event.t = window.phase / 1000.0;
  event.progress = window.progress;
  event.fade = window.fade;
  event.direction = (window.seed >> 6) & 1 ? 1 : -1;
  int kind = effect_kind(&EFFECT_HILL_EVENT, (window.seed >> 10) % HILL_EVENT_KINDS);
  hill_event_kinds[kind](&event);
}
