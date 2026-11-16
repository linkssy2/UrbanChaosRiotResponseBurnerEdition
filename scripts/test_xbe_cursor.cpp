// Production cursor artwork plus capture transition checks. No visible window
// or game is launched; only the capture/focus Win32 calls below are substituted.
#undef NDEBUG
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <vector>
static UINT cursor_dpi=96;
static UINT TestSystemDpi(){return cursor_dpi;}
static UINT TestWindowDpi(HWND){return cursor_dpi;}
#define GetDpiForSystem TestSystemDpi
#define GetDpiForWindow TestWindowDpi
#include "native/runtime/menu_cursor.h"
#undef GetDpiForSystem
#undef GetDpiForWindow

static HWND game_window = reinterpret_cast<HWND>(uintptr_t(1));
static bool menu_mode = true, gameplay = false, captured = false, foreground = true;
static long dx = 0, dy = 0;
static HCURSOR observed_cursor = nullptr;
static bool clipped = false;
static RECT observed_clip{};
static bool focused() { return foreground; }
static bool graphics_open=false;
static bool keyboard_active=true;
static int xbox_KeyboardMode(){return keyboard_active;}
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

static void artwork(NativeMenuCursor& owner) {
    HCURSOR cursor=owner.get(nullptr);
    assert(cursor && cursor==owner.get(nullptr));
    ICONINFO info{};assert(GetIconInfo(cursor,&info));
    BITMAP bitmap{};assert(GetObject(info.hbmColor,sizeof(bitmap),&bitmap));
    const UINT dpi=cursor_dpi;
    const int expected=MulDiv(std::clamp(GetSystemMetricsForDpi(SM_CXCURSOR,dpi?dpi:96),32,128),3,4);
    assert(!info.fIcon && bitmap.bmWidth>=24 && bitmap.bmWidth<=96 && bitmap.bmWidth==expected);
    assert(info.xHotspot==DWORD(MulDiv(3,bitmap.bmWidth,32)));
    assert(info.yHotspot==DWORD(MulDiv(2,bitmap.bmWidth,32)));
    BITMAPINFO format{};format.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    format.bmiHeader.biWidth=bitmap.bmWidth;format.bmiHeader.biHeight=-bitmap.bmHeight;
    format.bmiHeader.biPlanes=1;format.bmiHeader.biBitCount=32;
    std::vector<uint32_t> pixels(size_t(bitmap.bmWidth)*bitmap.bmHeight);
    HDC dc=CreateCompatibleDC(nullptr);
    assert(GetDIBits(dc,info.hbmColor,0,bitmap.bmHeight,pixels.data(),&format,DIB_RGB_COLORS));
    unsigned solid=0,transparent=0,red=0,amber=0,dark=0;
    for(uint32_t pixel:pixels){
        if(pixel>>24)++solid;else ++transparent;
        if(((pixel>>16)&255)>180 && ((pixel>>8)&255)<70 && (pixel&255)<70)++red;
        if(((pixel>>16)&255)>230 && ((pixel>>8)&255)>120 && (pixel&255)<150)++amber;
        if((pixel>>24) && ((pixel>>16)&255)<70 && ((pixel>>8)&255)<70 && (pixel&255)<70)++dark;
    }
    assert(solid && transparent && red && amber && dark);
    assert(pixels[info.yHotspot*bitmap.bmWidth+info.xHotspot]>>24);
    DeleteDC(dc);DeleteObject(info.hbmColor);DeleteObject(info.hbmMask);
}

int main() {
    NativeMenuCursor owner;
    for(UINT dpi:{96u,120u,144u,192u,288u,384u,96u,0u}){cursor_dpi=dpi;artwork(owner);}
    cursor_dpi=96;
    ReconcileCapture();
    assert(!captured && !clipped && observed_cursor && observed_cursor!=LoadCursor(nullptr,IDC_ARROW));
    HCURSOR ember=observed_cursor;
    gameplay=true;menu_mode=false;dx=40;dy=-20;
    ReconcileCapture();
    assert(captured && clipped && !observed_cursor && !dx && !dy);
    assert(observed_clip.left==10 && observed_clip.top==20 && observed_clip.right==1290 && observed_clip.bottom==740);
    dx=12;dy=15;ReconcileCapture();assert(dx==12 && dy==15); // polling does not discard movement
    menu_mode=true;ReconcileCapture();assert(!captured && !clipped && observed_cursor==ember && !dx && !dy);
    menu_mode=false;ReconcileCapture();assert(captured && clipped && !observed_cursor);
    foreground=false;ReconcileCapture();assert(!captured && !clipped && observed_cursor==LoadCursor(nullptr,IDC_ARROW));
    foreground=true;ReconcileCapture();assert(captured && clipped && !observed_cursor);
    graphics_open=true;ReconcileCapture();assert(!captured && !clipped && observed_cursor==ember);
    graphics_open=false;ReconcileCapture();assert(captured && clipped && !observed_cursor);
    menu_mode=true;ReconcileCapture();assert(!captured && observed_cursor==ember);
    keyboard_active=false;ReconcileCapture();assert(!captured&&!clipped&&!observed_cursor);
    graphics_open=true;ReconcileCapture();assert(!observed_cursor);
    keyboard_active=true;ReconcileCapture();assert(observed_cursor==ember);
    keyboard_active=false;foreground=false;ReconcileCapture();assert(observed_cursor==LoadCursor(nullptr,IDC_ARROW));
    std::puts("Burner ember artwork at 96-384 DPI, fallback DPI, hotspot/cache and startup, gameplay, pause, resume, focus-loss transitions passed");
}
