/*
 * Ship drawing
 *
 * Ships are seen from the side, nose towards +x, +y down. A design is the upper
 * half of a hull outline in tenths of a unit, mirrored to make the whole hull.
 * The hull is painted in four bands of light from above with a dark outline and
 * glowing engines. Each ship is drawn fully opaque and then blended onto the
 * scene at the requested opacity, so its overlapping parts do not stack up.
 *
 * Ships come in three roles. Small ones are the fighters used everywhere,
 * mid-size and big ones only appear in the fleet battle.
 */
typedef signed char ShipPoint[2];

typedef struct {
  const ShipPoint *points;
  int count;
} ShipOutline;

#define SHIP_OUTLINE(points) \
  { points, (int)(sizeof(points) / sizeof(points[0])) }
#define SHIP_NO_OUTLINE \
  { NULL, 0 }
#define SHIP_ENGINE_SIZE 2  // engine glow radius, tenths of a unit
#define SHIP_OPACITY 235    // opacity of a fully faded in ship

typedef struct {
  ShipOutline hull;         // upper half, nose first, tail last (both at y = 0)
  ShipOutline plates[2];    // lighter plates drawn over the hull, if any
  const ShipPoint *spikes;  // (x, height) of each spike on the back, if any
  int spike_count;
  ShipPoint engines[3];
  int engine_count;
  ShipPoint accent[2];  // glowing slit between these two points
  bool accent_on;
  bool eye;              // a glowing eye in the middle instead of a slit
  int nose, tail;        // extent of the hull along x
  int weapon;            // main weapon, fired from the nose (WEAPON_*)
  ShipPoint turrets[4];  // gun turret mounts on the hull, if any
  int turret_count;
  int turret_weapon;  // what the turrets fire
} ShipDesign;

typedef struct {
  Uint8 tone[4][3];  // shadow, mid, light, highlight
  Uint8 glow[3];     // engine glow and bounced light
  Uint8 accent[3];   // glowing slits and eyes
} ShipTeam;

static const ShipTeam ship_teams[2] = {
    {{{8, 12, 19}, {16, 25, 38}, {29, 43, 60}, {55, 74, 95}}, {90, 190, 255}, {150, 235, 255}},
    {{{19, 11, 8}, {36, 21, 15}, {55, 34, 24}, {88, 64, 48}}, {255, 150, 70}, {255, 208, 120}}};

/*
 * Weapons
 *
 * The weapon families are modelled on X4: Foundations. Each one has its own
 * look and its own timing: pulse lasers and mass drivers throw many fast bolts,
 * beam emitters hold a continuous beam, plasma is a slow heavy orb, flak bursts
 * into shrapnel short of the target, missiles and swarms curve in leaving
 * smoke, torpedoes are huge and slow, and the railgun charges before it fires.
 * Small ships carry one gun; mid-size and big ships also have turrets.
 */
enum {
  WEAPON_PULSE,
  WEAPON_BEAM,
  WEAPON_PLASMA,
  WEAPON_MASS_DRIVER,
  WEAPON_FLAK,
  WEAPON_MISSILE,
  WEAPON_SWARM,
  WEAPON_TORPEDO,
  WEAPON_RAIL,
  WEAPON_COUNT
};

typedef struct {
  double impact_ms;  // time from firing until a shot reaches the target
  double life_ms;    // time until nothing of a shot is left on screen
  int shots;         // shots in one volley
  double gap_ms;     // time between the shots of a volley
  double blast;      // size of the impact, in shot-size units
} WeaponSpec;

static const WeaponSpec weapon_specs[WEAPON_COUNT] = {
    {260.0, 820.0, 3, 90.0, 0.16},   // pulse laser
    {420.0, 1100.0, 1, 0.0, 0.20},   // beam emitter (the beam itself lasts 900 ms)
    {720.0, 1320.0, 1, 0.0, 0.50},   // plasma cannon
    {200.0, 650.0, 6, 42.0, 0.10},   // mass driver
    {480.0, 1030.0, 1, 0.0, 0.40},   // flak
    {950.0, 1500.0, 1, 0.0, 0.45},   // missile
    {880.0, 1430.0, 5, 75.0, 0.25},  // swarm missiles
    {1500.0, 2100.0, 1, 0.0, 0.90},  // torpedo
    {300.0, 850.0, 1, 0.0, 0.30}     // railgun (charges for 300 ms, then fires)
};

static const ShipPoint hull_dagger[] = {{45, 0},    {30, -3},  {12, -5},  {-4, -14},
                                        {-20, -22}, {-14, -8}, {-26, -5}, {-28, 0}};
static const ShipPoint spine_dagger[] = {{40, 0}, {18, -2}, {-20, -2}, {-28, 0}};
static const ShipPoint hull_manta[] = {{32, 0},    {20, -6},  {4, -16},  {-12, -30}, {-20, -27},
                                       {-14, -15}, {-26, -8}, {-32, -4}, {-32, 0}};
static const ShipPoint spine_manta[] = {{34, 0}, {10, -4}, {-30, -3}, {-32, 0}};
static const ShipPoint hull_hammerhead[] = {{38, 0},  {37, -18},  {29, -19}, {26, -6},
                                            {-8, -7}, {-22, -12}, {-30, -9}, {-30, 0}};
static const ShipPoint spine_hammerhead[] = {{26, 0}, {24, -3}, {-8, -4}, {-30, -3}, {-30, 0}};
static const ShipPoint hull_mandible[] = {{8, 0},     {22, -4},  {38, -12}, {22, -18}, {2, -15},
                                          {-14, -12}, {-26, -8}, {-32, -3}, {-32, 0}};
static const ShipPoint spine_mandible[] = {{2, 0}, {0, -5}, {-30, -4}, {-32, 0}};
static const ShipPoint hull_trident[] = {{46, 0},  {24, -3},   {16, -5},  {30, -16}, {12, -12},
                                         {-4, -9}, {-24, -12}, {-30, -6}, {-30, 0}};
static const ShipPoint spine_trident[] = {{44, 0}, {10, -2}, {-28, -2}, {-30, 0}};
static const ShipPoint hull_scythe[] = {{30, 0},    {22, -6},   {8, -16},  {-8, -26}, {-28, -36},
                                        {-20, -20}, {-14, -10}, {-28, -6}, {-30, 0}};
static const ShipPoint spine_scythe[] = {{32, 0}, {6, -4}, {-28, -3}, {-30, 0}};
static const ShipPoint hull_bat[] = {{26, 0},    {14, -8},   {-4, -20},  {-10, -31}, {-16, -22},
                                     {-18, -31}, {-24, -19}, {-28, -25}, {-30, -8},  {-32, 0}};
static const ShipPoint spine_bat[] = {{28, 0}, {8, -4}, {-30, -3}, {-32, 0}};
static const ShipPoint hull_leviathan[] = {{52, 0},   {38, -4},  {12, -7},
                                           {-30, -8}, {-46, -5}, {-50, 0}};
static const ShipPoint spine_leviathan[] = {{48, 0}, {12, -3}, {-46, -2}, {-50, 0}};
static const ShipPoint spikes_leviathan[] = {{20, 14}, {4, 18}, {-12, 18}, {-28, 14}};
static const ShipPoint hull_wedge[] = {{42, 0}, {-4, -16}, {-30, -22}, {-36, -14}, {-36, 0}};
static const ShipPoint plate_wedge[] = {{30, 0}, {-6, -10}, {-30, -14}, {-34, -8}, {-34, 0}};
static const ShipPoint top_wedge[] = {{16, 0}, {-8, -5}, {-30, -7}, {-34, -4}, {-34, 0}};
static const ShipPoint hull_swept[] = {{36, 0},  {12, -4},   {28, -22}, {6, -20},
                                       {-6, -8}, {-20, -10}, {-26, -4}, {-26, 0}};
static const ShipPoint spine_swept[] = {{34, 0}, {10, -2}, {-24, -2}, {-26, 0}};

// Indexes into ship_designs[], grouped by role.
enum { SHIP_SMALL, SHIP_MID, SHIP_BIG };
static const int ship_small_designs[] = {0, 1, 5, 6, 9};  // dagger, manta, scythe, bat, swept
static const int ship_mid_designs[] = {2, 3, 4};          // hammerhead, mandible, trident
static const int ship_big_designs[] = {7, 8};             // leviathan, wedge

static const ShipDesign ship_designs[] = {
    // 0 dagger
    {SHIP_OUTLINE(hull_dagger),
     {SHIP_OUTLINE(spine_dagger), SHIP_NO_OUTLINE},
     NULL,
     0,
     {{-28, -3}, {-28, 3}, {0, 0}},
     2,
     {{34, 0}, {22, 0}},
     true,
     false,
     45,
     -28,
     WEAPON_RAIL,
     {{0, 0}},
     0,
     WEAPON_PULSE},
    // 1 manta
    {SHIP_OUTLINE(hull_manta),
     {SHIP_OUTLINE(spine_manta), SHIP_NO_OUTLINE},
     NULL,
     0,
     {{-32, -3}, {-32, 3}, {0, 0}},
     2,
     {{26, 0}, {12, 0}},
     true,
     false,
     32,
     -32,
     WEAPON_SWARM,
     {{0, 0}},
     0,
     WEAPON_PULSE},
    // 2 hammerhead
    {SHIP_OUTLINE(hull_hammerhead),
     {SHIP_OUTLINE(spine_hammerhead), SHIP_NO_OUTLINE},
     NULL,
     0,
     {{-30, -4}, {-30, 4}, {0, 0}},
     2,
     {{36, -15}, {36, 15}},
     true,
     false,
     38,
     -30,
     WEAPON_FLAK,
     {{30, -12}, {30, 12}, {-12, 0}},
     3,
     WEAPON_MASS_DRIVER},
    // 3 mandible
    {SHIP_OUTLINE(hull_mandible),
     {SHIP_OUTLINE(spine_mandible), SHIP_NO_OUTLINE},
     NULL,
     0,
     {{-32, -3}, {-32, 3}, {0, 0}},
     2,
     {{0, 0}, {0, 0}},
     false,
     true,
     38,
     -32,
     WEAPON_PLASMA,
     {{-6, -8}, {-6, 8}, {-22, 0}},
     3,
     WEAPON_PULSE},
    // 4 trident
    {SHIP_OUTLINE(hull_trident),
     {SHIP_OUTLINE(spine_trident), SHIP_NO_OUTLINE},
     NULL,
     0,
     {{-30, -3}, {-30, 3}, {0, 0}},
     2,
     {{38, 0}, {26, 0}},
     true,
     false,
     46,
     -30,
     WEAPON_MISSILE,
     {{16, -9}, {16, 9}, {-12, 0}},
     3,
     WEAPON_FLAK},
    // 5 scythe
    {SHIP_OUTLINE(hull_scythe),
     {SHIP_OUTLINE(spine_scythe), SHIP_NO_OUTLINE},
     NULL,
     0,
     {{-30, -3}, {-30, 3}, {0, 0}},
     2,
     {{24, 0}, {10, 0}},
     true,
     false,
     30,
     -30,
     WEAPON_BEAM,
     {{0, 0}},
     0,
     WEAPON_PULSE},
    // 6 bat
    {SHIP_OUTLINE(hull_bat),
     {SHIP_OUTLINE(spine_bat), SHIP_NO_OUTLINE},
     NULL,
     0,
     {{-32, -3}, {-32, 3}, {0, 0}},
     2,
     {{20, 0}, {8, 0}},
     true,
     false,
     26,
     -32,
     WEAPON_PULSE,
     {{0, 0}},
     0,
     WEAPON_PULSE},
    // 7 leviathan
    {SHIP_OUTLINE(hull_leviathan),
     {SHIP_OUTLINE(spine_leviathan), SHIP_NO_OUTLINE},
     spikes_leviathan,
     4,
     {{-50, -2}, {-50, 2}, {0, 0}},
     2,
     {{44, 0}, {30, 0}},
     true,
     false,
     52,
     -50,
     WEAPON_TORPEDO,
     {{30, 0}, {8, 0}, {-14, 0}, {-34, 0}},
     4,
     WEAPON_MASS_DRIVER},
    // 8 wedge
    {SHIP_OUTLINE(hull_wedge),
     {SHIP_OUTLINE(plate_wedge), SHIP_OUTLINE(top_wedge)},
     NULL,
     0,
     {{-36, -10}, {-36, 0}, {-36, 10}},
     3,
     {{26, 0}, {8, 0}},
     true,
     false,
     42,
     -36,
     WEAPON_PULSE,
     {{10, 0}, {-10, -6}, {-10, 6}, {-26, 0}},
     4,
     WEAPON_BEAM},
    // 9 forward-swept
    {SHIP_OUTLINE(hull_swept),
     {SHIP_OUTLINE(spine_swept), SHIP_NO_OUTLINE},
     NULL,
     0,
     {{-26, -3}, {-26, 3}, {0, 0}},
     2,
     {{28, 0}, {16, 0}},
     true,
     false,
     36,
     -26,
     WEAPON_MASS_DRIVER,
     {{0, 0}},
     0,
     WEAPON_PULSE}};

// Maps design coordinates (tenths of a unit, origin at the design's own
// origin) to screen pixels. The ship is centred on x along its length.
typedef struct {
  int x, y, direction;
  double scale;                 // pixels per tenth
  double middle;                // design x that lands on `x`
  double cos_pitch, sin_pitch;  // tilt of the nose above the horizontal
} ShipTransform;

static void ship_to_pixels(const ShipTransform *xf, int tx, int ty, int *px, int *py) {
  double along = (tx - xf->middle) * xf->scale;
  double down = ty * xf->scale;
  *px = xf->x + (int)floor(xf->direction * (along * xf->cos_pitch + down * xf->sin_pitch) + 0.5);
  *py = xf->y + (int)floor(-along * xf->sin_pitch + down * xf->cos_pitch + 0.5);
}


// Fills px/py with the mirrored outline of a half outline; returns the point
// count.
static int ship_build_polygon(const ShipTransform *xf, const ShipOutline *outline, int *px,
                              int *py) {
  int count = 0;
  for (int i = 0; i < outline->count; i++) {
    ship_to_pixels(xf, outline->points[i][0], outline->points[i][1], &px[count], &py[count]);
    count++;
  }
  for (int i = outline->count - 2; i >= 1; i--) {
    ship_to_pixels(xf, outline->points[i][0], -outline->points[i][1], &px[count], &py[count]);
    count++;
  }
  return count;
}


static Uint8 ship_mix(int from, int to, double amount) {
  return (Uint8)(from + (to - from) * amount + 0.5);
}


// Colour of a lighting band (3 highlight .. 0 shadow), pushed lighter (bias >
// 0) or darker (bias < 0).
static void ship_tone(const ShipTeam *team, int band, double bias, Uint8 *rgb) {
  const Uint8 *target = team->tone[bias >= 0 ? 3 : 0];
  double amount = bias >= 0 ? bias : -bias;
  for (int i = 0; i < 3; i++) rgb[i] = ship_mix(team->tone[band][i], target[i], amount);
}


// The vertical stretches of a polygon on pixel column `col`, top to bottom.
static int ship_column_spans(const int *px, const int *py, int count, int col, int spans[][2]) {
  double x = col + 0.5;
  double crossings[24];
  int found = 0;
  for (int i = 0; i < count && found < 24; i++) {
    int j = (i + 1) % count;
    if ((px[i] <= x && x < px[j]) || (px[j] <= x && x < px[i])) {
      crossings[found++] = py[i] + (x - px[i]) * (double)(py[j] - py[i]) / (px[j] - px[i]);
    }
  }
  for (int i = 1; i < found; i++) {
    double value = crossings[i];
    int j = i - 1;
    while (j >= 0 && crossings[j] > value) {
      crossings[j + 1] = crossings[j];
      j--;
    }
    crossings[j + 1] = value;
  }
  int span_count = 0;
  for (int i = 0; i + 1 < found; i += 2) {
    int top = (int)ceil(crossings[i]);
    int bottom = (int)floor(crossings[i + 1]);
    if (bottom >= top && span_count < 12) {
      spans[span_count][0] = top;
      spans[span_count][1] = bottom;
      span_count++;
    }
  }
  return span_count;
}


// Paints one polygon in four bands of light with a dark outline.
static void ship_paint(RenderTarget *surface, const ShipTeam *team, const int *px, const int *py,
                       int count, double bias, int outline_width) {
  int min_x = px[0], max_x = px[0];
  for (int i = 1; i < count; i++) {
    if (px[i] < min_x) min_x = px[i];
    if (px[i] > max_x) max_x = px[i];
  }
  static const int bands[3] = {3, 2, 1};
  for (int col = min_x; col <= max_x; col++) {
    int spans[12][2];
    int span_count = ship_column_spans(px, py, count, col, spans);
    for (int span = 0; span < span_count; span++) {
      int top = spans[span][0], bottom = spans[span][1];
      int height = bottom - top + 1;
      int band_height[3];
      band_height[0] = (int)(height * 0.14 + 0.5);
      if (band_height[0] < 1) band_height[0] = 1;
      band_height[1] = (int)(height * 0.28 + 0.5);
      band_height[2] = (int)(height * 0.30 + 0.5);
      int y = top;
      Uint8 rgb[3];
      for (int band = 0; band < 3 && y <= bottom; band++) {
        if (band_height[band] <= 0) continue;
        int band_bottom = y + band_height[band] - 1;
        if (band_bottom > bottom) band_bottom = bottom;
        ship_tone(team, bands[band], bias, rgb);
        vlineRGBA(surface, col, y, band_bottom, rgb[0], rgb[1], rgb[2], 255);
        y = band_bottom + 1;
      }
      if (y <= bottom) {
        ship_tone(team, 0, bias, rgb);
        bool bounce = height >= 7;
        vlineRGBA(surface, col, y, bounce ? bottom - 1 : bottom, rgb[0], rgb[1], rgb[2], 255);
        if (bounce) {
          // A little bounced light along the lower edge.
          vlineRGBA(surface, col, bottom, bottom, ship_mix(rgb[0], team->glow[0], 0.35),
                    ship_mix(rgb[1], team->glow[1], 0.35), ship_mix(rgb[2], team->glow[2], 0.35),
                    255);
        }
      }
    }
  }
  for (int i = 0; i < count; i++) {
    int j = (i + 1) % count;
    thickLineRGBA(surface, px[i], py[i], px[j], py[j], outline_width, 8, 10, 18, 255);
  }
}


// Scratch render-target texture for drawing ships with opacity on GPU.
static SDL_Texture *ship_target_tex = NULL;
static int ship_target_w = 0, ship_target_h = 0;

// Draws one ship centred on (x, y), `length` pixels long, nose towards
// `direction` (1: right, -1: left) and tilted up by `pitch` radians, blended
// onto the target at `opacity` (0..255).
static void draw_ship_design(RenderTarget *surface, int x, int y, int design_index, int length,
                             int direction, int team_index, Uint8 opacity, double pitch = 0.0) {
  if (opacity == 0) return;
  const ShipDesign *design = &ship_designs[design_index];
  const ShipTeam *team = &ship_teams[team_index];
  ShipTransform xf;
  xf.x = x;
  xf.y = y;
  xf.direction = direction;
  xf.scale = length / (double)(design->nose - design->tail);
  xf.middle = (design->nose + design->tail) / 2.0;
  xf.cos_pitch = cos(pitch);
  xf.sin_pitch = sin(pitch);

  int reach = length + length / 4 + 12;
  int left = x - reach < 0 ? 0 : x - reach;
  int top = y - reach < 0 ? 0 : y - reach;
  int right = x + reach + 1 > surface->w ? surface->w : x + reach + 1;
  int bottom = y + reach + 1 > surface->h ? surface->h : y + reach + 1;
  if (right <= left || bottom <= top) return;
  SDL_Rect area;
  area.x = left;
  area.y = top;
  area.w = right - left;
  area.h = bottom - top;

  SDL_Texture *prev_target = SDL_GetRenderTarget(surface->renderer);
  RenderTarget draw_dst = *surface;
  RenderTarget *dst = surface;
  bool use_offscreen = (opacity < 255);
  SDL_Rect previous_clip;
  bool had_clip = SDL_RenderClipEnabled(surface->renderer);
  if (had_clip) SDL_GetRenderClipRect(surface->renderer, &previous_clip);

  if (use_offscreen) {
    if (!ship_target_tex || ship_target_w < area.w || ship_target_h < area.h) {
      int nw = ship_target_w > area.w ? ship_target_w : area.w;
      int nh = ship_target_h > area.h ? ship_target_h : area.h;
      if (ship_target_tex) SDL_DestroyTexture(ship_target_tex);
      ship_target_tex = SDL_CreateTexture(surface->renderer, SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_TARGET, nw, nh);
      if (ship_target_tex) {
        SDL_SetTextureBlendMode(ship_target_tex, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
        ship_target_w = nw;
        ship_target_h = nh;
      } else {
        use_offscreen = false;
      }
    }
  }

  if (use_offscreen) {
    SDL_SetRenderTarget(surface->renderer, ship_target_tex);
    SDL_Rect local_clip = {0, 0, area.w, area.h};
    SDL_SetRenderClipRect(surface->renderer, &local_clip);
    SDL_SetRenderDrawColor(surface->renderer, 0, 0, 0, 0);
    SDL_SetRenderDrawBlendMode(surface->renderer, SDL_BLENDMODE_NONE);
    SDL_FRect clear_rect = {0, 0, (float)area.w, (float)area.h};
    SDL_RenderFillRect(surface->renderer, &clear_rect);
    xf.x = x - area.x;
    xf.y = y - area.y;
    draw_dst.renderer = surface->renderer;
    draw_dst.w = area.w;
    draw_dst.h = area.h;
    dst = &draw_dst;
  } else {
    SDL_SetRenderClipRect(surface->renderer, &area);
  }

  int outline_width = xf.scale * 10.0 >= 16.0 ? 2 : 1;
  int engine_radius = (int)(SHIP_ENGINE_SIZE * xf.scale + 0.5);
  if (engine_radius < 1) engine_radius = 1;

  // Engine plumes go behind the hull.
  int nozzle_x[3], nozzle_y[3];
  for (int engine = 0; engine < design->engine_count; engine++) {
    ship_to_pixels(&xf, design->engines[engine][0], design->engines[engine][1], &nozzle_x[engine],
                   &nozzle_y[engine]);
    for (int layer = 0; layer < 4; layer++) {
      int plume = (int)(engine_radius * (8.0 - layer * 1.8));
      int plume_width = (int)(engine_radius * (1.3 - layer * 0.25));
      if (plume_width < 1) plume_width = 1;
      // The plume points straight back from the nose; its width is across the
      // ship.
      double tail_x = -direction * xf.cos_pitch, tail_y = xf.sin_pitch;
      double side_x = direction * xf.sin_pitch, side_y = xf.cos_pitch;
      filledTrigonRGBA(dst, nozzle_x[engine] + (int)(side_x * plume_width),
                       nozzle_y[engine] + (int)(side_y * plume_width),
                       nozzle_x[engine] - (int)(side_x * plume_width),
                       nozzle_y[engine] - (int)(side_y * plume_width),
                       nozzle_x[engine] + (int)(tail_x * plume),
                       nozzle_y[engine] + (int)(tail_y * plume), team->glow[0], team->glow[1],
                       team->glow[2], 36 + layer * 30);
    }
    filledCircleRGBA(dst, nozzle_x[engine], nozzle_y[engine], engine_radius * 2, team->glow[0],
                     team->glow[1], team->glow[2], 46);
  }

  int px[24], py[24];
  for (int spike = 0; spike < design->spike_count; spike++) {
    for (int side = -1; side <= 1; side += 2) {
      int spike_x = design->spikes[spike][0];
      int spike_height = design->spikes[spike][1];
      ship_to_pixels(&xf, spike_x, side * 7, &px[0], &py[0]);
      ship_to_pixels(&xf, spike_x - 5, side * spike_height, &px[1], &py[1]);
      ship_to_pixels(&xf, spike_x - 10, side * 7, &px[2], &py[2]);
      ship_paint(dst, team, px, py, 3, -0.30, outline_width);
    }
  }
  int count = ship_build_polygon(&xf, &design->hull, px, py);
  ship_paint(dst, team, px, py, count, 0.0, outline_width);
  for (int plate = 0; plate < 2; plate++) {
    if (design->plates[plate].count == 0) continue;
    count = ship_build_polygon(&xf, &design->plates[plate], px, py);
    ship_paint(dst, team, px, py, count, 0.16 * (plate + 1), 1);
  }

  if (design->accent_on) {
    int x1, y1, x2, y2;
    ship_to_pixels(&xf, design->accent[0][0], design->accent[0][1], &x1, &y1);
    ship_to_pixels(&xf, design->accent[1][0], design->accent[1][1], &x2, &y2);
    thickLineRGBA(dst, x1, y1, x2, y2, 3, team->accent[0], team->accent[1], team->accent[2], 70);
    thickLineRGBA(dst, x1, y1, x2, y2, 1, team->accent[0], team->accent[1], team->accent[2], 255);
  }
  // Gun turrets: a round mount with barrels that look different for each
  // weapon.
  for (int turret = 0; turret < design->turret_count; turret++) {
    int cx, cy;
    ship_to_pixels(&xf, design->turrets[turret][0], design->turrets[turret][1], &cx, &cy);
    int base = (int)(xf.scale * 3.4 + 0.5);
    if (base < 2) base = 2;
    double forward_x = direction * xf.cos_pitch, forward_y = -xf.sin_pitch;
    double across_x = -forward_y, across_y = forward_x;
    int weapon = design->turret_weapon;
    double barrel = base * (weapon == WEAPON_FLAK ? 1.7 : 2.5);
    int barrels = weapon == WEAPON_MASS_DRIVER ? 2 : 1;
    int barrel_width = (weapon == WEAPON_FLAK || base >= 4) ? 3 : 2;
    filledCircleRGBA(dst, cx, cy, base + 1, 8, 10, 18, 255);
    for (int b = 0; b < barrels; b++) {
      double shift = barrels == 2 ? (b ? 0.5 : -0.5) * base : 0.0;
      int sx = cx + (int)(across_x * shift), sy = cy + (int)(across_y * shift);
      int ex = sx + (int)(forward_x * barrel), ey = sy + (int)(forward_y * barrel);
      thickLineRGBA(dst, sx, sy, ex, ey, barrel_width + 2, 8, 10, 18, 255);
      thickLineRGBA(dst, sx, sy, ex, ey, barrel_width, team->tone[2][0], team->tone[2][1],
                    team->tone[2][2], 255);
      if (weapon == WEAPON_BEAM || weapon == WEAPON_PULSE) {
        filledCircleRGBA(dst, ex, ey, 1, team->accent[0], team->accent[1], team->accent[2], 255);
      }
    }
    filledCircleRGBA(dst, cx, cy, base, team->tone[2][0], team->tone[2][1], team->tone[2][2], 255);
    filledCircleRGBA(dst, cx - base / 3, cy - base / 3, (base + 2) / 3, team->tone[3][0],
                     team->tone[3][1], team->tone[3][2], 255);
  }
  if (design->eye) {
    int eye_x, eye_y;
    ship_to_pixels(&xf, 0, 0, &eye_x, &eye_y);
    int eye_unit = (int)(xf.scale * 10.0 + 0.5);
    filledCircleRGBA(dst, eye_x, eye_y, eye_unit * 6 / 10 + 2, team->accent[0], team->accent[1],
                     team->accent[2], 60);
    filledEllipseRGBA(dst, eye_x, eye_y, 3 * eye_unit / 10 + 2, eye_unit / 10 + 1, team->accent[0],
                      team->accent[1], team->accent[2], 255);
    filledCircleRGBA(dst, eye_x, eye_y, eye_unit / 10 < 1 ? 1 : eye_unit / 10, 255, 255, 255, 255);
  }
  // Hot engine cores sit on top of the hull.
  for (int engine = 0; engine < design->engine_count; engine++) {
    filledCircleRGBA(dst, nozzle_x[engine], nozzle_y[engine], engine_radius + 1,
                     ship_mix(team->glow[0], 255, 0.5), ship_mix(team->glow[1], 255, 0.5),
                     ship_mix(team->glow[2], 255, 0.5), 255);
    filledCircleRGBA(dst, nozzle_x[engine], nozzle_y[engine],
                     engine_radius / 2 < 1 ? 1 : engine_radius / 2, 255, 255, 255, 255);
  }

  if (use_offscreen) {
    SDL_SetRenderTarget(surface->renderer, prev_target);
    SDL_SetRenderClipRect(surface->renderer, had_clip ? &previous_clip : NULL);
    SDL_SetTextureColorMod(ship_target_tex, opacity, opacity, opacity);
    SDL_SetTextureAlphaMod(ship_target_tex, opacity);
    SDL_FRect src_f = {0, 0, (float)area.w, (float)area.h};
    SDL_FRect dst_f = {(float)area.x, (float)area.y, (float)area.w, (float)area.h};
    SDL_RenderTexture(surface->renderer, ship_target_tex, &src_f, &dst_f);
  } else {
    SDL_SetRenderClipRect(surface->renderer, had_clip ? &previous_clip : NULL);
  }
}


// The design that `variety` picks among the ships of a role.
static int ship_design_index(int role, unsigned int variety) {
  const int *designs = ship_small_designs;
  int count = (int)(sizeof(ship_small_designs) / sizeof(ship_small_designs[0]));
  if (role == SHIP_MID) {
    designs = ship_mid_designs;
    count = (int)(sizeof(ship_mid_designs) / sizeof(ship_mid_designs[0]));
  } else if (role == SHIP_BIG) {
    designs = ship_big_designs;
    count = (int)(sizeof(ship_big_designs) / sizeof(ship_big_designs[0]));
  }
  return designs[variety % (unsigned int)count];
}


// Draws a ship of the given role. `variety` picks one of the designs of that
// role.
static void draw_ship_role(RenderTarget *surface, int x, int y, int role, unsigned int variety,
                           int length, int direction, int team_index, Uint8 opacity,
                           double pitch = 0.0) {
  draw_ship_design(surface, x, y, ship_design_index(role, variety), length, direction, team_index,
                   opacity, pitch);
}


// Where a point of a design (the nose, a turret) ends up on screen.
static void ship_mount_point(int design_index, int x, int y, int length, int direction,
                             double pitch, int tx, int ty, double *out_x, double *out_y) {
  const ShipDesign *design = &ship_designs[design_index];
  double scale = length / (double)(design->nose - design->tail);
  double middle = (design->nose + design->tail) / 2.0;
  double along = (tx - middle) * scale;
  double down = ty * scale;
  *out_x = x + direction * (along * cos(pitch) + down * sin(pitch));
  *out_y = y - along * sin(pitch) + down * cos(pitch);
}


/*
 * Weapon and explosion effects
 *
 * Both are stateless: given how many milliseconds ago a shot was fired they
 * draw exactly what is on screen now.
 */
static const Uint8 weapon_team_colors[2][3] = {{110, 205, 235}, {255, 165, 105}};

static unsigned int fx_hash(unsigned int a, unsigned int b) {
  unsigned int value = a * 2654435761u + b * 2246822519u + 0x9e3779b9u;
  value ^= value >> 15;
  value *= 2246822519u;
  value ^= value >> 13;
  return value;
}


static Uint8 weapon_alpha(double alpha) {
  if (alpha < 0.0) alpha = 0.0;
  if (alpha > 255.0) alpha = 255.0;
  return (Uint8)alpha;
}


// Radius a shot of a ship `length` pixels long is drawn at.
static double weapon_unit(int length) {
  double unit = 10.0 + length * 0.25;
  return unit > 34.0 ? 34.0 : unit;
}


// A soft round glow: a faint halo, a brighter body and a hot core.
static void weapon_glow(RenderTarget *surface, double x, double y, double radius,
                        const Uint8 *color, double alpha) {
  int r = (int)(radius + 0.5);
  if (r < 1) r = 1;
  filledCircleRGBA(surface, (int)x, (int)y, r * 2, color[0], color[1], color[2],
                   weapon_alpha(alpha * 0.22));
  filledCircleRGBA(surface, (int)x, (int)y, r + r / 2, color[0], color[1], color[2],
                   weapon_alpha(alpha * 0.55));
  filledCircleRGBA(surface, (int)x, (int)y, (r + 1) / 2, 255, 255, 255, weapon_alpha(alpha));
}


// An explosion `age` ms after it began: a white flash, a fireball of
// overlapping blobs that cools from yellow through orange to grey smoke, a
// shockwave ring, glowing debris, and for a destroyed ship tumbling hull
// fragments and two smaller blasts after the first.
static void draw_explosion(RenderTarget *surface, double x, double y, double age, double radius,
                           bool big, unsigned int seed, int team, double fade) {
  if (radius < 2.0) radius = 2.0;
  double life = big ? 1700.0 : 380.0 + radius * 10.0;
  if (!big && life > 900.0) life = 900.0;
  if (age < 0.0 || age >= life || fade <= 0.0) return;
  double p = age / life;
  double inv = 1.0 - p;

  static const Uint8 stops[4][3] = {{255, 238, 190}, {246, 142, 62}, {118, 50, 34}, {60, 58, 66}};
  double position = p * 3.0;
  int stop = (int)position;
  if (stop > 2) stop = 2;
  double between = position - stop;
  Uint8 red = ship_mix(stops[stop][0], stops[stop + 1][0], between);
  Uint8 green = ship_mix(stops[stop][1], stops[stop + 1][1], between);
  Uint8 blue = ship_mix(stops[stop][2], stops[stop + 1][2], between);
  double body = 215.0 * fade * pow(inv, 1.3);
  int blobs = big ? 9 : 5;
  for (int blob = 0; blob < blobs; blob++) {
    unsigned int h = fx_hash(seed, blob);
    double angle = (h % 6283) / 1000.0;
    double spread = ((h >> 12) % 100) / 100.0;
    double distance = radius * (0.15 + 0.7 * spread) * (0.3 + 0.7 * sqrt(p));
    double size =
        radius * (0.35 + 0.35 * ((h >> 20) % 100) / 100.0) * (0.3 + 0.7 * smooth_step(0.0, 0.3, p));
    filledCircleRGBA(surface, (int)(x + cos(angle) * distance), (int)(y + sin(angle) * distance),
                     (int)size + 1, red, green, blue, weapon_alpha(body * 0.8));
  }
  if (p < 0.3) {
    double hot = 1.0 - p / 0.3;
    filledCircleRGBA(surface, (int)x, (int)y, (int)(radius * (0.2 + 0.6 * hot)) + 1, 255, 250, 235,
                     weapon_alpha(240.0 * fade * hot));
  }
  double ring = radius * (0.5 + 2.2 * sqrt(p));
  circleRGBA(surface, (int)x, (int)y, (int)ring, 255, 214, 160,
             weapon_alpha(170.0 * fade * inv * inv));
  if (big) {
    circleRGBA(surface, (int)x, (int)y, (int)(ring * 0.7), 255, 180, 110,
               weapon_alpha(120.0 * fade * inv * inv));
  }
  int pieces = big ? 16 : 8;
  for (int piece = 0; piece < pieces; piece++) {
    unsigned int h = fx_hash(seed + 77u, piece);
    double angle = (h % 6283) / 1000.0;
    double speed = radius * (1.2 + 2.0 * ((h >> 12) % 100) / 100.0);
    double reach = speed * (1.0 - pow(inv, 2.2));
    double trail = radius * 0.55 * inv;
    thickLineRGBA(surface, (int)(x + cos(angle) * reach), (int)(y + sin(angle) * reach),
                  (int)(x + cos(angle) * (reach - trail)), (int)(y + sin(angle) * (reach - trail)),
                  1, 255, 215 - piece * 3, 120, weapon_alpha(230.0 * fade * inv));
  }
  if (big) {
    const ShipTeam *hull = &ship_teams[team];
    for (int fragment = 0; fragment < 6; fragment++) {
      unsigned int h = fx_hash(seed + 191u, fragment);
      double angle = (h % 6283) / 1000.0;
      double speed = radius * (0.5 + 1.1 * ((h >> 12) % 100) / 100.0);
      double reach = speed * (1.0 - pow(inv, 1.6));
      double cx = x + cos(angle) * reach;
      double cy = y + sin(angle) * reach * 0.8;
      double spin = ((h >> 8) % 628) / 100.0 + p * (3.0 + fragment);
      double size = radius * 0.2;
      int px[3], py[3];
      for (int corner = 0; corner < 3; corner++) {
        double corner_angle = spin + corner * 2.0944;
        double corner_size = size * (corner == 0 ? 1.5 : 0.8);
        px[corner] = (int)(cx + cos(corner_angle) * corner_size);
        py[corner] = (int)(cy + sin(corner_angle) * corner_size);
      }
      Uint8 piece_alpha = weapon_alpha(255.0 * fade * inv);
      filledTrigonRGBA(surface, px[0], py[0], px[1], py[1], px[2], py[2], hull->tone[2][0],
                       hull->tone[2][1], hull->tone[2][2], piece_alpha);
      lineRGBA(surface, px[0], py[0], px[1], py[1], hull->glow[0], hull->glow[1], hull->glow[2],
               weapon_alpha(piece_alpha * 0.8));
    }
    if (age > 160.0) {
      draw_explosion(surface, x + radius * 0.6, y - radius * 0.25, age - 160.0, radius * 0.55,
                     false, seed + 1u, team, fade);
    }
    if (age > 340.0) {
      draw_explosion(surface, x - radius * 0.55, y + radius * 0.3, age - 340.0, radius * 0.5, false,
                     seed + 2u, team, fade);
    }
  }
}


// Point on the route of a guided shot: the straight line from the muzzle to the
// target, bent sideways by up to `bend` pixels half way.
static void weapon_route(double fx, double fy, double dx, double dy, double nx, double ny,
                         double fraction, double bend, double *x, double *y) {
  double arc = fraction < 1.0 ? bend * sin(3.14159265359 * fraction) : 0.0;
  *x = fx + dx * fraction + nx * arc;
  *y = fy + dy * fraction + ny * arc;
}


/*
 * One shot of `weapon`, fired from (fx, fy) at (tx, ty), `age` ms ago.
 * outcome: 0 the shot misses (guided weapons are shot down half way),
 *          1 it hits, 2 it hits and destroys the target (the caller draws
 * that). `unit` is the size of a shot in pixels (see weapon_unit), `shot` the
 * index of the shot within its volley.
 */
static void draw_weapon_shot(RenderTarget *surface, int weapon, int shot, double fx, double fy,
                             double tx, double ty, double age, double unit, int team, double fade,
                             int outcome, unsigned int seed) {
  const WeaponSpec *spec = &weapon_specs[weapon];
  if (age < 0.0 || age >= spec->life_ms || fade <= 0.0) return;
  const Uint8 *color = weapon_team_colors[team];
  bool guided = weapon == WEAPON_MISSILE || weapon == WEAPON_SWARM || weapon == WEAPON_TORPEDO;
  if (guided && outcome == 0) {
    // Shot down by point defence before it arrives.
    tx = fx + (tx - fx) * 0.7;
    ty = fy + (ty - fy) * 0.7;
  }
  double dx = tx - fx, dy = ty - fy;
  double span = sqrt(dx * dx + dy * dy);
  if (span < 1.0) span = 1.0;
  double ux = dx / span, uy = dy / span;
  double nx = -uy, ny = ux;
  double p = age / spec->impact_ms;
  double after = age - spec->impact_ms;
  bool lands = outcome != 0 || guided;
  bool arrived = lands && p >= 1.0;
  double miss_fade = 1.0;
  if (!lands && p > 1.0) miss_fade = 1.0 - (p - 1.0) / (spec->life_ms / spec->impact_ms - 1.0);
  double blast = unit * spec->blast * 1.6;
  if (age < 90.0 && weapon != WEAPON_RAIL && weapon != WEAPON_BEAM) {
    weapon_glow(surface, fx, fy, unit * 0.2 * (1.0 - age / 90.0), color, 230.0 * fade);
  }

  switch (weapon) {
    case WEAPON_PULSE: {
      if (!arrived) {
        double hx = fx + dx * p, hy = fy + dy * p;
        double tail = unit * 0.55;
        thickLineRGBA(surface, (int)(hx - ux * tail), (int)(hy - uy * tail), (int)hx, (int)hy, 2,
                      color[0], color[1], color[2], weapon_alpha(170.0 * fade * miss_fade));
        thickLineRGBA(surface, (int)(hx - ux * tail * 0.55), (int)(hy - uy * tail * 0.55), (int)hx,
                      (int)hy, 1, 255, 255, 255, weapon_alpha(240.0 * fade * miss_fade));
      }
      break;
    }
    case WEAPON_MASS_DRIVER: {
      double offset = ((shot % 3) - 1) * unit * 0.1;
      if (!arrived) {
        double hx = fx + dx * p + nx * offset, hy = fy + dy * p + ny * offset;
        double tail = unit * 0.55;
        thickLineRGBA(surface, (int)(hx - ux * tail), (int)(hy - uy * tail), (int)hx, (int)hy, 1,
                      255, 232, 160, weapon_alpha(235.0 * fade * miss_fade));
        pixelRGBA(surface, (int)hx, (int)hy, 255, 255, 255, weapon_alpha(255.0 * fade));
      }
      break;
    }
    case WEAPON_PLASMA: {
      static const Uint8 plasma[3] = {140, 255, 170};
      if (!arrived) {
        double radius = unit * 0.2 * (1.0 + 0.12 * sin(age / 45.0));
        for (int k = 5; k >= 1; k--) {
          double pk = p - k * 0.05;
          if (pk < 0.0) continue;
          weapon_glow(surface, fx + dx * pk, fy + dy * pk, radius * (1.0 - k * 0.13), plasma,
                      150.0 * fade * miss_fade * (1.0 - k / 6.0));
        }
        weapon_glow(surface, fx + dx * p, fy + dy * p, radius, plasma, 235.0 * fade * miss_fade);
      } else if (outcome != 0 && after < 500.0) {
        circleRGBA(surface, (int)tx, (int)ty, (int)(unit * 0.3 + after * unit * 0.0018), plasma[0],
                   plasma[1], plasma[2], weapon_alpha(200.0 * fade * (1.0 - after / 500.0)));
      }
      break;
    }
    case WEAPON_FLAK: {
      double bx = fx + dx * 0.92, by = fy + dy * 0.92;
      if (p < 1.0) {
        double hx = fx + dx * 0.92 * p, hy = fy + dy * 0.92 * p;
        double tail = unit * 0.4;
        thickLineRGBA(surface, (int)(hx - ux * tail), (int)(hy - uy * tail), (int)hx, (int)hy, 2,
                      255, 185, 90, weapon_alpha(210.0 * fade));
        pixelRGBA(surface, (int)hx, (int)hy, 255, 255, 255, weapon_alpha(255.0 * fade));
      } else if (after < 560.0) {
        double burst = after / 560.0;
        for (int piece = 0; piece < 14; piece++) {
          unsigned int h = fx_hash(seed + 5u, piece);
          double angle = piece * 0.4488 + (h % 100) * 0.003;
          double near_edge = unit * (0.15 + 0.95 * sqrt(burst));
          double far_edge = near_edge + unit * 0.4 * (1.0 - burst);
          thickLineRGBA(surface, (int)(bx + cos(angle) * near_edge),
                        (int)(by + sin(angle) * near_edge), (int)(bx + cos(angle) * far_edge),
                        (int)(by + sin(angle) * far_edge), 1, 255, 215, 120,
                        weapon_alpha(235.0 * fade * (1.0 - burst)));
        }
        filledCircleRGBA(surface, (int)bx, (int)by, (int)(unit * 0.3 * (0.5 + burst)) + 1, 110, 96,
                         84, weapon_alpha(110.0 * fade * (1.0 - burst)));
        if (after < 140.0) {
          filledCircleRGBA(surface, (int)bx, (int)by, (int)(unit * 0.45), 255, 245, 210,
                           weapon_alpha(230.0 * fade * (1.0 - after / 140.0)));
        }
      }
      if (outcome == 1) draw_explosion(surface, bx, by, after, blast, false, seed, team, fade);
      return;
    }
    case WEAPON_MISSILE:
    case WEAPON_SWARM:
    case WEAPON_TORPEDO: {
      bool swarm = weapon == WEAPON_SWARM, torpedo = weapon == WEAPON_TORPEDO;
      double bend = torpedo ? 0.0
                    : swarm ? unit * (0.9 + 0.35 * (shot % 3)) * (shot % 2 ? 1.0 : -1.0)
                            : unit * 0.7 * ((seed & 1) ? 1.0 : -1.0);
      double step = torpedo ? 0.035 : 0.05;
      double smoke_size = torpedo ? 3.0 : (swarm ? 1.0 : 1.5);
      double smoke_fade = p < 1.0 ? 1.0 : 1.0 - after / (spec->life_ms - spec->impact_ms);
      if (smoke_fade < 0.0) smoke_fade = 0.0;
      double reach = p < 1.0 ? p : 1.0;
      for (int k = 1; k <= 10; k++) {
        double pk = reach - k * step;
        if (pk < 0.0) break;
        double sx, sy;
        weapon_route(fx, fy, dx, dy, nx, ny, pk, bend, &sx, &sy);
        filledCircleRGBA(surface, (int)sx, (int)sy, (int)(smoke_size + k / 3.0), 176, 176, 188,
                         weapon_alpha(90.0 * fade * smoke_fade * (1.0 - k / 11.0)));
      }
      if (p < 1.0) {
        double hx, hy, ax, ay;
        weapon_route(fx, fy, dx, dy, nx, ny, p, bend, &hx, &hy);
        weapon_route(fx, fy, dx, dy, nx, ny, p + 0.01, bend, &ax, &ay);
        double bx = ax - hx, by = ay - hy;
        double bl = sqrt(bx * bx + by * by);
        if (bl < 0.001) {
          bx = ux;
          by = uy;
        } else {
          bx /= bl;
          by /= bl;
        }
        double body = unit * (torpedo ? 0.7 : (swarm ? 0.22 : 0.42));
        int width = torpedo ? 3 : (swarm ? 1 : 2);
        double tail_x = hx - bx * body, tail_y = hy - by * body;
        double flame = body * (0.9 + 0.3 * sin(age / 25.0 + shot));
        static const Uint8 flame_color[3] = {255, 170, 80};
        static const Uint8 torpedo_flame[3] = {120, 190, 255};
        weapon_glow(surface, tail_x, tail_y, torpedo ? unit * 0.28 : unit * 0.12,
                    torpedo ? torpedo_flame : flame_color, 200.0 * fade);
        thickLineRGBA(surface, (int)tail_x, (int)tail_y, (int)(tail_x - bx * flame),
                      (int)(tail_y - by * flame), width, 255, 200, 120, weapon_alpha(210.0 * fade));
        thickLineRGBA(surface, (int)tail_x, (int)tail_y, (int)hx, (int)hy,
                      width + (torpedo ? 1 : 0), torpedo ? 150 : 205, torpedo ? 160 : 208,
                      torpedo ? 190 : 218, weapon_alpha(255.0 * fade));
        filledCircleRGBA(surface, (int)hx, (int)hy, torpedo ? 2 : 1, 255, torpedo ? 90 : 255,
                         torpedo ? 80 : 255, weapon_alpha(255.0 * fade));
      }
      if (arrived && outcome != 2) {
        draw_explosion(surface, tx, ty, after, blast, torpedo && outcome == 1, seed, team, fade);
      }
      return;
    }
    case WEAPON_RAIL: {
      if (age < 300.0) {
        double charge = age / 300.0;
        weapon_glow(surface, fx, fy, unit * 0.5 * charge, color, 230.0 * fade * charge);
        for (int tick = 0; tick < 4; tick++) {
          double angle = tick * 1.5708 + 0.4;
          double outer = unit * (1.1 - 0.8 * charge);
          lineRGBA(surface, (int)(fx + cos(angle) * outer), (int)(fy + sin(angle) * outer),
                   (int)(fx + cos(angle) * outer * 0.6), (int)(fy + sin(angle) * outer * 0.6), 255,
                   255, 255, weapon_alpha(210.0 * fade * charge));
        }
      } else {
        double since = age - 300.0;
        double f = (1.0 - since / 550.0) * fade;
        double ex = tx, ey = ty;
        if (outcome == 0) {
          ex = tx + ux * span * 0.6;
          ey = ty + uy * span * 0.6;
        }
        thickLineRGBA(surface, (int)fx, (int)fy, (int)ex, (int)ey, 5, color[0], color[1], color[2],
                      weapon_alpha(70.0 * f));
        thickLineRGBA(surface, (int)fx, (int)fy, (int)ex, (int)ey, 3, color[0], color[1], color[2],
                      weapon_alpha(150.0 * f));
        thickLineRGBA(surface, (int)fx, (int)fy, (int)ex, (int)ey, 1, 255, 255, 255,
                      weapon_alpha(255.0 * f));
        circleRGBA(surface, (int)fx, (int)fy, (int)(unit * 0.15 + since * unit * 0.001), color[0],
                   color[1], color[2], weapon_alpha(170.0 * f));
        if (outcome == 1) draw_explosion(surface, tx, ty, since, blast, false, seed, team, fade);
      }
      return;
    }
    case WEAPON_BEAM: {
      if (age < 900.0) {
        double sweep =
            outcome == 0 ? unit * 1.1 * (1.0 - 2.0 * age / 900.0) : unit * 0.06 * sin(age / 23.0);
        double ex = tx + nx * sweep, ey = ty + ny * sweep;
        double reach = smooth_step(0.0, 70.0, age);
        double hx = fx + (ex - fx) * reach, hy = fy + (ey - fy) * reach;
        double f = smooth_step(0.0, 60.0, age) * (1.0 - smooth_step(650.0, 900.0, age)) * fade;
        int thick = 1 + (int)(unit / 16.0);
        thickLineRGBA(surface, (int)fx, (int)fy, (int)hx, (int)hy, thick * 2 + 3, color[0],
                      color[1], color[2], weapon_alpha(50.0 * f));
        thickLineRGBA(surface, (int)fx, (int)fy, (int)hx, (int)hy, thick + 1, color[0], color[1],
                      color[2], weapon_alpha(130.0 * f));
        thickLineRGBA(surface, (int)fx, (int)fy, (int)hx, (int)hy, thick, 255, 255, 255,
                      weapon_alpha(245.0 * f));
        weapon_glow(surface, fx, fy, unit * 0.22, color, 220.0 * f);
        weapon_glow(surface, hx, hy, unit * 0.28, color, 220.0 * f);
        if (outcome != 0 && reach >= 1.0) {
          for (int spark = 0; spark < 4; spark++) {
            unsigned int h = fx_hash(seed + (unsigned int)(age / 45.0), spark);
            double angle = (h % 6283) / 1000.0;
            double length = unit * (0.3 + ((h >> 12) % 60) / 100.0);
            lineRGBA(surface, (int)hx, (int)hy, (int)(hx + cos(angle) * length),
                     (int)(hy + sin(angle) * length), 255, 230, 170, weapon_alpha(220.0 * f));
          }
        }
      }
      if (outcome == 1 && after > 0.0) {
        draw_explosion(surface, tx, ty, after, blast, false, seed, team, fade);
      }
      return;
    }
  }
  // Pulse, mass driver and plasma end in a small blast where they hit.
  if (arrived && outcome == 1) {
    draw_explosion(surface, tx, ty, after, blast < 2.0 ? 2.0 : blast, false, seed, team, fade);
  }
}


static void render_fullscreen_event(RenderTarget *surface, Uint32 ticks, int scale,
                                    const SceneWeather *weather) {
  EffectWindow window;
  if (!effect_window(&EFFECT_FULLSCREEN, ticks, &window)) return;
  Uint32 seed = window.seed;
  Uint32 phase = window.phase;
  double fade = window.fade;
  // Kinds 4 and 5 are quiet slots that fullscreen_accept() already filters out.
  // When forced, fold them back so the effect always shows something.
  int kind = (seed >> 5) % 6;
  if (forced_effect == &EFFECT_FULLSCREEN) kind %= 4;
  kind = effect_kind(&EFFECT_FULLSCREEN, kind);
  if (kind >= 4) return;
  if (kind == 0) {
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 62, 94, 132, (Uint8)(10 * fade));
    // Top up to full strength without doubling up when the aurora weather is
    // on.
    draw_aurora(surface, ticks, scale, fade * (1.0 - weather->aurora));
  } else if (kind == 1) {
    Uint32 flash_phase = phase % 3400;
    double flash =
        smooth_step(0.0, 90.0, flash_phase) * (1.0 - smooth_step(170.0, 430.0, flash_phase));
    Uint8 alpha = (Uint8)(24 * fade * flash);
    if (alpha > 0) {
      boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 170, 192, 220, alpha);
    }
  } else if (kind == 2) {
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 17, 25, 44, (Uint8)(28 * fade));
    int radius = scale / 8;
    int moon_x = surface->w * (25 + (seed >> 12) % 50) / 100;
    int moon_y = surface->h * (18 + (seed >> 20) % 24) / 100;
    filledCircleRGBA(surface, moon_x, moon_y, radius + scale / 18, 119, 158, 194,
                     (Uint8)(24 * fade));
    filledCircleRGBA(surface, moon_x, moon_y, radius, 171, 183, 190, (Uint8)(190 * fade));
    filledCircleRGBA(surface, moon_x + radius / 3, moon_y, radius - radius / 7, 22, 27, 38,
                     (Uint8)(220 * fade));
  } else {
    int flare_x = surface->w * (28 + (seed >> 12) % 44) / 100;
    int flare_y = surface->h * (18 + (seed >> 20) % 25) / 100;
    int radius = scale / 13;
    boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 177, 82, 45, (Uint8)(13 * fade));
    filledCircleRGBA(surface, flare_x, flare_y, radius, 245, 137, 75, (Uint8)(45 * fade));
    filledCircleRGBA(surface, flare_x, flare_y, radius / 3, 255, 222, 169, (Uint8)(90 * fade));
    for (int ray = 0; ray < 12; ray++) {
      double angle = ray * 0.52359877559 + ticks / 9000.0;
      int end_x = flare_x + (int)(cos(angle) * scale * 0.42);
      int end_y = flare_y + (int)(sin(angle) * scale * 0.30);
      thickLineRGBA(surface, flare_x, flare_y, end_x, end_y, 1, 251, 174, 105, (Uint8)(18 * fade));
    }
  }
}


/*
 * Dogfights
 *
 * Like every effect a dogfight is stateless: where each ship is, which way it
 * points and which shots are in the air are all worked out from the time since
 * the effect began. A dogfight is made of pairs of ships, one of each team.
 * Each pair flies several passes across the sky. In a pass the leader flies a
 * route of wide loops, and the chaser follows the same route a little behind,
 * closing and dropping back, and fires its weapon at it. Most volleys miss; one
 * hits. The leader that was hit trails smoke for the rest of the pass, and on
 * the last pass it is destroyed. The next pass goes the other way with the
 * roles swapped.
 */
#define DOG_BURSTS 5     // volleys the chaser fires in a pass
#define DOG_HIT_BURST 3  // the volley that hits (the middle shot of it)

typedef struct {
  RenderTarget *surface;
  int scale;
  Uint32 seed;
  int pair_count;
  int pass_count;
  double pass_ms;     // length of one pass
  double stagger_ms;  // delay between the start of one pair and the next
  int length;         // ship length in pixels
} Dogfight;

typedef struct {
  bool visible;
  double x, y;
  int direction;  // 1: nose points right, -1: left
  double pitch;   // nose above the horizontal, radians
  double u;       // 0: entering the screen, 1: leaving it
  int pass;
  int team;
} DogPose;

static unsigned int dog_hash(unsigned int a, unsigned int b) { return fx_hash(a, b); }


// A point of the looping route of pair `pair` on pass `pass`; u runs 0..1
// across the screen.
static void dog_route_point(const Dogfight *fight, int pair, int pass, double u, double *x,
                            double *y) {
  int width = fight->surface->w;
  int loops = (int)(width / (fight->scale * 0.4) + 0.5);
  if (loops < 2) loops = 2;
  if (loops > 6) loops = 6;
  double margin = fight->scale / 10.0;
  double travel = width + 2.0 * margin;
  double drift = travel / (6.28318530718 * loops);
  double reach = drift * 1.45;  // wider than the drift, so the route loops
  double height = fight->scale * 0.032;
  double start = (dog_hash(fight->seed, pair * 8 + pass) % 6283) / 1000.0;
  double angle = 6.28318530718 * loops * u + start;
  double along = -margin + travel * u - reach * (sin(angle) - sin(start));
  *x = (pass % 2) ? width - along : along;
  *y = fight->surface->h * 0.078 - height * cos(angle);
}


// Where a ship of `pair` is at time t (ms since the effect began). Role 0
// leads, role 1 chases.
static void dog_pose(const Dogfight *fight, int pair, int role, double t, DogPose *pose) {
  pose->visible = false;
  double local = t - pair * fight->stagger_ms;
  if (local < 0.0 || local >= fight->pass_count * fight->pass_ms) return;
  int pass = (int)(local / fight->pass_ms);
  double u = (local - pass * fight->pass_ms) / fight->pass_ms * 1.15;
  if (role == 1) u -= 0.1 * (1.0 + 0.4 * sin(u * 9.0 + pair + pass));
  double before_x, before_y, after_x, after_y;
  dog_route_point(fight, pair, pass, u - 0.003, &before_x, &before_y);
  dog_route_point(fight, pair, pass, u + 0.003, &after_x, &after_y);
  dog_route_point(fight, pair, pass, u, &pose->x, &pose->y);
  double heading = atan2(-(after_y - before_y), after_x - before_x);
  if (after_x >= before_x) {
    pose->direction = 1;
    pose->pitch = heading;
  } else {
    pose->direction = -1;
    pose->pitch = atan2(sin(3.14159265359 - heading), cos(3.14159265359 - heading));
  }
  pose->u = u;
  pose->pass = pass;
  pose->team = (role + pass) % 2;
  pose->visible = pose->x > -fight->length && pose->x < fight->surface->w + fight->length;
}


// When, within a pass, a volley starts; and when its shot number `bolt` is
// fired.
static double dog_shot_time(const Dogfight *fight, int burst, int bolt, int weapon) {
  return (0.18 + 0.15 * burst) * fight->pass_ms + bolt * weapon_specs[weapon].gap_ms;
}


// When, within a pass, the leader is hit by the chaser's weapon.
static double dog_hit_time(const Dogfight *fight, int weapon) {
  return dog_shot_time(fight, DOG_HIT_BURST, weapon_specs[weapon].shots / 2, weapon) +
         weapon_specs[weapon].impact_ms;
}


static void render_dogfight(RenderTarget *surface, const EffectWindow *window, int scale,
                            int pair_count, int pass_count, double stagger) {
  Dogfight fight;
  fight.surface = surface;
  fight.scale = scale;
  fight.seed = window->seed;
  fight.pair_count = pair_count;
  fight.pass_count = pass_count;
  fight.pass_ms = (window->duration - 800.0) / (pass_count + (pair_count - 1) * stagger);
  fight.stagger_ms = fight.pass_ms * stagger;
  fight.length = scale / 34;
  if (fight.length < 14) fight.length = 14;
  double t = window->phase;
  double fade = window->fade;
  Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
  double unit = weapon_unit(fight.length);

  for (int pair = 0; pair < pair_count; pair++) {
    double local = t - pair * fight.stagger_ms;
    if (local < 0.0 || local >= pass_count * fight.pass_ms) continue;
    int pass = (int)(local / fight.pass_ms);
    double pass_age = local - pass * fight.pass_ms;
    bool last_pass = pass == pass_count - 1;
    double pass_start = pair * fight.stagger_ms + pass * fight.pass_ms;
    // The chaser is the second ship; it and the leader swap teams every pass.
    int chaser_team = (1 + pass) % 2, leader_team = pass % 2;
    int chaser_design =
        ship_design_index(SHIP_SMALL, dog_hash(fight.seed, pair * 2 + chaser_team) >> 4);
    int weapon = ship_designs[chaser_design].weapon;
    const WeaponSpec *spec = &weapon_specs[weapon];
    double hit_time = dog_hit_time(&fight, weapon);
    bool leader_hit = pass_age >= hit_time;
    bool leader_destroyed = leader_hit && last_pass;

    // Engine trails, and smoke behind the leader once it has been hit.
    for (int role = 0; role < 2; role++) {
      DogPose pose;
      dog_pose(&fight, pair, role, t, &pose);
      if (!pose.visible || (role == 0 && leader_destroyed)) continue;
      double last_x = pose.x, last_y = pose.y;
      for (int step = 1; step <= 10; step++) {
        DogPose earlier;
        dog_pose(&fight, pair, role, t - step * 50.0, &earlier);
        if (!earlier.visible || earlier.pass != pose.pass) break;
        thickLineRGBA(surface, (int)last_x, (int)last_y, (int)earlier.x, (int)earlier.y, 1,
                      ship_teams[pose.team].glow[0], ship_teams[pose.team].glow[1],
                      ship_teams[pose.team].glow[2], (Uint8)(95 * fade * (1.0 - step / 11.0)));
        last_x = earlier.x;
        last_y = earlier.y;
      }
      if (role == 0 && leader_hit) {
        for (int puff = 1; puff <= 9; puff++) {
          double puff_time = t - puff * 110.0;
          if (puff_time - pass_start < hit_time) break;
          DogPose smoke;
          dog_pose(&fight, pair, 0, puff_time, &smoke);
          if (!smoke.visible || smoke.pass != pose.pass) break;
          filledCircleRGBA(surface, (int)smoke.x, (int)smoke.y, 1 + puff / 2, 150, 150, 160,
                           (Uint8)(70 * fade * (1.0 - puff / 10.0)));
        }
      }
    }

    // The ships.
    for (int role = 0; role < 2; role++) {
      DogPose pose;
      dog_pose(&fight, pair, role, t, &pose);
      if (!pose.visible || (role == 0 && leader_destroyed)) continue;
      unsigned int variety = dog_hash(fight.seed, pair * 2 + pose.team) >> 4;
      draw_ship_role(surface, (int)floor(pose.x + 0.5), (int)floor(pose.y + 0.5), SHIP_SMALL,
                     variety, fight.length, pose.direction, pose.team, opacity, pose.pitch);
      if (role == 0 && leader_hit && !leader_destroyed) {
        // A burning patch where it was hit.
        filledCircleRGBA(surface, (int)pose.x, (int)pose.y, fight.length / 5 + 1, 255, 140, 60,
                         (Uint8)(110 * fade * (0.5 + 0.5 * sin(t / 70.0))));
      }
    }

    // Shots from the chaser at the leader, using the chaser's own weapon.
    for (int burst = 0; burst < DOG_BURSTS; burst++) {
      if (last_pass && burst > DOG_HIT_BURST) break;
      for (int bolt = 0; bolt < spec->shots; bolt++) {
        double fired = pass_start + dog_shot_time(&fight, burst, bolt, weapon);
        double age = t - fired;
        if (age < 0.0 || age >= spec->life_ms) continue;
        bool hit = burst == DOG_HIT_BURST && bolt == spec->shots / 2;
        // A beam stays on its target, so it follows both ships live.
        bool live = weapon == WEAPON_BEAM;
        DogPose shooter, target;
        dog_pose(&fight, pair, 1, live ? t : fired, &shooter);
        dog_pose(&fight, pair, 0, live ? t : fired + spec->impact_ms, &target);
        if (!shooter.visible || !target.visible) continue;
        double from_x, from_y;
        ship_mount_point(chaser_design, (int)shooter.x, (int)shooter.y, fight.length,
                         shooter.direction, shooter.pitch, ship_designs[chaser_design].nose, 0,
                         &from_x, &from_y);
        double to_x = target.x, to_y = target.y;
        if (!hit) {
          // Misses pass to one side or the other of the target.
          double side = ((burst * 3 + bolt) % 2 ? 1.0 : -1.0) * fight.length *
                        (0.7 + 0.2 * ((burst + bolt) % 3));
          double dx = to_x - from_x, dy = to_y - from_y;
          double span = sqrt(dx * dx + dy * dy) + 1.0;
          to_x += -dy / span * side;
          to_y += dx / span * side;
        }
        draw_weapon_shot(surface, weapon, bolt, from_x, from_y, to_x, to_y, age, unit, chaser_team,
                         fade, hit ? (last_pass ? 2 : 1) : 0,
                         dog_hash(fight.seed, pair * 64 + pass * 16 + burst * 4 + bolt));
      }
    }

    // The destroyed leader blows up where it was when the shot landed.
    if (leader_destroyed) {
      DogPose last;
      dog_pose(&fight, pair, 0, pass_start + hit_time, &last);
      draw_explosion(surface, last.x, last.y, pass_age - hit_time, fight.length * 1.0, true,
                     dog_hash(fight.seed, pair * 5 + 1), leader_team, fade);
    }
  }
}


// Flyby scenarios:
//   0: classic head-on pass firing pulse bolts.
//   1: pursuit ambush — a heavy mid-class raider chases two weaving fighters
//      across the sky, firing plasma/flak volleys behind them.
//   2: stealth torpedo run — a lone bomber swoops low, fires a glowing torpedo
//      at a distant silhouette cruiser, and banks away into the upper sky.
//   3: FTL slingshot — a cruiser drops out of hyperspace in a bright flash,
//      fires a broadside salvo at a patrolling picket, then accelerates to
//      warp.
//   4: blockade break — a fast runner threads between two hovering gunboats
//      firing crossfire flak bursts across its path.
//   5: stealth decloak strike — a cloaked corvette shimmers into view behind a
//      patrolling frigate, fires a railgun lance, and banks away.
static void render_battle_flyby(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_BATTLE_FLYBY, ticks, &window)) return;
  Uint32 phase = window.phase;
  int kind = effect_kind(&EFFECT_BATTLE_FLYBY, (window.seed >> 8) % 6);

  int size = scale / 170;
  if (size < 4) size = 4;
  int length = scale / 38;
  if (length < 14) length = 14;
  double fade = window.fade;
  double progress = window.progress;
  int center_y = surface->h * 11 / 100;
  Uint8 alpha = (Uint8)(150 * fade);
  Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);

  if (kind == 1) {
    int dir = (window.seed >> 3) & 1 ? 1 : -1;
    double span = surface->w + 4.0 * length + surface->w * 0.14;
    double lead_x =
        dir > 0 ? -length * 2.0 + progress * span : surface->w + length * 2.0 - progress * span;
    double chaser_x = lead_x - dir * surface->w * 0.14;
    double lead_y = center_y + sin(progress * 9.0) * scale * 0.022;
    double wing_y = center_y - sin(progress * 9.0 + 0.8) * scale * 0.024;
    double chaser_y = center_y + cos(progress * 5.0) * scale * 0.012;
    draw_ship_role(surface, (int)lead_x, (int)lead_y, SHIP_SMALL, window.seed >> 4, length, dir, 0,
                   opacity);
    draw_ship_role(surface, (int)(lead_x - dir * length * 1.4), (int)wing_y, SHIP_SMALL,
                   window.seed >> 9, (int)(length * 0.9), dir, 0, opacity);
    draw_ship_role(surface, (int)chaser_x, (int)chaser_y, SHIP_MID, window.seed >> 13,
                   (int)(length * 1.55), dir, 1, opacity);
    Uint32 volley = phase % 1800;
    if (volley < weapon_specs[WEAPON_PLASMA].life_ms && phase >= volley) {
      double fire_p = (double)(phase - volley) / window.duration;
      if (fire_p > 0.08 && fire_p < 0.88) {
        double f_lead_x =
            dir > 0 ? -length * 2.0 + fire_p * span : surface->w + length * 2.0 - fire_p * span;
        double f_chaser_x = f_lead_x - dir * surface->w * 0.14;
        double f_chaser_y = center_y + cos(fire_p * 5.0) * scale * 0.012;
        double hit_p =
            (double)(phase - volley + weapon_specs[WEAPON_PLASMA].impact_ms) / window.duration;
        double h_lead_x =
            dir > 0 ? -length * 2.0 + hit_p * span : surface->w + length * 2.0 - hit_p * span;
        double h_lead_y = center_y + sin(hit_p * 9.0) * scale * 0.022;
        draw_weapon_shot(surface, WEAPON_PLASMA, 0, f_chaser_x + dir * length * 0.75, f_chaser_y,
                         h_lead_x - dir * length * 0.3, h_lead_y, (double)volley, scale / 95.0, 1,
                         fade, 0, fx_hash(window.seed, (int)(phase / 1800)));
      }
    }
    return;
  }
  if (kind == 2) {
    int dir = (window.seed >> 4) & 1 ? 1 : -1;
    // Target cruiser drifting slowly on the far side
    double tx = (dir > 0) ? surface->w * 0.74 - progress * scale * 0.08
                          : surface->w * 0.26 + progress * scale * 0.08;
    double ty = center_y - scale * 0.015;
    draw_ship_role(surface, (int)tx, (int)ty, SHIP_MID, window.seed >> 5, (int)(length * 1.45),
                   -dir, 1, opacity);
    // Bomber swoops in, releases torpedo at progress ~0.30, then climbs away
    double bx = (dir > 0) ? -length * 2.0 + progress * (surface->w + 4.0 * length)
                          : surface->w + length * 2.0 - progress * (surface->w + 4.0 * length);
    double climb_t = smooth_step(0.42, 1.0, progress);
    double climb = climb_t * climb_t;
    double by = center_y + scale * 0.032 - climb * scale * 0.095;
    double pitch = -0.08 + climb_t * 0.58;
    draw_ship_role(surface, (int)bx, (int)by, SHIP_SMALL, window.seed >> 11, (int)(length * 1.1),
                   dir, 0, opacity, pitch);
    if (progress > 0.30 && progress < 0.82) {
      double torp_age = (progress - 0.30) * window.duration;
      if (torp_age < weapon_specs[WEAPON_TORPEDO].life_ms) {
        double launch_x = (dir > 0)
                              ? -length * 2.0 + 0.30 * (surface->w + 4.0 * length)
                              : surface->w + length * 2.0 - 0.30 * (surface->w + 4.0 * length);
        draw_weapon_shot(surface, WEAPON_TORPEDO, 0, launch_x, center_y + scale * 0.032, tx, ty,
                         torp_age, scale / 95.0, 0, fade, 1, fx_hash(window.seed, 77));
      }
    }
    return;
  }
  if (kind == 3) {
    int dir = (window.seed >> 3) & 1 ? 1 : -1;
    // Patrolling picket ship
    double px = (dir > 0) ? surface->w * 0.72 - progress * scale * 0.14
                          : surface->w * 0.28 + progress * scale * 0.14;
    double py = center_y + scale * 0.025;
    draw_ship_role(surface, (int)px, (int)py, SHIP_SMALL, window.seed >> 5, length, -dir, 1,
                   opacity);
    // Cruiser decelerates in (0.0..0.28), coasts (0.28..0.68), then streaks
    // away in warp off-screen (0.68..1.0).
    double warp_in = (progress < 0.28) ? 0.22 * (1.0 - pow(1.0 - progress / 0.28, 2.2)) : 0.22;
    double coast = 0.22 * progress;
    double warp_out = (progress > 0.68) ? 0.75 * pow((progress - 0.68) / 0.32, 2.5) : 0.0;
    double u = warp_in + coast + warp_out;
    double entry_x = (dir > 0) ? surface->w * 0.12 : surface->w * 0.88;
    double cx = entry_x + dir * u * surface->w * 0.95;
    double cy = center_y - scale * 0.015;
    if (progress < 0.22) {
      double flash = 1.0 - progress / 0.22;
      static const Uint8 warp_c[3] = {155, 225, 255};
      weapon_glow(surface, entry_x, cy, size * 2.5 * flash, warp_c, 190.0 * flash * fade);
    }
    if (progress > 0.68) {
      double streak = (progress - 0.68) / 0.32;
      thickLineRGBA(surface, (int)cx, (int)cy, (int)(cx - dir * streak * scale * 0.28), (int)cy, 2,
                    165, 225, 255, (Uint8)(160 * (1.0 - streak * 0.5) * fade));
    }
    draw_ship_role(surface, (int)cx, (int)cy, SHIP_MID, window.seed >> 9, (int)(length * 1.5), dir,
                   0, opacity);
    if (progress > 0.25 && progress < 0.70) {
      double age = (progress - 0.25) * window.duration;
      if (age < weapon_specs[WEAPON_BEAM].life_ms) {
        draw_weapon_shot(surface, WEAPON_BEAM, 0, cx, cy, px, py, age, scale / 95.0, 0, fade, 1,
                         fx_hash(window.seed, 51));
      }
    }
    return;
  }
  if (kind == 4) {
    int dir = (window.seed >> 5) & 1 ? 1 : -1;
    // Two blockade gunboats above and below that drift across and fade cleanly
    // with the window.
    double drift = (progress - 0.5) * scale * 0.06;
    for (int g = 0; g < 2; g++) {
      double gx = surface->w * (g == 0 ? 0.38 : 0.62) - dir * drift;
      double gy = center_y + (g == 0 ? -scale * 0.042 : scale * 0.042);
      draw_ship_role(surface, (int)gx, (int)gy, SHIP_MID, window.seed + g * 19,
                     (int)(length * 1.25), -dir, 1, opacity);
    }
    // Fast runner weaving between them
    double rx = (dir > 0) ? -length * 2.0 + progress * (surface->w + 4.0 * length)
                          : surface->w + length * 2.0 - progress * (surface->w + 4.0 * length);
    double ry = center_y + sin(progress * 10.0) * scale * 0.022;
    draw_ship_role(surface, (int)rx, (int)ry, SHIP_SMALL, window.seed >> 6, length, dir, 0,
                   opacity);
    Uint32 volley = phase % 1600;
    if (volley < weapon_specs[WEAPON_FLAK].life_ms && phase >= volley) {
      double fire_p = (double)(phase - volley) / window.duration;
      if (fire_p > 0.10 && fire_p < 0.85) {
        int g = ((int)(phase / 1600)) & 1;
        double fire_drift = (fire_p - 0.5) * scale * 0.06;
        double gx = surface->w * (g == 0 ? 0.38 : 0.62) - dir * fire_drift;
        double gy = center_y + (g == 0 ? -scale * 0.042 : scale * 0.042);
        double burst_p =
            (double)(phase - volley + weapon_specs[WEAPON_FLAK].impact_ms) / window.duration;
        double bx = (dir > 0) ? -length * 2.0 + burst_p * (surface->w + 4.0 * length)
                              : surface->w + length * 2.0 - burst_p * (surface->w + 4.0 * length);
        double by = center_y + sin(burst_p * 10.0) * scale * 0.022;
        draw_weapon_shot(surface, WEAPON_FLAK, 0, gx, gy, bx, by, (double)volley, scale / 95.0, 1,
                         fade, 0, fx_hash(window.seed, (int)(phase / 1600)));
      }
    }
    return;
  }
  if (kind == 5) {
    int dir = (window.seed >> 5) & 1 ? 1 : -1;
    double fx = (dir > 0) ? -length * 2.0 + progress * (surface->w + 4.0 * length)
                          : surface->w + length * 2.0 - progress * (surface->w + 4.0 * length);
    double fy = center_y - scale * 0.014;
    draw_ship_role(surface, (int)fx, (int)fy, SHIP_MID, window.seed >> 4, (int)(length * 1.4), dir,
                   1, opacity);
    // Decloaking attacker approaches from behind, fires, then banks upward
    // and flies off-screen while remaining visible.
    double sx = fx - dir * surface->w * (0.20 - 0.06 * smooth_step(0.18, 0.52, progress));
    double climb = smooth_step(0.54, 1.0, progress);
    double sy = fy + scale * 0.025 - climb * climb * scale * 0.09;
    double cloak = smooth_step(0.18, 0.34, progress);
    Uint8 s_op = (Uint8)(opacity * cloak);
    if (s_op > 4) {
      draw_ship_role(surface, (int)sx, (int)sy, SHIP_SMALL, window.seed >> 11, (int)(length * 1.15),
                     dir, 0, s_op, climb * 0.5);
    }
    if (progress > 0.36 && progress < 0.62) {
      double age = (progress - 0.36) * window.duration;
      if (age < weapon_specs[WEAPON_RAIL].life_ms) {
        double hit_p = 0.36 + weapon_specs[WEAPON_RAIL].impact_ms / window.duration;
        double hx = (dir > 0) ? -length * 2.0 + hit_p * (surface->w + 4.0 * length)
                              : surface->w + length * 2.0 - hit_p * (surface->w + 4.0 * length);
        draw_weapon_shot(surface, WEAPON_RAIL, 0, sx, sy, hx, fy, age, scale / 95.0, 0, fade, 1,
                         fx_hash(window.seed, 91));
      }
    }
    return;
  }

  int left_x = (int)(-length + progress * (surface->w + 2.0 * length));
  int right_x = surface->w - left_x;
  int arc = (int)(sin(progress * 6.28318530718) * scale * 0.018);
  int maneuver = (int)(sin(progress * 12.56637061436) * scale * 0.014);
  int left_y = center_y + arc + maneuver;
  int right_y = center_y - arc - maneuver;

  draw_ship_role(surface, left_x, left_y, SHIP_SMALL, window.seed >> 4, length, 1, 0, opacity);
  draw_ship_role(surface, right_x, right_y, SHIP_SMALL, window.seed >> 11, length, -1, 1, opacity);

  Uint32 volley_phase = phase % 2600;
  if (volley_phase < 300 && progress > 0.08 && progress < 0.94) {
    int bolt_length = scale / 18;
    if (bolt_length < 12) bolt_length = 12;
    Uint8 bolt_alpha = (Uint8)(alpha * 0.8);
    thickLineRGBA(surface, left_x + length / 2, left_y, left_x + length / 2 + bolt_length,
                  left_y - size / 3, 1, 143, 204, 224, bolt_alpha);
    thickLineRGBA(surface, right_x - length / 2, right_y, right_x - length / 2 - bolt_length,
                  right_y + size / 3, 1, 220, 164, 126, bolt_alpha);
  }
  if (volley_phase > 1850 && volley_phase < 2250) {
    double hit_progress = (double)(volley_phase - 1850) / 400.0;
    int impact_x = surface->w / 2 + (int)(sin(phase / 1700.0) * scale * 0.045);
    int impact_y = center_y + (int)(sin(phase / 950.0) * scale * 0.025);
    Uint8 impact_alpha = (Uint8)(alpha * (1.0 - hit_progress));
    int radius = size / 2 + (int)(size * hit_progress);
    filledCircleRGBA(surface, impact_x, impact_y, radius, 219, 158, 111,
                     (Uint8)(impact_alpha * 0.65));
    filledCircleRGBA(surface, impact_x, impact_y, size / 4, 238, 216, 177, impact_alpha);
  }
}


// Dogfight scenarios:
//   0: looping chase (1 pair, 2 passes).
//   1: scissors duel — two fighters weave from opposite edges in mirrored
//      sine-loops, exchanging fire on each crossover until one is downed.
//   2: strafing run — an interceptor dives repeatedly on a mid-class courier
//      crossing the upper sky while a defender tries to screen it.
//   3: vertical climb & stall — two fighters pull into steep twin climbing
//      arcs, stall near the top of the sky, and dive back down trading shots.
//   4: wingman rescue — a damaged fighter flees with a pursuer on its tail
//      until a friendly wingman dives in from above, downs the pursuer, and
//      escorts the survivor off-screen.
//   5: asteroid slalom — two fighters weave around four drifting sky-rocks,
//      exchanging pulse bolts between the boulders.
static void render_dogfight_effect(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_DOGFIGHT, ticks, &window)) return;
  int kind = effect_kind(&EFFECT_DOGFIGHT, (window.seed >> 6) % 6);
  if (kind == 1) {
    double progress = window.progress;
    double fade = window.fade;
    int length = scale / 34;
    if (length < 14) length = 14;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    double unit = weapon_unit(length);
    int winner = (window.seed >> 3) & 1;
    double destroy_p = 0.82;
    for (int side = 0; side < 2; side++) {
      if (side != winner && progress >= destroy_p) continue;
      double u = progress;
      double x = (side == 0) ? -length + u * (surface->w + 2.0 * length)
                             : surface->w + length - u * (surface->w + 2.0 * length);
      double y = surface->h * 0.11 + sin(u * 18.84955592 + side * 3.14159265) * scale * 0.042;
      double dx = (side == 0 ? 1.0 : -1.0) * (surface->w + 2.0 * length);
      double dy = cos(u * 18.84955592 + side * 3.14159265) * 18.84955592 * scale * 0.042;
      double heading = atan2(-dy, dx);
      int dir = dx >= 0 ? 1 : -1;
      double pitch =
          dir > 0 ? heading : atan2(sin(3.14159265359 - heading), cos(3.14159265359 - heading));
      draw_ship_role(surface, (int)x, (int)y, SHIP_SMALL, window.seed + side * 19, length, dir,
                     side, opacity, pitch);
      if (side != winner && progress > 0.52) {
        for (int puff = 1; puff <= 7; puff++) {
          double pu = u - puff * 0.012;
          if (pu < 0.0) break;
          double px = (side == 0) ? -length + pu * (surface->w + 2.0 * length)
                                  : surface->w + length - pu * (surface->w + 2.0 * length);
          double py = surface->h * 0.11 + sin(pu * 18.84955592 + side * 3.14159265) * scale * 0.042;
          filledCircleRGBA(surface, (int)px, (int)py, 1 + puff / 2, 150, 150, 160,
                           (Uint8)(70 * fade * (1.0 - puff / 8.0)));
        }
      }
    }
    // Exchange fire near crossovers
    Uint32 age = window.phase % 1500;
    int shot_idx = (int)(window.phase / 1500);
    int shooter = (shot_idx + winner) % 2;
    int weapon = (shooter == 0) ? WEAPON_PULSE : WEAPON_MASS_DRIVER;
    if (age < weapon_specs[weapon].life_ms && window.phase >= age) {
      double fire_u = (double)(window.phase - age) / window.duration;
      double hit_u =
          (double)(window.phase - age + weapon_specs[weapon].impact_ms) / window.duration;
      if (fire_u > 0.10 && hit_u < destroy_p) {
        int target = 1 - shooter;
        double sx = (shooter == 0) ? -length + fire_u * (surface->w + 2.0 * length)
                                   : surface->w + length - fire_u * (surface->w + 2.0 * length);
        double sy =
            surface->h * 0.11 + sin(fire_u * 18.84955592 + shooter * 3.14159265) * scale * 0.042;
        double tx = (target == 0) ? -length + hit_u * (surface->w + 2.0 * length)
                                  : surface->w + length - hit_u * (surface->w + 2.0 * length);
        double ty =
            surface->h * 0.11 + sin(hit_u * 18.84955592 + target * 3.14159265) * scale * 0.042;
        draw_weapon_shot(surface, weapon, 0, sx, sy, tx, ty, (double)age, unit, shooter, fade,
                         fire_u > 0.68 ? 1 : 0, fx_hash(window.seed, shot_idx));
      }
    }
    if (progress >= destroy_p) {
      double u = destroy_p;
      double lx = (winner != 0) ? -length + u * (surface->w + 2.0 * length)
                                : surface->w + length - u * (surface->w + 2.0 * length);
      double ly =
          surface->h * 0.11 + sin(u * 18.84955592 + (1 - winner) * 3.14159265) * scale * 0.042;
      draw_explosion(surface, lx, ly, (progress - destroy_p) * window.duration, length * 1.1, true,
                     fx_hash(window.seed, 99), 1 - winner, fade);
    }
    return;
  }
  if (kind == 2) {
    double progress = window.progress;
    double fade = window.fade;
    int sm_len = scale / 34;
    if (sm_len < 14) sm_len = 14;
    int mid_len = (int)(sm_len * 1.6);
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    int dir = (window.seed >> 5) & 1 ? 1 : -1;
    double cx = (dir > 0) ? -mid_len + progress * (surface->w + 2.0 * mid_len)
                          : surface->w + mid_len - progress * (surface->w + 2.0 * mid_len);
    double cy = surface->h * 0.13 + sin(progress * 5.0) * scale * 0.012;
    draw_ship_role(surface, (int)cx, (int)cy, SHIP_MID, window.seed + 7, mid_len, dir, 0, opacity);
    // Escort fighter weaving ahead/above the courier
    double ex = cx + dir * mid_len * 1.5;
    double ey = cy - scale * 0.032 + sin(progress * 14.0) * scale * 0.015;
    draw_ship_role(surface, (int)ex, (int)ey, SHIP_SMALL, window.seed + 13, sm_len, dir, 0,
                   opacity);
    // Diving interceptor making 3 continuous looping strafing passes
    double theta = progress * 3.0 * 2.0 * M_PI;
    double ix = cx + sin(theta) * scale * 0.24;
    double iy = cy - scale * 0.045 - cos(theta) * scale * 0.055;
    double dcx = dir * (surface->w + 2.0 * mid_len);
    double idx = dcx + cos(theta) * 6.0 * M_PI * scale * 0.24;
    double idy = sin(theta) * 6.0 * M_PI * scale * 0.055;
    double iheading = atan2(-idy, idx);
    int idir = idx >= 0 ? 1 : -1;
    double ipitch =
        idir > 0 ? iheading : atan2(sin(3.14159265359 - iheading), cos(3.14159265359 - iheading));
    draw_ship_role(surface, (int)ix, (int)iy, SHIP_SMALL, window.seed + 29, sm_len, idir, 1,
                   opacity, ipitch);
    double pass_u = fmod(progress * 3.0, 1.0);
    if (pass_u > 0.28 && pass_u < 0.70) {
      double age = (pass_u - 0.28) * (window.duration / 3.0);
      draw_weapon_shot(surface, WEAPON_RAIL, 0, ix, iy, cx, cy, age, weapon_unit(sm_len), 1, fade,
                       1, fx_hash(window.seed, (int)(progress * 3.0)));
    }
    return;
  }
  if (kind == 3) {
    double progress = window.progress;
    double fade = window.fade;
    int length = scale / 34;
    if (length < 14) length = 14;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    double unit = weapon_unit(length);
    for (int side = 0; side < 2; side++) {
      double lag = side * 0.06;
      double u = progress - lag;
      if (u < 0.0 || u > 1.0) continue;
      // Parabolic climb & stall arc across the center of the sky
      double x = surface->w * (0.18 + u * 0.64) + sin(u * 12.566 + side * 1.4) * scale * 0.02;
      double y = surface->h * 0.20 - sin(u * M_PI) * scale * 0.14 + side * scale * 0.015;
      double dx = surface->w * 0.64;
      double dy = -cos(u * M_PI) * M_PI * scale * 0.14;
      double pitch = atan2(-dy, dx);
      draw_ship_role(surface, (int)x, (int)y, SHIP_SMALL, window.seed + side * 31, length, 1, side,
                     opacity, pitch);
    }
    Uint32 age = window.phase % 1400;
    if (age < weapon_specs[WEAPON_PULSE].life_ms && window.phase >= age) {
      double fire_p = (double)(window.phase - age) / window.duration;
      double hit_p =
          (double)(window.phase - age + weapon_specs[WEAPON_PULSE].impact_ms) / window.duration;
      if (fire_p > 0.15 && hit_p < 0.90) {
        double u1 = fire_p - 0.06;
        double u0 = hit_p;
        double x1 = surface->w * (0.18 + u1 * 0.64) + sin(u1 * 12.566 + 1.4) * scale * 0.02;
        double y1 = surface->h * 0.20 - sin(u1 * M_PI) * scale * 0.14 + scale * 0.015;
        double x0 = surface->w * (0.18 + u0 * 0.64) + sin(u0 * 12.566) * scale * 0.02;
        double y0 = surface->h * 0.20 - sin(u0 * M_PI) * scale * 0.14;
        draw_weapon_shot(surface, WEAPON_PULSE, 0, x1, y1, x0, y0, (double)age, unit, 1, fade,
                         fire_p > 0.60 ? 1 : 0, fx_hash(window.seed, (int)(window.phase / 1400)));
      }
    }
    return;
  }
  if (kind == 4) {
    double progress = window.progress;
    double fade = window.fade;
    int length = scale / 34;
    if (length < 14) length = 14;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    double unit = weapon_unit(length);
    int dir = (window.seed >> 4) & 1 ? 1 : -1;
    // Fleeing damaged lead fighter (team 0)
    double lx = (dir > 0) ? -length + progress * (surface->w + 2.0 * length)
                          : surface->w + length - progress * (surface->w + 2.0 * length);
    double ly = surface->h * 0.13 + sin(progress * 7.0) * scale * 0.016;
    draw_ship_role(surface, (int)lx, (int)ly, SHIP_SMALL, window.seed + 11, length, dir, 0,
                   opacity);
    // Pursuing enemy (team 1), destroyed at progress 0.68
    double px = lx - dir * scale * 0.10;
    double py = ly + cos(progress * 9.0) * scale * 0.012;
    if (progress < 0.68) {
      draw_ship_role(surface, (int)px, (int)py, SHIP_SMALL, window.seed + 23, length, dir, 1,
                     opacity);
    } else {
      draw_explosion(surface, px, py, (progress - 0.68) * window.duration, length * 1.1, true,
                     fx_hash(window.seed, 88), 1, fade);
    }
    // Rescuing wingman (team 0) swoops down from above to intercept
    double wx = lx - dir * scale * (0.28 - 0.22 * smooth_step(0.15, 0.72, progress));
    double wy = ly - scale * 0.11 * (1.0 - smooth_step(0.20, 0.72, progress));
    draw_ship_role(surface, (int)wx, (int)wy, SHIP_SMALL, window.seed + 37, length, dir, 0, opacity,
                   progress < 0.70 ? -0.25 : 0.0);
    Uint32 volley = window.phase % 900;
    if (volley < weapon_specs[WEAPON_MASS_DRIVER].life_ms && window.phase >= volley) {
      double fire_p = (double)(window.phase - volley) / window.duration;
      double hit_p = (double)(window.phase - volley + weapon_specs[WEAPON_MASS_DRIVER].impact_ms) /
                     window.duration;
      if (fire_p > 0.44 && fire_p < 0.68) {
        double flx = (dir > 0) ? -length + fire_p * (surface->w + 2.0 * length)
                               : surface->w + length - fire_p * (surface->w + 2.0 * length);
        double fly = surface->h * 0.13 + sin(fire_p * 7.0) * scale * 0.016;
        double fwx = flx - dir * scale * (0.28 - 0.22 * smooth_step(0.15, 0.72, fire_p));
        double fwy = fly - scale * 0.11 * (1.0 - smooth_step(0.20, 0.72, fire_p));
        double hlx = (dir > 0) ? -length + hit_p * (surface->w + 2.0 * length)
                               : surface->w + length - hit_p * (surface->w + 2.0 * length);
        double hly = surface->h * 0.13 + sin(hit_p * 7.0) * scale * 0.016;
        double hpx = hlx - dir * scale * 0.10;
        double hpy = hly + cos(hit_p * 9.0) * scale * 0.012;
        draw_weapon_shot(surface, WEAPON_MASS_DRIVER, 0, fwx, fwy, hpx, hpy, (double)volley, unit,
                         0, fade, 1, fx_hash(window.seed, (int)(window.phase / 900)));
      }
    }
    return;
  }
  if (kind == 5) {
    double progress = window.progress;
    double fade = window.fade;
    int length = scale / 34;
    if (length < 14) length = 14;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    double unit = weapon_unit(length);
    int dir = (window.seed >> 4) & 1 ? 1 : -1;
    // Four drifting sky-rocks
    for (int r = 0; r < 4; r++) {
      double rx = surface->w * (0.22 + r * 0.19) + dir * (progress - 0.5) * scale * 0.04;
      double ry = surface->h * (r % 2 == 0 ? 0.10 : 0.16);
      int rad = (int)(scale * (0.012 + (r % 2) * 0.005));
      filledCircleRGBA(surface, (int)rx, (int)ry, rad, 34, 38, 48, weapon_alpha(200.0 * fade));
      circleRGBA(surface, (int)rx, (int)ry, rad, 72, 82, 98, weapon_alpha(150.0 * fade));
    }
    // Lead and pursuing fighters weaving around the boulders
    for (int s = 0; s < 2; s++) {
      double u = progress - s * 0.08;
      if (u < 0.0 || u > 1.0) continue;
      double sx = (dir > 0) ? -length + u * (surface->w + 2.0 * length)
                            : surface->w + length - u * (surface->w + 2.0 * length);
      double sy = surface->h * 0.13 + sin(u * 15.708 + s * 0.7) * scale * 0.036;
      draw_ship_role(surface, (int)sx, (int)sy, SHIP_SMALL, window.seed + s * 29, length, dir, s,
                     opacity);
    }
    Uint32 age = window.phase % 1300;
    if (age < weapon_specs[WEAPON_PULSE].life_ms && window.phase >= age) {
      double fp = (double)(window.phase - age) / window.duration;
      double hp =
          (double)(window.phase - age + weapon_specs[WEAPON_PULSE].impact_ms) / window.duration;
      if (fp > 0.14 && hp < 0.92) {
        double fu = fp - 0.08;
        double fsx = (dir > 0) ? -length + fu * (surface->w + 2.0 * length)
                               : surface->w + length - fu * (surface->w + 2.0 * length);
        double fsy = surface->h * 0.13 + sin(fu * 15.708 + 0.7) * scale * 0.036;
        double htx = (dir > 0) ? -length + hp * (surface->w + 2.0 * length)
                               : surface->w + length - hp * (surface->w + 2.0 * length);
        double hty = surface->h * 0.13 + sin(hp * 15.708) * scale * 0.036;
        draw_weapon_shot(surface, WEAPON_PULSE, 0, fsx, fsy, htx, hty, (double)age, unit, 1, fade,
                         fp > 0.65 ? 1 : 0, fx_hash(window.seed, (int)(window.phase / 1300)));
      }
    }
    return;
  }
  render_dogfight(surface, &window, scale, 1, 2, 0.0);
}


// Background battle scenarios:
//   0: furball — two or three pairs fighting at once.
//   1: convoy ambush — two mid-class ships cross the upper sky under fire from
//      darting interceptors launching swarm missiles and flak bursts.
//   2: orbital barrage — a pair of distant capital ships hold station on
//   opposite
//      flanks, trading heavy beam and railgun volleys across a fighter screen.
//   3: minefield gauntlet — a mid-class blockade runner weaves through drifting
//      proximity mines that detonate behind it while fighters give chase.
//   4: carrier launch — a heavy carrier holds station, launching a wing of
//      fighters that fan out to intercept incoming enemy bombers.
static void render_background_battle(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_BACKGROUND_BATTLE, ticks, &window)) return;
  int kind = effect_kind(&EFFECT_BACKGROUND_BATTLE, (window.seed >> 7) % 5);
  if (kind == 1) {
    double progress = window.progress;
    double fade = window.fade;
    int dir = (window.seed >> 4) & 1 ? 1 : -1;
    int mid_len = scale / 22;
    if (mid_len < 18) mid_len = 18;
    int sm_len = mid_len / 2;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    double cx = dir > 0 ? -mid_len * 2 + progress * (surface->w + 4.0 * mid_len)
                        : surface->w + mid_len * 2 - progress * (surface->w + 4.0 * mid_len);
    double cy = surface->h * 0.13;
    for (int c = 0; c < 2; c++) {
      double sx = cx - dir * c * mid_len * 1.9;
      double sy = cy + (c == 0 ? -scale * 0.015 : scale * 0.022);
      draw_ship_role(surface, (int)sx, (int)sy, SHIP_MID, window.seed + c * 17, mid_len, dir, 0,
                     opacity);
      // Raiders diving from above and below
      double rx = sx + sin(window.phase / 1300.0 + c * 2.1) * scale * 0.11;
      double ry =
          sy + (c == 0 ? -1 : 1) * scale * 0.055 * (0.6 + 0.4 * cos(window.phase / 900.0 + c));
      int rdir = cos(window.phase / 1300.0 + c * 2.1) >= 0 ? 1 : -1;
      draw_ship_role(surface, (int)rx, (int)ry, SHIP_SMALL, window.seed + 41 + c, sm_len, rdir, 1,
                     opacity);
      Uint32 age = window.phase % 2100;
      int weapon = (c == 0) ? WEAPON_SWARM : WEAPON_FLAK;
      if (age < weapon_specs[weapon].life_ms && window.phase >= age) {
        double fire_ph = (double)(window.phase - age);
        double fire_p = fire_ph / window.duration;
        double hit_p = (fire_ph + weapon_specs[weapon].impact_ms) / window.duration;
        if (fire_p > 0.06 && hit_p < 0.94) {
          double fcx = dir > 0 ? -mid_len * 2 + fire_p * (surface->w + 4.0 * mid_len)
                               : surface->w + mid_len * 2 - fire_p * (surface->w + 4.0 * mid_len);
          double fsx = fcx - dir * c * mid_len * 1.9;
          double frx = fsx + sin(fire_ph / 1300.0 + c * 2.1) * scale * 0.11;
          double fry =
              sy + (c == 0 ? -1 : 1) * scale * 0.055 * (0.6 + 0.4 * cos(fire_ph / 900.0 + c));
          double hcx = dir > 0 ? -mid_len * 2 + hit_p * (surface->w + 4.0 * mid_len)
                               : surface->w + mid_len * 2 - hit_p * (surface->w + 4.0 * mid_len);
          double hsx = hcx - dir * c * mid_len * 1.9;
          draw_weapon_shot(surface, weapon, 0, frx, fry, hsx, sy, (double)age, scale / 90.0, 1,
                           fade, 1, fx_hash(window.seed, (int)(window.phase / 2100) + c));
        }
      }
    }
    return;
  }
  if (kind == 2) {
    double progress = window.progress;
    double fade = window.fade;
    int big_len = scale / 14;
    if (big_len < 24) big_len = 24;
    int sm_len = scale / 38;
    if (sm_len < 12) sm_len = 12;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    double lx = surface->w * 0.20 + sin(progress * 3.14159265) * scale * 0.03;
    double rx = surface->w * 0.80 - sin(progress * 3.14159265) * scale * 0.03;
    double ly = surface->h * 0.11 + cos(window.phase / 1900.0) * scale * 0.01;
    double ry = surface->h * 0.13 + sin(window.phase / 2100.0) * scale * 0.01;
    draw_ship_role(surface, (int)lx, (int)ly, SHIP_BIG, window.seed + 3, big_len, 1, 0, opacity);
    draw_ship_role(surface, (int)rx, (int)ry, SHIP_BIG, window.seed + 11, big_len, -1, 1, opacity);
    // Fighter screen weaving in the middle
    for (int f = 0; f < 3; f++) {
      double fx =
          surface->w * 0.50 + sin(window.phase / (1100.0 + f * 230) + f * 2.1) * scale * 0.15;
      double fy =
          surface->h * 0.12 + cos(window.phase / (850.0 + f * 170) + f * 1.4) * scale * 0.035;
      int fdir = cos(window.phase / (1100.0 + f * 230) + f * 2.1) >= 0 ? 1 : -1;
      draw_ship_role(surface, (int)fx, (int)fy, SHIP_SMALL, window.seed + 50 + f, sm_len, fdir,
                     f % 2, opacity);
    }
    Uint32 b_age = window.phase % 2600;
    if (b_age < 1100 && progress > 0.08 && progress < 0.92) {
      int side = ((int)(window.phase / 2600)) & 1;
      double sx = side == 0 ? lx + big_len * 0.45 : rx - big_len * 0.45;
      double sy = side == 0 ? ly : ry;
      double tx = side == 0 ? rx : lx;
      double ty = side == 0 ? ry : ly;
      draw_weapon_shot(surface, side == 0 ? WEAPON_BEAM : WEAPON_RAIL, 0, sx, sy, tx, ty,
                       (double)b_age, weapon_unit(big_len), side, fade, 1,
                       fx_hash(window.seed, (int)(window.phase / 2600)));
    }
    return;
  }
  if (kind == 3) {
    double progress = window.progress;
    double fade = window.fade;
    int dir = (window.seed >> 4) & 1 ? 1 : -1;
    int mid_len = scale / 22;
    if (mid_len < 18) mid_len = 18;
    int sm_len = mid_len / 2;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    double rx = (dir > 0) ? -mid_len + progress * (surface->w + 2.0 * mid_len)
                          : surface->w + mid_len - progress * (surface->w + 2.0 * mid_len);
    double ry = surface->h * 0.12 + sin(progress * 11.0) * scale * 0.025;
    draw_ship_role(surface, (int)rx, (int)ry, SHIP_MID, window.seed + 5, mid_len, dir, 0, opacity);
    // Pursuing fighters
    for (int f = 0; f < 2; f++) {
      double fx = rx - dir * (mid_len * 2.2 + f * mid_len * 1.1);
      double fy = ry + (f == 0 ? -scale * 0.02 : scale * 0.022);
      draw_ship_role(surface, (int)fx, (int)fy, SHIP_SMALL, window.seed + 23 + f, sm_len, dir, 1,
                     opacity);
    }
    // Proximity mines across the corridor that detonate as the runner passes
    static const Uint8 mine_c[3] = {245, 110, 85};
    for (int m = 0; m < 7; m++) {
      double mp = 0.14 + m * 0.11;
      double mx = (dir > 0) ? -mid_len + mp * (surface->w + 2.0 * mid_len)
                            : surface->w + mid_len - mp * (surface->w + 2.0 * mid_len);
      double my = surface->h * 0.12 + sin(mp * 11.0 + (m % 2 ? 1.5 : -1.5)) * scale * 0.028;
      if (progress < mp + 0.04) {
        double blink = 0.4 + 0.6 * fabs(sin(window.phase / 220.0 + m));
        weapon_glow(surface, mx, my, 1.3, mine_c, 165.0 * blink * fade);
      } else {
        double age = (progress - (mp + 0.04)) * window.duration;
        if (age < 1200.0) {
          draw_explosion(surface, mx, my, age, sm_len * 0.9, false, fx_hash(window.seed, 90 + m), 1,
                         fade);
        }
      }
    }
    return;
  }
  if (kind == 4) {
    double progress = window.progress;
    double fade = window.fade;
    int dir = (window.seed >> 4) & 1 ? 1 : -1;
    int big_len = scale / 14;
    if (big_len < 24) big_len = 24;
    int sm_len = scale / 38;
    if (sm_len < 12) sm_len = 12;
    Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);
    double entry = smooth_step(0.0, 0.20, progress);
    double cx = (dir > 0) ? -big_len + entry * (surface->w * 0.24 + big_len)
                          : surface->w + big_len - entry * (surface->w * 0.24 + big_len);
    double cy = surface->h * 0.12 + sin(window.phase / 2000.0) * scale * 0.008;
    draw_ship_role(surface, (int)cx, (int)cy, SHIP_BIG, window.seed + 11, big_len, dir, 0, opacity);
    // 2 incoming enemy mid-ships on the opposite flank
    double ex_arr[2], ey_arr[2];
    for (int e = 0; e < 2; e++) {
      ex_arr[e] = (dir > 0) ? surface->w + sm_len * 2 - progress * (surface->w * 0.42 + sm_len * 2)
                            : -sm_len * 2 + progress * (surface->w * 0.42 + sm_len * 2);
      ey_arr[e] = surface->h * (0.10 + e * 0.045);
      draw_ship_role(surface, (int)ex_arr[e], (int)ey_arr[e], SHIP_MID, window.seed + 61 + e,
                     (int)(sm_len * 1.6), -dir, 1, opacity);
      Uint32 age = window.phase % 2200;
      if (age < weapon_specs[WEAPON_MASS_DRIVER].life_ms && progress > 0.20 && progress < 0.88) {
        draw_weapon_shot(surface, WEAPON_MASS_DRIVER, 0, ex_arr[e], ey_arr[e], cx, cy, (double)age,
                         weapon_unit(sm_len), 1, fade, 0,
                         fx_hash(window.seed, (int)(window.phase / 2200) + e));
      }
    }
    // 3 launched fighters fanning out from the carrier to intercept
    for (int f = 0; f < 3; f++) {
      double fu = progress - (0.14 + f * 0.10);
      if (fu < 0.0) continue;
      double fx = cx + dir * fu * surface->w * 0.58;
      double fy = cy + (f - 1) * scale * 0.032 * smooth_step(0.0, 0.3, fu);
      draw_ship_role(surface, (int)fx, (int)fy, SHIP_SMALL, window.seed + 31 + f, sm_len, dir, 0,
                     opacity);
      Uint32 fage = window.phase % 1400;
      if (fage < weapon_specs[WEAPON_PULSE].life_ms && fu > 0.12 && progress < 0.88) {
        int tgt = f % 2;
        draw_weapon_shot(surface, WEAPON_PULSE, 0, fx, fy, ex_arr[tgt], ey_arr[tgt], (double)fage,
                         weapon_unit(sm_len), 0, fade, 1,
                         fx_hash(window.seed, (int)(window.phase / 1400) + f));
      }
    }
    return;
  }
  render_dogfight(surface, &window, scale, 2 + (window.seed >> 5) % 2, 3, 0.35);
}


/*
 * Fleet battle
 *
 * Two fleets close, hold and trade fire. Like every effect it is stateless: a
 * ship's place is a function of time, each gun fires on a schedule of its own,
 * and a shot is drawn from how long ago it was fired. The losing fleet loses
 * its ships one at a time in the second half of the battle (the last one
 * survives and retreats); the winners' guns favour a ship that is about to die,
 * so every destruction is preceded by hits.
 */
typedef struct {
  RenderTarget *surface;
  int scale;
  Uint32 seed;
  int kind;
  double start;  // scene time the battle began
  double duration;
  int winner;  // the side that does not lose ships
  int size, fighter_size;
  int count[2];
  int center_y;
} Fleet;

// Time between volleys of each weapon (ms); turrets fire half as often again.
static const double weapon_period[WEAPON_COUNT] = {1300.0, 3400.0, 2800.0, 1700.0, 2100.0,
                                                   3800.0, 3400.0, 6500.0, 4200.0};

static double fleet_progress(const Fleet *fleet, double t) {
  return (t - fleet->start) / fleet->duration;
}


static void fleet_pose(const Fleet *fleet, int side, int ship, double t, double *x, double *y) {
  double progress = fleet_progress(fleet, t);
  double approach = smooth_step(0.04, 0.31, progress);
  double retreat = smooth_step(0.78, 0.97, progress);
  int width = fleet->surface->w;
  double anchor;
  if (side == 0) {
    double hold = width * (0.37 + 0.008 * sin(t / 3600.0));
    anchor = -fleet->size * 6 + approach * (hold + fleet->size * 6) -
             (fleet->winner == 0 ? 0.0 : retreat * width * 0.62);
  } else {
    double hold = width * (0.63 + 0.008 * sin(t / 3900.0 + 1.7));
    anchor = width + fleet->size * 6 - approach * (width - hold + fleet->size * 6) +
             (fleet->winner == 1 ? 0.0 : retreat * width * 0.62);
  }
  int direction = side == 0 ? 1 : -1;
  int lane = ship == 0 ? 0 : (ship % 2 ? (ship + 1) / 2 : -(ship / 2));
  double offset = direction * ((ship + 1) / 2) * fleet->size * 3;
  if (fleet->kind == 1) {
    // Crescent encirclement: winner forms a wide enveloping arc while the
    // outnumbered/trapped side clusters tighter toward the center.
    double spread = (side == fleet->winner) ? 1.55 : 0.72;
    offset = (side == fleet->winner)
                 ? direction * (fabs((double)lane) * fleet->size * 1.9 - fleet->size * 2.2)
                 : direction * ((ship + 1) / 2) * fleet->size * 2.1;
    double weave = sin(t / (510.0 + ship * 127) + ship * 1.8 + side * 2.4) * fleet->scale * 0.008;
    *x = anchor + offset + weave;
    *y = fleet->center_y + lane * (fleet->scale / 34.0) * spread +
         sin(t / (420.0 + ship * 83) + ship * 1.3) * fleet->size * 0.65;
    return;
  }
  if (fleet->kind == 2) {
    // Parallel broadside: both lines steam across the sky side-by-side at
    // different altitudes, trading broadsides before the loser peels off.
    double sweep = -width * 0.08 + progress * (width * 1.32);
    double sep = fleet->scale * (0.048 + 0.014 * sin(progress * 6.2831853));
    double line_y = fleet->center_y + (side == 0 ? -sep : sep);
    double retreat_y = (side != fleet->winner) ? retreat * fleet->scale * 0.14 : 0.0;
    *x = sweep - ship * fleet->size * 3.4 +
         sin(t / (600.0 + ship * 90) + side * 2.0) * fleet->size * 0.8;
    *y = line_y - retreat_y + sin(t / (480.0 + ship * 110) + ship * 1.5) * fleet->size * 0.55;
    return;
  }
  if (fleet->kind == 3) {
    // Dreadnought siege: one side anchors around a central flagship while
    // the attackers orbit in a wider flanking column.
    if (side == fleet->winner) {
      *x = anchor + (ship == 0 ? 0.0 : -direction * fleet->size * 2.6 * ((ship + 1) / 2)) +
           sin(t / (700.0 + ship * 80)) * fleet->size * 0.4;
      *y = fleet->center_y + lane * fleet->scale / 30.0;
    } else {
      double ang = progress * 5.2 + ship * 0.55;
      double rad_x = fleet->scale * 0.24;
      double rad_y = fleet->scale * 0.068;
      double edge = (side == 0) ? -fleet->size * 6.0 : width + fleet->size * 6.0;
      double cx = edge + approach * (width * 0.50 - edge) +
                  (side == 0 ? -retreat * width * 0.62 : retreat * width * 0.62);
      *x = cx + cos(ang) * rad_x;
      *y = fleet->center_y + sin(ang) * rad_y;
    }
    return;
  }
  if (fleet->kind == 4) {
    // Pincer crossfire: the winning fleet splits into two flanking wings
    // (left and right) closing around a retreating center column.
    if (side == fleet->winner) {
      int wing = ship % 2;
      double start_x = wing == 0 ? -fleet->size * 6.0 : width + fleet->size * 6.0;
      double target_x = wing == 0 ? width * 0.30 : width * 0.70;
      double wx = start_x + approach * (target_x - start_x);
      double wy = fleet->center_y + (wing == 0 ? -1 : 1) * fleet->scale * 0.042;
      *x = wx + (ship / 2) * (wing == 0 ? -1 : 1) * fleet->size * 2.4 +
           sin(t / (540.0 + ship * 95)) * fleet->size * 0.5;
      *y = wy + cos(t / (480.0 + ship * 70)) * fleet->size * 0.5;
    } else {
      double start_x = (side == 0) ? -fleet->size * 6.0 : width + fleet->size * 6.0;
      double cx = start_x + approach * (width * 0.50 - start_x);
      double cy = fleet->center_y - retreat * fleet->scale * 0.22;
      *x = cx + lane * fleet->size * 2.2 + sin(t / (450.0 + ship * 80)) * fleet->size * 0.6;
      *y = cy + (ship - fleet->count[side] / 2.0) * fleet->size * 1.4;
    }
    return;
  }
  if (fleet->kind == 5) {
    // V-wedge charge: the winning fleet forms a sharp spearhead wedge that
    // drives straight through a staggered defensive screen.
    if (side == fleet->winner) {
      double vx = -fleet->size * 6 + approach * (width * 0.45 + fleet->size * 6) +
                  smooth_step(0.48, 0.98, progress) * (width * 0.75);
      int rank = (ship + 1) / 2;
      *x = (side == 0 ? vx - rank * fleet->size * 2.8 : (width - vx) + rank * fleet->size * 2.8);
      *y = fleet->center_y + lane * fleet->size * 1.9 +
           sin(t / (520.0 + ship * 90)) * fleet->size * 0.4;
    } else {
      *x = anchor + (ship % 2 ? -1 : 1) * fleet->size * 2.2;
      *y = fleet->center_y + lane * fleet->scale / 30.0 +
           sin(t / (440.0 + ship * 75)) * fleet->size * 0.5;
    }
    return;
  }
  double weave = sin(t / (510.0 + ship * 127) + ship * 1.8 + side * 2.4) * fleet->scale * 0.009;
  double arc = sin(t / 1900.0 + progress * 8.0) * fleet->scale * 0.010;
  *x = anchor + offset + weave;
  *y = fleet->center_y + (side == 0 ? arc : -arc) + lane * fleet->scale / 36.0 +
       sin(t / (370.0 + ship * 83) + ship * 1.3) * fleet->size * 0.75;
}


static int fleet_role(const Fleet *fleet, int side, int ship) {
  if (fleet->kind == 3) {
    if (side == fleet->winner && ship == 0) return SHIP_BIG;
    return (ship % 2 == 0) ? SHIP_MID : SHIP_SMALL;
  }
  unsigned int type = fleet->seed >> ((ship * 3 + side * 7) % 24);
  if (ship == 0 && ((fleet->seed >> (side + 2)) & 1)) return SHIP_BIG;
  if (type % 3 == 0) return SHIP_MID;
  return SHIP_SMALL;
}


static int fleet_length(const Fleet *fleet, int ship, int role) {
  if (role == SHIP_BIG) return fleet->size * 10;
  if (role == SHIP_MID) return fleet->size * 7;
  return (fleet->fighter_size + (ship % 3 == 0 ? 2 : 0)) * 5;
}


static int fleet_design(const Fleet *fleet, int side, int ship) {
  unsigned int variety = (fleet->seed >> 9) + ship * 5 + side * 3;
  return ship_design_index(fleet_role(fleet, side, ship), variety);
}


// Scene time at which a ship of the losing fleet is destroyed; -1 for any other
// ship.
static double fleet_death(const Fleet *fleet, int side, int ship) {
  if (side == fleet->winner || ship == fleet->count[side] - 1) return -1.0;
  double when = 0.46 + 0.34 * (fx_hash(fleet->seed, side * 16 + ship) % 100) / 100.0;
  return fleet->start + when * fleet->duration;
}


static int fleet_direction(const Fleet *fleet, int side, int ship, double t) {
  if (fleet->kind == 2) return 1;
  if (fleet->kind == 3 && side != fleet->winner) {
    double x, y, nx, ny;
    fleet_pose(fleet, side, ship, t, &x, &y);
    fleet_pose(fleet, side, ship, t + 60.0, &nx, &ny);
    return (nx >= x) ? 1 : -1;
  }
  if (fleet->kind == 4 && side == fleet->winner) {
    return (ship % 2 == 0) ? 1 : -1;
  }
  return side == 0 ? 1 : -1;
}


// Draws the shots in the air now from every gun of one ship.
static void fleet_fire(const Fleet *fleet, int side, int ship, double now, double fade) {
  int design_index = fleet_design(fleet, side, ship);
  const ShipDesign *design = &ship_designs[design_index];
  int length = fleet_length(fleet, ship, fleet_role(fleet, side, ship));
  double unit = weapon_unit(length);
  double death = fleet_death(fleet, side, ship);
  int direction = fleet_direction(fleet, side, ship, now);
  int enemy = 1 - side;
  for (int gun = 0; gun < 1 + design->turret_count; gun++) {
    int weapon = gun == 0 ? design->weapon : design->turret_weapon;
    const WeaponSpec *spec = &weapon_specs[weapon];
    unsigned int gun_hash = fx_hash(fleet->seed, 1000 + side * 200 + ship * 20 + gun);
    double period =
        weapon_period[weapon] * (gun == 0 ? 1.0 : 1.5) * (0.85 + 0.3 * (gun_hash % 100) / 100.0);
    double offset = (gun_hash >> 8) % (unsigned int)period;
    double volley = (spec->shots - 1) * spec->gap_ms;
    double since = now - fleet->start - offset;
    if (since < 0.0) continue;
    int newest = (int)(since / period);
    double earliest = since - spec->life_ms - volley;
    int oldest = earliest < 0.0 ? 0 : (int)(earliest / period);
    int mount_x = gun == 0 ? design->nose : design->turrets[gun - 1][0];
    int mount_y = gun == 0 ? 0 : design->turrets[gun - 1][1];
    for (int k = oldest; k <= newest; k++) {
      double volley_start = fleet->start + offset + k * period;
      double volley_progress = fleet_progress(fleet, volley_start);
      if (volley_progress < 0.14 || volley_progress > 0.9) continue;
      if (death >= 0.0 && volley_start >= death) continue;
      unsigned int volley_hash = fx_hash(gun_hash, k);
      double lands = volley_start + spec->impact_ms;
      // Prefer a ship that is about to be destroyed.
      int target = -1;
      bool finishing = false;
      for (int j = 0; j < fleet->count[enemy]; j++) {
        double d = fleet_death(fleet, enemy, j);
        if (d > lands && d < lands + 1500.0 && volley_hash % 100 < 70) {
          target = j;
          finishing = true;
          break;
        }
      }
      if (target < 0) {
        int first = (volley_hash >> 8) % fleet->count[enemy];
        for (int tries = 0; tries < fleet->count[enemy]; tries++) {
          int j = (first + tries) % fleet->count[enemy];
          double d = fleet_death(fleet, enemy, j);
          if (d < 0.0 || d > lands) {
            target = j;
            break;
          }
        }
      }
      if (target < 0) continue;
      bool hit = finishing || (volley_hash >> 16) % 100 < 40;
      int target_length = fleet_length(fleet, target, fleet_role(fleet, enemy, target));
      for (int bolt = 0; bolt < spec->shots; bolt++) {
        double fired = volley_start + bolt * spec->gap_ms;
        double age = now - fired;
        if (age < 0.0 || age >= spec->life_ms) continue;
        // A beam stays on its target, so it follows both ships live.
        bool live = weapon == WEAPON_BEAM;
        double sx, sy, tx, ty, mx, my;
        fleet_pose(fleet, side, ship, live ? now : fired, &sx, &sy);
        fleet_pose(fleet, enemy, target, live ? now : fired + spec->impact_ms, &tx, &ty);
        ship_mount_point(design_index, (int)sx, (int)sy, length, direction, 0.0, mount_x, mount_y,
                         &mx, &my);
        double dx = tx - mx, dy = ty - my;
        double span = sqrt(dx * dx + dy * dy) + 1.0;
        if (hit) {
          // Hits spread over the hull.
          ty += ((bolt + k) % 3 - 1) * target_length * 0.12;
        } else {
          double side_offset =
              ((volley_hash >> 4) % 2 ? 1.0 : -1.0) * target_length * (0.5 + 0.25 * (bolt % 3));
          tx += -dy / span * side_offset;
          ty += dx / span * side_offset;
        }
        draw_weapon_shot(fleet->surface, weapon, bolt, mx, my, tx, ty, age, unit, side, fade,
                         hit ? 1 : 0, fx_hash(volley_hash, bolt));
      }
    }
  }
}


static void render_fleet_battle(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_FLEET_BATTLE, ticks, &window)) return;
  Fleet fleet;
  fleet.surface = surface;
  fleet.scale = scale;
  fleet.seed = window.seed;
  fleet.kind = effect_kind(&EFFECT_FLEET_BATTLE, (window.seed >> 7) % 6);
  fleet.start = (double)ticks - window.phase;
  fleet.duration = window.duration;
  fleet.winner = (window.seed ^ (window.seed >> 13) ^ (window.seed >> 23)) & 1;
  fleet.size = scale / 72;
  if (fleet.size < 10) fleet.size = 10;
  fleet.fighter_size = fleet.size / 2;
  if (fleet.fighter_size < 7) fleet.fighter_size = 7;
  fleet.count[0] = 3 + (window.seed >> 4) % 4;
  fleet.count[1] = 3 + (window.seed >> 12) % 4;
  fleet.center_y = surface->h * 12 / 100;
  Uint32 seed = window.seed;
  double progress = window.progress;
  double fade = window.fade;
  double now = ticks;
  Uint8 opacity = (Uint8)(SHIP_OPACITY * fade);

  // The ships. One that is about to be destroyed burns and trails smoke.
  for (int side = 0; side < 2; side++) {
    for (int ship = 0; ship < fleet.count[side]; ship++) {
      double death = fleet_death(&fleet, side, ship);
      if (death >= 0.0 && now >= death) continue;
      double x, y;
      fleet_pose(&fleet, side, ship, now, &x, &y);
      int direction = fleet_direction(&fleet, side, ship, now);
      int role = fleet_role(&fleet, side, ship);
      int length = fleet_length(&fleet, ship, role);
      if (death >= 0.0 && now > death - 3500.0) {
        for (int puff = 1; puff <= 9; puff++) {
          double px, py;
          fleet_pose(&fleet, side, ship, now - puff * 110.0, &px, &py);
          filledCircleRGBA(surface, (int)(px - direction * length * 0.3), (int)py,
                           2 + puff / 2 + length / 30, 150, 150, 160,
                           weapon_alpha(85.0 * fade * (1.0 - puff / 10.0)));
        }
      }
      draw_ship_design(surface, (int)floor(x + 0.5), (int)floor(y + 0.5),
                       fleet_design(&fleet, side, ship), length, direction, side, opacity);
      if (death >= 0.0 && now > death - 3500.0) {
        weapon_glow(surface, x - direction * length * 0.15, y - length * 0.04, length * 0.09,
                    weapon_team_colors[1], fade * (110.0 + 90.0 * sin(now / 55.0 + ship)));
      }
    }
  }

  double reinforcement =
      smooth_step(0.36, 0.45, progress) * (1.0 - smooth_step(0.70, 0.79, progress));
  if (reinforcement > 0.0) {
    int reserves = 1 + (seed >> 20) % 3;
    for (int side = 0; side < 2; side++) {
      int direction = side == 0 ? 1 : -1;
      double anchor_x, anchor_y;
      fleet_pose(&fleet, side, 0, now, &anchor_x, &anchor_y);
      for (int ship = 0; ship < reserves; ship++) {
        int x = (int)anchor_x - direction * (int)(surface->w * 0.16 * (1.0 - reinforcement)) -
                direction * ship * fleet.fighter_size * 3;
        int y = (int)anchor_y + (ship - reserves / 2) * scale / 25;
        draw_ship_role(surface, x, y, SHIP_SMALL, (seed >> 13) + ship * 5 + side * 3,
                       fleet.fighter_size * 5, direction, side,
                       (Uint8)(SHIP_OPACITY * 0.85 * fade * reinforcement));
      }
    }
  }

  // Weapons fire, and the doomed ships blow up.
  for (int side = 0; side < 2; side++) {
    for (int ship = 0; ship < fleet.count[side]; ship++) {
      fleet_fire(&fleet, side, ship, now, fade);
    }
  }
  for (int side = 0; side < 2; side++) {
    for (int ship = 0; ship < fleet.count[side]; ship++) {
      double death = fleet_death(&fleet, side, ship);
      if (death < 0.0 || now < death || now - death >= 1700.0) continue;
      double x, y;
      fleet_pose(&fleet, side, ship, death, &x, &y);
      int length = fleet_length(&fleet, ship, fleet_role(&fleet, side, ship));
      draw_explosion(surface, x, y, now - death, length * 0.55 + 6.0, true,
                     fx_hash(seed, 300 + side * 16 + ship), side, fade);
    }
  }
}


// Screen row of ridge `layer` at pixel column x.
static double ground_y(RenderTarget *surface, int layer, double x) {
  double nx = x / surface->w;
  if (nx < 0.0) nx = 0.0;
  if (nx > 0.999) nx = 0.999;
  return surface->h * ridge_height(layer, nx);
}
