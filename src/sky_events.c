/*
 * Rare sky events
 *
 * Every rare sky event is a small function taking a SkyEvent: an object that
 * crosses the sky (x, y) in `direction`, `size` pixels big, with `alpha`
 * already faded for fade-in/out. progress/phase say how far along it is.
 * To add one: write sky_event_NN(), append it to sky_event_kinds[] below and
 * bump RARE_SKY_KINDS.
 */
typedef struct {
  RenderTarget *surface;
  Uint32 ticks;
  int scale;  // shorter screen side
  Uint32 seed;
  Uint32 phase;     // ms since the event started
  double progress;  // 0..1 through the event
  double fade;      // 0..1 fade in/out
  int direction;    // 1: moves left to right, -1: right to left
  int x, y;         // current position of the object
  int size;
  Uint8 alpha;  // 150 * fade
} SkyEvent;

#define SKY_EVENT_UNPACK(e)             \
  RenderTarget *surface = (e)->surface; \
  Uint32 ticks = (e)->ticks;            \
  int scale = (e)->scale;               \
  Uint32 seed = (e)->seed;              \
  Uint32 phase = (e)->phase;            \
  double progress = (e)->progress;      \
  double fade = (e)->fade;              \
  int direction = (e)->direction;       \
  int x = (e)->x, y = (e)->y;           \
  int center_x = x, center_y = y;       \
  int size = (e)->size;                 \
  Uint8 alpha = (e)->alpha;             \
  (void)surface;                        \
  (void)ticks;                          \
  (void)scale;                          \
  (void)seed;                           \
  (void)phase;                          \
  (void)progress;                       \
  (void)fade;                           \
  (void)direction;                      \
  (void)x;                              \
  (void)y;                              \
  (void)center_x;                       \
  (void)center_y;                       \
  (void)size;                           \
  (void)alpha

static void sky_event_23(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int ground_x = center_x < 0 ? 0 : center_x;
  if (ground_x >= surface->w) ground_x = surface->w - 1;
  double nx = (double)ground_x / surface->w;
  double ridge = ridge_height(RIDGE_NEAR, nx);
  int ground_y = (int)(surface->h * ridge);
  Uint8 pulse = (Uint8)(alpha * (0.45 + 0.55 * (0.5 + 0.5 * sin(progress * 18.0))));
  boxRGBA(surface, center_x - size, ground_y - size / 2, center_x + size, ground_y + size / 2, 35,
          38, 42, alpha);
  filledCircleRGBA(surface, center_x - size / 2, ground_y - size / 2, 2, 83, 123, 77, alpha);
  filledCircleRGBA(surface, center_x - size, ground_y - size, size / 2, 83, 56, 55, pulse);
  filledCircleRGBA(surface, center_x + size, ground_y - size, size / 2, 54, 75, 120, pulse);
}


static void sky_event_24(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int fold_size = size * 3;
  int unfold = (int)(fold_size * (0.35 + 0.65 * sin(progress * 3.14159265359)));
  filledTrigonRGBA(surface, center_x, center_y - fold_size, center_x - unfold,
                   center_y + fold_size / 2, center_x, center_y, 167, 193, 205, alpha);
  filledTrigonRGBA(surface, center_x, center_y - fold_size, center_x, center_y, center_x + unfold,
                   center_y + fold_size / 2, 211, 177, 137, alpha);
  filledTrigonRGBA(surface, center_x, center_y - fold_size, center_x - fold_size / 3,
                   center_y + fold_size, center_x + fold_size / 3, center_y + fold_size, 120, 151,
                   169, alpha);
  thickLineRGBA(surface, center_x, center_y - fold_size, center_x, center_y + fold_size, 1, 244,
                227, 198, alpha);
  for (int crease = 1; crease <= 4; crease++) {
    int crease_x = center_x + (crease % 2 ? unfold : -unfold) * crease / 4;
    thickLineRGBA(surface, center_x, center_y - fold_size + crease * fold_size / 2, crease_x,
                  center_y + fold_size / 2, 1, 235, 220, 193, alpha);
  }
}


static void sky_event_25(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int wheel_radius = scale / 180;
  if (wheel_radius < 3) wheel_radius = 3;
  int wheelbase = scale / 18;
  if (wheelbase < 34) wheelbase = 34;
  int rear_x = direction > 0
                   ? -wheelbase + (int)(progress * (surface->w + 2.0 * wheelbase))
                   : surface->w + wheelbase - (int)(progress * (surface->w + 2.0 * wheelbase));
  int front_x = rear_x + direction * wheelbase;
  int ridge_x = rear_x + direction * wheelbase / 2;
  if (ridge_x < 0) ridge_x = 0;
  if (ridge_x >= surface->w) ridge_x = surface->w - 1;
  double nx = (double)ridge_x / surface->w;
  double ridge = ridge_height(RIDGE_NEAR, nx);
  int wheel_y = (int)(surface->h * ridge) - wheel_radius;
  int seat_x = rear_x + direction * wheelbase * 38 / 100;
  int crank_x = rear_x + direction * wheelbase * 64 / 100;
  int seat_y = wheel_y - wheel_radius * 3;
  circleRGBA(surface, rear_x, wheel_y, wheel_radius, 26, 30, 36, alpha);
  circleRGBA(surface, front_x, wheel_y, wheel_radius, 26, 30, 36, alpha);
  for (int spoke = 0; spoke < 6; spoke++) {
    double spoke_angle = progress * 6.28318530718 + spoke * 1.0471975512;
    int spoke_dx = (int)(cos(spoke_angle) * wheel_radius);
    int spoke_dy = (int)(sin(spoke_angle) * wheel_radius);
    thickLineRGBA(surface, rear_x, wheel_y, rear_x + spoke_dx, wheel_y + spoke_dy, 1, 119, 130, 137,
                  alpha);
    thickLineRGBA(surface, front_x, wheel_y, front_x + spoke_dx, wheel_y + spoke_dy, 1, 119, 130,
                  137, alpha);
  }
  thickLineRGBA(surface, rear_x, wheel_y, seat_x, seat_y, 2, 205, 120, 74, alpha);
  thickLineRGBA(surface, seat_x, seat_y, crank_x, wheel_y, 2, 205, 120, 74, alpha);
  thickLineRGBA(surface, crank_x, wheel_y, rear_x, wheel_y, 2, 205, 120, 74, alpha);
  thickLineRGBA(surface, seat_x, seat_y, front_x, wheel_y, 2, 205, 120, 74, alpha);
  thickLineRGBA(surface, seat_x, seat_y - wheel_radius * 2, crank_x, wheel_y - wheel_radius * 4, 3,
                54, 65, 77, alpha);
  double pedal_angle = progress * 12.56637061436;
  int pedal_x = crank_x + (int)(cos(pedal_angle) * wheel_radius * 2);
  int pedal_y = wheel_y + (int)(sin(pedal_angle) * wheel_radius * 2);
  int handlebar_x = front_x - direction * wheelbase / 8;
  int handlebar_y = wheel_y - wheel_radius * 3;
  thickLineRGBA(surface, seat_x, seat_y - wheel_radius * 2, handlebar_x, handlebar_y, 2, 54, 65, 77,
                alpha);
  thickLineRGBA(surface, seat_x, seat_y - wheel_radius * 2, pedal_x, pedal_y, 2, 54, 65, 77, alpha);
  thickLineRGBA(surface, pedal_x, pedal_y, crank_x, wheel_y, 1, 205, 120, 74, alpha);
  filledCircleRGBA(surface, crank_x, wheel_y - wheel_radius * 6, wheel_radius / 2 + 1, 224, 194,
                   151, alpha);
}


static void sky_event_26(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int creature = 0; creature < 5; creature++) {
    int creature_x = center_x + (creature - 2) * size * 2;
    int creature_y = center_y + (int)(sin(progress * 6.28 + creature) * size * 2);
    thickLineRGBA(surface, creature_x - size / 2, creature_y, creature_x, creature_y - size / 2, 1,
                  25, 30, 36, alpha);
    thickLineRGBA(surface, creature_x, creature_y - size / 2, creature_x + size / 2, creature_y, 1,
                  25, 30, 36, alpha);
  }
  int fish_y = surface->h * 61 / 100 - (int)(sin(progress * 3.14159265359) * scale * 0.14);
  filledTrigonRGBA(surface, center_x + size * 2, fish_y, center_x - size, fish_y - size,
                   center_x - size, fish_y + size, 105, 187, 203, alpha);
  filledTrigonRGBA(surface, center_x - size, fish_y - size, center_x - size * 2, fish_y,
                   center_x - size, fish_y + size, 84, 157, 176, alpha);
}


static void sky_event_27(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int cube = 0; cube < 4; cube++) {
    int cube_x = center_x + (cube - 2) * size * 2;
    int cube_y = center_y + (int)(sin(progress * 6.28 + cube) * size * 2);
    int half = size / 2 + 2;
    double rotation = progress * 6.28318530718 + cube * 0.9;
    int offset_x = (int)(cos(rotation) * half);
    int offset_y = (int)(sin(rotation) * half);
    int top_x = cube_x + offset_x;
    int top_y = cube_y - offset_y;
    int right_x = cube_x + offset_y;
    int right_y = cube_y + offset_x;
    int bottom_x = cube_x - offset_x;
    int bottom_y = cube_y + offset_y;
    int left_x = cube_x - offset_y;
    int left_y = cube_y - offset_x;
    filledTrigonRGBA(surface, top_x, top_y, right_x, right_y, bottom_x, bottom_y, 155, 214, 237,
                     (Uint8)(alpha * 0.38));
    filledTrigonRGBA(surface, top_x, top_y, left_x, left_y, bottom_x, bottom_y, 200, 230, 241,
                     (Uint8)(alpha * 0.28));
    thickLineRGBA(surface, top_x, top_y, bottom_x, bottom_y, 1, 224, 245, 251, alpha);
    filledCircleRGBA(surface, top_x, top_y, 1, 255, 255, 255, alpha);
  }
}


static void sky_event_28(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int card = 0; card < 3; card++) {
    int card_x =
        center_x + (card - 1) * (int)(size * (2.0 + 0.3 * sin(progress * 6.28318530718 + card)));
    int card_y = center_y - abs(card - 1) * size / 2 +
                 (int)(cos(progress * 6.28318530718 + card) * size / 3);
    int card_half_width = (int)(size * (0.18 + 0.32 * fabs(cos(progress * 6.28318530718 + card))));
    if (card_half_width < 2) card_half_width = 2;
    boxRGBA(surface, card_x - card_half_width, card_y - size, card_x + card_half_width,
            card_y + size, 225, 220, 203, alpha);
    thickLineRGBA(surface, card_x, card_y - size / 2, card_x, card_y + size / 2, 1, 163, 57, 64,
                  alpha);
  }
  int coin_radius = size / 2 + 2;
  int coin_x = center_x + (int)(sin(progress * 12.56) * size * 4);
  int coin_y = center_y - size * 2 - (int)(fabs(sin(progress * 12.56)) * size * 3);
  filledCircleRGBA(surface, coin_x, coin_y, coin_radius, 205, 161, 83, alpha);
  circleRGBA(surface, coin_x, coin_y, coin_radius - 1, 255, 226, 159, alpha);
}


static void sky_event_29(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 82, 17, 29, (Uint8)(15 * fade));
  unsigned int bolt_seed = seed;
  int bolt_x = center_x;
  int bolt_y = surface->h * 12 / 100;
  for (int bolt = 0; bolt < 8; bolt++) {
    bolt_seed = bolt_seed * 1664525u + 1013904223u;
    int next_x = bolt_x + (int)(bolt_seed % (scale / 12 + 1)) - scale / 24;
    int next_y = bolt_y + scale / 15;
    thickLineRGBA(surface, bolt_x, bolt_y, next_x, next_y, 2, 232, 67, 83, alpha);
    bolt_x = next_x;
    bolt_y = next_y;
  }
}


static void sky_event_30(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int cup = 0; cup < 3; cup++) {
    int cup_x = center_x + (cup - 1) * size * 3;
    int cup_y = center_y + abs(cup - 1) * size / 3;
    boxRGBA(surface, cup_x - size, cup_y - size, cup_x + size, cup_y + size, 211, 204, 190, alpha);
    filledCircleRGBA(surface, cup_x, cup_y - size, size / 2, (Uint8)(105 + cup * 33),
                     (Uint8)(59 + cup * 21), (Uint8)(36 + cup * 19), alpha);
    circleRGBA(surface, cup_x + size, cup_y, size / 2, 211, 204, 190, alpha);
    for (int steam = 0; steam < 3; steam++) {
      int steam_x = cup_x - size / 2 + steam * size / 2;
      int steam_y = cup_y - size * 2 - (int)(fabs(sin(progress * 8.0 + steam)) * size);
      thickLineRGBA(surface, steam_x, steam_y + size, steam_x + size / 4, steam_y, 1, 206, 206, 198,
                    (Uint8)(alpha * 0.65));
    }
  }
}


static void sky_event_31(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int sprig = 0; sprig < 5; sprig++) {
    int stem_x = center_x + (sprig - 2) * size;
    int stem_y = center_y + size * 2;
    int tip_y = stem_y - size * (3 + sprig % 3);
    int sway = (int)(sin(ticks / 330.0 + sprig * 1.4) * size / 5);
    thickLineRGBA(surface, stem_x, stem_y, stem_x + sway, tip_y, 2, 92, 130, 73, alpha);
    for (int leaf = 1; leaf <= 4; leaf++) {
      int leaf_y = stem_y - leaf * size * 3 / 4;
      int leaf_direction = leaf % 2 ? -1 : 1;
      int leaf_x = stem_x + sway * leaf / 4;
      int leaf_tip_x = leaf_x + leaf_direction * size;
      filledTrigonRGBA(surface, leaf_x, leaf_y, leaf_tip_x, leaf_y - size / 3, leaf_tip_x,
                       leaf_y + size / 3, (Uint8)(74 + sprig * 6), (Uint8)(126 + leaf * 9), 71,
                       alpha);
    }
  }
}


static void sky_event_32(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int light = 0; light < 18; light++) {
    int light_x = (seed + light * surface->w / 18) % surface->w;
    int light_y = surface->h * (14 + (light * 17 + seed) % 62) / 100;
    Uint8 pulse = (Uint8)(alpha * (0.35 + 0.65 * (0.5 + 0.5 * sin(progress * 18.0 + light))));
    filledCircleRGBA(surface, light_x, light_y, 2 + light % 3, (Uint8)(90 + light * 7),
                     (Uint8)(129 + light * 5), (Uint8)(176 + light * 3), pulse);
  }
}


static void sky_event_33(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int burner_y = surface->h * 78 / 100;
  filledTrigonRGBA(surface, center_x, burner_y - size, center_x - size, burner_y + size / 2,
                   center_x + size, burner_y + size / 2, 104, 87, 67, alpha);
  for (int curl = 0; curl < 4; curl++) {
    int smoke_x = center_x + (int)(sin(progress * 7.0 + curl) * size * 2);
    int smoke_y = burner_y - size * (2 + curl) - (int)(progress * size * (curl + 1));
    filledCircleRGBA(surface, smoke_x, smoke_y, size / 3 + 1, (Uint8)(151 + curl * 12),
                     (Uint8)(159 + curl * 10), (Uint8)(166 + curl * 8),
                     (Uint8)(alpha * (0.45 - curl * 0.07)));
  }
}


static void sky_event_34(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int wisp = 0; wisp < 7; wisp++) {
    int smoke_x = center_x + (int)(sin(progress * 5.0 + wisp) * size * (2 + wisp));
    int smoke_y = center_y - wisp * size / 2;
    filledCircleRGBA(surface, smoke_x, smoke_y, size / 2 + 1, (Uint8)(143 + wisp * 9),
                     (Uint8)(150 + wisp * 8), (Uint8)(162 + wisp * 7), (Uint8)(alpha * 0.42));
  }
}


static void sky_event_35(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int temple_y = surface->h * 71 / 100;
  for (int step = 0; step < 5; step++) {
    int half_width = size * (5 - step);
    int top_y = temple_y - step * size / 2;
    boxRGBA(surface, center_x - half_width, top_y - size / 2, center_x + half_width,
            top_y + size / 2, (Uint8)(116 + step * 12), (Uint8)(102 + step * 8), 83, alpha);
  }
  for (int ray = 0; ray < 12; ray++) {
    double angle = ray * 0.52359877559 + progress;
    int end_x = center_x + (int)(cos(angle) * size * 5);
    int end_y = surface->h * 32 / 100 + (int)(sin(angle) * size * 3);
    thickLineRGBA(surface, center_x, surface->h * 32 / 100, end_x, end_y, 1, 218, 183, 120,
                  (Uint8)(alpha * 0.45));
  }
}


static void sky_event_36(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, 24, 12, 39, (Uint8)(20 * fade));
  int eye_x = center_x;
  int eye_y = surface->h * 36 / 100;
  int eye_radius = size * 2;
  filledCircleRGBA(surface, eye_x, eye_y, eye_radius, 5, 4, 14, alpha);
  circleRGBA(surface, eye_x, eye_y, eye_radius + size, 117, 89, 161, (Uint8)(alpha * 0.8));
  for (int tendril = 0; tendril < 8; tendril++) {
    double angle = tendril * 0.78539816339 + progress * direction;
    int previous_x = eye_x + (int)(cos(angle) * eye_radius);
    int previous_y = eye_y + (int)(sin(angle) * eye_radius);
    for (int segment = 1; segment <= 5; segment++) {
      int next_x = eye_x + (int)(cos(angle) * (eye_radius + segment * size));
      int next_y = eye_y + (int)(sin(angle) * (eye_radius + segment * size) +
                                 sin(progress * 10.0 + segment + tendril) * size / 2);
      thickLineRGBA(surface, previous_x, previous_y, next_x, next_y, 2, 83, 68, 127,
                    (Uint8)(alpha * 0.72));
      previous_x = next_x;
      previous_y = next_y;
    }
  }
}


static void sky_event_37(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int mirror_x = surface->w * (30 + (seed >> 8) % 40) / 100;
  int mirror_y = surface->h * 43 / 100;
  int mirror_width = size * 6;
  int mirror_height = size * 10;
  boxRGBA(surface, mirror_x - mirror_width / 2, mirror_y - mirror_height / 2,
          mirror_x + mirror_width / 2, mirror_y + mirror_height / 2, 37, 55, 76,
          (Uint8)(alpha * 0.45));
  thickLineRGBA(surface, mirror_x - mirror_width / 2, mirror_y - mirror_height / 2,
                mirror_x + mirror_width / 2, mirror_y - mirror_height / 2, 2, 205, 221, 231, alpha);
  thickLineRGBA(surface, mirror_x - mirror_width / 2, mirror_y + mirror_height / 2,
                mirror_x + mirror_width / 2, mirror_y + mirror_height / 2, 2, 205, 221, 231, alpha);
  for (int reflection = 0; reflection < 8; reflection++) {
    int line_y = mirror_y - mirror_height / 2 + reflection * mirror_height / 7;
    int ripple = (int)(sin(progress * 12.0 + reflection) * size / 2);
    thickLineRGBA(surface, mirror_x - mirror_width / 2 + size / 3 + ripple, line_y,
                  mirror_x + mirror_width / 2 - size / 3 - ripple, line_y + size / 4, 1, 150, 196,
                  218, (Uint8)(alpha * (0.45 + reflection * 0.04)));
  }
  int reflected_sun_y = mirror_y + mirror_height / 3 + (int)(sin(progress * 8.0) * size / 2);
  filledCircleRGBA(surface, mirror_x, reflected_sun_y, size / 2, 226, 218, 188,
                   (Uint8)(alpha * 0.72));
}


static void sky_event_38(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int water_y = surface->h * 77 / 100;
  for (int wave = 0; wave < 9; wave++) {
    int previous_x = 0;
    int previous_y = water_y + wave * size / 2;
    for (int segment = 1; segment <= 16; segment++) {
      int next_x = surface->w * segment / 16;
      int next_y = water_y + wave * size / 2 +
                   (int)(sin(segment * 0.75 + progress * 10.0 + wave) * size / 3);
      thickLineRGBA(surface, previous_x, previous_y, next_x, next_y, 1, 101, 156, 190,
                    (Uint8)(alpha * 0.55));
      previous_x = next_x;
      previous_y = next_y;
    }
  }
  for (int glint = 0; glint < 14; glint++) {
    int glint_x = (seed + glint * surface->w / 14) % surface->w;
    int glint_y = water_y + (int)(sin(progress * 8.0 + glint) * size * 2);
    thickLineRGBA(surface, glint_x - size / 3, glint_y, glint_x + size / 3, glint_y, 1, 192, 222,
                  231, (Uint8)(alpha * 0.65));
  }
}


static void sky_event_39(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int orbit_x = center_x;
  int orbit_y = center_y;
  int orbit_radius = size * 5;
  ellipseRGBA(surface, orbit_x, orbit_y, orbit_radius, orbit_radius / 2, 137, 172, 201,
              (Uint8)(alpha * 0.6));
  for (int satellite = 0; satellite < 5; satellite++) {
    double angle = progress * 12.56637061436 + satellite * 1.25663706144;
    int satellite_x = orbit_x + (int)(cos(angle) * orbit_radius);
    int satellite_y = orbit_y + (int)(sin(angle) * orbit_radius / 2);
    filledCircleRGBA(surface, satellite_x, satellite_y, 2 + satellite % 3, 180, 204, 221, alpha);
  }
  filledCircleRGBA(surface, orbit_x, orbit_y, size,
                   (Uint8)(180 + (int)(35 * sin(progress * 12.56))), 141, 215, alpha);
}


static void sky_event_40(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int plane_y = center_y + (int)(sin(progress * 6.28318530718) * size * 2);
  int nose_x = center_x + direction * size * 3;
  int tail_x = center_x - direction * size * 3;
  thickLineRGBA(surface, tail_x, plane_y, nose_x, plane_y, 3, 140, 153, 164, alpha);
  thickLineRGBA(surface, center_x - direction * size, plane_y - size / 5,
                center_x + direction * size, plane_y + size / 5, 2, 170, 182, 191, alpha);
  filledTrigonRGBA(surface, tail_x + direction * size, plane_y, tail_x - direction * size,
                   plane_y - size, tail_x - direction * size, plane_y + size, 126, 141, 154, alpha);
  if ((phase / 240) % 2 == 0) {
    filledCircleRGBA(surface, center_x - size * 2, plane_y, 2, 255, 54, 45, alpha);
    filledCircleRGBA(surface, center_x + size * 2, plane_y, 2, 255, 54, 45, alpha);
  }
}


static void sky_event_41(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int planet_x = surface->w / 2;
  int planet_y = surface->h * 66 / 100;
  int satellite_x = planet_x + (int)(cos(progress * 6.28318530718) * scale * 0.22);
  int satellite_y = planet_y + (int)(sin(progress * 6.28318530718) * scale * 0.11);
  filledCircleRGBA(surface, planet_x, planet_y, scale / 12, 32, 46, 65, (Uint8)(alpha * 0.6));
  int panel_width = size * 2;
  boxRGBA(surface, satellite_x - panel_width * 2, satellite_y - size / 3, satellite_x - size,
          satellite_y + size / 3, 79, 133, 163, alpha);
  boxRGBA(surface, satellite_x + size, satellite_y - size / 3, satellite_x + panel_width * 2,
          satellite_y + size / 3, 79, 133, 163, alpha);
  boxRGBA(surface, satellite_x - size, satellite_y - size / 2, satellite_x + size,
          satellite_y + size / 2, 151, 164, 170, alpha);
  if ((phase / 300) % 2 == 0) {
    filledCircleRGBA(surface, satellite_x, satellite_y, 2, 255, 48, 41, alpha);
  }
}


static void sky_event_42(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int blimp_y = center_y + (int)(sin(progress * 6.28318530718) * size);
  int blimp_length = size * 5;
  for (int section = -2; section <= 2; section++) {
    int section_radius = size * (3 - abs(section)) / 2;
    if (section_radius < 2) section_radius = 2;
    filledCircleRGBA(surface, center_x + section * size, blimp_y, section_radius, 165, 169, 158,
                     alpha);
  }
  thickLineRGBA(surface, center_x - blimp_length / 3, blimp_y + size / 2,
                center_x + blimp_length / 3, blimp_y + size / 2, 2, 100, 103, 99, alpha);
  filledTrigonRGBA(surface, center_x + direction * blimp_length / 2, blimp_y,
                   center_x + direction * blimp_length / 2 - direction * size, blimp_y - size,
                   center_x + direction * blimp_length / 2 - direction * size, blimp_y + size, 127,
                   137, 133, alpha);
  if ((phase / 360) % 2 == 0) {
    filledCircleRGBA(surface, center_x, blimp_y, 2, 255, 48, 41, alpha);
  }
}


// Three hot air balloons drift by, bobbing, their burners flaring now and then.
static void sky_event_43(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 colors[3][3] = {{214, 92, 76}, {231, 184, 86}, {92, 147, 201}};
  for (int balloon = 0; balloon < 3; balloon++) {
    const Uint8 *c = colors[balloon];
    int radius = (int)(size * (1.3 - 0.2 * balloon));
    int bx = center_x - direction * balloon * size * 5;
    int by = center_y + (int)(sin(progress * 9.4 + balloon * 1.7) * size) + balloon * size * 2;
    filledCircleRGBA(surface, bx, by, radius, c[0], c[1], c[2], alpha);
    filledTrigonRGBA(surface, bx - radius * 6 / 10, by + radius * 7 / 10, bx + radius * 6 / 10,
                     by + radius * 7 / 10, bx, by + radius * 16 / 10, (Uint8)(c[0] * 0.8),
                     (Uint8)(c[1] * 0.8), (Uint8)(c[2] * 0.8), alpha);
    ellipseRGBA(surface, bx, by, radius / 2, radius, 245, 235, 215, (Uint8)(alpha * 0.6));
    lineRGBA(surface, bx - radius / 2, by + radius * 11 / 10, bx - radius / 5, by + radius * 2, 90,
             72, 60, alpha);
    lineRGBA(surface, bx + radius / 2, by + radius * 11 / 10, bx + radius / 5, by + radius * 2, 90,
             72, 60, alpha);
    boxRGBA(surface, bx - radius / 5, by + radius * 2, bx + radius / 5,
            by + radius * 2 + radius / 4, 105, 80, 60, alpha);
    if ((phase / 140 + balloon) % 7 < 2) {
      filledCircleRGBA(surface, bx, by + radius * 3 / 2, 2, 255, 190, 90, alpha);
    }
  }
}


// A V of geese flaps past.
static void sky_event_44(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int goose = -3; goose <= 3; goose++) {
    int row = abs(goose);
    int gx = center_x - direction * row * size * 2;
    int gy = center_y + goose * size;
    double flap = sin(phase / 150.0 + row * 0.7 + (goose < 0 ? 1.0 : 0.0));
    int wing = size * 4 / 5;
    int tip_y = gy - (int)(flap * wing * 0.7);
    thickLineRGBA(surface, gx, gy, gx - wing, tip_y, 2, 60, 70, 85, alpha);
    thickLineRGBA(surface, gx, gy, gx + wing, tip_y, 2, 60, 70, 85, alpha);
    filledCircleRGBA(surface, gx + direction * size / 4, gy, 2, 60, 70, 85, alpha);
  }
}


// A small meteor shower: streaks cross the sky one after another.
static void sky_event_45(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int meteor = 0; meteor < 7; meteor++) {
    double begin = meteor * 0.11;
    double u = (progress - begin) / 0.25;
    if (u < 0.0 || u > 1.0) continue;
    unsigned int h = fx_hash(seed + meteor * 977u, 7);
    double sx = surface->w * (0.1 + 0.8 * (h & 255) / 255.0);
    double sy = surface->h * (0.03 + 0.2 * ((h >> 8) & 255) / 255.0);
    double reach = scale * 0.5;
    double hx = sx + direction * reach * u, hy = sy + reach * 0.45 * u;
    double tail = size * 5.0;
    Uint8 a = weapon_alpha(alpha * 1.4 * sin(3.14159265 * u));
    thickLineRGBA(surface, (int)hx, (int)hy, (int)(hx - direction * tail), (int)(hy - tail * 0.45),
                  2, 205, 220, 240, (Uint8)(a * 0.5));
    thickLineRGBA(surface, (int)hx, (int)hy, (int)(hx - direction * tail * 0.5),
                  (int)(hy - tail * 0.22), 1, 255, 250, 235, a);
    filledCircleRGBA(surface, (int)hx, (int)hy, 2, 255, 250, 235, a);
  }
}


// Three lights in a triangle hang in the sky, turn slowly and dart sideways.
static void sky_event_46(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 white[3] = {255, 240, 200};
  double base_x = surface->w * (0.2 + 0.4 * ((seed >> 5) % 100) / 100.0);
  double dart = smooth_step(0.45, 0.5, progress) * surface->w * 0.12 +
                smooth_step(0.75, 0.78, progress) * surface->w * 0.3;
  double cx = base_x + direction * dart;
  double cy = center_y + sin(progress * 8.0) * size * 0.3;
  double radius = size * 1.4;
  int px[3], py[3];
  for (int k = 0; k < 3; k++) {
    double angle = progress * 2.0 + k * 2.0944;
    px[k] = (int)(cx + cos(angle) * radius);
    py[k] = (int)(cy + sin(angle) * radius * 0.6);
  }
  filledTrigonRGBA(surface, px[0], py[0], px[1], py[1], px[2], py[2], 20, 24, 34,
                   (Uint8)(alpha * 0.4));
  for (int k = 0; k < 3; k++) {
    weapon_glow(surface, px[k], py[k], 2.5, white, alpha * 1.4);
  }
  if ((phase / 400) % 2 == 0) {
    filledCircleRGBA(surface, (int)cx, (int)cy, 2, 255, 60, 50, alpha);
  }
}


// A small plane writes a loop in the sky with a white trail.
static void sky_event_47(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double radius = size * 4.0;
  double span = surface->w * 1.2;
  double start_x = direction > 0 ? -surface->w * 0.1 : surface->w * 1.1;
  double base_y = surface->h * 0.3 + (seed >> 12) % 100 * surface->h * 0.001;
  const int samples = 70;
  double last_x = 0.0, last_y = 0.0;
  bool have_last = false;
  for (int k = 0; k <= samples; k++) {
    double s = progress - 0.25 * (samples - k) / samples;
    if (s < 0.0) continue;
    double a = 6.2831853 * smooth_step(0.4, 0.6, s);
    double px = start_x + direction * (s * span - radius * sin(a));
    double py = base_y - radius * (1.0 - cos(a));
    if (have_last) {
      double recent = (double)k / samples;
      thickLineRGBA(surface, (int)last_x, (int)last_y, (int)px, (int)py, 3, 240, 244, 250,
                    (Uint8)(alpha * 0.55 * recent));
    }
    last_x = px;
    last_y = py;
    have_last = true;
    if (k == samples) {
      double ahead = progress - 0.004;
      double a2 = 6.2831853 * smooth_step(0.4, 0.6, ahead);
      double qx = start_x + direction * (ahead * span - radius * sin(a2));
      double qy = base_y - radius * (1.0 - cos(a2));
      double dx = px - qx, dy = py - qy;
      double len = sqrt(dx * dx + dy * dy);
      if (len < 0.001) {
        dx = direction;
        dy = 0.0;
        len = 1.0;
      }
      dx /= len;
      dy /= len;
      thickLineRGBA(surface, (int)(px - dx * size), (int)(py - dy * size), (int)(px + dx * size),
                    (int)(py + dy * size), 3, 230, 232, 238, alpha);
      thickLineRGBA(surface, (int)(px - dy * size * 0.9), (int)(py + dx * size * 0.9),
                    (int)(px + dy * size * 0.9), (int)(py - dx * size * 0.9), 2, 200, 205, 215,
                    alpha);
    }
  }
}


// A huge whale made of faint dusk colours glides through the sky, its belly
// dotted with lights.
static void sky_event_48(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const double profile[7] = {1.0, 1.5, 1.6, 1.4, 1.0, 0.7, 0.45};
  int whale_x[7], whale_y[7];
  for (int i = 6; i >= 0; i--) {
    whale_x[i] = center_x - direction * i * size * 8 / 5;
    whale_y[i] = center_y + (int)(sin(phase / 700.0 - i * 0.7) * size * 0.5);
    filledCircleRGBA(surface, whale_x[i], whale_y[i], (int)(size * profile[i]), 70, 85, 120,
                     (Uint8)(alpha * 0.8));
  }
  for (int i = 1; i < 6; i++) {
    filledCircleRGBA(surface, whale_x[i], whale_y[i] + (int)(size * profile[i] * 0.55), 2, 150, 220,
                     230, (Uint8)(alpha * (0.6 + 0.4 * sin(phase / 300.0 + i))));
  }
  int tail_x = whale_x[6] - direction * size / 2, tail_y = whale_y[6];
  double wave = sin(phase / 700.0 - 6 * 0.7 - 0.5);
  filledTrigonRGBA(surface, tail_x, tail_y, tail_x - direction * size,
                   tail_y - size + (int)(wave * size / 2), tail_x - direction * size / 3, tail_y,
                   70, 85, 120, (Uint8)(alpha * 0.8));
  filledTrigonRGBA(surface, tail_x, tail_y, tail_x - direction * size,
                   tail_y + size + (int)(wave * size / 2), tail_x - direction * size / 3, tail_y,
                   70, 85, 120, (Uint8)(alpha * 0.8));
  filledTrigonRGBA(surface, whale_x[2], whale_y[2] + size, whale_x[2] - direction * size,
                   whale_y[2] + size * 2, whale_x[2] + direction * size / 4,
                   whale_y[2] + size * 3 / 2, 55, 68, 100, (Uint8)(alpha * 0.8));
  filledCircleRGBA(surface, whale_x[0] + direction * size / 3, whale_y[0] - size / 4, 2, 230, 240,
                   250, alpha);
}


// A distant storm cell: a dark anvil cloud with rain below and sudden lightning
// bolts.
static void sky_event_49(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  bool flash = fx_hash(seed, phase / 170) % 7 == 0;
  unsigned int bolt_seed = fx_hash(seed, phase / 170 + 11);
  int r = size * 3 / 2;
  Uint8 cr = flash ? 200 : 66, cg = flash ? 210 : 72, cb = flash ? 250 : 92;
  for (int puff = 0; puff < 5; puff++) {
    filledCircleRGBA(surface, center_x + (puff - 2) * r * 7 / 10, center_y - (puff % 2) * r / 3,
                     r * (puff == 2 ? 12 : 9) / 10, cr, cg, cb, (Uint8)(alpha * 0.85));
  }
  boxRGBA(surface, center_x - r * 2, center_y + r / 2, center_x + r * 2, center_y + r * 3 / 5, 55,
          60, 78, (Uint8)(alpha * 0.8));
  for (int drop = 0; drop < 14; drop++) {
    int rx = center_x - r * 3 / 2 + (drop * r * 3 / 13);
    int ry = center_y + r * 3 / 5 + ((phase / 40 + drop * 5) % 10) * size / 5;
    lineRGBA(surface, rx, ry, rx - 2, ry + size / 3, 140, 155, 185, (Uint8)(alpha * 0.45));
  }
  if (flash) {
    int bx = center_x + (int)(bolt_seed % 5) * r / 3 - r;
    int by = center_y + r / 2;
    for (int segment = 0; segment < 6; segment++) {
      int nx = bx + (int)((bolt_seed >> (segment * 3)) % 7) * size / 5 - size * 3 / 5;
      int ny = by + size * 4 / 5;
      thickLineRGBA(surface, bx, by, nx, ny, 3, 190, 205, 255, (Uint8)(alpha * 0.4));
      thickLineRGBA(surface, bx, by, nx, ny, 1, 255, 255, 255, alpha);
      bx = nx;
      by = ny;
    }
  }
}


// A total solar eclipse: a dark moon slides over the sun, a corona blooms at
// totality and a bright glint flares on the edge as it ends.
static void sky_event_50(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double cx = surface->w * (0.3 + 0.4 * ((seed >> 4) % 100) / 100.0);
  double radius = size * 1.6;
  double offset = (progress - 0.5) * radius * 5.0 * direction;
  double cover = 1.0 - smooth_step(0.0, radius * 1.2, fabs(offset));
  Uint8 solid = (Uint8)(fade * 235);
  filledCircleRGBA(surface, (int)cx, center_y, (int)radius, 255, 236, 170, solid);
  if (cover > 0.0) {
    for (int ray = 0; ray < 16; ray++) {
      double angle = ray * 0.3927 + progress * 0.5;
      double length = radius * (1.6 + 0.6 * sin(ray * 2.1 + phase / 500.0)) * cover;
      thickLineRGBA(surface, (int)(cx + cos(angle) * radius), (int)(center_y + sin(angle) * radius),
                    (int)(cx + cos(angle) * (radius + length)),
                    (int)(center_y + sin(angle) * (radius + length)), 1, 235, 240, 255,
                    (Uint8)(alpha * 0.8 * cover));
    }
    circleRGBA(surface, (int)cx, center_y, (int)(radius * 1.15), 255, 245, 215,
               (Uint8)(alpha * cover));
  }
  filledCircleRGBA(surface, (int)(cx + offset), center_y, (int)radius, 12, 14, 24, solid);
  if (fabs(offset) > radius * 0.03 && fabs(offset) < radius * 0.35) {
    static const Uint8 glint[3] = {255, 245, 220};
    weapon_glow(surface, cx - (offset > 0 ? 1 : -1) * radius * 0.95, center_y, radius * 0.3, glint,
                alpha * 1.6);
  }
}


// A long jade dragon undulates across the sky, with a mane of fins and whiskers
// streaming behind its snout.
static void sky_event_51(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  const int segments = 16;
  double wave = phase / 260.0;
  int px[segments + 1], py[segments + 1];
  for (int i = 0; i <= segments; i++) {
    px[i] = x - direction * i * size * 11 / 10;
    py[i] = y + (int)(sin(wave - i * 0.55) * size * (0.6 + 0.08 * i));
  }
  for (int i = segments; i >= 0; i--) {
    int r = (int)(size * (0.2 + 0.55 * (1.0 - (double)i / segments)));
    if (i == 0) r = size * 3 / 4;
    if (r < 2) r = 2;
    filledCircleRGBA(surface, px[i], py[i], r, 96, 170, 128, (Uint8)(alpha * 0.9));
    filledCircleRGBA(surface, px[i], py[i] + r / 3, r * 2 / 3, 200, 215, 150,
                     (Uint8)(alpha * 0.45));
    if (i % 3 == 1) {
      filledTrigonRGBA(surface, px[i] - 2, py[i] - r, px[i] + 2, py[i] - r, px[i] - direction,
                       py[i] - r - size / 2, 220, 160, 90, (Uint8)(alpha * 0.9));
    }
    if (i == 4 || i == 10) {
      thickLineRGBA(surface, px[i], py[i] + r, px[i] + direction * size / 3, py[i] + r + size / 2,
                    2, 80, 140, 106, alpha);
    }
  }
  filledTrigonRGBA(surface, x + direction * size / 3, y - size / 3, x + direction * size / 3,
                   y + size / 3, x + direction * size * 13 / 10, y + size / 10, 96, 170, 128,
                   alpha);
  thickLineRGBA(surface, x - direction * size / 10, y - size * 7 / 10, x - direction * size * 6 / 5,
                y - size * 3 / 2, 2, 220, 180, 100, alpha);
  for (int whisker = -1; whisker <= 1; whisker += 2) {
    thickLineRGBA(surface, x + direction * size * 12 / 10, y + size / 10,
                  x + direction * size * 2 + direction * size / 2,
                  y + size / 3 + whisker * (int)(sin(wave * 2.0) * size * 0.5) + whisker * size / 3,
                  1, 230, 220, 170, (Uint8)(alpha * 0.8));
  }
  filledCircleRGBA(surface, x + direction * size / 5, y - size / 5, 2, 255, 220, 120, alpha);
  filledTrigonRGBA(surface, px[segments], py[segments] - 2, px[segments], py[segments] + 2,
                   px[segments] - direction * size, py[segments], 220, 160, 90,
                   (Uint8)(alpha * 0.9));
}


// A small drift of jellyfish pulses through the sky, their tentacles trailing.
static void sky_event_52(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int j = 0; j < 5; j++) {
    int jx = x - direction * j * size * 2 + (j % 2) * size;
    int jy = y + (int)(sin(phase / 700.0 + j * 1.3) * size * 1.5) + (j % 3) * size;
    double pulse = 0.5 + 0.5 * sin(phase / 380.0 + j);
    int r = (int)(size * (0.9 + 0.15 * pulse));
    Uint8 cr = j % 2 ? 140 : 220, cg = j % 2 ? 210 : 150, cb = j % 2 ? 230 : 200;
    filledPieRGBA(surface, jx, jy, r, 180, 360, cr, cg, cb, (Uint8)(alpha * 0.55));
    ellipseRGBA(surface, jx, jy, r, r / 4 + 1, cr, cg, cb, (Uint8)(alpha * 0.8));
    filledCircleRGBA(surface, jx, jy - r / 3, r / 4 + 1, 255, 255, 255, (Uint8)(alpha * 0.35));
    for (int k = 0; k < 6; k++) {
      int tx = jx + (int)((k - 2.5) * r * 0.35), ty = jy;
      for (int seg = 1; seg <= 4; seg++) {
        int nx = tx + (int)(sin(phase / 300.0 - seg * 0.8 + k) * size * 0.25 * seg) / 2;
        int ny = ty + size / 2;
        lineRGBA(surface, tx, ty, nx, ny, cr, cg, cb, (Uint8)(alpha * (0.55 - 0.1 * seg)));
        tx = nx;
        ty = ny;
      }
    }
  }
}


// A train of satellites crosses the sky in a line, glints running along it.
static void sky_event_53(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 white[3] = {240, 244, 255};
  const int count = 14;
  int spacing = size * 6 / 5;
  for (int i = 0; i < count; i++) {
    double glint = 0.5 + 0.5 * sin(phase / 250.0 - i * 0.7);
    int sx = x - direction * i * spacing, sy = y + i * spacing / 8;
    double fade_tail = 1.0 - (double)i / (count * 1.4);
    filledCircleRGBA(surface, sx, sy, glint > 0.85 ? 2 : 1, white[0], white[1], white[2],
                     weapon_alpha(alpha * (0.5 + 0.5 * glint) * fade_tail * 1.4));
    if (glint > 0.8) weapon_glow(surface, sx, sy, 2.0, white, alpha * glint * fade_tail);
  }
}


// A star explodes: it flares with spikes, then a coloured shock shell expands
// and thins out.
static void sky_event_54(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 white[3] = {235, 240, 255};
  double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
  double flare = smooth_step(0.1, 0.28, progress) * (1.0 - smooth_step(0.35, 0.7, progress));
  double glow = size * 0.2 + size * 2.2 * flare;
  weapon_glow(surface, cx, center_y, glow, white, 235.0 * fade * (0.25 + 0.75 * flare));
  double spike = size * (1.0 + 8.0 * flare);
  for (int k = 0; k < 4; k++) {
    double a = k * SCENE_PI / 4.0, len = k % 2 ? spike * 0.5 : spike;
    thickLineRGBA(surface, (int)(cx - cos(a) * len), (int)(center_y - sin(a) * len),
                  (int)(cx + cos(a) * len), (int)(center_y + sin(a) * len), 1, 235, 240, 255,
                  weapon_alpha(200.0 * flare * fade));
  }
  double shock = smooth_step(0.28, 1.0, progress);
  if (shock > 0.0) {
    double radius = shock * scale * 0.2;
    for (int k = 0; k < 3; k++) {
      Uint8 a = weapon_alpha(160.0 * (1.0 - shock) * fade * (1.0 - k * 0.25));
      circleRGBA(surface, (int)cx, center_y, (int)(radius * (1.0 - k * 0.12)), k == 1 ? 255 : 180,
                 k == 1 ? 150 : 150, k == 1 ? 170 : 255, a);
    }
    filledCircleRGBA(surface, (int)cx, center_y, (int)(radius * 0.95), 180, 150, 255,
                     weapon_alpha(10.0 * (1.0 - shock) * fade));
  }
}


// Dandelion seeds drift past on their little parachutes.
static void sky_event_55(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 9; i++) {
    int sx = x - direction * i * size * 9 / 5;
    int sy = y + (int)(sin(phase / 900.0 + i * 1.9) * size * 2.0) + (i % 3) * size * 6 / 5;
    double tilt = sin(phase / 700.0 + i) * 0.4;
    int stem_x = sx - (int)(sin(tilt) * size * 0.8), stem_y = sy + (int)(cos(tilt) * size * 0.8);
    lineRGBA(surface, sx, sy, stem_x, stem_y, 235, 235, 225, (Uint8)(alpha * 0.8));
    filledCircleRGBA(surface, stem_x, stem_y, 1, 200, 180, 140, alpha);
    for (int ray = 0; ray < 7; ray++) {
      double a = -SCENE_PI * (0.1 + 0.8 * ray / 6.0) + tilt;
      lineRGBA(surface, sx, sy, sx + (int)(cos(a) * size * 0.6), sy + (int)(sin(a) * size * 0.6),
               235, 235, 225, (Uint8)(alpha * 0.9));
    }
  }
}


// A double rainbow sweeps in across the sky and fades.
static void sky_event_56(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 band[7][3] = {{220, 70, 70},  {235, 150, 60}, {235, 215, 80}, {90, 190, 100},
                                   {70, 140, 225}, {80, 90, 190},  {140, 90, 190}};
  double cx = surface->w * (0.25 + 0.5 * ((seed >> 5) % 100) / 100.0);
  double base_y = surface->h * 0.58, radius = scale * 0.55;
  double reveal = smooth_step(0.0, 0.35, progress);
  int thick = scale / 120;
  if (thick < 3) thick = 3;
  const int segments = 40;
  for (int pass = 0; pass < 2; pass++) {
    for (int b = 0; b < 7; b++) {
      double r = pass == 0 ? radius - b * thick : radius * 1.25 + b * thick * 0.8;
      const Uint8 *c = band[pass == 0 ? b : 6 - b];
      double px = 0.0, py = 0.0;
      for (int k = 0; k <= segments; k++) {
        double u = reveal * k / segments;
        double a = SCENE_PI + u * SCENE_PI;
        double nx = cx + cos(a) * r, ny = base_y + sin(a) * r * 0.9;
        if (k > 0) {
          thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, thick, c[0], c[1], c[2],
                        (Uint8)(alpha * (pass == 0 ? 0.42 : 0.18)));
        }
        px = nx;
        py = ny;
      }
    }
  }
}


// A sun halo: a pale ring with two bright sundogs on either side and a faint
// light pillar.
static void sky_event_57(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 warm[3] = {255, 240, 200};
  double cx = surface->w * (0.3 + 0.4 * ((seed >> 5) % 100) / 100.0);
  double R = scale * 0.2;
  double shimmer = 0.85 + 0.15 * sin(phase / 400.0);
  weapon_glow(surface, cx, center_y, size * 0.8, warm, 235.0 * fade);
  for (int k = 0; k < 3; k++) {
    circleRGBA(surface, (int)cx, center_y, (int)R + k, 255, 250, 225,
               weapon_alpha(70.0 * fade * shimmer - k * 18));
  }
  circleRGBA(surface, (int)cx, center_y, (int)R - 2, 255, 190, 170,
             weapon_alpha(40.0 * fade * shimmer));
  thickLineRGBA(surface, (int)cx, (int)(center_y - R * 1.4), (int)cx, (int)(center_y + R * 1.4), 2,
                255, 245, 220, weapon_alpha(30.0 * fade));
  for (int side = -1; side <= 1; side += 2) {
    double dx = cx + side * R * 1.02;
    weapon_glow(surface, dx, center_y, size * 0.5, warm, 200.0 * fade * shimmer);
    for (int q = 0; q < 6; q++) {
      double f = q / 5.0;
      Uint8 cr = (Uint8)(255 - 85 * f), cg = (Uint8)(150 + 50 * f), cb = (Uint8)(120 + 135 * f);
      thickLineRGBA(surface, (int)(dx + side * q * size * 0.5), center_y,
                    (int)(dx + side * (q + 1) * size * 0.5), center_y, 2, cr, cg, cb,
                    weapon_alpha(110.0 * fade * (1.0 - f * 0.8)));
    }
  }
}


static double sky_track_x(const SkyEvent *event, double progress) {
  double margin = event->scale / 12.0;
  double span = event->surface->w + 2.0 * margin;
  return event->direction > 0 ? -margin + progress * span
                              : event->surface->w + margin - progress * span;
}


// A small plane flies over and skydivers drop out, free-fall and open their
// canopies.
static void sky_event_58(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 canopy[4][3] = {
      {190, 80, 70}, {80, 130, 190}, {220, 200, 90}, {110, 170, 110}};
  filledEllipseRGBA(surface, x, y, size * 7 / 5, size * 7 / 20 + 1, 30, 34, 44, alpha);
  thickLineRGBA(surface, x - direction * size / 5, y, x + direction * size / 5, y - size / 2, 2, 30,
                34, 44, alpha);
  filledTrigonRGBA(surface, x - direction * size, y, x - direction * size * 3 / 2, y - size / 2,
                   x - direction * size * 6 / 5, y, 30, 34, 44, alpha);
  thickLineRGBA(surface, x + direction * size * 3 / 2, y - size / 3, x + direction * size * 3 / 2,
                y + size / 3, 1, 190, 200, 210, (Uint8)(alpha * 0.6));
  for (int j = 0; j < 5; j++) {
    double drop = 0.2 + 0.07 * j;
    double dp = progress - drop;
    if (dp < 0.0) continue;
    double jx = sky_track_x(event, drop) + direction * scale * 0.05 * (1.0 - exp(-dp * 12.0));
    double fall =
        dp < 0.1 ? dp * dp * scale * 10.0 : 0.1 * 0.1 * scale * 10.0 + (dp - 0.1) * scale * 0.35;
    double jy = y + size / 3 + fall;
    if (dp < 0.1) {
      thickLineRGBA(surface, (int)jx, (int)jy, (int)(jx + direction * size / 2),
                    (int)(jy + size / 5), 2, 30, 34, 44, alpha);
      filledCircleRGBA(surface, (int)jx, (int)jy, 2, 30, 34, 44, alpha);
    } else {
      jx += sin(dp * 10.0 + j) * size * 0.3;
      const Uint8 *c = canopy[j % 4];
      int cr = size * 4 / 5 + 2;
      double open = smooth_step(0.1, 0.13, dp);
      int radius = (int)(cr * open) + 1;
      filledPieRGBA(surface, (int)jx, (int)jy - cr * 2, radius, 180, 360, c[0], c[1], c[2], alpha);
      lineRGBA(surface, (int)jx - radius, (int)jy - cr * 2, (int)jx, (int)jy, 30, 34, 44,
               (Uint8)(alpha * 0.7));
      lineRGBA(surface, (int)jx + radius, (int)jy - cr * 2, (int)jx, (int)jy, 30, 34, 44,
               (Uint8)(alpha * 0.7));
      filledCircleRGBA(surface, (int)jx, (int)jy, 2, 30, 34, 44, alpha);
    }
  }
}


// A flying carpet glides by with a turbaned rider and a trail of golden
// sparkles.
static void sky_event_59(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 gold[3] = {255, 230, 150};
  const int seg = 10;
  double half = size * 2.2, wave = phase / 220.0;
  int prev_x = 0, prev_y = 0, rider_x = x, rider_y = y;
  for (int i = 0; i <= seg; i++) {
    double u = (double)i / seg;
    int px = x + (int)(direction * (u - 0.5) * 2.0 * half);
    int py = y + (int)(sin(wave - u * 5.0) * size * 0.35);
    int thickness = size * 7 / 20 + 2;
    if (i > 0) {
      Uint8 cr = i % 2 ? 140 : 110, cg = i % 2 ? 50 : 40, cb = i % 2 ? 60 : 70;
      filledTrigonRGBA(surface, prev_x, prev_y, px, py, prev_x, prev_y + thickness, cr, cg, cb,
                       alpha);
      filledTrigonRGBA(surface, px, py, px, py + thickness, prev_x, prev_y + thickness, cr, cg, cb,
                       alpha);
      lineRGBA(surface, prev_x, prev_y, px, py, 210, 170, 90, alpha);
      lineRGBA(surface, prev_x, prev_y + thickness, px, py + thickness, 210, 170, 90, alpha);
    }
    if (i == 4) {
      rider_x = px;
      rider_y = py;
    }
    prev_x = px;
    prev_y = py;
  }
  for (int fringe = 0; fringe < 4; fringe++) {
    int fy = prev_y + fringe * size / 8;
    lineRGBA(surface, prev_x, fy, prev_x + direction * size / 4, fy + size / 8, 210, 170, 90,
             alpha);
  }
  filledCircleRGBA(surface, rider_x, rider_y - size * 7 / 10, size / 5 + 1, 30, 24, 30, alpha);
  filledCircleRGBA(surface, rider_x, rider_y - size * 4 / 5, size / 4, 220, 215, 200, alpha);
  filledTrigonRGBA(surface, rider_x - size / 4, rider_y, rider_x + size / 4, rider_y, rider_x,
                   rider_y - size * 6 / 10, 40, 36, 50, alpha);
  for (int k = 0; k < 12; k++) {
    double age = k / 12.0;
    double tx = x - direction * (half + age * size * 7.0),
           ty = y + sin(wave * 0.8 + k) * size * 0.5 + age * size;
    weapon_glow(surface, tx, ty, 1.6, gold, alpha * (1.0 - age) * 1.4);
  }
}


// A glowing double helix of DNA turns as it flies across the sky.
static void sky_event_60(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  const int pairs = 22;
  double spin = phase / 380.0;
  int prev_a = 0, prev_b = 0, prev_x = 0;
  for (int i = 0; i < pairs; i++) {
    int ax = x - direction * i * size * 11 / 20;
    double s_ = sin(spin + i * 0.5), c_ = cos(spin + i * 0.5);
    int ya = y + (int)(s_ * size * 1.1), yb = y - (int)(s_ * size * 1.1);
    double tail = 1.0 - 0.6 * i / pairs;
    lineRGBA(surface, ax, ya, ax, yb, 200, 210, 240, (Uint8)(alpha * 0.35 * tail));
    if (i > 0) {
      lineRGBA(surface, prev_x, prev_a, ax, ya, 255, 140, 170, (Uint8)(alpha * 0.7 * tail));
      lineRGBA(surface, prev_x, prev_b, ax, yb, 130, 220, 255, (Uint8)(alpha * 0.7 * tail));
    }
    filledCircleRGBA(surface, ax, ya, c_ > 0 ? 3 : 2, 255, 140, 170, (Uint8)(alpha * tail));
    filledCircleRGBA(surface, ax, yb, c_ < 0 ? 3 : 2, 130, 220, 255, (Uint8)(alpha * tail));
    prev_x = ax;
    prev_a = ya;
    prev_b = yb;
  }
}


// A single huge snowflake spins slowly as it sails down across the sky.
static void sky_event_61(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 ice[3] = {215, 238, 255};
  double R = size * 2.6;
  double fy = y + progress * scale * 0.15;
  double rot = phase / 2600.0 * direction;
  for (int arm = 0; arm < 6; arm++) {
    double a = rot + arm * SCENE_PI / 3.0;
    double ux = cos(a), uy = sin(a);
    thickLineRGBA(surface, x, (int)fy, (int)(x + ux * R), (int)(fy + uy * R), 2, ice[0], ice[1],
                  ice[2], alpha);
    for (int b = 0; b < 2; b++) {
      double at = b == 0 ? 0.45 : 0.75, len = R * (b == 0 ? 0.3 : 0.2);
      double bx = x + ux * R * at, by = fy + uy * R * at;
      for (int side = -1; side <= 1; side += 2) {
        double ba = a + side * 0.8;
        lineRGBA(surface, (int)bx, (int)by, (int)(bx + cos(ba) * len), (int)(by + sin(ba) * len),
                 ice[0], ice[1], ice[2], alpha);
      }
    }
  }
  weapon_glow(surface, x, fy, size * 0.35, ice, alpha * 1.3);
  int sparkle = (int)(phase / 300) % 6;
  double sa = rot + sparkle * SCENE_PI / 3.0;
  weapon_glow(surface, x + cos(sa) * R, fy + sin(sa) * R, 2.0, ice,
              alpha * (0.5 + 0.5 * sin(phase / 90.0)));
}


// A pulsar: a small star whose twin beams sweep round like a lighthouse, with
// pulse rings.
static void sky_event_62(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 blue[3] = {150, 200, 255};
  double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
  double length = scale * 0.35, spin = phase / 420.0;
  for (int beam = 0; beam < 2; beam++) {
    double a = spin + beam * SCENE_PI;
    double ex = cx + cos(a) * length, ey = center_y + sin(a) * length;
    double wx = -sin(a) * size * 0.35, wy = cos(a) * size * 0.35;
    filledTrigonRGBA(surface, (int)cx, center_y, (int)(ex + wx), (int)(ey + wy), (int)(ex - wx),
                     (int)(ey - wy), blue[0], blue[1], blue[2], (Uint8)(alpha * 0.18));
    lineRGBA(surface, (int)cx, center_y, (int)ex, (int)ey, 220, 235, 255, (Uint8)(alpha * 0.5));
  }
  double pulse = 0.5 + 0.5 * sin(spin * 3.0);
  weapon_glow(surface, cx, center_y, size * 0.45 * (1.0 + 0.2 * pulse), blue, 235.0 * fade);
  double ring = (phase % 1500) / 1500.0;
  circleRGBA(surface, (int)cx, center_y, (int)(ring * size * 3.0), blue[0], blue[1], blue[2],
             weapon_alpha(alpha * (1.0 - ring)));
}


// A swirling portal opens in the sky, throws sparks and shoots out a bright
// streak before it closes.
static void sky_event_63(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 swirl[4][3] = {
      {150, 90, 255}, {120, 140, 255}, {100, 200, 255}, {190, 230, 255}};
  double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
  double open = smooth_step(0.0, 0.2, progress) * (1.0 - smooth_step(0.8, 1.0, progress));
  double R = size * 2.4 * open;
  if (R < 2.0) return;
  double spin = phase / 500.0;
  filledEllipseRGBA(surface, (int)cx, center_y, (int)(R * 0.8), (int)R, 8, 6, 22,
                    weapon_alpha(alpha * 1.3));
  for (int k = 0; k < 4; k++) {
    double r = R * (1.0 - 0.16 * k);
    double dir = k % 2 ? 1.0 : -1.0;
    for (int seg = 0; seg < 16; seg++) {
      double a0 = dir * spin * (1.0 + 0.3 * k) + seg * SCENE_TAU / 16.0,
             a1 = a0 + SCENE_TAU / 16.0 * 0.6;
      thickLineRGBA(surface, (int)(cx + cos(a0) * r * 0.8), (int)(center_y + sin(a0) * r),
                    (int)(cx + cos(a1) * r * 0.8), (int)(center_y + sin(a1) * r), 2, swirl[k][0],
                    swirl[k][1], swirl[k][2], weapon_alpha(alpha * 1.3));
    }
  }
  ellipseRGBA(surface, (int)cx, center_y, (int)(R * 0.85), (int)(R * 1.05), 170, 120, 255,
              weapon_alpha(alpha * 0.8));
  for (int j = 0; j < 10; j++) {
    double u = fmod(phase / 1800.0 + j / 10.0, 1.0), a = j * 2.4;
    weapon_glow(surface, cx + cos(a) * R * 0.8 * (0.5 + u * 1.1),
                center_y + sin(a) * R * (0.5 + u * 1.1), 1.6, swirl[3], alpha * (1.0 - u) * open);
  }
  double shot = (progress - 0.45) / 0.2;
  if (shot > 0.0 && shot < 1.0) {
    double sx = cx + direction * shot * scale * 0.6, sy = center_y - shot * scale * 0.05;
    thickLineRGBA(surface, (int)(sx - direction * size * 3), (int)(sy + size * 0.3), (int)sx,
                  (int)sy, 2, 200, 230, 255, weapon_alpha(alpha * (1.0 - shot)));
    weapon_glow(surface, sx, sy, 2.5, swirl[3], 235.0 * fade * (1.0 - shot));
  }
}


// A ghostly sailing ship, half transparent, crosses the sky with its mist wake
// and a green lantern.
static void sky_event_64(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 lantern[3] = {150, 255, 200};
  double flicker = 0.8 + 0.2 * sin(phase / 170.0);
  Uint8 a = weapon_alpha(alpha * 0.8 * flicker);
  int by = y + (int)(sin(phase / 900.0) * size * 0.25);
// Hull and masts are placed in units of `size` along the heading.
#define GHOST_X(u) (x + direction * (int)((u) * size))
#define GHOST_Y(v) (by + (int)((v) * size))
  filledTrigonRGBA(surface, GHOST_X(-3.4), GHOST_Y(-0.6), GHOST_X(3.4), GHOST_Y(-0.3), GHOST_X(2.4),
                   GHOST_Y(1.0), 110, 140, 160, a);
  filledTrigonRGBA(surface, GHOST_X(-3.4), GHOST_Y(-0.6), GHOST_X(2.4), GHOST_Y(1.0), GHOST_X(-2.4),
                   GHOST_Y(1.0), 110, 140, 160, a);
  lineRGBA(surface, GHOST_X(-3.4), GHOST_Y(-0.6), GHOST_X(3.4), GHOST_Y(-0.3), 190, 215, 225, a);
  thickLineRGBA(surface, GHOST_X(3.4), GHOST_Y(-0.3), GHOST_X(4.8), GHOST_Y(-1.0), 1, 190, 215, 225,
                a);
  for (int p = 0; p < 4; p++) {
    filledCircleRGBA(surface, GHOST_X(-1.6 + p * 1.1), GHOST_Y(0.2), 1, 170, 255, 220, a);
  }
  static const double mast_u[3] = {-1.8, 0.2, 2.0};
  static const double mast_h[3] = {3.4, 4.4, 3.4};
  for (int m = 0; m < 3; m++) {
    thickLineRGBA(surface, GHOST_X(mast_u[m]), GHOST_Y(-0.4), GHOST_X(mast_u[m]),
                  GHOST_Y(-mast_h[m]), 1, 190, 215, 225, a);
    for (int sail = 0; sail < 2; sail++) {
      double top = -mast_h[m] + 0.3 + sail * mast_h[m] * 0.45, bottom = top + mast_h[m] * 0.4;
      Uint8 sa = weapon_alpha(alpha * 0.4 * flicker);
      filledTrigonRGBA(surface, GHOST_X(mast_u[m]), GHOST_Y(top), GHOST_X(mast_u[m]),
                       GHOST_Y(bottom), GHOST_X(mast_u[m] + 1.3), GHOST_Y((top + bottom) / 2.0),
                       200, 215, 230, sa);
    }
  }
  weapon_glow(surface, GHOST_X(-3.2), GHOST_Y(-0.9), 2.0, lantern, alpha * 1.5 * flicker);
  for (int puff = 0; puff < 6; puff++) {
    double age = fmod(phase / 2200.0 + puff / 6.0, 1.0);
    filledCircleRGBA(surface, GHOST_X(-3.4 - age * 4.0), GHOST_Y(0.6 - age * 0.6),
                     (int)(size * (0.2 + 0.4 * age)), 200, 215, 225,
                     weapon_alpha(alpha * 0.35 * (1.0 - age)));
  }
#undef GHOST_X
#undef GHOST_Y
}


// Paper airplanes glide across the sky in a loose group, banking as they go.
static void sky_event_65(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int plane = 0; plane < 5; plane++) {
    double bank = sin(phase / 700.0 + plane * 1.3);
    int px = x - direction * (plane * size * 3 + (plane % 2) * size);
    int py = y + (int)(sin(phase / 900.0 + plane * 2.1) * size * 1.5) + plane * size;
    int nose_x = px + direction * size, tail_x = px - direction * size;
    int wing = (int)(size * (0.5 + 0.35 * fabs(bank)));
    int fold = (int)(bank * size * 0.25);
    filledTrigonRGBA(surface, nose_x, py, tail_x, py - wing, tail_x, py + fold, 238, 240, 244,
                     alpha);
    filledTrigonRGBA(surface, nose_x, py, tail_x, py + wing, tail_x, py + fold, 188, 196, 212,
                     alpha);
    lineRGBA(surface, nose_x, py, tail_x, py + fold, 110, 120, 140, alpha);
    for (int dot = 1; dot <= 5; dot++) {
      pixelRGBA(surface, tail_x - direction * dot * size / 2, py + fold, 220, 228, 240,
                (Uint8)(alpha * (1.0 - dot / 6.0)));
    }
  }
}


// Soap bubbles drift by: thin iridescent rings, each with a bright glint.
static void sky_event_66(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int bubble = 0; bubble < 8; bubble++) {
    int r = size * (4 + bubble % 3) / 6 + 3;
    int bx = x - direction * bubble * size * 3 / 2 + (int)(sin(phase / 800.0 + bubble) * size);
    int by = y + bubble * size / 2 + (int)(sin(phase / 600.0 + bubble * 1.7) * size);
    circleRGBA(surface, bx, by, r, 255, 140, 220, (Uint8)(alpha * 0.55));
    circleRGBA(surface, bx, by, r - 1, 120, 230, 255, (Uint8)(alpha * 0.45));
    circleRGBA(surface, bx, by, r - 2, 255, 240, 140, (Uint8)(alpha * 0.25));
    filledCircleRGBA(surface, bx, by, r - 2, 200, 220, 255, (Uint8)(alpha * 0.08));
    filledCircleRGBA(surface, bx - r / 3, by - r / 3, r / 6 + 1, 255, 255, 255, alpha);
  }
}


// A big banded ringed planet slides slowly across the sky.
static void sky_event_67(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size * 2;
  int ring_w = r * 22 / 10, ring_h = r * 6 / 10;
  ellipseRGBA(surface, x, y, ring_w, ring_h, 210, 190, 150, (Uint8)(alpha * 0.55));
  ellipseRGBA(surface, x, y, ring_w - 3, ring_h - 1, 190, 170, 130, (Uint8)(alpha * 0.4));
  ellipseRGBA(surface, x, y, ring_w * 8 / 10, ring_h * 8 / 10, 220, 200, 160,
              (Uint8)(alpha * 0.35));
  filledCircleRGBA(surface, x, y, r, 150, 118, 92, alpha);
  for (int band = -3; band <= 3; band++) {
    int by = y + band * r / 4;
    double dy = (double)(by - y) / r;
    int half = (int)(r * sqrt(1.0 - dy * dy));
    lineRGBA(surface, x - half, by, x + half, by, band % 2 ? 190 : 120, band % 2 ? 150 : 92,
             band % 2 ? 110 : 70, (Uint8)(alpha * 0.6));
  }
  filledCircleRGBA(surface, x - r / 3, y - r / 3, r / 2, 255, 235, 200, (Uint8)(alpha * 0.12));
  for (int step = 0; step < 40; step++) {
    double a0 = SCENE_PI * step / 40.0, a1 = SCENE_PI * (step + 1) / 40.0;
    lineRGBA(surface, x + (int)(cos(a0) * ring_w), y + (int)(sin(a0) * ring_h),
             x + (int)(cos(a1) * ring_w), y + (int)(sin(a1) * ring_h), 215, 195, 155,
             (Uint8)(alpha * 0.8));
  }
}


// A constellation draws itself: stars light up and thin lines join them one by
// one.
static void sky_event_68(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const double pts[7][2] = {{-3, 0},  {-1.5, -1.5}, {0, -0.4},  {1.8, -1.8},
                                   {3, 0.3}, {1.4, 1.6},   {-0.8, 1.4}};
  static const Uint8 star[3] = {235, 240, 255};
  double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
  int u = size * 2;
  double draw = smooth_step(0.1, 0.7, progress) * 6.0;
  for (int i = 0; i < 7; i++) {
    double sx = cx + pts[i][0] * u, sy = y + pts[i][1] * u;
    if (i < 6) {
      double f = fmin(draw - i, 1.0);
      if (f > 0.0) {
        double nx = cx + pts[i + 1][0] * u, ny = y + pts[i + 1][1] * u;
        lineRGBA(surface, (int)sx, (int)sy, (int)(sx + (nx - sx) * f), (int)(sy + (ny - sy) * f),
                 170, 195, 235, (Uint8)(alpha * 0.7));
      }
    }
    double lit = smooth_step(i * 0.08, i * 0.08 + 0.1, progress);
    weapon_glow(surface, sx, sy, 1.5 + (i % 3) * 0.5, star,
                235.0 * fade * lit * (0.7 + 0.3 * sin(phase / 300.0 + i)));
  }
}


// A flock of folded paper cranes flaps slowly across the sky.
static void sky_event_69(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int crane = 0; crane < 6; crane++) {
    int cx = x - direction * (crane * size * 2 + (crane % 2) * size);
    int cy = y + (int)(sin(phase / 800.0 + crane * 1.6) * size) + (crane % 3) * size;
    double flap = sin(phase / 260.0 + crane * 0.9);
    int wing = (int)(size * (0.8 + 0.4 * flap));
    Uint8 tint = (Uint8)(215 + crane * 6);
    filledTrigonRGBA(surface, cx, cy, cx - direction * size, cy - wing, cx - direction * size / 3,
                     cy, tint, 228, 232, alpha);
    filledTrigonRGBA(surface, cx, cy, cx - direction * size, cy + wing / 2,
                     cx - direction * size / 3, cy, 175, 185, 205, alpha);
    thickLineRGBA(surface, cx, cy, cx + direction * size, cy - size / 3, 1, 235, 120, 120, alpha);
    thickLineRGBA(surface, cx, cy, cx - direction * size * 3 / 2, cy + size / 4, 1, 215, 222, 232,
                  alpha);
  }
}


// A string of glowing sky lanterns rises and drifts, each pulsing softly.
static void sky_event_70(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 warm[3] = {255, 170, 80};
  for (int lantern = 0; lantern < 9; lantern++) {
    double rise = progress * size * 3.0 + lantern * size * 0.6;
    double lx = x - direction * lantern * size * 2 + sin(phase / 900.0 + lantern) * size * 0.8;
    double ly = y + (lantern % 3) * size * 1.5 - rise * 0.4;
    double pulse = 0.7 + 0.3 * sin(phase / 250.0 + lantern * 1.7);
    weapon_glow(surface, lx, ly, size * 0.7, warm, 200.0 * fade * pulse);
    boxRGBA(surface, (int)lx - 2, (int)ly - 3, (int)lx + 2, (int)ly + 3, 255, 200, 120,
            (Uint8)(alpha * pulse));
  }
}


// A big owl glides silently across the moonless sky, wings spread wide.
static void sky_event_71(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int by = y + (int)(sin(phase / 1100.0) * size * 0.6);
  double beat = sin(phase / 420.0);
  int span = size * 3, lift = (int)(beat * size * 0.8);
  for (int side = -1; side <= 1; side += 2) {
    int tip_x = x + side * span / 2 * 1, tip_y = by - lift;
    filledTrigonRGBA(surface, x, by - size / 4, tip_x, tip_y, x + side * size / 3, by + size / 3,
                     28, 24, 30, alpha);
    filledTrigonRGBA(surface, x, by - size / 4, tip_x, tip_y, x + side * span / 4,
                     by - lift / 2 - size / 2, 40, 34, 40, alpha);
  }
  filledEllipseRGBA(surface, x, by, size / 2 + 1, size * 7 / 10, 36, 30, 34, alpha);
  filledCircleRGBA(surface, x, by - size * 3 / 4, size / 2, 44, 38, 42, alpha);
  filledTrigonRGBA(surface, x - size / 3, by - size, x - size / 6, by - size * 5 / 4, x - size / 10,
                   by - size * 6 / 5, 44, 38, 42, alpha);
  filledTrigonRGBA(surface, x + size / 3, by - size, x + size / 6, by - size * 5 / 4, x + size / 10,
                   by - size * 6 / 5, 44, 38, 42, alpha);
  filledCircleRGBA(surface, x - size / 5, by - size * 3 / 4, 2, 255, 210, 90, alpha);
  filledCircleRGBA(surface, x + size / 5, by - size * 3 / 4, 2, 255, 210, 90, alpha);
}


// A gust lifts a swirl of leaves that spiral across the sky and settle.
static void sky_event_72(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int leaf = 0; leaf < 16; leaf++) {
    double u = leaf / 16.0;
    double spin = phase / 400.0 + leaf * 0.9;
    double lx = x - direction * (leaf * size * 0.9) + cos(spin) * size * (1.0 + u * 2.0);
    double ly = y + sin(spin) * size * (0.6 + u) + u * size * 3.0;
    double turn = spin * 1.7;
    Uint8 r = (Uint8)(150 + (leaf * 17) % 80), g = (Uint8)(70 + (leaf * 29) % 70);
    filledEllipseRGBA(surface, (int)lx, (int)ly, (int)(2 + fabs(cos(turn)) * size * 0.4),
                      (int)(1 + fabs(sin(turn)) * size * 0.3), r, g, 40, alpha);
  }
}


// A murmuration: a dense cloud of small birds twists and swells as it moves.
static void sky_event_73(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int bird = 0; bird < 70; bird++) {
    unsigned int h = fx_hash(seed, bird);
    double a = (h % 6283) / 1000.0, r = ((h >> 12) % 100) / 100.0;
    double shape = 1.0 + 0.5 * sin(phase / 700.0 + a * 2.0);
    double bx = x + cos(a + phase / 1500.0) * size * 4.0 * r * shape;
    double by = y + sin(a + phase / 1500.0) * size * 1.6 * r * (2.0 - shape) +
                sin(phase / 300.0 + bird) * 2.0;
    double flap = sin(phase / 90.0 + bird);
    thickLineRGBA(surface, (int)bx - 2, (int)(by + flap * 1.5), (int)bx, (int)by, 1, 14, 18, 28,
                  alpha);
    thickLineRGBA(surface, (int)bx, (int)by, (int)bx + 2, (int)(by + flap * 1.5), 1, 14, 18, 28,
                  alpha);
  }
}


// A single bright aurora curtain unfurls, hangs rippling and folds away.
static void sky_event_74(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double open = smooth_step(0.0, 0.25, progress) * (1.0 - smooth_step(0.75, 1.0, progress));
  const int columns = 36;
  double left = surface->w * 0.1, width = surface->w * 0.8;
  for (int c = 0; c < columns; c++) {
    double u = (double)c / columns;
    double top = surface->h * 0.08 + sin(u * 7.0 + phase / 900.0) * scale * 0.03;
    double length = scale * (0.12 + 0.08 * sin(u * 11.0 - phase / 600.0)) * open;
    double cx = left + width * u;
    Uint8 g = (Uint8)(190 + 60 * sin(u * 5.0)), b = (Uint8)(150 + 90 * u);
    for (int seg = 0; seg < 4; seg++) {
      double f0 = seg / 4.0, f1 = (seg + 1) / 4.0;
      thickLineRGBA(surface, (int)cx, (int)(top + length * f0), (int)cx, (int)(top + length * f1),
                    3, 90, g, b, weapon_alpha(alpha * (0.55 - 0.12 * seg) * 0.5));
    }
  }
}


// A great comet with a bright head and a two-part tail: a straight blue ion
// tail and a curved golden dust tail.
static void sky_event_75(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 head[3] = {235, 245, 255};
  int len = size * 12;
  for (int k = 0; k < 24; k++) {
    double u = k / 24.0;
    double ix = x - direction * u * len, iy = y - u * len * 0.18;
    double dx = x - direction * u * len * 0.8, dy = y + u * u * len * 0.35;
    filledCircleRGBA(surface, (int)ix, (int)iy, (int)(size * 0.25 * (1.0 - u)) + 1, 120, 180, 255,
                     weapon_alpha(alpha * 0.5 * (1.0 - u)));
    filledCircleRGBA(surface, (int)dx, (int)dy, (int)(size * 0.45 * (0.4 + u * 0.6)) + 1, 255, 215,
                     140, weapon_alpha(alpha * 0.28 * (1.0 - u)));
  }
  weapon_glow(surface, x, y, size * 0.8, head, 235.0 * fade);
}


// A small flock of pterosaurs with long head crests glides past on wide, slow
// wing beats.
static void sky_event_76(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int p = 0; p < 4; p++) {
    int px = x - direction * p * size * 4,
        py = y + (p % 2) * size * 2 + (int)(sin(phase / 1000.0 + p) * size * 0.6);
    double beat = sin(phase / 520.0 + p * 0.8);
    int span = size * 2, lift = (int)(beat * size);
    for (int side = -1; side <= 1; side += 2) {
      int elbow_x = px + side * span / 2, elbow_y = py - lift / 2 - size / 3;
      int tip_x = px + side * span, tip_y = py + lift / 2;
      thickLineRGBA(surface, px, py, elbow_x, elbow_y, 2, 36, 30, 40, alpha);
      thickLineRGBA(surface, elbow_x, elbow_y, tip_x, tip_y, 2, 36, 30, 40, alpha);
      filledTrigonRGBA(surface, px, py, elbow_x, elbow_y, tip_x, tip_y, 50, 42, 52,
                       (Uint8)(alpha * 0.8));
    }
    filledEllipseRGBA(surface, px, py, size / 2, size / 5 + 1, 42, 36, 46, alpha);
    thickLineRGBA(surface, px + direction * size / 2, py, px + direction * size, py - size / 6, 2,
                  42, 36, 46, alpha);
    thickLineRGBA(surface, px + direction * size / 2, py - size / 8, px - direction * size / 4,
                  py - size / 2, 1, 150, 70, 60, alpha);
    thickLineRGBA(surface, px - direction * size / 2, py, px - direction * size, py + size / 5, 1,
                  36, 30, 40, alpha);
  }
}


// A swarm of luminous butterflies flutters across the sky in a loose, wandering
// cloud.
static void sky_event_77(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 tints[3][3] = {{150, 220, 255}, {255, 170, 220}, {190, 255, 190}};
  for (int b = 0; b < 14; b++) {
    unsigned int h = fx_hash(seed, b);
    double bx = x - direction * (h % 7) * size * 1.4 + sin(phase / 700.0 + b * 1.3) * size * 1.5;
    double by = y + ((h >> 8) % 7) * size * 0.6 + sin(phase / 530.0 + b * 2.1) * size;
    double flap = fabs(sin(phase / 110.0 + b * 0.7));
    int wing = (int)(1 + flap * size * 0.5);
    const Uint8 *c = tints[b % 3];
    filledEllipseRGBA(surface, (int)bx - wing, (int)by, wing, wing * 2 / 3 + 1, c[0], c[1], c[2],
                      (Uint8)(alpha * 0.8));
    filledEllipseRGBA(surface, (int)bx + wing, (int)by, wing, wing * 2 / 3 + 1, c[0], c[1], c[2],
                      (Uint8)(alpha * 0.8));
    weapon_glow(surface, bx, by, 1.2, c, 120.0 * fade);
  }
}


// A zeppelin cruises by sweeping a searchlight beam below it, its fins and
// gondola lit.
static void sky_event_78(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int len = size * 7, h = size * 3 / 2;
  int zy = y + (int)(sin(phase / 1200.0) * size * 0.5);
  double aim = 1.2 + sin(phase / 800.0) * 0.5;
  int lx = x + direction * size, ly = zy + h;
  double reach = scale * 0.5;
  filledTrigonRGBA(surface, lx, ly, (int)(lx + direction * cos(aim - 0.07) * reach),
                   (int)(ly + sin(aim - 0.07) * reach),
                   (int)(lx + direction * cos(aim + 0.07) * reach),
                   (int)(ly + sin(aim + 0.07) * reach), 255, 250, 215, (Uint8)(alpha * 0.2));
  filledEllipseRGBA(surface, x, zy, len / 2, h, 120, 126, 138, alpha);
  filledEllipseRGBA(surface, x, zy - h / 4, len / 2 - 2, h * 2 / 3, 160, 166, 178,
                    (Uint8)(alpha * 0.8));
  for (int fin = -1; fin <= 1; fin += 2) {
    filledTrigonRGBA(surface, x - direction * len / 2, zy, x - direction * (len / 2 + size),
                     zy + fin * h, x - direction * len / 3, zy + fin * h / 2, 96, 102, 114, alpha);
  }
  boxRGBA(surface, x + direction * size - size / 2, zy + h - 2, x + direction * size + size / 2,
          zy + h + size / 3, 70, 74, 88, alpha);
  if ((phase / 500) % 2 == 0)
    filledCircleRGBA(surface, x - direction * len / 2, zy, 2, 255, 60, 50, alpha);
}


// Five small saucers hold a V formation, wheel together and wink their rim
// lights.
static void sky_event_79(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int u = 0; u < 5; u++) {
    int row = (u + 1) / 2, side = u == 0 ? 0 : (u % 2 ? -1 : 1);
    int ux = x - direction * row * size * 3;
    int uy = y + side * row * size * 2 + (int)(sin(phase / 600.0 + row) * size * 0.4);
    filledEllipseRGBA(surface, ux, uy, size, size / 3 + 1, 120, 132, 150, alpha);
    filledEllipseRGBA(surface, ux, uy - size / 4, size / 2, size / 3 + 1, 160, 215, 230,
                      (Uint8)(alpha * 0.9));
    for (int light = -1; light <= 1; light++) {
      if (((phase / 160) + u + light + 3) % 3 == 0)
        filledCircleRGBA(surface, ux + light * size * 2 / 3, uy + size / 5, 1, 255, 240, 160,
                         alpha);
    }
  }
}


// A ball of plasma drifts across the sky throwing crackling arcs of violet
// light.
static void sky_event_80(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 violet[3] = {190, 140, 255};
  weapon_glow(surface, x, y, size * 1.1, violet, 220.0 * fade);
  unsigned int tick = phase / 90;
  for (int arc = 0; arc < 7; arc++) {
    double a = fx_hash(seed + arc, tick) % 6283 / 1000.0;
    double len = size * (1.5 + (fx_hash(seed, tick + arc * 7) % 100) / 60.0);
    double px = x, py = y;
    for (int seg = 1; seg <= 5; seg++) {
      double f = seg / 5.0;
      double nx =
          x + cos(a) * len * f + ((int)(fx_hash(tick, arc * 9 + seg) % 7) - 3) * size * 0.12;
      double ny =
          y + sin(a) * len * f + ((int)(fx_hash(tick, arc * 11 + seg) % 7) - 3) * size * 0.12;
      thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 1, 235, 225, 255,
                    (Uint8)(alpha * (1.0 - f * 0.6)));
      px = nx;
      py = ny;
    }
  }
}


// Red sprites: faint crimson jellyfish-shaped flashes blink high above a
// distant storm.
static void sky_event_81(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double cx = surface->w * (0.2 + 0.6 * ((seed >> 5) % 100) / 100.0);
  for (int burst = 0; burst < 3; burst++) {
    unsigned int step = phase / 380 + burst * 5;
    double life = fmod(phase / 380.0 + burst * 0.37, 1.0);
    if (fx_hash(seed, step) % 3 == 0) continue;
    double bx = cx + ((int)(fx_hash(seed, step + 3) % 200) - 100) * size * 0.05;
    double flash = (1.0 - life) * (1.0 - life);
    for (int tendril = -3; tendril <= 3; tendril++) {
      double top = y - size * 2.2 + abs(tendril) * size * 0.2;
      thickLineRGBA(surface, (int)(bx + tendril * size * 0.35), (int)top,
                    (int)(bx + tendril * size * 0.2), (int)(top + size * 2.0 * flash + size * 0.4),
                    1, 255, 70, 80, weapon_alpha(alpha * 1.3 * flash));
    }
    filledEllipseRGBA(surface, (int)bx, (int)(y - size * 2.2), (int)(size * 1.5),
                      (int)(size * 0.35), 255, 70, 90, weapon_alpha(alpha * 0.5 * flash));
  }
}


// A modular space station crosses the sky: truss, habitat tubes, big solar
// wings catching the light.
static void sky_event_82(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int tilt = (int)(sin(phase / 1500.0) * size * 0.3);
  thickLineRGBA(surface, x - size * 4, y + tilt, x + size * 4, y - tilt, 2, 150, 158, 170, alpha);
  for (int mod = -1; mod <= 1; mod++) {
    int mx = x + mod * size * 2;
    filledEllipseRGBA(surface, mx, y - mod * tilt / 4, size * 3 / 4, size / 3 + 1, 178, 184, 194,
                      alpha);
    thickLineRGBA(surface, mx, y - size * 3 / 4, mx, y + size * 3 / 4, 2, 120, 128, 140, alpha);
  }
  for (int side = -1; side <= 1; side += 2) {
    int wx = x + side * size * 4;
    boxRGBA(surface, wx - size / 4, y - size * 2, wx + size / 4, y + size * 2, 60, 105, 160, alpha);
    for (int cell = 1; cell < 4; cell++)
      lineRGBA(surface, wx - size / 4, y - size * 2 + cell * size, wx + size / 4,
               y - size * 2 + cell * size, 130, 180, 230, (Uint8)(alpha * 0.8));
  }
  double glint = 0.5 + 0.5 * sin(phase / 220.0);
  static const Uint8 white[3] = {250, 252, 255};
  weapon_glow(surface, x + size * 4, y - size, 1.8, white, 200.0 * fade * glint);
}


// A binary star: two suns, one gold and one blue, orbit each other and pass in
// front of one another.
static void sky_event_83(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 gold[3] = {255, 215, 130}, blue[3] = {150, 200, 255};
  double cx = surface->w * (0.25 + 0.5 * ((seed >> 5) % 100) / 100.0);
  double a = phase / 900.0, r = size * 1.6;
  double gx = cx + cos(a) * r, gy = y + sin(a) * r * 0.4;
  double bx = cx - cos(a) * r, by = y - sin(a) * r * 0.4;
  if (sin(a) >= 0.0) {
    weapon_glow(surface, bx, by, size * 0.55, blue, 235.0 * fade);
    weapon_glow(surface, gx, gy, size * 0.8, gold, 235.0 * fade);
  } else {
    weapon_glow(surface, gx, gy, size * 0.8, gold, 235.0 * fade);
    weapon_glow(surface, bx, by, size * 0.55, blue, 235.0 * fade);
  }
  ellipseRGBA(surface, (int)cx, y, (int)r, (int)(r * 0.4), 200, 210, 235, (Uint8)(alpha * 0.25));
}


// A hawk circles on still wings, then folds them and stoops in a steep dive.
static void sky_event_84(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double cx = surface->w * (0.25 + 0.5 * ((seed >> 5) % 100) / 100.0);
  double dive = smooth_step(0.6, 0.85, progress);
  double a = phase / 800.0;
  double hx = cx + cos(a) * size * 4.0 * (1.0 - dive) + dive * direction * size * 3.0;
  double hy = y + sin(a) * size * 1.2 * (1.0 - dive) + dive * dive * scale * 0.35;
  double span = size * (1.4 - 1.0 * dive);
  int dir = cos(a) > 0 ? -1 : 1;
  if (dive > 0.0) dir = direction;
  thickLineRGBA(surface, (int)(hx - span), (int)(hy + span * 0.15), (int)hx, (int)hy, 2, 40, 32, 30,
                alpha);
  thickLineRGBA(surface, (int)hx, (int)hy, (int)(hx + span), (int)(hy + span * 0.15), 2, 40, 32, 30,
                alpha);
  filledEllipseRGBA(surface, (int)hx, (int)hy, (int)(size * 0.45),
                    (int)(size * (0.2 + 0.4 * dive)) + 1, 52, 40, 36, alpha);
  filledTrigonRGBA(surface, (int)hx, (int)(hy + size * 0.3), (int)(hx - size * 0.2),
                   (int)(hy + size * 0.9), (int)(hx + size * 0.2), (int)(hy + size * 0.9), 70, 52,
                   44, alpha);
  filledCircleRGBA(surface, (int)(hx + dir * size * 0.4), (int)(hy - size * 0.1),
                   (int)(size * 0.17) + 1, 60, 46, 40, alpha);
}


// A bunch of escaped party balloons floats up and away on their trailing
// strings.
static void sky_event_85(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 colors[5][3] = {
      {230, 80, 90}, {240, 200, 80}, {90, 170, 230}, {130, 220, 140}, {200, 120, 230}};
  for (int b = 0; b < 7; b++) {
    double sway = sin(phase / 700.0 + b * 1.4);
    int bx = x - direction * (b % 4) * size + (int)(sway * size * 0.5);
    int by = y - (int)(progress * size * 2.0) - (b / 4) * size * 2 + (b % 3) * size / 2;
    const Uint8 *c = colors[b % 5];
    int r = size * 2 / 3 + 2;
    double sx = bx, sy = by + r;
    for (int seg = 1; seg <= 6; seg++) {
      double nx = bx - sway * size * 0.2 * seg * 0.3 + sin(phase / 300.0 + seg + b) * 2.0,
             ny = by + r + seg * size * 0.5;
      lineRGBA(surface, (int)sx, (int)sy, (int)nx, (int)ny, 200, 205, 215, (Uint8)(alpha * 0.7));
      sx = nx;
      sy = ny;
    }
    filledEllipseRGBA(surface, bx, by, r * 4 / 5, r, c[0], c[1], c[2], alpha);
    filledTrigonRGBA(surface, bx - 2, by + r, bx + 2, by + r, bx, by + r + 3, c[0], c[1], c[2],
                     alpha);
    filledEllipseRGBA(surface, bx - r / 3, by - r / 3, r / 5 + 1, r / 3 + 1, 255, 255, 255,
                      (Uint8)(alpha * 0.5));
  }
}


// A golden haze of pollen drifts through the sky, each grain glinting as it
// turns.
static void sky_event_86(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 gold[3] = {255, 225, 120};
  for (int g = 0; g < 40; g++) {
    unsigned int h = fx_hash(seed, g);
    double gx = x - direction * (h % 12) * size * 0.9 + sin(phase / 900.0 + g * 0.7) * size;
    double gy = y + ((h >> 8) % 10) * size * 0.5 + cos(phase / 1100.0 + g * 1.1) * size * 0.8;
    double glint = 0.5 + 0.5 * sin(phase / 200.0 + g * 2.0);
    filledCircleRGBA(surface, (int)gx, (int)gy, 1, gold[0], gold[1], gold[2],
                     (Uint8)(alpha * (0.5 + 0.5 * glint)));
    if (glint > 0.92) weapon_glow(surface, gx, gy, 1.5, gold, 180.0 * fade);
  }
}


// A serpent of pale mist winds across the sky, thickening in the middle, with
// two faint glowing eyes.
static void sky_event_87(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  const int segs = 20;
  for (int i = segs; i >= 0; i--) {
    double u = (double)i / segs;
    double sx = x - direction * i * size * 0.9;
    double sy = y + sin(phase / 500.0 - i * 0.45) * size * 1.2;
    double r = size * (0.3 + 0.9 * sin(3.14159 * (1.0 - u) * 0.9 + 0.1));
    filledCircleRGBA(surface, (int)sx, (int)sy, (int)r + 1, 200, 215, 235,
                     (Uint8)(alpha * 0.25 * (1.0 - u * 0.6)));
    filledCircleRGBA(surface, (int)sx, (int)(sy - r * 0.2), (int)(r * 0.6) + 1, 235, 242, 250,
                     (Uint8)(alpha * 0.18));
  }
  weapon_glow(surface, x + direction * size * 0.3, y - size * 0.15, 1.5,
              (const Uint8[]){255, 240, 170}, 220.0 * fade);
  weapon_glow(surface, x + direction * size * 0.3, y + size * 0.25, 1.5,
              (const Uint8[]){255, 240, 170}, 220.0 * fade);
}


// Two small sky-galleons with billowing sails sail past, lanterns glowing at
// their sterns.
static void sky_event_88(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 lamp[3] = {255, 200, 120};
  for (int ship = 0; ship < 2; ship++) {
    int sx = x - direction * ship * size * 7,
        sy = y + ship * size * 2 + (int)(sin(phase / 800.0 + ship * 2.0) * size * 0.5);
    int len = size * 3 - ship * size / 2;
    filledTrigonRGBA(surface, sx - len, sy, sx + len, sy - size / 3, sx + len * 3 / 4,
                     sy + size * 2 / 3, 70, 46, 40, alpha);
    filledTrigonRGBA(surface, sx - len, sy, sx + len * 3 / 4, sy + size * 2 / 3, sx - len * 3 / 4,
                     sy + size * 2 / 3, 70, 46, 40, alpha);
    for (int mast = 0; mast < 2; mast++) {
      int mx = sx - len / 3 + mast * len * 2 / 3;
      thickLineRGBA(surface, mx, sy, mx, sy - size * 3, 1, 50, 36, 32, alpha);
      double belly = sin(phase / 600.0 + mast + ship) * size * 0.2 + size * 0.7;
      filledTrigonRGBA(surface, mx, sy - size * 3 + 2, mx, sy - size / 2,
                       mx + direction * (int)belly, sy - size * 3 / 2, 235, 228, 210, alpha);
    }
    weapon_glow(surface, sx - direction * len, sy - size / 4, 1.8, lamp, 210.0 * fade);
    for (int puff = 0; puff < 3; puff++) {
      double u = fmod(phase / 1500.0 + puff / 3.0, 1.0);
      filledCircleRGBA(surface, (int)(sx - direction * (len + u * size * 3)), sy + size / 3,
                       (int)(size * 0.3 * (0.5 + u)) + 1, 215, 225, 240,
                       (Uint8)(alpha * 0.3 * (1.0 - u)));
    }
  }
}


// A pale jellyfish drifts through the air, its bell pulsing and its tentacles
// trailing behind.
static void sky_event_89(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 glow[3] = {190, 170, 255};
  double pulse = 0.5 + 0.5 * sin(phase / 420.0);
  int bw = (int)(size * (1.5 + 0.35 * pulse)), bh = (int)(size * (1.0 - 0.2 * pulse));
  for (int t = -3; t <= 3; t++) {
    double sx = x + t * bw / 4.0, sy = y + bh / 2;
    for (int seg = 1; seg <= 7; seg++) {
      double nx = sx - direction * size * 0.15 + sin(phase / 380.0 - seg * 0.9 + t) * size * 0.25,
             ny = sy + size * 0.5;
      lineRGBA(surface, (int)sx, (int)sy, (int)nx, (int)ny, 210, 195, 255,
               (Uint8)(alpha * 0.6 * (1.0 - seg / 9.0)));
      sx = nx;
      sy = ny;
    }
  }
  filledEllipseRGBA(surface, x, y, bw, bh, 200, 185, 250, (Uint8)(alpha * 0.55));
  filledEllipseRGBA(surface, x, y + bh / 4, bw * 2 / 3, bh / 2 + 1, 235, 225, 255,
                    (Uint8)(alpha * 0.4));
  weapon_glow(surface, x, y, size * 0.45, glow, 120.0 * fade);
}


// Two manta rays glide past, wingtips rippling, one a little behind and below
// the other.
static void sky_event_90(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int ray = 0; ray < 2; ray++) {
    double cx = x - direction * ray * size * 3.5,
           cy = y + ray * size * 1.6 + sin(phase / 900.0 + ray) * size * 0.4;
    double flap = sin(phase / 350.0 - ray * 0.8);
    for (int side = -1; side <= 1; side += 2) {
      double tipx = cx - direction * size * 0.4, tipy = cy + side * size * 1.8 - flap * size * 0.7;
      filledTrigonRGBA(surface, (int)(cx + direction * size * 0.8), (int)cy, (int)tipx, (int)tipy,
                       (int)(cx - direction * size * 0.4), (int)cy, 96, 116, 148, alpha);
    }
    filledEllipseRGBA(surface, (int)cx, (int)cy, size, size / 3 + 1, 110, 132, 164, alpha);
    filledEllipseRGBA(surface, (int)(cx + direction * size / 4), (int)(cy - 1), size / 2,
                      size / 6 + 1, 160, 180, 205, (Uint8)(alpha * 0.7));
    for (int seg = 1; seg <= 5; seg++) {
      lineRGBA(surface, (int)(cx - direction * (size * 0.8 + (seg - 1) * size * 0.4)),
               (int)(cy + sin(phase / 300.0 - seg) * 2),
               (int)(cx - direction * (size * 0.8 + seg * size * 0.4)),
               (int)(cy + sin(phase / 300.0 - seg - 1) * 2), 110, 132, 164, alpha);
    }
  }
}


// Perfect smoke rings roll across the sky, each one spreading and thinning as
// it goes.
static void sky_event_91(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 5; i++) {
    double u = fmod(phase / 4200.0 * 0.6 + i * 0.2, 1.0);
    double rx = x - direction * i * size * 2.2;
    double r = size * (0.5 + 1.1 * u), tilt = 0.45 + 0.1 * sin(phase / 700.0 + i);
    double a = alpha * 0.55 * (1.0 - u);
    ellipseRGBA(surface, (int)rx, y + (i % 2) * size, (int)(r * tilt) + 1, (int)r, 232, 232, 238,
                (Uint8)a);
    ellipseRGBA(surface, (int)rx, y + (i % 2) * size, (int)(r * tilt) + 2, (int)r + 1, 215, 218,
                228, (Uint8)(a * 0.6));
    ellipseRGBA(surface, (int)rx, y + (i % 2) * size, (int)(r * tilt), (int)r - 1, 200, 205, 218,
                (Uint8)(a * 0.4));
  }
}


// Three hang gliders in coloured wings drift down in a loose line, banking on
// the air.
static void sky_event_92(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 wing[3][3] = {{220, 90, 70}, {240, 190, 70}, {80, 150, 210}};
  for (int g = 0; g < 3; g++) {
    double gx = x - direction * g * size * 3.5,
           gy = y + g * size * 1.2 + sin(phase / 1100.0 + g * 1.7) * size * 0.4;
    double bank = sin(phase / 1300.0 + g * 2.0) * 0.25;
    int span = size * 2;
    int lx = (int)(gx - cos(bank) * span), ly = (int)(gy + sin(bank) * span * 0.5);
    int rx = (int)(gx + cos(bank) * span), ry = (int)(gy - sin(bank) * span * 0.5);
    filledTrigonRGBA(surface, lx, ly, rx, ry, (int)(gx + direction * size * 0.4),
                     (int)(gy - size * 0.55), wing[g][0], wing[g][1], wing[g][2], alpha);
    filledTrigonRGBA(surface, lx, ly, rx, ry, (int)(gx - direction * size * 0.2),
                     (int)(gy + size * 0.05), wing[g][0] * 2 / 3, wing[g][1] * 2 / 3,
                     wing[g][2] * 2 / 3, alpha);
    thickLineRGBA(surface, (int)gx, (int)gy, (int)gx, (int)(gy + size * 0.8), 1, 30, 30, 36, alpha);
    filledCircleRGBA(surface, (int)gx, (int)(gy + size * 0.9), size / 7 + 1, 30, 30, 36, alpha);
  }
}


// A flurry of maple seeds spins down through the sky, each a single whirling
// wing.
static void sky_event_93(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 18; i++) {
    unsigned int h = fx_hash(seed, i);
    double u = fmod(phase / 3600.0 + (h % 100) / 100.0, 1.0);
    double sx = x - direction * ((h >> 8) % 14) * size * 0.8 + sin(phase / 700.0 + i) * size * 0.4;
    double sy = y - size * 1.5 + u * size * 5.0;
    double spin = phase / 90.0 + i * 1.3;
    double len = size * 0.6;
    double ex = sx + cos(spin) * len, ey = sy + sin(spin) * len * 0.35;
    thickLineRGBA(surface, (int)sx, (int)sy, (int)ex, (int)ey, 2, 190, 150, 90,
                  (Uint8)(alpha * (1.0 - u * 0.5)));
    filledCircleRGBA(surface, (int)sx, (int)sy, 1, 120, 90, 60, (Uint8)(alpha * (1.0 - u * 0.5)));
  }
}


// Three koi swim through the air in lazy S-curves, their tails fanning behind
// them.
static void sky_event_94(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 body[3][3] = {{255, 140, 60}, {245, 245, 240}, {230, 70, 60}};
  for (int k = 0; k < 3; k++) {
    double hx = x - direction * k * size * 3.2, hy = y + k * size * 1.1;
    for (int seg = 6; seg >= 0; seg--) {
      double sx = hx - direction * seg * size * 0.55;
      double sy = hy + sin(phase / 450.0 - seg * 0.7 + k * 1.3) * size * 0.45;
      double r = size * (0.5 - seg * 0.045);
      filledCircleRGBA(surface, (int)sx, (int)sy, (int)r + 1, body[k][0], body[k][1], body[k][2],
                       alpha);
      if (seg % 2 == 1)
        filledCircleRGBA(surface, (int)sx, (int)(sy - r * 0.3), (int)(r * 0.5) + 1, 230, 80, 60,
                         (Uint8)(alpha * 0.6));
    }
    double tx = hx - direction * 7 * size * 0.55,
           ty = hy + sin(phase / 450.0 - 7 * 0.7 + k * 1.3) * size * 0.45;
    double fan = sin(phase / 300.0 + k) * size * 0.4;
    filledTrigonRGBA(surface, (int)(tx + direction * 2), (int)ty, (int)(tx - direction * size),
                     (int)(ty - size * 0.7 + fan), (int)(tx - direction * size),
                     (int)(ty + size * 0.7 + fan), body[k][0], body[k][1], body[k][2],
                     (Uint8)(alpha * 0.7));
    filledCircleRGBA(surface, (int)(hx + direction * size * 0.25), (int)(hy - size * 0.1), 1, 20,
                     20, 24, alpha);
  }
}


// A flock of bright pinwheels spins across the sky on the wind.
static void sky_event_95(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 col[5][3] = {
      {240, 80, 80}, {250, 200, 60}, {80, 190, 240}, {120, 220, 120}, {220, 120, 230}};
  for (int p = 0; p < 5; p++) {
    double px = x - direction * p * size * 2.4,
           py = y + sin(phase / 800.0 + p * 1.4) * size * 0.9 + (p % 2) * size;
    double spin = phase / (160.0 + p * 20.0) * direction;
    double r = size * (0.75 - 0.06 * p);
    for (int blade = 0; blade < 4; blade++) {
      double a = spin + blade * 1.5708;
      filledTrigonRGBA(surface, (int)px, (int)py, (int)(px + cos(a) * r), (int)(py + sin(a) * r),
                       (int)(px + cos(a + 0.9) * r * 0.7), (int)(py + sin(a + 0.9) * r * 0.7),
                       col[p][0], col[p][1], col[p][2], blade % 2 ? alpha : (Uint8)(alpha * 0.75));
    }
    filledCircleRGBA(surface, (int)px, (int)py, 1, 250, 250, 250, alpha);
  }
}


// Two jets cross the sky on opposite diagonals, each drawing a long white
// contrail that slowly spreads and fades.
static void sky_event_96(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int jet = 0; jet < 2; jet++) {
    double dir = jet ? -direction : direction;
    double slope = jet ? 0.22 : -0.18;
    double hx = jet ? x + direction * size * 4 : x, hy = y + (jet ? size * 3 : 0);
    for (int i = 0; i < 40; i++) {
      double back = i * size * 0.5;
      double tx = hx - dir * back, ty = hy - slope * back;
      double age = i / 40.0;
      filledCircleRGBA(surface, (int)tx, (int)ty, (int)(2 + age * size * 0.3), 250, 250, 255,
                       (Uint8)(alpha * 0.95 * (1.0 - age)));
    }
    filledTrigonRGBA(surface, (int)(hx + dir * size * 0.5), (int)hy, (int)(hx - dir * size * 0.2),
                     (int)(hy - size * 0.3), (int)(hx - dir * size * 0.2), (int)(hy + size * 0.3),
                     220, 224, 232, alpha);
    pixelRGBA(surface, (int)(hx + dir * size * 0.5), (int)hy, 255, 255, 255, alpha);
  }
}


// A long ribbon of changing colour twists through the sky as if held by an
// invisible dancer.
static void sky_event_97(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double px = x, py = y;
  for (int i = 1; i <= 36; i++) {
    double nx = x - direction * i * size * 0.45;
    double ny = y + sin(phase / 380.0 - i * 0.38) * size * (0.4 + i * 0.05) +
                cos(phase / 710.0 - i * 0.2) * size * 0.4;
    double hue = phase / 900.0 + i * 0.18;
    Uint8 r = (Uint8)(190 + 60 * sin(hue)), g = (Uint8)(160 + 80 * sin(hue + 2.1)),
          b = (Uint8)(190 + 60 * sin(hue + 4.2));
    thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 3 - i / 16, r, g, b,
                  (Uint8)(alpha * (1.0 - i / 40.0)));
    px = nx;
    py = ny;
  }
  weapon_glow(surface, x, y, 1.6, (const Uint8[]){255, 240, 220}, 200.0 * fade);
}


// A giant moth with eye-spotted wings beats slowly across the sky, glowing
// softly.
static void sky_event_98(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 spot[3] = {255, 220, 140};
  double beat = cos(phase / 260.0);
  double wing = size * (0.4 + 1.5 * fabs(beat));
  double bob = sin(phase / 700.0) * size * 0.5;
  double cy = y + bob;
  for (int side = -1; side <= 1; side += 2) {
    double wy = cy - side * size * 0.9 * (0.4 + 0.6 * fabs(beat));
    filledTrigonRGBA(surface, x, (int)cy, (int)(x - direction * size * 1.2), (int)(wy),
                     (int)(x + direction * size * 0.8), (int)(wy + side * size * 0.2), 205, 170,
                     140, alpha);
    filledEllipseRGBA(surface, (int)(x - direction * size * 0.4), (int)(cy - side * wing * 0.45),
                      (int)(size * 0.9), (int)(wing * 0.45) + 1, 225, 185, 150,
                      (Uint8)(alpha * 0.8));
    weapon_glow(surface, x - direction * size * 0.5, cy - side * wing * 0.5, size * 0.18, spot,
                190.0 * fade);
  }
  filledEllipseRGBA(surface, (int)(x - direction * size * 0.1), (int)cy, (int)(size * 0.9),
                    (int)(size * 0.22) + 1, 96, 76, 64, alpha);
  filledCircleRGBA(surface, (int)(x + direction * size * 0.8), (int)cy, (int)(size * 0.2) + 1, 90,
                   70, 60, alpha);
  lineRGBA(surface, (int)(x + direction * size), (int)cy, (int)(x + direction * size * 1.5),
           (int)(cy - size * 0.4), 200, 190, 170, alpha);
  lineRGBA(surface, (int)(x + direction * size), (int)cy, (int)(x + direction * size * 1.5),
           (int)(cy + size * 0.1), 200, 190, 170, alpha);
}


// A phoenix of living flame crosses the sky, wings beating, shedding sparks
// that drift down behind it.
static void sky_event_99(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 flame[3] = {255, 170, 60};
  double beat = sin(phase / 240.0);
  for (int k = 18; k >= 1; k--) {
    double u = k / 18.0;
    double px = x - direction * k * size * 0.45,
           py = y + sin(phase / 500.0 - k * 0.3) * size * 0.4 + u * size * 0.8;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(size * 0.35 * (1.0 - u * 0.7)) + 1, 255,
                     (Uint8)(190 - 140 * u), (Uint8)(70 - 40 * u),
                     (Uint8)(alpha * 0.7 * (1.0 - u)));
  }
  for (int side = -1; side <= 1; side += 2) {
    double wy =
        y - side * size * 1.6 * (0.3 + 0.7 * fabs(beat)) - (beat > 0 ? 0 : -side * size * 0.3);
    filledTrigonRGBA(surface, x, y, (int)(x - direction * size * 1.8), (int)wy,
                     (int)(x + direction * size * 0.5), (int)(wy + side * size * 0.3), 255, 120, 40,
                     alpha);
    filledTrigonRGBA(surface, x, y, (int)(x - direction * size * 1.1), (int)((y + wy) / 2),
                     (int)(x + direction * size * 0.3), (int)((y + wy) / 2), 255, 215, 90,
                     (Uint8)(alpha * 0.8));
  }
  filledEllipseRGBA(surface, x, y, (int)(size * 0.9), (int)(size * 0.3) + 1, 255, 150, 50, alpha);
  filledCircleRGBA(surface, (int)(x + direction * size * 0.9), (int)(y - size * 0.15),
                   (int)(size * 0.25) + 1, 255, 190, 80, alpha);
  weapon_glow(surface, x, y, size * 0.5, flame, 120.0 * fade);
}


// A drift of diamond dust: tiny ice crystals that flash into four-point glints
// one after another.
static void sky_event_100(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int g = 0; g < 46; g++) {
    unsigned int h = fx_hash(seed, g);
    double gx = x - direction * (h % 16) * size * 0.8 + sin(phase / 1200.0 + g) * size * 0.3;
    double gy = y + ((h >> 8) % 12) * size * 0.45 + phase / 40.0 * 0.0;
    double cyc = fmod(phase / 1400.0 + (h >> 16 & 255) / 255.0, 1.0);
    double flash = cyc < 0.4 ? sin(cyc / 0.4 * 3.14159) : 0.0;
    filledCircleRGBA(surface, (int)gx, (int)gy, 2, 215, 235, 255, (Uint8)(alpha * 1.4));
    if (flash > 0.05) {
      double l = size * (0.15 + 0.4 * flash);
      Uint8 a = (Uint8)(fmin(255.0, alpha * 1.7) * flash);
      thickLineRGBA(surface, (int)(gx - l), (int)gy, (int)(gx + l), (int)gy, 1, 240, 248, 255, a);
      thickLineRGBA(surface, (int)gx, (int)(gy - l), (int)gx, (int)(gy + l), 1, 240, 248, 255, a);
      weapon_glow(surface, gx, gy, 1.5, (const Uint8[]){200, 225, 255}, 160.0 * flash * fade);
    }
  }
}


// A solar sailer glides by: a big bright square sail on a thin mast catching
// sunlight in bands, a tiny hull below.
static void sky_event_101(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double tilt = sin(phase / 1500.0) * 0.2;
  double sw = size * 2.2;
  double lx = x - sw, rx = x + sw;
  double ly = y - size * 1.4 + tilt * size, ry = y - size * 1.4 - tilt * size;
  for (int b = 0; b < 6; b++) {
    double f0 = b / 6.0, f1 = (b + 1) / 6.0;
    double x0 = lx + (rx - lx) * f0, x1 = lx + (rx - lx) * f1;
    double y0 = ly + (ry - ly) * f0, y1 = ly + (ry - ly) * f1;
    Uint8 shade = (Uint8)(150 + 90 * (0.5 + 0.5 * sin(phase / 700.0 - b * 0.9)));
    filledTrigonRGBA(surface, (int)x0, (int)y0, (int)x1, (int)y1, (int)x0, (int)(y0 + size * 2.4),
                     shade, (Uint8)(shade - 20), (Uint8)(shade - 70), alpha);
    filledTrigonRGBA(surface, (int)x1, (int)y1, (int)x1, (int)(y1 + size * 2.4), (int)x0,
                     (int)(y0 + size * 2.4), shade, (Uint8)(shade - 20), (Uint8)(shade - 70),
                     alpha);
  }
  for (int c = 0; c < 4; c++)
    lineRGBA(surface, (int)x, (int)(y + size * 1.0), (int)(lx + (rx - lx) * (c + 0.5) / 4.0),
             (int)(ly + size * 2.4), 150, 150, 160, (Uint8)(alpha * 0.5));
  filledEllipseRGBA(surface, x, (int)(y + size * 1.1), (int)(size * 0.5), (int)(size * 0.25) + 1,
                    190, 195, 205, alpha);
  weapon_glow(surface, lx + (rx - lx) * (0.5 + 0.5 * sin(phase / 900.0)), ly + size * 0.8, 1.8,
              (const Uint8[]){255, 245, 210}, 210.0 * fade);
}


// A small gas giant with three moons that circle it while the whole system
// drifts across the sky.
static void sky_event_102(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double R = size * 1.1;
  filledCircleRGBA(surface, x, y, (int)R, 150, 110, 150, alpha);
  for (int band = -2; band <= 2; band++) {
    double by = band * R * 0.32;
    double hw = sqrt(fmax(0.0, R * R - by * by));
    boxRGBA(surface, (int)(x - hw), (int)(y + by - R * 0.07), (int)(x + hw),
            (int)(y + by + R * 0.07), 190 - band * 12, 140, 170 + band * 10, (Uint8)(alpha * 0.8));
  }
  filledCircleRGBA(surface, (int)(x - R * 0.3), (int)(y - R * 0.3), (int)(R * 0.65), 255, 235, 240,
                   (Uint8)(alpha * 0.18));
  for (int m = 0; m < 3; m++) {
    double a = phase / (900.0 + m * 500.0) + m * 2.1;
    double orbit = R * (1.8 + m * 0.7);
    double mx = x + cos(a) * orbit, my = y + sin(a) * orbit * 0.3;
    bool front = sin(a) > 0;
    ellipseRGBA(surface, x, y, (int)orbit, (int)(orbit * 0.3), 200, 200, 220,
                (Uint8)(alpha * 0.12));
    if (front || fabs(mx - x) > R)
      filledCircleRGBA(surface, (int)mx, (int)my, (int)(size * 0.22) + 1, 220 - m * 20,
                       215 - m * 25, 205 - m * 15, alpha);
  }
}


// Four dragonflies zip and hover across the sky, their wings shimmering.
static void sky_event_103(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int d = 0; d < 4; d++) {
    double burst = sin(phase / 700.0 + d * 1.7);
    double px = x - direction * d * size * 2.0 + burst * size * 0.9 * direction,
           py = y + (d - 1.5) * size * 0.9 + sin(phase / 330.0 + d) * size * 0.25;
    double flick = 0.5 + 0.5 * sin(phase / 28.0 + d);
    for (int pair = 0; pair < 2; pair++) {
      double ox = px - direction * size * (0.1 + pair * 0.25);
      filledEllipseRGBA(surface, (int)ox, (int)(py - size * 0.35), (int)(size * 0.14) + 1,
                        (int)(size * (0.5 + 0.2 * flick)), 200, 235, 250,
                        (Uint8)(alpha * (0.35 + 0.3 * flick)));
      filledEllipseRGBA(surface, (int)ox, (int)(py + size * 0.35), (int)(size * 0.14) + 1,
                        (int)(size * (0.5 + 0.2 * (1.0 - flick))), 200, 235, 250,
                        (Uint8)(alpha * (0.35 + 0.3 * (1.0 - flick))));
    }
    thickLineRGBA(surface, (int)(px + direction * size * 0.3), (int)py,
                  (int)(px - direction * size * 1.0), (int)py, 2, 40, 150, 190, alpha);
    filledCircleRGBA(surface, (int)(px + direction * size * 0.35), (int)py, (int)(size * 0.14) + 1,
                     30, 120, 160, alpha);
  }
}


// A swirl of pink petals is lifted up and whirls across the sky, turning as it
// goes.
static void sky_event_104(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int p = 0; p < 34; p++) {
    unsigned int h = fx_hash(seed, p);
    double a = phase / 600.0 + p * 0.55;
    double r = size * (0.4 + (h % 100) / 100.0 * 2.2);
    double px = x - direction * (h >> 8 & 15) * size * 0.35 + cos(a) * r,
           py = y + sin(a) * r * 0.45 + (p - 17) * size * 0.04;
    double turn = phase / 150.0 + p;
    filledEllipseRGBA(surface, (int)px, (int)py,
                      (int)(size * 0.22 * (0.4 + 0.6 * fabs(cos(turn)))) + 1,
                      (int)(size * 0.14) + 1, 255, (Uint8)(170 + 40 * (p % 3)), 200,
                      (Uint8)(alpha * (0.6 + 0.4 * sin(a))));
  }
}


// A great ring of twelve glowing signs turns slowly across the sky inside a
// thinner counter-turning ring.
static void sky_event_105(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 sign[3] = {255, 225, 150};
  double R = size * 2.6;
  ellipseRGBA(surface, x, y, (int)R, (int)(R * 0.5), 220, 200, 150, (Uint8)(alpha * 0.5));
  ellipseRGBA(surface, x, y, (int)(R * 0.7), (int)(R * 0.35), 200, 190, 160, (Uint8)(alpha * 0.4));
  for (int k = 0; k < 12; k++) {
    double a = phase / 2000.0 + k * 0.5236;
    double px = x + cos(a) * R, py = y + sin(a) * R * 0.5;
    double lit = 0.5 + 0.5 * sin(phase / 400.0 + k);
    weapon_glow(surface, px, py, 1.0 + lit, sign, 130.0 * (0.4 + lit) * fade);
    double a2 = -phase / 1500.0 + k * 0.5236;
    lineRGBA(surface, (int)(x + cos(a2) * R * 0.7), (int)(y + sin(a2) * R * 0.35), (int)px, (int)py,
             230, 210, 160, (Uint8)(alpha * 0.18));
    filledCircleRGBA(surface, (int)(x + cos(a2) * R * 0.7), (int)(y + sin(a2) * R * 0.35), 1, 240,
                     225, 190, (Uint8)(alpha * 0.8));
  }
}


// A small spiral galaxy turns as it drifts, its arms made of thousands of faint
// stars around a warm core.
static void sky_event_106(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 core[3] = {255, 230, 190};
  double rot = phase / 1800.0;
  for (int arm = 0; arm < 2; arm++) {
    for (int i = 0; i < 70; i++) {
      unsigned int h = fx_hash(seed + arm * 31, i);
      double t = i / 70.0;
      double a = rot + arm * 3.14159 + t * 5.2 + ((h % 100) - 50) / 400.0;
      double r = size * (0.3 + 2.8 * t) * (0.9 + (h >> 8 & 31) / 150.0);
      double px = x + cos(a) * r, py = y + sin(a) * r * 0.42;
      filledCircleRGBA(surface, (int)px, (int)py, t < 0.5 ? 1 : 0 + 1,
                       (Uint8)(200 + 55 * (1.0 - t)), (Uint8)(210 + 30 * (1.0 - t)), 255,
                       (Uint8)(alpha * (1.2 - t)));
    }
  }
  weapon_glow(surface, x, y, size * 0.4, core, 200.0 * fade);
}


// A hummingbird darts through the sky, hovering and shooting forward in bursts,
// its wings a blur.
static void sky_event_107(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double hover = sin(phase / 900.0);
  double hx = x + direction * hover * size * 1.2,
         hy = y + sin(phase / 130.0) * 1.5 + cos(phase / 600.0) * size * 0.5;
  double flap = sin(phase / 24.0);
  for (int w = 0; w < 2; w++) {
    double ang = (w ? -1.0 : 1.0) * (0.9 + 0.5 * flap);
    filledEllipseRGBA(surface, (int)(hx - direction * size * 0.2),
                      (int)(hy - size * 0.5 * cos(ang)), (int)(size * 0.14) + 1, (int)(size * 0.55),
                      190, 215, 235, (Uint8)(alpha * (w ? 0.3 : 0.55)));
  }
  filledEllipseRGBA(surface, (int)hx, (int)hy, (int)(size * 0.5), (int)(size * 0.25) + 1, 40, 160,
                    120, alpha);
  filledEllipseRGBA(surface, (int)(hx + direction * size * 0.2), (int)(hy + size * 0.1),
                    (int)(size * 0.3), (int)(size * 0.14) + 1, 220, 90, 100, (Uint8)(alpha * 0.9));
  filledCircleRGBA(surface, (int)(hx + direction * size * 0.55), (int)(hy - size * 0.1),
                   (int)(size * 0.2) + 1, 30, 130, 100, alpha);
  thickLineRGBA(surface, (int)(hx + direction * size * 0.7), (int)(hy - size * 0.1),
                (int)(hx + direction * size * 1.4), (int)(hy - size * 0.05), 1, 230, 230, 235,
                alpha);
  filledTrigonRGBA(surface, (int)(hx - direction * size * 0.4), (int)hy,
                   (int)(hx - direction * size * 0.9), (int)(hy - size * 0.2),
                   (int)(hx - direction * size * 0.9), (int)(hy + size * 0.3), 30, 110, 100, alpha);
}


// A bunch of coloured umbrellas floats down through the sky, each rocking on
// the air.
static void sky_event_108(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 col[5][3] = {
      {220, 70, 80}, {240, 200, 70}, {70, 150, 220}, {130, 200, 110}, {190, 110, 220}};
  for (int u = 0; u < 5; u++) {
    double ux = x - direction * u * size * 2.4,
           uy = y + u * size * 0.6 + sin(phase / 700.0 + u * 1.3) * size * 0.5;
    double rock = sin(phase / 500.0 + u) * 0.25;
    int r = (int)(size * (0.9 - 0.06 * u));
    double hx = ux + sin(rock) * size * 1.4, hy = uy + cos(rock) * size * 1.4;
    lineRGBA(surface, (int)ux, (int)uy, (int)hx, (int)hy, 60, 50, 50, alpha);
    lineRGBA(surface, (int)hx, (int)hy, (int)(hx - 3), (int)(hy + 2), 60, 50, 50, alpha);
    filledPieRGBA(surface, (int)ux, (int)uy, r, 180 + (int)(rock * 57.3), 360 + (int)(rock * 57.3),
                  col[u][0], col[u][1], col[u][2], alpha);
    for (int rib = -1; rib <= 1; rib++)
      lineRGBA(surface, (int)ux, (int)uy, (int)(ux + rib * r * 0.6 + sin(rock) * 0),
               (int)(uy - r * 0.8), 255, 255, 255, (Uint8)(alpha * 0.3));
  }
}


// Two swans fly low in perfect step, necks stretched, wings beating in unison.
static void sky_event_109(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double beat = sin(phase / 340.0);
  for (int sw = 0; sw < 2; sw++) {
    double sx = x - direction * sw * size * 2.4,
           sy = y + sw * size * 0.9 + sin(phase / 1200.0 + sw) * size * 0.2;
    for (int side = -1; side <= 1; side += 2) {
      double tipy = sy - side * size * (0.4 + 1.4 * fabs(beat)) * (beat > 0 ? 1.0 : 0.6);
      filledTrigonRGBA(surface, (int)(sx - direction * size * 0.3), (int)sy,
                       (int)(sx + direction * size * 0.4), (int)sy,
                       (int)(sx - direction * size * 0.9), (int)tipy, 240, 242, 248,
                       (Uint8)(alpha * (side > 0 ? 1.0 : 0.7)));
    }
    filledEllipseRGBA(surface, (int)sx, (int)sy, (int)(size * 0.9), (int)(size * 0.28) + 1, 250,
                      250, 252, alpha);
    for (int seg = 0; seg < 5; seg++) {
      double nx = sx + direction * (size * 0.8 + seg * size * 0.28),
             ny = sy - sin(seg * 0.6) * size * 0.2 - seg * size * 0.02;
      filledCircleRGBA(surface, (int)nx, (int)ny, (int)(size * 0.1) + 1, 250, 250, 252, alpha);
    }
    filledCircleRGBA(surface, (int)(sx + direction * size * 2.3), (int)(sy - size * 0.35),
                     (int)(size * 0.14) + 1, 250, 250, 252, alpha);
    filledTrigonRGBA(surface, (int)(sx + direction * size * 2.4), (int)(sy - size * 0.38),
                     (int)(sx + direction * size * 2.75), (int)(sy - size * 0.32),
                     (int)(sx + direction * size * 2.4), (int)(sy - size * 0.28), 240, 150, 60,
                     alpha);
  }
}


// A school of silver flying fish leaps in long arcs across the sky, one after
// another.
static void sky_event_110(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int k = 0; k < 7; k++) {
    double u = fmod(phase / 1700.0 + k * 0.14, 1.0);
    double fx = x - direction * (k * size * 1.0) + direction * (u - 0.5) * size * 5.0;
    double fy = y + (k % 3) * size * 0.5 - sin(u * 3.14159) * size * 2.2;
    double ang = atan2(-cos(u * 3.14159) * 2.2 * 3.14159, 5.0);
    double dx = direction * cos(ang), dy = sin(ang) * direction;
    filledEllipseRGBA(surface, (int)fx, (int)fy, (int)(size * 0.5) + 1, (int)(size * 0.16) + 1, 205,
                      220, 235, alpha);
    filledTrigonRGBA(surface, (int)(fx - dx * size * 0.5), (int)(fy - dy * size * 0.5),
                     (int)(fx - dx * size * 0.9), (int)(fy - dy * size * 0.5 - size * 0.3),
                     (int)(fx - dx * size * 0.9), (int)(fy - dy * size * 0.5 + size * 0.3), 170,
                     195, 225, alpha);
    filledTrigonRGBA(surface, (int)fx, (int)fy, (int)(fx - direction * size * 0.3),
                     (int)(fy - size * 0.7), (int)(fx + direction * size * 0.1),
                     (int)(fy - size * 0.1), 215, 235, 255, (Uint8)(alpha * 0.7));
  }
}


// A small floating island with a tree, a lamp-lit hut and a thin waterfall that
// falls away into the dark.
static void sky_event_111(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double bob = sin(phase / 900.0) * size * 0.3;
  double iy = y + bob, w = size * 2.4;
  filledTrigonRGBA(surface, (int)(x - w), (int)iy, (int)(x + w), (int)iy,
                   (int)(x + sin(phase / 2000.0) * size * 0.2), (int)(iy + size * 2.6), 96, 80, 84,
                   alpha);
  filledEllipseRGBA(surface, x, (int)iy, (int)w, (int)(size * 0.35) + 1, 80, 150, 90, alpha);
  thickLineRGBA(surface, (int)(x - w * 0.4), (int)iy, (int)(x - w * 0.4), (int)(iy - size * 1.1), 2,
                96, 70, 56, alpha);
  filledCircleRGBA(surface, (int)(x - w * 0.4), (int)(iy - size * 1.3), (int)(size * 0.6) + 1, 60,
                   140, 80, alpha);
  boxRGBA(surface, (int)(x + w * 0.2), (int)(iy - size * 0.7), (int)(x + w * 0.5), (int)iy, 200,
          180, 150, alpha);
  filledTrigonRGBA(surface, (int)(x + w * 0.15), (int)(iy - size * 0.7), (int)(x + w * 0.55),
                   (int)(iy - size * 0.7), (int)(x + w * 0.35), (int)(iy - size * 1.2), 150, 70, 70,
                   alpha);
  weapon_glow(surface, x + w * 0.35, iy - size * 0.35, 0.8, (const Uint8[]){255, 210, 130},
              210.0 * fade);
  for (int d = 0; d < 14; d++) {
    double u = fmod(phase / 900.0 + d / 14.0, 1.0);
    filledCircleRGBA(surface, (int)(x + w * 0.85 + sin(d) * 1.5), (int)(iy + u * size * 4.0), 1,
                     190, 220, 250, (Uint8)(alpha * (1.0 - u)));
  }
}


// A lens-shaped cloud hovers, layered like a stack of plates, its rim glowing
// in slow pastel pulses.
static void sky_event_112(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int l = 0; l < 5; l++) {
    double w = size * (3.2 - l * 0.45), yy = y + l * size * 0.28;
    filledEllipseRGBA(surface, x, (int)yy, (int)w, (int)(size * 0.28) + 1, 190 - l * 10,
                      190 - l * 12, 215 - l * 8, (Uint8)(alpha * 0.8));
    double hue = phase / 700.0 + l * 0.8;
    ellipseRGBA(surface, x, (int)yy, (int)w, (int)(size * 0.28) + 1, (Uint8)(210 + 40 * sin(hue)),
                (Uint8)(190 + 50 * sin(hue + 2.1)), (Uint8)(230 + 25 * sin(hue + 4.2)),
                (Uint8)(alpha * 0.9));
  }
  for (int k = 0; k < 8; k++) {
    double a = phase / 800.0 + k * 0.785;
    weapon_glow(surface, x + cos(a) * size * 3.0, y + size * 0.7 + sin(a) * size * 0.3, 0.8,
                (const Uint8[]){230, 210, 255}, 120.0 * (0.5 + 0.5 * sin(a)) * fade);
  }
}


// Three dolphins leap through the sky in graceful arcs, one after another.
static void sky_event_113(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int k = 0; k < 3; k++) {
    double u = fmod(phase / 2300.0 + k * 0.28, 1.0);
    double span = size * 6.0, rise = size * 2.4;
    double hx = x - direction * k * size * 4.0 + direction * (u - 0.5) * span;
    double hy = y + k * size * 0.5 - sin(u * 3.14159) * rise;
    double vx = direction * span, vy = -cos(u * 3.14159) * rise * 3.14159;
    double vl = sqrt(vx * vx + vy * vy);
    double dx = vx / vl, dy = vy / vl;
    for (int seg = 0; seg < 6; seg++) {
      double r = size * (0.33 - fabs(seg - 1.5) * 0.04 - seg * 0.01);
      filledCircleRGBA(surface, (int)(hx - dx * seg * size * 0.4),
                       (int)(hy - dy * seg * size * 0.4), (int)r + 1, 120, 160, 200, alpha);
    }
    filledTrigonRGBA(surface, (int)(hx - dx * size * 0.8), (int)(hy - dy * size * 0.8 - size * 0.1),
                     (int)(hx - dx * size * 1.2), (int)(hy - dy * size * 1.2 - size * 0.55),
                     (int)(hx - dx * size * 1.5), (int)(hy - dy * size * 1.5), 90, 130, 170, alpha);
    double tx = hx - dx * size * 2.5, ty = hy - dy * size * 2.5;
    thickLineRGBA(surface, (int)(tx - size * 0.1), (int)(ty - size * 0.4),
                  (int)(tx + dx * size * 0.1), (int)ty, 2, 100, 140, 180, alpha);
    thickLineRGBA(surface, (int)(tx - size * 0.1), (int)(ty + size * 0.4),
                  (int)(tx + dx * size * 0.1), (int)ty, 2, 100, 140, 180, alpha);
    filledCircleRGBA(surface, (int)(hx + dx * size * 0.35),
                     (int)(hy + dy * size * 0.35 - size * 0.04), (int)(size * 0.2) + 1, 160, 195,
                     225, alpha);
  }
}


// A pinwheel firework spins in the sky, throwing curved arms of gold sparks.
static void sky_event_114(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double spin = phase / 140.0;
  for (int arm = 0; arm < 6; arm++) {
    for (int i = 0; i < 12; i++) {
      double a = spin - i * 0.22 + arm * 1.0472;
      double r = size * (0.4 + i * 0.22);
      filledCircleRGBA(surface, (int)(x + cos(a) * r), (int)(y + sin(a) * r), i < 3 ? 2 : 1, 255,
                       (Uint8)(220 - i * 12), (Uint8)(120 - i * 8),
                       (Uint8)(alpha * 1.4 * (1.0 - i / 12.0)));
    }
  }
  weapon_glow(surface, x, y, size * 0.5, (const Uint8[]){255, 210, 120}, 200.0 * fade);
}


// A thunderbird glides overhead, its wide wings edged with crackling blue
// lightning.
static void sky_event_115(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double beat = sin(phase / 500.0);
  for (int side = -1; side <= 1; side += 2) {
    double tipx = x - direction * size * 1.2, tipy = y - side * size * (1.6 + 1.2 * fabs(beat));
    filledTrigonRGBA(surface, x, y, (int)tipx, (int)tipy, (int)(x + direction * size * 0.6),
                     (int)(y + side * size * 0.1), 120, 110, 175, alpha);
    double bx = x, by = y;
    unsigned int h = fx_hash(seed, (unsigned int)(phase / 90));
    for (int seg = 1; seg <= 6; seg++) {
      double f = seg / 6.0;
      double nx = x + (tipx - x) * f + ((int)((h >> seg * 3) & 7) - 3) * size * 0.08;
      double ny = y + (tipy - y) * f + ((int)((h >> (seg * 3 + 1)) & 7) - 3) * size * 0.08;
      lineRGBA(surface, (int)bx, (int)by, (int)nx, (int)ny, 150, 200, 255,
               (Uint8)(alpha * 1.6 > 255 ? 255 : alpha * 1.6));
      bx = nx;
      by = ny;
    }
  }
  filledEllipseRGBA(surface, x, y, (int)(size * 0.9), (int)(size * 0.28) + 1, 85, 80, 130, alpha);
  filledCircleRGBA(surface, (int)(x + direction * size), y, (int)(size * 0.22) + 1, 85, 80, 130,
                   alpha);
  weapon_glow(surface, x + direction * size * 1.1, y - size * 0.05, 0.9,
              (const Uint8[]){255, 230, 120}, 230.0 * fade);
  weapon_glow(surface, x, y, size * 0.5, (const Uint8[]){130, 180, 255}, 90.0 * fade);
}


// A slim crescent moon sails across the sky with a tiny boat hung from its tip,
// fishing for a star.
static void sky_event_116(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double R = size * 2.6;
  for (int i = 0; i <= 40; i++) {
    double a = -1.2 + 2.4 * i / 40.0;
    double px = x + R * 0.6 - cos(a) * R, py = y - sin(a) * R;
    double r = size * 0.55 * cos(a / 1.2 * 1.5708) + 1.0;
    filledCircleRGBA(surface, (int)px, (int)py, (int)r, 245, 235, 200, alpha);
  }
  double bx = x + R * 0.6 - cos(1.2) * R, by = y - sin(1.2) * R;
  double sway = sin(phase / 500.0) * size * 0.3;
  lineRGBA(surface, (int)bx, (int)by, (int)(bx + sway), (int)(by + size * 1.2), 200, 200, 200,
           alpha);
  filledTrigonRGBA(surface, (int)(bx + sway - size * 0.4), (int)(by + size * 1.2),
                   (int)(bx + sway + size * 0.4), (int)(by + size * 1.2), (int)(bx + sway),
                   (int)(by + size * 1.5), 120, 80, 60, alpha);
  filledTrigonRGBA(surface, (int)(bx + sway), (int)(by + size * 1.2), (int)(bx + sway),
                   (int)(by + size * 0.6), (int)(bx + sway + size * 0.35), (int)(by + size * 1.15),
                   235, 230, 220, alpha);
  lineRGBA(surface, (int)(bx + sway), (int)(by + size * 1.3), (int)(bx + sway - size * 0.3),
           (int)(by + size * 2.6), 200, 200, 200, (Uint8)(alpha * 0.7));
  weapon_glow(surface, bx + sway - size * 0.3, by + size * 2.6, 1.2, (const Uint8[]){255, 245, 190},
              220.0 * (0.6 + 0.4 * sin(phase / 250.0)) * fade);
}


// A big crystal sphere rolls through the sky with a slow storm of gold and
// violet turning inside it.
static void sky_event_117(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double R = size * 1.5;
  filledCircleRGBA(surface, x, y, (int)R, 70, 80, 130, (Uint8)(alpha * 0.5));
  for (int i = 0; i < 60; i++) {
    double t = i / 60.0;
    double a = phase / 700.0 + t * 12.0;
    double r = R * 0.85 * t;
    double px = x + cos(a) * r, py = y + sin(a) * r * 0.9;
    filledCircleRGBA(surface, (int)px, (int)py, t < 0.4 ? 2 : 1, (Uint8)(255 * (1.0 - t * 0.4)),
                     (Uint8)(200 - 100 * t), (Uint8)(120 + 120 * t),
                     (Uint8)(alpha * 1.6 * (1.0 - t * 0.4)));
  }
  circleRGBA(surface, x, y, (int)R, 200, 215, 255, (Uint8)(alpha * 1.1));
  filledEllipseRGBA(surface, (int)(x - R * 0.4), (int)(y - R * 0.5), (int)(R * 0.25),
                    (int)(R * 0.12) + 1, 255, 255, 255, (Uint8)(alpha * 0.8));
  weapon_glow(surface, x, y, R * 0.3, (const Uint8[]){255, 215, 150}, 120.0 * fade);
}


// A wavy musical staff unrolls across the sky and glowing notes rise and fall
// along it.
static void sky_event_118(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int line = 0; line < 5; line++) {
    double px = x, py = y + line * size * 0.3;
    for (int i = 1; i <= 30; i++) {
      double nx = x - direction * i * size * 0.5;
      double ny = y + line * size * 0.3 + sin(phase / 600.0 - i * 0.35) * size * 0.8;
      lineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 225, 225, 240,
               (Uint8)(alpha * 0.55 * (1.0 - i / 32.0)));
      px = nx;
      py = ny;
    }
  }
  for (int n = 0; n < 8; n++) {
    double i = 2.0 + n * 3.6;
    double nx = x - direction * i * size * 0.5;
    double ny = y + ((n * 7) % 5) * size * 0.3 + sin(phase / 600.0 - i * 0.35) * size * 0.8;
    double lit = 0.6 + 0.4 * sin(phase / 300.0 + n);
    filledEllipseRGBA(surface, (int)nx, (int)ny, (int)(size * 0.18) + 1, (int)(size * 0.13) + 1,
                      255, 225, 150, (Uint8)(alpha * 1.4 * lit));
    thickLineRGBA(surface, (int)(nx + size * 0.15), (int)ny, (int)(nx + size * 0.15),
                  (int)(ny - size * 0.8), 1, 255, 225, 150, (Uint8)(alpha * 1.2 * lit));
  }
}


// A big diamond kite flies on a long string, its tail of paper bows whipping in
// the wind.
static void sky_event_119(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double tilt = sin(phase / 700.0) * 0.2;
  double w = size * 0.9, h = size * 1.3;
  double cx = x, cy = y;
  filledTrigonRGBA(surface, (int)cx, (int)(cy - h), (int)(cx - w), (int)cy, (int)cx,
                   (int)(cy + h * 0.2), 235, 90, 80, alpha);
  filledTrigonRGBA(surface, (int)cx, (int)(cy - h), (int)(cx + w), (int)cy, (int)cx,
                   (int)(cy + h * 0.2), 250, 200, 80, alpha);
  filledTrigonRGBA(surface, (int)(cx - w), (int)cy, (int)cx, (int)(cy + h * 0.2), (int)cx,
                   (int)(cy + h), 80, 150, 220, alpha);
  filledTrigonRGBA(surface, (int)(cx + w), (int)cy, (int)cx, (int)(cy + h * 0.2), (int)cx,
                   (int)(cy + h), 100, 200, 130, alpha);
  lineRGBA(surface, (int)cx, (int)(cy - h), (int)cx, (int)(cy + h), 255, 255, 255,
           (Uint8)(alpha * 0.5));
  lineRGBA(surface, (int)(cx - w), (int)cy, (int)(cx + w), (int)cy, 255, 255, 255,
           (Uint8)(alpha * 0.5));
  double tx = cx, ty = cy + h;
  for (int i = 1; i <= 12; i++) {
    double nx = cx - direction * i * size * 0.4 +
                sin(phase / 250.0 - i * 0.8) * size * 0.4 * (i / 12.0) + tilt * i;
    double ny = cy + h + i * size * 0.28;
    lineRGBA(surface, (int)tx, (int)ty, (int)nx, (int)ny, 230, 230, 235, alpha);
    if (i % 2 == 0) {
      filledTrigonRGBA(surface, (int)nx, (int)ny, (int)(nx - size * 0.3), (int)(ny - size * 0.18),
                       (int)(nx - size * 0.3), (int)(ny + size * 0.18), i % 4 ? 240 : 100, 120,
                       i % 4 ? 100 : 230, alpha);
      filledTrigonRGBA(surface, (int)nx, (int)ny, (int)(nx + size * 0.3), (int)(ny - size * 0.18),
                       (int)(nx + size * 0.3), (int)(ny + size * 0.18), i % 4 ? 240 : 100, 120,
                       i % 4 ? 100 : 230, alpha);
    }
    tx = nx;
    ty = ny;
  }
}


// Two comets circle one another as they travel, their tails braiding into a
// bright double spiral.
static void sky_event_120(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int c = 0; c < 2; c++) {
    for (int i = 0; i < 26; i++) {
      double a = phase / 450.0 + c * 3.14159 - i * 0.16;
      double r = size * 1.2;
      double px = x - direction * i * size * 0.18 + cos(a) * r, py = y + sin(a) * r * 0.8;
      double f = 1.0 - i / 26.0;
      filledCircleRGBA(surface, (int)px, (int)py, (int)(1 + 2 * f), c ? 150 : 255, c ? 200 : 190,
                       c ? 255 : 120, (Uint8)(alpha * 1.4 * f));
    }
    double a = phase / 450.0 + c * 3.14159;
    weapon_glow(surface, x + cos(a) * size * 1.2, y + sin(a) * size * 0.96, 1.4,
                c ? (const Uint8[]){160, 210, 255} : (const Uint8[]){255, 200, 130}, 230.0 * fade);
  }
}


// A curtain of glowing glass beads falls in long strands, swaying as if hung on
// invisible threads.
static void sky_event_121(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int strand = 0; strand < 14; strand++) {
    unsigned int h = fx_hash(seed, strand);
    double sx = x - direction * strand * size * 0.7;
    double top = y - size * 1.0 + ((h >> 4) & 7) * size * 0.1;
    for (int b = 0; b < 9; b++) {
      double v = fmod(phase / 2600.0 + b / 9.0 + (h & 63) / 64.0, 1.0);
      double by = top + v * size * 6.0,
             bx = sx + sin(phase / 800.0 + strand * 0.5 + v * 3.0) * size * 0.3;
      double hue = strand * 0.5 + b * 0.7;
      Uint8 c[3] = {(Uint8)(190 + 60 * sin(hue)), (Uint8)(190 + 60 * sin(hue + 2.1)),
                    (Uint8)(215 + 40 * sin(hue + 4.2))};
      filledCircleRGBA(surface, (int)bx, (int)by, 2, c[0], c[1], c[2],
                       (Uint8)(alpha * 1.4 * sin(v * 3.14159)));
    }
  }
}


// A great sea turtle paddles slowly through the sky, its shell patterned with
// softly glowing hexagons.
static void sky_event_122(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 shell_glow[3] = {120, 240, 200};
  double bob = sin(phase / 1100.0) * size * 0.3;
  double cy = y + bob;
  double stroke = sin(phase / 700.0);
  for (int side = -1; side <= 1; side += 2) {
    double fx = x + direction * size * 0.7, fy = cy + side * size * 0.6;
    double tipx = fx + direction * size * 0.5 - direction * stroke * size * 0.5,
           tipy = cy + side * size * (1.6 + 0.4 * stroke);
    filledTrigonRGBA(surface, (int)fx, (int)fy, (int)(fx - direction * size * 0.5),
                     (int)(fy + side * size * 0.1), (int)tipx, (int)tipy, 70, 130, 110, alpha);
    double bx = x - direction * size * 0.9, by = cy + side * size * 0.45;
    filledTrigonRGBA(surface, (int)bx, (int)by, (int)(bx - direction * size * 0.4), (int)by,
                     (int)(bx - direction * size * 0.6),
                     (int)(by + side * size * (0.7 + 0.2 * stroke)), 70, 130, 110, alpha);
  }
  filledEllipseRGBA(surface, x, (int)cy, (int)(size * 1.5), (int)(size * 1.0), 70, 120, 90, alpha);
  filledEllipseRGBA(surface, x, (int)cy, (int)(size * 1.2), (int)(size * 0.78), 96, 150, 110,
                    alpha);
  for (int k = 0; k < 7; k++) {
    double a = k * 0.8976;
    double hx = k == 0 ? x : x + cos(a) * size * 0.62, hy = k == 0 ? cy : cy + sin(a) * size * 0.45;
    double lit = 0.5 + 0.5 * sin(phase / 500.0 + k * 0.9);
    weapon_glow(surface, hx, hy, size * 0.14, shell_glow, 110.0 * lit * fade);
    for (int e = 0; e < 6; e++) {
      double a1 = e * 1.0472, a2 = (e + 1) * 1.0472;
      lineRGBA(surface, (int)(hx + cos(a1) * size * 0.3), (int)(hy + sin(a1) * size * 0.22),
               (int)(hx + cos(a2) * size * 0.3), (int)(hy + sin(a2) * size * 0.22), 200, 245, 225,
               (Uint8)(alpha * 0.55));
    }
  }
  filledCircleRGBA(surface, (int)(x + direction * size * 1.6), (int)(cy - size * 0.05),
                   (int)(size * 0.38), 90, 150, 120, alpha);
  filledCircleRGBA(surface, (int)(x + direction * size * 1.75), (int)(cy - size * 0.15), 1, 20, 24,
                   30, alpha);
}


// A line of prayer flags streams across the sky on an unseen cord, each flag
// rippling in turn.
static void sky_event_123(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 col[5][3] = {
      {70, 130, 220}, {240, 240, 240}, {220, 70, 70}, {80, 190, 100}, {245, 205, 70}};
  double px = x, py = y;
  for (int i = 1; i <= 24; i++) {
    double nx = x - direction * i * size * 0.55;
    double ny = y + sin(phase / 900.0 - i * 0.3) * size * 0.5 + i * size * 0.04;
    lineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 200, 200, 205, (Uint8)(alpha * 0.8));
    if (i % 2 == 0) {
      double ripple = sin(phase / 180.0 - i * 0.9) * size * 0.2;
      const Uint8 *c = col[(i / 2) % 5];
      double fw = size * 0.45, fh = size * 0.6;
      filledTrigonRGBA(surface, (int)nx, (int)ny, (int)(nx + fw * 0.9 * -direction),
                       (int)(ny + ripple), (int)nx, (int)(ny + fh), c[0], c[1], c[2],
                       (Uint8)(alpha * (1.0 - i / 30.0)));
      filledTrigonRGBA(surface, (int)(nx + fw * 0.9 * -direction), (int)(ny + ripple),
                       (int)(nx + fw * 0.9 * -direction), (int)(ny + fh + ripple), (int)nx,
                       (int)(ny + fh), c[0], c[1], c[2], (Uint8)(alpha * (1.0 - i / 30.0)));
    }
    px = nx;
    py = ny;
  }
}


// A small spinning top of light floats across the sky, shedding a spiral of
// sparks from its tip.
static void sky_event_124(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 tint[3] = {255, 190, 120};
  double wob = sin(phase / 300.0) * size * 0.15;
  double spin = phase / 90.0;
  filledTrigonRGBA(surface, (int)(x - size * 0.7 + wob), y, (int)(x + size * 0.7 + wob), y, (int)x,
                   (int)(y + size * 1.2), 235, 120, 80, alpha);
  for (int band = 0; band < 3; band++) {
    double w = size * (0.55 - band * 0.15), yy = y + band * size * 0.22 + size * 0.1;
    double shift = cos(spin + band) * w * 0.7;
    lineRGBA(surface, (int)(x - w + wob * (1.0 - band * 0.3)), (int)yy,
             (int)(x + w + wob * (1.0 - band * 0.3)), (int)yy, 255, 235, 190, alpha);
    filledCircleRGBA(surface, (int)(x + shift), (int)yy, 1, 255, 255, 230, alpha);
  }
  thickLineRGBA(surface, (int)(x + wob), y, (int)(x + wob * 0.5), (int)(y - size * 0.5), 2, 220,
                190, 140, alpha);
  for (int i = 0; i < 26; i++) {
    double a = spin * 0.6 - i * 0.4;
    double r = size * (0.15 + i * 0.07);
    double px = x - direction * i * size * 0.15 + cos(a) * r,
           py = y + size * 1.2 + sin(a) * r * 0.4;
    double f = 1.0 - i / 26.0;
    filledCircleRGBA(surface, (int)px, (int)py, f > 0.6 ? 2 : 1, 255, (Uint8)(220 * f + 30), 120,
                     (Uint8)(alpha * 1.4 * f));
  }
  weapon_glow(surface, x, y + size * 1.2, 1.4, tint, 210.0 * fade);
}


// A feather drifts down through the sky, tumbling gently, leaving a faint
// shimmering trail.
static void sky_event_125(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 sheen[3] = {255, 245, 230};
  for (int i = 14; i >= 0; i--) {
    double age = i * 70.0;
    double ph = phase - age;
    double fx = x - direction * i * size * 0.25 + sin(ph / 600.0) * size * 1.0;
    double fy = y + ph / 500.0 * 0.0 + cos(ph / 900.0) * size * 0.4 + i * size * 0.15;
    filledCircleRGBA(surface, (int)fx, (int)fy, 1, 255, 245, 230,
                     (Uint8)(alpha * 0.6 * (1.0 - i / 15.0)));
  }
  double sway = sin(phase / 600.0), turn = phase / 700.0;
  double fx = x + sway * size * 1.0, fy = y + cos(phase / 900.0) * size * 0.4;
  double len = size * 1.6, ang = sin(turn) * 0.9 + 0.3;
  double tx = fx + cos(ang) * len, ty = fy + sin(ang) * len * 0.6;
  double bx = fx - cos(ang) * len, by = fy - sin(ang) * len * 0.6;
  for (int v = 0; v < 12; v++) {
    double f = (v + 0.5) / 12.0;
    double mx = bx + (tx - bx) * f, my = by + (ty - by) * f;
    double w = size * 0.5 * sin(f * 3.14159) * (0.5 + 0.5 * fabs(cos(turn)));
    double nx = -sin(ang) * 0.6, ny = cos(ang);
    lineRGBA(surface, (int)mx, (int)my, (int)(mx + nx * w), (int)(my + ny * w + size * 0.1), 240,
             236, 230, alpha);
    lineRGBA(surface, (int)mx, (int)my, (int)(mx - nx * w), (int)(my - ny * w + size * 0.1), 240,
             236, 230, alpha);
  }
  thickLineRGBA(surface, (int)bx, (int)by, (int)tx, (int)ty, 1, 200, 190, 180, alpha);
  weapon_glow(surface, tx, ty, 1.0, sheen, 90.0 * fade);
}


// A school of glowing lantern-fish drifts through the sky, each with a bright
// bulb dangling in front of its mouth.
static void sky_event_126(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 bulb[3] = {150, 255, 220};
  for (int f = 0; f < 6; f++) {
    unsigned int h = fx_hash(seed, f);
    double fx = x - direction * f * size * 3.0 + sin(phase / 800.0 + f) * size * 0.3;
    double fy = y + (f % 3 - 1) * size * 1.3 + ((int)(h % 3) - 1) * size * 0.3 +
                sin(phase / 600.0 + f * 1.7) * size * 0.3;
    double len = size * (0.8 + 0.1 * (h % 4));
    filledEllipseRGBA(surface, (int)fx, (int)fy, (int)len, (int)(len * 0.45) + 1, 90, 120, 170,
                      alpha);
    filledEllipseRGBA(surface, (int)fx, (int)(fy - len * 0.1), (int)(len * 0.8),
                      (int)(len * 0.25) + 1, 140, 175, 220, (Uint8)(alpha * 0.8));
    double wag = sin(phase / 260.0 + f) * len * 0.3;
    filledTrigonRGBA(surface, (int)(fx - direction * len * 0.8), (int)fy,
                     (int)(fx - direction * len * 1.5), (int)(fy - len * 0.4 + wag),
                     (int)(fx - direction * len * 1.5), (int)(fy + len * 0.4 + wag), 100, 135, 185,
                     alpha);
    double mx = fx + direction * len * 0.9;
    filledCircleRGBA(surface, (int)(mx - direction * len * 0.3), (int)(fy - len * 0.1), 1, 240, 250,
                     255, alpha);
    double bx = mx + direction * len * 0.4,
           by = fy - len * 0.9 + sin(phase / 400.0 + f) * len * 0.15;
    lineRGBA(surface, (int)(fx + direction * len * 0.4), (int)(fy - len * 0.4), (int)bx, (int)by,
             150, 190, 200, (Uint8)(alpha * 0.8));
    weapon_glow(surface, bx, by, 2.6, bulb, 230.0 * (0.7 + 0.3 * sin(phase / 300.0 + f)) * fade);
  }
}


// A ring of paper cutout stars spins slowly like a mobile, each star turning on
// its own thread.
static void sky_event_127(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 col[4][3] = {
      {255, 220, 120}, {255, 160, 170}, {150, 210, 255}, {190, 240, 170}};
  double R = size * 2.2;
  double turn = phase / 1800.0;
  lineRGBA(surface, x, (int)(y - size * 2.4), x, y, 210, 210, 220, (Uint8)(alpha * 0.6));
  ellipseRGBA(surface, x, y, (int)R, (int)(R * 0.22), 210, 210, 220, (Uint8)(alpha * 0.7));
  for (int k = 0; k < 6; k++) {
    double a = turn + k * 1.0472;
    double sx = x + cos(a) * R, sy = y + sin(a) * R * 0.22;
    double drop = size * (0.9 + 0.35 * (k % 3));
    lineRGBA(surface, (int)sx, (int)sy, (int)sx, (int)(sy + drop), 200, 200, 210,
             (Uint8)(alpha * 0.6));
    double cy = sy + drop, spin = phase / 400.0 * (k % 2 ? 1.0 : -1.0) + k;
    double squash = fabs(cos(spin * 0.5)) * 0.7 + 0.3;
    const Uint8 *c = col[k % 4];
    for (int p = 0; p < 5; p++) {
      double a1 = p * 1.2566 - 1.5708, a2 = a1 + 0.6283, a3 = a1 + 1.2566;
      double r1 = size * 0.5, r2 = size * 0.2;
      filledTrigonRGBA(surface, (int)(sx + cos(a1) * r1 * squash), (int)(cy + sin(a1) * r1),
                       (int)(sx + cos(a2) * r2 * squash), (int)(cy + sin(a2) * r2), (int)sx,
                       (int)cy, c[0], c[1], c[2], alpha);
      filledTrigonRGBA(surface, (int)(sx + cos(a2) * r2 * squash), (int)(cy + sin(a2) * r2),
                       (int)(sx + cos(a3) * r1 * squash), (int)(cy + sin(a3) * r1), (int)sx,
                       (int)cy, c[0], c[1], c[2], alpha);
    }
  }
}


// A rolling wave of cloud curls over like surf, spray flying off its crest in
// fine droplets.
static void sky_event_128(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double R = size * 2.4;
  double sweep = 0.75 + 0.15 * sin(phase / 900.0);
  // A long swell rises from the left and curls over into a spiral in front.
  for (int i = 0; i < 60; i++) {
    double t = i / 59.0;
    double theta = 3.14159 - t * 4.4 * sweep * 1.3;
    double r = R * (1.0 - 0.6 * t);
    double px = x + direction * cos(theta) * r, py = y - sin(theta) * r * 0.8;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(size * (0.5 - 0.3 * t)) + 1, 225, 232, 245,
                     (Uint8)(alpha * (0.9 - 0.4 * t)));
  }
  for (int i = 0; i < 14; i++) {
    double f = i / 13.0;
    filledCircleRGBA(surface, (int)(x - direction * (R * 1.0 + (1.0 - f) * size * 3.0)),
                     (int)(y + R * 0.2 + f * size * 0.1 - (1.0 - f) * size * 0.2),
                     (int)(size * 0.45) + 1, 190, 205, 230, (Uint8)(alpha * 0.45 * f));
  }
  filledEllipseRGBA(surface, x, (int)(y + R * 0.95), (int)(R * 1.5), (int)(size * 0.3) + 1, 170,
                    190, 220, (Uint8)(alpha * 0.4));
  for (int d = 0; d < 22; d++) {
    unsigned int h = fx_hash(seed, d);
    double u = fmod(phase / 1100.0 + (h % 100) / 100.0, 1.0);
    double sx = x + direction * (R * 0.2 + u * size * 2.4),
           sy = y - R * 0.75 - sin(u * 3.14159) * size * 1.0 + u * u * size * 2.0;
    double jitter = ((int)((h >> 8) % 7) - 3) * size * 0.08;
    filledCircleRGBA(surface, (int)(sx + jitter), (int)sy, 1, 245, 250, 255,
                     (Uint8)(alpha * 1.5 * (1.0 - u)));
  }
}


// A great white heron glides across the sky with slow wingbeats, long legs
// trailing, a ripple of light under each stroke.
static void sky_event_129(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double bob = cos(phase / 520.0) * size * 0.15;
  double cy = y + bob;
  for (int wing = 1; wing >= 0; wing--) {
    // Far wing (wing 1) is dimmer and a little behind; both beat together.
    double lag = wing ? 0.12 : 0.0;
    double lift = sin(phase / 520.0 - lag * 3.0);
    double ox = wing ? direction * size * 0.25 : 0.0;
    double tipx = x + ox - direction * size * (1.6 + 0.5 * fabs(lift)),
           tipy = cy - size * 2.0 * lift - size * 0.3;
    double armx = x + ox - direction * size * 0.2, army = cy - size * 1.2 * lift - size * 0.15;
    Uint8 a = (Uint8)(alpha * (wing ? 0.65 : 1.0));
    filledTrigonRGBA(surface, (int)(x + ox + direction * size * 0.6), (int)cy,
                     (int)(x + ox - direction * size * 0.7), (int)cy, (int)armx, (int)army, 245,
                     246, 250, a);
    filledTrigonRGBA(surface, (int)(x + ox - direction * size * 0.7), (int)cy, (int)armx, (int)army,
                     (int)tipx, (int)tipy, 228, 232, 244, a);
    lineRGBA(surface, (int)armx, (int)army, (int)tipx, (int)tipy, 200, 206, 224, a);
  }
  filledEllipseRGBA(surface, x, (int)cy, (int)(size * 0.9), (int)(size * 0.24) + 1, 250, 250, 252,
                    alpha);
  for (int seg = 0; seg < 5; seg++) {
    double f = seg / 4.0;
    double nx = x + direction * (size * 0.8 + f * size * 0.7),
           ny = cy - size * 0.1 - sin(f * 3.14159) * size * 0.45 + f * size * 0.05;
    filledCircleRGBA(surface, (int)nx, (int)ny, (int)(size * 0.1) + 1, 250, 250, 252, alpha);
  }
  filledCircleRGBA(surface, (int)(x + direction * size * 1.6), (int)(cy - size * 0.05),
                   (int)(size * 0.16) + 1, 250, 250, 252, alpha);
  filledTrigonRGBA(surface, (int)(x + direction * size * 1.7), (int)(cy - size * 0.1),
                   (int)(x + direction * size * 2.4), (int)(cy - size * 0.02),
                   (int)(x + direction * size * 1.7), (int)(cy + size * 0.03), 235, 190, 80, alpha);
  for (int leg = 0; leg < 2; leg++) {
    thickLineRGBA(surface, (int)(x - direction * size * 0.7), (int)(cy + size * 0.05 * leg),
                  (int)(x - direction * size * (2.0 + 0.15 * leg)),
                  (int)(cy + size * 0.35 + size * 0.05 * leg), 1, 60, 60, 66, alpha);
  }
  for (int r = 0; r < 3; r++) {
    double u = fmod(phase / 1040.0 + r / 3.0, 1.0);
    ellipseRGBA(surface, (int)(x - direction * u * size * 0.6), (int)(cy + size * 1.6),
                (int)(size * (0.3 + u * 1.0)), (int)(size * (0.06 + u * 0.18)) + 1, 220, 230, 245,
                (Uint8)(alpha * 0.45 * (1.0 - u)));
  }
}


// A flight of paper boats with glowing candle-flames sails through the sky on a
// slow river of mist.
static void sky_event_130(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 flame[3] = {255, 200, 110};
  for (int m = 0; m < 14; m++) {
    double mx = x - direction * m * size * 0.9,
           my = y + size * 1.2 + sin(phase / 700.0 - m * 0.5) * size * 0.35;
    filledCircleRGBA(surface, (int)mx, (int)my, (int)(size * 0.7), 190, 205, 230,
                     (Uint8)(alpha * 0.2));
  }
  for (int b = 0; b < 5; b++) {
    double bx = x - direction * b * size * 2.5,
           by = y + sin(phase / 700.0 - b * 1.1) * size * 0.35 - (b % 2) * size * 0.2;
    double rock = sin(phase / 500.0 + b) * 0.12;
    double w = size * (0.8 - 0.05 * b);
    filledTrigonRGBA(surface, (int)(bx - w), (int)(by - w * 0.1 * rock * 8), (int)(bx + w),
                     (int)(by + w * 0.1 * rock * 8), (int)bx, (int)(by + w * 0.45), 245, 240, 228,
                     alpha);
    filledTrigonRGBA(surface, (int)bx, (int)(by - w * 0.9), (int)bx, (int)by,
                     (int)(bx - direction * w * 0.7), (int)by, 235, 228, 214, alpha);
    filledTrigonRGBA(surface, (int)bx, (int)(by - w * 0.6), (int)bx, (int)by,
                     (int)(bx + direction * w * 0.6), (int)by, 250, 246, 236, alpha);
    weapon_glow(surface, bx, by - w * 0.15, 1.0 + w * 0.05, flame,
                220.0 * (0.75 + 0.25 * sin(phase / 130.0 + b * 2.0)) * fade);
  }
}


// A glowing hourglass of drifting sand turns slowly end over end, a thin bright
// thread falling through its waist.
static void sky_event_131(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 sand[3] = {255, 215, 130};
  double turn = phase / 1500.0;
  double flip = fmod(turn, 2.0);
  double ang = flip < 1.0 ? 0.0 : 3.14159 * (flip - 1.0);
  double ca = cos(ang), sa = sin(ang);
  double hh = size * 1.6, hw = size * 0.8;
#define HG(px, py) (int)(x + (px) * ca - (py) * sa), (int)(y + (px) * sa + (py) * ca)
  filledTrigonRGBA(surface, HG(-hw, -hh), HG(hw, -hh), HG(0.0, 0.0), 120, 140, 190,
                   (Uint8)(alpha * 0.45));
  filledTrigonRGBA(surface, HG(-hw, hh), HG(hw, hh), HG(0.0, 0.0), 120, 140, 190,
                   (Uint8)(alpha * 0.45));
  double fill = flip < 1.0 ? flip : 1.0;
  double top = 1.0 - fill, bot = fill;
  filledTrigonRGBA(surface, HG(-hw * top, -hh * 0.2 - hh * 0.8 * (1.0 - top)),
                   HG(hw * top, -hh * 0.2 - hh * 0.8 * (1.0 - top)), HG(0.0, 0.0), sand[0], sand[1],
                   sand[2], (Uint8)(alpha * 1.1));
  filledTrigonRGBA(surface, HG(-hw * bot, hh - hh * 0.5 * bot), HG(hw * bot, hh - hh * 0.5 * bot),
                   HG(0.0, hh - hh * 0.9 * bot), sand[0], sand[1], sand[2], (Uint8)(alpha * 1.1));
  lineRGBA(surface, HG(0.0, 0.0), HG(0.0, hh * 0.8 * (flip < 1.0 ? 1.0 : 0.0)), 255, 240, 190,
           alpha);
  lineRGBA(surface, HG(-hw, -hh), HG(hw, -hh), 220, 220, 235, alpha);
  lineRGBA(surface, HG(-hw, hh), HG(hw, hh), 220, 220, 235, alpha);
#undef HG
  weapon_glow(surface, x, y, 1.6, sand, 150.0 * fade);
}


// A drift of glowing spores rises like champagne bubbles and bursts at the top
// into tiny rings of pale light.
static void sky_event_132(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int b = 0; b < 30; b++) {
    unsigned int h = fx_hash(seed, b);
    double u = fmod(phase / 2800.0 + (h % 100) / 100.0, 1.0);
    double bx = x - direction * ((h >> 8) % 16) * size * 0.5 + sin(phase / 700.0 + b) * size * 0.25;
    double by = y + size * 2.5 - u * size * 4.0;
    double r = size * (0.12 + 0.1 * ((h >> 16) % 3));
    if (u < 0.88) {
      filledCircleRGBA(surface, (int)bx, (int)by, (int)r + 1, 180, 230, 255, (Uint8)(alpha * 0.5));
      circleRGBA(surface, (int)bx, (int)by, (int)r + 1, 230, 250, 255, (Uint8)(alpha * 1.1));
      pixelRGBA(surface, (int)(bx - r * 0.3), (int)(by - r * 0.3), 255, 255, 255, alpha);
    } else {
      double q = (u - 0.88) / 0.12;
      ellipseRGBA(surface, (int)bx, (int)by, (int)(r + q * size * 0.5) + 1,
                  (int)(r + q * size * 0.5) + 1, 220, 245, 255, (Uint8)(alpha * 1.3 * (1.0 - q)));
    }
  }
}


// A lone figure on a glowing disc surfs the air on a long ribbon of light,
// carving slow S-curves.
static void sky_event_133(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 trail[3] = {140, 220, 255};
  for (int i = 1; i <= 30; i++) {
    double px = x - direction * i * size * 0.5;
    double py = y + sin(phase / 500.0 - i * 0.28) * size * 0.9;
    double f = 1.0 - i / 31.0;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(1 + 3 * f), trail[0], trail[1], trail[2],
                     (Uint8)(alpha * 1.3 * f));
  }
  double bank = cos(phase / 500.0) * 0.5;
  double sx = x, sy = y + sin(phase / 500.0) * size * 0.9;
  thickLineRGBA(surface, (int)(sx - size * 1.0), (int)(sy + size * 0.1 + bank * size * 0.4),
                (int)(sx + size * 1.0), (int)(sy + size * 0.1 - bank * size * 0.4), 3, 230, 240,
                255, alpha);
  weapon_glow(surface, sx, sy + size * 0.1, size * 0.35, trail, 150.0 * fade);
  thickLineRGBA(surface, (int)sx, (int)(sy + size * 0.1), (int)(sx + bank * size * 0.5),
                (int)(sy - size * 1.1), 4, 235, 225, 210, alpha);
  filledCircleRGBA(surface, (int)(sx + bank * size * 0.6), (int)(sy - size * 1.4),
                   (int)(size * 0.3) + 1, 245, 235, 220, alpha);
  thickLineRGBA(surface, (int)(sx + bank * size * 0.4), (int)(sy - size * 0.8),
                (int)(sx - size * 0.9 + bank * size * 0.4),
                (int)(sy - size * 0.5 - bank * size * 0.6), 3, 235, 225, 210, alpha);
  thickLineRGBA(surface, (int)(sx + bank * size * 0.4), (int)(sy - size * 0.8),
                (int)(sx + size * 0.9 + bank * size * 0.4),
                (int)(sy - size * 0.5 + bank * size * 0.6), 3, 235, 225, 210, alpha);
}


// A bright aurora-coloured ribbon of birds wheels across the sky in a slow
// figure-eight, trailing glimmers.
static void sky_event_134(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int b = 0; b < 16; b++) {
    double t = phase / 1500.0 - b * 0.2;
    double ex = sin(t) * size * 5.0, ey = sin(t * 2.0) * size * 1.8;
    double px = x + ex, py = y + ey;
    double vx = cos(t) * size * 5.0, vy = cos(t * 2.0) * size * 3.6;
    double l = sqrt(vx * vx + vy * vy) + 0.001;
    double dx = vx / l, dy = vy / l;
    double hue = b * 0.4 + phase / 800.0;
    Uint8 r = (Uint8)(150 + 90 * sin(hue)), g = (Uint8)(190 + 60 * sin(hue + 2.1)),
          bl = (Uint8)(215 + 40 * sin(hue + 4.2));
    double flap = sin(phase / 140.0 + b) * size * 0.3;
    thickLineRGBA(surface, (int)(px - dx * size * 0.3 - dy * (size * 0.6 + flap)),
                  (int)(py - dy * size * 0.3 + dx * (size * 0.6 + flap)), (int)px, (int)py, 2, r, g,
                  bl, (Uint8)(alpha * 1.4));
    thickLineRGBA(surface, (int)(px - dx * size * 0.3 + dy * (size * 0.6 + flap)),
                  (int)(py - dy * size * 0.3 - dx * (size * 0.6 + flap)), (int)px, (int)py, 2, r, g,
                  bl, (Uint8)(alpha * 1.4));
    filledCircleRGBA(surface, (int)px, (int)py, 2, 255, 255, 255, alpha);
    for (int k = 1; k <= 3; k++)
      filledCircleRGBA(surface, (int)(px - dx * k * size * 0.35), (int)(py - dy * k * size * 0.35),
                       1, r, g, bl, (Uint8)(alpha * 0.5 / k));
  }
}


// A small orrery drifts by: a bright sun with three planets circling it on
// faint rings, one with its own moon.
static void sky_event_135(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 sun[3] = {255, 220, 140};
  static const Uint8 planet[3][3] = {{150, 190, 255}, {255, 160, 120}, {190, 255, 190}};
  for (int p = 0; p < 3; p++) {
    double orbit = size * (1.8 + p * 1.3);
    ellipseRGBA(surface, x, y, (int)orbit, (int)(orbit * 0.4), 200, 205, 225,
                (Uint8)(alpha * 0.35));
    double a = phase / (500.0 + p * 380.0) + p * 2.0;
    double px = x + cos(a) * orbit, py = y + sin(a) * orbit * 0.4;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(size * (0.32 + 0.08 * p)) + 1, planet[p][0],
                     planet[p][1], planet[p][2], (Uint8)(alpha * 1.4));
    if (p == 1) {
      double ma = phase / 160.0;
      filledCircleRGBA(surface, (int)(px + cos(ma) * size * 0.45), (int)(py + sin(ma) * size * 0.2),
                       1, 235, 235, 240, alpha);
    }
  }
  weapon_glow(surface, x, y, size * 0.6, sun, 210.0 * fade);
}


// A lace of frost spreads across the sky like a window pane: branching ice
// ferns grow outward and glitter.
static void sky_event_136(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 ice[3] = {210, 240, 255};
  double grow = smooth_step(0.0, 1.0, fmod(phase / 3200.0, 1.0));
  for (int arm = 0; arm < 6; arm++) {
    double a = arm * 1.0472 + phase / 6000.0;
    double len = size * 3.2 * grow;
    double ex = x + cos(a) * len, ey = y + sin(a) * len * 0.8;
    thickLineRGBA(surface, x, y, (int)ex, (int)ey, 2, ice[0], ice[1], ice[2], alpha);
    for (int k = 1; k <= 6; k++) {
      double f = k / 7.0;
      double bx = x + cos(a) * len * f, by = y + sin(a) * len * 0.8 * f;
      double bl = size * 1.0 * (1.0 - f) * grow;
      for (int side = -1; side <= 1; side += 2) {
        lineRGBA(surface, (int)bx, (int)by, (int)(bx + cos(a + side * 0.9) * bl),
                 (int)(by + sin(a + side * 0.9) * bl * 0.8), ice[0], ice[1], ice[2],
                 (Uint8)(alpha * 0.9));
      }
    }
    double tw = sin(phase / 200.0 + arm * 1.3);
    if (tw > 0.5) weapon_glow(surface, ex, ey, 1.4, ice, 220.0 * (tw - 0.5) * 2.0 * fade);
  }
  weapon_glow(surface, x, y, size * 0.3, ice, 160.0 * fade);
}


// A string of glowing fairy lights unspools across the sky, each bulb twinkling
// in a travelling wave.
static void sky_event_137(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double px = x, py = y;
  for (int i = 1; i <= 26; i++) {
    double nx = x - direction * i * size * 0.6;
    double ny = y + sin(phase / 900.0 - i * 0.35) * size * 0.8 + sin(i * 0.9) * size * 0.2;
    lineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 190, 190, 200,
             (Uint8)(alpha * 0.6 * (1.0 - i / 30.0)));
    double wave = 0.5 + 0.5 * sin(phase / 250.0 - i * 0.7);
    double hue = i * 0.5;
    Uint8 c[3] = {(Uint8)(220 + 35 * sin(hue)), (Uint8)(190 + 60 * sin(hue + 2.1)),
                  (Uint8)(170 + 80 * sin(hue + 4.2))};
    weapon_glow(surface, nx, ny + size * 0.15, 1.2 + wave, c,
                230.0 * (0.3 + 0.7 * wave) * (1.0 - i / 30.0) * fade);
    px = nx;
    py = ny;
  }
}


// A lone hot-air lantern balloon rises through a swarm of tiny sparks that
// spiral up around it like a chimney draught.
static void sky_event_138(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 warm[3] = {255, 190, 100};
  double bx = x, by = y + sin(phase / 800.0) * size * 0.4;
  double r = size * 1.1;
  filledEllipseRGBA(surface, (int)bx, (int)by, (int)(r * 0.8), (int)r, 235, 120, 80, alpha);
  filledEllipseRGBA(surface, (int)(bx - r * 0.2), (int)(by - r * 0.2), (int)(r * 0.4),
                    (int)(r * 0.6), 255, 190, 140, (Uint8)(alpha * 0.8));
  for (int k = -1; k <= 1; k++)
    lineRGBA(surface, (int)(bx + k * r * 0.5), (int)(by + r * 0.7), (int)(bx + k * r * 0.2),
             (int)(by + r * 1.6), 200, 190, 180, alpha);
  boxRGBA(surface, (int)(bx - r * 0.25), (int)(by + r * 1.6), (int)(bx + r * 0.25),
          (int)(by + r * 1.9), 130, 90, 70, alpha);
  weapon_glow(surface, bx, by + r * 1.5, 1.6, warm, 230.0 * (0.8 + 0.2 * sin(phase / 90.0)) * fade);
  for (int i = 0; i < 40; i++) {
    double u = fmod(phase / 2200.0 + i / 40.0, 1.0);
    double a = u * 14.0 + i;
    double sx = bx + cos(a) * r * (0.5 + 0.7 * u),
           sy = by + r * 1.5 - u * size * 5.0 + sin(a) * 2.0;
    filledCircleRGBA(surface, (int)sx, (int)sy, 1, 255, 210, 140, (Uint8)(alpha * 1.5 * (1.0 - u)));
  }
}


// "The Colour Out of Space" (Lovecraft): an unearthly, nameless miasma of
// sickly violet and phosphorescent chartreuse drifts across the sky, trailing
// slow, gravity-defying tendrils and motes that pulse out of phase with nature.
static void sky_event_139(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 violet[3] = {178, 72, 245};
  static const Uint8 blight[3] = {152, 248, 128};
  double drift = sin(phase / 1100.0) * size * 0.45;
  double cx = x, cy = y + drift;
  double rx = size * 2.4, ry = size * 1.25;
  // Soft, shifting outer miasma lobes (alternating unearthly hues)
  for (int lobe = 0; lobe < 7; lobe++) {
    double a = lobe * 0.9 + phase / 1400.0;
    double lx = cx + cos(a) * rx * (0.35 + 0.25 * sin(phase / 900.0 + lobe));
    double ly = cy + sin(a * 1.3) * ry * 0.55;
    double lr = size * (0.9 + 0.35 * sin(phase / 650.0 + lobe * 1.7));
    bool alt = (lobe % 2 == 0);
    Uint8 r = alt ? violet[0] : blight[0];
    Uint8 g = alt ? violet[1] : blight[1];
    Uint8 b = alt ? violet[2] : blight[2];
    filledEllipseRGBA(surface, (int)lx, (int)ly, (int)(lr * 1.2), (int)(lr * 0.75), r, g, b,
                      weapon_alpha(26.0 * fade));
  }
  // Anomalous core: a dark void-heart rimmed with shifting alien spectra
  filledCircleRGBA(surface, (int)cx, (int)cy, (int)(size * 0.42) + 1, 10, 6, 18,
                   weapon_alpha(190.0 * fade));
  double pulse = 0.7 + 0.3 * sin(phase / 320.0);
  weapon_glow(surface, cx, cy, size * 0.55 * pulse, violet, 120.0 * fade);
  weapon_glow(surface, cx + cos(phase / 500.0) * size * 0.25, cy + sin(phase / 420.0) * size * 0.15,
              size * 0.35, blight, 110.0 * fade);
  // Slow descending and ascending tendrils of strange luminescence
  for (int t = 0; t < 6; t++) {
    double tx0 = cx + (t - 2.5) * size * 0.55;
    double px = tx0, py = cy + size * 0.25;
    for (int seg = 1; seg <= 7; seg++) {
      double u = seg / 7.0;
      double nx = tx0 + sin(phase / 700.0 + t * 1.3 + seg * 0.7) * size * 0.35;
      double ny = cy + size * (0.25 + u * 2.3);
      const Uint8 *c = (t % 2 == 0) ? blight : violet;
      lineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, c[0], c[1], c[2],
               weapon_alpha(135.0 * (1.0 - u * 0.8) * fade));
      px = nx;
      py = ny;
    }
  }
  // Drifting spore-motes of the Colour
  for (int i = 0; i < 18; i++) {
    double u = fmod(phase / 2400.0 + i / 18.0, 1.0);
    double mx = cx + sin(i * 2.7 + u * 5.0) * size * 2.1;
    double my = cy + cos(i * 1.9 + u * 3.5) * size * 1.4 - u * size * 0.8;
    const Uint8 *c = (i % 3 == 0) ? violet : blight;
    weapon_glow(surface, mx, my, 1.4, c, 190.0 * (1.0 - u) * fade);
  }
}


// "Black Stars" (Chambers' Carcosa / Ligotti): a triad of pitch-black
// suns ringed by pale, feverish coronas and twin eclipsed crescents; faint
// geometric filaments link them like a forbidden constellation.
static void sky_event_140(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 corona[3] = {235, 224, 175};
  static const Uint8 ember[3] = {185, 130, 110};
  double base_a = phase / 2600.0;
  double px[3], py[3], pr[3];
  for (int i = 0; i < 3; i++) {
    double a = base_a + i * (2.0 * M_PI / 3.0);
    double dist = size * (i == 0 ? 1.5 : 1.9);
    px[i] = x + cos(a) * dist;
    py[i] = y + sin(a) * dist * 0.62;
    pr[i] = size * (i == 0 ? 0.65 : 0.45);
  }
  // Faint constellation filaments connecting the black stars
  for (int i = 0; i < 3; i++) {
    int j = (i + 1) % 3;
    lineRGBA(surface, (int)px[i], (int)py[i], (int)px[j], (int)py[j], corona[0], corona[1],
             corona[2], weapon_alpha(55.0 * fade));
  }
  // Draw each black star: pale eclipse corona, void disc, and cold rays
  for (int i = 0; i < 3; i++) {
    double pulse = 0.8 + 0.2 * sin(phase / 410.0 + i * 2.1);
    double r = pr[i];
    filledCircleRGBA(surface, (int)px[i], (int)py[i], (int)(r * 1.38 * pulse), corona[0], corona[1],
                     corona[2], weapon_alpha(38.0 * fade));
    circleRGBA(surface, (int)px[i], (int)py[i], (int)(r * 1.15), corona[0], corona[1], corona[2],
               weapon_alpha(150.0 * fade));
    filledCircleRGBA(surface, (int)px[i], (int)py[i], (int)r + 1, 5, 4, 8,
                     weapon_alpha(245.0 * fade));
    for (int ray = 0; ray < 4; ray++) {
      double ra = ray * (M_PI / 2.0) + phase / 1900.0 * (i % 2 ? -1 : 1);
      double len = r * (1.9 + 0.35 * pulse);
      lineRGBA(surface, (int)(px[i] + cos(ra) * r), (int)(py[i] + sin(ra) * r),
               (int)(px[i] + cos(ra) * len), (int)(py[i] + sin(ra) * len), ember[0], ember[1],
               ember[2], weapon_alpha(110.0 * fade));
    }
  }
}


// "Solaris" (Lem): a strange, living ocean-mimoid of symmetry-breaking
// concentric shells and iridescent ribs floats across the sky, folding and
// unfolding impossible crystalline symmetries as it passes.
static void sky_event_141(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 shell_c[3] = {135, 205, 230};
  static const Uint8 core_c[3] = {245, 165, 130};
  double morph = phase / 1200.0;
  double r0 = size * 1.45;
  // Outer translucent undulating membrane
  const int pts = 24;
  int px_prev = 0, py_prev = 0;
  for (int i = 0; i <= pts; i++) {
    double a = (2.0 * M_PI * i) / pts;
    double wobble = 1.0 + 0.24 * sin(a * 3.0 + morph) + 0.14 * cos(a * 5.0 - morph * 1.3);
    int px = x + (int)(cos(a) * r0 * wobble);
    int py = y + (int)(sin(a) * r0 * 0.72 * wobble);
    if (i > 0) {
      thickLineRGBA(surface, px_prev, py_prev, px, py, 2, shell_c[0], shell_c[1], shell_c[2],
                    weapon_alpha(150.0 * fade));
    }
    px_prev = px;
    py_prev = py;
  }
  // Inner counter-rotating symmetry ribs
  for (int rib = 0; rib < 6; rib++) {
    double a = rib * (M_PI / 3.0) + morph * 0.45;
    double rx = cos(a) * r0 * 0.92;
    double ry = sin(a) * r0 * 0.66;
    lineRGBA(surface, x, y, (int)(x + rx), (int)(y + ry), shell_c[0], shell_c[1], shell_c[2],
             weapon_alpha(95.0 * fade));
    weapon_glow(surface, x + rx * 0.75, y + ry * 0.75, 1.5, core_c, 155.0 * fade);
  }
  // Living amber-rose core shifting slowly inside the shell
  double cx = x + cos(morph * 0.8) * size * 0.2;
  double cy = y + sin(morph * 1.1) * size * 0.14;
  weapon_glow(surface, cx, cy, size * 0.42, core_c, 165.0 * fade);
  filledCircleRGBA(surface, (int)cx, (int)cy, (int)(size * 0.24) + 1, 255, 210, 190,
                   weapon_alpha(200.0 * fade));
}


// "Annihilation" / Area X (Jeff VanderMeer): a shimmering, refractive
// curtain-lens of bioluminescent gold and sea-teal spores arcs silently across
// the sky, bending starlight into slow concentric ripples.
static void sky_event_142(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 gold[3] = {245, 215, 115};
  static const Uint8 teal[3] = {105, 230, 195};
  double rx = size * 2.2;
  double ry = size * 1.15;
  // Concentric refractive shimmer rings
  for (int ring = 0; ring < 3; ring++) {
    double f = 0.55 + ring * 0.35 + 0.08 * sin(phase / 550.0 + ring);
    const Uint8 *c = (ring % 2 == 0) ? teal : gold;
    ellipseRGBA(surface, x, y, (int)(rx * f), (int)(ry * f), c[0], c[1], c[2],
                weapon_alpha((125.0 - ring * 28.0) * fade));
  }
  // Golden-spore filaments drifting inside the shimmer
  for (int i = 0; i < 16; i++) {
    double a = phase / 1500.0 + i * (2.0 * M_PI / 16.0);
    double d = 0.25 + 0.7 * fmod(i * 0.37 + phase / 3200.0, 1.0);
    double sx = x + cos(a) * rx * d;
    double sy = y + sin(a) * ry * d;
    const Uint8 *c = (i % 3 == 0) ? teal : gold;
    weapon_glow(surface, sx, sy, 1.6, c, 175.0 * fade);
  }
}


// "Blindsight" (Peter Watts): Rorschach — a jagged, asymmetric fractal-torus
// of matte-black ribs shrouded in a cold, stuttering magenta-indigo EM corona;
// faint magnetic-field arcs flick between its spines as it drifts.
static void sky_event_143(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 em_mag[3] = {195, 85, 225};
  static const Uint8 em_ice[3] = {115, 175, 245};
  double rx = size * 1.9, ry = size * 1.15;
  double spin = phase / 2100.0;
  // Cold electromagnetic corona
  filledEllipseRGBA(surface, x, y, (int)(rx * 1.15), (int)(ry * 1.15), em_mag[0], em_mag[1],
                    em_mag[2], weapon_alpha(20.0 * fade));
  // Jagged matte-black fractal ribs around an occluded central void
  const int ribs = 14;
  for (int i = 0; i < ribs; i++) {
    double a = spin + i * (2.0 * M_PI / ribs);
    double r1 = 0.42 + 0.14 * sin(i * 2.7 + spin * 1.5);
    double r2 = 0.95 + 0.25 * cos(i * 3.1 - spin);
    int x1 = x + (int)(cos(a) * rx * r1);
    int y1 = y + (int)(sin(a) * ry * r1);
    int x2 = x + (int)(cos(a + 0.18) * rx * r2);
    int y2 = y + (int)(sin(a + 0.18) * ry * r2);
    thickLineRGBA(surface, x1, y1, x2, y2, 3, 10, 12, 20, weapon_alpha(240.0 * fade));
    // Stuttering magnetic arc at outer spine tips
    if ((i + (int)(phase / 240)) % 3 == 0) {
      weapon_glow(surface, x2, y2, 1.4, (i % 2 == 0) ? em_mag : em_ice, 140.0 * fade);
    }
  }
  filledEllipseRGBA(surface, x, y, (int)(rx * 0.38), (int)(ry * 0.34), 6, 7, 14,
                    weapon_alpha(230.0 * fade));
}


// "The Three-Body Problem" (Cixin Liu): a chaotic, unfolding sophon — a giant
// etched geometric eye-sphere in the upper sky, accompanied by three wandering
// suns drifting out of alignment.
static void sky_event_144(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 etch_c[3] = {155, 225, 250};
  static const Uint8 sun_c[3][3] = {{255, 195, 115}, {255, 140, 120}, {210, 235, 255}};
  double unfold = sin(smooth_step(0.05, 0.95, progress) * M_PI);
  double r = size * (0.95 + 0.95 * unfold);
  // Etched dimensional-unfolding circuit rings
  for (int ring = 0; ring < 3; ring++) {
    double rr = r * (0.42 + ring * 0.28);
    circleRGBA(surface, x, y, (int)rr, etch_c[0], etch_c[1], etch_c[2],
               weapon_alpha((145.0 - ring * 30.0) * fade));
    double a = phase / (900.0 + ring * 400.0) * (ring % 2 ? -1 : 1);
    lineRGBA(surface, (int)(x + cos(a) * rr), (int)(y + sin(a) * rr), (int)(x - cos(a) * rr),
             (int)(y - sin(a) * rr), etch_c[0], etch_c[1], etch_c[2], weapon_alpha(75.0 * fade));
  }
  // Central watching pupil
  filledCircleRGBA(surface, x, y, (int)(r * 0.18) + 1, 8, 12, 22, weapon_alpha(225.0 * fade));
  // Three chaotic suns orbiting unevenly around the unfolding eye
  for (int s_i = 0; s_i < 3; s_i++) {
    double a = phase / (1100.0 + s_i * 350.0) + s_i * 2.1;
    double dist = r * (0.65 + 0.35 * sin(phase / 700.0 + s_i * 1.7));
    double sx = x + cos(a) * dist;
    double sy = y + sin(a) * dist * 0.65;
    weapon_glow(surface, sx, sy, 2.2, sun_c[s_i], 195.0 * fade);
  }
}


// "Story of Your Life" (Ted Chiang): a circular Heptapod logogram of dark
// ink-smoke tendrils and varying-weight arcs coalesces in the sky, its misty
// calligraphy breathing slowly before dissolving.
static void sky_event_145(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double r = size * 1.45;
  double breath = 0.88 + 0.12 * sin(phase / 950.0);
  const int segs = 36;
  for (int i = 0; i < segs; i++) {
    double a0 = (2.0 * M_PI * i) / segs;
    double a1 = (2.0 * M_PI * (i + 1)) / segs;
    double w0 = 1.0 + 0.12 * sin(a0 * 5.0 + seed) + 0.08 * cos(a0 * 9.0);
    double w1 = 1.0 + 0.12 * sin(a1 * 5.0 + seed) + 0.08 * cos(a1 * 9.0);
    int x0 = x + (int)(cos(a0) * r * w0 * breath);
    int y0 = y + (int)(sin(a0) * r * w0 * breath);
    int x1 = x + (int)(cos(a1) * r * w1 * breath);
    int y1 = y + (int)(sin(a1) * r * w1 * breath);
    int thick = 1 + (int)(2.5 * (0.5 + 0.5 * sin(a0 * 3.0 + seed * 0.7)));
    thickLineRGBA(surface, x0, y0, x1, y1, thick, 18, 22, 32, weapon_alpha(230.0 * fade));
    lineRGBA(surface, x0, y0, x1, y1, 145, 165, 190, weapon_alpha(70.0 * fade));
    // Wispy ink tendrils radiating from thick clusters
    if (i % 4 == 0) {
      double tr = r * (1.28 + 0.18 * sin(phase / 700.0 + i));
      lineRGBA(surface, x0, y0, x + (int)(cos(a0 + 0.15) * tr), y + (int)(sin(a0 + 0.15) * tr), 28,
               34, 48, weapon_alpha(150.0 * fade));
    }
  }
}


// "Rendezvous with Rama" (Arthur C. Clarke): a colossal, slowly rotating
// cylindrical habitat with banded hull ribs, a lit aft radiator ring, and
// faint navigation strobes drifting silently across the upper sky.
static void sky_event_146(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double len = size * 3.4;
  double rad = size * 0.72;
  double cx = x;
  double cy = y;
  int x0 = (int)(cx - direction * len * 0.5);
  int x1 = (int)(cx + direction * len * 0.5);
  int left = x0 < x1 ? x0 : x1;
  int right = x0 < x1 ? x1 : x0;
  Uint8 hull_a = weapon_alpha(205.0 * fade);
  boxRGBA(surface, left, (int)(cy - rad), right, (int)(cy + rad), 46, 54, 68, hull_a);
  // Rotating longitudinal hull bands
  double spin = phase / 1400.0;
  for (int b = 0; b < 4; b++) {
    double by = cy + sin(spin + b * (M_PI * 0.5)) * rad * 0.85;
    lineRGBA(surface, left, (int)by, right, (int)by, 94, 110, 132, weapon_alpha(115.0 * fade));
  }
  // End-cap ellipses and aft radiator glow
  filledEllipseRGBA(surface, x1, (int)cy, (int)(rad * 0.34), (int)rad, 28, 34, 46, hull_a);
  ellipseRGBA(surface, x0, (int)cy, (int)(rad * 0.34), (int)rad, 155, 195, 225,
              weapon_alpha(150.0 * fade));
  static const Uint8 strobe[3] = {160, 225, 255};
  weapon_glow(surface, x0, cy, 1.5, strobe, 165.0 * fade);
}


// Dyson swarm (Stapledon / Dyson): a pale, ember-amber star partially
// occluded by concentric orbital collector rings and flickering power-relay
// glints drifting in formation around its corona.
static void sky_event_147(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 star_c[3] = {255, 205, 130};
  static const Uint8 glint_c[3] = {155, 220, 255};
  double r = size * 0.72;
  weapon_glow(surface, x, y, r * 1.4, star_c, 135.0 * fade);
  filledCircleRGBA(surface, x, y, (int)r, 245, 185, 105, weapon_alpha(205.0 * fade));
  // Tilted collector rings
  for (int ring = 0; ring < 3; ring++) {
    double rx = size * (1.2 + ring * 0.45);
    double ry = rx * (0.28 + ring * 0.12);
    ellipseRGBA(surface, x, y, (int)rx, (int)ry, 42, 50, 68, weapon_alpha(210.0 * fade));
  }
  // Orbiting collector panels / relay glints
  for (int i = 0; i < 12; i++) {
    double a = phase / (1100.0 + (i % 3) * 350.0) + i * (2.0 * M_PI / 12.0);
    double rx = size * (1.2 + (i % 3) * 0.45);
    double ry = rx * (0.28 + (i % 3) * 0.12);
    double px = x + cos(a) * rx;
    double py = y + sin(a) * ry;
    boxRGBA(surface, (int)px - 2, (int)py - 1, (int)px + 2, (int)py + 1, 22, 28, 40,
            weapon_alpha(230.0 * fade));
    if ((i + (int)(phase / 260)) % 4 == 0) {
      weapon_glow(surface, px, py, 1.3, glint_c, 150.0 * fade);
    }
  }
}


// Pulsar lighthouse: a rapidly spinning magnetized neutron star sweeping twin
// relativistic cyan-violet beams across an elliptical accretion halo.
static void sky_event_148(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 core_c[3] = {195, 240, 255};
  static const Uint8 beam_c[3] = {125, 210, 255};
  static const Uint8 halo_c[3] = {175, 130, 245};
  double sweep = phase / 340.0;
  double beam_len = size * 3.4;
  // Equator accretion ring
  ellipseRGBA(surface, x, y, (int)(size * 1.45), (int)(size * 0.42), halo_c[0], halo_c[1],
              halo_c[2], weapon_alpha(115.0 * fade));
  // Twin rotating relativistic jets
  for (int j = 0; j < 2; j++) {
    double a = sweep + j * M_PI;
    double dx = cos(a), dy = sin(a) * 0.65;
    double ex = x + dx * beam_len, ey = y + dy * beam_len;
    thickLineRGBA(surface, x, y, (int)ex, (int)ey, 4, beam_c[0], beam_c[1], beam_c[2],
                  weapon_alpha(55.0 * fade));
    thickLineRGBA(surface, x, y, (int)ex, (int)ey, 2, 240, 250, 255, weapon_alpha(185.0 * fade));
  }
  weapon_glow(surface, x, y, size * 0.42, core_c, 225.0 * fade);
}


// Quantum prism: a rotating crystal octahedron refracts a single silver
// starlight ray into a fanning rainbow spectrum across the upper sky.
static void sky_event_149(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 spec_c[5][3] = {
      {255, 115, 115}, {255, 200, 105}, {140, 245, 165}, {115, 215, 255}, {195, 145, 255}};
  double r = size * 1.15;
  double rot = phase / 1900.0;
  int px[4], py[4];
  for (int i = 0; i < 4; i++) {
    double a = rot + i * (M_PI / 2.0);
    px[i] = x + (int)(cos(a) * r);
    py[i] = y + (int)(sin(a) * r * 0.72);
  }
  // Incoming silver ray
  thickLineRGBA(surface, x - (int)(size * 2.8), y - (int)(size * 0.6), x, y, 2, 235, 242, 255,
                weapon_alpha(150.0 * fade));
  // Refracted spectrum fan
  for (int b = 0; b < 5; b++) {
    double dy = (b - 2) * size * 0.36;
    thickLineRGBA(surface, x, y, x + (int)(size * 3.0), (int)(y + dy), 2, spec_c[b][0],
                  spec_c[b][1], spec_c[b][2], weapon_alpha(145.0 * fade));
  }
  // Translucent crystal prism frame
  for (int i = 0; i < 4; i++) {
    int j = (i + 1) % 4;
    thickLineRGBA(surface, px[i], py[i], px[j], py[j], 2, 215, 235, 255,
                  weapon_alpha(200.0 * fade));
  }
  static const Uint8 core[3] = {240, 248, 255};
  weapon_glow(surface, x, y, size * 0.25, core, 180.0 * fade);
}


// Orbital docking ring: a rotating spoked wheel station with running lights
// and a tiny shuttle approaching along its central axis.
static void sky_event_150(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 rim_c[3] = {135, 165, 200};
  static const Uint8 beacon_c[3] = {255, 185, 95};
  static const Uint8 shuttle_c[3] = {155, 225, 255};
  double rx = size * 1.45;
  double ry = size * 0.62;
  double rot = phase / 1400.0;
  ellipseRGBA(surface, x, y, (int)rx, (int)ry, rim_c[0], rim_c[1], rim_c[2],
              weapon_alpha(150.0 * fade));
  ellipseRGBA(surface, x, y, (int)(rx * 0.72), (int)(ry * 0.72), rim_c[0], rim_c[1], rim_c[2],
              weapon_alpha(105.0 * fade));
  for (int i = 0; i < 6; i++) {
    double a = rot + i * (M_PI / 3.0);
    double sx = x + cos(a) * rx;
    double sy = y + sin(a) * ry;
    lineRGBA(surface, x, y, (int)sx, (int)sy, rim_c[0], rim_c[1], rim_c[2],
             weapon_alpha(115.0 * fade));
    if ((i + (int)(phase / 420)) % 2 == 0) {
      weapon_glow(surface, sx, sy, 1.3, beacon_c, 150.0 * fade);
    }
  }
  filledCircleRGBA(surface, x, y, 2, 220, 235, 255, weapon_alpha(200.0 * fade));
  // Tiny shuttle gliding toward the hub
  double sh_off = (1.0 - progress) * size * 1.6;
  double sh_x = x - direction * sh_off;
  double sh_y = y + size * 0.35 * (1.0 - progress);
  weapon_glow(surface, sh_x, sh_y, 1.2, shuttle_c, 175.0 * fade);
}


// Ion storm arc: twin charged plasma nodes crackle across the upper sky with
// flickering electric-violet filaments between them.
static void sky_event_151(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 node_c[3] = {155, 210, 255};
  double sep = size * 1.8;
  double ax = x - sep, ay = y + sin(phase / 520.0) * size * 0.3;
  double bx = x + sep, by = y - sin(phase / 520.0) * size * 0.3;
  weapon_glow(surface, ax, ay, size * 0.25, node_c, 185.0 * fade);
  weapon_glow(surface, bx, by, size * 0.25, node_c, 185.0 * fade);
  Uint32 step = phase / 90;
  for (int f = 0; f < 2; f++) {
    double px = ax, py = ay;
    for (int s = 1; s <= 5; s++) {
      double t = s / 5.0;
      double nx = ax + (bx - ax) * t;
      double jitter =
          (s < 5) ? (((int)(fx_hash(seed + f * 31, step * 7 + s) % 100) - 50) / 50.0) * size * 0.48
                  : 0.0;
      double ny = ay + (by - ay) * t + jitter;
      thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, f == 0 ? 2 : 1, f == 0 ? 185 : 225,
                    f == 0 ? 140 : 235, 255, weapon_alpha((f == 0 ? 145.0 : 190.0) * fade));
      px = nx;
      py = ny;
    }
  }
}


// Solar corona flare: a distant eclipsed sunlet in the upper sky erupting
// with three slowly curling golden-amber prominence loops.
static void sky_event_152(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 flare_c[3] = {255, 175, 85};
  static const Uint8 rim_c[3] = {255, 225, 145};
  double r = size * 0.72;
  weapon_glow(surface, x, y, r * 1.65, flare_c, 95.0 * fade);
  for (int i = 0; i < 3; i++) {
    double base_a = progress * 0.9 + i * (2.0 * M_PI / 3.0);
    double lx = x + cos(base_a) * r * 1.45;
    double ly = y + sin(base_a) * r * 1.45;
    double p1x = x + cos(base_a - 0.35) * r * 0.92;
    double p1y = y + sin(base_a - 0.35) * r * 0.92;
    double p2x = x + cos(base_a + 0.35) * r * 0.92;
    double p2y = y + sin(base_a + 0.35) * r * 0.92;
    thickLineRGBA(surface, (int)p1x, (int)p1y, (int)lx, (int)ly, 2, flare_c[0], flare_c[1],
                  flare_c[2], weapon_alpha(150.0 * fade));
    thickLineRGBA(surface, (int)lx, (int)ly, (int)p2x, (int)p2y, 2, flare_c[0], flare_c[1],
                  flare_c[2], weapon_alpha(150.0 * fade));
  }
  filledCircleRGBA(surface, x, y, (int)r, 16, 18, 28, weapon_alpha(235.0 * fade));
  circleRGBA(surface, x, y, (int)r + 1, rim_c[0], rim_c[1], rim_c[2], weapon_alpha(195.0 * fade));
}


// Nebula butterfly: a bipolar cosmic cloud with twin glowing magenta-cyan
// wing lobes and a bright central white-dwarf core.
static void sky_event_153(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 lobe_l[3] = {225, 115, 215};
  static const Uint8 lobe_r[3] = {115, 210, 255};
  static const Uint8 core_c[3] = {235, 248, 255};
  double span = size * 1.15;
  double wing_r = size * 0.78;
  weapon_glow(surface, x - span * 0.65, y - size * 0.28, wing_r, lobe_l, 85.0 * fade);
  weapon_glow(surface, x - span * 0.65, y + size * 0.28, wing_r * 0.8, lobe_l, 75.0 * fade);
  weapon_glow(surface, x + span * 0.65, y - size * 0.28, wing_r, lobe_r, 85.0 * fade);
  weapon_glow(surface, x + span * 0.65, y + size * 0.28, wing_r * 0.8, lobe_r, 75.0 * fade);
  weapon_glow(surface, x, y, size * 0.32, core_c, 210.0 * fade);
}


// Comet shower: a tight cluster of three icy cyan-white cometoids streaking
// diagonally in formation with faint parallel dust tails.
static void sky_event_154(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 head_c[3] = {205, 240, 255};
  static const Uint8 tail_c[3] = {125, 190, 245};
  for (int i = 0; i < 3; i++) {
    double ox = (i - 1) * size * 0.95;
    double oy = (i == 1 ? -0.45 : 0.35) * size;
    double cx = x + ox, cy = y + oy;
    double tx = cx - direction * size * 2.6;
    double ty = cy - size * 0.75;
    thickLineRGBA(surface, (int)cx, (int)cy, (int)tx, (int)ty, 2, tail_c[0], tail_c[1], tail_c[2],
                  weapon_alpha(120.0 * fade));
    weapon_glow(surface, cx, cy, 1.5, head_c, 200.0 * fade);
  }
}


// Starlight prism: a floating crystalline octahedron in the upper sky slowly
// tumbling and refracting a faint rainbow spectrum downward.
static void sky_event_155(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double r = size * 0.95;
  double tilt = sin(phase / 900.0) * r * 0.4;
  static const Uint8 rim_c[3] = {195, 235, 255};
  lineRGBA(surface, x, (int)(y - r), (int)(x + tilt + r * 0.65), y, rim_c[0], rim_c[1], rim_c[2],
           weapon_alpha(200.0 * fade));
  lineRGBA(surface, (int)(x + tilt + r * 0.65), y, x, (int)(y + r), rim_c[0], rim_c[1], rim_c[2],
           weapon_alpha(200.0 * fade));
  lineRGBA(surface, x, (int)(y + r), (int)(x + tilt - r * 0.65), y, rim_c[0], rim_c[1], rim_c[2],
           weapon_alpha(200.0 * fade));
  lineRGBA(surface, (int)(x + tilt - r * 0.65), y, x, (int)(y - r), rim_c[0], rim_c[1], rim_c[2],
           weapon_alpha(200.0 * fade));
  lineRGBA(surface, (int)(x + tilt - r * 0.65), y, (int)(x + tilt + r * 0.65), y, rim_c[0],
           rim_c[1], rim_c[2], weapon_alpha(140.0 * fade));
  static const Uint8 spec[3][3] = {{255, 145, 145}, {155, 245, 185}, {145, 195, 255}};
  for (int i = 0; i < 3; i++) {
    double dx = (i - 1) * size * 0.45;
    lineRGBA(surface, x, (int)(y + r * 0.4), (int)(x + dx), (int)(y + r * 2.1), spec[i][0],
             spec[i][1], spec[i][2], weapon_alpha(135.0 * fade));
  }
}


// Celestial compass: a rotating golden astrolabe ring in the upper sky with
// four cardinal pointers and a bright pole-star center.
static void sky_event_156(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 gold_c[3] = {245, 205, 125};
  double r = size * 1.15;
  double rot = phase / 1200.0;
  circleRGBA(surface, x, y, (int)r, gold_c[0], gold_c[1], gold_c[2], weapon_alpha(180.0 * fade));
  circleRGBA(surface, x, y, (int)(r * 0.68), gold_c[0], gold_c[1], gold_c[2],
             weapon_alpha(120.0 * fade));
  for (int i = 0; i < 4; i++) {
    double a = rot + i * (M_PI * 0.5);
    lineRGBA(surface, (int)(x + cos(a) * r * 0.45), (int)(y + sin(a) * r * 0.45),
             (int)(x + cos(a) * r * 1.25), (int)(y + sin(a) * r * 1.25), gold_c[0], gold_c[1],
             gold_c[2], weapon_alpha(190.0 * fade));
  }
  weapon_glow(surface, x, y, 1.5, gold_c, 200.0 * fade);
}


// Sky jellyfish bloom: three translucent bioluminescent medusae pulsing
// gently across the upper sky with trailing tentacles.
static void sky_event_157(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 jelly_c[3] = {135, 225, 255};
  for (int j = 0; j < 3; j++) {
    double jx = x + (j - 1) * size * 1.45;
    double jy = y + (j == 1 ? -0.4 : 0.4) * size + sin(phase / 420.0 + j) * 4.0;
    double pulse = 0.8 + 0.2 * sin(phase / 280.0 + j * 1.3);
    double r = size * 0.55 * pulse;
    filledEllipseRGBA(surface, (int)jx, (int)jy, (int)r, (int)(r * 0.65), jelly_c[0], jelly_c[1],
                      jelly_c[2], weapon_alpha(135.0 * fade));
    for (int t = -1; t <= 1; t++) {
      double tx = jx + t * r * 0.45;
      double sway = sin(phase / 320.0 + t + j) * size * 0.25;
      lineRGBA(surface, (int)tx, (int)(jy + r * 0.4), (int)(tx - direction * size * 0.35 + sway),
               (int)(jy + size * 1.35), jelly_c[0], jelly_c[1], jelly_c[2],
               weapon_alpha(115.0 * fade));
    }
  }
}


// Twin eclipse: a dark moonlet crossing a pale amber star, leaving a bright
// diamond-ring flare along one limb.
static void sky_event_158(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 sun_c[3] = {255, 210, 135};
  double r = size * 0.85;
  weapon_glow(surface, x, y, r * 1.4, sun_c, 130.0 * fade);
  filledCircleRGBA(surface, x, y, (int)r, 250, 200, 120, weapon_alpha(220.0 * fade));
  double off = (progress - 0.5) * r * 0.9;
  filledCircleRGBA(surface, (int)(x + off), y, (int)(r * 0.92), 14, 18, 30,
                   weapon_alpha(245.0 * fade));
  weapon_glow(surface, x - r * 0.72, y - r * 0.35, 2.2, sun_c, 225.0 * fade);
}


// Starlight loom: a geometric web of fine silver-blue threads woven between
// four drifting star nodes.
static void sky_event_159(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 node_c[3] = {185, 225, 255};
  double pts[4][2];
  for (int i = 0; i < 4; i++) {
    double a = phase / 1400.0 + i * (M_PI * 0.5);
    double rx = size * (1.25 + 0.35 * sin(phase / 800.0 + i));
    pts[i][0] = x + cos(a) * rx;
    pts[i][1] = y + sin(a) * rx * 0.65;
  }
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      lineRGBA(surface, (int)pts[i][0], (int)pts[i][1], (int)pts[j][0], (int)pts[j][1], node_c[0],
               node_c[1], node_c[2], weapon_alpha(120.0 * fade));
    }
    weapon_glow(surface, pts[i][0], pts[i][1], 1.4, node_c, 195.0 * fade);
  }
}


// Magnetosphere ribbon: twin undulating violet-cyan magnetic field arcs
// curving across the upper sky.
static void sky_event_160(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 c0[3] = {145, 215, 255};
  static const Uint8 c1[3] = {205, 135, 245};
  double span = size * 3.6;
  for (int band = 0; band < 2; band++) {
    const Uint8 *c = band == 0 ? c0 : c1;
    double px = x - span * 0.5;
    double py = y + (band == 0 ? -size * 0.35 : size * 0.35);
    for (int i = 1; i <= 14; i++) {
      double u = i / 14.0;
      double nx = x - span * 0.5 + u * span;
      double ny = y + (band == 0 ? -1 : 1) * size * 0.35 +
                  sin(u * M_PI * 2.0 + phase / 650.0 + band) * size * 0.55;
      thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 2, c[0], c[1], c[2],
                    weapon_alpha(145.0 * fade));
      px = nx;
      py = ny;
    }
  }
}


// Orbital ringlet: a tilted halo of shimmering ice-dust moonlets rotating
// smoothly around a faint central gravity well.
static void sky_event_161(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 ice_c[3] = {185, 230, 255};
  double rx = size * 1.65;
  double ry = size * 0.48;
  ellipseRGBA(surface, x, y, (int)rx, (int)ry, 95, 135, 185, weapon_alpha(95.0 * fade));
  for (int i = 0; i < 8; i++) {
    double a = phase / 950.0 + i * (M_PI / 4.0);
    double mx = x + cos(a) * rx;
    double my = y + sin(a) * ry;
    weapon_glow(surface, mx, my, (i % 2 == 0 ? 1.4 : 1.0), ice_c, 185.0 * fade);
  }
}


// Void manta: a wide translucent indigo-cyan sky-ray gliding silently with
// softly undulating wingtips.
static void sky_event_162(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double wing = size * 1.75;
  double flap = sin(phase / 420.0) * size * 0.45;
  double body = size * 0.65;
  Uint8 m_a = weapon_alpha(175.0 * fade);
  filledTrigonRGBA(surface, (int)(x + direction * body), y, x, (int)(y - wing * 0.65 + flap),
                   (int)(x - direction * body), y, 55, 115, 175, m_a);
  filledTrigonRGBA(surface, (int)(x + direction * body), y, x, (int)(y + wing * 0.65 - flap),
                   (int)(x - direction * body), y, 45, 95, 155, m_a);
  lineRGBA(surface, (int)(x - direction * body), y, (int)(x - direction * body * 2.1),
           (int)(y + sin(phase / 300.0) * size * 0.2), 115, 210, 245, weapon_alpha(150.0 * fade));
}


// Harmonic chimes: five vertical glowing golden tuning bars suspended in the
// upper sky, pulsing in a traveling wave.
static void sky_event_163(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 chime_c[3] = {255, 215, 125};
  for (int i = -2; i <= 2; i++) {
    double cx = x + i * size * 0.55;
    double h = size * (1.15 - fabs((double)i) * 0.18);
    double pulse = 0.45 + 0.55 * sin(phase / 380.0 - i * 0.8);
    thickLineRGBA(surface, (int)cx, (int)(y - h * 0.5), (int)cx, (int)(y + h * 0.5), 2, chime_c[0],
                  chime_c[1], chime_c[2], weapon_alpha(185.0 * pulse * fade));
    weapon_glow(surface, cx, y - h * 0.5, 1.1, chime_c, 160.0 * pulse * fade);
  }
}


// Supernova remnant: an expanding filamentary shell of rose-amber and teal
// gas around a bright pulsar core.
static void sky_event_164(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 gas_a[3] = {245, 145, 175};
  static const Uint8 gas_b[3] = {115, 215, 225};
  static const Uint8 core_c[3] = {235, 248, 255};
  double r = size * (0.85 + 0.35 * progress);
  circleRGBA(surface, x, y, (int)r, gas_a[0], gas_a[1], gas_a[2], weapon_alpha(135.0 * fade));
  circleRGBA(surface, x, y, (int)(r * 0.72), gas_b[0], gas_b[1], gas_b[2],
             weapon_alpha(135.0 * fade));
  for (int i = 0; i < 6; i++) {
    double a = phase / 1600.0 + i * (M_PI / 3.0);
    weapon_glow(surface, x + cos(a) * r * 0.85, y + sin(a) * r * 0.85, 1.3,
                i % 2 == 0 ? gas_a : gas_b, 145.0 * fade);
  }
  weapon_glow(surface, x, y, 2.0, core_c, 220.0 * fade);
}


// Solar kite: a diamond-shaped golden-amber solar sailer riding a faint
// turquoise ion wake.
static void sky_event_165(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 sail_c[3] = {245, 205, 115};
  static const Uint8 wake_c[3] = {115, 225, 235};
  double r = size * 1.1;
  filledTrigonRGBA(surface, (int)(x + direction * r), y, x, (int)(y - r * 0.7),
                   (int)(x - direction * r * 0.5), y, sail_c[0], sail_c[1], sail_c[2],
                   weapon_alpha(175.0 * fade));
  filledTrigonRGBA(surface, (int)(x + direction * r), y, x, (int)(y + r * 0.7),
                   (int)(x - direction * r * 0.5), y, sail_c[0] - 30, sail_c[1] - 30,
                   sail_c[2] - 20, weapon_alpha(175.0 * fade));
  thickLineRGBA(surface, (int)(x - direction * r * 0.5), y, (int)(x - direction * r * 2.4), y, 2,
                wake_c[0], wake_c[1], wake_c[2], weapon_alpha(135.0 * fade));
  weapon_glow(surface, x - direction * r * 0.5, y, 1.2, wake_c, 195.0 * fade);
}


// Pulsar cross: a spinning neutron star with twin opposing cyan-white
// relativistic jets and a faint equatorial ring.
static void sky_event_166(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 jet_c[3] = {165, 235, 255};
  double rot = phase / 650.0;
  double jlen = size * 1.9;
  thickLineRGBA(surface, (int)(x - cos(rot) * jlen), (int)(y - sin(rot) * jlen),
                (int)(x + cos(rot) * jlen), (int)(y + sin(rot) * jlen), 2, jet_c[0], jet_c[1],
                jet_c[2], weapon_alpha(175.0 * fade));
  ellipseRGBA(surface, x, y, (int)(size * 0.9), (int)(size * 0.35), 140, 195, 240,
              weapon_alpha(120.0 * fade));
  weapon_glow(surface, x, y, 1.8, jet_c, 225.0 * fade);
}


// Cloud whale: a gentle dark-blue leviathan silhouette gliding through the
// upper sky with glowing bioluminescent flank markings.
static void sky_event_167(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 bio_c[3] = {135, 235, 225};
  double rx = size * 1.9;
  double ry = size * 0.58;
  filledEllipseRGBA(surface, x, y, (int)rx, (int)ry, 26, 38, 62, weapon_alpha(225.0 * fade));
  double tail_x = x - direction * rx * 1.15;
  double fluke = sin(phase / 450.0) * size * 0.3;
  filledTrigonRGBA(surface, (int)(x - direction * rx * 0.75), y,
                   (int)(tail_x - direction * size * 0.4), (int)(y - size * 0.45 + fluke),
                   (int)(tail_x - direction * size * 0.4), (int)(y + size * 0.45 + fluke), 26, 38,
                   62, weapon_alpha(225.0 * fade));
  for (int i = -1; i <= 1; i++) {
    weapon_glow(surface, x + i * size * 0.55, y + size * 0.1, 1.0, bio_c, 165.0 * fade);
  }
}


// Meteoric rosary: a curved string of five glowing amber-white embers
// drifting in formation.
static void sky_event_168(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 bead_c[3] = {255, 215, 145};
  for (int i = 0; i < 5; i++) {
    double u = (i - 2) * 0.55;
    double bx = x + u * size * 1.1;
    double by = y + (u * u - 1.0) * size * 0.35;
    weapon_glow(surface, bx, by, 1.5 - fabs(u) * 0.35, bead_c, (200.0 - fabs(u) * 35.0) * fade);
  }
}


// Binary pulsar: two tight cyan-white neutron stars waltzing around a shared
// center with sweeping faint beam cones.
static void sky_event_169(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 star_c[3] = {165, 235, 255};
  double a = phase / 450.0;
  double sep = size * 0.75;
  for (int i = 0; i < 2; i++) {
    double ang = a + i * M_PI;
    double sx = x + cos(ang) * sep;
    double sy = y + sin(ang) * sep * 0.55;
    weapon_glow(surface, sx, sy, 1.4, star_c, 210.0 * fade);
  }
  circleRGBA(surface, x, y, (int)sep, 115, 175, 225, weapon_alpha(75.0 * fade));
}


// Sky lantern fleet: five warm paper-gold lanterns drifting upward in a
// gentle diagonal cluster.
static void sky_event_170(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 lan_c[3] = {255, 195, 105};
  for (int i = 0; i < 5; i++) {
    double lx = x + (i - 2) * size * 0.85;
    double ly = y + sin(phase / 500.0 + i) * size * 0.35 - progress * size * 0.8;
    boxRGBA(surface, (int)(lx - 3), (int)(ly - 4), (int)(lx + 3), (int)(ly + 4), lan_c[0], lan_c[1],
            lan_c[2], weapon_alpha(195.0 * fade));
    weapon_glow(surface, lx, ly + 2, 1.2, lan_c, 175.0 * fade);
  }
}


// Chrono-ring: three concentric pale-gold astrolabe arcs rotating in
// opposite directions around a quiet star.
static void sky_event_171(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 arc_c[3] = {235, 205, 145};
  for (int r = 0; r < 3; r++) {
    double rad = size * (0.65 + r * 0.42);
    double dir = (r % 2 == 0) ? 1.0 : -1.0;
    double a0 = phase / (800.0 + r * 250.0) * dir;
    for (int s = 0; s < 6; s++) {
      double t0 = a0 + s * 0.35;
      double t1 = t0 + 0.25;
      lineRGBA(surface, (int)(x + cos(t0) * rad), (int)(y + sin(t0) * rad),
               (int)(x + cos(t1) * rad), (int)(y + sin(t1) * rad), arc_c[0], arc_c[1], arc_c[2],
               weapon_alpha(165.0 * fade));
    }
  }
  weapon_glow(surface, x, y, 1.3, arc_c, 195.0 * fade);
}


// Aurora crown: a shimmering overhead halo of emerald-teal rays fanning
// outward from a zenith point.
static void sky_event_172(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 ray_c[3] = {115, 245, 195};
  for (int i = 0; i < 10; i++) {
    double a = i * (2.0 * M_PI / 10.0) + phase / 2200.0;
    double r0 = size * 0.45;
    double r1 = size * (1.25 + 0.35 * sin(phase / 480.0 + i * 1.1));
    thickLineRGBA(surface, (int)(x + cos(a) * r0), (int)(y + sin(a) * r0), (int)(x + cos(a) * r1),
                  (int)(y + sin(a) * r1), 2, ray_c[0], ray_c[1], ray_c[2],
                  weapon_alpha(135.0 * fade));
  }
}


// Orbital mirror array: three golden hexagonal reflector satellites
// drifting in formation and glinting in starlight.
static void sky_event_173(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 mir_c[3] = {255, 215, 130};
  for (int i = -1; i <= 1; i++) {
    double mx = x + i * size * 1.25;
    double my = y + (i == 0 ? -size * 0.35 : size * 0.35);
    double r = size * 0.45;
    circleRGBA(surface, (int)mx, (int)my, (int)r, mir_c[0], mir_c[1], mir_c[2],
               weapon_alpha(180.0 * fade));
    double glint = 0.5 + 0.5 * sin(phase / 320.0 + i * 1.9);
    weapon_glow(surface, mx, my, 1.3, mir_c, 195.0 * glint * fade);
  }
}


// Starlight dolphin: a sleek luminous cyan silhouette leaping in a smooth
// parabolic arc across the upper sky.
static void sky_event_174(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 dol_c[3] = {125, 225, 255};
  double arc = sin(progress * M_PI) * size * 1.2;
  double dy = y - arc;
  filledEllipseRGBA(surface, x, (int)dy, (int)(size * 1.25), (int)(size * 0.42), 55, 145, 205,
                    weapon_alpha(195.0 * fade));
  filledTrigonRGBA(surface, x, (int)(dy - size * 0.35), (int)(x - direction * size * 0.45),
                   (int)(dy - size * 0.85), (int)(x - direction * size * 0.35),
                   (int)(dy - size * 0.25), 55, 145, 205, weapon_alpha(195.0 * fade));
  weapon_glow(surface, x + direction * size * 0.8, dy, 1.1, dol_c, 185.0 * fade);
}


// Plasma ring: an expanding luminous violet-blue torus drifting slowly
// across the upper sky.
static void sky_event_175(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 ring_c[3] = {185, 155, 255};
  double r = size * (0.75 + 0.65 * progress);
  circleRGBA(surface, x, y, (int)r, ring_c[0], ring_c[1], ring_c[2], weapon_alpha(175.0 * fade));
  circleRGBA(surface, x, y, (int)(r * 0.82), 135, 215, 255, weapon_alpha(135.0 * fade));
}


// Star-weaver moth: a pale luminescent luna moth with twin sweeping
// emerald-white wings fluttering across the sky.
static void sky_event_176(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 wing_c[3] = {185, 250, 210};
  double flap = 0.45 + 0.55 * fabs(sin(phase / 190.0));
  double w = size * 1.35 * flap;
  double h = size * 0.95;
  filledTrigonRGBA(surface, x, y, (int)(x - w), (int)(y - h * 0.7), (int)(x - w * 0.65),
                   (int)(y + h * 0.6), wing_c[0], wing_c[1], wing_c[2], weapon_alpha(170.0 * fade));
  filledTrigonRGBA(surface, x, y, (int)(x + w), (int)(y - h * 0.7), (int)(x + w * 0.65),
                   (int)(y + h * 0.6), wing_c[0], wing_c[1], wing_c[2], weapon_alpha(170.0 * fade));
  weapon_glow(surface, x, y, 1.1, wing_c, 200.0 * fade);
}


// Jellyfish caravan: four tiny lavender-cyan medusae drifting in a diagonal
// string across the upper sky.
static void sky_event_177(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 jel_c[3] = {175, 195, 255};
  for (int i = 0; i < 4; i++) {
    double jx = x + (i - 1.5) * size * 0.9;
    double jy = y + (i - 1.5) * size * 0.35 + sin(phase / 350.0 + i) * 3.0;
    double r = size * (0.36 - i * 0.03);
    filledEllipseRGBA(surface, (int)jx, (int)jy, (int)r, (int)(r * 0.65), jel_c[0], jel_c[1],
                      jel_c[2], weapon_alpha(150.0 * fade));
    lineRGBA(surface, (int)jx, (int)(jy + r * 0.5), (int)(jx - direction * size * 0.3),
             (int)(jy + size * 0.9), jel_c[0], jel_c[1], jel_c[2], weapon_alpha(115.0 * fade));
  }
}


// Corona ring: a dark eclipsed disc ringed by six softly pulsing golden
// prominence flares.
static void sky_event_178(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 flare_c[3] = {255, 195, 105};
  double r = size * 0.78;
  filledCircleRGBA(surface, x, y, (int)r, 14, 16, 26, weapon_alpha(235.0 * fade));
  circleRGBA(surface, x, y, (int)r + 1, flare_c[0], flare_c[1], flare_c[2],
             weapon_alpha(180.0 * fade));
  for (int i = 0; i < 6; i++) {
    double a = phase / 1400.0 + i * (M_PI / 3.0);
    double pulse = 0.65 + 0.35 * sin(phase / 400.0 + i * 1.4);
    weapon_glow(surface, x + cos(a) * (r + 2), y + sin(a) * (r + 2), 1.3 * pulse, flare_c,
                185.0 * fade);
  }
}


// Ribbon comet: a bright emerald-white comet trailing a sinusoidal waving
// ion tail.
static void sky_event_179(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 head_c[3] = {185, 255, 225};
  double px = x, py = y;
  for (int i = 1; i <= 10; i++) {
    double u = i / 10.0;
    double nx = x - direction * u * size * 3.2;
    double ny = y + sin(u * 6.0 - phase / 260.0) * size * 0.38;
    thickLineRGBA(surface, (int)px, (int)py, (int)nx, (int)ny, 2, head_c[0], head_c[1], head_c[2],
                  weapon_alpha((1.0 - u) * 165.0 * fade));
    px = nx;
    py = ny;
  }
  weapon_glow(surface, x, y, 1.6, head_c, 220.0 * fade);
}


// Gravity lens arc: twin pale-blue Einstein arcs curving around an unseen
// dark mass with two lensed star glints.
static void sky_event_180(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 lens_c[3] = {155, 215, 255};
  double r = size * 1.1;
  ellipseRGBA(surface, x, y, (int)r, (int)(r * 0.72), lens_c[0], lens_c[1], lens_c[2],
              weapon_alpha(135.0 * fade));
  weapon_glow(surface, x - r * 0.85, y - r * 0.25, 1.4, lens_c, 200.0 * fade);
  weapon_glow(surface, x + r * 0.85, y + r * 0.25, 1.2, lens_c, 185.0 * fade);
}


// Celestial fan: a delicate fan of seven pale-gold starlight rays
// unfolding and sweeping gently across the upper sky.
static void sky_event_181(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 fan_c[3] = {245, 220, 155};
  double spread = 0.22 + 0.14 * sin(phase / 700.0);
  double len = size * 1.9;
  for (int i = -3; i <= 3; i++) {
    double a = -M_PI * 0.5 + i * spread;
    lineRGBA(surface, x, (int)(y + size * 0.5), (int)(x + cos(a) * len),
             (int)(y + size * 0.5 + sin(a) * len), fan_c[0], fan_c[1], fan_c[2],
             weapon_alpha(145.0 * fade));
    weapon_glow(surface, x + cos(a) * len, y + size * 0.5 + sin(a) * len, 1.0, fan_c, 175.0 * fade);
  }
}


// Comet pair: two icy blue-white comets sweeping in parallel with crisp
// twin tails.
static void sky_event_182(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 head_c[3] = {195, 238, 255};
  for (int i = 0; i < 2; i++) {
    double cx = x + (i == 0 ? -size * 0.6 : size * 0.6);
    double cy = y + (i == 0 ? -size * 0.45 : size * 0.45);
    thickLineRGBA(surface, (int)cx, (int)cy, (int)(cx - direction * size * 2.8),
                  (int)(cy - size * 0.6), 2, 125, 195, 245, weapon_alpha(135.0 * fade));
    weapon_glow(surface, cx, cy, 1.5, head_c, 210.0 * fade);
  }
}


// Star-crown halo: a ring of eight twinkling pearl-cyan stars rotating
// slowly around a soft nebula haze.
static void sky_event_183(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 star_c[3] = {205, 245, 255};
  double r = size * 1.15;
  weapon_glow(surface, x, y, size * 0.7, star_c, 85.0 * fade);
  for (int i = 0; i < 8; i++) {
    double a = phase / 1200.0 + i * (M_PI / 4.0);
    double tw = 0.5 + 0.5 * sin(phase / 250.0 + i * 1.7);
    weapon_glow(surface, x + cos(a) * r, y + sin(a) * r * 0.65, 1.2 * tw + 0.4, star_c,
                185.0 * fade);
  }
}


// Sky-kite squadron: three luminous rose-amber diamond kites gliding in
// chevron formation with fluttering tails.
static void sky_event_184(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 kite_c[3] = {255, 175, 135};
  for (int i = 0; i < 3; i++) {
    double kx = x - direction * abs(i - 1) * size * 1.1;
    double ky = y + (i - 1) * size * 0.75;
    double r = size * 0.45;
    filledTrigonRGBA(surface, (int)(kx + direction * r), (int)ky, (int)kx, (int)(ky - r * 0.6),
                     (int)(kx - direction * r * 0.4), (int)ky, kite_c[0], kite_c[1], kite_c[2],
                     weapon_alpha(180.0 * fade));
    filledTrigonRGBA(surface, (int)(kx + direction * r), (int)ky, (int)kx, (int)(ky + r * 0.6),
                     (int)(kx - direction * r * 0.4), (int)ky, kite_c[0], kite_c[1], kite_c[2],
                     weapon_alpha(180.0 * fade));
    lineRGBA(surface, (int)(kx - direction * r * 0.4), (int)ky, (int)(kx - direction * r * 1.8),
             (int)(ky + sin(phase / 240.0 + i) * size * 0.25), kite_c[0], kite_c[1], kite_c[2],
             weapon_alpha(135.0 * fade));
  }
}


// Sky-whale pod: two luminous indigo-aqua sky-whales gliding in tandem
// across the upper sky.
static void sky_event_185(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 bio_c[3] = {135, 235, 235};
  for (int w = 0; w < 2; w++) {
    double wx = x + (w == 0 ? 0.0 : -direction * size * 2.1);
    double wy = y + (w == 0 ? -size * 0.25 : size * 0.55) + sin(phase / 550.0 + w) * size * 0.2;
    double sc = (w == 0) ? 1.0 : 0.72;
    filledEllipseRGBA(surface, (int)wx, (int)wy, (int)(size * 1.4 * sc), (int)(size * 0.44 * sc),
                      34, 58, 96, weapon_alpha(215.0 * fade));
    weapon_glow(surface, wx + direction * size * 0.4 * sc, wy, 1.1 * sc, bio_c, 175.0 * fade);
  }
}


// Star-crystal snowflake: a six-pointed pale-cyan ice crystal rotating
// serenely in the upper sky.
static void sky_event_186(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 ice_c[3] = {195, 240, 255};
  double r = size * 1.15;
  double rot = phase / 1400.0;
  for (int i = 0; i < 3; i++) {
    double a = rot + i * (M_PI / 3.0);
    lineRGBA(surface, (int)(x - cos(a) * r), (int)(y - sin(a) * r), (int)(x + cos(a) * r),
             (int)(y + sin(a) * r), ice_c[0], ice_c[1], ice_c[2], weapon_alpha(185.0 * fade));
  }
  weapon_glow(surface, x, y, 1.3, ice_c, 200.0 * fade);
}


// Ion ringlet swarm: three tiny glowing violet-blue plasma rings drifting
// in a loose triangle formation.
static void sky_event_187(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 ring_c[3] = {175, 165, 255};
  for (int i = 0; i < 3; i++) {
    double rx = x + (i - 1) * size * 1.05;
    double ry = y + (i == 1 ? -size * 0.55 : size * 0.45);
    double rad = size * (0.38 + 0.08 * sin(phase / 320.0 + i));
    circleRGBA(surface, (int)rx, (int)ry, (int)rad, ring_c[0], ring_c[1], ring_c[2],
               weapon_alpha(185.0 * fade));
  }
}


// Starlight pendulum arc: a sweeping golden-white meteor arc leaving a
// curved trail of five fading sparks.
static void sky_event_188(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 spark_c[3] = {255, 225, 155};
  for (int i = 0; i < 5; i++) {
    double u = i * 0.35;
    double sx = x - direction * u * size * 1.5;
    double sy = y - u * u * size * 0.25;
    weapon_glow(surface, sx, sy, 1.6 - i * 0.25, spark_c, (210.0 - i * 35.0) * fade);
  }
}


// V-formation geese: a crisp chevron of 9 migratory birds with synchronized,
// slightly staggered wingbeats.
static void sky_event_189(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 9; i++) {
    int rank = (i + 1) / 2;
    int side = (i == 0) ? 0 : ((i % 2 == 1) ? -1 : 1);
    double bx = x - direction * rank * size * 0.85;
    double by = y + side * rank * size * 0.55;
    double flap = sin(phase / 190.0 - rank * 0.45) * 0.85;
    draw_sleek_bird(surface, bx, by, size * 0.85, direction, flap, 0.0, 32, 42, 60,
                    weapon_alpha(205.0 * fade), true);
  }
}


// Moonlit albatross: a wide-winged oceanic glider banking gently on long
// slender swept wings.
static void sky_event_190(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double pitch = sin(phase / 900.0) * 0.14;
  double flap = 0.22 + sin(phase / 650.0) * 0.16;
  draw_sleek_bird(surface, x, y, size * 1.9, direction, flap, pitch, 195, 210, 230,
                  weapon_alpha(210.0 * fade), true);
}


// Swift squadron: 7 crescent-winged swifts banking together in tight echelon.
static void sky_event_191(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 7; i++) {
    double bx = x - direction * i * size * 0.65;
    double by = y + (i - 3) * size * 0.35 + sin(phase / 320.0 + i * 0.3) * size * 0.18;
    double flap = sin(phase / 110.0 + i * 0.7) * 0.9;
    draw_sleek_bird(surface, bx, by, size * 0.62, direction, flap,
                    cos(phase / 320.0 + i * 0.3) * 0.18, 28, 36, 52, weapon_alpha(200.0 * fade),
                    true);
  }
}


// Crane migration: a long undulating diagonal line of 8 graceful cranes.
static void sky_event_192(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 8; i++) {
    double bx = x - direction * i * size * 0.95;
    double by = y + (i - 3.5) * size * 0.42 + sin(phase / 500.0 + i * 0.6) * size * 0.2;
    double flap = sin(phase / 260.0 + i * 0.5) * 0.82;
    draw_sleek_bird(surface, bx, by, size * 1.05, direction, flap, -0.06, 175, 190, 215,
                    weapon_alpha(195.0 * fade), true);
  }
}


// Starlight murmuration: a fluid twisting cloud of 28 small birds morphing
// smoothly across the upper sky.
static void sky_event_193(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 28; i++) {
    double a = i * 0.48 + phase / 450.0;
    double r = size * (0.4 + 0.85 * sin(i * 0.37 + phase / 620.0));
    double bx = x + cos(a) * r * 1.4;
    double by = y + sin(a * 1.3) * r * 0.65;
    double flap = sin(phase / 120.0 + i * 1.1) * 0.9;
    draw_sleek_bird(surface, bx, by, size * 0.36, direction, flap, sin(a) * 0.15, 26, 34, 48,
                    weapon_alpha(185.0 * fade), false);
  }
}


// Twin hawks: two agile hawks spiraling around each other in aerial chase.
static void sky_event_194(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 2; i++) {
    double a = phase / 280.0 + i * M_PI;
    double bx = x + cos(a) * size * 0.55;
    double by = y + sin(a) * size * 0.65;
    double flap = sin(phase / 150.0 + i * 1.7) * 0.85;
    draw_sleek_bird(surface, bx, by, size * 0.95, direction, flap, cos(a) * 0.25, 34, 42, 58,
                    weapon_alpha(215.0 * fade), true);
  }
}


// Night owl patrol: a large pale-bellied owl flying silent and steady with
// deep, slow wingstrokes.
static void sky_event_195(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double by = y + sin(phase / 420.0) * size * 0.25;
  double flap = sin(phase / 270.0) * 0.88;
  draw_sleek_bird(surface, x, by, size * 1.35, direction, flap, 0.0, 195, 205, 225,
                  weapon_alpha(205.0 * fade), true);
}


// Condor thermal: a massive condor with broad wings riding a high-altitude arc.
static void sky_event_196(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double by = y - sin(progress * M_PI) * size * 1.1;
  double pitch = -cos(progress * M_PI) * 0.18;
  double flap = 0.24 + sin(phase / 800.0) * 0.12;
  draw_sleek_bird(surface, x, by, size * 2.0, direction, flap, pitch, 24, 30, 42,
                  weapon_alpha(220.0 * fade), true);
}


// Storm petrels: five small dark seabirds weaving in sinusoidal wave-skimming
// flight across the sky.
static void sky_event_197(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 5; i++) {
    double bx = x - direction * i * size * 0.75;
    double by = y + sin(phase / 220.0 + i * 0.9) * size * 0.65;
    double flap = sin(phase / 130.0 + i * 1.3) * 0.9;
    draw_sleek_bird(surface, bx, by, size * 0.56, direction, flap,
                    cos(phase / 220.0 + i * 0.9) * 0.28, 28, 36, 52, weapon_alpha(195.0 * fade),
                    true);
  }
}


// Raven trio: three sleek ravens flying in a loose staggered trail.
static void sky_event_198(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 3; i++) {
    double bx = x - direction * i * size * 1.1;
    double by = y + (i - 1) * size * 0.55 + sin(phase / 380.0 + i) * size * 0.2;
    double flap = sin(phase / 200.0 + i * 1.4) * 0.85;
    draw_sleek_bird(surface, bx, by, size * 0.92, direction, flap, 0.05, 22, 26, 38,
                    weapon_alpha(215.0 * fade), true);
  }
}


// Aurora bird: a ghostly bioluminescent cyan-emerald spectral bird gliding
// with a faint stardust wake.
static void sky_event_199(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 aura_c[3] = {125, 235, 210};
  for (int i = 1; i <= 5; i++) {
    weapon_glow(surface, x - direction * i * size * 0.55, y + sin(phase / 300.0 + i) * size * 0.12,
                1.2 - i * 0.15, aura_c, (150.0 - i * 24.0) * fade);
  }
  double flap = sin(phase / 240.0) * 0.75;
  draw_sleek_bird(surface, x, y, size * 1.35, direction, flap, 0.0, 145, 245, 220,
                  weapon_alpha(215.0 * fade), true);
}


// Peregrine pursuit: a fast raptor in a shallow power-dive chasing a nimble
// smaller bird that jinks ahead.
static void sky_event_200(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double prey_x = x + direction * size * 1.25;
  double prey_y = y + sin(phase / 150.0) * size * 0.45;
  draw_sleek_bird(surface, prey_x, prey_y, size * 0.50, direction, sin(phase / 95.0) * 0.95,
                  cos(phase / 150.0) * 0.28, 175, 195, 220, weapon_alpha(200.0 * fade), true);
  draw_sleek_bird(surface, x - direction * size * 0.35,
                  y - size * 0.15 + sin(phase / 200.0) * size * 0.25, size * 1.05, direction, -0.35,
                  0.18, 28, 34, 48, weapon_alpha(225.0 * fade), true);
}


// Pelican line: five pelicans gliding in synchronized single-file line.
static void sky_event_201(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double shared_flap = (sin(phase / 700.0) > 0.2) ? sin(phase / 210.0) * 0.8 : 0.15;
  for (int i = 0; i < 5; i++) {
    double bx = x - direction * i * size * 1.05;
    double by = y + sin(phase / 550.0 + i * 0.4) * size * 0.15;
    draw_sleek_bird(surface, bx, by, size * 1.0, direction, shared_flap, 0.0, 165, 180, 205,
                    weapon_alpha(195.0 * fade), true);
  }
}


// Silver-winged tern: three slender arctic terns with long forked tails and
// silver-lit upper wings slicing cleanly through the night air.
static void sky_event_202(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 3; i++) {
    double bx = x - direction * i * size * 0.9;
    double by = y + (i - 1) * size * 0.52;
    double flap = sin(phase / 160.0 + i * 0.9) * 0.88;
    draw_sleek_bird(surface, bx, by, size * 0.82, direction, flap, -0.04, 185, 205, 230,
                    weapon_alpha(205.0 * fade), true);
  }
}


// Constellation flock: seven luminous pale-gold star-birds flying in a loose
// diamond formation, each trailing a tiny pinpoint glimmer.
static void sky_event_203(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 star_c[3] = {245, 220, 150};
  static const int dx[7] = {2, 1, 1, 0, 0, -1, -2};
  static const int dy[7] = {0, -1, 1, -2, 2, 0, 0};
  for (int i = 0; i < 7; i++) {
    double bx = x + direction * dx[i] * size * 0.7;
    double by = y + dy[i] * size * 0.55;
    double flap = sin(phase / 210.0 + i * 0.8) * 0.78;
    weapon_glow(surface, bx - direction * size * 0.35, by, 0.75, star_c, 145.0 * fade);
    draw_sleek_bird(surface, bx, by, size * 0.72, direction, flap, 0.0, 235, 215, 165,
                    weapon_alpha(205.0 * fade), true);
  }
}


// NEW_SKY_EVENTS_ABOVE (new sky_event_NN() functions go above this line)

#define SKY_CLASSIC_VARIANTS 3

// The three classic sky objects: pulsing star, comet and shooting star.
static void sky_event_classic(const SkyEvent *event, int variant) {
  SKY_EVENT_UNPACK(event);
  switch (variant) {
    case 0: {
      int pulse_radius = size + (int)(progress * size * 2);
      filledCircleRGBA(surface, x, y, size / 2 + 1, 245, 233, 207, alpha);
      circleRGBA(surface, x, y, pulse_radius, 162, 193, 213, (Uint8)(alpha * 0.72));
      for (int ray = 0; ray < 8; ray++) {
        double angle = ray * 0.78539816339 + progress;
        int ray_x = x + (int)(cos(angle) * pulse_radius * 1.7);
        int ray_y = y + (int)(sin(angle) * pulse_radius * 1.7);
        thickLineRGBA(surface, x, y, ray_x, ray_y, 1, 180, 203, 220, (Uint8)(alpha * 0.35));
      }
      break;
    }
    case 1: {
      int tail_x = x - direction * size * 4;
      int tail_y = y - size;
      thickLineRGBA(surface, x, y, tail_x, tail_y, 2, 194, 205, 218, alpha);
      filledCircleRGBA(surface, x, y, size / 2, 227, 224, 211, alpha);
      break;
    }
    default: {
      int trail_x = x - direction * size * 4;
      int trail_y = y + size;
      thickLineRGBA(surface, trail_x, trail_y, x, y, 2, 202, 177, 130, alpha);
      filledCircleRGBA(surface, x, y, size / 2, 235, 221, 185, alpha);
      for (int fragment = 1; fragment <= 2; fragment++) {
        int fragment_x = x - direction * size * (fragment + 1);
        int fragment_y = y + size * fragment;
        pixelRGBA(surface, fragment_x, fragment_y, 214, 193, 159, (Uint8)(alpha * 0.75));
      }
      break;
    }
  }
}


static void (*const sky_event_kinds[])(const SkyEvent *) = {
    sky_event_23,  sky_event_24,  sky_event_25,  sky_event_26,  sky_event_27,  sky_event_28,
    sky_event_29,  sky_event_30,  sky_event_31,  sky_event_32,  sky_event_33,  sky_event_34,
    sky_event_35,  sky_event_36,  sky_event_37,  sky_event_38,  sky_event_39,  sky_event_40,
    sky_event_41,  sky_event_42,  sky_event_43,  sky_event_44,  sky_event_45,  sky_event_46,
    sky_event_47,  sky_event_48,  sky_event_49,  sky_event_50,  sky_event_51,  sky_event_52,
    sky_event_53,  sky_event_54,  sky_event_55,  sky_event_56,  sky_event_57,  sky_event_58,
    sky_event_59,  sky_event_60,  sky_event_61,  sky_event_62,  sky_event_63,  sky_event_64,
    sky_event_65,  sky_event_66,  sky_event_67,  sky_event_68,  sky_event_69,  sky_event_70,
    sky_event_71,  sky_event_72,  sky_event_73,  sky_event_74,  sky_event_75,  sky_event_76,
    sky_event_77,  sky_event_78,  sky_event_79,  sky_event_80,  sky_event_81,  sky_event_82,
    sky_event_83,  sky_event_84,  sky_event_85,  sky_event_86,  sky_event_87,  sky_event_88,
    sky_event_89,  sky_event_90,  sky_event_91,  sky_event_92,  sky_event_93,  sky_event_94,
    sky_event_95,  sky_event_96,  sky_event_97,  sky_event_98,  sky_event_99,  sky_event_100,
    sky_event_101, sky_event_102, sky_event_103, sky_event_104, sky_event_105, sky_event_106,
    sky_event_107, sky_event_108, sky_event_109, sky_event_110, sky_event_111, sky_event_112,
    sky_event_113, sky_event_114, sky_event_115, sky_event_116, sky_event_117, sky_event_118,
    sky_event_119, sky_event_120, sky_event_121, sky_event_122, sky_event_123, sky_event_124,
    sky_event_125, sky_event_126, sky_event_127, sky_event_128, sky_event_129, sky_event_130,
    sky_event_131, sky_event_132, sky_event_133, sky_event_134, sky_event_135, sky_event_136,
    sky_event_137, sky_event_138, sky_event_139, sky_event_140, sky_event_141, sky_event_142,
    sky_event_143, sky_event_144, sky_event_145, sky_event_146, sky_event_147, sky_event_148,
    sky_event_149, sky_event_150, sky_event_151, sky_event_152, sky_event_153, sky_event_154,
    sky_event_155, sky_event_156, sky_event_157, sky_event_158, sky_event_159, sky_event_160,
    sky_event_161, sky_event_162, sky_event_163, sky_event_164, sky_event_165, sky_event_166,
    sky_event_167, sky_event_168, sky_event_169, sky_event_170, sky_event_171, sky_event_172,
    sky_event_173, sky_event_174, sky_event_175, sky_event_176, sky_event_177, sky_event_178,
    sky_event_179, sky_event_180, sky_event_181, sky_event_182, sky_event_183, sky_event_184,
    sky_event_185, sky_event_186, sky_event_187, sky_event_188, sky_event_189, sky_event_190,
    sky_event_191, sky_event_192, sky_event_193, sky_event_194, sky_event_195, sky_event_196,
    sky_event_197, sky_event_198, sky_event_199, sky_event_200, sky_event_201, sky_event_202,
    sky_event_203,
    // NEW_SKY_KINDS_ABOVE
};
static_assert(RARE_SKY_CLASSIC_KINDS + sizeof(sky_event_kinds) / sizeof(sky_event_kinds[0]) ==
                  RARE_SKY_KINDS,
              "RARE_SKY_KINDS does not match sky_event_kinds[]");

static void render_rare_sky_event(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_RARE_SKY, ticks, &window)) return;

  SkyEvent event;
  event.surface = surface;
  event.ticks = ticks;
  event.scale = scale;
  event.seed = window.seed;
  event.phase = window.phase;
  event.progress = window.progress;
  event.fade = window.fade;
  event.direction = (window.seed & 1) ? 1 : -1;
  event.y = surface->h * (8 + (window.seed >> 8) % 39) / 100;
  int margin = scale / 12;
  event.x = event.direction > 0
                ? -margin + (int)(window.progress * (surface->w + 2.0 * margin))
                : surface->w + margin - (int)(window.progress * (surface->w + 2.0 * margin));
  event.size = scale / (26 + (window.seed >> 16) % 24);
  if (event.size < 4) event.size = 4;
  event.alpha = (Uint8)(150 * window.fade);

  // Kinds >= RARE_SKY_CLASSIC_KINDS are thinned out by rare_sky_accept().
  int kind = effect_kind(&EFFECT_RARE_SKY, (window.seed >> 24) % RARE_SKY_KINDS);
  if (kind < RARE_SKY_CLASSIC_KINDS) {
    sky_event_classic(&event, kind % SKY_CLASSIC_VARIANTS);
  } else {
    sky_event_kinds[kind - RARE_SKY_CLASSIC_KINDS](&event);
  }
}


static void render_foreground_event(RenderTarget *surface, Uint32 ticks, int scale) {
  EffectWindow window;
  if (!effect_window(&EFFECT_FOREGROUND, ticks, &window)) return;
  Uint32 seed = window.seed;
  int kind = effect_kind(&EFFECT_FOREGROUND, seed % 4);
  double progress = window.progress;
  double fade = window.fade;
  int count = 24 + kind * 7;
  for (int particle = 0; particle < count; particle++) {
    Uint32 particle_seed = (Uint32)particle * 1664525u + seed;
    int start_x = particle_seed % surface->w;
    int start_y = (particle_seed >> 9) % surface->h;
    int speed = 3 + (particle_seed >> 20) % 8;
    int drift = (int)(ticks / (30 + speed * 7));
    int particle_x = (start_x + drift + surface->w) % surface->w;
    int particle_y = start_y;
    Uint8 red = 191, green = 204, blue = 215;
    Uint8 alpha = (Uint8)((18 + (particle_seed >> 24) % 32) * fade);
    if (kind == 0) {
      particle_y = (start_y + drift * 2) % surface->h;
      particle_x += (int)(sin(ticks / 500.0 + particle) * scale * 0.02);
      red = 207;
      green = (Uint8)(91 + particle % 5 * 18);
      blue = 54;
      thickLineRGBA(surface, particle_x, particle_y, particle_x - 2, particle_y + 4, 1, red, green,
                    blue, alpha);
    } else if (kind == 1) {
      particle_y = (start_y - drift + surface->h) % surface->h;
      red = 245;
      green = (Uint8)(112 + particle % 4 * 18);
      blue = 67;
      filledCircleRGBA(surface, particle_x, particle_y, 1 + particle % 2, red, green, blue, alpha);
    } else if (kind == 2) {
      particle_y = (start_y + drift * 3) % surface->h;
      red = 167;
      green = 213;
      blue = 231;
      filledCircleRGBA(surface, particle_x, particle_y, 1 + particle % 2, red, green, blue, alpha);
    } else {
      particle_y = surface->h * (28 + particle_seed % 58) / 100 -
                   (int)(progress * scale * (0.025 + particle % 3 * 0.01));
      int radius = scale / 230;
      if (radius < 2) radius = 2;
      filledCircleRGBA(surface, particle_x, particle_y, radius, 153, 167, 180,
                       (Uint8)(alpha * 0.3));
    }
  }
}


static void render_rare_screen_fade(RenderTarget *surface, Uint32 ticks) {
  EffectWindow window;
  if (!effect_window(&EFFECT_SCREEN_FADE, ticks, &window)) return;
  Uint8 color = (window.seed & 0x80) ? 255 : 0;
  boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, color, color, color,
          (Uint8)(255 * window.fade));
}
