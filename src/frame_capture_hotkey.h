#ifndef UCRR_FRAME_CAPTURE_HOTKEY_H
#define UCRR_FRAME_CAPTURE_HOTKEY_H
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
/* Only a real nonrepeated F8 keydown in an opted-in run signals the existing
 * presentation-owned recorder. No per-frame polling or diagnostic policy change. */
static int uc_frame_capture_hotkey(WPARAM key,LPARAM flags){
    if(key!=VK_F8||(flags&(1LL<<30)))return 0;
    const char*enabled=getenv("UCRR_FRAME_CAPTURE");
    if(!enabled||strcmp(enabled,"1"))return 0;
    wchar_t name[80];swprintf(name,80,L"Local\\UCRR-Frames-%lu",GetCurrentProcessId());
    HANDLE event=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);
    if(!event)return 0;
    int signaled=SetEvent(event)!=0;CloseHandle(event);return signaled;
}
#endif
