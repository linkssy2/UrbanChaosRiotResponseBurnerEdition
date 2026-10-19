"""Opt-in reporter trace, staged only in explicitly requested diagnostic builds."""
from pathlib import Path

def replace(s, old, new):
    assert s.count(old)==1, old
    return s.replace(old,new)

def prepare(area):
    root=Path(__file__).parent
    for directory in ('common','compat'):
        (area/'src'/directory/'xbe_input_diagnostic.h').write_text((root/'xbe_input_diagnostic.h').read_text())
    p=area/'src/compat/input.cpp';s=p.read_text()
    s='#include "xbe_input_diagnostic.h"\n'+s
    s=replace(s, '    const char* name = *slot ?', '    input_diagnostic::Log("device slot=%d opened=%d type=%d vendor=%04x product=%04x", int(slot-g_pads), *slot!=nullptr, *slot?int(SDL_GetRealGamepadType(*slot)):0, *slot?SDL_GetGamepadVendor(*slot):0, *slot?SDL_GetGamepadProduct(*slot):0);\n    const char* name = *slot ?')
    s=replace(s,'        SDL_CloseGamepad(g_pads[i]);','        input_diagnostic::Log("device removed slot=%d",i);\n        SDL_CloseGamepad(g_pads[i]);')
    s=replace(s,'        LOGE("input: SDL failed', '        input_diagnostic::Log("SDL initialization failed");\n        LOGE("input: SDL failed')
    p.write_text(s)
    p=area/'src/common/xbe_trial_menu_input.cpp';s=p.read_text();s='#include "xbe_input_diagnostic.h"\n'+s
    s=replace(s,'    ControlsCaptureMessage(msg,wp);','''    if(input_diagnostic::Enabled()){
        static unsigned keys=0,clicks=0,moves=0;static input_diagnostic::Rate rate;
        keys+=msg==WM_KEYDOWN;clicks+=msg==WM_LBUTTONDOWN||msg==WM_RBUTTONDOWN;moves+=msg==WM_MOUSEMOVE;
        if(rate.Due()){input_diagnostic::Log("host keys=%u clicks=%u moves=%u focus=%d",keys,clicks,moves,focused());keys=clicks=moves=0;}
    }
    ControlsCaptureMessage(msg,wp);''')
    s=replace(s,'    if(port<0||port>=4)return;','''    if(port<0||port>=4)return;
    if(input_diagnostic::Enabled()){
        static input_diagnostic::Rate rate[4];
        if(rate[port].Due()){
            unsigned held=0;for(int k=1;k<256;++k)held+=(GetAsyncKeyState(k)&0x8000)!=0;
            input_diagnostic::Log("pad port=%d connected=%d buttons=%x analog=%u,%u,%u,%u,%u,%u,%u,%u axes=%d,%d,%d,%d focus=%d keyboard=%d menu=%d gameplay=%d ui=%d release=%d intro=%u capture=%d held_count=%u",port,pad.connected,pad.buttons,pad.analog[0],pad.analog[1],pad.analog[2],pad.analog[3],pad.analog[4],pad.analog[5],pad.analog[6],pad.analog[7],pad.lx,pad.ly,pad.rx,pad.ry,focused(),xbox_KeyboardMode(),menu_mode,gameplay,TrialGraphicsOpen(),graphics_release_pending[port],intro_gate.owner,captured,held);
        }
    }''')
    s=replace(s, '    pad.buttons|=keys.wButtons;', r'''    if(input_diagnostic::Enabled()){
        static input_diagnostic::Rate rate;
        if(rate.Due())input_diagnostic::Log("delivered keyboard=%d digital=%x confirm=%u back=%u",xbox_KeyboardMode(),keys.wButtons,keys.bAnalogButtons[0],keys.bAnalogButtons[1]);
    }
    pad.buttons|=keys.wButtons;''')
    p.write_text(s)
    p=area/'src/common/xbe_trial_graphics_ui.cpp';s=p.read_text();s='#include "xbe_input_diagnostic.h"\n'+s
    s=replace(s,'        changed=frontend_active.load()!=active;','''        if(input_diagnostic::Enabled()){
            static input_diagnostic::Rate rate;
            if(rate.Due())input_diagnostic::Log("frontend epoch=%llu owner=%x screen=%x kind=%u source_kind=%u rows=%u selected=%u active=%d native=%d generation=%llu",(unsigned long long)frame.source.epoch,frame.source.owner,frame.source.screen,frame.source.kind,frame.source.source_kind,frame.source.count,frame.source.selected,active,native,(unsigned long long)navigation_generation);
        }
        changed=frontend_active.load()!=active;''')
    p.write_text(s)
