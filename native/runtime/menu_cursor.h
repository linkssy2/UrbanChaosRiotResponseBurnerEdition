#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

// An OS cursor stays responsive independently of the game's frame rate.
// Miniature riot shield: steel rim, dark body and glazed viewport. The red
// upper-left rim marks the click hotspot; no conventional arrow is overlaid.
class NativeMenuCursor {
    struct Cached {int size;HCURSOR handle;};
    std::vector<Cached> cursors_;
    static HCURSOR create(int size){
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth=size;info.bmiHeader.biHeight=-size;
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        void *bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);
        HBITMAP color=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
        const int stride=((size+15)/16)*2;
        std::vector<uint8_t> mask_bits(size_t(stride)*size,0);
        HBITMAP mask=CreateBitmap(size,size,1,1,mask_bits.data());
        if(!dc||!color||!bits||!mask){if(mask)DeleteObject(mask);if(color)DeleteObject(color);if(dc)DeleteDC(dc);throw std::runtime_error("Menu cursor allocation failed");}
        HGDIOBJ old=SelectObject(dc,color);ZeroMemory(bits,size_t(size)*size*4);
        auto polygon=[&](std::initializer_list<POINT> shape,COLORREF fill){
            std::vector<POINT> points(shape);for(auto &point:points){point.x=MulDiv(point.x,size,32);point.y=MulDiv(point.y,size,32);}
            SetDCBrushColor(dc,fill);return Polygon(dc,points.data(),(int)points.size())!=FALSE;
        };
        HGDIOBJ old_pen=SelectObject(dc,GetStockObject(DC_PEN)),old_brush=SelectObject(dc,GetStockObject(DC_BRUSH));
        SetDCPenColor(dc,RGB(12,12,12));
        bool drawn=polygon({{3,2},{23,2},{28,7},{27,23},{22,28},{14,31},{6,27},{3,22}},RGB(185,191,188));
        SetDCPenColor(dc,RGB(83,91,94));
        drawn=polygon({{5,4},{22,4},{26,8},{25,22},{20,26},{14,29},{8,25},{5,21}},RGB(43,51,55))&&drawn;
        SetDCPenColor(dc,RGB(12,18,20));
        drawn=polygon({{7,7},{22,7},{24,9},{23,16},{7,16}},RGB(89,119,126))&&drawn;
        SetDCPenColor(dc,RGB(155,185,185));
        drawn=polygon({{9,9},{19,9},{13,13},{9,13}},RGB(155,185,185))&&drawn;
        SetDCPenColor(dc,RGB(220,40,0));
        drawn=polygon({{3,2},{10,2},{10,4},{5,4},{5,8},{3,8}},RGB(220,40,0))&&drawn;
        drawn=polygon({{9,25},{14,27},{20,24},{20,26},{14,29},{9,27}},RGB(220,40,0))&&drawn;
        HFONT font=CreateFontW(-MulDiv(6,size,32),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FIXED_PITCH,L"Consolas");
        if(font){
            HGDIOBJ old_font=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(244,244,231));
            RECT label{MulDiv(6,size,32),MulDiv(17,size,32),MulDiv(25,size,32),MulDiv(24,size,32)};
            drawn=DrawTextW(dc,L"POLICE",6,&label,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX)!=0&&drawn;
            SelectObject(dc,old_font);DeleteObject(font);
        }else drawn=false;
        GdiFlush();
        auto pixels=static_cast<uint32_t*>(bits);
        for(size_t i=0;i<size_t(size)*size;i++)if(pixels[i]&0xffffff) pixels[i]|=0xff000000u;
        SelectObject(dc,old_pen);SelectObject(dc,old_brush);SelectObject(dc,old);
        ICONINFO icon{};icon.xHotspot=MulDiv(3,size,32);icon.yHotspot=MulDiv(2,size,32);icon.hbmColor=color;icon.hbmMask=mask;
        HCURSOR cursor=drawn?static_cast<HCURSOR>(CreateIconIndirect(&icon)):nullptr;
        DeleteObject(mask);DeleteObject(color);DeleteDC(dc);
        if(!cursor)throw std::runtime_error("Menu cursor creation failed");return cursor;
    }
public:
    ~NativeMenuCursor(){for(const auto &cursor:cursors_)DestroyCursor(cursor.handle);}
    HCURSOR get(HWND window){
        const UINT dpi=window?GetDpiForWindow(window):GetDpiForSystem();
        const int size=std::clamp(GetSystemMetricsForDpi(SM_CXCURSOR,dpi?dpi:96),32,128);
        for(const auto &cursor:cursors_)if(cursor.size==size)return cursor.handle;
        HCURSOR handle=create(size);try{cursors_.push_back({size,handle});}catch(...){DestroyCursor(handle);throw;}return handle;
    }
};
