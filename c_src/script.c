/*
#  Created by Boyd Multerer on 2021-03-09.
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#include <string.h>

#include "utils.h"
#include "comms.h"
#include "script.h"
#include "image.h"


#define HASH_ID(id)  tommy_inthash_u32(id)

tommy_hashlin   scripts = {0};



//---------------------------------------------------------
void init_scripts( void ) {
  // init the hash table
  tommy_hashlin_init( &scripts );
}

//=============================================================================
// internal utilities for working with the script hash

// isolate all knowledge of the hash table implementation to these functions

//---------------------------------------------------------
static int _comparator(const void* arg, const void* obj) {
  return *(const uint32_t*)arg != ((const script_t*)obj)->id;
}

//---------------------------------------------------------
script_t* get_script( uint32_t id ) {
  return tommy_hashlin_search(
    &scripts,
    _comparator,
    &id,
    HASH_ID( id ) 
  );
}

//---------------------------------------------------------
void do_delete_script( int id ) {
  script_t* p_script = get_script( id );
  if ( p_script ) {
    tommy_hashlin_remove_existing( &scripts, &p_script->node );
    free( p_script );
  }
}



//---------------------------------------------------------
script_t* put_script( int* p_msg_length ) {

  // read the id of the script, which is in the first four bytes 
  uint32_t id;
  read_bytes_down( &id, sizeof(uint32_t), p_msg_length );

  // if there already is a script with that id, delete it
  do_delete_script( id );

  // alloc the script struct and space for the script itself
  int struct_size = ALIGN_UP(sizeof(script_t), 64);
  int alloc_size = struct_size + *p_msg_length;
  script_t *p_script = malloc( alloc_size );
  if ( !p_script ) return NULL;

  // initialize the script
  p_script->id = id;
  p_script->size = *p_msg_length;
  p_script->p_data = ((void*)p_script) + struct_size;

  // read in the script itself
  read_bytes_down( p_script->p_data, *p_msg_length, p_msg_length );

  // insert the script into the tommy hash
  tommy_hashlin_insert( &scripts, &p_script->node, p_script, HASH_ID(id) );

  // return the completed struct
  return p_script;
}

//---------------------------------------------------------
void delete_script( int* p_msg_length ) {
  // read the id of the script, which is in the first four bytes 
  uint32_t id;
  read_bytes_down( &id, sizeof(uint32_t), p_msg_length );
  do_delete_script( id );
}

//---------------------------------------------------------
void reset_scripts() {
  // deallocates all the objects iterating the hashtable
  tommy_hashlin_foreach( &scripts, free );

  // deallocates the hashtable
  tommy_hashlin_done( &scripts );

  // re-init the hash table
  tommy_hashlin_init( &scripts );
}


//=============================================================================
// rendering

static void put_msg_i( char* msg, int n ) {
  char buff[200];
  sprintf(buff, "%s %d", msg, n);
  send_puts(buff);
}

static inline GLubyte get_byte( void* p, uint32_t offset ) {
  return *((GLubyte*)(p + offset));
}

static inline GLushort get_uint16( void* p, uint32_t offset ) {
  return ntoh_ui16(*((GLushort*)(p + offset)));
}

static inline GLuint get_uint32( void* p, uint32_t offset ) {
  return ntoh_ui32(*((GLuint*)(p + offset)));
}

static inline GLfloat get_float( void* p, uint32_t offset ) {
  return ntoh_f32(*((GLfloat*)(p + offset)));
}


//---------------------------------------------------------
void set_font( char* p_font, NVGcontext* p_ctx ) {
  int font_id = nvgFindFont(p_ctx, p_font);
  if (font_id >= 0) {
    nvgFontFaceId(p_ctx, font_id);
  }
}

void render_text( char* p_text, unsigned int size, NVGcontext* p_ctx )
{
  float       x     = 0;
  float       y     = 0;
  const char* start = p_text;
  const char* end   = start + size;
  float       lineh;
  nvgTextMetrics(p_ctx, NULL, NULL, &lineh);
  NVGtextRow rows[3];
  int        nrows, i;

  // up to this code to break the lines...
  while ((nrows = nvgTextBreakLines(p_ctx, start, end, 1000, rows, 3)))
  {
    for (i = 0; i < nrows; i++)
    {
      NVGtextRow* row = &rows[i];
      nvgText(p_ctx, x, y, row->start, row->end);
      y += lineh;
    }
    // Keep going...
    start = rows[nrows - 1].next;
  }
}


int render_sprites( NVGcontext* p_ctx, void* p, int i, uint16_t  count ) {

  // // get the count of draw commands
  // GLuint img_id = get_uint32( p, i );
  // i += 4;

  // get a pointer to the id
  hash_id_t* p_id = p + i;
  i += 32;

  // loop the draw commands and draw each
  for ( int n = 0; n < count; n++ ) {
    GLfloat sx = get_float( p, i );
    GLfloat sy = get_float( p, i + 4 );
    GLfloat sw = get_float( p, i + 8 );
    GLfloat sh = get_float( p, i + 12 );
    GLfloat dx = get_float( p, i + 16 );
    GLfloat dy = get_float( p, i + 20 );
    GLfloat dw = get_float( p, i + 24 );
    GLfloat dh = get_float( p, i + 28 );

    draw_image( p_ctx, p_id, sx, sy, sw, sh, dx, dy, dw, dh );

    i += 32;
  }

  return i;
}


// void* arc(NVGcontext* p_ctx, void* p_script)
// {
//   // bring sector data onto the stack
//   arc_sector_t sector = *(arc_sector_t*) p_script;

//   // clamp the angle to a circle
//   float angle = sector.finish - sector.start;
//   angle       = angle > TAU ? TAU : angle;
//   angle       = angle < -TAU ? -TAU : angle;

//   // calculate the number of segments
//   int   segment_count = log2(sector.radius) * fabsf(angle) * 2;
//   float increment     = angle / segment_count;
//   float a             = sector.start;

//   // don't draw anything if the angle is so small that the segment_count is zero
//   if (segment_count > 0)
//   {
//     // Arc starts on the perimeter. Sector starts in the center.
//     for (int i = 0; i <= segment_count; ++i)
//     {
//       float px, py;
//       px = sector.radius * cos(a);
//       py = sector.radius * sin(a);
//       if (i == 0)
//       {
//         nvgMoveTo(p_ctx, px, py);
//       }
//       else
//       {
//         nvgLineTo(p_ctx, px, py);
//       }
//       a += increment;
//     }
//     // arc doesn't close
//   }

//   return (void *)((char *)p_script + sizeof(arc_sector_t));
// }

// void* sector(NVGcontext* p_ctx, void* p_script)
// {
//   // bring sector data onto the stack
//   arc_sector_t sector = *(arc_sector_t*) p_script;

//   // clamp the angle to a circle
//   float angle = sector.finish - sector.start;
//   angle       = angle > TAU ? TAU : angle;
//   angle       = angle < -TAU ? -TAU : angle;

//   // calculate the number of segments
//   int   segment_count = log2(sector.radius) * fabsf(angle) * 2;
//   float increment     = angle / segment_count;
//   float a             = sector.start;

//   // don't draw anything if the angle is so small that the segment_count is zero
//   if (segment_count > 0)
//   {
//     // Arc starts on the perimeter. Sector starts in the center.
//     nvgMoveTo(p_ctx, 0, 0);

//     for (int i = 0; i <= segment_count; ++i)
//     {
//       float px, py;
//       px = sector.radius * cos(a);
//       py = sector.radius * sin(a);
//       nvgLineTo(p_ctx, px, py);
//       a += increment;
//     }
//     nvgClosePath(p_ctx);
//   }

//   return (void *)((char *)p_script + sizeof(arc_sector_t));
// }









//---------------------------------------------------------
void render_script( uint32_t id, NVGcontext* p_ctx ) {
  // get the script
  script_t* p_script = get_script( id );
  if ( !p_script ) {
    return;
  }

  // track the state pushes
  int push_count = 0;
  

  // setup
  void* p = p_script->p_data;
  int i = 0;

  while ( i < p_script->size ) {
    int op = get_uint16(p, i);
    int param = get_uint16(p, i + 2);
    i += 4;

    switch( op ) {
      case 0x01:        // draw_line
        nvgBeginPath(p_ctx);
        nvgMoveTo(p_ctx, get_float(p, i), get_float(p, i+4));
        nvgLineTo(p_ctx, get_float(p, i + 8), get_float(p, i + 12));
        if (param & 2) nvgStroke(p_ctx);
        i += 16;
        break;
      case 0x02:        // draw_triangle
        nvgBeginPath(p_ctx);
        nvgMoveTo(p_ctx, get_float(p, i), get_float(p, i+4));
        nvgLineTo(p_ctx, get_float(p, i + 8), get_float(p, i + 12));
        nvgLineTo(p_ctx, get_float(p, i + 16), get_float(p, i + 20));
        nvgClosePath(p_ctx);
        if (param & 1) nvgFill(p_ctx);
        if (param & 2) nvgStroke(p_ctx);
        i += 24;
        break;
      case 0x03:        // draw_quad
        nvgBeginPath(p_ctx);
        nvgMoveTo(p_ctx, get_float(p, i), get_float(p, i+4));
        nvgLineTo(p_ctx, get_float(p, i + 8), get_float(p, i + 12));
        nvgLineTo(p_ctx, get_float(p, i + 16), get_float(p, i + 20));
        nvgLineTo(p_ctx, get_float(p, i + 24), get_float(p, i + 28));
        nvgClosePath(p_ctx);
        if (param & 1) nvgFill(p_ctx);
        if (param & 2) nvgStroke(p_ctx);
        i += 32;
        break;
      case 0x04:        // draw_rect
        nvgBeginPath(p_ctx);
        nvgRect(p_ctx, 0, 0, get_float(p, i), get_float(p, i+4));
        if (param & 1) nvgFill(p_ctx);
        if (param & 2) nvgStroke(p_ctx);
        i += 8;
        break;
      case 0x05:        // draw_rrect
        nvgBeginPath(p_ctx);
        nvgRoundedRect(p_ctx, 0, 0, get_float(p, i), get_float(p, i + 4), get_float(p, i + 8));
        if (param & 1) nvgFill(p_ctx);
        if (param & 2) nvgStroke(p_ctx);
        i += 12;
        break;
      case 0x06:        // draw_arc
        nvgBeginPath(p_ctx);
        nvgArc(p_ctx,
          0, 0,
          get_float(p, i), 0, get_float(p, i + 4),
          get_float(p, i + 4) > 0 ? NVG_CW : NVG_CCW
        );
        if (param & 1) nvgFill(p_ctx);
        if (param & 2) nvgStroke(p_ctx);
        i += 8;
        break;
      case 0x07:        // draw_sector
        nvgBeginPath(p_ctx);
        nvgMoveTo(p_ctx, 0, 0);
        nvgLineTo(p_ctx, get_float(p, i), 0);
        nvgArc(p_ctx,
          0, 0,
          get_float(p, i), 0, get_float(p, i + 4),
          get_float(p, i + 4) > 0 ? NVG_CW : NVG_CCW
        );
        nvgClosePath(p_ctx);
        if (param & 1) nvgFill(p_ctx);
        if (param & 2) nvgStroke(p_ctx);
        i += 8;
        break;
      case 0x08:        // draw_circle
        nvgBeginPath(p_ctx);
        nvgCircle(p_ctx, 0, 0, get_float(p, i));
        if (param & 1) nvgFill(p_ctx);
        if (param & 2) nvgStroke(p_ctx);
        i += 4;
        break;
      case 0x09:        // draw_ellipse
        nvgBeginPath(p_ctx);
        nvgEllipse(p_ctx, 0, 0, get_float(p, i), get_float(p, i + 4));
        if (param & 1) nvgFill(p_ctx);
        if (param & 2) nvgStroke(p_ctx);
        i += 8;
        break;
      case 0x0A:        // draw_text - byte count is in param
        render_text( p + i, param, p_ctx );
        i += param;
        // skip any padding
        switch( param % 4 ) {
          case 0: break;
          case 1: i += 3; break;
          case 2: i += 2; break;
          case 3: i += 1; break;
          default: break;
        }
        break;







      case 0x0B:        // draw_sprites
        i = render_sprites( p_ctx, p, i, param );
        break;









      case 0x0F:        // render_script
        // fun with recursion
        render_script( param, p_ctx );
        break;

      case 0x20:        // begin_path
        nvgBeginPath(p_ctx);
        break;
      case 0x21:        // close_path
        nvgClosePath(p_ctx);
        break;
      case 0x22:        // fill
        nvgFill(p_ctx);
        break;
      case 0x23:        // stroke
        nvgStroke(p_ctx);
        break;

      case 0x26:        // move_to
        nvgMoveTo(p_ctx, get_float(p, i), get_float(p, i+4));
        i += 8;
        break;
      case 0x27:        // line_to
        nvgLineTo(p_ctx, get_float(p, i), get_float(p, i+4));
        i += 8;
        break;
      case 0x28:        // arc_to
        nvgArcTo(p_ctx,
          get_float(p, i), get_float(p, i+4),     // x1, y1
          get_float(p, i+8), get_float(p, i+12),  // x2, y2
          get_float(p, i+16)                      // radius
        );
        i += 20;
        break;
      case 0x29:        // bezier_to
        nvgBezierTo(p_ctx,
          get_float(p, i), get_float(p, i+4),     // c1x, c1y
          get_float(p, i+8), get_float(p, i+12),  // c2x, c2y
          get_float(p, i+16), get_float(p, i+20)  // x, y
        );
        i += 24;
        break;
      case 0x2A:        // quadratic_to
        nvgQuadTo(p_ctx,
          get_float(p, i), get_float(p, i+4),     // cx, cy
          get_float(p, i+8), get_float(p, i+12)   // x, y
        );
        i += 16;
        break;

      case 0x40:        // push_state
        push_count++;
        nvgSave(p_ctx);
        break;
      case 0x41:        // pop_state
        if ( push_count > 0 ) {
          push_count--;
          nvgRestore( p_ctx );
        }
        break;
      case 0x42:        // pop_push_state
        if ( push_count > 0 ) {
          push_count--;
          nvgRestore( p_ctx );
        }
        push_count++;
        nvgSave(p_ctx);
        break;

      // case 0x43:        // clear
      case 0x44:        // scissor
        nvgScissor(p_ctx, 0, 0, get_float(p, i), get_float(p, i+4));
        i += 8;
        break;

      case 0x50:        // transform
        nvgTransform(
          p_ctx,
          get_float(p, i), get_float(p, i+4), 
          get_float(p, i+8), get_float(p, i+12),
          get_float(p, i+16), get_float(p, i+20)
        );
        i += 24;
        break;
      case 0x51:        // scale
        nvgScale(p_ctx, get_float(p, i), get_float(p, i+4));
        i += 8;
        break;
      case 0x52:        // rotate
        nvgRotate(p_ctx, get_float(p, i));
        i += 4;
        break;
      case 0x53:        // translate
        nvgTranslate(p_ctx, get_float(p, i), get_float(p, i+4));
        i += 8;
        break;


      case 0x60:        // fill_color
        nvgFillColor(p_ctx, nvgRGBA(
          get_byte(p,i), get_byte(p,i+1), get_byte(p,i+2), get_byte(p,i+3)
        ));
        i += 4;
        break;
      case 0x61:        // fill_linear
        nvgFillPaint(p_ctx,
          nvgLinearGradient(
            p_ctx,
            get_float(p, i), get_float(p, i+4), get_float(p, i+8), get_float(p, i+12),
            nvgRGBA(get_byte(p,i+16), get_byte(p,i+17), get_byte(p,i+18), get_byte(p,i+19)),
            nvgRGBA(get_byte(p,i+20), get_byte(p,i+21), get_byte(p,i+22), get_byte(p,i+23))
          )
        );
        i += 24;
        break;
      case 0x62:        // fill_radial
        nvgFillPaint(p_ctx,
          nvgRadialGradient(
            p_ctx,
            get_float(p, i), get_float(p, i+4), get_float(p, i+8), get_float(p, i+12),
            nvgRGBA(get_byte(p,i+16), get_byte(p,i+17), get_byte(p,i+18), get_byte(p,i+19)),
            nvgRGBA(get_byte(p,i+20), get_byte(p,i+21), get_byte(p,i+22), get_byte(p,i+23))
          )
        );
        i += 24;
        break;
      case 0x63:        // fill_image
        set_fill_image(
          p_ctx,
          p + i
          // get_uint32(p, i + 0)     // driver id
        );
        i += 32;
      break; 

      case 0x70:        // stroke width
        nvgStrokeWidth(p_ctx, param / 4.0);
        break;
      case 0x71:        // stroke color
        nvgStrokeColor(p_ctx, nvgRGBA(
          get_byte(p,i), get_byte(p,i+1), get_byte(p,i+2), get_byte(p,i+3)
        ));
        i += 4;
        break;
      case 0x72:        // stroke_linear
        nvgStrokePaint(p_ctx,
          nvgLinearGradient(
            p_ctx,
            get_float(p, i), get_float(p, i+4), get_float(p, i+8), get_float(p, i+12),
            nvgRGBA(get_byte(p,i+16), get_byte(p,i+17), get_byte(p,i+18), get_byte(p,i+19)),
            nvgRGBA(get_byte(p,i+20), get_byte(p,i+21), get_byte(p,i+22), get_byte(p,i+23))
          )
        );
        i += 24;
        break;
      case 0x73:        // stroke_radial
        nvgStrokePaint(p_ctx,
          nvgRadialGradient(
            p_ctx,
            get_float(p, i), get_float(p, i+4), get_float(p, i+8), get_float(p, i+12),
            nvgRGBA(get_byte(p,i+16), get_byte(p,i+17), get_byte(p,i+18), get_byte(p,i+19)),
            nvgRGBA(get_byte(p,i+20), get_byte(p,i+21), get_byte(p,i+22), get_byte(p,i+23))
          )
        );
        i += 24;
        break;
      case 0x74:        // stroke_image
        set_stroke_image(
          p_ctx,
          p + i
          // get_uint32(p, i + 0)     // driver id
        );
        i += 32;
      break;



      case 0x80:        // line cap
        switch(param) {
          case 0x00: nvgLineCap(p_ctx, NVG_BUTT); break;
          case 0x01: nvgLineCap(p_ctx, NVG_ROUND); break;
          case 0x02: nvgLineCap(p_ctx, NVG_SQUARE); break;
        }
        break;
      case 0x81:        // line join
        switch(param) {
          case 0x00: nvgLineJoin(p_ctx, NVG_BEVEL); break;
          case 0x01: nvgLineJoin(p_ctx, NVG_ROUND); break;
          case 0x02: nvgLineJoin(p_ctx, NVG_MITER); break;
        }
        break;
      case 0x82:        // miter limit
        nvgMiterLimit(p_ctx, param);
        break;


      case 0x90:        // font
        set_font( p + i, p_ctx );
        i += param;
        // skip any padding
        switch( param % 4 ) {
          case 0: break;
          case 1: i += 3; break;
          case 2: i += 2; break;
          case 3: i += 1; break;
          default: break;
        }
        break;
      case 0x91:        // font_size
          nvgFontSize( p_ctx, param / 4.0 );
        // font_size = param / 4
        // ctx.font = `${font_size}px ${font}`
        break;
      case 0x92:        // text_align
        switch( param ) {
          case 0x00:    // left
            nvgTextAlignH(p_ctx, NVG_ALIGN_LEFT);
            break;
          case 0x01:    // center
            nvgTextAlignH(p_ctx, NVG_ALIGN_CENTER);
            break;
          case 0x02:    // right
            nvgTextAlignH(p_ctx, NVG_ALIGN_RIGHT);
            break;
        }
        break;
      case 0x93:        // text_base
        switch( param ) {
          case 0x00:    // top
            nvgTextAlignV(p_ctx, NVG_ALIGN_TOP);
            break;
          case 0x01:    // middle
            nvgTextAlignV(p_ctx, NVG_ALIGN_MIDDLE);
            break;
          case 0x02:    // alphabetic
            nvgTextAlignV(p_ctx, NVG_ALIGN_BASELINE);
            break;
          case 0x03:    // bottom
            nvgTextAlignV(p_ctx, NVG_ALIGN_BOTTOM);
            break;
        }
        break;


// static NVGstate* nvg__getState(NVGcontext* ctx)
// {
//   return &ctx->states[ctx->nstates-1];
// }

// void nvgTextAlign(NVGcontext* ctx, int align)
// {
//   NVGstate* state = nvg__getState(ctx);
//   state->textAlign = align;
// }


      default:
        put_msg_i( "Unknown OP: ", op );
        break;
    }
  }

  // if there are unbalanced pushes, clear them
  while ( push_count > 0 ) {
    push_count--;
    nvgRestore( p_ctx );
  }
}

