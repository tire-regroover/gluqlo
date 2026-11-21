#include "gfx_primitives.h"

// --- SDL3 GPU-accelerated primitive helpers (replacing SDL_gfx) ---

static inline SDL_FColor fcolor_rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  return SDL_FColor{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}


static inline void set_draw_color(RenderTarget *dst, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  SDL_SetRenderDrawColor(dst->renderer, r, g, b, a);
  SDL_SetRenderDrawBlendMode(dst->renderer, a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
}


void pixelRGBA(RenderTarget *dst, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  if (a == 0) return;
  set_draw_color(dst, r, g, b, a);
  SDL_RenderPoint(dst->renderer, (float)x, (float)y);
}


void lineRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b,
              Uint8 a) {
  if (a == 0) return;
  set_draw_color(dst, r, g, b, a);
  SDL_RenderLine(dst->renderer, (float)x1, (float)y1, (float)x2, (float)y2);
}


void vlineRGBA(RenderTarget *dst, int x, int y1, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  lineRGBA(dst, x, y1, x, y2, r, g, b, a);
}


void rectangleRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b,
                   Uint8 a) {
  if (a == 0) return;
  if (x1 > x2) {
    int t = x1;
    x1 = x2;
    x2 = t;
  }
  if (y1 > y2) {
    int t = y1;
    y1 = y2;
    y2 = t;
  }
  SDL_FRect rect = {(float)x1, (float)y1, (float)(x2 - x1 + 1), (float)(y2 - y1 + 1)};
  set_draw_color(dst, r, g, b, a);
  SDL_RenderRect(dst->renderer, &rect);
}


void thickLineRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, int width_px, Uint8 r,
                   Uint8 g, Uint8 b, Uint8 a) {
  if (a == 0 || width_px <= 0) return;
  if (width_px == 1) {
    lineRGBA(dst, x1, y1, x2, y2, r, g, b, a);
    return;
  }
  float dx = (float)(x2 - x1);
  float dy = (float)(y2 - y1);
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 0.001f) {
    SDL_FRect rect = {(float)x1 - width_px * 0.5f, (float)y1 - width_px * 0.5f, (float)width_px,
                      (float)width_px};
    set_draw_color(dst, r, g, b, a);
    SDL_RenderFillRect(dst->renderer, &rect);
    return;
  }
  float half = width_px * 0.5f;
  float nx = -dy / len * half;
  float ny = dx / len * half;
  SDL_FColor col = fcolor_rgba(r, g, b, a);
  SDL_Vertex verts[4];
  verts[0].position = SDL_FPoint{x1 + nx, y1 + ny};
  verts[1].position = SDL_FPoint{x1 - nx, y1 - ny};
  verts[2].position = SDL_FPoint{x2 - nx, y2 - ny};
  verts[3].position = SDL_FPoint{x2 + nx, y2 + ny};
  for (int i = 0; i < 4; i++) {
    verts[i].color = col;
    verts[i].tex_coord = SDL_FPoint{0, 0};
  }
  static const int idx[6] = {0, 1, 2, 0, 2, 3};
  SDL_SetRenderDrawBlendMode(dst->renderer, a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
  SDL_RenderGeometry(dst->renderer, NULL, verts, 4, idx, 6);
}


void boxRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b,
             Uint8 a) {
  if (a == 0) return;
  if (x1 > x2) {
    int t = x1;
    x1 = x2;
    x2 = t;
  }
  if (y1 > y2) {
    int t = y1;
    y1 = y2;
    y2 = t;
  }
  SDL_FRect rect = {(float)x1, (float)y1, (float)(x2 - x1 + 1), (float)(y2 - y1 + 1)};
  set_draw_color(dst, r, g, b, a);
  SDL_RenderFillRect(dst->renderer, &rect);
}


void filledTrigonRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, int x3, int y3, Uint8 r,
                      Uint8 g, Uint8 b, Uint8 a) {
  if (a == 0) return;
  SDL_FColor col = fcolor_rgba(r, g, b, a);
  SDL_Vertex verts[3];
  verts[0].position = SDL_FPoint{(float)x1, (float)y1};
  verts[1].position = SDL_FPoint{(float)x2, (float)y2};
  verts[2].position = SDL_FPoint{(float)x3, (float)y3};
  for (int i = 0; i < 3; i++) {
    verts[i].color = col;
    verts[i].tex_coord = SDL_FPoint{0, 0};
  }
  SDL_SetRenderDrawBlendMode(dst->renderer, a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
  SDL_RenderGeometry(dst->renderer, NULL, verts, 3, NULL, 0);
}


static inline int circle_segments(int rx, int ry) {
  int r = rx > ry ? rx : ry;
  if (r <= 2) return 8;
  if (r <= 8) return 16;
  if (r <= 24) return 28;
  if (r <= 64) return 44;
  return 64;
}


void filledEllipseRGBA(RenderTarget *dst, int cx, int cy, int rx, int ry, Uint8 r, Uint8 g, Uint8 b,
                       Uint8 a) {
  if (a == 0 || rx < 0 || ry < 0) return;
  if (rx == 0 && ry == 0) {
    pixelRGBA(dst, cx, cy, r, g, b, a);
    return;
  }
  if (rx == 0) {
    lineRGBA(dst, cx, cy - ry, cx, cy + ry, r, g, b, a);
    return;
  }
  if (ry == 0) {
    lineRGBA(dst, cx - rx, cy, cx + rx, cy, r, g, b, a);
    return;
  }
  int segs = circle_segments(rx, ry);
  SDL_Vertex verts[66];
  int indices[64 * 3];
  SDL_FColor col = fcolor_rgba(r, g, b, a);
  verts[0].position = SDL_FPoint{(float)cx + 0.5f, (float)cy + 0.5f};
  verts[0].color = col;
  verts[0].tex_coord = SDL_FPoint{0, 0};
  for (int i = 0; i < segs; i++) {
    float ang = (float)(2.0 * M_PI * i / segs);
    verts[i + 1].position = SDL_FPoint{cx + 0.5f + cosf(ang) * rx, cy + 0.5f + sinf(ang) * ry};
    verts[i + 1].color = col;
    verts[i + 1].tex_coord = SDL_FPoint{0, 0};
    indices[i * 3 + 0] = 0;
    indices[i * 3 + 1] = i + 1;
    indices[i * 3 + 2] = (i + 1 < segs) ? i + 2 : 1;
  }
  SDL_SetRenderDrawBlendMode(dst->renderer, a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
  SDL_RenderGeometry(dst->renderer, NULL, verts, segs + 1, indices, segs * 3);
}


void ellipseRGBA(RenderTarget *dst, int cx, int cy, int rx, int ry, Uint8 r, Uint8 g, Uint8 b,
                 Uint8 a) {
  if (a == 0 || rx < 0 || ry < 0) return;
  int segs = circle_segments(rx, ry);
  SDL_FPoint pts[65];
  for (int i = 0; i <= segs; i++) {
    float ang = (float)(2.0 * M_PI * (i % segs) / segs);
    pts[i].x = cx + 0.5f + cosf(ang) * rx;
    pts[i].y = cy + 0.5f + sinf(ang) * ry;
  }
  set_draw_color(dst, r, g, b, a);
  SDL_RenderLines(dst->renderer, pts, segs + 1);
}


void filledCircleRGBA(RenderTarget *dst, int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b,
                      Uint8 a) {
  filledEllipseRGBA(dst, cx, cy, radius, radius, r, g, b, a);
}


void circleRGBA(RenderTarget *dst, int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  ellipseRGBA(dst, cx, cy, radius, radius, r, g, b, a);
}


void filledPieRGBA(RenderTarget *dst, int cx, int cy, int radius, int start_deg, int end_deg,
                   Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  if (a == 0 || radius <= 0) return;
  int span = end_deg - start_deg;
  while (span < 0) span += 360;
  if (span == 0) return;
  if (span >= 360) {
    filledCircleRGBA(dst, cx, cy, radius, r, g, b, a);
    return;
  }
  int segs = (int)(circle_segments(radius, radius) * (span / 360.0)) + 2;
  if (segs < 3) segs = 3;
  if (segs > 64) segs = 64;
  SDL_Vertex verts[66];
  int indices[64 * 3];
  SDL_FColor col = fcolor_rgba(r, g, b, a);
  verts[0].position = SDL_FPoint{(float)cx + 0.5f, (float)cy + 0.5f};
  verts[0].color = col;
  verts[0].tex_coord = SDL_FPoint{0, 0};
  for (int i = 0; i <= segs; i++) {
    float deg = start_deg + (float)span * i / segs;
    float rad = deg * (float)(M_PI / 180.0);
    verts[i + 1].position =
        SDL_FPoint{cx + 0.5f + cosf(rad) * radius, cy + 0.5f + sinf(rad) * radius};
    verts[i + 1].color = col;
    verts[i + 1].tex_coord = SDL_FPoint{0, 0};
    if (i < segs) {
      indices[i * 3 + 0] = 0;
      indices[i * 3 + 1] = i + 1;
      indices[i * 3 + 2] = i + 2;
    }
  }
  SDL_SetRenderDrawBlendMode(dst->renderer, a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
  SDL_RenderGeometry(dst->renderer, NULL, verts, segs + 2, indices, segs * 3);
}


// draw rounded box onto an SDL_Surface (used once at startup for the card
