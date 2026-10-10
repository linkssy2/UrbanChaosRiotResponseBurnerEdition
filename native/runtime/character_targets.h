#pragma once
#include <cstdint>
#include <map>
#include <vector>

// Borrowed live registry/property owners. Return 1 for found/true, 0 for
// missing/false, -1 for an unsupported operation. Failures throw; they are
// never converted to "no enemy". Coordinates retain the original X/Y/Z units.
struct NativeCharacterTargetServices {
    void *owner=nullptr;
    int (*resolve)(void *,int32_t,float position[3])=nullptr;
    int (*property)(void *,int32_t,const char *,int32_t *)=nullptr;
    int (*visible)(void *,int32_t)=nullptr; //E05C0(id,-1,0,{0,0,0},1)
    int32_t (*player)(void *)=nullptr;
    int (*dialogue)(void *,uint32_t,unsigned)=nullptr; //DED50, zero on success
};

// 2F5F0/2F170 and39C40/397E0 membership,39960 relationships,3A0D0/3A020
// caches. refresh() is the original faction-manager update boundary, NOT an
// on-demand target query. Registration/removal does not refresh cached IDs.
class NativeCharacterFactions {
    struct Group {std::vector<int32_t> members,friends,enemies,opponents;
        std::map<int32_t,std::vector<int32_t>> attackers;};
    std::map<int32_t,Group> groups_;
    Group &group(int32_t);const Group &group(int32_t)const;
public:
    NativeCharacterFactions();
    static int32_t key(int32_t authored);
    void add(int32_t authored,int32_t actor);
    void remove(int32_t authored,int32_t actor);
    bool opponent(int32_t authored,int32_t actor)const;
    bool friendly(int32_t authored,int32_t actor)const; //39900, distinct from !opponent.
    const std::vector<int32_t> &opponents(int32_t authored)const;
    int32_t attackers(int32_t authored,int32_t actor,const NativeCharacterTargetServices &)const;
    int32_t attacker(int32_t authored,int32_t actor,uint32_t index,const NativeCharacterTargetServices &)const;
    void refresh(const NativeCharacterTargetServices &);
};

struct NativeCharacterTargetState {
    int32_t current=-1,forced=-1,preferred=-1;
    bool allow_incapacitated=false;
    float player_seen=0; //controller5C; caller owns its original timer update
};
struct NativeCharacterTargetActor {
    int32_t id=-1,faction=-1; //-1 means actor100 is null, not faction0
    float position[3]{},forward[3]{0,0,1};
};
bool native_character_target_valid(const NativeCharacterTargetState &,
    const NativeCharacterTargetActor &,const NativeCharacterFactions &,
    const NativeCharacterTargetServices &,int32_t target); //33AB0
//33B40 validates every read and clears a retired/ineligible retained target.
int32_t native_character_current_target(NativeCharacterTargetState &,
    const NativeCharacterTargetActor &,const NativeCharacterFactions &,
    const NativeCharacterTargetServices &);
int32_t native_character_select_target(const NativeCharacterTargetState &,
    const NativeCharacterTargetActor &,const NativeCharacterFactions &,
    const NativeCharacterTargetServices &,bool prefer_new,bool require_visible);
// Full35BC0 wrapper; returns whether controller3C changed. Earlier effects are
// committed on service failure, matching other mission owners; do not retry.
bool native_character_acquire_target(NativeCharacterTargetState &,
    const NativeCharacterTargetActor &,const NativeCharacterFactions &,
    const NativeCharacterTargetServices &,bool prefer_new,bool require_visible);
