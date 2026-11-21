#include "hill_events_internal.h"

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


// Signal tower: a tall wooden watchtower with a rotating amber searchlight.
static void hill_signal_tower(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 101, 233));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(230.0 * event->fade);
  double h = s * 0.085;
  thickLineRGBA(surface, (int)(cx - s * 0.015), (int)g, (int)(cx - s * 0.006), (int)(g - h), 2,
                110, 95, 80, a);
  thickLineRGBA(surface, (int)(cx + s * 0.015), (int)g, (int)(cx + s * 0.006), (int)(g - h), 2,
                110, 95, 80, a);
  lineRGBA(surface, (int)(cx - s * 0.011), (int)(g - h * 0.45), (int)(cx + s * 0.011),
           (int)(g - h * 0.45), 110, 95, 80, a);
  double ang = sin(event->t * 1.2) * 0.55;
  double bx = cx + sin(ang) * s * 0.14;
  double by = (g - h) - cos(ang) * s * 0.04;
  thickLineRGBA(surface, (int)cx, (int)(g - h), (int)bx, (int)by, 3, 255, 215, 130,
                weapon_alpha(55.0 * event->fade));
  static const Uint8 lamp[3] = {255, 215, 135};
  weapon_glow(surface, cx, g - h, 1.3, lamp, 210.0 * event->fade);
}


// Moon garden: pale silver blossoms opening along the ridge crest.
static void hill_moon_garden(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 103, 239));
  double open = smooth_step(0.05, 0.45, event->progress);
  static const Uint8 glow_c[3] = {195, 225, 255};
  for (int i = 0; i < 7; i++) {
    double fx = cx + (i - 3) * s * 0.024;
    double fg = hill_ground(event, fx);
    double fh = s * (0.018 + 0.006 * (i % 3)) * open;
    lineRGBA(surface, (int)fx, (int)fg, (int)fx, (int)(fg - fh), 75, 125, 95,
             weapon_alpha(200.0 * event->fade));
    filledCircleRGBA(surface, (int)fx, (int)(fg - fh), (int)(2 + 2 * open), 220, 235, 255,
                     weapon_alpha(210.0 * open * event->fade));
    weapon_glow(surface, fx, fg - fh, 0.85, glow_c, 120.0 * open * event->fade);
  }
}


// Windmill row: three slender modern turbines turning together on the slope.
static void hill_windmill_row(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 107, 241));
  Uint8 a = weapon_alpha(220.0 * event->fade);
  for (int i = -1; i <= 1; i++) {
    double tx = cx + i * s * 0.045;
    double tg = hill_ground(event, tx);
    double h = s * (0.068 - abs(i) * 0.010);
    thickLineRGBA(surface, (int)tx, (int)tg, (int)tx, (int)(tg - h), 2, 165, 175, 195, a);
    for (int b = 0; b < 3; b++) {
      double ang = event->t * 2.2 + i * 0.8 + b * SCENE_TAU / 3.0;
      lineRGBA(surface, (int)tx, (int)(tg - h), (int)(tx + cos(ang) * s * 0.028),
               (int)(tg - h + sin(ang) * s * 0.028), 205, 215, 230, a);
    }
  }
}


// Shrine torii: a vermilion-tinted gate with two warm stone lanterns.
static void hill_shrine_torii(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 109, 251));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(230.0 * event->fade);
  double w = s * 0.026, h = s * 0.055;
  thickLineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 3, 185, 70, 55, a);
  thickLineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 3, 185, 70, 55, a);
  thickLineRGBA(surface, (int)(cx - w * 1.35), (int)(g - h), (int)(cx + w * 1.35), (int)(g - h), 3,
                205, 80, 60, a);
  thickLineRGBA(surface, (int)(cx - w * 1.1), (int)(g - h * 0.78), (int)(cx + w * 1.1),
                (int)(g - h * 0.78), 2, 180, 70, 55, a);
  static const Uint8 lan[3] = {255, 185, 95};
  weapon_glow(surface, cx - w * 1.6, g - s * 0.014, 0.9, lan, 180.0 * event->fade);
  weapon_glow(surface, cx + w * 1.6, g - s * 0.014, 0.9, lan, 180.0 * event->fade);
}


// Starry tent: a glowing amber canvas tent beside a crackling campfire.
static void hill_starry_tent(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 113, 257));
  double g = hill_ground(event, cx);
  double w = s * 0.032, h = s * 0.036;
  filledTrigonRGBA(surface, (int)(cx - w), (int)g, (int)(cx + w), (int)g, (int)cx, (int)(g - h),
                   220, 145, 70, weapon_alpha(205.0 * event->fade));
  static const Uint8 fire[3] = {255, 165, 75};
  double fx = cx + w * 1.6;
  double fg = hill_ground(event, fx);
  weapon_glow(surface, fx, fg - s * 0.008, 1.0 + 0.2 * sin(event->t * 7.0), fire,
              195.0 * event->fade);
}


// Ice spire: a faceted frost pillar catching faint cyan starlight.
static void hill_ice_spire(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 127, 263));
  double g = hill_ground(event, cx);
  double h = s * 0.075 * smooth_step(0.0, 0.25, event->progress);
  filledTrigonRGBA(surface, (int)(cx - s * 0.016), (int)g, (int)(cx + s * 0.016), (int)g, (int)cx,
                   (int)(g - h), 140, 205, 240, weapon_alpha(190.0 * event->fade));
  filledTrigonRGBA(surface, (int)cx, (int)g, (int)(cx + s * 0.016), (int)g, (int)cx, (int)(g - h),
                   185, 230, 255, weapon_alpha(150.0 * event->fade));
}


// Lantern procession: five small walkers carrying warm lanterns across the hill.
static void hill_lantern_procession(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double lead = (dir > 0) ? surface->w * (0.12 + 0.76 * event->progress)
                          : surface->w * (0.88 - 0.76 * event->progress);
  for (int i = 0; i < 5; i++) {
    double wx = lead - dir * i * s * 0.032;
    double wg = hill_ground(event, wx);
    hill_walker(surface, wx, wg, s * 0.022, dir, event->t * 3.5 + i * 0.9, true, event->fade,
                event->t, i);
  }
}


// Observatory dome: a silver dome opening its slit toward the zenith.
static void hill_observatory_dome(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 131, 269));
  double g = hill_ground(event, cx);
  int r = (int)(s * 0.034);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  filledPieRGBA(surface, (int)cx, (int)(g - s * 0.012), r, 180, 360, 135, 150, 175, a);
  boxRGBA(surface, (int)(cx - r), (int)(g - s * 0.012), (int)(cx + r), (int)g, 95, 108, 128, a);
  double beam = smooth_step(0.2, 0.5, event->progress);
  if (beam > 0.01) {
    lineRGBA(surface, (int)cx, (int)(g - s * 0.044), (int)(cx + s * 0.05),
             (int)(g - s * 0.16), 195, 225, 255, weapon_alpha(110.0 * beam * event->fade));
  }
}


// Weather vane: a tall iron mast with spinning anemometer cups and arrow.
static void hill_weather_vane(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 137, 271));
  double g = hill_ground(event, cx);
  double h = s * 0.068;
  Uint8 a = weapon_alpha(225.0 * event->fade);
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 2, 145, 155, 170, a);
  double spin = cos(event->t * 4.5) * s * 0.018;
  lineRGBA(surface, (int)(cx - spin), (int)(g - h * 0.85), (int)(cx + spin), (int)(g - h * 0.85),
           200, 210, 225, a);
  filledCircleRGBA(surface, (int)(cx + spin), (int)(g - h * 0.85), 2, 220, 230, 240, a);
  filledCircleRGBA(surface, (int)(cx - spin), (int)(g - h * 0.85), 2, 220, 230, 240, a);
  thickLineRGBA(surface, (int)(cx - s * 0.018), (int)(g - h), (int)(cx + s * 0.020), (int)(g - h),
                2, 215, 185, 120, a);
}


// Geyser vent: steam building and erupting in a tall pale plume.
static void hill_geyser_vent(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 139, 277));
  double g = hill_ground(event, cx);
  double erupt = sin(event->progress * SCENE_PI);
  for (int i = 0; i < 12; i++) {
    double u = fmod(event->t * 1.4 + i / 12.0, 1.0);
    double py = g - u * s * 0.13 * (0.3 + 0.7 * erupt);
    double px = cx + sin(u * 6.0 + i) * s * 0.008 * u;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(2 + u * s * 0.008), 205, 225, 240,
                     weapon_alpha(150.0 * (1.0 - u) * event->fade));
  }
}


// Bonsai pine: a windswept cliff pine silhouetted against a soft lantern.
static void hill_bonsai_pine(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 149, 281));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(230.0 * event->fade);
  thickLineRGBA(surface, (int)cx, (int)g, (int)(cx + s * 0.018), (int)(g - s * 0.045), 3, 75, 58,
                48, a);
  filledEllipseRGBA(surface, (int)(cx + s * 0.022), (int)(g - s * 0.048), (int)(s * 0.024),
                    (int)(s * 0.010), 42, 78, 56, a);
  filledEllipseRGBA(surface, (int)(cx - s * 0.004), (int)(g - s * 0.036), (int)(s * 0.016),
                    (int)(s * 0.008), 38, 70, 50, a);
}


// Brazier tripod: an iron tripod bowl burning with steady orange flame.
static void hill_brazier_tripod(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 151, 283));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(230.0 * event->fade);
  double h = s * 0.030;
  lineRGBA(surface, (int)(cx - s * 0.014), (int)g, (int)cx, (int)(g - h), 110, 105, 105, a);
  lineRGBA(surface, (int)(cx + s * 0.014), (int)g, (int)cx, (int)(g - h), 110, 105, 105, a);
  filledEllipseRGBA(surface, (int)cx, (int)(g - h), (int)(s * 0.014), (int)(s * 0.005), 95, 90, 95,
                    a);
  static const Uint8 flame[3] = {255, 145, 55};
  weapon_glow(surface, cx, g - h - s * 0.008, 1.3 + 0.25 * sin(event->t * 8.0), flame,
              215.0 * event->fade);
}


// Flagpole: a tall mast with a pennant snapping briskly in the ridge wind.
static void hill_flagpole(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 157, 293));
  double g = hill_ground(event, cx);
  double h = s * 0.075;
  Uint8 a = weapon_alpha(225.0 * event->fade);
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 2, 185, 195, 210, a);
  double wave = sin(event->t * 9.0) * s * 0.006;
  filledTrigonRGBA(surface, (int)cx, (int)(g - h), (int)cx, (int)(g - h + s * 0.018),
                   (int)(cx + event->direction * s * 0.036), (int)(g - h + s * 0.009 + wave), 195,
                   75, 75, a);
}


// Glowworm arch: a natural stone arch draped in faint blue-green glowworms.
static void hill_glowworm_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 163, 307));
  double g = hill_ground(event, cx);
  double r = s * 0.036;
  Uint8 a = weapon_alpha(215.0 * event->fade);
  for (int i = 0; i < 8; i++) {
    double a0 = SCENE_PI + i * SCENE_PI / 8.0;
    double a1 = SCENE_PI + (i + 1) * SCENE_PI / 8.0;
    thickLineRGBA(surface, (int)(cx + cos(a0) * r), (int)(g + sin(a0) * r),
                  (int)(cx + cos(a1) * r), (int)(g + sin(a1) * r), 4, 65, 72, 85, a);
  }
  static const Uint8 gw[3] = {115, 235, 195};
  for (int i = 1; i < 7; i++) {
    double ang = SCENE_PI + i * SCENE_PI / 7.0;
    weapon_glow(surface, cx + cos(ang) * r * 0.85, g + sin(ang) * r * 0.85, 0.6, gw,
                (140.0 + 50.0 * sin(event->t * 2.5 + i)) * event->fade);
  }
}


// Haystacks: three rounded harvest stacks under drifting firefly-like chaff.
static void hill_haystacks(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 167, 311));
  Uint8 a = weapon_alpha(220.0 * event->fade);
  for (int i = -1; i <= 1; i++) {
    double hx = cx + i * s * 0.036;
    double hg = hill_ground(event, hx);
    int r = (int)(s * (0.016 - abs(i) * 0.002));
    filledPieRGBA(surface, (int)hx, (int)hg, r, 180, 360, 175, 145, 75, a);
  }
}


// Standing stones: a pair of weathered megaliths with a carved glowing rune.
static void hill_standing_stones(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 173, 313));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  boxRGBA(surface, (int)(cx - s * 0.022), (int)(g - s * 0.048), (int)(cx - s * 0.010), (int)g, 85,
          92, 105, a);
  boxRGBA(surface, (int)(cx + s * 0.008), (int)(g - s * 0.062), (int)(cx + s * 0.022), (int)g, 92,
          98, 112, a);
  static const Uint8 rune[3] = {135, 205, 255};
  weapon_glow(surface, cx + s * 0.015, g - s * 0.036, 0.85, rune,
              (135.0 + 55.0 * sin(event->t * 2.0)) * event->fade);
}


// Kite festival: three colorful diamond kites tugging on lines from the hill.
static void hill_kite_festival(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 179, 317));
  double g = hill_ground(event, cx);
  static const Uint8 cols[3][3] = {{235, 115, 95}, {115, 205, 235}, {245, 205, 95}};
  for (int i = 0; i < 3; i++) {
    double ax = cx + (i - 1) * s * 0.030;
    double ag = hill_ground(event, ax);
    double kx = ax + event->direction * s * 0.028 + sin(event->t * 2.2 + i) * s * 0.010;
    double ky = ag - s * (0.075 + 0.015 * i) + cos(event->t * 2.7 + i) * s * 0.007;
    lineRGBA(surface, (int)ax, (int)ag, (int)kx, (int)ky, 180, 190, 205,
             weapon_alpha(130.0 * event->fade));
    filledTrigonRGBA(surface, (int)kx, (int)(ky - s * 0.010), (int)(kx - s * 0.008), (int)ky,
                     (int)(kx + s * 0.008), (int)ky, cols[i][0], cols[i][1], cols[i][2],
                     weapon_alpha(220.0 * event->fade));
    filledTrigonRGBA(surface, (int)kx, (int)(ky + s * 0.012), (int)(kx - s * 0.008), (int)ky,
                     (int)(kx + s * 0.008), (int)ky, cols[i][0], cols[i][1], cols[i][2],
                     weapon_alpha(190.0 * event->fade));
  }
  (void)g;
}


// Firepot row: five clay firepots lining the ridge path with warm embers.
static void hill_firepot_row(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 181, 331));
  static const Uint8 emb[3] = {255, 155, 70};
  for (int i = -2; i <= 2; i++) {
    double px = cx + i * s * 0.024;
    double pg = hill_ground(event, px);
    boxRGBA(surface, (int)(px - 3), (int)(pg - 5), (int)(px + 3), (int)pg, 120, 75, 55,
            weapon_alpha(220.0 * event->fade));
    weapon_glow(surface, px, pg - 7, 0.85, emb,
                (170.0 + 45.0 * sin(event->t * 6.0 + i * 1.3)) * event->fade);
  }
}


// Clockwork bird: a brass mechanical bird flapping steadily on a tall perch.
static void hill_clockwork_bird(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 191, 337));
  double g = hill_ground(event, cx);
  double h = s * 0.052;
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 2, 175, 145, 85,
                weapon_alpha(220.0 * event->fade));
  double flap = sin(event->t * 4.2) * 0.8;
  draw_sleek_bird(surface, cx, g - h - s * 0.008, s * 0.022, event->direction, flap, 0.0, 225, 190,
                  110, weapon_alpha(230.0 * event->fade), true);
}


// Mirage pool: a still reflective basin mirroring two faint star points.
static void hill_mirage_pool(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 193, 347));
  double g = hill_ground(event, cx);
  filledEllipseRGBA(surface, (int)cx, (int)g, (int)(s * 0.042), (int)(s * 0.007), 75, 135, 185,
                    weapon_alpha(180.0 * event->fade));
  static const Uint8 star[3] = {215, 235, 255};
  weapon_glow(surface, cx - s * 0.014, g, 0.65, star,
              (130.0 + 40.0 * sin(event->t * 3.1)) * event->fade);
  weapon_glow(surface, cx + s * 0.016, g, 0.65, star,
              (130.0 + 40.0 * cos(event->t * 2.7)) * event->fade);
}


// Signal mirror: flashes of reflected moonlight blinking in slow Morse pulses.
static void hill_signal_mirror(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 197, 349));
  double g = hill_ground(event, cx);
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - s * 0.036), 2, 145, 155, 170,
                weapon_alpha(210.0 * event->fade));
  double pulse = fmax(0.0, sin(event->t * 5.0));
  static const Uint8 flash[3] = {235, 245, 255};
  weapon_glow(surface, cx, g - s * 0.038, 0.9 + 1.1 * pulse, flash,
              (80.0 + 170.0 * pulse) * event->fade);
}


// Cairn lantern: a stacked stone cairn crowned with a blue-white trail lamp.
static void hill_cairn_lantern(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 199, 353));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  double y = g;
  for (int i = 0; i < 4; i++) {
    int rx = (int)(s * (0.014 - i * 0.0025));
    int ry = (int)(s * 0.005) + 1;
    filledEllipseRGBA(surface, (int)cx, (int)(y - ry), rx, ry, 105 + i * 10, 112 + i * 10,
                      125 + i * 10, a);
    y -= ry * 1.8;
  }
  static const Uint8 lamp[3] = {175, 225, 255};
  weapon_glow(surface, cx, y - 3, 1.0, lamp, 195.0 * event->fade);
}


// Reed boat: a curved crescent skiff resting on the dark ridge swell.
static void hill_reed_boat(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 211, 359));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(220.0 * event->fade);
  double sway = sin(event->t * 1.8) * 0.08;
  filledEllipseRGBA(surface, (int)cx, (int)(g - s * 0.005), (int)(s * 0.034), (int)(s * 0.008),
                    155, 130, 85, a);
  lineRGBA(surface, (int)cx, (int)(g - s * 0.005), (int)(cx + sway * s * 0.1),
           (int)(g - s * 0.045), 185, 165, 120, a);
  filledTrigonRGBA(surface, (int)cx, (int)(g - s * 0.045), (int)cx, (int)(g - s * 0.012),
                   (int)(cx + event->direction * s * 0.024), (int)(g - s * 0.018), 215, 210, 195,
                   weapon_alpha(180.0 * event->fade));
}


// Wind chimes row: three wooden frames with swaying silver tubes.
static void hill_wind_chimes_row(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 223, 367));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(220.0 * event->fade);
  double h = s * 0.048;
  thickLineRGBA(surface, (int)(cx - s * 0.025), (int)g, (int)(cx - s * 0.025), (int)(g - h), 2,
                130, 100, 75, a);
  thickLineRGBA(surface, (int)(cx + s * 0.025), (int)g, (int)(cx + s * 0.025), (int)(g - h), 2,
                130, 100, 75, a);
  thickLineRGBA(surface, (int)(cx - s * 0.028), (int)(g - h), (int)(cx + s * 0.028), (int)(g - h),
                2, 150, 115, 85, a);
  for (int i = -2; i <= 2; i++) {
    double tx = cx + i * s * 0.009;
    double sw = sin(event->t * 3.4 + i * 0.7) * s * 0.004;
    lineRGBA(surface, (int)tx, (int)(g - h), (int)(tx + sw), (int)(g - h * 0.45), 205, 220, 240,
             a);
  }
}


// Sundial plinth: a stone column casting a rotating shadow needle.
static void hill_sundial_plinth(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 227, 373));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  boxRGBA(surface, (int)(cx - s * 0.008), (int)(g - s * 0.032), (int)(cx + s * 0.008), (int)g, 130,
          135, 145, a);
  filledEllipseRGBA(surface, (int)cx, (int)(g - s * 0.034), (int)(s * 0.018), (int)(s * 0.005),
                    165, 170, 180, a);
  double ang = event->t * 0.6;
  lineRGBA(surface, (int)cx, (int)(g - s * 0.034), (int)(cx + cos(ang) * s * 0.015),
           (int)(g - s * 0.044), 225, 200, 130, a);
}


// Star map table: an illuminated circular chart glowing softly on a pedestal.
static void hill_star_map_table(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 229, 379));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(220.0 * event->fade);
  boxRGBA(surface, (int)(cx - s * 0.005), (int)(g - s * 0.026), (int)(cx + s * 0.005), (int)g, 95,
          100, 115, a);
  filledEllipseRGBA(surface, (int)cx, (int)(g - s * 0.028), (int)(s * 0.026), (int)(s * 0.008), 55,
                    95, 145, a);
  static const Uint8 chart[3] = {155, 220, 255};
  weapon_glow(surface, cx, g - s * 0.028, 1.1, chart, 165.0 * event->fade);
}


// Lantern gate: a wooden archway hung with a swaying paper lantern.
static void hill_lantern_gate(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 233, 383));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  double w = s * 0.022, h = s * 0.052;
  thickLineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 2, 115, 85, 65, a);
  thickLineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 2, 115, 85, 65, a);
  thickLineRGBA(surface, (int)(cx - w * 1.2), (int)(g - h), (int)(cx + w * 1.2), (int)(g - h), 3,
                135, 95, 70, a);
  double lx = cx + sin(event->t * 2.4) * s * 0.006;
  double ly = g - h + s * 0.016;
  lineRGBA(surface, (int)cx, (int)(g - h), (int)lx, (int)ly, 160, 140, 120, a);
  static const Uint8 lan[3] = {255, 185, 95};
  weapon_glow(surface, lx, ly, 1.0, lan, 195.0 * event->fade);
}


// Dew web: a silver orb-weaver web glistening between two tall grass stalks.
static void hill_dew_web(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 239, 389));
  double g = hill_ground(event, cx);
  double h = s * 0.045;
  Uint8 a = weapon_alpha(190.0 * event->fade);
  lineRGBA(surface, (int)(cx - s * 0.020), (int)g, (int)(cx - s * 0.022), (int)(g - h), 85, 125,
           95, a);
  lineRGBA(surface, (int)(cx + s * 0.020), (int)g, (int)(cx + s * 0.022), (int)(g - h), 85, 125,
           95, a);
  circleRGBA(surface, (int)cx, (int)(g - h * 0.6), (int)(s * 0.016), 195, 220, 240, a);
  circleRGBA(surface, (int)cx, (int)(g - h * 0.6), (int)(s * 0.009), 195, 220, 240, a);
}


// Ember tree: a leafless dark tree whose tips glow like cooling coals.
static void hill_ember_tree(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 241, 397));
  double g = hill_ground(event, cx);
  double h = s * 0.065;
  Uint8 a = weapon_alpha(225.0 * event->fade);
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h * 0.6), 3, 65, 50, 48, a);
  static const Uint8 emb[3] = {255, 125, 65};
  for (int i = -2; i <= 2; i++) {
    double bx = cx + i * s * 0.015;
    double by = g - h + abs(i) * s * 0.008;
    lineRGBA(surface, (int)cx, (int)(g - h * 0.55), (int)bx, (int)by, 75, 55, 50, a);
    weapon_glow(surface, bx, by, 0.75, emb,
                (145.0 + 45.0 * sin(event->t * 3.0 + i)) * event->fade);
  }
}


// Ice lantern: a carved hollow ice block lit from within by a warm candle.
static void hill_ice_lantern(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 251, 401));
  double g = hill_ground(event, cx);
  int r = (int)(s * 0.014) + 2;
  boxRGBA(surface, (int)(cx - r), (int)(g - r * 2), (int)(cx + r), (int)g, 165, 215, 245,
          weapon_alpha(180.0 * event->fade));
  static const Uint8 can[3] = {255, 195, 105};
  weapon_glow(surface, cx, g - r, 1.1, can,
              (180.0 + 35.0 * sin(event->t * 7.5)) * event->fade);
}


// Bell buoy: a grounded coastal bell frame rocking gently as if on waves.
static void hill_bell_buoy(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 257, 409));
  double g = hill_ground(event, cx);
  double sway = sin(event->t * 2.0) * s * 0.006;
  double h = s * 0.055;
  Uint8 a = weapon_alpha(220.0 * event->fade);
  lineRGBA(surface, (int)(cx - s * 0.015), (int)g, (int)(cx + sway), (int)(g - h), 145, 155, 170,
           a);
  lineRGBA(surface, (int)(cx + s * 0.015), (int)g, (int)(cx + sway), (int)(g - h), 145, 155, 170,
           a);
  filledCircleRGBA(surface, (int)(cx + sway), (int)(g - h * 0.65), (int)(s * 0.006) + 2, 215, 185,
                   110, a);
}


// Frost ferns: delicate silver-blue fern fronds unfurling along the ridge.
static void hill_frost_ferns(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 263, 419));
  double grow = smooth_step(0.0, 0.4, event->progress);
  for (int i = -2; i <= 2; i++) {
    double fx = cx + i * s * 0.018;
    double fg = hill_ground(event, fx);
    double h = s * 0.036 * grow;
    double lean = i * s * 0.008;
    thickLineRGBA(surface, (int)fx, (int)fg, (int)(fx + lean), (int)(fg - h), 2, 165, 215, 245,
                  weapon_alpha(195.0 * event->fade));
  }
}


// Paper lanterns: a low string of warm glowing festival lanterns between posts.
static void hill_paper_lanterns(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 269, 421));
  double span = s * 0.055;
  double g0 = hill_ground(event, cx - span);
  double g1 = hill_ground(event, cx + span);
  double h = s * 0.040;
  Uint8 a = weapon_alpha(220.0 * event->fade);
  thickLineRGBA(surface, (int)(cx - span), (int)g0, (int)(cx - span), (int)(g0 - h), 2, 115, 95,
                80, a);
  thickLineRGBA(surface, (int)(cx + span), (int)g1, (int)(cx + span), (int)(g1 - h), 2, 115, 95,
                80, a);
  static const Uint8 lan[3] = {255, 180, 90};
  for (int i = -2; i <= 2; i++) {
    double u = (i + 2) / 4.0;
    double lx = (cx - span) + u * 2.0 * span;
    double ly = ((g0 - h) * (1.0 - u) + (g1 - h) * u) + sin(u * SCENE_PI) * s * 0.012;
    weapon_glow(surface, lx, ly, 0.85, lan, 185.0 * event->fade);
  }
}


// Crystal cluster: five translucent amethyst crystals catching moonlight.
static void hill_crystal_cluster(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 271, 431));
  double g = hill_ground(event, cx);
  for (int i = -2; i <= 2; i++) {
    double bx = cx + i * s * 0.010;
    double h = s * (0.045 - abs(i) * 0.008);
    double tip_x = bx + i * s * 0.007;
    filledTrigonRGBA(surface, (int)(bx - s * 0.006), (int)g, (int)(bx + s * 0.006), (int)g,
                     (int)tip_x, (int)(g - h), 175, 125, 225, weapon_alpha(195.0 * event->fade));
  }
}


// Smoke signals: rhythmic puffs of pale smoke rising from a ridge fire.
static void hill_smoke_signals(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 277, 433));
  double g = hill_ground(event, cx);
  static const Uint8 emb[3] = {255, 145, 65};
  weapon_glow(surface, cx, g - 3, 0.85, emb, 175.0 * event->fade);
  for (int i = 0; i < 6; i++) {
    double u = fmod(event->t * 0.35 + i / 6.0, 1.0);
    double px = cx + event->direction * u * s * 0.045;
    double py = g - u * s * 0.14;
    filledCircleRGBA(surface, (int)px, (int)py, (int)(3 + u * s * 0.012), 185, 195, 210,
                     weapon_alpha(130.0 * (1.0 - u) * event->fade));
  }
}


// Shepherd hut: a small wheeled wooden hut with one warm window.
static void hill_shepherd_hut(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 281, 439));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  double w = s * 0.026, h = s * 0.028;
  boxRGBA(surface, (int)(cx - w), (int)(g - h), (int)(cx + w), (int)(g - s * 0.005), 115, 92, 74,
          a);
  filledEllipseRGBA(surface, (int)cx, (int)(g - h), (int)(w * 1.05), (int)(s * 0.007), 95, 105,
                    120, a);
  circleRGBA(surface, (int)(cx - w * 0.65), (int)(g - s * 0.005), (int)(s * 0.006) + 1, 135, 120,
             100, a);
  circleRGBA(surface, (int)(cx + w * 0.65), (int)(g - s * 0.005), (int)(s * 0.006) + 1, 135, 120,
             100, a);
  boxRGBA(surface, (int)(cx - 3), (int)(g - h * 0.65), (int)(cx + 3), (int)(g - h * 0.35), 255,
          205, 115, weapon_alpha(230.0 * event->fade));
}


// Moonstone ring: seven low pale stones arranged in an arc on the hill.
static void hill_moonstone_ring(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 283, 443));
  static const Uint8 ms[3] = {190, 220, 255};
  for (int i = -3; i <= 3; i++) {
    double sx = cx + i * s * 0.016;
    double sg = hill_ground(event, sx);
    double h = s * (0.018 - abs(i) * 0.002);
    filledEllipseRGBA(surface, (int)sx, (int)(sg - h * 0.5), (int)(s * 0.005) + 1, (int)(h * 0.5),
                      165, 185, 215, weapon_alpha(210.0 * event->fade));
    weapon_glow(surface, sx, sg - h * 0.7, 0.55, ms, 110.0 * event->fade);
  }
}


// Weather balloon: a silver sounding balloon tethered and bobbing above.
static void hill_weather_balloon(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 293, 449));
  double g = hill_ground(event, cx);
  double bx = cx + sin(event->t * 1.4) * s * 0.015;
  double by = g - s * 0.095 + cos(event->t * 1.9) * s * 0.006;
  lineRGBA(surface, (int)cx, (int)g, (int)bx, (int)by, 165, 175, 190,
           weapon_alpha(150.0 * event->fade));
  filledCircleRGBA(surface, (int)bx, (int)by, (int)(s * 0.016) + 2, 215, 225, 238,
                   weapon_alpha(215.0 * event->fade));
}


// Water wheel: a wooden mill wheel turning beside a dark sluice trough.
static void hill_water_wheel(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 307, 457));
  double g = hill_ground(event, cx);
  int r = (int)(s * 0.026);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  circleRGBA(surface, (int)cx, (int)(g - r), r, 155, 115, 80, a);
  for (int i = 0; i < 6; i++) {
    double ang = event->t * 1.6 + i * SCENE_TAU / 6.0;
    lineRGBA(surface, (int)(cx - cos(ang) * r), (int)(g - r - sin(ang) * r),
             (int)(cx + cos(ang) * r), (int)(g - r + sin(ang) * r), 145, 105, 75, a);
  }
}


// Starry campfire: a bright hearth with drifting golden sparks rising high.
static void hill_starry_campfire(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 311, 461));
  double g = hill_ground(event, cx);
  static const Uint8 fire[3] = {255, 165, 70};
  weapon_glow(surface, cx, g - 4, 1.3 + 0.25 * sin(event->t * 9.0), fire, 210.0 * event->fade);
  for (int i = 0; i < 8; i++) {
    double u = fmod(event->t * 0.7 + i * 0.13, 1.0);
    double sx = cx + sin(u * 9.0 + i) * s * 0.015;
    double sy = g - u * s * 0.09;
    pixelRGBA(surface, (int)sx, (int)sy, 255, 215, 125,
              weapon_alpha(220.0 * (1.0 - u) * event->fade));
  }
}


// Beacon arch: a stone gateway crowned with a steady blue-green beacon.
static void hill_beacon_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 313, 463));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  double w = s * 0.022, h = s * 0.050;
  boxRGBA(surface, (int)(cx - w), (int)(g - h), (int)(cx - w * 0.55), (int)g, 95, 105, 120, a);
  boxRGBA(surface, (int)(cx + w * 0.55), (int)(g - h), (int)(cx + w), (int)g, 95, 105, 120, a);
  boxRGBA(surface, (int)(cx - w * 1.1), (int)(g - h - s * 0.010), (int)(cx + w * 1.1),
          (int)(g - h), 115, 125, 140, a);
  static const Uint8 bcn[3] = {120, 235, 205};
  weapon_glow(surface, cx, g - h - s * 0.016, 1.2, bcn, 205.0 * event->fade);
}


// Night heron: a tall wading silhouette pacing slowly along the ridge.
static void hill_night_heron(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  int dir = event->direction;
  double hx = (dir > 0) ? surface->w * (0.20 + 0.60 * event->progress)
                        : surface->w * (0.80 - 0.60 * event->progress);
  double hg = hill_ground(event, hx);
  Uint8 a = weapon_alpha(220.0 * event->fade);
  lineRGBA(surface, (int)hx, (int)hg, (int)hx, (int)(hg - s * 0.024), 145, 160, 180, a);
  filledEllipseRGBA(surface, (int)hx, (int)(hg - s * 0.026), (int)(s * 0.012), (int)(s * 0.006),
                    155, 175, 200, a);
  thickLineRGBA(surface, (int)(hx + dir * s * 0.008), (int)(hg - s * 0.028),
                (int)(hx + dir * s * 0.014), (int)(hg - s * 0.042), 2, 165, 185, 210, a);
  lineRGBA(surface, (int)(hx + dir * s * 0.014), (int)(hg - s * 0.042),
           (int)(hx + dir * s * 0.026), (int)(hg - s * 0.040), 215, 200, 155, a);
}


// Firefly bush: a dark shrub pulsing with a dozen tiny chartreuse lights.
static void hill_firefly_bush(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 317, 467));
  double g = hill_ground(event, cx);
  filledEllipseRGBA(surface, (int)cx, (int)(g - s * 0.012), (int)(s * 0.028), (int)(s * 0.016),
                    32, 55, 42, weapon_alpha(215.0 * event->fade));
  static const Uint8 ff[3] = {195, 245, 115};
  for (int i = 0; i < 8; i++) {
    double fx = cx + sin(i * 2.1 + event->t * 0.9) * s * 0.026;
    double fy = g - s * 0.014 - fabs(cos(i * 1.7 + event->t * 1.1)) * s * 0.020;
    double blink = fmax(0.0, sin(event->t * 3.5 + i * 1.9));
    weapon_glow(surface, fx, fy, 0.55, ff, 175.0 * blink * event->fade);
  }
}


// Tethered kite: a single glowing box-kite bobbing high above a small stake.
static void hill_tethered_kite(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 331, 479));
  double g = hill_ground(event, cx);
  double kx = cx + event->direction * s * 0.035 + sin(event->t * 1.8) * s * 0.012;
  double ky = g - s * 0.095 + cos(event->t * 2.3) * s * 0.008;
  lineRGBA(surface, (int)cx, (int)g, (int)kx, (int)ky, 185, 195, 210,
           weapon_alpha(135.0 * event->fade));
  boxRGBA(surface, (int)(kx - s * 0.010), (int)(ky - s * 0.010), (int)(kx + s * 0.010),
          (int)(ky + s * 0.010), 235, 165, 95, weapon_alpha(205.0 * event->fade));
}


// Stone well: a mossy circular well with a small pitched shingle roof.
static void hill_stone_well(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 337, 487));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(225.0 * event->fade);
  double w = s * 0.016, h = s * 0.040;
  boxRGBA(surface, (int)(cx - w), (int)(g - s * 0.016), (int)(cx + w), (int)g, 110, 118, 130, a);
  lineRGBA(surface, (int)(cx - w * 0.8), (int)(g - s * 0.016), (int)(cx - w * 0.8), (int)(g - h),
           125, 100, 75, a);
  lineRGBA(surface, (int)(cx + w * 0.8), (int)(g - s * 0.016), (int)(cx + w * 0.8), (int)(g - h),
           125, 100, 75, a);
  filledTrigonRGBA(surface, (int)(cx - w * 1.25), (int)(g - h), (int)(cx + w * 1.25),
                   (int)(g - h), (int)cx, (int)(g - h - s * 0.016), 95, 78, 68, a);
}


// Aurora spire: a slim crystal obelisk pulsing with pale emerald light.
static void hill_aurora_spire(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 347, 491));
  double g = hill_ground(event, cx);
  double h = s * 0.072;
  filledTrigonRGBA(surface, (int)(cx - s * 0.010), (int)g, (int)(cx + s * 0.010), (int)g, (int)cx,
                   (int)(g - h), 115, 215, 185, weapon_alpha(195.0 * event->fade));
  static const Uint8 aur[3] = {125, 245, 200};
  weapon_glow(surface, cx, g - h * 0.75, 1.1, aur,
              (145.0 + 55.0 * sin(event->t * 2.5)) * event->fade);
}


// Lantern boat: a small glowing paper boat perched on a ridge puddle.
static void hill_lantern_boat(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 349, 499));
  double g = hill_ground(event, cx);
  filledEllipseRGBA(surface, (int)cx, (int)g, (int)(s * 0.036), (int)(s * 0.006), 65, 115, 160,
                    weapon_alpha(165.0 * event->fade));
  double bx = cx + sin(event->t * 1.5) * s * 0.014;
  filledTrigonRGBA(surface, (int)(bx - s * 0.010), (int)g, (int)(bx + s * 0.010), (int)g, (int)bx,
                   (int)(g - s * 0.014), 245, 235, 215, weapon_alpha(220.0 * event->fade));
  static const Uint8 lan[3] = {255, 195, 110};
  weapon_glow(surface, bx, g - s * 0.006, 0.7, lan, 160.0 * event->fade);
}


// Camp lantern: a single brass hurricane lamp hanging from a crooked staff.
static void hill_camp_lantern(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 353, 503));
  double g = hill_ground(event, cx);
  double h = s * 0.050;
  Uint8 a = weapon_alpha(220.0 * event->fade);
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 2, 120, 95, 75, a);
  lineRGBA(surface, (int)cx, (int)(g - h), (int)(cx + s * 0.016), (int)(g - h), 120, 95, 75, a);
  double lx = cx + s * 0.016 + sin(event->t * 2.8) * s * 0.004;
  double ly = g - h + s * 0.012;
  lineRGBA(surface, (int)(cx + s * 0.016), (int)(g - h), (int)lx, (int)ly, 155, 145, 130, a);
  static const Uint8 lamp[3] = {255, 190, 95};
  weapon_glow(surface, lx, ly, 1.1, lamp, 210.0 * event->fade);
}


// Wind sock: an illuminated striped cone streaming from a ridge pole.
static void hill_wind_sock(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 359, 509));
  double g = hill_ground(event, cx);
  double h = s * 0.062;
  Uint8 a = weapon_alpha(220.0 * event->fade);
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 2, 175, 185, 200, a);
  double wave = sin(event->t * 7.0) * s * 0.004;
  filledTrigonRGBA(surface, (int)cx, (int)(g - h), (int)cx, (int)(g - h + s * 0.012),
                   (int)(cx + event->direction * s * 0.038), (int)(g - h + s * 0.006 + wave), 235,
                   125, 75, a);
}


// Moon gate: a circular stone portal framing a soft glow of mist.
static void hill_moon_gate(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 367, 521));
  double g = hill_ground(event, cx);
  int r = (int)(s * 0.028);
  Uint8 a = weapon_alpha(220.0 * event->fade);
  filledCircleRGBA(surface, (int)cx, (int)(g - r), r, 115, 145, 180,
                   weapon_alpha(75.0 * event->fade));
  circleRGBA(surface, (int)cx, (int)(g - r), r, 145, 155, 170, a);
  circleRGBA(surface, (int)cx, (int)(g - r), r + 2, 125, 135, 150, a);
}


// Starry trellis: a garden arch covered in tiny twinkling white blossoms.
static void hill_starry_trellis(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 373, 523));
  double g = hill_ground(event, cx);
  double r = s * 0.030;
  Uint8 a = weapon_alpha(210.0 * event->fade);
  for (int i = 0; i < 8; i++) {
    double a0 = SCENE_PI + i * SCENE_PI / 8.0;
    double a1 = SCENE_PI + (i + 1) * SCENE_PI / 8.0;
    lineRGBA(surface, (int)(cx + cos(a0) * r), (int)(g + sin(a0) * r), (int)(cx + cos(a1) * r),
             (int)(g + sin(a1) * r), 115, 145, 105, a);
  }
  static const Uint8 bl[3] = {235, 245, 255};
  for (int i = 0; i <= 6; i++) {
    double ang = SCENE_PI + i * SCENE_PI / 6.0;
    weapon_glow(surface, cx + cos(ang) * r, g + sin(ang) * r, 0.55, bl,
                (140.0 + 60.0 * sin(event->t * 2.8 + i)) * event->fade);
  }
}


// Radio beacon: a tall radio tower with a rhythmic crimson warning strobe.
static void hill_radio_beacon(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 379, 541));
  double g = hill_ground(event, cx);
  double h = s * 0.092;
  Uint8 a = weapon_alpha(220.0 * event->fade);
  lineRGBA(surface, (int)(cx - s * 0.010), (int)g, (int)cx, (int)(g - h), 145, 155, 175, a);
  lineRGBA(surface, (int)(cx + s * 0.010), (int)g, (int)cx, (int)(g - h), 145, 155, 175, a);
  double blink = fmax(0.0, sin(event->t * 4.5));
  static const Uint8 red[3] = {255, 75, 75};
  weapon_glow(surface, cx, g - h, 0.9 + 0.5 * blink, red, 220.0 * blink * event->fade);
}


// Owl perch: a perched barn owl rotating its head slowly on a crooked post.
static void hill_owl_perch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 383, 547));
  double g = hill_ground(event, cx);
  double h = s * 0.044;
  Uint8 a = weapon_alpha(225.0 * event->fade);
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 3, 105, 85, 68, a);
  filledEllipseRGBA(surface, (int)cx, (int)(g - h - s * 0.009), (int)(s * 0.008), (int)(s * 0.011),
                    205, 195, 180, a);
  filledCircleRGBA(surface, (int)(cx + sin(event->t * 1.1) * 2.0), (int)(g - h - s * 0.020),
                   (int)(s * 0.006) + 1, 225, 215, 200, a);
}


// Snowdrift tree: a frost-laden evergreen shimmering in the night breeze.
static void hill_snowdrift_tree(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 389, 557));
  double g = hill_ground(event, cx);
  Uint8 a = weapon_alpha(220.0 * event->fade);
  for (int i = 0; i < 3; i++) {
    double y0 = g - s * (0.015 + i * 0.018);
    double w = s * (0.022 - i * 0.005);
    filledTrigonRGBA(surface, (int)(cx - w), (int)y0, (int)(cx + w), (int)y0, (int)cx,
                     (int)(y0 - s * 0.026), 165, 200, 225, a);
  }
}


// Glow mushrooms: a cluster of bioluminescent toadstools pulsing softly.
static void hill_glow_mushrooms(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 397, 563));
  static const Uint8 bio[3] = {125, 235, 215};
  for (int i = -1; i <= 1; i++) {
    double mx = cx + i * s * 0.015;
    double mg = hill_ground(event, mx);
    double h = s * (0.018 - abs(i) * 0.004);
    lineRGBA(surface, (int)mx, (int)mg, (int)mx, (int)(mg - h), 175, 215, 210,
             weapon_alpha(200.0 * event->fade));
    filledPieRGBA(surface, (int)mx, (int)(mg - h), (int)(s * 0.009) + 2, 180, 360, 115, 230, 210,
                  weapon_alpha(215.0 * event->fade));
    weapon_glow(surface, mx, mg - h, 0.75, bio,
                (130.0 + 45.0 * sin(event->t * 2.5 + i)) * event->fade);
  }
}


// Lantern bridge: a low arched footbridge with two warm corner lamps.
static void hill_lantern_bridge_two(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 401, 569));
  double g = hill_ground(event, cx);
  double w = s * 0.045;
  Uint8 a = weapon_alpha(220.0 * event->fade);
  thickLineRGBA(surface, (int)(cx - w), (int)g, (int)cx, (int)(g - s * 0.016), 3, 135, 105, 80, a);
  thickLineRGBA(surface, (int)cx, (int)(g - s * 0.016), (int)(cx + w), (int)g, 3, 135, 105, 80, a);
  static const Uint8 lan[3] = {255, 190, 95};
  weapon_glow(surface, cx - w, g - s * 0.018, 0.85, lan, 185.0 * event->fade);
  weapon_glow(surface, cx + w, g - s * 0.018, 0.85, lan, 185.0 * event->fade);
}


// Starry obelisk: a tall dark monolith etched with rising golden light.
static void hill_starry_obelisk(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 409, 571));
  double g = hill_ground(event, cx);
  double h = s * 0.080;
  boxRGBA(surface, (int)(cx - s * 0.008), (int)(g - h), (int)(cx + s * 0.008), (int)g, 55, 65, 82,
          weapon_alpha(225.0 * event->fade));
  filledTrigonRGBA(surface, (int)(cx - s * 0.008), (int)(g - h), (int)(cx + s * 0.008),
                   (int)(g - h), (int)cx, (int)(g - h - s * 0.014), 75, 88, 110,
                   weapon_alpha(225.0 * event->fade));
  static const Uint8 gold[3] = {255, 215, 125};
  weapon_glow(surface, cx, g - h - s * 0.014, 0.9, gold, 180.0 * event->fade);
}


// Crane roost: two slender cranes standing motionless on one leg.
static void hill_crane_roost(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 419, 577));
  Uint8 a = weapon_alpha(215.0 * event->fade);
  for (int i = 0; i < 2; i++) {
    double bx = cx + (i - 0.5) * s * 0.028;
    double bg = hill_ground(event, bx);
    lineRGBA(surface, (int)bx, (int)bg, (int)bx, (int)(bg - s * 0.022), 185, 195, 210, a);
    filledEllipseRGBA(surface, (int)bx, (int)(bg - s * 0.024), (int)(s * 0.010), (int)(s * 0.005),
                      220, 228, 240, a);
    lineRGBA(surface, (int)(bx + s * 0.008), (int)(bg - s * 0.024), (int)(bx + s * 0.012),
             (int)(bg - s * 0.036), 220, 228, 240, a);
  }
}


// Wind harp: a tall stringed frame humming with shimmering silver waves.
static void hill_wind_harp_two(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 421, 587));
  double g = hill_ground(event, cx);
  double h = s * 0.065;
  Uint8 a = weapon_alpha(220.0 * event->fade);
  thickLineRGBA(surface, (int)(cx - s * 0.016), (int)g, (int)(cx - s * 0.008), (int)(g - h), 2,
                165, 145, 105, a);
  thickLineRGBA(surface, (int)(cx + s * 0.016), (int)g, (int)(cx + s * 0.008), (int)(g - h), 2,
                165, 145, 105, a);
  lineRGBA(surface, (int)(cx - s * 0.008), (int)(g - h), (int)(cx + s * 0.008), (int)(g - h), 185,
           165, 120, a);
  for (int i = -1; i <= 1; i++) {
    double sx = cx + i * s * 0.005;
    lineRGBA(surface, (int)sx, (int)g, (int)(sx + sin(event->t * 10.0 + i) * 2.0), (int)(g - h),
             205, 225, 245, weapon_alpha(165.0 * event->fade));
  }
}


// Beacon dome: a geodesic glass dome glowing warmly from inside.
static void hill_beacon_dome(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 431, 593));
  double g = hill_ground(event, cx);
  int r = (int)(s * 0.028);
  filledPieRGBA(surface, (int)cx, (int)g, r, 180, 360, 235, 185, 110,
                weapon_alpha(150.0 * event->fade));
  circleRGBA(surface, (int)cx, (int)g, r, 185, 205, 225, weapon_alpha(200.0 * event->fade));
}


// Fire wheel: a spinning spark-wheel mounted on a wooden ridge post.
static void hill_fire_wheel_two(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 433, 599));
  double g = hill_ground(event, cx);
  double h = s * 0.048;
  lineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 125, 105, 85,
           weapon_alpha(200.0 * event->fade));
  static const Uint8 spk[3] = {255, 185, 85};
  for (int i = 0; i < 6; i++) {
    double ang = event->t * 5.0 + i * SCENE_TAU / 6.0;
    double sx = cx + cos(ang) * s * 0.020;
    double sy = g - h + sin(ang) * s * 0.020;
    weapon_glow(surface, sx, sy, 0.7, spk, 175.0 * event->fade);
  }
}


// Dew flowers: tall pale blossoms swaying gently in the night breeze.
static void hill_dew_flowers(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 439, 601));
  for (int i = -2; i <= 2; i++) {
    double fx = cx + i * s * 0.018;
    double fg = hill_ground(event, fx);
    double sway = sin(event->t * 1.9 + i * 0.8) * s * 0.006;
    double h = s * (0.026 + (abs(i) % 2) * 0.008);
    lineRGBA(surface, (int)fx, (int)fg, (int)(fx + sway), (int)(fg - h), 85, 135, 100,
             weapon_alpha(200.0 * event->fade));
    filledCircleRGBA(surface, (int)(fx + sway), (int)(fg - h), 3, 220, 235, 255,
                     weapon_alpha(220.0 * event->fade));
  }
}


// Star lantern: a hanging brass-framed lantern swaying from a crooked ridge post.
static void hill_star_lantern(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 443, 607));
  double g = hill_ground(event, cx);
  double h = s * 0.056;
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 2, 110, 95, 80,
                weapon_alpha(210.0 * event->fade));
  double arm_x = cx + event->direction * s * 0.022;
  lineRGBA(surface, (int)cx, (int)(g - h), (int)arm_x, (int)(g - h + s * 0.005), 110, 95, 80,
           weapon_alpha(200.0 * event->fade));
  double swing = sin(event->t * 2.1) * s * 0.004;
  double lx = arm_x + swing;
  double ly = g - h + s * 0.018;
  lineRGBA(surface, (int)arm_x, (int)(g - h + s * 0.005), (int)lx, (int)ly, 150, 140, 120,
           weapon_alpha(190.0 * event->fade));
  static const Uint8 lan[3] = {255, 210, 110};
  weapon_glow(surface, lx, ly, 1.2, lan, 180.0 * event->fade);
  filledCircleRGBA(surface, (int)lx, (int)ly, 3, 255, 230, 155,
                   weapon_alpha(230.0 * event->fade));
}


// Frost chimes: icy crystal rods suspended from a horizontal ridge bar.
static void hill_frost_chimes(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 449, 613));
  double g = hill_ground(event, cx);
  double h = s * 0.048;
  double w = s * 0.028;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 120, 130, 145,
           weapon_alpha(190.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 120, 130, 145,
           weapon_alpha(190.0 * event->fade));
  lineRGBA(surface, (int)(cx - w), (int)(g - h), (int)(cx + w), (int)(g - h), 135, 145, 160,
           weapon_alpha(200.0 * event->fade));
  for (int i = -2; i <= 2; i++) {
    double rx = cx + i * s * 0.009;
    double sway = sin(event->t * 2.4 + i * 0.9) * s * 0.003;
    double len = s * (0.016 + (abs(i) % 2) * 0.007);
    thickLineRGBA(surface, (int)rx, (int)(g - h), (int)(rx + sway), (int)(g - h + len), 2, 185,
                  230, 255, weapon_alpha(210.0 * event->fade));
  }
}


// Ember braziers: three low iron bowls glowing with pulsing orange coals.
static void hill_ember_braziers(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 457, 617));
  static const Uint8 coal[3] = {255, 130, 60};
  for (int i = -1; i <= 1; i++) {
    double bx = cx + i * s * 0.032;
    double bg = hill_ground(event, bx);
    double pulse = 0.75 + 0.25 * sin(event->t * 3.5 + i * 1.4);
    filledTrigonRGBA(surface, (int)(bx - s * 0.010), (int)(bg - s * 0.012),
                     (int)(bx + s * 0.010), (int)(bg - s * 0.012), (int)bx, (int)bg, 75, 70, 75,
                     weapon_alpha(215.0 * event->fade));
    weapon_glow(surface, bx, bg - s * 0.014, 1.0, coal, 175.0 * pulse * event->fade);
  }
}


// Stone arch: a weathered megalithic arch framing a small flickering hearth.
static void hill_stone_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 461, 619));
  double g = hill_ground(event, cx);
  double w = s * 0.022;
  double h = s * 0.044;
  boxRGBA(surface, (int)(cx - w - 3), (int)(g - h), (int)(cx - w + 3), (int)g, 90, 95, 108,
          weapon_alpha(220.0 * event->fade));
  boxRGBA(surface, (int)(cx + w - 3), (int)(g - h), (int)(cx + w + 3), (int)g, 90, 95, 108,
          weapon_alpha(220.0 * event->fade));
  boxRGBA(surface, (int)(cx - w - 5), (int)(g - h - 5), (int)(cx + w + 5), (int)(g - h + 2), 100,
          105, 118, weapon_alpha(225.0 * event->fade));
  static const Uint8 hth[3] = {255, 175, 90};
  weapon_glow(surface, cx, g - s * 0.008, 0.9, hth, 165.0 * event->fade);
}


// Lantern post row: four slender ridge posts lighting up in sequence.
static void hill_lantern_post_row(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 463, 631));
  static const Uint8 lan[3] = {255, 215, 130};
  for (int i = 0; i < 4; i++) {
    double px = cx + (i - 1.5) * s * 0.026;
    double pg = hill_ground(event, px);
    double h = s * 0.036;
    lineRGBA(surface, (int)px, (int)pg, (int)px, (int)(pg - h), 100, 95, 90,
             weapon_alpha(200.0 * event->fade));
    double lit = smooth_step(i * 0.12, i * 0.12 + 0.15, event->progress);
    weapon_glow(surface, px, pg - h, 0.85, lan, 180.0 * lit * event->fade);
  }
}


// Reed lanterns: swaying tall reeds tipped with soft amber-green bulbs.
static void hill_reed_lanterns(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 467, 641));
  static const Uint8 bulb[3] = {195, 240, 140};
  for (int i = -2; i <= 2; i++) {
    double rx = cx + i * s * 0.015;
    double rg = hill_ground(event, rx);
    double sway = sin(event->t * 2.2 + i * 0.7) * s * 0.008;
    double h = s * (0.030 + (abs(i) % 2) * 0.010);
    lineRGBA(surface, (int)rx, (int)rg, (int)(rx + sway), (int)(rg - h), 80, 120, 85,
             weapon_alpha(200.0 * event->fade));
    weapon_glow(surface, rx + sway, rg - h, 0.65, bulb, 165.0 * event->fade);
  }
}


// Star sundial: a stone dial whose gnomon projects a rotating beam of starlight.
static void hill_star_sundial(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.26 + 0.48 * hill_random(event, 479, 643));
  double g = hill_ground(event, cx);
  boxRGBA(surface, (int)(cx - s * 0.022), (int)(g - s * 0.010), (int)(cx + s * 0.022), (int)g, 95,
          100, 115, weapon_alpha(215.0 * event->fade));
  double ang = -SCENE_PI * 0.5 + sin(event->t * 0.9) * 0.65;
  double gx = cx + cos(ang) * s * 0.032;
  double gy = g - s * 0.010 + sin(ang) * s * 0.032;
  thickLineRGBA(surface, (int)cx, (int)(g - s * 0.010), (int)gx, (int)gy, 2, 190, 210, 240,
                weapon_alpha(210.0 * event->fade));
  static const Uint8 tip[3] = {175, 220, 255};
  weapon_glow(surface, gx, gy, 0.85, tip, 170.0 * event->fade);
}


// Windmill beacon: a small ridge windmill with a warm lantern at the hub.
static void hill_windmill_beacon(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 487, 647));
  double g = hill_ground(event, cx);
  double h = s * 0.048;
  filledTrigonRGBA(surface, (int)(cx - s * 0.014), (int)g, (int)(cx + s * 0.014), (int)g,
                   (int)cx, (int)(g - h), 85, 80, 90, weapon_alpha(215.0 * event->fade));
  double rot = event->t * 1.8 * event->direction;
  for (int i = 0; i < 4; i++) {
    double a = rot + i * SCENE_PI * 0.5;
    lineRGBA(surface, (int)cx, (int)(g - h), (int)(cx + cos(a) * s * 0.026),
             (int)(g - h + sin(a) * s * 0.026), 175, 170, 160,
             weapon_alpha(205.0 * event->fade));
  }
  static const Uint8 hub[3] = {255, 210, 125};
  weapon_glow(surface, cx, g - h, 0.8, hub, 175.0 * event->fade);
}


// Ice crystals: three translucent blue-white spires catching starlight on the ridge.
static void hill_ice_crystals(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 491, 653));
  for (int i = -1; i <= 1; i++) {
    double px = cx + i * s * 0.014;
    double pg = hill_ground(event, px);
    double h = s * (0.044 - abs(i) * 0.011);
    double w = s * 0.007;
    filledTrigonRGBA(surface, (int)(px - w), (int)pg, (int)(px + w), (int)pg, (int)px,
                     (int)(pg - h), 165, 220, 255, weapon_alpha(190.0 * event->fade));
  }
}


// Lantern arch two: a wooden moon-arch hung with five warm glowing paper globes.
static void hill_lantern_arch_two(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 499, 659));
  double g = hill_ground(event, cx);
  double r = s * 0.032;
  static const Uint8 lan[3] = {255, 190, 95};
  for (int i = 0; i <= 4; i++) {
    double a = SCENE_PI + i * SCENE_PI / 4.0;
    double lx = cx + cos(a) * r;
    double ly = g + sin(a) * r;
    weapon_glow(surface, lx, ly, 0.7, lan, 165.0 * event->fade);
  }
}


// Fire totem: a carved wooden pillar crowned with a flickering amber flame.
static void hill_fire_totem(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 503, 661));
  double g = hill_ground(event, cx);
  double h = s * 0.048;
  boxRGBA(surface, (int)(cx - 3), (int)(g - h), (int)(cx + 3), (int)g, 95, 78, 65,
          weapon_alpha(220.0 * event->fade));
  static const Uint8 flm[3] = {255, 160, 70};
  double pulse = 0.8 + 0.2 * sin(event->t * 4.5);
  weapon_glow(surface, cx, g - h - s * 0.006, 1.1 * pulse, flm, 190.0 * event->fade);
}


// Star flowerbed: five glowing silver-blue blossoms opening along the ridge.
static void hill_star_flowerbed(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 509, 673));
  static const Uint8 blm[3] = {185, 225, 255};
  for (int i = -2; i <= 2; i++) {
    double fx = cx + i * s * 0.016;
    double fg = hill_ground(event, fx);
    double h = s * (0.016 + (abs(i) % 2) * 0.007);
    lineRGBA(surface, (int)fx, (int)fg, (int)fx, (int)(fg - h), 85, 125, 95,
             weapon_alpha(195.0 * event->fade));
    weapon_glow(surface, fx, fg - h, 0.65, blm, 175.0 * event->fade);
  }
}


// Clockwork owl: a brass owl perched on a post with blinking amber eyes.
static void hill_clockwork_owl(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.26 + 0.48 * hill_random(event, 521, 677));
  double g = hill_ground(event, cx);
  double h = s * 0.042;
  lineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 100, 90, 80,
           weapon_alpha(205.0 * event->fade));
  filledEllipseRGBA(surface, (int)cx, (int)(g - h - s * 0.008), 5, 7, 175, 145, 95,
                    weapon_alpha(225.0 * event->fade));
  if (fmod(event->t, 3.2) > 0.2) {
    pixelRGBA(surface, (int)cx - 2, (int)(g - h - s * 0.011), 255, 220, 100,
              weapon_alpha(240.0 * event->fade));
    pixelRGBA(surface, (int)cx + 2, (int)(g - h - s * 0.011), 255, 220, 100,
              weapon_alpha(240.0 * event->fade));
  }
}


// Ether windmill: a slender tower with six glowing cyan-white sails turning slowly.
static void hill_ether_windmill(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 523, 683));
  double g = hill_ground(event, cx);
  double h = s * 0.052;
  lineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 115, 130, 150,
           weapon_alpha(205.0 * event->fade));
  double rot = event->t * 1.4;
  for (int i = 0; i < 6; i++) {
    double a = rot + i * SCENE_TAU / 6.0;
    lineRGBA(surface, (int)cx, (int)(g - h), (int)(cx + cos(a) * s * 0.022),
             (int)(g - h + sin(a) * s * 0.022), 165, 225, 255,
             weapon_alpha(200.0 * event->fade));
  }
}


// Crystal obelisk: a tall amethyst spire pulsing gently against the night sky.
static void hill_crystal_obelisk(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 541, 691));
  double g = hill_ground(event, cx);
  double h = s * 0.056;
  double w = s * 0.008;
  filledTrigonRGBA(surface, (int)(cx - w), (int)g, (int)(cx + w), (int)g, (int)cx, (int)(g - h),
                   175, 135, 235, weapon_alpha(195.0 * event->fade));
  static const Uint8 cry[3] = {205, 165, 255};
  double pulse = 0.7 + 0.3 * sin(event->t * 2.5);
  weapon_glow(surface, cx, g - h * 0.7, 1.0, cry, 165.0 * pulse * event->fade);
}


// Sky mirror: a tilted silver dish on the ridge reflecting a faint beam upward.
static void hill_sky_mirror(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 547, 701));
  double g = hill_ground(event, cx);
  lineRGBA(surface, (int)(cx - s * 0.012), (int)g, (int)cx, (int)(g - s * 0.020), 100, 110, 125,
           weapon_alpha(200.0 * event->fade));
  lineRGBA(surface, (int)(cx + s * 0.012), (int)g, (int)cx, (int)(g - s * 0.020), 100, 110, 125,
           weapon_alpha(200.0 * event->fade));
  thickLineRGBA(surface, (int)(cx - s * 0.016), (int)(g - s * 0.026), (int)(cx + s * 0.016),
                (int)(g - s * 0.016), 2, 200, 220, 240, weapon_alpha(220.0 * event->fade));
  lineRGBA(surface, (int)cx, (int)(g - s * 0.021), (int)(cx + event->direction * s * 0.025),
           (int)(g - s * 0.085), 185, 220, 255, weapon_alpha(110.0 * event->fade));
}


// Bell post: a small bronze bell swinging gently beneath a wooden crossbeam.
static void hill_bell_post(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 557, 709));
  double g = hill_ground(event, cx);
  double h = s * 0.044;
  lineRGBA(surface, (int)(cx - s * 0.012), (int)g, (int)(cx - s * 0.012), (int)(g - h), 100, 85,
           70, weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx + s * 0.012), (int)g, (int)(cx + s * 0.012), (int)(g - h), 100, 85,
           70, weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx - s * 0.014), (int)(g - h), (int)(cx + s * 0.014), (int)(g - h), 110,
           95, 78, weapon_alpha(210.0 * event->fade));
  double sw = sin(event->t * 2.3) * s * 0.004;
  filledCircleRGBA(surface, (int)(cx + sw), (int)(g - h + s * 0.010), 4, 210, 175, 105,
                   weapon_alpha(225.0 * event->fade));
}


// Fire pit: a stone-ringed fire pit sending up faint drifting embers.
static void hill_fire_pit(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 563, 719));
  double g = hill_ground(event, cx);
  filledEllipseRGBA(surface, (int)cx, (int)g, (int)(s * 0.014), 3, 85, 80, 82,
                    weapon_alpha(215.0 * event->fade));
  static const Uint8 pit[3] = {255, 145, 65};
  weapon_glow(surface, cx, g - s * 0.005, 1.0, pit, 185.0 * event->fade);
  for (int i = 0; i < 4; i++) {
    double rise = fmod(event->t * 0.5 + i * 0.25, 1.0);
    double ex = cx + sin(event->t * 2.0 + i * 1.6) * s * 0.008;
    double ey = g - rise * s * 0.040;
    pixelRGBA(surface, (int)ex, (int)ey, 255, 195, 105,
              weapon_alpha(200.0 * (1.0 - rise) * event->fade));
  }
}


// Moon pond: a shallow ridge pool reflecting a shimmering silver crescent.
static void hill_moon_pond(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 569, 727));
  double g = hill_ground(event, cx);
  filledEllipseRGBA(surface, (int)cx, (int)g, (int)(s * 0.026), 4, 55, 85, 130,
                    weapon_alpha(190.0 * event->fade));
  double shim = sin(event->t * 2.8) * s * 0.006;
  lineRGBA(surface, (int)(cx - s * 0.010 + shim), (int)g, (int)(cx + s * 0.010 + shim), (int)g,
           210, 230, 255, weapon_alpha(200.0 * event->fade));
}


// Lantern fence: a low wooden ridge fence hung with three warm paper lanterns.
static void hill_lantern_fence(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 571, 733));
  static const Uint8 lan[3] = {255, 195, 105};
  for (int i = -1; i <= 1; i++) {
    double px = cx + i * s * 0.025;
    double pg = hill_ground(event, px);
    lineRGBA(surface, (int)px, (int)pg, (int)px, (int)(pg - s * 0.022), 95, 82, 70,
             weapon_alpha(200.0 * event->fade));
    weapon_glow(surface, px, pg - s * 0.022, 0.75, lan, 165.0 * event->fade);
  }
  double g0 = hill_ground(event, cx - s * 0.025);
  double g1 = hill_ground(event, cx + s * 0.025);
  lineRGBA(surface, (int)(cx - s * 0.025), (int)(g0 - s * 0.016), (int)(cx + s * 0.025),
           (int)(g1 - s * 0.016), 95, 82, 70, weapon_alpha(190.0 * event->fade));
}


// Star telescope: a brass tripod telescope scanning the night sky.
static void hill_star_telescope(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 577, 739));
  double g = hill_ground(event, cx);
  double piv_y = g - s * 0.024;
  lineRGBA(surface, (int)(cx - s * 0.010), (int)g, (int)cx, (int)piv_y, 120, 110, 95,
           weapon_alpha(210.0 * event->fade));
  lineRGBA(surface, (int)(cx + s * 0.010), (int)g, (int)cx, (int)piv_y, 120, 110, 95,
           weapon_alpha(210.0 * event->fade));
  double ang = -0.6 + sin(event->t * 0.7) * 0.35;
  double tx = cx + event->direction * cos(ang) * s * 0.028;
  double ty = piv_y + sin(ang) * s * 0.028;
  thickLineRGBA(surface, (int)cx, (int)piv_y, (int)tx, (int)ty, 3, 200, 175, 110,
                weapon_alpha(225.0 * event->fade));
}


// Glowing moss: patches of bioluminescent teal moss pulsing along the ridge.
static void hill_glowing_moss(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 587, 743));
  static const Uint8 mos[3] = {105, 235, 195};
  for (int i = -2; i <= 2; i++) {
    double mx = cx + i * s * 0.018;
    double mg = hill_ground(event, mx);
    double pulse = 0.65 + 0.35 * sin(event->t * 2.0 + i * 1.1);
    weapon_glow(surface, mx, mg - 2, 0.75, mos, 155.0 * pulse * event->fade);
  }
}


// Frost arch: a crystalline ice arch sparkling softly on the ridge.
static void hill_frost_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 593, 751));
  double g = hill_ground(event, cx);
  double r = s * 0.028;
  for (int i = 0; i < 10; i++) {
    double a0 = SCENE_PI + i * SCENE_PI / 10.0;
    double a1 = SCENE_PI + (i + 1) * SCENE_PI / 10.0;
    thickLineRGBA(surface, (int)(cx + cos(a0) * r), (int)(g + sin(a0) * r),
                  (int)(cx + cos(a1) * r), (int)(g + sin(a1) * r), 3, 175, 225, 255,
                  weapon_alpha(200.0 * event->fade));
  }
}


// Torch circle: four wooden torches arranged in a shallow arc on the ridge.
static void hill_torch_circle(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 599, 757));
  static const Uint8 trc[3] = {255, 165, 70};
  for (int i = 0; i < 4; i++) {
    double tx = cx + (i - 1.5) * s * 0.020;
    double tg = hill_ground(event, tx);
    double h = s * 0.024;
    lineRGBA(surface, (int)tx, (int)tg, (int)tx, (int)(tg - h), 100, 80, 65,
             weapon_alpha(205.0 * event->fade));
    double flicker = 0.8 + 0.2 * sin(event->t * 5.0 + i * 1.3);
    weapon_glow(surface, tx, tg - h, 0.8, trc, 175.0 * flicker * event->fade);
  }
}


// Reed wind: a cluster of tall silver-tipped reeds bending together in gusts.
static void hill_reed_wind(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 601, 761));
  double gust = sin(event->t * 2.1) * s * 0.012;
  for (int i = -3; i <= 3; i++) {
    double rx = cx + i * s * 0.010;
    double rg = hill_ground(event, rx);
    double h = s * (0.026 + (abs(i) % 2) * 0.008);
    lineRGBA(surface, (int)rx, (int)rg, (int)(rx + gust), (int)(rg - h), 110, 145, 115,
             weapon_alpha(200.0 * event->fade));
    pixelRGBA(surface, (int)(rx + gust), (int)(rg - h), 210, 225, 215,
              weapon_alpha(220.0 * event->fade));
  }
}


// Beacon cairn: a stacked stone cairn topped with a pulsing blue-white crystal.
static void hill_beacon_cairn(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.25 + 0.50 * hill_random(event, 607, 769));
  double g = hill_ground(event, cx);
  for (int i = 0; i < 4; i++) {
    int rw = (int)(s * (0.012 - i * 0.002));
    filledEllipseRGBA(surface, (int)cx, (int)(g - i * 4), rw, 3, 100 + i * 8, 105 + i * 8,
                      115 + i * 8, weapon_alpha(220.0 * event->fade));
  }
  static const Uint8 cry[3] = {175, 225, 255};
  double pulse = 0.7 + 0.3 * sin(event->t * 2.7);
  weapon_glow(surface, cx, g - 16, 0.9, cry, 175.0 * pulse * event->fade);
}


// Star pavilion: a small pointed-roof ridge pavilion lit by a warm central lamp.
static void hill_star_pavilion(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 613, 773));
  double g = hill_ground(event, cx);
  double w = s * 0.020;
  double h = s * 0.028;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 100, 95, 110,
           weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 100, 95, 110,
           weapon_alpha(205.0 * event->fade));
  filledTrigonRGBA(surface, (int)(cx - w - 4), (int)(g - h), (int)(cx + w + 4), (int)(g - h),
                   (int)cx, (int)(g - h - s * 0.016), 85, 80, 95,
                   weapon_alpha(220.0 * event->fade));
  static const Uint8 lmp[3] = {255, 210, 120};
  weapon_glow(surface, cx, g - h * 0.5, 0.9, lmp, 180.0 * event->fade);
}


// Star arch: two slender stone pillars topped with a glowing silver-gold arc.
static void hill_star_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.24 + 0.52 * hill_random(event, 617, 787));
  double g = hill_ground(event, cx);
  double w = s * 0.024;
  double h = s * 0.036;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 105, 110, 125,
           weapon_alpha(210.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 105, 110, 125,
           weapon_alpha(210.0 * event->fade));
  for (int i = 0; i < 8; i++) {
    double a0 = SCENE_PI + i * SCENE_PI / 8.0;
    double a1 = SCENE_PI + (i + 1) * SCENE_PI / 8.0;
    thickLineRGBA(surface, (int)(cx + cos(a0) * w), (int)(g - h + sin(a0) * w * 0.7),
                  (int)(cx + cos(a1) * w), (int)(g - h + sin(a1) * w * 0.7), 2, 235, 220, 165,
                  weapon_alpha(215.0 * event->fade));
  }
}


// Star pergola: hillside pergola draped with glowing cyan-gold starlight vines.
static void hill_star_pergola(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 101, 211));
  double g = hill_ground(event, cx);
  double w = s * 0.032;
  double h = s * 0.034;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 95, 78, 66,
           weapon_alpha(210.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 95, 78, 66,
           weapon_alpha(210.0 * event->fade));
  thickLineRGBA(surface, (int)(cx - w * 1.15), (int)(g - h), (int)(cx + w * 1.15), (int)(g - h), 2,
                115, 92, 74, weapon_alpha(220.0 * event->fade));
  for (int i = -2; i <= 2; i++) {
    double vx = cx + i * w * 0.4;
    double vy = g - h + 4 + (i & 1) * 4 + sin(event->t * 2.0 + i) * 1.5;
    filledCircleRGBA(surface, (int)vx, (int)vy, 2, 165, 235, 245,
                     weapon_alpha(210.0 * event->fade));
  }
}


// Moonlit gazebo: a domed hillside shelter with a warm hanging lantern.
static void hill_moonlit_gazebo(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 131, 241));
  double g = hill_ground(event, cx);
  double w = s * 0.026;
  double h = s * 0.028;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 110, 115, 130,
           weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 110, 115, 130,
           weapon_alpha(205.0 * event->fade));
  filledTrigonRGBA(surface, (int)(cx - w * 1.25), (int)(g - h), (int)(cx + w * 1.25),
                   (int)(g - h), (int)cx, (int)(g - h * 1.6), 95, 105, 135,
                   weapon_alpha(210.0 * event->fade));
  double sway = sin(event->t * 2.1) * 2.0;
  filledCircleRGBA(surface, (int)(cx + sway), (int)(g - h * 0.65), 3, 255, 220, 145,
                   weapon_alpha(220.0 * event->fade));
}


// Lantern belfry: a slender timber frame holding a swaying amber bell-lantern.
static void hill_lantern_belfry(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.18 + 0.64 * hill_random(event, 151, 263));
  double g = hill_ground(event, cx);
  double w = s * 0.015;
  double h = s * 0.044;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w * 0.7), (int)(g - h), 100, 80, 62,
           weapon_alpha(210.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w * 0.7), (int)(g - h), 100, 80, 62,
           weapon_alpha(210.0 * event->fade));
  thickLineRGBA(surface, (int)(cx - w * 0.9), (int)(g - h), (int)(cx + w * 0.9), (int)(g - h), 2,
                120, 96, 72, weapon_alpha(220.0 * event->fade));
  double sway = sin(event->t * 2.4) * 3.0;
  lineRGBA(surface, (int)cx, (int)(g - h), (int)(cx + sway), (int)(g - h * 0.62), 130, 120, 100,
           weapon_alpha(180.0 * event->fade));
  filledCircleRGBA(surface, (int)(cx + sway), (int)(g - h * 0.62), 3, 255, 195, 110,
                   weapon_alpha(225.0 * event->fade));
}


// Aurora cairn: stacked hillside stones topped with a pulsing emerald-cyan orb.
static void hill_aurora_cairn(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.16 + 0.68 * hill_random(event, 173, 281));
  double g = hill_ground(event, cx);
  double base_r = s * 0.011;
  if (base_r < 3.0) base_r = 3.0;
  for (int i = 0; i < 3; i++) {
    double r = base_r * (1.0 - 0.22 * i);
    double cy = g - r - i * base_r * 1.1;
    filledCircleRGBA(surface, (int)cx, (int)cy, (int)r, 95, 102, 114,
                     weapon_alpha(210.0 * event->fade));
  }
  double pulse = 0.75 + 0.25 * sin(event->t * 2.6);
  filledCircleRGBA(surface, (int)cx, (int)(g - base_r * 3.8), 3, 125, 245, 210,
                   weapon_alpha(220.0 * pulse * event->fade));
}


// Signal lantern post: a tall iron post with twin blinking harbor-style lamps.
static void hill_signal_lantern_post(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.18 + 0.64 * hill_random(event, 191, 307));
  double g = hill_ground(event, cx);
  double h = s * 0.048;
  thickLineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 2, 85, 92, 108,
                weapon_alpha(215.0 * event->fade));
  lineRGBA(surface, (int)(cx - s * 0.012), (int)(g - h * 0.85), (int)(cx + s * 0.012),
           (int)(g - h * 0.85), 95, 102, 118, weapon_alpha(205.0 * event->fade));
  bool left_on = ((int)(event->t * 2.0) % 2) == 0;
  filledCircleRGBA(surface, (int)(cx - s * 0.012), (int)(g - h * 0.85), 3, 255, 175, 95,
                   weapon_alpha((left_on ? 235.0 : 85.0) * event->fade));
  filledCircleRGBA(surface, (int)(cx + s * 0.012), (int)(g - h * 0.85), 3, 130, 230, 255,
                   weapon_alpha((left_on ? 85.0 : 235.0) * event->fade));
}


// Starry obelisk pair: two matching dark monoliths linked by a faint starlight arc.
static void hill_starry_obelisk_pair(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 211, 331));
  double span = s * 0.028;
  double h = s * 0.044;
  for (int side = -1; side <= 1; side += 2) {
    double ox = cx + side * span;
    double g = hill_ground(event, ox);
    filledTrigonRGBA(surface, (int)(ox - 4), (int)g, (int)(ox + 4), (int)g, (int)ox, (int)(g - h),
                     70, 76, 92, weapon_alpha(215.0 * event->fade));
    filledCircleRGBA(surface, (int)ox, (int)(g - h - 2), 2, 225, 235, 255,
                     weapon_alpha(210.0 * event->fade));
  }
  double g_mid = hill_ground(event, cx);
  lineRGBA(surface, (int)(cx - span), (int)(hill_ground(event, cx - span) - h - 2), (int)(cx + span),
           (int)(hill_ground(event, cx + span) - h - 2), 185, 215, 255,
           weapon_alpha((100.0 + 50.0 * sin(event->t * 2.2)) * event->fade));
  (void)g_mid;
}


// Glowing reed bed: swaying hillside reeds tipped with pale lime-gold buds.
static void hill_glowing_reed_bed(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.14 + 0.72 * hill_random(event, 223, 349));
  for (int i = -3; i <= 3; i++) {
    double rx = cx + i * s * 0.008;
    double g = hill_ground(event, rx);
    double sway = sin(event->t * 2.2 + i * 0.7) * 4.0;
    double h = s * (0.022 + 0.004 * (3 - abs(i)));
    lineRGBA(surface, (int)rx, (int)g, (int)(rx + sway), (int)(g - h), 85, 118, 92,
             weapon_alpha(200.0 * event->fade));
    filledCircleRGBA(surface, (int)(rx + sway), (int)(g - h), 2, 210, 245, 145,
                     weapon_alpha(215.0 * event->fade));
  }
}


// Ember shrine: a small stepped stone altar holding a warm flickering flame.
static void hill_ember_shrine(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 239, 367));
  double g = hill_ground(event, cx);
  double w = s * 0.018;
  double h = s * 0.022;
  boxRGBA(surface, (int)(cx - w), (int)(g - h * 0.4), (int)(cx + w), (int)g, 92, 88, 96,
          weapon_alpha(215.0 * event->fade));
  boxRGBA(surface, (int)(cx - w * 0.65), (int)(g - h * 0.8), (int)(cx + w * 0.65),
          (int)(g - h * 0.4), 110, 104, 114, weapon_alpha(215.0 * event->fade));
  double flicker = sin(event->t * 7.0) * 1.5;
  filledCircleRGBA(surface, (int)(cx + flicker * 0.5), (int)(g - h - 3), 3, 255, 165, 85,
                   weapon_alpha(230.0 * event->fade));
}


// Wind chime arch: a curved hillside frame hung with swaying silver chimes.
static void hill_wind_chime_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 251, 389));
  double g = hill_ground(event, cx);
  double w = s * 0.025;
  double h = s * 0.036;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 108, 114, 128,
           weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 108, 114, 128,
           weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx - w), (int)(g - h), (int)(cx + w), (int)(g - h), 118, 124, 138,
           weapon_alpha(210.0 * event->fade));
  for (int i = -2; i <= 2; i++) {
    double bx = cx + i * w * 0.34;
    double sway = sin(event->t * 2.8 + i * 0.8) * 3.0;
    lineRGBA(surface, (int)bx, (int)(g - h), (int)(bx + sway), (int)(g - h * 0.55), 210, 225, 245,
             weapon_alpha(210.0 * event->fade));
  }
}


// Lunar sundial: a stone pedestal with a glowing silver gnomon sweeping slowly.
static void hill_lunar_sundial(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 271, 409));
  double g = hill_ground(event, cx);
  double h = s * 0.024;
  boxRGBA(surface, (int)(cx - 6), (int)(g - h), (int)(cx + 6), (int)g, 96, 102, 114,
          weapon_alpha(210.0 * event->fade));
  double tip_x = cx + sin(event->t * 0.8) * s * 0.014;
  double tip_y = g - h - s * 0.016;
  thickLineRGBA(surface, (int)cx, (int)(g - h), (int)tip_x, (int)tip_y, 2, 215, 225, 245,
                weapon_alpha(215.0 * event->fade));
}


// Beacon tripod: three iron legs supporting a pulsing warm-amber lantern.
static void hill_beacon_tripod(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.18 + 0.64 * hill_random(event, 283, 421));
  double g = hill_ground(event, cx);
  double w = s * 0.016;
  double h = s * 0.032;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)cx, (int)(g - h), 88, 94, 106,
           weapon_alpha(210.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)cx, (int)(g - h), 88, 94, 106,
           weapon_alpha(210.0 * event->fade));
  lineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 88, 94, 106,
           weapon_alpha(210.0 * event->fade));
  double pulse = 0.7 + 0.3 * sin(event->t * 3.0);
  filledCircleRGBA(surface, (int)cx, (int)(g - h - 2), 3, 255, 200, 120,
                   weapon_alpha(230.0 * pulse * event->fade));
}


// Tidepool glow: a shallow hillside basin with bioluminescent cyan ripples.
static void hill_tidepool_glow(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 293, 433));
  double g = hill_ground(event, cx);
  double w = s * 0.032;
  filledEllipseRGBA(surface, (int)cx, (int)g, (int)w, 4, 48, 92, 118,
                    weapon_alpha(195.0 * event->fade));
  double r = (0.4 + 0.6 * fmod(event->t * 0.6, 1.0)) * w * 0.85;
  ellipseRGBA(surface, (int)cx, (int)g, (int)r, 2, 130, 240, 255,
              weapon_alpha(190.0 * event->fade));
}


// Starry pavilion two: a peaked wooden roof over two glowing porch lanterns.
static void hill_starry_pavilion_two(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.22 + 0.56 * hill_random(event, 307, 449));
  double g = hill_ground(event, cx);
  double w = s * 0.028;
  double h = s * 0.026;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 102, 86, 72,
           weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 102, 86, 72,
           weapon_alpha(205.0 * event->fade));
  filledTrigonRGBA(surface, (int)(cx - w * 1.2), (int)(g - h), (int)(cx + w * 1.2), (int)(g - h),
                   (int)cx, (int)(g - h * 1.65), 88, 96, 118, weapon_alpha(210.0 * event->fade));
  filledCircleRGBA(surface, (int)(cx - w * 0.65), (int)(g - h * 0.6), 2, 255, 215, 135,
                   weapon_alpha(220.0 * event->fade));
  filledCircleRGBA(surface, (int)(cx + w * 0.65), (int)(g - h * 0.6), 2, 255, 215, 135,
                   weapon_alpha(220.0 * event->fade));
}


// Crystal lantern: a faceted blue-violet crystal perched on a stone plinth.
static void hill_crystal_lantern(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 313, 461));
  double g = hill_ground(event, cx);
  double h = s * 0.028;
  boxRGBA(surface, (int)(cx - 5), (int)(g - h * 0.35), (int)(cx + 5), (int)g, 92, 98, 112,
          weapon_alpha(210.0 * event->fade));
  filledTrigonRGBA(surface, (int)(cx - 5), (int)(g - h * 0.35), (int)(cx + 5),
                   (int)(g - h * 0.35), (int)cx, (int)(g - h), 165, 200, 255,
                   weapon_alpha((185.0 + 45.0 * sin(event->t * 2.5)) * event->fade));
}


// Lantern boat two: a small crescent hull gliding along a misty hill crest.
static void hill_lantern_boat_two(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * event->progress);
  double g = hill_ground(event, cx) - 3.0;
  double w = s * 0.020;
  lineRGBA(surface, (int)(cx - w), (int)(g - 4), (int)(cx + w), (int)(g - 4), 130, 110, 92,
           weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx - w * 0.7), (int)g, (int)(cx + w * 0.7), (int)g, 130, 110, 92,
           weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)cx, (int)(g - 4), (int)cx, (int)(g - s * 0.022), 140, 125, 105,
           weapon_alpha(195.0 * event->fade));
  filledCircleRGBA(surface, (int)cx, (int)(g - s * 0.022), 2, 255, 205, 125,
                   weapon_alpha(225.0 * event->fade));
}


// Beacon windmill two: a hillside windmill with a glowing cyan hub and spinning sails.
static void hill_beacon_windmill_two(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 331, 479));
  double g = hill_ground(event, cx);
  double h = s * 0.042;
  double r = s * 0.018;
  filledTrigonRGBA(surface, (int)(cx - 5), (int)g, (int)(cx + 5), (int)g, (int)cx, (int)(g - h),
                   82, 88, 102, weapon_alpha(210.0 * event->fade));
  for (int i = 0; i < 4; i++) {
    double a = event->t * 1.8 + i * SCENE_PI * 0.5;
    lineRGBA(surface, (int)cx, (int)(g - h), (int)(cx + cos(a) * r), (int)(g - h + sin(a) * r),
             195, 210, 230, weapon_alpha(205.0 * event->fade));
  }
  filledCircleRGBA(surface, (int)cx, (int)(g - h), 2, 130, 235, 255,
                   weapon_alpha(230.0 * event->fade));
}


// Dew lantern row: four low curved stems holding glowing pale-aqua dew drops.
static void hill_dew_lantern_row(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.18 + 0.64 * hill_random(event, 347, 491));
  for (int i = 0; i < 4; i++) {
    double px = cx + (i - 1.5) * s * 0.015;
    double g = hill_ground(event, px);
    double h = s * 0.020;
    double tip_x = px + 3.0 + sin(event->t * 2.0 + i) * 1.5;
    lineRGBA(surface, (int)px, (int)g, (int)tip_x, (int)(g - h), 88, 122, 102,
             weapon_alpha(200.0 * event->fade));
    filledCircleRGBA(surface, (int)tip_x, (int)(g - h + 2), 2, 155, 245, 230,
                     weapon_alpha(220.0 * event->fade));
  }
}


// Firefly gate: two wooden posts with drifting yellow-green fireflies between them.
static void hill_firefly_gate(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 353, 503));
  double g = hill_ground(event, cx);
  double w = s * 0.022;
  double h = s * 0.032;
  lineRGBA(surface, (int)(cx - w), (int)g, (int)(cx - w), (int)(g - h), 96, 82, 68,
           weapon_alpha(205.0 * event->fade));
  lineRGBA(surface, (int)(cx + w), (int)g, (int)(cx + w), (int)(g - h), 96, 82, 68,
           weapon_alpha(205.0 * event->fade));
  for (int i = 0; i < 4; i++) {
    double fx = cx + sin(event->t * 1.7 + i * 1.5) * w * 0.8;
    double fy = g - h * (0.3 + 0.15 * i) + cos(event->t * 2.3 + i) * 3.0;
    filledCircleRGBA(surface, (int)fx, (int)fy, 2, 215, 250, 125,
                     weapon_alpha(215.0 * event->fade));
  }
}


// Observatory spire: a slender dome-topped tower with a scanning starlight beam.
static void hill_observatory_spire(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 367, 521));
  double g = hill_ground(event, cx);
  double h = s * 0.044;
  boxRGBA(surface, (int)(cx - 4), (int)(g - h), (int)(cx + 4), (int)g, 86, 92, 108,
          weapon_alpha(210.0 * event->fade));
  filledCircleRGBA(surface, (int)cx, (int)(g - h), 5, 105, 115, 138,
                   weapon_alpha(210.0 * event->fade));
  double beam_dx = sin(event->t * 1.1) * s * 0.045;
  lineRGBA(surface, (int)cx, (int)(g - h - 2), (int)(cx + beam_dx), (int)(g - h - s * 0.04), 195,
           230, 255, weapon_alpha(150.0 * event->fade));
}


// Ember brazier row: three iron tripods holding flickering orange coals.
static void hill_ember_brazier_row(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 379, 541));
  for (int i = -1; i <= 1; i++) {
    double bx = cx + i * s * 0.025;
    double g = hill_ground(event, bx);
    double h = s * 0.016;
    lineRGBA(surface, (int)(bx - 3), (int)g, (int)bx, (int)(g - h), 85, 80, 82,
             weapon_alpha(205.0 * event->fade));
    lineRGBA(surface, (int)(bx + 3), (int)g, (int)bx, (int)(g - h), 85, 80, 82,
             weapon_alpha(205.0 * event->fade));
    double flick = sin(event->t * 6.5 + i * 2.0) * 1.2;
    filledCircleRGBA(surface, (int)(bx + flick * 0.5), (int)(g - h - 2), 2, 255, 165, 80,
                     weapon_alpha(225.0 * event->fade));
  }
}


// Frost lantern: a pale-blue crystalline lamp hanging from a curved iron crook.
static void hill_frost_lantern(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 397, 563));
  double g = hill_ground(event, cx);
  double h = s * 0.042;
  lineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 96, 106, 122,
           weapon_alpha(210.0 * event->fade));
  lineRGBA(surface, (int)cx, (int)(g - h), (int)(cx + s * 0.012), (int)(g - h + 3), 96, 106, 122,
           weapon_alpha(210.0 * event->fade));
  double sway = sin(event->t * 2.3) * 2.0;
  filledCircleRGBA(surface, (int)(cx + s * 0.012 + sway), (int)(g - h + 7), 3, 175, 230, 255,
                   weapon_alpha(225.0 * event->fade));
}


// Reed pergola: a woven arch of slender reeds with glowing emerald firefly buds.
static void hill_reed_pergola(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 409, 577));
  double g = hill_ground(event, cx);
  double w = s * 0.028;
  double h = s * 0.030;
  for (int i = 0; i < 6; i++) {
    double a0 = SCENE_PI + i * SCENE_PI / 6.0;
    double a1 = SCENE_PI + (i + 1) * SCENE_PI / 6.0;
    lineRGBA(surface, (int)(cx + cos(a0) * w), (int)(g + sin(a0) * h),
             (int)(cx + cos(a1) * w), (int)(g + sin(a1) * h), 96, 128, 92,
             weapon_alpha(205.0 * event->fade));
  }
  for (int i = -1; i <= 1; i++) {
    double px = cx + i * w * 0.45;
    double py = g - h * 0.75 + sin(event->t * 2.5 + i) * 2.0;
    filledCircleRGBA(surface, (int)px, (int)py, 2, 165, 250, 175,
                     weapon_alpha(220.0 * event->fade));
  }
}


// Lantern cairn row: three low stone mounds each crowned with a warm paper lamp.
static void hill_lantern_cairn_row(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 421, 593));
  for (int i = -1; i <= 1; i++) {
    double bx = cx + i * s * 0.026;
    double g = hill_ground(event, bx);
    filledTrigonRGBA(surface, (int)(bx - 6), (int)g, (int)(bx + 6), (int)g, (int)bx,
                     (int)(g - s * 0.015), 90, 96, 108, weapon_alpha(205.0 * event->fade));
    filledCircleRGBA(surface, (int)bx, (int)(g - s * 0.018), 2, 255, 205, 130,
                     weapon_alpha(225.0 * event->fade));
  }
}


// Skywatch mast: a tall hillside mast with a rotating anemometer and beacon tip.
static void hill_skywatch_mast(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 433, 607));
  double g = hill_ground(event, cx);
  double h = s * 0.048;
  lineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 108, 114, 128,
           weapon_alpha(210.0 * event->fade));
  double arm = cos(event->t * 3.0) * s * 0.012;
  lineRGBA(surface, (int)(cx - arm), (int)(g - h * 0.7), (int)(cx + arm), (int)(g - h * 0.7), 165,
           180, 200, weapon_alpha(200.0 * event->fade));
  filledCircleRGBA(surface, (int)cx, (int)(g - h), 2, 255, 190, 120,
                   weapon_alpha(225.0 * event->fade));
}


// Moonstone arch: a pale stone ring-gate glowing with cool silver-blue light.
static void hill_moonstone_arch(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 443, 619));
  double g = hill_ground(event, cx);
  double r = s * 0.022;
  for (int i = 0; i < 8; i++) {
    double a0 = SCENE_PI + i * SCENE_PI / 8.0;
    double a1 = SCENE_PI + (i + 1) * SCENE_PI / 8.0;
    thickLineRGBA(surface, (int)(cx + cos(a0) * r), (int)(g + sin(a0) * r),
                  (int)(cx + cos(a1) * r), (int)(g + sin(a1) * r), 2, 180, 210, 245,
                  weapon_alpha(205.0 * event->fade));
  }
  filledCircleRGBA(surface, (int)cx, (int)(g - r), 2, 225, 245, 255,
                   weapon_alpha(225.0 * event->fade));
}


// Tidebell post: a coastal timber post with a swaying brass bell.
static void hill_tidebell_post(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 457, 631));
  double g = hill_ground(event, cx);
  double h = s * 0.040;
  lineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 102, 84, 66,
           weapon_alpha(210.0 * event->fade));
  lineRGBA(surface, (int)cx, (int)(g - h), (int)(cx + s * 0.012), (int)(g - h), 102, 84, 66,
           weapon_alpha(210.0 * event->fade));
  double sway = sin(event->t * 2.7) * 2.5;
  filledTrigonRGBA(surface, (int)(cx + s * 0.012 + sway - 3), (int)(g - h + 7),
                   (int)(cx + s * 0.012 + sway + 3), (int)(g - h + 7),
                   (int)(cx + s * 0.012 + sway * 0.4), (int)(g - h + 1), 225, 195, 115,
                   weapon_alpha(220.0 * event->fade));
}


// Starry windmill three: a slender mill with glowing gold-tipped blades.
static void hill_starry_windmill_three(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 467, 643));
  double g = hill_ground(event, cx);
  double h = s * 0.044;
  double r = s * 0.019;
  lineRGBA(surface, (int)cx, (int)g, (int)cx, (int)(g - h), 102, 108, 124,
           weapon_alpha(210.0 * event->fade));
  for (int i = 0; i < 3; i++) {
    double a = event->t * 1.9 + i * SCENE_TAU / 3.0;
    double bx = cx + cos(a) * r;
    double by = g - h + sin(a) * r;
    lineRGBA(surface, (int)cx, (int)(g - h), (int)bx, (int)by, 185, 195, 215,
             weapon_alpha(200.0 * event->fade));
    filledCircleRGBA(surface, (int)bx, (int)by, 2, 255, 225, 145,
                     weapon_alpha(220.0 * event->fade));
  }
}


// Glowing fern: unfurling emerald fronds tipped with tiny cyan spores.
static void hill_glowing_fern(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.18 + 0.64 * hill_random(event, 479, 659));
  double g = hill_ground(event, cx);
  for (int i = -1; i <= 1; i++) {
    double h = s * (0.028 - abs(i) * 0.005);
    double tip_x = cx + i * s * 0.014 + sin(event->t * 2.1 + i) * 2.0;
    lineRGBA(surface, (int)cx, (int)g, (int)tip_x, (int)(g - h), 78, 138, 102,
             weapon_alpha(210.0 * event->fade));
    filledCircleRGBA(surface, (int)tip_x, (int)(g - h), 2, 155, 250, 220,
                     weapon_alpha(220.0 * event->fade));
  }
}


// Beacon well: a low stone well emitting a gentle upward column of cyan motes.
static void hill_beacon_well(const HillEvent *event) {
  RenderTarget *surface = event->surface;
  double s = event->scale;
  double cx = surface->w * (0.20 + 0.60 * hill_random(event, 491, 673));
  double g = hill_ground(event, cx);
  boxRGBA(surface, (int)(cx - 7), (int)(g - 7), (int)(cx + 7), (int)g, 92, 98, 112,
          weapon_alpha(210.0 * event->fade));
  for (int i = 0; i < 3; i++) {
    double my = g - 10 - fmod(event->t * 14.0 + i * 8.0, s * 0.035);
    filledCircleRGBA(surface, (int)(cx + sin(event->t * 2.0 + i) * 2.5), (int)my, 2, 145, 235,
                     255, weapon_alpha(205.0 * event->fade));
  }
}


// NEW_HILL_EVENTS_ABOVE (new hill_xxx() functions go above this line)

void (*const hill_event_kinds[])(const HillEvent *) = {
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
    hill_signal_tower,
    hill_moon_garden,
    hill_windmill_row,
    hill_shrine_torii,
    hill_starry_tent,
    hill_ice_spire,
    hill_lantern_procession,
    hill_observatory_dome,
    hill_weather_vane,
    hill_geyser_vent,
    hill_bonsai_pine,
    hill_brazier_tripod,
    hill_flagpole,
    hill_glowworm_arch,
    hill_haystacks,
    hill_standing_stones,
    hill_kite_festival,
    hill_firepot_row,
    hill_clockwork_bird,
    hill_mirage_pool,
    hill_signal_mirror,
    hill_cairn_lantern,
    hill_reed_boat,
    hill_wind_chimes_row,
    hill_sundial_plinth,
    hill_star_map_table,
    hill_lantern_gate,
    hill_dew_web,
    hill_ember_tree,
    hill_ice_lantern,
    hill_bell_buoy,
    hill_frost_ferns,
    hill_paper_lanterns,
    hill_crystal_cluster,
    hill_smoke_signals,
    hill_shepherd_hut,
    hill_moonstone_ring,
    hill_weather_balloon,
    hill_water_wheel,
    hill_starry_campfire,
    hill_beacon_arch,
    hill_night_heron,
    hill_firefly_bush,
    hill_tethered_kite,
    hill_stone_well,
    hill_aurora_spire,
    hill_lantern_boat,
    hill_camp_lantern,
    hill_wind_sock,
    hill_moon_gate,
    hill_starry_trellis,
    hill_radio_beacon,
    hill_owl_perch,
    hill_snowdrift_tree,
    hill_glow_mushrooms,
    hill_lantern_bridge_two,
    hill_starry_obelisk,
    hill_crane_roost,
    hill_wind_harp_two,
    hill_beacon_dome,
    hill_fire_wheel_two,
    hill_dew_flowers,
    hill_star_lantern,
    hill_frost_chimes,
    hill_ember_braziers,
    hill_stone_arch,
    hill_lantern_post_row,
    hill_reed_lanterns,
    hill_star_sundial,
    hill_windmill_beacon,
    hill_ice_crystals,
    hill_lantern_arch_two,
    hill_fire_totem,
    hill_star_flowerbed,
    hill_clockwork_owl,
    hill_ether_windmill,
    hill_crystal_obelisk,
    hill_sky_mirror,
    hill_bell_post,
    hill_fire_pit,
    hill_moon_pond,
    hill_lantern_fence,
    hill_star_telescope,
    hill_glowing_moss,
    hill_frost_arch,
    hill_torch_circle,
    hill_reed_wind,
    hill_beacon_cairn,
    hill_star_pavilion,
    hill_star_arch,
    hill_star_pergola,
    hill_moonlit_gazebo,
    hill_lantern_belfry,
    hill_aurora_cairn,
    hill_signal_lantern_post,
    hill_starry_obelisk_pair,
    hill_glowing_reed_bed,
    hill_ember_shrine,
    hill_wind_chime_arch,
    hill_lunar_sundial,
    hill_beacon_tripod,
    hill_tidepool_glow,
    hill_starry_pavilion_two,
    hill_crystal_lantern,
    hill_lantern_boat_two,
    hill_beacon_windmill_two,
    hill_dew_lantern_row,
    hill_firefly_gate,
    hill_observatory_spire,
    hill_ember_brazier_row,
    hill_frost_lantern,
    hill_reed_pergola,
    hill_lantern_cairn_row,
    hill_skywatch_mast,
    hill_moonstone_arch,
    hill_tidebell_post,
    hill_starry_windmill_three,
    hill_glowing_fern,
    hill_beacon_well,
    // NEW_HILL_KINDS_ABOVE
};
static_assert(sizeof(hill_event_kinds) / sizeof(hill_event_kinds[0]) == HILL_EVENT_KINDS,
              "HILL_EVENT_KINDS does not match hill_event_kinds[]");

void render_hill_event(RenderTarget *surface, Uint32 ticks, int scale) {
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
