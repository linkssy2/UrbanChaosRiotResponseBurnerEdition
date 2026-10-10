#include "audio_parameter.h"
#include <math.h>
#include <string.h>
static uint16_t u16(const uint8_t*p){return(uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const uint8_t*p){return(uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
static int16_t s16(uint16_t x){return(int16_t)(x<32768?x:(int32_t)x-65536);}
static int equal(const char*a,const char*b){for(;;){unsigned x=(unsigned char)*a++,y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;if(!x)return 1;}}
int uc_audio_parameter_find(const void*raw,size_t size,const void*strings,size_t bytes,
 uint16_t sound,const char*name,UCAudioParameterCurve*out,int*found){
 const uint8_t*p=raw,*params=NULL,*maps=NULL,*points=NULL;size_t pn=0,mn=0,qn=0;
 if(!p||size<4||memcmp(p,"ampp",4)||!strings||!name||!out||!found)return 1;
 for(size_t at=4;at<size;){size_t n;if(size-at<8)return 1;n=u32(p+at+4);if(n>size-at-8||(n&1)>size-at-8-n)return 1;
  if(!memcmp(p+at,"ampp",4)){if(params)return 1;params=p+at+8;pn=n;}
  else if(!memcmp(p+at,"ampm",4)){if(maps)return 1;maps=p+at+8;mn=n;}
  else if(!memcmp(p+at,"ampq",4)){if(points)return 1;points=p+at+8;qn=n;}
  else return 1;at+=8+n+(n&1);}
 if(!params||!maps||!points||pn<4||mn<4||qn<4||pn!=4+(size_t)u32(params)*12||mn!=4+(size_t)u32(maps)*12||qn!=4+(size_t)u32(points)*4)return 1;
 for(uint32_t i=0;i<u32(params);++i){const uint8_t*r=params+4+(size_t)i*12;size_t offset=u16(r+8);if(u16(r)!=sound)continue;
  if(offset>=bytes||!memchr((const uint8_t*)strings+offset,0,bytes-offset))return 1;
  if(equal((const char*)strings+offset,name)){uint32_t map=u32(r+4);if(r[3]!=1||map>=u32(maps))return 1;const uint8_t*m=maps+4+(size_t)map*12;uint16_t count=u16(m+4);uint32_t first=u32(m+8);
   if(!count||u16(m+2)>2||first>u32(points)||count>u32(points)-first)return 1;
   UCAudioParameterCurve result={m,points+4+(size_t)first*4,count};*out=result;*found=1;return 0;}}
 *found=0;return 0;
}
int uc_audio_parameter_curve(const UCAudioParameterCurve*c,uint32_t count,float increment,float value,UCAudioParameterResult*out){
 uint32_t upper=0,lower;uint16_t a,b,bits;float x,scale=.00001525902189669642f;double wide;
 if(!c||!c->mapping||!c->points||!c->count||!out||!isfinite(increment)||!isfinite(value)||u16(c->mapping+2)>2)return 1;
 for(uint32_t i=1;i<c->count;++i)if(u16(c->points+i*4)<u16(c->points+(i-1)*4))return 1;
 wide=(double)count*(double)increment+(double)value;x=(float)wide;if(wide<0)x=0;else if(x>1)x=1;
 while(upper<c->count&&(float)u16(c->points+upper*4)*scale<=x)++upper;if(upper==c->count)--upper;lower=upper?upper-1:0;
 a=u16(c->points+lower*4);b=u16(c->points+upper*4);bits=u16(c->points+lower*4+2);
 if(a==b)bits=u16(c->points+upper*4+2);
 else {float d=(float)b*scale;float span=(float)(b-a)*scale;int32_t change=(int32_t)s16(u16(c->points+upper*4+2))-s16(bits);d-=x;d/=span;d=1.f-d;d*=(float)change;bits=(uint16_t)((int32_t)d+bits);}
 *out=(UCAudioParameterResult){u16(c->mapping),u16(c->mapping+2),s16(bits)};return 0;
}
