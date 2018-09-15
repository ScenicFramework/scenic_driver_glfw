/*
#  Created by Boyd Multerer on June 1, 2018.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#

functions to play a compiled render script
*/

  #include <stdio.h>
  // #include <string.h>
  #include <math.h>

  #include <stdlib.h>
  #include <GLFW/glfw3.h>
  // #include <GLES2/gl2.h>

  #include "nanovg/nanovg.h"
  #include "types.h"
  #include "comms.h"
  #include "render_script.h"
  #include "tx.h"


  // state control
  #define OP_PUSH_STATE              0X01
  #define OP_POP_STATE               0X02
  #define OP_RESET_STATE             0X03

  #define OP_RUN_SCRIPT              0X04

  // RENDER STYLES
  #define OP_PAINT_LINEAR            0X06
  #define OP_PAINT_BOX               0X07
  #define OP_PAINT_RADIAL            0X08
  #define OP_PAINT_IMAGE             0X09

  #define OP_ANTI_ALIAS              0X0A

  #define OP_STROKE_WIDTH            0X0C
  #define OP_STROKE_COLOR            0X0D
  #define OP_STROKE_PAINT            0X0E

  #define OP_FILL_COLOR              0X10
  #define OP_FILL_PAINT              0x11

  #define OP_MITER_LIMIT             0X14
  #define OP_LINE_CAP                0X15
  #define OP_LINE_JOIN               0X16
  #define OP_GLOBAL_ALPHA            0X17

  // SCISSORING
  #define OP_SCISSOR                 0X1B
  #define OP_INTERSECT_SCISSOR       0X1C
  #define OP_RESET_SCISSOR           0X1D

  // PATH OPERATIONS
  #define OP_PATH_BEGIN              0X20
  
  #define OP_PATH_MOVE_TO            0X21
  #define OP_PATH_LINE_TO            0X22
  #define OP_PATH_BEZIER_TO          0X23
  #define OP_PATH_QUADRATIC_TO       0X24
  #define OP_PATH_ARC_TO             0X25
  #define OP_PATH_CLOSE              0X26
  #define OP_PATH_WINDING            0X27
  
  #define OP_FILL                    0X29
  #define OP_STROKE                  0X2A
  
  #define OP_TRIANGLE                0X2C
  #define OP_ARC                     0X2D
  #define OP_RECT                    0X2E
  #define OP_ROUND_RECT              0X2F
  #define OP_ROUND_RECT_VAR          0X30
  #define OP_ELLIPSE                 0X31
  #define OP_CIRCLE                  0X32
  #define OP_SECTOR                  0X33

  #define OP_TEXT                    0x34


  // TRANSFORM OPERATIONS
  #define OP_TX_RESET                0X36
  #define OP_TX_IDENTITY             0X37
  #define OP_TX_MATRIX               0X38
  #define OP_TX_TRANSLATE            0X39
  #define OP_TX_SCALE                0X3A
  #define OP_TX_ROTATE               0X3B
  #define OP_TX_SKEW_X               0X3C
  #define OP_TX_SKEW_Y               0X3D


  #define OP_FONT                    0X40
  #define OP_FONT_BLUR               0X41
  #define OP_FONT_SIZE               0X42
  #define OP_TEXT_ALIGN              0X43
  #define OP_TEXT_HEIGHT             0X44


  #define OP_TERMINATE               0XFF

  static const float        TAU  = NVG_PI * 2;

  NVGpaint current_paint;

//=============================================================================
// access functions for scripts


void delete_script( window_data_t* p_data, GLuint id ) {
  if (p_data->p_scripts[id]) {
    free(p_data->p_scripts[id]);
  p_data->p_scripts[id] = NULL;
  }
}

void delete_all( window_data_t* p_data ){
  for (GLuint i = 0; i < p_data->num_scripts; i++ ) {
    delete_script( p_data, i );
  }
}

void put_script( window_data_t* p_data, GLuint id, void* p_script ) {
  delete_script( p_data, id );
  p_data->p_scripts[id] = p_script;
}

void* get_script( window_data_t* p_data, GLuint id ) {
  return p_data->p_scripts[id];
}



//=============================================================================
// types

typedef struct __attribute__((__packed__)) 
{
  GLuint      r;
  GLuint      g;
  GLuint      b;
  GLuint      a;
} color_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     x;
  GLfloat     y;
} xy_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     x;
  GLfloat     y;
  GLfloat     w;
  GLfloat     h;
} xywh_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     w;
  GLfloat     h;
} wh_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     w;
  GLfloat     h;
  GLfloat     r;
} whr_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     c1x;
  GLfloat     c1y;
  GLfloat     c2x;
  GLfloat     c2y;
  GLfloat     x;
  GLfloat     y;
} bezier_to_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     cx;
  GLfloat     cy;
  GLfloat     x;
  GLfloat     y;
} quadratic_to_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     x1;
  GLfloat     y1;
  GLfloat     x2;
  GLfloat     y2;
  GLfloat     radius;
} arc_to_t;

// typedef struct __attribute__((__packed__)) 
// {
//   GLfloat     cx;
//   GLfloat     cy;
//   GLfloat     radius;
//   GLfloat     a0;
//   GLfloat     a1;
//   byte        direction;
// } arc_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     rx;
  GLfloat     ry;
} ellipse_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     r;
} circle_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     radius;
  GLfloat     start;
  GLfloat     finish;
} arc_sector_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     a;
  GLfloat     b;
  GLfloat     c;
  GLfloat     d;
  GLfloat     e;
  GLfloat     f;
} matrix_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     x0;
  GLfloat     y0;
  GLfloat     x1;
  GLfloat     y1;
  GLfloat     x2;
  GLfloat     y2;
} triangle_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     sx;
  GLfloat     sy;
  GLfloat     ex;
  GLfloat     ey;
  GLuint      sr;
  GLuint      sg;
  GLuint      sb;
  GLuint      sa;
  GLuint      er;
  GLuint      eg;
  GLuint      eb;
  GLuint      ea;
} linear_gradient_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     x;
  GLfloat     y;
  GLfloat     w;
  GLfloat     h;
  GLfloat     radius;
  GLfloat     feather;
  GLuint      sr;
  GLuint      sg;
  GLuint      sb;
  GLuint      sa;
  GLuint      er;
  GLuint      eg;
  GLuint      eb;
  GLuint      ea;
} box_gradient_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     cx;
  GLfloat     cy;
  GLfloat     r_in;
  GLfloat     r_out;
  GLuint      sr;
  GLuint      sg;
  GLuint      sb;
  GLuint      sa;
  GLuint      er;
  GLuint      eg;
  GLuint      eb;
  GLuint      ea;
} radial_gradient_t;

typedef struct __attribute__((__packed__)) 
{
  GLfloat     ox;
  GLfloat     oy;
  GLfloat     ex;
  GLfloat     ey;
  GLfloat     angle;
  GLuint      alpha;
  GLuint      key_size;
} image_pattern_t;

typedef struct __attribute__((__packed__)) 
{
  GLuint      size;
} text_t;

//=============================================================================
// operations

//---------------------------------------------------------
// run script
void* internal_run_script( void* p_script, window_data_t* p_data ) {
  GLuint id = *(GLuint*)p_script;
  // char buff[200];
  // sprintf(buff, "run_script %d", id);
  // send_puts( buff );
  run_script( id, p_data );
  return p_script + sizeof(GLuint);
}

//---------------------------------------------------------
// paint setup

void* paint_linear( NVGcontext* p_ctx, void* p_script ) {
  linear_gradient_t* grad = (linear_gradient_t*)p_script;

  current_paint = nvgLinearGradient(
    p_ctx, 
    grad->sx, grad->sy, grad->ex, grad->ey,
    nvgRGBA(grad->sr, grad->sg, grad->sb, grad->sa),
    nvgRGBA(grad->er, grad->eg, grad->eb, grad->ea)
    );
  
  return p_script + sizeof(linear_gradient_t);
}

void* paint_box( NVGcontext* p_ctx, void* p_script ) {
  box_gradient_t* grad = (box_gradient_t*)p_script;

  current_paint = nvgBoxGradient(
    p_ctx, 
    grad->x, grad->y, grad->w, grad->h,
    grad->radius, grad->feather,
    nvgRGBA(grad->sr, grad->sg, grad->sb, grad->sa),
    nvgRGBA(grad->er, grad->eg, grad->eb, grad->ea)
    );
  
  return p_script + sizeof(box_gradient_t);
}

void* paint_radial( NVGcontext* p_ctx, void* p_script ) {
  radial_gradient_t* grad = (radial_gradient_t*)p_script;

  current_paint = nvgRadialGradient(
    p_ctx, 
    grad->cx, grad->cy, grad->r_in, grad->r_out,
    nvgRGBA(grad->sr, grad->sg, grad->sb, grad->sa),
    nvgRGBA(grad->er, grad->eg, grad->eb, grad->ea)
    );
  
  return p_script + sizeof(radial_gradient_t);
}

void* paint_image( NVGcontext* p_ctx, void* p_script, window_data_t* p_data ) {
  image_pattern_t* img = (image_pattern_t*)p_script;
  p_script += sizeof(image_pattern_t);

  float alpha = (float)img->alpha / 255.0;

  // get the image id from the hash.
  int id = get_tx_id(p_data->p_tx_ids, p_script);

  // if the id is -1, then it isn't loaded
  if ( id < 0 ) {
    send_cache_miss( p_script );
  } else {

    // if ox, oy, ex, ey are all zero, then use the
    // natural h/w of the image
    float ox = img->ox;
    float oy = img->oy;
    float ex = img->ex;
    float ey = img->ey;
    if ( ox == 0.0 && oy == 0.0 && ex == 0.0 && ey == 0.0 ) {
      int w, h;
      nvgImageSize(p_ctx, id, &w, &h);
      ex = w;
      ey = h;
    }

    // the id is loaded and found
    current_paint = nvgImagePattern(
      p_ctx, 
      ox, oy, ex, ey,
      img->angle, id, alpha
    );
  }

  return p_script + img->key_size;
}


//---------------------------------------------------------
// render styles


void* shape_anti_alias( NVGcontext* p_ctx, void* p_script ) {
  nvgShapeAntiAlias(p_ctx, *(int*)p_script);
  return p_script + sizeof(int);
}

void* stroke_color( NVGcontext* p_ctx, void* p_script ) {
  color_t* color = (color_t*)p_script;
  nvgStrokeColor(p_ctx, nvgRGBA(color->r, color->g, color->b, color->a));
  return p_script + sizeof(color_t);
}

void* shape_width( NVGcontext* p_ctx, void* p_script ) {
  nvgStrokeWidth(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}

void* fill_color( NVGcontext* p_ctx, void* p_script ) {
  color_t* color = (color_t*)p_script;
  nvgFillColor(p_ctx, nvgRGBA(color->r, color->g, color->b, color->a));
  return p_script + sizeof(color_t);
}


void* miter_limit( NVGcontext* p_ctx, void* p_script ) {
  nvgMiterLimit(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}

void* line_cap( NVGcontext* p_ctx, void* p_script ) {
  uint32_t join = *(uint32_t*)p_script;
  switch( join ) {
    case 0:   nvgLineCap(p_ctx, NVG_BUTT);     break;
    case 1:   nvgLineCap(p_ctx, NVG_ROUND);     break;
    case 2:   nvgLineCap(p_ctx, NVG_SQUARE);     break;
    default: break;
  }
  return p_script + sizeof(uint32_t);
}

void* line_join( NVGcontext* p_ctx, void* p_script ) {
  uint32_t join = *(uint32_t*)p_script;
  switch( join ) {
    case 0:   nvgLineJoin(p_ctx, NVG_MITER);     break;
    case 1:   nvgLineJoin(p_ctx, NVG_ROUND);     break;
    case 2:   nvgLineJoin(p_ctx, NVG_BEVEL);     break;
    default: break;
  }
  return p_script + sizeof(uint32_t);
}

void* global_alpha( NVGcontext* p_ctx, void* p_script ) {
  nvgGlobalAlpha(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}

//---------------------------------------------------------
// scissors

void* scissor( NVGcontext* p_ctx, void* p_script ) {
  wh_t* wh = (wh_t*)p_script;
  nvgScissor(p_ctx, 0, 0, wh->w, wh->h);
  return p_script + sizeof(wh_t);
}

void* intersect_scissor( NVGcontext* p_ctx, void* p_script ) {
  wh_t* wh = (wh_t*)p_script;
  nvgIntersectScissor(p_ctx, 0, 0, wh->w, wh->h);
  return p_script + sizeof(wh_t);
}

//---------------------------------------------------------
// paths

void* move_to( NVGcontext* p_ctx, void* p_script ) {
  xy_t* xywh = (xy_t*)p_script;
  nvgMoveTo(p_ctx, xywh->x, xywh->y);
  return p_script + sizeof(xy_t);
}

void* line_to( NVGcontext* p_ctx, void* p_script ) {
  xy_t* xywh = (xy_t*)p_script;
  nvgLineTo(p_ctx, xywh->x, xywh->y);
  return p_script + sizeof(xy_t);
}

void* bezier_to( NVGcontext* p_ctx, void* p_script ) {
  bezier_to_t* bezier = (bezier_to_t*)p_script;
  nvgBezierTo(p_ctx, bezier->c1x, bezier->c1y, bezier->c2x, bezier->c2y, bezier->x, bezier->y);
  return p_script + sizeof(bezier_to_t);
}

void* quadratic_to( NVGcontext* p_ctx, void* p_script ) {
  quadratic_to_t* quad = (quadratic_to_t*)p_script;
  nvgQuadTo(p_ctx, quad->cx, quad->cy, quad->x, quad->y);
  return p_script + sizeof(quadratic_to_t);
}

void* arc_to( NVGcontext* p_ctx, void* p_script ) {
  arc_to_t* arc = (arc_to_t*)p_script;
  nvgArcTo(p_ctx, arc->x1, arc->y1, arc->x2, arc->y2, arc->radius);
  return p_script + sizeof(arc_to_t);
}

void* path_winding( NVGcontext* p_ctx, void* p_script ) {
  if (*(int*)p_script) {
    nvgPathWinding(p_ctx, NVG_SOLID);
  } else {
    nvgPathWinding(p_ctx, NVG_HOLE);
  }
  return p_script + sizeof(int);
}

void* triangle( NVGcontext* p_ctx, void* p_script ) {
  triangle_t* tri = (triangle_t*)p_script;
  nvgMoveTo(p_ctx, tri->x0, tri->y0);
  nvgLineTo(p_ctx, tri->x1, tri->y1);
  nvgLineTo(p_ctx, tri->x2, tri->y2);
  nvgClosePath(p_ctx);
  return p_script + sizeof(triangle_t);
}

void* rect( NVGcontext* p_ctx, void* p_script ) {
  wh_t* wh = (wh_t*)p_script;
  nvgRect(p_ctx, 0, 0, wh->w, wh->h);
  return p_script + sizeof(wh_t);
}

void* round_rect( NVGcontext* p_ctx, void* p_script ) {
  whr_t* whr = (whr_t*)p_script;
  nvgRoundedRect(p_ctx, 0, 0, whr->w, whr->h, whr->r);
  return p_script + sizeof(whr_t);
}

void* ellipse( NVGcontext* p_ctx, void* p_script ) {
  ellipse_t* ellipse = (ellipse_t*)p_script;
  nvgEllipse(p_ctx, 0, 0, ellipse->rx, ellipse->ry);
  return p_script + sizeof(ellipse_t);
}

void* circle( NVGcontext* p_ctx, void* p_script ) {
  GLfloat radius = *(GLfloat*)p_script;
  nvgCircle(p_ctx, 0, 0, radius);
  return p_script + sizeof(GLfloat);
}

void* arc( NVGcontext* p_ctx, void* p_script ) {
  // bring sector data onto the stack
  arc_sector_t sector = *(arc_sector_t*)p_script;

  // clamp the angle to a circle
  float angle = sector.finish - sector.start;
  angle = angle > TAU ? TAU : angle;
  angle = angle < -TAU ? -TAU : angle;

  // calculate the number of segments
  int segment_count = log2(sector.radius) * fabsf(angle) * 2;
  float increment = angle / segment_count;
  float a = sector.start;


  // don't draw anything if the angle is so small that the segment_count is zero
  if ( segment_count > 0 ) {
    // Arc starts on the perimeter. Sector starts in the center.
    for (int i = 0; i <= segment_count; ++i) {
      float px, py;
      px = sector.radius * cos(a);
      py = sector.radius * sin(a);
      if (i == 0 ) {
        nvgMoveTo(p_ctx, px, py);
      } else {
        nvgLineTo(p_ctx, px, py);
      }
      a += increment ;
    }
    // arc doesn't close
  }

  return p_script + sizeof(arc_sector_t);
}


void* sector( NVGcontext* p_ctx, void* p_script ) {
  // bring sector data onto the stack
  arc_sector_t sector = *(arc_sector_t*)p_script;

  // clamp the angle to a circle
  float angle = sector.finish - sector.start;
  angle = angle > TAU ? TAU : angle;
  angle = angle < -TAU ? -TAU : angle;


  // calculate the number of segments
  int segment_count = log2(sector.radius) * fabsf(angle) * 2;
  float increment = angle / segment_count;
  float a = sector.start;

  // don't draw anything if the angle is so small that the segment_count is zero
  if ( segment_count > 0 ) {
    // Arc starts on the perimeter. Sector starts in the center.
    nvgMoveTo(p_ctx, 0, 0);

    for (int i = 0; i <= segment_count; ++i) {
      float px, py;
      px = sector.radius * cos(a);
      py = sector.radius * sin(a);
      nvgLineTo(p_ctx, px, py);
      a += increment ;
    }
    nvgClosePath(p_ctx);    
  }

  return p_script + sizeof(arc_sector_t);
}

void* text( NVGcontext* p_ctx, void* p_script ) {
  text_t* text_info = (text_t*)p_script;
  p_script += sizeof(text_t);

  float x = 0;
  float y = 0;
  const char* start = p_script;
  const char* end = start + text_info->size;
  float lineh;
  nvgTextMetrics(p_ctx, NULL, NULL, &lineh);
  NVGtextRow rows[3];
  int nrows, i;

  // up to this code to break the lines...
  while ((nrows = nvgTextBreakLines(p_ctx, start, end, 1000, rows, 3))) {
    for (i = 0; i < nrows; i++) {
      NVGtextRow* row = &rows[i];
      nvgText(p_ctx, x, y, row->start, row->end);
      y += lineh;
    }
    // Keep going...
    start = rows[nrows-1].next;
  }

  return p_script + text_info->size;
}



// typedef struct __attribute__((__packed__)) 
// {
//   GLfloat     x;
//   GLfloat     y;
//   GLuint      size;
// } text_t;




//---------------------------------------------------------
// transforms


void* tx_rotate( NVGcontext* p_ctx, void* p_script ) {
  nvgRotate(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}

void* tx_translate( NVGcontext* p_ctx, void* p_script ) {
  xy_t* xy = (xy_t*)p_script;
  nvgTranslate(p_ctx, xy->x, xy->y);
  return p_script + sizeof(xy_t);  
}

void* tx_scale( NVGcontext* p_ctx, void* p_script ) {
  xy_t* xy = (xy_t*)p_script;
  nvgScale(p_ctx, xy->x, xy->y);
  return p_script + sizeof(xy_t);  
}

void* tx_skew_x( NVGcontext* p_ctx, void* p_script ) {
  nvgSkewX(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}

void* tx_skew_y( NVGcontext* p_ctx, void* p_script ) {
  nvgSkewY(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}

void* tx_matrix( NVGcontext* p_ctx, void* p_script ) {
  matrix_t* tx = (matrix_t*)p_script;
  nvgTransform(p_ctx, tx->a, tx->b, tx->c, tx->d, tx->e, tx->f);
  return p_script + sizeof(matrix_t);
}

//---------------------------------------------------------
// font styles

void* font( NVGcontext* p_ctx, void* p_script ) {
  GLuint  name_length = *(GLuint*)p_script;
  p_script += sizeof(GLuint);

  // get the id for the font. If it isn't loaded, request it
  int font_id = nvgFindFont(p_ctx, p_script);
  if ( font_id >= 0 ) {
    nvgFontFaceId(p_ctx, font_id);
  } else {
    // the font is NOT loaded. Request it from the ex code above
    send_font_miss( p_script );
  }

  return p_script + name_length;
}

void* font_blur( NVGcontext* p_ctx, void* p_script ) {
  nvgFontBlur(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}

void* font_size( NVGcontext* p_ctx, void* p_script ) {
  nvgFontSize(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}

void* text_align( NVGcontext* p_ctx, void* p_script ) {
  nvgTextAlign(p_ctx, *(uint32_t*)p_script);
  return p_script + sizeof(uint32_t);
}

void* text_height( NVGcontext* p_ctx, void* p_script ) {
  nvgTextLineHeight(p_ctx, *(float*)p_script);
  return p_script + sizeof(float);
}



//=============================================================================
// the main script function

//---------------------------------------------------------
void run_script( GLuint script_id, window_data_t* p_data ) {
  char buff[200];

  // sprintf(buff, "script id: %d", script_id);
  // send_puts(buff);

  // get the script in question. bail if it isn't there
  void* p_script = get_script( p_data, script_id );
  if (p_script == NULL) {
    // sprintf(buff, "Tried to render NULL script %d", script_id);
    // send_puts( buff );
    return;
  };

  // setup
  NVGcontext* p_ctx = p_data->context.p_ctx;

  // get the first op
  GLuint op = *(GLuint*)p_script;

  // loop though the script, running each command in turn.
  // recurse into more script calls if necessary
  while ( op != OP_TERMINATE ) {
    // sprintf(buff, "op: 0x%X", op);
    // send_puts(buff);

    // advance the pointer past the op code
    p_script += sizeof(GLuint);

    // take the appropriate action based on the command id
    switch( op ) {
      // state control
      case OP_PUSH_STATE:               nvgSave(p_ctx);     break;
      case OP_POP_STATE:                nvgRestore(p_ctx);  break;
      case OP_RESET_STATE:              nvgReset(p_ctx);    break;

      // script control
      case OP_RUN_SCRIPT:               p_script = internal_run_script( p_script, p_data ); break;

      // render styles
      case OP_PAINT_LINEAR:             p_script = paint_linear( p_ctx, p_script ); break;
      case OP_PAINT_BOX:                p_script = paint_box( p_ctx, p_script ); break;
      case OP_PAINT_RADIAL:             p_script = paint_radial( p_ctx, p_script ); break;
      case OP_PAINT_IMAGE:              p_script = paint_image( p_ctx, p_script, p_data ); break;

      case OP_ANTI_ALIAS:               p_script = shape_anti_alias( p_ctx, p_script ); break;

      case OP_STROKE_WIDTH:             p_script = shape_width( p_ctx, p_script ); break;
      case OP_STROKE_COLOR:             p_script = stroke_color( p_ctx, p_script ); break;
      case OP_STROKE_PAINT:             nvgStrokePaint(p_ctx, current_paint); break;

      case OP_FILL_COLOR:               p_script = fill_color( p_ctx, p_script ); break;
      case OP_FILL_PAINT:               nvgFillPaint(p_ctx, current_paint); break;

      case OP_MITER_LIMIT:              p_script = miter_limit( p_ctx, p_script ); break;
      case OP_LINE_CAP:                 p_script = line_cap( p_ctx, p_script ); break;
      case OP_LINE_JOIN:                p_script = line_join( p_ctx, p_script ); break;
      case OP_GLOBAL_ALPHA:             p_script = global_alpha( p_ctx, p_script ); break;

      // scissoring
      case OP_SCISSOR:                  p_script = scissor( p_ctx, p_script ); break;
      case OP_INTERSECT_SCISSOR:        p_script = intersect_scissor( p_ctx, p_script ); break;
      case OP_RESET_SCISSOR:            nvgResetScissor(p_ctx); break;

      // path operations
      case OP_PATH_BEGIN:               nvgBeginPath(p_ctx); break;

      case OP_PATH_MOVE_TO:             p_script = move_to( p_ctx, p_script ); break;
      case OP_PATH_LINE_TO:             p_script = line_to( p_ctx, p_script ); break;
      case OP_PATH_BEZIER_TO:           p_script = bezier_to( p_ctx, p_script ); break;
      case OP_PATH_QUADRATIC_TO:        p_script = quadratic_to( p_ctx, p_script ); break;
      case OP_PATH_ARC_TO:              p_script = arc_to( p_ctx, p_script ); break;
      case OP_PATH_CLOSE:               nvgClosePath(p_ctx); break;
      case OP_PATH_WINDING:             p_script = path_winding( p_ctx, p_script ); break;

      case OP_FILL:                     nvgFill(p_ctx); break;
      case OP_STROKE:                   nvgStroke(p_ctx); break;

      case OP_TRIANGLE:                 p_script = triangle( p_ctx, p_script ); break;
      case OP_ARC:                      p_script = arc( p_ctx, p_script ); break;
      case OP_RECT:                     p_script = rect( p_ctx, p_script ); break;
      case OP_ROUND_RECT:               p_script = round_rect( p_ctx, p_script ); break;
      case OP_ROUND_RECT_VAR:           break;
      case OP_ELLIPSE:                  p_script = ellipse( p_ctx, p_script ); break;
      case OP_CIRCLE:                   p_script = circle( p_ctx, p_script ); break;
      case OP_SECTOR:                   p_script = sector( p_ctx, p_script ); break;
      case OP_TEXT:                     p_script = text( p_ctx, p_script ); break;

      // transform operations
      case OP_TX_RESET:                 nvgResetTransform(p_ctx); break;
      case OP_TX_IDENTITY:              break;
      case OP_TX_MATRIX:                p_script = tx_matrix( p_ctx, p_script ); break;
      case OP_TX_TRANSLATE:             p_script = tx_translate( p_ctx, p_script ); break;
      case OP_TX_SCALE:                 p_script = tx_scale( p_ctx, p_script ); break;
      case OP_TX_ROTATE:                p_script = tx_rotate( p_ctx, p_script ); break;
      case OP_TX_SKEW_X:                p_script = tx_skew_x( p_ctx, p_script ); break;
      case OP_TX_SKEW_Y:                p_script = tx_skew_y( p_ctx, p_script ); break;

      // font styles
      case OP_FONT:                     p_script = font( p_ctx, p_script ); break;
      case OP_FONT_BLUR:                p_script = font_blur( p_ctx, p_script ); break;
      case OP_FONT_SIZE:                p_script = font_size( p_ctx, p_script ); break;
      case OP_TEXT_ALIGN:               p_script = text_align( p_ctx, p_script ); break;
      case OP_TEXT_HEIGHT:              p_script = text_height( p_ctx, p_script ); break;


      case OP_TERMINATE:                return;

      default:
        sprintf(buff, "!!!Unknown script command: %d", op);
        send_puts( buff );
        // send_puts( "!!!Unknown script command" );
        return;
    }

    // prep the next op code
    op = *(GLuint*)p_script;
  }
}