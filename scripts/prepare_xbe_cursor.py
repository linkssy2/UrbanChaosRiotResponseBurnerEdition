"""Restore the existing shield cursor and give the window sole cursor ownership.

The pinned ImGui Win32 backend otherwise consumes WM_SETCURSOR before the game
hook and may replace the hidden gameplay cursor during NewFrame. Run on the
copied xbe_trial_menu_input.cpp; no upstream or original artwork is changed.
"""
from pathlib import Path


CURSOR = r'''
static HCURSOR TrialMenuCursor() {
    // Same procedural artwork/hotspot as the accepted native/CONTROL menus.
    // Only the window thread calls this; DPI variants are cached by its owner.
    try { thread_local NativeMenuCursor cursor; return cursor.get(game_window); }
    catch (...) { return LoadCursor(nullptr, IDC_ARROW); }
}
'''

RECONCILE = r'''static void ReconcileCapture(){
    bool want=focused()&&gameplay&&!menu_mode&&!TrialGraphicsOpen();
    if(captured!=want){captured=want;dx=dy=0;}
    if(want){RECT r;GetClientRect(game_window,&r);MapWindowPoints(game_window,nullptr,(POINT*)&r,2);ClipCursor(&r);SetCursor(nullptr);}
    else{ClipCursor(nullptr);SetCursor(focused()?(xbox_KeyboardMode()?TrialMenuCursor():nullptr):LoadCursor(nullptr,IDC_ARROW));}
}'''


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def prepare(source, area):
    area = Path(area)
    root = Path(__file__).resolve().parents[1]
    backend = (root / 'build/upstream-review-xboxxbe-20261014/runtime/app/main.cpp').read_text()
    # Initialization precedes the game window hook, so changing the ImGui flag
    # here is effective before either the first UI frame or guest execution.
    assert backend.index('ImGui::CreateContext();') < backend.index('app::kGame.window(app::g_hwnd)')
    source = once(source, '#include "core/input.h"',
                  '#include "core/input.h"\n#include "imgui.h"\n#include "native/runtime/menu_cursor.h"')
    begin = source.index('static void ReconcileCapture(){')
    end = source.index('\n}', begin) + 2
    source = source[:begin] + CURSOR + RECONCILE + source[end:]
    source = once(source, 'void TrialInputWindow(void* window){', '''void TrialInputWindow(void* window){
    // Do not nest the UI lock with input_lock: Win32 and renderer share it.
    {
        std::lock_guard guard(app::UiLock());
        if(ImGui::GetCurrentContext()) {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        }
    }''')
    source = once(source,
                  'if(!RegisterRawInputDevices(&device,1,sizeof(device)))throw std::runtime_error("Raw mouse registration failed");',
                  'if(!RegisterRawInputDevices(&device,1,sizeof(device)))throw std::runtime_error("Raw mouse registration failed");\n    ReconcileCapture();')
    source = once(source, 'if(msg==WM_KILLFOCUS||(msg==WM_ACTIVATEAPP&&!wp)){',
                  'if(msg==WM_KILLFOCUS||msg==WM_DESTROY||(msg==WM_ACTIVATEAPP&&!wp)){')
    source = once(source, 'keyboard_wheel_clear();ClipCursor(nullptr);',
                  'keyboard_wheel_clear();ClipCursor(nullptr);SetCursor(LoadCursor(nullptr,IDC_ARROW));')
    source = once(source, 'if(msg==kCapture||msg==WM_SETFOCUS||msg==WM_SIZE)',
                  'if(msg==kCapture||msg==WM_SETFOCUS||msg==WM_SIZE||msg==WM_DPICHANGED)')
    fixture = Path(__file__).with_name('test_xbe_cursor.cpp').read_text()
    fixture = once(fixture, '// GENERATED_CURSOR', CURSOR)
    fixture = once(fixture, '// GENERATED_RECONCILE', RECONCILE)
    (area / 'src/compat/test_cursor.cpp').write_text(fixture)
    return source
