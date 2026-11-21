#include "sky_events_internal.h"
#include "backdrop.h"
#include "scene_core.h"

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


// Albatross pair: two long-winged ocean gliders banking slowly in tandem.
static void sky_event_204(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 2; i++) {
    double bx = x - direction * i * size * 1.4;
    double by = y + (i - 0.5) * size * 0.65 + sin(phase / 600.0 + i) * size * 0.2;
    double flap = 0.15 + sin(phase / 480.0 + i * 0.9) * 0.25;
    draw_sleek_bird(surface, bx, by, size * 1.4, direction, flap, sin(phase / 700.0) * 0.14, 210,
                    220, 235, weapon_alpha(205.0 * fade), true);
  }
}


// Comet shower: three small icy comets streaking in parallel with cyan tails.
static void sky_event_205(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 ice_c[3] = {155, 225, 255};
  for (int i = 0; i < 3; i++) {
    double cx = x - direction * i * size * 1.1;
    double cy = y + (i - 1) * size * 0.75;
    double tx = cx - direction * size * 3.2;
    double ty = cy - size * 0.75;
    thickLineRGBA(surface, (int)cx, (int)cy, (int)tx, (int)ty, 2, 130, 205, 245,
                  weapon_alpha(150.0 * fade));
    weapon_glow(surface, cx, cy, 1.1, ice_c, 175.0 * fade);
    filledCircleRGBA(surface, (int)cx, (int)cy, size / 4 + 1, 225, 245, 255,
                     weapon_alpha(230.0 * fade));
  }
}


// Prism halo: a rotating hexagonal ice-crystal halo around a pale moonlet.
static void sky_event_206(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double rot = progress * 1.4;
  int r = size * 2;
  filledCircleRGBA(surface, x, y, size / 3 + 1, 235, 240, 250, weapon_alpha(190.0 * fade));
  for (int i = 0; i < 6; i++) {
    double a0 = rot + i * SCENE_TAU / 6.0;
    double a1 = rot + (i + 1) * SCENE_TAU / 6.0;
    thickLineRGBA(surface, x + (int)(cos(a0) * r), y + (int)(sin(a0) * r),
                  x + (int)(cos(a1) * r), y + (int)(sin(a1) * r), 1, 175, 210, 245,
                  weapon_alpha(150.0 * fade));
  }
}


// Wind ribbons: two sinuous pale-lavender air currents drifting across.
static void sky_event_207(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int s = 0; s < 2; s++) {
    double prev_x = x - direction * size * 3.0;
    double prev_y = y + (s - 0.5) * size * 0.9;
    for (int i = 1; i <= 14; i++) {
      double u = i / 14.0;
      double nx = x + direction * (u - 0.5) * size * 6.0;
      double ny = y + (s - 0.5) * size * 0.9 + sin(u * 5.5 + phase / 260.0 + s * 1.4) * size * 0.45;
      thickLineRGBA(surface, (int)prev_x, (int)prev_y, (int)nx, (int)ny, 2, 185, 175, 230,
                    weapon_alpha(145.0 * sin(u * SCENE_PI) * fade));
      prev_x = nx;
      prev_y = ny;
    }
  }
}


// Celestial whale: a slow dark silhouette with soft cyan bioluminescent spots.
static void sky_event_208(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double bob = sin(phase / 420.0) * size * 0.3;
  double cy = y + bob;
  filledEllipseRGBA(surface, x, (int)cy, size * 2, size * 2 / 3, 24, 34, 52,
                    weapon_alpha(210.0 * fade));
  filledTrigonRGBA(surface, x - direction * size * 2, (int)cy,
                   x - direction * size * 3, (int)(cy - size * 0.7),
                   x - direction * size * 3, (int)(cy + size * 0.6), 24, 34, 52,
                   weapon_alpha(200.0 * fade));
  static const Uint8 bio_c[3] = {120, 215, 235};
  for (int i = -1; i <= 1; i++) {
    weapon_glow(surface, x + direction * i * size * 0.65, cy, 0.7, bio_c, 155.0 * fade);
  }
}


// Paper gliders: four crisp folded gliders riding a gentle thermal.
static void sky_event_209(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 4; i++) {
    double gx = x - direction * i * size * 1.1;
    double gy = y + ((i % 2) ? 1 : -1) * size * 0.45 + sin(phase / 320.0 + i) * size * 0.2;
    int tip_x = (int)(gx + direction * size * 0.8);
    int tail_x = (int)(gx - direction * size * 0.6);
    filledTrigonRGBA(surface, tip_x, (int)gy, tail_x, (int)(gy - size * 0.4), tail_x,
                     (int)(gy + size * 0.25), 225, 228, 235, weapon_alpha(185.0 * fade));
    lineRGBA(surface, tip_x, (int)gy, tail_x, (int)gy, 155, 165, 185, weapon_alpha(190.0 * fade));
  }
}


// Eclipse ring: a dark disc with a thin glowing golden corona drifting by.
static void sky_event_210(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size + 2;
  static const Uint8 cor_c[3] = {255, 210, 130};
  weapon_glow(surface, x, y, 2.2, cor_c, 130.0 * fade);
  circleRGBA(surface, x, y, r + 1, 255, 220, 150, weapon_alpha(205.0 * fade));
  filledCircleRGBA(surface, x, y, r, 12, 16, 28, weapon_alpha(235.0 * fade));
}


// Crane chevron: five cranes gliding in an asymmetric V-formation.
static void sky_event_211(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 5; i++) {
    int rank = i / 2 + (i % 2);
    int side = (i == 0) ? 0 : ((i % 2) ? 1 : -1);
    double bx = x - direction * rank * size * 0.95;
    double by = y + side * rank * size * 0.6;
    double flap = sin(phase / 240.0 + i * 0.7) * 0.75;
    draw_sleek_bird(surface, bx, by, size * 0.9, direction, flap, 0.0, 215, 225, 238,
                    weapon_alpha(200.0 * fade), true);
  }
}


// Astrolabe wheel: delicate spinning brass-toned rings and pointers.
static void sky_event_212(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size * 2;
  circleRGBA(surface, x, y, r, 215, 185, 120, weapon_alpha(175.0 * fade));
  circleRGBA(surface, x, y, r * 2 / 3, 195, 165, 105, weapon_alpha(150.0 * fade));
  double a = phase / 650.0;
  thickLineRGBA(surface, x - (int)(cos(a) * r), y - (int)(sin(a) * r),
                x + (int)(cos(a) * r), y + (int)(sin(a) * r), 1, 230, 200, 135,
                weapon_alpha(170.0 * fade));
  thickLineRGBA(surface, x - (int)(cos(a + 1.1) * r), y - (int)(sin(a + 1.1) * r),
                x + (int)(cos(a + 1.1) * r), y + (int)(sin(a + 1.1) * r), 1, 210, 180, 120,
                weapon_alpha(140.0 * fade));
}


// Noctilucent wisps: layered silver-blue high-altitude cloud streaks.
static void sky_event_213(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 4; i++) {
    int wy = y + (i - 1) * (size / 2 + 2);
    int w = size * (3 - (i % 2));
    filledEllipseRGBA(surface, x + (i - 1) * size / 2, wy, w, size / 4 + 1, 150, 195, 235,
                      weapon_alpha((110.0 - i * 16.0) * fade));
  }
}


// Sky lantern cluster: five warm paper lanterns drifting gently upward.
static void sky_event_214(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 lan_c[3] = {255, 175, 85};
  for (int i = 0; i < 5; i++) {
    double lx = x + (i - 2) * size * 0.95 + sin(phase / 380.0 + i * 1.3) * size * 0.25;
    double ly = y + ((i % 3) - 1) * size * 0.7 - progress * size * 1.4;
    weapon_glow(surface, lx, ly, 1.1, lan_c, 145.0 * fade);
    filledEllipseRGBA(surface, (int)lx, (int)ly, size / 4 + 2, size / 3 + 2, 245, 165, 80,
                      weapon_alpha(205.0 * fade));
  }
}


// Manta pair: two graceful sky mantas gliding with undulating wingtips.
static void sky_event_215(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 2; i++) {
    double mx = x - direction * i * size * 1.6;
    double my = y + (i - 0.5) * size * 0.8 + sin(phase / 340.0 + i) * size * 0.25;
    double wing = sin(phase / 240.0 + i * 1.1) * size * 0.9;
    filledTrigonRGBA(surface, (int)(mx + direction * size), (int)my,
                     (int)(mx - direction * size * 0.6), (int)(my - wing),
                     (int)(mx - direction * size * 0.6), (int)(my + wing * 0.8), 115, 145, 195,
                     weapon_alpha(185.0 * fade));
    lineRGBA(surface, (int)(mx - direction * size * 0.6), (int)my,
             (int)(mx - direction * size * 2.0), (int)my, 135, 165, 215,
             weapon_alpha(160.0 * fade));
  }
}


// Shooting star arc: a curved meteor leaving a sparkling amber-white wake.
static void sky_event_216(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double arc = sin(progress * SCENE_PI) * size * 1.2;
  double hx = x;
  double hy = y - arc;
  double tx = x - direction * size * 3.5;
  double ty = y - sin(fmax(0.0, progress - 0.12) * SCENE_PI) * size * 1.2;
  thickLineRGBA(surface, (int)hx, (int)hy, (int)tx, (int)ty, 2, 255, 230, 175,
                weapon_alpha(200.0 * fade));
  filledCircleRGBA(surface, (int)hx, (int)hy, size / 4 + 2, 255, 248, 225,
                   weapon_alpha(235.0 * fade));
}


// Sundog twin: pale prismatic mock-suns flanking a faint central arc.
static void sky_event_217(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int span = size * 2;
  circleRGBA(surface, x, y, span, 165, 195, 225, weapon_alpha(75.0 * fade));
  static const Uint8 dog_c[3] = {255, 215, 160};
  weapon_glow(surface, x - span, y, 1.3, dog_c, 165.0 * fade);
  weapon_glow(surface, x + span, y, 1.3, dog_c, 165.0 * fade);
}


// Swallowtail drift: three large nocturnal moths gliding on high thermals.
static void sky_event_218(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 3; i++) {
    double mx = x - direction * i * size * 1.1;
    double my = y + (i - 1) * size * 0.65 + sin(phase / 190.0 + i * 1.2) * size * 0.3;
    double flap = 0.4 + 0.6 * fabs(sin(phase / 120.0 + i));
    int w = (int)(size * 0.55 * flap) + 2;
    filledEllipseRGBA(surface, (int)mx, (int)(my - w / 2), size / 3 + 1, w, 195, 185, 220,
                      weapon_alpha(175.0 * fade));
    filledEllipseRGBA(surface, (int)mx, (int)(my + w / 2), size / 3 + 1, w, 175, 165, 205,
                      weapon_alpha(160.0 * fade));
  }
}


// Starlight harp: vertical shimmering aurora strings plucked by a passing wave.
static void sky_event_219(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -3; i <= 3; i++) {
    double sx = x + i * size * 0.42;
    double wave = sin(progress * 8.0 + i * 0.7) * size * 0.25;
    thickLineRGBA(surface, (int)sx, y - size * 2, (int)(sx + wave), y + size * 2, 1, 130, 230,
                  195, weapon_alpha((145.0 - abs(i) * 14.0) * fade));
  }
}


// Cloud galleon: a translucent mist-ship with billowing vapor sails.
static void sky_event_220(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  filledEllipseRGBA(surface, x, y + size / 2, size * 2, size / 3 + 1, 165, 185, 215,
                    weapon_alpha(145.0 * fade));
  for (int m = -1; m <= 1; m += 2) {
    int mx = x + direction * m * size * 2 / 3;
    lineRGBA(surface, mx, y + size / 2, mx, y - size * 3 / 2, 190, 205, 225,
             weapon_alpha(140.0 * fade));
    filledTrigonRGBA(surface, mx, y - size * 3 / 2, mx + direction * size, y - size / 4, mx,
                     y + size / 4, 185, 205, 230, weapon_alpha(130.0 * fade));
  }
}


// Moonbow arc: a faint pale-silver and soft-violet lunar rainbow arc.
static void sky_event_221(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size * 3;
  for (int i = 0; i < 12; i++) {
    double a0 = SCENE_PI + i * SCENE_PI / 12.0;
    double a1 = SCENE_PI + (i + 1) * SCENE_PI / 12.0;
    thickLineRGBA(surface, x + (int)(cos(a0) * r), y + (int)(sin(a0) * r),
                  x + (int)(cos(a1) * r), y + (int)(sin(a1) * r), 2, 195, 205, 235,
                  weapon_alpha(125.0 * fade));
    thickLineRGBA(surface, x + (int)(cos(a0) * (r - 3)), y + (int)(sin(a0) * (r - 3)),
                  x + (int)(cos(a1) * (r - 3)), y + (int)(sin(a1) * (r - 3)), 1, 185, 165, 225,
                  weapon_alpha(105.0 * fade));
  }
}


// Comet glider: a sleek winged silhouette towing a pale-gold stardust wake.
static void sky_event_222(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int tail_x = x - direction * size * 3;
  thickLineRGBA(surface, x, y, tail_x, y + size / 3, 2, 235, 205, 130, weapon_alpha(145.0 * fade));
  filledTrigonRGBA(surface, x + direction * size, y, x - direction * size, y - size * 2 / 3,
                   x - direction * size / 2, y, 210, 225, 245, weapon_alpha(200.0 * fade));
  filledTrigonRGBA(surface, x + direction * size, y, x - direction * size, y + size * 2 / 3,
                   x - direction * size / 2, y, 180, 200, 230, weapon_alpha(185.0 * fade));
}


// Prism halo: a faint iridescent ring surrounding a pale drifting moonlet.
static void sky_event_223(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size * 2;
  circleRGBA(surface, x, y, r, 160, 215, 240, weapon_alpha(120.0 * fade));
  circleRGBA(surface, x, y, r - 2, 230, 175, 210, weapon_alpha(100.0 * fade));
  filledCircleRGBA(surface, x, y, size / 2 + 1, 235, 240, 250, weapon_alpha(190.0 * fade));
}


// Wind ribbon: a sinuous ribbon of pale-jade mist rippling through the sky.
static void sky_event_224(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -4; i < 4; i++) {
    int x0 = x + i * size / 2;
    int x1 = x + (i + 1) * size / 2;
    int y0 = y + (int)(sin(phase / 200.0 + i * 0.7) * size * 0.6);
    int y1 = y + (int)(sin(phase / 200.0 + (i + 1) * 0.7) * size * 0.6);
    thickLineRGBA(surface, x0, y0, x1, y1, 2, 155, 225, 200, weapon_alpha(155.0 * fade));
  }
}


// Starflower bloom: a rotating six-petaled glowing blossom drifting across the sky.
static void sky_event_225(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double rot = phase / 600.0;
  for (int i = 0; i < 6; i++) {
    double a = rot + i * SCENE_TAU / 6.0;
    int px = x + (int)(cos(a) * size * 0.9);
    int py = y + (int)(sin(a) * size * 0.9);
    filledCircleRGBA(surface, px, py, size / 3 + 1, 235, 195, 245, weapon_alpha(175.0 * fade));
  }
  filledCircleRGBA(surface, x, y, size / 3 + 1, 255, 240, 175, weapon_alpha(220.0 * fade));
}


// Twin owls: two silent owls gliding wingtip-to-wingtip across the night sky.
static void sky_event_226(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 2; i++) {
    double bx = x - direction * i * size * 1.4;
    double by = y + (i == 0 ? -size * 0.45 : size * 0.45);
    double flap = sin(phase / 210.0 + i * 0.8) * 0.7;
    draw_sleek_bird(surface, bx, by, size * 0.95, direction, flap, 0.0, 200, 190, 175,
                    weapon_alpha(195.0 * fade), true);
  }
}


// Meteorite shower: three staggered amber-white streaks racing in parallel.
static void sky_event_227(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 3; i++) {
    int mx = x - direction * i * size * 1.4;
    int my = y + (i - 1) * size * 0.7;
    thickLineRGBA(surface, mx, my, mx - direction * size * 2, my - size / 2, 2, 255, 225, 165,
                  weapon_alpha((195.0 - i * 30.0) * fade));
    filledCircleRGBA(surface, mx, my, 2, 255, 245, 220, weapon_alpha(230.0 * fade));
  }
}


// Sky jellyfish: a translucent bell pulsing gently with trailing filaments.
static void sky_event_228(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int rx = size;
  int ry = (int)(size * (0.65 + 0.15 * sin(phase / 200.0)));
  filledPieRGBA(surface, x, y, rx, 180, 360, 165, 215, 245, weapon_alpha(155.0 * fade));
  for (int i = -2; i <= 2; i++) {
    int tx = x + i * (rx / 3);
    int wave = (int)(sin(phase / 180.0 + i) * size * 0.35);
    lineRGBA(surface, tx, y, tx + wave, y + ry + size, 150, 200, 240, weapon_alpha(130.0 * fade));
  }
}


// Paper crane pair: two origami cranes banking gently in tandem.
static void sky_event_229(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 2; i++) {
    int cx = x - direction * i * size * 1.4;
    int cy = y + (i == 0 ? -size / 3 : size / 2);
    int wing = (int)(size * (0.5 + 0.4 * sin(phase / 200.0 + i)));
    filledTrigonRGBA(surface, cx + direction * size, cy, cx - direction * size / 2, cy, cx,
                     cy - wing, 235, 230, 215, weapon_alpha(200.0 * fade));
    filledTrigonRGBA(surface, cx + direction * size, cy, cx - direction * size / 2, cy, cx,
                     cy + wing / 2, 205, 200, 185, weapon_alpha(175.0 * fade));
  }
}


// Aurora ribbon: a vertical curtain of pale green and violet light drifting by.
static void sky_event_230(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -3; i <= 3; i++) {
    int ax = x + i * (size / 2 + 1);
    int sway = (int)(sin(phase / 240.0 + i * 0.6) * size * 0.4);
    thickLineRGBA(surface, ax, y - size * 2, ax + sway, y + size * 2, 2, 110, 225, 175,
                  weapon_alpha((125.0 - abs(i) * 14.0) * fade));
  }
}


// Fireball ember: a bright orange-gold meteoroid shedding glowing sparks behind it.
static void sky_event_231(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  static const Uint8 emb[3] = {255, 155, 65};
  weapon_glow(surface, x, y, 1.3, emb, 195.0 * fade);
  filledCircleRGBA(surface, x, y, size / 3 + 1, 255, 220, 140, weapon_alpha(230.0 * fade));
  for (int i = 1; i <= 4; i++) {
    filledCircleRGBA(surface, x - direction * i * size * 0.7, y + (i % 2 ? 2 : -2), 1, 255, 175,
                     95, weapon_alpha((180.0 - i * 32.0) * fade));
  }
}


// Starlit kite: a diamond kite with a fluttering ribbon tail drifting across the sky.
static void sky_event_232(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  filledTrigonRGBA(surface, x, y - size, x - size * 2 / 3, y, x + size * 2 / 3, y, 235, 170, 125,
                   weapon_alpha(200.0 * fade));
  filledTrigonRGBA(surface, x, y + size, x - size * 2 / 3, y, x + size * 2 / 3, y, 210, 145, 105,
                   weapon_alpha(185.0 * fade));
  int wave = (int)(sin(phase / 160.0) * size * 0.4);
  lineRGBA(surface, x, y + size, x - direction * size, y + size * 2 + wave, 240, 225, 195,
           weapon_alpha(170.0 * fade));
}


// Starlight glider: a delta-wing glider with faint teal wingtip trails.
static void sky_event_233(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  filledTrigonRGBA(surface, x + direction * size, y, x - direction * size, y - size * 2 / 3,
                   x - direction * size, y + size * 2 / 3, 185, 215, 240, weapon_alpha(190.0 * fade));
  lineRGBA(surface, x - direction * size, y - size * 2 / 3, x - direction * size * 3,
           y - size * 2 / 3, 120, 230, 220, weapon_alpha(135.0 * fade));
  lineRGBA(surface, x - direction * size, y + size * 2 / 3, x - direction * size * 3,
           y + size * 2 / 3, 120, 230, 220, weapon_alpha(135.0 * fade));
}


// Moonseed float: glowing dandelion-like star seeds drifting on a gentle breeze.
static void sky_event_234(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -1; i <= 1; i++) {
    int sx = x + i * size;
    int sy = y + (int)(sin(phase / 210.0 + i * 1.1) * size * 0.6);
    filledCircleRGBA(surface, sx, sy + size / 2, 2, 235, 240, 210, weapon_alpha(200.0 * fade));
    lineRGBA(surface, sx, sy + size / 2, sx, sy - size / 3, 195, 210, 225,
             weapon_alpha(160.0 * fade));
    lineRGBA(surface, sx - size / 3, sy - size / 2, sx + size / 3, sy - size / 2, 220, 235, 250,
             weapon_alpha(175.0 * fade));
  }
}


// Solar wind sail: a luminous triangular sail catching a stream of golden motes.
static void sky_event_235(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  filledTrigonRGBA(surface, x + direction * size, y, x - direction * size, y - size * 4 / 3,
                   x - direction * size, y + size * 4 / 3, 255, 225, 165, weapon_alpha(165.0 * fade));
  lineRGBA(surface, x + direction * size, y, x - direction * size * 2, y, 255, 240, 200,
           weapon_alpha(190.0 * fade));
}


// Starlight pendulum: a glowing bob swinging beneath a drifting silver arc.
static void sky_event_236(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double sw = sin(phase / 240.0) * 0.7;
  int bx = x + (int)(sin(sw) * size * 2.0);
  int by = y + (int)(cos(sw) * size * 2.0);
  lineRGBA(surface, x, y - size, bx, by, 180, 200, 225, weapon_alpha(165.0 * fade));
  static const Uint8 bob[3] = {185, 225, 255};
  weapon_glow(surface, bx, by, 1.0, bob, 185.0 * fade);
}


// Sky-whale pod: two gentle sky-whales swimming side-by-side through the clouds.
static void sky_event_237(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 2; i++) {
    int wx = x - direction * i * size * 1.5;
    int wy = y + (i == 0 ? -size / 2 : size / 2);
    filledEllipseRGBA(surface, wx, wy, size, size / 2 + 1, 130, 165, 210,
                      weapon_alpha(180.0 * fade));
    filledTrigonRGBA(surface, wx - direction * size, wy, wx - direction * size * 3 / 2,
                     wy - size / 2, wx - direction * size * 3 / 2, wy + size / 2, 130, 165, 210,
                     weapon_alpha(170.0 * fade));
  }
}


// Ringed planetoid: a small lavender-gray globe with a tilted golden ring.
static void sky_event_238(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size * 2 / 3 + 2;
  filledCircleRGBA(surface, x, y, r, 155, 145, 185, weapon_alpha(200.0 * fade));
  ellipseRGBA(surface, x, y, r * 2, r / 2 + 1, 230, 200, 145, weapon_alpha(180.0 * fade));
}


// Sparkler swirl: a spiraling trail of tiny pale-gold sparks drifting across the sky.
static void sky_event_239(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 6; i++) {
    double a = phase / 140.0 + i * 0.9;
    int sx = x - direction * i * size / 2 + (int)(cos(a) * size * 0.6);
    int sy = y + (int)(sin(a) * size * 0.6);
    filledCircleRGBA(surface, sx, sy, 2, 255, 225, 140, weapon_alpha((210.0 - i * 25.0) * fade));
  }
}


// Crescent moth: a pale-cyan nocturnal moth with curving crescent wings.
static void sky_event_240(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int flap = (int)(size * (0.4 + 0.6 * fabs(sin(phase / 120.0))));
  filledEllipseRGBA(surface, x, y - flap / 2, size / 2, flap, 175, 230, 235,
                    weapon_alpha(180.0 * fade));
  filledEllipseRGBA(surface, x, y + flap / 2, size / 2, flap, 155, 210, 220,
                    weapon_alpha(165.0 * fade));
}


// Starlit ringlet: a spinning golden hoop trailing tiny sparkling motes.
static void sky_event_241(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size;
  circleRGBA(surface, x, y, r, 245, 215, 130, weapon_alpha(190.0 * fade));
  double a = phase / 200.0;
  filledCircleRGBA(surface, x + (int)(cos(a) * r), y + (int)(sin(a) * r), 2, 255, 245, 200,
                   weapon_alpha(230.0 * fade));
}


// Twin starlets: two pale-blue and warm-gold stars orbiting each other as they drift.
static void sky_event_242(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double a = phase / 220.0;
  int dx = (int)(cos(a) * size * 0.8);
  int dy = (int)(sin(a) * size * 0.8);
  filledCircleRGBA(surface, x + dx, y + dy, size / 4 + 2, 165, 215, 255,
                   weapon_alpha(220.0 * fade));
  filledCircleRGBA(surface, x - dx, y - dy, size / 4 + 2, 255, 215, 145,
                   weapon_alpha(220.0 * fade));
}


// Floating lantern boat: a crescent paper skiff carrying a tiny amber lantern.
static void sky_event_243(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  filledPieRGBA(surface, x, y, size, 0, 180, 210, 175, 130, weapon_alpha(190.0 * fade));
  static const Uint8 lan[3] = {255, 200, 110};
  weapon_glow(surface, x, y - size / 3, 0.9, lan, 180.0 * fade);
}


// Silver swift: a fast, slender swift banking smoothly across the upper sky.
static void sky_event_244(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double flap = sin(phase / 110.0) * 0.95;
  draw_sleek_bird(surface, x, y, size * 0.85, direction, flap, 0.1, 185, 210, 235,
                  weapon_alpha(200.0 * fade), true);
}


// Stardust spiral: a curling helix of pale-gold motes drifting through the night.
static void sky_event_245(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 8; i++) {
    double a = phase / 180.0 + i * 0.65;
    int sx = x - direction * i * size / 3 + (int)(cos(a) * size * 0.45);
    int sy = y + (int)(sin(a) * size * 0.45);
    pixelRGBA(surface, sx, sy, 255, 230, 165, weapon_alpha((220.0 - i * 20.0) * fade));
  }
}


// Sky compass: a faint glowing four-pointed star rotating slowly as it crosses.
static void sky_event_246(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  double rot = phase / 500.0;
  for (int i = 0; i < 4; i++) {
    double a = rot + i * SCENE_PI * 0.5;
    lineRGBA(surface, x, y, x + (int)(cos(a) * size), y + (int)(sin(a) * size), 215, 235, 255,
             weapon_alpha(190.0 * fade));
  }
  filledCircleRGBA(surface, x, y, 2, 245, 250, 255, weapon_alpha(230.0 * fade));
}


// Vapor rings: three expanding smoke-like rings drifting gently across the sky.
static void sky_event_247(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 3; i++) {
    int rx = x - direction * i * size * 1.1;
    int r = (size / 2 + i * 3) + (int)(progress * size * 0.5);
    circleRGBA(surface, rx, y, r, 185, 200, 220, weapon_alpha((150.0 - i * 30.0) * fade));
  }
}


// Meteor pair: two parallel cyan-white streaks racing together across the sky.
static void sky_event_248(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 2; i++) {
    int my = y + (i == 0 ? -size / 2 : size / 2);
    thickLineRGBA(surface, x, my, x - direction * size * 3, my - size / 2, 2, 165, 230, 255,
                  weapon_alpha(205.0 * fade));
    filledCircleRGBA(surface, x, my, 2, 240, 250, 255, weapon_alpha(235.0 * fade));
  }
}


// Starlit ribbon: a drifting banner of pale lavender and silver stars.
static void sky_event_249(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -2; i <= 2; i++) {
    int sx = x + i * size;
    int sy = y + (int)(sin(phase / 190.0 + i * 0.9) * size * 0.5);
    filledCircleRGBA(surface, sx, sy, 2, 215, 200, 250, weapon_alpha(205.0 * fade));
    if (i < 2) {
      int nx = x + (i + 1) * size;
      int ny = y + (int)(sin(phase / 190.0 + (i + 1) * 0.9) * size * 0.5);
      lineRGBA(surface, sx, sy, nx, ny, 190, 180, 235, weapon_alpha(145.0 * fade));
    }
  }
}


// Twilight comet arc: a curved golden-amber comet with a sparkling tail.
static void sky_event_250(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int head_x = x;
  int head_y = y + (int)(sin(progress * SCENE_PI) * -size * 1.2);
  for (int i = 1; i <= 5; i++) {
    int tx = head_x - direction * i * (size / 2);
    int ty = head_y + (int)(i * i * 0.6);
    filledCircleRGBA(surface, tx, ty, i < 3 ? 2 : 1, 255, 200, 120,
                     weapon_alpha((180.0 - i * 28.0) * fade));
  }
  filledCircleRGBA(surface, head_x, head_y, 3, 255, 235, 175, weapon_alpha(230.0 * fade));
}


// Aurora halo: an expanding emerald-teal ring with a soft inner shimmer.
static void sky_event_251(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = (int)(size * (0.7 + 0.5 * sin(phase / 240.0)));
  if (r < 3) r = 3;
  circleRGBA(surface, x, y, r, 110, 240, 200, weapon_alpha(175.0 * fade));
  circleRGBA(surface, x, y, r - 2, 165, 255, 230, weapon_alpha(115.0 * fade));
}


// Stardust spiral: a rotating pinwheel of tiny gold and violet sparks.
static void sky_event_252(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 6; i++) {
    double a = phase / 210.0 + i * SCENE_TAU / 6.0;
    double r = size * (0.25 + 0.14 * i);
    int sx = x + (int)(cos(a) * r);
    int sy = y + (int)(sin(a) * r * 0.7);
    Uint8 cr = (i & 1) ? 210 : 255;
    Uint8 cg = (i & 1) ? 175 : 225;
    Uint8 cb = (i & 1) ? 255 : 155;
    filledCircleRGBA(surface, sx, sy, 2, cr, cg, cb, weapon_alpha(195.0 * fade));
  }
}


// Nebula wisp: overlapping translucent rose and teal cloud puffs.
static void sky_event_253(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -1; i <= 1; i++) {
    int cx = x + i * (size / 2);
    int cy = y + (int)(sin(phase / 300.0 + i) * size * 0.25);
    int r = size / 2 + 2 - abs(i);
    if (r < 3) r = 3;
    Uint8 cr = (i == 0) ? 235 : 140;
    Uint8 cg = (i == 0) ? 160 : 215;
    Uint8 cb = (i == 0) ? 225 : 230;
    filledCircleRGBA(surface, cx, cy, r, cr, cg, cb, weapon_alpha(115.0 * fade));
  }
}


// Comet cross: two brief pale-gold shooting streaks crossing at an angle.
static void sky_event_254(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int d = size;
  lineRGBA(surface, x - d, y - d / 2, x + d, y + d / 2, 255, 225, 165, weapon_alpha(185.0 * fade));
  lineRGBA(surface, x - d / 2, y + d / 2, x + d / 2, y - d / 2, 200, 230, 255,
           weapon_alpha(175.0 * fade));
  filledCircleRGBA(surface, x, y, 2, 255, 248, 225, weapon_alpha(230.0 * fade));
}


// Lunar corona: concentric iridescent ice-blue and pale-gold rings.
static void sky_event_255(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size;
  if (r < 5) r = 5;
  circleRGBA(surface, x, y, r, 175, 220, 255, weapon_alpha(150.0 * fade));
  circleRGBA(surface, x, y, r * 2 / 3, 250, 225, 170, weapon_alpha(175.0 * fade));
  filledCircleRGBA(surface, x, y, r / 4 + 1, 245, 248, 255, weapon_alpha(200.0 * fade));
}


// Prism flare: a diamond-shaped refraction burst with spectral rays.
static void sky_event_256(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size;
  lineRGBA(surface, x - r, y, x + r, y, 255, 195, 195, weapon_alpha(170.0 * fade));
  lineRGBA(surface, x, y - r, x, y + r, 185, 240, 255, weapon_alpha(170.0 * fade));
  filledCircleRGBA(surface, x, y, 3, 255, 250, 240, weapon_alpha(225.0 * fade));
}


// Meteor fork: a fast meteor that splits into two diverging ember trails.
static void sky_event_257(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int tail = size * 2;
  lineRGBA(surface, x - direction * tail, y - tail / 4, x, y, 255, 195, 130,
           weapon_alpha(180.0 * fade));
  lineRGBA(surface, x, y, x + direction * (tail / 2), y - tail / 3, 255, 225, 170,
           weapon_alpha(195.0 * fade));
  lineRGBA(surface, x, y, x + direction * (tail / 2), y + tail / 4, 255, 170, 120,
           weapon_alpha(195.0 * fade));
  filledCircleRGBA(surface, x, y, 2, 255, 245, 215, weapon_alpha(230.0 * fade));
}


// Star kite: a drifting diamond constellation with a trailing tail of sparks.
static void sky_event_258(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size / 2 + 2;
  lineRGBA(surface, x, y - r, x + r, y, 195, 225, 255, weapon_alpha(180.0 * fade));
  lineRGBA(surface, x + r, y, x, y + r, 195, 225, 255, weapon_alpha(180.0 * fade));
  lineRGBA(surface, x, y + r, x - r, y, 195, 225, 255, weapon_alpha(180.0 * fade));
  lineRGBA(surface, x - r, y, x, y - r, 195, 225, 255, weapon_alpha(180.0 * fade));
  for (int i = 1; i <= 3; i++) {
    int ty = y + r + i * 5;
    int tx = x - direction * i * 4 + (int)(sin(phase / 180.0 + i) * 3.0);
    filledCircleRGBA(surface, tx, ty, 1, 255, 230, 175, weapon_alpha((190.0 - i * 35.0) * fade));
  }
}


// Crescent mirage: a shimmering double crescent arc in pale silver-blue.
static void sky_event_259(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size / 2 + 4;
  for (int i = 0; i < 6; i++) {
    double a0 = -0.9 + i * 0.3;
    double a1 = a0 + 0.3;
    lineRGBA(surface, x + (int)(cos(a0) * r), y + (int)(sin(a0) * r), x + (int)(cos(a1) * r),
             y + (int)(sin(a1) * r), 205, 225, 255, weapon_alpha(190.0 * fade));
    lineRGBA(surface, x + (int)(cos(a0) * (r - 4)) - 3, y + (int)(sin(a0) * (r - 4)),
             x + (int)(cos(a1) * (r - 4)) - 3, y + (int)(sin(a1) * (r - 4)), 175, 200, 240,
             weapon_alpha(140.0 * fade));
  }
}


// Celestial harp: parallel shimmering vertical light strings drifting in the sky.
static void sky_event_260(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -2; i <= 2; i++) {
    int sx = x + i * (size / 3 + 2);
    int h = size - abs(i) * 3;
    if (h < 4) h = 4;
    int wave = (int)(sin(phase / 170.0 + i * 0.9) * 2.5);
    lineRGBA(surface, sx, y - h / 2, sx + wave, y + h / 2, 185, 225, 255,
             weapon_alpha(175.0 * fade));
  }
}


// Polar shimmer: vertical ice-cyan light curtains swaying gently.
static void sky_event_261(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -3; i <= 3; i++) {
    int bx = x + i * (size / 4 + 2);
    int sway = (int)(sin(phase / 200.0 + i * 0.6) * 4.0);
    int h = size + (3 - abs(i)) * 3;
    lineRGBA(surface, bx, y - h / 2, bx + sway, y + h / 2, 130, 245, 225,
             weapon_alpha((150.0 - abs(i) * 20.0) * fade));
  }
}


// Ion trail: a segmented violet-cyan streak with glowing nodes.
static void sky_event_262(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 4; i++) {
    int nx = x - direction * i * (size / 2 + 2);
    int ny = y + (int)(sin(phase / 200.0 + i * 0.8) * 3.0);
    filledCircleRGBA(surface, nx, ny, i == 0 ? 3 : 2, 165, 220, 255,
                     weapon_alpha((210.0 - i * 35.0) * fade));
  }
}


// Amber flare: a pulsing warm-orange star with four diagonal rays.
static void sky_event_263(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = (int)(size * (0.6 + 0.3 * sin(phase / 160.0)));
  if (r < 4) r = 4;
  lineRGBA(surface, x - r, y - r, x + r, y + r, 255, 190, 115, weapon_alpha(175.0 * fade));
  lineRGBA(surface, x - r, y + r, x + r, y - r, 255, 190, 115, weapon_alpha(175.0 * fade));
  filledCircleRGBA(surface, x, y, 3, 255, 225, 165, weapon_alpha(230.0 * fade));
}


// Glider flock: three pale-silver V-shaped gliders cruising in formation.
static void sky_event_264(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = 0; i < 3; i++) {
    int gx = x - direction * i * (size / 2 + 3);
    int gy = y + (i - 1) * (size / 3 + 2);
    lineRGBA(surface, gx - direction * 5, gy - 3, gx, gy, 210, 225, 245,
             weapon_alpha(185.0 * fade));
    lineRGBA(surface, gx - direction * 5, gy + 3, gx, gy, 210, 225, 245,
             weapon_alpha(185.0 * fade));
  }
}


// Comet wheel: a rotating ring of four amber-white comet sparks.
static void sky_event_265(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size / 2 + 2;
  for (int i = 0; i < 4; i++) {
    double a = phase / 200.0 + i * SCENE_PI * 0.5;
    int sx = x + (int)(cos(a) * r);
    int sy = y + (int)(sin(a) * r);
    filledCircleRGBA(surface, sx, sy, 2, 255, 225, 165, weapon_alpha(205.0 * fade));
  }
}


// Starlit cascade: three staggered falling star motes with faint vertical tails.
static void sky_event_266(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -1; i <= 1; i++) {
    int sx = x + i * (size / 2 + 2);
    int sy = y + (int)((phase / 12 + i * 7) % (size + 6)) - size / 2;
    lineRGBA(surface, sx, sy - 5, sx, sy, 200, 225, 255, weapon_alpha(150.0 * fade));
    filledCircleRGBA(surface, sx, sy, 2, 240, 248, 255, weapon_alpha(215.0 * fade));
  }
}


// Twin halo: two interlocking pale-gold and sky-blue orbital rings.
static void sky_event_267(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  int r = size / 2 + 2;
  if (r < 4) r = 4;
  circleRGBA(surface, x - r / 3, y, r, 255, 225, 160, weapon_alpha(165.0 * fade));
  circleRGBA(surface, x + r / 3, y, r, 165, 215, 255, weapon_alpha(165.0 * fade));
}


// Sky lantern trio: three warm paper lanterns drifting upward in a gentle arc.
static void sky_event_268(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -1; i <= 1; i++) {
    int lx = x + i * (size / 2 + 3);
    int ly = y - abs(i) * 4 + (int)(sin(phase / 220.0 + i) * 3.0);
    boxRGBA(surface, lx - 2, ly - 3, lx + 2, ly + 3, 255, 195, 115, weapon_alpha(195.0 * fade));
    filledCircleRGBA(surface, lx, ly + 1, 1, 255, 240, 190, weapon_alpha(230.0 * fade));
  }
}


// Aurora ribbon: a sinusoidal emerald-violet wave across the upper sky.
static void sky_event_269(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -3; i < 3; i++) {
    int x0 = x + i * (size / 2 + 2);
    int y0 = y + (int)(sin(phase / 180.0 + i * 0.7) * size * 0.45);
    int x1 = x + (i + 1) * (size / 2 + 2);
    int y1 = y + (int)(sin(phase / 180.0 + (i + 1) * 0.7) * size * 0.45);
    thickLineRGBA(surface, x0, y0, x1, y1, 2, 125, 240, 210, weapon_alpha(175.0 * fade));
  }
}


// Starlight crown: a five-point upward arc of pale gold and cyan stars.
static void sky_event_270(const SkyEvent *event) {
  SKY_EVENT_UNPACK(event);
  for (int i = -2; i <= 2; i++) {
    double a = -SCENE_PI * 0.5 + i * 0.35;
    int sx = x + (int)(cos(a) * size * 0.9);
    int sy = y + (int)(sin(a) * size * 0.6) + size / 2;
    filledCircleRGBA(surface, sx, sy, 2, i == 0 ? 255 : 195, i == 0 ? 235 : 225,
                     i == 0 ? 175 : 255, weapon_alpha(210.0 * fade));
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


void (*const sky_event_kinds[])(const SkyEvent *) = {
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
    sky_event_203, sky_event_204, sky_event_205, sky_event_206, sky_event_207, sky_event_208,
    sky_event_209, sky_event_210, sky_event_211, sky_event_212, sky_event_213, sky_event_214,
    sky_event_215, sky_event_216, sky_event_217, sky_event_218, sky_event_219, sky_event_220,
    sky_event_221, sky_event_222, sky_event_223, sky_event_224, sky_event_225, sky_event_226,
    sky_event_227, sky_event_228, sky_event_229, sky_event_230, sky_event_231, sky_event_232,
    sky_event_233, sky_event_234, sky_event_235, sky_event_236, sky_event_237, sky_event_238,
    sky_event_239, sky_event_240, sky_event_241, sky_event_242, sky_event_243, sky_event_244,
    sky_event_245, sky_event_246, sky_event_247, sky_event_248, sky_event_249, sky_event_250,
    sky_event_251, sky_event_252, sky_event_253, sky_event_254, sky_event_255, sky_event_256,
    sky_event_257, sky_event_258, sky_event_259, sky_event_260, sky_event_261, sky_event_262,
    sky_event_263, sky_event_264, sky_event_265, sky_event_266, sky_event_267, sky_event_268,
    sky_event_269, sky_event_270,
    // NEW_SKY_KINDS_ABOVE
};
static_assert(RARE_SKY_CLASSIC_KINDS + sizeof(sky_event_kinds) / sizeof(sky_event_kinds[0]) ==
                  RARE_SKY_KINDS,
              "RARE_SKY_KINDS does not match sky_event_kinds[]");

void render_rare_sky_event(RenderTarget *surface, Uint32 ticks, int scale) {
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


void render_foreground_event(RenderTarget *surface, Uint32 ticks, int scale) {
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


void render_rare_screen_fade(RenderTarget *surface, Uint32 ticks) {
  static bool swapped = false;
  static Uint32 last_slot = 0;
  EffectWindow window;
  if (!effect_window(&EFFECT_SCREEN_FADE, ticks, &window)) {
    swapped = false;
    return;
  }
  Uint8 alpha = (Uint8)(255 * window.fade);
  if (alpha == 255 && (!swapped || last_slot != window.slot)) {
    randomize_hills(window.seed);
    invalidate_backdrop();
    swapped = true;
    last_slot = window.slot;
  }
  Uint8 color = (window.seed & 0x80) ? 255 : 0;
  boxRGBA(surface, 0, 0, surface->w - 1, surface->h - 1, color, color, color, alpha);
}
