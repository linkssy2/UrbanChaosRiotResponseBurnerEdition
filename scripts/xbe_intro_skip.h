#pragma once
#include <cstdint>

namespace trial_intro {
// Physical edges outlive each movie object. Opening another movie never turns
// a still-held input into a new press, and no queued press crosses that boundary.
struct Gate {
    uint32_t owner=0, kind=0;
    unsigned keyboard=0, pending=0;
    bool ready=false, used=false;
    bool held[5]={}, blocked[5]={}, consumed[5]={}; // keyboard, then four pads

    void Context(uint32_t next, uint32_t movie, bool opening, bool eligible) {
        if(owner&&!next)for(unsigned i=0;i<5;++i)consumed[i]|=held[i];
        if(next!=owner||movie!=kind||opening){pending=0;used=false;}
        owner=next;kind=movie;ready=next&&eligible;
        if(!ready)pending=0;
    }
    void Close(uint32_t previous) {
        if(owner&&owner==previous){
            for(unsigned i=0;i<5;++i)consumed[i]|=held[i];
            owner=kind=pending=0;ready=used=false;
        }
    }
    void FocusLost() {
        pending=0;
        for(auto& value:blocked)value=true;
    }
    void Poll(unsigned source,bool down,bool focused) {
        if(source>=5)return;
        if(!focused){blocked[source]=true;pending=0;held[source]=down;return;}
        if(!down){blocked[source]=consumed[source]=false;}
        if(down&&!held[source]&&!blocked[source]&&owner&&ready&&!used)
            pending|=1u<<source;
        held[source]=down;
    }
    void Keyboard(unsigned mask,bool focused) {
        keyboard=mask;Poll(0,mask!=0,focused);
    }
    void Key(unsigned bit,bool down,bool focused) {
        Keyboard(down?keyboard|bit:keyboard&~bit,focused);
    }
    template<class Analog> void Pad(int port,uint32_t buttons,const Analog& analog,bool focused) {
        if(port<0||port>=4)return;
        bool down=buttons!=0;
        for(auto value:analog)down|=value>30; // ignore trigger noise, as XInput does
        Poll(unsigned(port)+1,down,focused);
    }
    bool Take(uint32_t screen,unsigned port) {
        if(!owner||owner!=screen||port>=4||!ready||used)return false;
        const unsigned mask=(1u<<(port+1))|(port==0?1u:0u);
        if(!(pending&mask))return false;
        pending=0;used=true;
        for(unsigned i=0;i<5;++i)consumed[i]|=held[i];
        return true;
    }
    bool OwnsPad(int port) const {
        return port>=0&&port<4&&(owner||consumed[port+1]||(port==0&&consumed[0]));
    }
};
}

void TrialIntroMovie(uint32_t owner,uint32_t kind,bool opening,bool ready);
bool TrialIntroTake(uint32_t owner,unsigned port);
void TrialIntroClose(uint32_t owner);
