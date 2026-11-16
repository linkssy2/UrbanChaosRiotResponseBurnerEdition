#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

// An OS cursor stays responsive independently of the game's frame rate.
// A compact ember pointer: charred outline, orange flame and dark inner core.
// The sharp upper-left flame tip marks the click hotspot.
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
        SetDCPenColor(dc,RGB(15,12,10));
        bool drawn=polygon({{3,2},{24,16},{17,17},{23,27},{17,30},{12,21},{6,26}},RGB(220,69,17));
        SetDCPenColor(dc,RGB(255,158,42));
        drawn=polygon({{4,4},{20,15},{14,15},{19,24},{16,25},{11,17},{7,21}},RGB(255,158,42))&&drawn;
        SetDCPenColor(dc,RGB(51,25,16));
        drawn=polygon({{7,10},{15,15},{12,16},{18,26},{17,27},{10,18},{8,21}},RGB(51,25,16))&&drawn;
        SetDCPenColor(dc,RGB(255,220,126));
        drawn=polygon({{4,4},{11,10},{8,10},{6,15}},RGB(255,220,126))&&drawn;
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
        const int size=MulDiv(std::clamp(GetSystemMetricsForDpi(SM_CXCURSOR,dpi?dpi:96),32,128),3,4);
        for(const auto &cursor:cursors_)if(cursor.size==size)return cursor.handle;
        HCURSOR handle=create(size);try{cursors_.push_back({size,handle});}catch(...){DestroyCursor(handle);throw;}return handle;
    }
};
