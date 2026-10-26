#undef NDEBUG
#include <cassert>
#include <cstdio>
#include "xbe_coop_menu.h"
using namespace trial_coop;
static TrialFrontendFrame Main(){
    TrialFrontendFrame f{};f.list_kind=1;
    auto& s=f.source;s.epoch=7;s.screen=0x600000;s.owner=0x601000;s.kind=UC_HF_LIST;s.count=3;
    auto& r=s.rows[1];r.row=0x602000;r.index=1;r.enabled=1;r.actions=UC_HF_ROW_ACTIVATE;r.callback=0x271730;
    std::strcpy(r.label,"NewFrontend-Singleplayer-1");return f;
}
int main(){
    auto main=Main();MenuState m;UcHybridFrontendAction action{};
    Reset();SetAvailable(false);assert(!m.Begin(main));SetAvailable(true);
    assert(CampaignRow(main)==1); // The route is identified, never assumed row zero.
    for(unsigned first=0;first<5;++first)for(unsigned second=0;second<5;++second){
        Reset();m={};assert(m.Begin(main));m.Choose(0,first);m.Choose(1,second);
        assert(!m.Submit(main,true,action));m.Confirm(0);assert(!m.Submit(main,true,action));m.Confirm(1);
        assert(!m.Submit(main,false,action)&&!Requested());
        auto changed=main;changed.source.epoch++;assert(!m.Submit(changed,true,action));
        changed=main;changed.source.rows[1].row++;assert(!m.Submit(changed,true,action));
        changed=main;changed.source.rows[1].callback=0x256900;assert(!m.Submit(changed,true,action));
        changed=main;changed.source.rows[1].enabled=0;assert(!m.Submit(changed,true,action));
        assert(m.Submit(main,true,action)&&Requested()&&!Active());
        assert(Selected(0)==Character(first)&&Selected(1)==Character(second));
        assert(action.epoch==7&&action.screen==0x600000&&action.owner==0x601000&&action.row==0x602000&&action.index==1&&action.kind==UC_HF_ACTIVATE);
        assert(!m.Submit(main,true,action));m.Choose(0,(first+1)%5);assert(Selected(0)==Character(first));
        m.Observe({});assert(Requested());m.Observe(main);assert(Requested());
        auto levels=main;levels.source.kind=UC_HF_LEVELS;levels.source.owner++;levels.source.epoch++;
        m.Observe(levels);assert(m.phase==MenuState::Campaign);
        auto fading=main;fading.source.source_kind|=UC_HF_TRANSITION;m.Observe(fading);assert(Requested());
        m.Observe(main);assert(!Requested()&&m.phase==MenuState::Idle);
    }
    Reset();m={};auto changed=main;std::strcpy(changed.source.rows[1].label,"NewFrontEnd-PlayDemo-1");
    assert(!m.Begin(changed));changed=main;changed.source.rows[1].actions=0;assert(!m.Begin(changed));
    assert(m.Begin(main));m.Move(0,-1);assert(m.choice[0]==4&&m.choice[1]==0);
    m.Confirm(0);m.Move(0,1);assert(m.choice[0]==0&&!m.ready[0]);
    m.Move(1,1);m.Confirm(1);assert(m.choice[1]==1&&m.ready[1]&&!m.ready[0]);
    changed=main;changed.source.screen++;m.Observe(changed);assert(m.phase==MenuState::Idle&&!Requested());
    assert(m.Begin(main));m.Confirm(0);m.Confirm(1);assert(m.Submit(main,true,action));
    m.Cancel();assert(!Requested()); // Explicit campaign Back and Single Player.
    SetActive(true);assert(!m.Begin(main));SetActive(false);
    assert(m.Begin(main));m.Confirm(0);m.Confirm(1);assert(m.Submit(main,true,action));
    auto levels=main;levels.source.kind=UC_HF_LEVELS;m.Observe(levels);SetActive(true);
    m.Observe(main);assert(m.phase==MenuState::Leaving&&Active()&&Requested());
    SetActive(false);m.Observe({});assert(m.phase==MenuState::Idle&&!Requested());
    assert(m.Begin(main));m.Cancel();
    MenuPad pad;
    assert(!pad.Sample(true,4));assert(!pad.Sample(true,4));assert(!pad.Sample(true,0));
    assert(pad.Sample(true,4)==4);assert(!pad.Sample(true,4));
    assert(!pad.Sample(false,0)&&!pad.connected&&!pad.released);
    assert(!pad.Sample(true,4));assert(!pad.Sample(true,0));assert(pad.Sample(true,8)==8);
    Reset();puts("co-op menu: 25 character pairs, exact campaign identity, readiness, disconnect, handoff and cancellation passed (offline)");
}
