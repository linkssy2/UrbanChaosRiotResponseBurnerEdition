/* Reuse CONTROL's key artwork; this pure GDI helper has no input/runtime hooks. */
#include <stdio.h>
#include <stdlib.h>
#pragma warning(push)
#pragma warning(disable: 4456 4505 4996) /* Unchanged CONTROL artwork header. */
#include "../../third_party/xboxrecomp/src/nv2a/nv2a_pc_hud_art.h"
#pragma warning(pop)
int native_hud_keycap(uint32_t *pixels,unsigned key,const char *label,const char *caption){
    const unsigned width=caption?384:128;
    BITMAPINFO info={0};void *bits=NULL;
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=(LONG)width;
    info.bmiHeader.biHeight=-128;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    HDC dc=CreateCompatibleDC(NULL);
    HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,NULL,0);
    if(!dc||!bitmap||!bits){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);return 0;}
    HGDIOBJ old=SelectObject(dc,bitmap);memset(bits,0,width*128*4);
    pc_hud_icon(dc,0,0,key,label);
    if(caption){
        HFONT font=CreateFontA(-48,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Arial");
        HGDIOBJ prior=SelectObject(dc,font);RECT bounds={136,16,384,112};
        SetTextColor(dc,RGB(237,237,220));SetBkMode(dc,TRANSPARENT);
        DrawTextA(dc,caption,-1,&bounds,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        SelectObject(dc,prior);DeleteObject(font);
    }
    GdiFlush();
    for(unsigned i=0;i<width*128;++i){uint32_t c=((uint32_t*)bits)[i];
        pixels[i]=(c&0xffffffu)|(c&0xffffffu?0xff000000u:0);}
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return 1;
}
