#pragma once
#include <cstdint>
struct X86Context;

// The real mounted weapon's matrix, before its native muzzle/attachment queries.
void TrialCoopTurretWeaponMatrix(X86Context&,uint8_t*);
class TrialCoopTurretView {
public:
    ~TrialCoopTurretView();
    void Select(const X86Context&,uint8_t*);
    void Clear();
private:
    uint8_t* base_=nullptr;
    uint32_t owner_=0,id_=0,wrapper_=0,render_=0,flags_=0;
    uint8_t visible_=0;
};
