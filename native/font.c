#include "font.h"
#include <math.h>
#include <string.h>
static uint16_t u16(const uint8_t*p){return p[0]|(uint16_t)p[1]<<8;}
static uint32_t u32(const uint8_t*p){return u16(p)|(uint32_t)u16(p+2)<<16;}
static float f32(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
int uc_font_read(const void*bytes,size_t size,UCFont*out){
    const uint8_t*p=bytes;if(!p||!out||size<34||u16(p)!=5||u16(p+2)!=1)return 1;
    UCFont f={0};f.height=f32(p+4);f.line_height=f32(p+16);f.baseline=f32(p+20);f.maximum=u16(p+28);
    size_t end=30+(size_t)(f.maximum+1)*2;if(end>size-4)return 1;f.mapping=p+30;f.count=u32(p+end);end+=4;
    if(!f.count||f.count>65536||size-end!=(size_t)f.count*24||!isfinite(f.height)||f.height<=0||
       !isfinite(f.line_height)||f.line_height<=0||!isfinite(f.baseline))return 1;
    f.glyphs=p+end;for(uint32_t i=0;i<=f.maximum;i++)if(u16(f.mapping+i*2)>=f.count)return 1;
    for(uint32_t i=0;i<f.count;i++){const uint8_t*g=f.glyphs+i*24;
        for(unsigned j=0;j<4;j++)if(!isfinite(f32(g+j*4))||f32(g+j*4)<0||f32(g+j*4)>1)return 1;
        if(f32(g)>f32(g+8)||f32(g+4)>f32(g+12)||(int16_t)u16(g+18)<0)return 1;}
    *out=f;return 0;
}
static const uint8_t*glyph(const UCFont*f,unsigned c){return f->glyphs+24*(c>f->maximum?0:u16(f->mapping+c*2));}
static float advance(const uint8_t*g,float scale){return ((float)(int16_t)u16(g+20)+(float)(int16_t)u16(g+16))*scale;}
int uc_font_layout_flags(const UCFont*f,const char*text,float x,float y,float scale,float left,float right,uint32_t flags,int fractional,
    UCFontQuad*out,size_t capacity,size_t*count,float*height){
    if(!f||!text||!out||!count||!height||!isfinite(x)||!isfinite(y)||!isfinite(scale)||scale<=0||
       !isfinite(left)||!isfinite(right)||right<left||(flags!=4&&flags!=16))return 1;
    size_t length=strlen(text);if(length>capacity||length>4096||(length&&text[length-1]==' '))return 1;
    for(size_t i=0;i<length;i++)if((uint8_t)text[i]<32||(uint8_t)text[i]>126)return 1;
    size_t n=0;float h=0;
    for(size_t i=0;i<length;i++){
        unsigned c=(uint8_t)text[i];
        if(flags==16&&c==' '&&text[i+1]!='?'){
            float end=x+advance(glyph(f,' '),scale);
            for(size_t j=i+1;j<length;j++){
                if(text[j]==' '&&text[j+1]!='?')break;
                end=advance(glyph(f,(uint8_t)text[j]),scale)+end;
            }
            if(end>right){x=left;float dy=f->line_height*scale;y+=dy;h+=dy;c=(uint8_t)text[++i];}
        }
        const uint8_t*g=glyph(f,c);x+=(float)(int16_t)u16(g+16)*scale;
        if(c!=' '){float px=x-.5f,py=y-.5f,bottom=py+f->height*scale;
            if(!fractional){px=floorf(px);py=floorf(py);bottom=floorf(bottom);}
            out[n++]=(UCFontQuad){px,py,px+(float)(int16_t)u16(g+18)*scale,bottom,f32(g),f32(g+4),f32(g+8),f32(g+12)};}
        x+=(float)(int16_t)u16(g+20)*scale;
    }
    *count=n;*height=h;return 0;
}
int uc_font_layout(const UCFont*f,const char*text,float x,float y,float scale,float left,float right,int fractional,
    UCFontQuad*out,size_t capacity,size_t*count,float*height){
    return uc_font_layout_flags(f,text,x,y,scale,left,right,16,fractional,out,capacity,count,height);
}
int uc_font_blur_quad(const UCFontQuad*q,float blur,uint32_t color,UCFontDraw out[4],size_t*count){
    if(!q||!out||!count||!isfinite(blur)||blur<0||blur>1000000)return 1;
    unsigned n=1;float spread=0;
    if(blur!=0){
        double extent=(double)blur*.0333333350718021392822265625;spread=(float)extent;
        n=(unsigned)(int32_t)(extent*.5);if(n>=4)n=4;else if(n<=1)n=1;
        float factor=1.f/(float)n;if(factor<.1f)factor=.1f;
        uint32_t packed=0;for(unsigned i=0;i<4;i++){
            float channel=(float)((color>>(i*8))&255u)*.0039215688593685626983642578125f;
            if(i==3)channel*=factor;int value=(int)(channel*255.f);if(value>255)value=255;else if(value<0)value=0;
            packed|=(uint32_t)value<<(i*8);
        }color=packed;
    }
    for(unsigned i=0;i<n;i++){
        double offset=(double)i/(float)n*spread;out[i]=(UCFontDraw){*q,color};
        out[i].quad.x0=(float)((double)q->x0+offset);out[i].quad.x1=(float)((double)q->x1+offset);
    }
    *count=n;return 0;
}
