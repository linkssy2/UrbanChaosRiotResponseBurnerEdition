#ifndef UCRR_XBE_COOP_H
#define UCRR_XBE_COOP_H
#include <atomic>

// Host requests cross to the guest only at the campaign's player startup.
// Physical controller routing changes only after that startup succeeds.
namespace trial_coop {
inline std::atomic<bool> requested=false,active=false,available=false;
enum class Character:unsigned {TZero,Avenger,Banger,Magnum,Prophet};
inline std::atomic<Character> characters[2]{Character::TZero,Character::TZero};
inline std::atomic<bool> resources_ready=false,resource_failure=false;
inline bool Requested(){return requested.load();}
inline bool Active(){return active.load();}
inline bool Available(){return available.load();}
inline void SetAvailable(bool value){available=value;}
inline Character Selected(unsigned slot){return slot<2?characters[slot].load():Character::TZero;}
inline bool ResourcesReady(){return resources_ready.load();}
inline void ResourcesReady(bool value){resources_ready=value;}
inline bool ResourceFailure(){return resource_failure.load();}
inline void SetResourceFailure(bool value){resource_failure=value;}
inline bool Request(Character first=Character::TZero,Character second=Character::TZero){
    if(Requested()||Active()||unsigned(first)>4||unsigned(second)>4)return false;
    characters[0]=first;characters[1]=second;requested=true;return true;
}
inline void SetActive(bool value){active=value;}
inline void Reset(){active=false;requested=false;resources_ready=false;resource_failure=false;}
}
#endif
