#include "xbe_coop_downed_camera.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_body.h"
#include "xbe_coop_death.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace coop_downed_camera {
struct Matrix {float v[16]{};};
struct State {
    uint32_t owner=0,owner_id=0,body=0,body_id=0,world=0;
    Matrix relative,last;
    bool valid=false;
};
State states[2];
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
Matrix Load(const uint8_t* b,uint32_t p){Matrix m;std::memcpy(m.v,b+p,sizeof m.v);return m;}
Matrix Multiply(const Matrix& a,const Matrix& b){
    Matrix out;
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
        for(unsigned k=0;k<4;++k)out.v[r*4+c]+=a.v[r*4+k]*b.v[k*4+c];
    return out;
}
bool Rigid(Matrix& m){
    for(float x:m.v)if(!std::isfinite(x)||std::fabs(x)>1.e7f)return false;
    if(std::fabs(m.v[3])+std::fabs(m.v[7])+std::fabs(m.v[11])>.01f||std::fabs(m.v[15]-1)>.01f)return false;
    for(unsigned r=0;r<3;++r){
        float length=0;for(unsigned c=0;c<3;++c)length+=m.v[r*4+c]*m.v[r*4+c];
        if(length<.01f||length>100.f)return false;
        length=std::sqrt(length);for(unsigned c=0;c<3;++c)m.v[r*4+c]/=length;
    }
    for(unsigned r=0;r<3;++r)for(unsigned s=r+1;s<3;++s){
        float dot=0;for(unsigned c=0;c<3;++c)dot+=m.v[r*4+c]*m.v[s*4+c];
        if(std::fabs(dot)>.01f)return false;
    }
    return true;
}
Matrix Inverse(const Matrix& m){
    Matrix out;out.v[15]=1;
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)out.v[r*4+c]=m.v[c*4+r];
    for(unsigned c=0;c<3;++c)for(unsigned k=0;k<3;++k)out.v[12+c]-=m.v[12+k]*out.v[k*4+c];
    return out;
}
bool Owner(const uint8_t* b,uint32_t owner,unsigned slot){
    return b&&trial_coop::Active()&&slot<2&&TrialCoopPlayer(slot)==owner&&
        TrialGuestReadable(owner,0x890)&&Word(b,owner)==0x46fbe8&&Word(b,owner+0x368)==slot&&
        Word(b,owner+0x54)!=0xffffffffu&&!(Word(b,owner+0x5c)&1);
}
bool Head(const X86Context& c,uint8_t* b,uint32_t body,Matrix& head){
    if(!TrialGuestReadable(body,0x270)||Word(b,body)!=0x45d040||c.esp<0x18000)return false;
    const uint32_t wrapper=Word(b,body+0x74);
    if(!TrialGuestReadable(wrapper,4))return false;
    const uint32_t render=Word(b,wrapper);
    if(!TrialGuestReadable(render,0x68)||Word(b,render)!=0x457880)return false;
    // Retail animated render interface +24 exposes its skeleton at render+8.
    const uint32_t skeleton=Word(b,render+8);
    if(!TrialGuestReadable(skeleton,0x14)||Word(b,skeleton+8)==0||Word(b,skeleton+16)==0)return false;
    const uint32_t scratch=(c.esp-0x100)&~15u;
    if(!TrialGuestReadable(scratch-0x2000,0x2040))return false;
    X86Context temp=c;temp.esp=scratch;temp.ecx=body;
    const unsigned csr=_mm_getcsr();
    const uint32_t found=kernel::CallGuest(temp,0x20b950,{0x45c6dc,scratch});
    _mm_setcsr(csr);
    if(!(found&255))return false;
    // Same named-head/local-to-world composition used by retail 1D6060.
    head=Multiply(Load(b,scratch),Load(b,body+0x10));
    return Rigid(head);
}
bool Matches(const uint8_t* b,const State& s,unsigned slot){
    return s.valid&&Owner(b,s.owner,slot)&&Word(b,s.owner+0x54)==s.owner_id&&
        Word(b,0x51c4dc)==s.world&&TrialCoopBodyForPlayer(b,s.owner)==s.body&&
        TrialGuestReadable(s.body,0x270)&&Word(b,s.body+0x54)==s.body_id;
}
}

void TrialCoopDownedCameraReset(){for(auto& s:coop_downed_camera::states)s={};}

void TrialCoopDownedCameraCapture(const X86Context& c,uint8_t* b){
    using namespace coop_downed_camera;
    const uint32_t owner=c.ecx;
    if(!b||!TrialGuestReadable(owner,0x890))return;
    const unsigned slot=Word(b,owner+0x368);
    if(!Owner(b,owner,slot)||Word(b,owner+0x170)==12)return;
    State& s=states[slot];s={};
    const uint32_t body=TrialCoopBodyForPlayer(b,owner);
    Matrix head,eye=Load(b,owner+0x6b0);
    if(!Head(c,b,body,head)||!Rigid(eye))return;
    Matrix relative=Multiply(eye,Inverse(head));
    float distance=0;for(unsigned i=12;i<15;++i)distance+=relative.v[i]*relative.v[i];
    if(distance>80.f*80.f||!Rigid(relative))return;
    // Calibrate from the living eye frame; no guessed skeleton axes or bone ID.
    s={owner,Word(b,owner+0x54),body,Word(b,body+0x54),Word(b,0x51c4dc),relative,eye,true};
}

void TrialCoopDownedCameraUpdate(const X86Context& c,uint8_t* b){
    using namespace coop_downed_camera;
    if(!b||!trial_coop::Active()){TrialCoopDownedCameraReset();return;}
    const uint32_t camera=c.esi;
    if(!TrialGuestReadable(camera,0x230)||Word(b,camera)!=0x47c780||
       Word(b,camera+0x210)!=2||(Word(b,camera+0x5c)&1))return;
    for(unsigned slot=0;slot<2;++slot){
        auto& s=states[slot];
        if(!Matches(b,s,slot)||!TrialCoopPlayerDowned(b,s.owner)){s={};continue;}
        if(Word(b,camera+0x54)!=Word(b,s.owner+0x308)||Word(b,camera+0x58)!=s.owner_id||
           Word(b,camera+0x22c)!=s.owner_id)continue;
        Matrix head;
        if(Head(c,b,s.body,head)){
            Matrix pose=Multiply(s.relative,head);
            if(Rigid(pose))s.last=pose;
        }
        // Keep the last valid pose on a missing frame; FOV/projection stay native.
        std::memcpy(b+camera+0x10,s.last.v,sizeof s.last.v);
        return;
    }
}
