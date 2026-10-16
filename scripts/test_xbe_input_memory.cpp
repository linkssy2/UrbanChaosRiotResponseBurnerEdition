#include <cstdint>
namespace mem {
bool IsCommitted(uint32_t a){return a==0x10000||a==0xd007e000||a==0xd007f000;}
}
#include "xbe_input_memory.h"
int main(){
    if(!TrialGuestReadable(0x10000,4096)||!TrialGuestReadable(0xd007eff0,32))return 1;
    if(TrialGuestReadable(0,4)||TrialGuestReadable(0x10000,0))return 2;
    if(TrialGuestReadable(0x10ff0,32)||TrialGuestReadable(0xd007fff0,32))return 3;
    if(TrialGuestReadable(0xfffffff0,32)||TrialGuestReadable(0xd007dfff,2))return 4;
    return 0;
}
