#pragma once
#include <cstdint>
// The new runtime places stacks above physical RAM; validate every mapped page.
static bool TrialGuestReadable(uint32_t address,uint32_t bytes){
    if(address<0x10000u||!bytes||uint64_t(address)+bytes>0x100000000ull)return false;
    uint32_t last=(address+bytes-1)&~0xfffu;
    for(uint32_t page=address&~0xfffu;;page+=0x1000){
        if(!mem::IsCommitted(page))return false;
        if(page==last)return true;
    }
}
