#ifndef GLUQLO_GFX_PRIMITIVES_H
#define GLUQLO_GFX_PRIMITIVES_H

#include "gluqlo.h"

void pixelRGBA(RenderTarget *dst, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void lineRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b,
              Uint8 a);
void vlineRGBA(RenderTarget *dst, int x, int y1, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void rectangleRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b,
                   Uint8 a);
void thickLineRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, int width_px, Uint8 r,
                   Uint8 g, Uint8 b, Uint8 a);
void boxRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void filledTrigonRGBA(RenderTarget *dst, int x1, int y1, int x2, int y2, int x3, int y3, Uint8 r,
                      Uint8 g, Uint8 b, Uint8 a);
void filledEllipseRGBA(RenderTarget *dst, int cx, int cy, int rx, int ry, Uint8 r, Uint8 g, Uint8 b,
                       Uint8 a);
void ellipseRGBA(RenderTarget *dst, int cx, int cy, int rx, int ry, Uint8 r, Uint8 g, Uint8 b,
                 Uint8 a);
void filledCircleRGBA(RenderTarget *dst, int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b,
                      Uint8 a);
void circleRGBA(RenderTarget *dst, int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void filledPieRGBA(RenderTarget *dst, int cx, int cy, int radius, int start_deg, int end_deg,
                   Uint8 r, Uint8 g, Uint8 b, Uint8 a);

#endif
