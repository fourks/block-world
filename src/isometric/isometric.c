#include "isometric.h"

static bool s_enabled = true;
static GPoint s_projection_offset;

static GBitmap *s_fb = NULL;
static GSize s_fb_size;
static uint8_t *s_fb_data = NULL;

GPoint project(Vec3 input) {
  GPoint result;
  if(s_enabled) {
    result.x = s_projection_offset.x + (input.x - input.y);
    result.y = s_projection_offset.y + ((input.x / 2) + (input.y / 2)) - input.z;
  } else {
    result.x = input.x;
    result.y = input.y;
  }
  return result;
}

static void set_pixel(GPoint pixel, GColor color) {
  if(pixel.x >= 0 && pixel.x < 144 && pixel.y >= 0 && pixel.y < 168) {
    memset(&s_fb_data[(pixel.y * s_fb_size.w) + pixel.x], (uint8_t)color.argb, 1);
  }
}

/**
 * http://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
 */
static void bresenham_line(GPoint start, GPoint finish, GColor color) {
  int dx = abs(finish.x - start.x);
  int sx = (start.x < finish.x) ? 1 : -1;
  int dy = abs(finish.y - start.y);
  int sy = (start.y < finish.y) ? 1 : -1;
  int err = ((dx > dy) ? dx : -dy) / 2;
  int e2;
  while(true) {
    set_pixel(GPoint(start.x, start.y), color);
    if(start.x == finish.x && start.y == finish.y) {
      break;
    }
    e2 = err;
    if(e2 > -dx) {
      err -= dy;
      start.x += sx;
    }
    if(e2 < dy) {
      err += dx;
      start.y += sy;
    }
  }
}

GBitmap* isometric_begin(GContext *ctx) {
  s_fb = graphics_capture_frame_buffer(ctx);
#ifdef PBL_PLATFORM_BASALT
  s_fb_data = gbitmap_get_data(s_fb);
  s_fb_size = gbitmap_get_bounds(s_fb).size;
#else
  s_fb_data = s_fb->addr;
  s_fb_size = s_fb->bounds.size;
#endif

  // Optionally further use the framebuffer GBitmap
  return s_fb;
}

void isometric_finish(GContext *ctx) {
  if(s_fb) {
    graphics_release_frame_buffer(ctx, s_fb);

    s_fb = NULL;
    s_fb_data = NULL;
  }
}

void isometric_set_enabled(bool b) {
  s_enabled = b;
}

void isometric_set_projection_offset(GPoint offset) {
  s_projection_offset = offset;
}

void isometric_draw_rect(Vec3 origin, GSize size, GColor color) {
  // Top
  GPoint start = project(origin);
  GPoint finish = project(Vec3(origin.x + size.w, origin.y, origin.z));
  bresenham_line(start, finish, color);

  // Right
  start = project(Vec3(origin.x + size.w, origin.y, origin.z));
  finish = project(Vec3(origin.x + size.w, origin.y + size.h, origin.z));
  bresenham_line(start, finish, color);

  // Bottom
  start = project(Vec3(origin.x, origin.y + size.h, origin.z));
  finish = project(Vec3(origin.x + size.w, origin.y + size.h, origin.z));
  bresenham_line(start, finish, color);

  // Left
  start = project(origin);
  finish = project(Vec3(origin.x, origin.y + size.h, origin.z));
  bresenham_line(start, finish, color);
}

void isometric_fill_rect(Vec3 origin, GSize size, GColor color) {
  for(int i = 0; i < 2; i++) {
    for(int y = origin.y; y < origin.y + size.h; y++) {
      bresenham_line(
        project(Vec3(origin.x, y, origin.z)), 
        project(Vec3(origin.x + size.w, y, origin.z)),
        color
      );
    }
    origin.z--; // Draw twice to fill
  }
}

void isometric_fill_box(Vec3 origin, GSize size, int z_height, GColor color) {
  // Draw only front for speed when OPTIMIZE_FILL_BOX is defined
  int z = 0;
  for(z = origin.z; z < origin.z + z_height; z++) {
#ifdef OPTIMIZE_FILL_BOX
    // Right
    GPoint start = project(Vec3(origin.x + size.w, origin.y, z));
    GPoint finish = project(Vec3(origin.x + size.w, origin.y + size.h, z));
    bresenham_line(start, finish, color);

    // Bottom
    start = project(Vec3(origin.x, origin.y + size.h, z));
    finish = project(Vec3(origin.x + size.w, origin.y + size.h, z));
    bresenham_line(start, finish, color);
#else
    isometric_draw_rect(Vec3(origin.x, origin.y, z), size, color);
#endif
  }

  // Fill in top
  isometric_fill_rect(Vec3(origin.x, origin.y, (z_height > 0) ? z - 1 : z), size, color);
}

void isometric_draw_box(Vec3 origin, GSize size, int z_height, GColor color) {
  // Bottom
  GPoint start = project(Vec3(origin.x + size.w, origin.y, origin.z));
  GPoint finish = project(Vec3(origin.x + size.w, origin.y + size.h, origin.z));
  bresenham_line(start, finish, color);
  start = project(Vec3(origin.x, origin.y + size.h, origin.z));
  finish = project(Vec3(origin.x + size.w, origin.y + size.h, origin.z));
  bresenham_line(start, finish, color);

  // Top
  isometric_draw_rect(Vec3(origin.x, origin.y, origin.z + z_height), size, color);

  // Sides
  bresenham_line(
    project(Vec3(origin.x, origin.y + size.h, origin.z)), 
    project(Vec3(origin.x, origin.y + size.h, origin.z + z_height)),
    color
  );
  bresenham_line(
    project(Vec3(origin.x + size.w, origin.y + size.h, origin.z)), 
    project(Vec3(origin.x + size.w, origin.y + size.h, origin.z + z_height)),
    color
  );
  bresenham_line(
    project(Vec3(origin.x + size.w, origin.y, origin.z)), 
    project(Vec3(origin.x + size.w, origin.y, origin.z + z_height)),
    color
  );
}

void isometric_draw_pixel(Vec3 point, GColor color) {
  set_pixel(project(point), color);
}