#pragma once
#include "character_cover.h"
extern "C" {
#include "../cop_attack.h"
}
//Retained1C0CB0/1C0AE0/1C0BD0 owner, including the actual shared-manager job.
//Call close after condition39240 and before shared behavior328F0.
class NativeCopAttackCover {
    NativeMissionWorld &world_;NativeCharacterCoverPoints &points_;
    NativeCharacterNavigationManager &manager_;NativeCharacterNavigator &navigator_;
    int32_t actor_,sender_,authored_;
    std::unique_ptr<NativeCharacterCoverSearch> search_;
    void release(int32_t);void claim(UCCopAttack &);
public:
    NativeCopAttackCover(NativeMissionWorld &,NativeCharacterCoverPoints &,
        NativeCharacterNavigationManager &,NativeCharacterNavigator &,
        int32_t actor,int32_t behavior,int32_t authored_cover=-1);
    void step(UCCopAttack &);
    void close(UCCopAttack &);
    bool pending()const{return search_!=nullptr;}
};
