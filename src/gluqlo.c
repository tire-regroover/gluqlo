#include "gluqlo.h"
#include "gfx_primitives.h"
#include "scene_core.h"
#include "ships_battles.h"
#include "hill_events.h"
#include "sky_events.h"
#include "backdrop.h"
#include "clock_render.h"

#include <errno.h>
#include <time.h>

const char *TITLE = "Gluqlo L-Space";
const int DEFAULT_WIDTH = 1024;
const int DEFAULT_HEIGHT = 768;

bool twentyfourh = true;
bool leadingzero = false;
bool fullscreen = false;
bool animate = true;
bool anykeyclose = false;

int past_h = -1, past_m = -1;
int width = DEFAULT_WIDTH;
int height = DEFAULT_HEIGHT;
static const Uint32 AMBIENT_FRAME_INTERVAL = 100;
bool profile_rendering = false;

TTF_Font *font_time = NULL;
TTF_Font *font_mode = NULL;
TTF_Font *font_date = NULL;

const SDL_Color FONT_COLOR = {0xda, 0xda, 0xe2, 255};
const SDL_Color DATE_COLOR = {0xc6, 0xc2, 0xcf, 255};
const SDL_Color BACKGROUND_COLOR = {0x12, 0x14, 0x1d, 255};
const SDL_Color CARD_BORDER_COLOR = {0x37, 0x3d, 0x4e, 255};

static SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
static RenderTarget screen_target = {NULL, DEFAULT_WIDTH, DEFAULT_HEIGHT};
RenderTarget *screen = &screen_target;
static Uint32 user_event_type = 0;

Uint32 SDLCALL update_time(void *userdata, SDL_TimerID timerID, Uint32 interval) {
  (void)userdata;
  (void)timerID;
  SDL_Event e;
  SDL_Event pending_event;
  int pending = SDL_PeepEvents(&pending_event, 1, SDL_PEEKEVENT, user_event_type, user_event_type);
  if (pending < 0) {
    fprintf(stderr, "Unable to check pending clock events: %s\n", SDL_GetError());
    return interval;
  }
  if (pending == 0) {
    SDL_zero(e);
    e.type = user_event_type;
    e.user.code = 0;
    e.user.data1 = NULL;
    e.user.data2 = NULL;
    if (!SDL_PushEvent(&e)) {
      fprintf(stderr, "Unable to queue clock event: %s\n", SDL_GetError());
    }
  }

  return interval;
}


// Returns the value that follows option argv[*i] and advances *i, or NULL (with
// a message) if it is missing.
static const char *option_value(int argc, char **argv, int *i) {
  if (*i + 1 >= argc) {
    fprintf(stderr, "Option %s needs a value\n", argv[*i]);
    return NULL;
  }
  return argv[++*i];
}


static bool parse_int_in_range(const char *text, long min, long max, long *value) {
  char *end = NULL;
  errno = 0;
  long parsed = strtol(text, &end, 0);
  if (errno != 0 || end == text || *end != '\0' || parsed < min || parsed > max) return false;
  *value = parsed;
  return true;
}


int main(int argc, char **argv) {
  char *wid_env;
  double display_scale_factor = 1;
  bool seed_given = false;
  Uint32 scene_seed = 0;
  profile_rendering = getenv("GLUQLO_PROFILE") != NULL;

  Uint32 wid = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp("--help", argv[i]) == 0 || strcmp("-help", argv[i]) == 0) {
      printf("Usage: %s [OPTION...]\nOptions:\n", argv[0]);
      printf("  -help\t\tDisplay this\n");
      printf("  -root, -f\tFullscreen\n");
      printf("  -noflip\tDisable the flip animation (change time in one frame)\n");
      printf("  -anykeyclose\tClose app when mouse move or any key pressed\n");
      printf("  -ampm\t\tUse 12-hour clock format (AM/PM)\n");
      printf("  -leadingzero\tAlways display hour with two digits\n");
      printf("  -w\t\tCustom width\n");
      printf("  -h\t\tCustom height\n");
      printf("  -r\t\tCustom resolution in WxH format\n");
      printf("  -s\t\tCustom display scale factor\n");
      printf(
          "  -effect NAME[:KIND]\tShow only one scheduled effect, repeating "
          "(NAME 'list' lists them)\n");
      printf(
          "  -seed N\tStart the scenery (season, weather, effects) at a fixed "
          "point instead of a random one\n");
      return 0;
    } else if (strcmp("-root", argv[i]) == 0 || strcmp("-f", argv[i]) == 0 ||
               strcmp("--fullscreen", argv[i]) == 0) {
      fullscreen = true;
    } else if (strcmp("-noflip", argv[i]) == 0) {
      animate = false;
    } else if (strcmp("-anykeyclose", argv[i]) == 0) {
      anykeyclose = true;
    } else if (strcmp("-ampm", argv[i]) == 0) {
      twentyfourh = false;
    } else if (strcmp("-leadingzero", argv[i]) == 0) {
      leadingzero = true;
    } else if (strcmp("-r", argv[i]) == 0 || strcmp("--resolution", argv[i]) == 0) {
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      int parsed_width, parsed_height;
      char trailing;
      if (sscanf(value, "%dx%d%c", &parsed_width, &parsed_height, &trailing) != 2 ||
          parsed_width < 1 || parsed_width > 16384 || parsed_height < 1 || parsed_height > 16384) {
        fprintf(stderr, "Invalid resolution -- %s (expected WxH, e.g. 1280x720)\n", value);
        return 1;
      }
      width = parsed_width;
      height = parsed_height;
    } else if (strcmp("-w", argv[i]) == 0 || strcmp("-h", argv[i]) == 0) {
      bool is_width = argv[i][1] == 'w';
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      long parsed;
      if (!parse_int_in_range(value, 1, 16384, &parsed)) {
        fprintf(stderr, "Invalid %s -- %s\n", is_width ? "width" : "height", value);
        return 1;
      }
      if (is_width)
        width = (int)parsed;
      else
        height = (int)parsed;
    } else if (strcmp("-s", argv[i]) == 0) {
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      char *end = NULL;
      double parsed = strtod(value, &end);
      if (end == value || *end != '\0' || !(parsed > 0.0 && parsed <= 16.0)) {
        fprintf(stderr, "Invalid scale factor -- %s (expected a number above 0)\n", value);
        return 1;
      }
      display_scale_factor = parsed;
    } else if (strcmp("-window-id", argv[i]) == 0) {
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      long parsed;
      if (!parse_int_in_range(value, 1, 0xffffffffL, &parsed)) {
        fprintf(stderr, "Invalid window id -- %s\n", value);
        return 1;
      }
      wid = (Uint32)parsed;
    } else if (strcmp("-seed", argv[i]) == 0) {
      const char *value = option_value(argc, argv, &i);
      if (!value) return 1;
      long parsed;
      if (!parse_int_in_range(value, 0, 0xffffffffL, &parsed)) {
        fprintf(stderr, "Invalid seed -- %s (expected a number)\n", value);
        return 1;
      }
      scene_seed = (Uint32)parsed;
      seed_given = true;
    } else if (strcmp("-effect", argv[i]) == 0) {
      const char *effect_value = option_value(argc, argv, &i);
      if (!effect_value) {
        fprintf(stderr, "Try -effect list for the available effects.\n");
        return 1;
      }
      char effect_arg[64];
      snprintf(effect_arg, sizeof(effect_arg), "%s", effect_value);
      if (strcmp(effect_arg, "list") == 0) {
        for (size_t effect = 0; effect < effect_count; effect++) {
          if (effect_table[effect]->kind_count > 0) {
            printf("%s (kinds 0-%d)\n", effect_table[effect]->name,
                   effect_table[effect]->kind_count - 1);
          } else {
            printf("%s\n", effect_table[effect]->name);
          }
        }
        return 0;
      }
      char *kind_separator = strchr(effect_arg, ':');
      if (kind_separator) *kind_separator = '\0';
      forced_effect = find_effect(effect_arg);
      if (!forced_effect) {
        printf("Unknown effect -- %s\n", effect_arg);
        printf("Try -effect list for the available effects.\n");
        return 1;
      }
      if (kind_separator) {
        char *kind_end = NULL;
        long requested = strtol(kind_separator + 1, &kind_end, 10);
        if (kind_end == kind_separator + 1 || *kind_end != '\0' || requested < 0 ||
            requested >= forced_effect->kind_count) {
          if (forced_effect->kind_count > 0) {
            printf("Effect %s takes a kind from 0 to %d\n", forced_effect->name,
                   forced_effect->kind_count - 1);
          } else {
            printf("Effect %s has no kinds\n", forced_effect->name);
          }
          return 1;
        }
        forced_kind = (int)requested;
      }
    } else {
      fprintf(stderr, "Invalid option -- %s\n", argv[i]);
      fprintf(stderr, "Try --help for more information.\n");
      return 1;
    }
  }

  // Each launch starts at a different point of the season/weather/effect
  // schedule. -effect starts at 0 so the chosen effect begins straight away.
  if (forced_effect)
    scene_offset = 0;
  else if (seed_given)
    scene_offset = scene_offset_from_seed(scene_seed);
  else
    scene_offset = random_scene_offset();

  if (wid == 0) {
    if ((wid_env = getenv("XSCREENSAVER_WINDOW")) != NULL) {
      wid = (Uint32)strtol(wid_env, NULL, 0);
    }
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    return 1;
  }
  atexit(SDL_Quit);

  if (!TTF_Init()) {
    fprintf(stderr, "Unable to init SDL_ttf: %s\n", SDL_GetError());
    return 1;
  }
  atexit(TTF_Quit);

  SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, TITLE);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, !fullscreen && wid == 0);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
  if (fullscreen && wid == 0) {
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
  }
  if (wid != 0) {
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER, (Sint64)wid);
  }
  window = SDL_CreateWindowWithProperties(props);
  SDL_DestroyProperties(props);
  if (!window) {
    fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
    return 1;
  }

  renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    return 1;
  }
  SDL_SetRenderVSync(renderer, 1);
  screen_target.renderer = renderer;

  if (fullscreen || wid) {
    SDL_HideCursor();
  }

  user_event_type = SDL_RegisterEvents(1);
  if (user_event_type == 0) {
    fprintf(stderr, "Unable to register timer event: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 1;
  }

  layout_clock(display_scale_factor);
  render_clock(20, 19, false);

  // main loop
  bool done = false;
  SDL_Event event;
  SDL_TimerID timer = SDL_AddTimer(AMBIENT_FRAME_INTERVAL, update_time, NULL);
  if (!timer) {
    fprintf(stderr, "Unable to start clock timer: %s\n", SDL_GetError());
    destroy_clock_caches();
    if (backdrop_base) SDL_DestroyTexture(backdrop_base);
    if (date_texture_cache) SDL_DestroyTexture(date_texture_cache);
    if (ship_target_tex) SDL_DestroyTexture(ship_target_tex);
    TTF_CloseFont(font_time);
    TTF_CloseFont(font_mode);
    TTF_CloseFont(font_date);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 1;
  }

  float mouse_x = -1.0f;
  float mouse_y = -1.0f;

  while (!done && SDL_WaitEvent(&event)) {
    if (event.type == user_event_type) {
      time_t rawtime;
      struct tm current_time;
      time(&rawtime);
      localtime_r(&rawtime, &current_time);
      bool time_changed = current_time.tm_hour != past_h || current_time.tm_min != past_m;
      if (time_changed) {
        render_animation();
      } else {
        render_clock(20, 19, true);
      }
      continue;
    }
    switch (event.type) {
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      case SDL_EVENT_WINDOW_RESIZED:
        layout_clock(display_scale_factor);
        render_clock(20, 19, true);
        break;

      case SDL_EVENT_KEY_DOWN:
        if (anykeyclose) {
          done = true;
          break;
        }
        switch (event.key.key) {
          case SDLK_ESCAPE:
          case SDLK_Q:
            done = true;
            break;
          default:
            break;
        }
        break;

      case SDL_EVENT_MOUSE_MOTION:
        if ((mouse_x < 0.0f) || (mouse_y < 0.0f)) {
          mouse_x = event.motion.x;
          mouse_y = event.motion.y;
        }
        if (((mouse_x != event.motion.x) || (mouse_y != event.motion.y)) && anykeyclose)
          done = true;
        break;

      case SDL_EVENT_QUIT:
        done = true;
        break;
    }
  }

  SDL_RemoveTimer(timer);

  destroy_clock_caches();
  if (backdrop_base) SDL_DestroyTexture(backdrop_base);
  if (ship_target_tex) SDL_DestroyTexture(ship_target_tex);
  if (date_texture_cache) SDL_DestroyTexture(date_texture_cache);

  TTF_CloseFont(font_time);
  TTF_CloseFont(font_mode);
  TTF_CloseFont(font_date);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return 0;
}
