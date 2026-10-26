#pragma once
#include <cstdint>

// Raw capture is separate from gameplay bindings. The opening click/button and
// any held input must be released before a new binding can be accepted.
namespace trial_controls {
struct CaptureGate {
    unsigned device=0, pending=0;
    bool active=false, armed=false;
    void Begin(unsigned next){device=next;pending=0;active=true;armed=false;}
    void Cancel(){active=armed=false;pending=0;}
    void Sample(unsigned keyboard,unsigned controller,bool focused){
        if(!active)return;
        if(!focused){Cancel();return;}
        if(!armed){if(!keyboard&&!controller)armed=true;return;}
        const unsigned source=device?controller:keyboard;
        if(source){pending=source;active=false;}
    }
    void Wheel(unsigned source){
        if(active&&armed&&!device&&(source==256||source==257)){pending=source;active=false;}
    }
    unsigned Take(){const auto value=pending;pending=0;return value;}
};
void BeginCapture(unsigned device,unsigned player=0);
void CancelCapture(unsigned player=0);
unsigned TakeCapture(unsigned player=0);
bool CaptureWaiting(unsigned player=0);
}
