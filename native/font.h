#ifndef UC_NATIVE_FONT_H
#define UC_NATIVE_FONT_H
#include <stddef.h>
#include <stdint.h>
typedef struct UCFont {
    const uint8_t *mapping,*glyphs;uint32_t maximum,count;
    float height,line_height,baseline;
} UCFont;
typedef struct UCFontQuad {float x0,y0,x1,y1,u0,v0,u1,v1;}UCFontQuad;
/*Version5 ABC, borrowed immutable bytes. No font substitutions.*/
int uc_font_read(const void *,size_t,UCFont *);
/*171A0/18280 ordinary printable ASCII, flags10 word wrapping. No button/dynamic
 *glyphs, uppercase conversion, outline/blur, alternate alignment or clipping.
 *Terminal spaces are rejected: the source wrap branch can run past their NUL.
 *Finite pixel inputs, scale>0; caller supplies capacity for every byte. Quads
 *are the original65170 corner/UV inputs, including its half-pixel floor rule.*/
int uc_font_layout(const UCFont *,const char *,float x,float y,float scale,
    float left,float right,int fractional,UCFontQuad *,size_t,size_t *,float *height);
/*18A70 admitted text policies: flags4 uses the caller's explicit scale without
 * wrapping; flags16 wraps (caller supplies source default scale1). Neither
 * selects horizontal/vertical alignment. Other flag combinations fail. */
int uc_font_layout_flags(const UCFont *,const char *,float x,float y,float scale,
    float left,float right,uint32_t flags,int fractional,UCFontQuad *,size_t,size_t *,float *height);
typedef struct UCFontDraw {UCFontQuad quad;uint32_t argb;} UCFontDraw;
/*18280 notification blur: call per already laid-out glyph, retaining glyph
 * then pass order. Offsets follow pixel flooring; color is source ARGB. The
 * flags4/16 notification domain does not include bit8's separate outline.*/
int uc_font_blur_quad(const UCFontQuad *,float blur,uint32_t argb,UCFontDraw out[4],size_t *count);
#endif
