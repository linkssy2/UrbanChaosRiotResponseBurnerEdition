// Production cursor artwork plus capture transition checks. No visible window
// or game is launched; only the capture/focus Win32 calls below are substituted.
#undef NDEBUG
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <vector>
#include "native/runtime/menu_cursor.h"

static HWND game_window = reinterpret_cast<HWND>(uintptr_t(1));
static bool menu_mode = true, gameplay = false, captured = false, foreground = true;
static long dx = 0, dy = 0;
static HCURSOR observed_cursor = nullptr;
static bool clipped = false;
static RECT observed_clip{};
static bool focused() { return foreground; }
static bool graphics_open=false;
static bool TrialGraphicsOpen() { return graphics_open; }
// GENERATED_CURSOR

static BOOL TestGetClientRect(HWND, RECT* r) { *r = {0,0,1280,720}; return TRUE; }
static int TestMapWindowPoints(HWND, HWND, POINT* p, UINT count) {
    for (UINT i=0;i<count;++i) { p[i].x+=10; p[i].y+=20; } return 0;
}
static BOOL TestClipCursor(const RECT* r) { clipped=r!=nullptr; if(r)observed_clip=*r; return TRUE; }
static HCURSOR TestSetCursor(HCURSOR value) { HCURSOR prior=observed_cursor;observed_cursor=value;return prior; }
#define GetClientRect TestGetClientRect
#define MapWindowPoints TestMapWindowPoints
#define ClipCursor TestClipCursor
#define SetCursor TestSetCursor
// GENERATED_RECONCILE
#undef GetClientRect
#undef MapWindowPoints
#undef ClipCursor
#undef SetCursor

static void artwork() {
    NativeMenuCursor owner;
    HCURSOR cursor=owner.get(nullptr);
    assert(cursor && cursor==owner.get(nullptr));
    ICONINFO info{};assert(GetIconInfo(cursor,&info));
    BITMAP bitmap{};assert(GetObject(info.hbmColor,sizeof(bitmap),&bitmap));
    assert(!info.fIcon && bitmap.bmWidth>=32 && bitmap.bmWidth<=128);
    assert(info.xHotspot==DWORD(MulDiv(3,bitmap.bmWidth,32)));
    assert(info.yHotspot==DWORD(MulDiv(2,bitmap.bmWidth,32)));
    BITMAPINFO format{};format.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    format.bmiHeader.biWidth=bitmap.bmWidth;format.bmiHeader.biHeight=-bitmap.bmHeight;
    format.bmiHeader.biPlanes=1;format.bmiHeader.biBitCount=32;
    std::vector<uint32_t> pixels(size_t(bitmap.bmWidth)*bitmap.bmHeight);
    HDC dc=CreateCompatibleDC(nullptr);
    assert(GetDIBits(dc,info.hbmColor,0,bitmap.bmHeight,pixels.data(),&format,DIB_RGB_COLORS));
    unsigned solid=0,transparent=0,red=0;
    for(uint32_t pixel:pixels){
        if(pixel>>24)++solid;else ++transparent;
        if(((pixel>>16)&255)>180 && ((pixel>>8)&255)<70 && (pixel&255)<70)++red;
    }
    assert(solid && transparent && red);
    DeleteDC(dc);DeleteObject(info.hbmColor);DeleteObject(info.hbmMask);
}

int main() {
    artwork();
    ReconcileCapture();
    assert(!captured && !clipped && observed_cursor && observed_cursor!=LoadCursor(nullptr,IDC_ARROW));
    HCURSOR shield=observed_cursor;
    gameplay=true;menu_mode=false;dx=40;dy=-20;
    ReconcileCapture();
    assert(captured && clipped && !observed_cursor && !dx && !dy);
    assert(observed_clip.left==10 && observed_clip.top==20 && observed_clip.right==1290 && observed_clip.bottom==740);
    dx=12;dy=15;ReconcileCapture();assert(dx==12 && dy==15); // polling does not discard movement
    menu_mode=true;ReconcileCapture();assert(!captured && !clipped && observed_cursor==shield && !dx && !dy);
    menu_mode=false;ReconcileCapture();assert(captured && clipped && !observed_cursor);
    foreground=false;ReconcileCapture();assert(!captured && !clipped && observed_cursor==LoadCursor(nullptr,IDC_ARROW));
    foreground=true;ReconcileCapture();assert(captured && clipped && !observed_cursor);
    graphics_open=true;ReconcileCapture();assert(!captured && !clipped && observed_cursor==shield);
    graphics_open=false;ReconcileCapture();assert(captured && clipped && !observed_cursor);
    menu_mode=true;ReconcileCapture();assert(!captured && observed_cursor==shield);
    std::puts("existing shield artwork/hotspot/cache and startup, gameplay, pause, resume, focus-loss transitions passed");
}
