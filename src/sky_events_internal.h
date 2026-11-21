#ifndef GLUQLO_SKY_EVENTS_INTERNAL_H
#define GLUQLO_SKY_EVENTS_INTERNAL_H

#include "sky_events.h"
#include "gfx_primitives.h"
#include "scene_core.h"

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

void sky_event_23(const SkyEvent *event);
void sky_event_24(const SkyEvent *event);
void sky_event_25(const SkyEvent *event);
void sky_event_26(const SkyEvent *event);
void sky_event_27(const SkyEvent *event);
void sky_event_28(const SkyEvent *event);
void sky_event_29(const SkyEvent *event);
void sky_event_30(const SkyEvent *event);
void sky_event_31(const SkyEvent *event);
void sky_event_32(const SkyEvent *event);
void sky_event_33(const SkyEvent *event);
void sky_event_34(const SkyEvent *event);
void sky_event_35(const SkyEvent *event);
void sky_event_36(const SkyEvent *event);
void sky_event_37(const SkyEvent *event);
void sky_event_38(const SkyEvent *event);
void sky_event_39(const SkyEvent *event);
void sky_event_40(const SkyEvent *event);
void sky_event_41(const SkyEvent *event);
void sky_event_42(const SkyEvent *event);
void sky_event_43(const SkyEvent *event);
void sky_event_44(const SkyEvent *event);
void sky_event_45(const SkyEvent *event);
void sky_event_46(const SkyEvent *event);
void sky_event_47(const SkyEvent *event);
void sky_event_48(const SkyEvent *event);
void sky_event_49(const SkyEvent *event);
void sky_event_50(const SkyEvent *event);
void sky_event_51(const SkyEvent *event);
void sky_event_52(const SkyEvent *event);
void sky_event_53(const SkyEvent *event);
void sky_event_54(const SkyEvent *event);
void sky_event_55(const SkyEvent *event);
void sky_event_56(const SkyEvent *event);
void sky_event_57(const SkyEvent *event);
void sky_event_58(const SkyEvent *event);
void sky_event_59(const SkyEvent *event);
void sky_event_60(const SkyEvent *event);
void sky_event_61(const SkyEvent *event);
void sky_event_62(const SkyEvent *event);
void sky_event_63(const SkyEvent *event);
void sky_event_64(const SkyEvent *event);
void sky_event_65(const SkyEvent *event);
void sky_event_66(const SkyEvent *event);
void sky_event_67(const SkyEvent *event);
void sky_event_68(const SkyEvent *event);
void sky_event_69(const SkyEvent *event);
void sky_event_70(const SkyEvent *event);
void sky_event_71(const SkyEvent *event);
void sky_event_72(const SkyEvent *event);
void sky_event_73(const SkyEvent *event);
void sky_event_74(const SkyEvent *event);
void sky_event_75(const SkyEvent *event);
void sky_event_76(const SkyEvent *event);
void sky_event_77(const SkyEvent *event);
void sky_event_78(const SkyEvent *event);
void sky_event_79(const SkyEvent *event);
void sky_event_80(const SkyEvent *event);
void sky_event_81(const SkyEvent *event);
void sky_event_82(const SkyEvent *event);
void sky_event_83(const SkyEvent *event);
void sky_event_84(const SkyEvent *event);
void sky_event_85(const SkyEvent *event);
void sky_event_86(const SkyEvent *event);
void sky_event_87(const SkyEvent *event);
void sky_event_88(const SkyEvent *event);
void sky_event_89(const SkyEvent *event);
void sky_event_90(const SkyEvent *event);
void sky_event_91(const SkyEvent *event);
void sky_event_92(const SkyEvent *event);
void sky_event_93(const SkyEvent *event);
void sky_event_94(const SkyEvent *event);
void sky_event_95(const SkyEvent *event);
void sky_event_96(const SkyEvent *event);
void sky_event_97(const SkyEvent *event);
void sky_event_98(const SkyEvent *event);
void sky_event_99(const SkyEvent *event);
void sky_event_100(const SkyEvent *event);
void sky_event_101(const SkyEvent *event);
void sky_event_102(const SkyEvent *event);
void sky_event_103(const SkyEvent *event);
void sky_event_104(const SkyEvent *event);
void sky_event_105(const SkyEvent *event);
void sky_event_106(const SkyEvent *event);
void sky_event_107(const SkyEvent *event);
void sky_event_108(const SkyEvent *event);
void sky_event_109(const SkyEvent *event);
void sky_event_110(const SkyEvent *event);
void sky_event_111(const SkyEvent *event);
void sky_event_112(const SkyEvent *event);
void sky_event_113(const SkyEvent *event);
void sky_event_114(const SkyEvent *event);

#endif
