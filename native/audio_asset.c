#include "audio_asset.h"
#include <stdlib.h>
#include <string.h>

static uint16_t u16(const uint8_t *p) {return (uint16_t)(p[0] | (uint16_t)p[1]<<8);}
static uint32_t u32(const uint8_t *p) {return (uint32_t)u16(p) | (uint32_t)u16(p+2)<<16;}
static int tag(const uint8_t *p,const char *s) {return !memcmp(p,s,4);}
static int clamp(int x,int lo,int hi) {return x<lo?lo:x>hi?hi:x;}
static const int steps[89]={
 7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,
 73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,
 449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,
 2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,
 7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,
 24623,27086,29794,32767};
static void block(const uint8_t *p,uint32_t channels,int16_t *out) {
 static const int changes[8]={-1,-1,-1,-1,2,4,6,8};
 uint32_t ch,n;
 for(ch=0;ch<channels;ch++) {
  int predictor=(int16_t)u16(p+ch*4),index=p[ch*4+2];
  out[ch]=(int16_t)predictor;
  /* Current compatibility APU consumes predictor + 63 nibbles per block. */
  for(n=0;n<63;n++) {
   unsigned packed=p[channels*4+(n/8)*channels*4+ch*4+(n%8)/2];
   unsigned code=(packed>>((n&1)*4))&15;
   int step=steps[index],delta=(step>>3)+((code&1)?step>>2:0)+
    ((code&2)?step>>1:0)+((code&4)?step:0);
   predictor=clamp(predictor+((code&8)?-delta:delta),-32768,32767);
   index=clamp(index+changes[code&7],0,88);
   out[(n+1)*channels+ch]=(int16_t)predictor;
  }
 }
}
void uc_audio_asset_free(UCAudioAsset *a) {
 if(a){free(a->samples);memset(a,0,sizeof(*a));}
}
static UCAudioStatus asset_load(const void *source,size_t size,UCAudioAsset *out,int stream) {
 const uint8_t *p=(const uint8_t *)source,*fmt=NULL,*data=NULL,*fact=NULL,*wsmp=NULL;
 uint32_t bytes=0,block_bytes,ch;size_t at,wsmp_size=0,fmt_size=0,blocks,i;
 unsigned seen=0,chunks=0;UCAudioAsset a={0};
 if(!out)return UC_AUDIO_ARGUMENT;
 memset(out,0,sizeof(*out));
 if(!p)return UC_AUDIO_ARGUMENT;
 if(size>64u*1024u*1024u)return UC_AUDIO_LIMIT;
 if(size<12 || !tag(p,"RIFF") || u32(p+4)!=size-8)return UC_AUDIO_FORMAT;
 if(!tag(p+8,stream?"WAVS":"WAVE"))return UC_AUDIO_UNSUPPORTED;
 for(at=12;at<size;) {
  const uint8_t *v;uint32_t n;unsigned bit=0;
  if(size-at<8)return UC_AUDIO_FORMAT;
  n=u32(p+at+4);v=p+at+8;
  if(n>size-at-8 || (n&1)>size-at-8-n)return UC_AUDIO_FORMAT;
  if(++chunks>64)return UC_AUDIO_LIMIT;
  if(tag(p+at,"fmt ")){bit=1;fmt=v;fmt_size=n;}
  else if(tag(p+at,"data")){bit=2;data=v;bytes=n;}
  else if(tag(p+at,"fact")){bit=4;fact=v;if(n!=4)return UC_AUDIO_FORMAT;}
  else if(tag(p+at,"wsmp")){bit=8;wsmp=v;wsmp_size=n;}
  else if(tag(p+at,"guid")){bit=16;if(n!=16)return UC_AUDIO_FORMAT;}
  else if(tag(p+at,"name")){bit=32;if(!n || !memchr(v,0,n))return UC_AUDIO_FORMAT;}
  else if(stream && tag(p+at,"pad ")){bit=64;}
  else if(stream && tag(p+at,"pdat")){bit=128;} /* Validated duplicate prefix by stream loader. */
  else return UC_AUDIO_UNSUPPORTED;
  if(seen&bit)return UC_AUDIO_FORMAT;
  seen|=bit;at+=8+(size_t)n+(n&1);
 }
 if(!fmt || !data || !fact || fmt_size<20)return UC_AUDIO_FORMAT;
 ch=u16(fmt+2);block_bytes=36*ch;
 if(fmt_size!=20 || u16(fmt)!=0x69 || (ch!=1 && ch!=2) ||
    u16(fmt+12)!=block_bytes || u16(fmt+14)!=4 || u16(fmt+16)!=2 || u16(fmt+18)!=64)
  return UC_AUDIO_UNSUPPORTED;
 if(!u32(fmt+4) || u32(fmt+4)>192000 || !bytes || bytes%block_bytes)return UC_AUDIO_FORMAT;
 blocks=bytes/block_bytes;
 if(blocks>(64u*1024u*1024u)/(64*ch*sizeof(int16_t)))return UC_AUDIO_LIMIT;
 a.sample_rate=u32(fmt+4);a.channels=ch;a.frame_count=(uint32_t)(blocks*64);
 a.authored_frames=u32(fact);a.encoded_bytes=bytes;
 if(!a.authored_frames || a.authored_frames>a.frame_count || a.frame_count-a.authored_frames>=64)
  return UC_AUDIO_FORMAT;
 if(wsmp) {
  if(wsmp_size<20)return UC_AUDIO_FORMAT;
  if(u32(wsmp)!=20 || u32(wsmp+16)!=1 || wsmp_size!=36 ||
     u32(wsmp+20)!=16 || u32(wsmp+24)!=0)return UC_AUDIO_UNSUPPORTED;
  a.loop_start_bytes=u32(wsmp+28);a.loop_length_bytes=u32(wsmp+32);
  if(!a.loop_length_bytes || a.loop_start_bytes>bytes ||
     a.loop_length_bytes>bytes-a.loop_start_bytes ||
     a.loop_start_bytes%block_bytes || a.loop_length_bytes%block_bytes)return UC_AUDIO_FORMAT;
  a.loop_start_frame=a.loop_start_bytes/block_bytes*64;
  a.loop_frame_count=a.loop_length_bytes/block_bytes*64;
 }
 for(i=0;i<blocks;i++)for(ch=0;ch<a.channels;ch++)
  if(data[i*block_bytes+ch*4+2]>88 || data[i*block_bytes+ch*4+3])return UC_AUDIO_FORMAT;
 a.samples=(int16_t *)malloc((size_t)a.frame_count*a.channels*sizeof(int16_t));
 if(!a.samples)return UC_AUDIO_MEMORY;
 for(i=0;i<blocks;i++)block(data+i*block_bytes,a.channels,a.samples+i*64*a.channels);
 *out=a;return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_asset_load(const void *source,size_t size,UCAudioAsset *out) {
 return asset_load(source,size,out,0);
}
typedef struct StreamDescriptor {
 UCAudioStreamInfo info;
 const uint8_t *guid,*fmt,*fact,*name,*prefix,*wsmp;uint32_t name_bytes,prefix_bytes;
} StreamDescriptor;
static UCAudioStatus stream_descriptor(const void *source,size_t size,StreamDescriptor *d) {
 const uint8_t *p=(const uint8_t *)source;size_t at;unsigned seen=0;
 memset(d,0,sizeof(*d));
 if(!p)return UC_AUDIO_ARGUMENT;
 if(size>65536)return UC_AUDIO_LIMIT;
 if(size<12 || !tag(p,"RIFF") || u32(p+4)!=size-8)return UC_AUDIO_FORMAT;
 if(!tag(p+8,"WAVS"))return UC_AUDIO_UNSUPPORTED;
 for(at=12;at<size;) {
  const uint8_t *v;uint32_t n;unsigned bit;
  if(size-at<8)return UC_AUDIO_FORMAT;
  n=u32(p+at+4);v=p+at+8;
  if(n>size-at-8 || (n&1)>size-at-8-n)return UC_AUDIO_FORMAT;
  if(tag(p+at,"guid")){bit=1;if(n!=16)return UC_AUDIO_FORMAT;d->guid=v;}
  else if(tag(p+at,"fmt ")){bit=2;if(n!=20)return UC_AUDIO_UNSUPPORTED;d->fmt=v;}
  else if(tag(p+at,"fact")){bit=4;if(n!=4)return UC_AUDIO_FORMAT;d->fact=v;}
  else if(tag(p+at,"name")){bit=8;if(!n||!memchr(v,0,n))return UC_AUDIO_FORMAT;d->name=v;d->name_bytes=n;}
  else if(tag(p+at,"strm")){bit=16;if(n!=8)return UC_AUDIO_FORMAT;
   d->info.data_offset=u32(v);d->info.encoded_bytes=u32(v+4);}
  else if(tag(p+at,"rsaf")){bit=32;if(n!=8)return UC_AUDIO_FORMAT;
   d->info.file_offset=u32(v);d->info.file_bytes=u32(v+4);}
  else if(tag(p+at,"pdat")){bit=64;if(!n)return UC_AUDIO_FORMAT;d->prefix=v;d->prefix_bytes=n;}
  else if(tag(p+at,"wsmp")){bit=128;if(n!=36)return UC_AUDIO_UNSUPPORTED;d->wsmp=v;}
  else return UC_AUDIO_UNSUPPORTED;
  if(seen&bit)return UC_AUDIO_FORMAT;
  seen|=bit;at+=8+(size_t)n+(n&1);
 }
 if((seen&63)!=63 || d->prefix_bytes>d->info.encoded_bytes || d->info.data_offset<20 || !d->info.encoded_bytes ||
    d->info.file_bytes<d->info.data_offset ||
    d->info.encoded_bytes>d->info.file_bytes-d->info.data_offset ||
    d->info.file_offset>UINT32_MAX-d->info.file_bytes)return UC_AUDIO_FORMAT;
 if(d->info.file_bytes>64u*1024u*1024u)return UC_AUDIO_LIMIT;
 return UC_AUDIO_OK;
}
UCAudioStatus uc_audio_stream_info(const void *source,size_t size,UCAudioStreamInfo *out) {
 StreamDescriptor d;UCAudioStatus status;
 if(!out)return UC_AUDIO_ARGUMENT;
 memset(out,0,sizeof(*out));status=stream_descriptor(source,size,&d);
 if(status==UC_AUDIO_OK)*out=d.info;
 return status;
}
UCAudioStatus uc_audio_stream_asset_load(const void *source,size_t size,
 const void *span,size_t span_size,UCAudioAsset *out) {
 StreamDescriptor d;const uint8_t *p=(const uint8_t *)span;size_t end,at;
 UCAudioStatus status;unsigned seen=0;
 if(!out)return UC_AUDIO_ARGUMENT;
 memset(out,0,sizeof(*out));status=stream_descriptor(source,size,&d);
 if(status!=UC_AUDIO_OK)return status;
 if(!p)return UC_AUDIO_ARGUMENT;
 if(span_size!=d.info.file_bytes || span_size<12 || !tag(p,"RIFF") || !tag(p+8,"WAVS"))return UC_AUDIO_FORMAT;
 end=(size_t)u32(p+4)+8;if(end<12||end>span_size)return UC_AUDIO_FORMAT;
 for(at=12;at<end;) {
  const uint8_t *v,*expected=NULL;uint32_t n,want=0;unsigned bit;
  if(end-at<8)return UC_AUDIO_FORMAT;
  n=u32(p+at+4);v=p+at+8;
  if(n>end-at-8 || (n&1)>end-at-8-n)return UC_AUDIO_FORMAT;
  if(tag(p+at,"guid")){bit=1;expected=d.guid;want=16;}
  else if(tag(p+at,"fmt ")){bit=2;expected=d.fmt;want=20;}
  else if(tag(p+at,"fact")){bit=4;expected=d.fact;want=4;}
  else if(tag(p+at,"name")){bit=8;expected=d.name;want=d.name_bytes;}
  else if(tag(p+at,"data")){bit=16;if(at+8!=d.info.data_offset||n!=d.info.encoded_bytes)return UC_AUDIO_FORMAT;}
  else if(tag(p+at,"pad ")){bit=32;}
  else if(tag(p+at,"pdat")){bit=64;expected=d.prefix;want=d.prefix_bytes;
   if(!expected||n!=want)return UC_AUDIO_FORMAT;}
  else if(tag(p+at,"wsmp")){bit=128;expected=d.wsmp;want=36;if(!expected)return UC_AUDIO_FORMAT;}
  else return UC_AUDIO_UNSUPPORTED;
  if((seen&bit)||(expected&&(n!=want||memcmp(v,expected,n))))return UC_AUDIO_FORMAT;
  seen|=bit;at+=8+(size_t)n+(n&1);
 }
 if((seen&31)!=31||!!(seen&64)!=!!d.prefix||!!(seen&128)!=!!d.wsmp)return UC_AUDIO_FORMAT;
 /* PC preload uses the full data chunk once. The optional pdat is a verified
  * duplicate prefix, never prepended (which would repeat the attack). */
 if(d.prefix_bytes&&memcmp(d.prefix,p+d.info.data_offset,d.prefix_bytes))return UC_AUDIO_FORMAT;
 return asset_load(p,end,out,1);
}
