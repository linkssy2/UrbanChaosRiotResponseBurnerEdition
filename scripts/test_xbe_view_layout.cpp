// Production memory guards/scopes, without a game or renderer launch.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
namespace trial_graphics {
static unsigned fov=53,reticle_percent=100,weapon_percent=100;static bool wide=true;
uint32_t FieldOfView(){return fov;}bool Widescreen(){return wide;}
uint32_t ReticleScale(){return reticle_percent;}
uint32_t WeaponFovScale(){return weapon_percent;}
}
#include "../common/xbe_view_layout.cpp"
static void W(uint8_t*b,unsigned a,unsigned v){std::memcpy(b+a,&v,4);}
static constexpr unsigned view=0x71000,model=0x72000;
static std::vector<std::array<float,16>> projections;
static unsigned model_draws;
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    if(target==0x701001||target==0x701002){
        assert(c.ecx==0x76000);x87_push(c,target==0x701001?640.0:480.0);c.esp+=4;return;
    }
    if(target==0xa2650){c.eax=view;c.esp+=4;return;}
    if(target==0xade90){c.eax=c.ecx+0xd0;c.esp+=4;return;}
    if(target==0x214e90){c.eax=c.ecx+0x110;c.esp+=4;return;}
    assert(target==0xadf50&&c.ecx==view);
    std::array<float,16> matrix;std::memcpy(matrix.data(),b+Word(b,c.esp+4),64);
    projections.push_back(matrix);std::memcpy(b+view+0xd0,matrix.data(),64);
    b[view+0x193]=1;c.esp+=8;
}
X86_FUNC(sub_003E2B80){assert(false);}
X86_FUNC(sub_003E3610){assert(false);}
X86_FUNC(sub_0008E7C0){ctx.esp+=4;}
X86_FUNC(sub_003E7080){ctx.esp+=4;}
X86_FUNC(sub_000BB870){++model_draws;ctx.esp+=20;}
X86_FUNC(sub_000BB5C0){++model_draws;ctx.esp+=8;}
X86_FUNC(sub_000BB500){++model_draws;ctx.esp+=12;}
X86_FUNC(sub_000BB300){++model_draws;ctx.esp+=12;}
X86_FUNC(sub_000BAFB0){++model_draws;ctx.esp+=12;}
X86_FUNC(sub_0008BD60){assert(ctx.ecx==0x770d0);ctx.eax=0x76000;ctx.esp+=4;}
// GENERATED_MODEL_OPERATION
// GENERATED_POINT_PROJECTION

static void CheckProjectedHud(uint8_t* b){
    const unsigned owner=0x78000,point=0x78100,matrix=0x78200,screen=0x78300;
    const unsigned a=0x78400,z=0x78410;
    W(b,owner,0x45312c);W(b,0x76000,0x76010);
    W(b,0x76034,0x701001);W(b,0x76038,0x701002);
    Put(b,0x44bf94,1);Put(b,0x44c1f8,.5f);Put(b,0x44d024,0);
    unsigned checks=0;
    for(bool wide:{false,true})for(unsigned fov:{40u,53u,70u,90u})
        for(unsigned weapon:{50u,100u,150u})for(float x:{-100.f,0.f,100.f}){
            trial_graphics::wide=wide;trial_graphics::fov=fov;trial_graphics::weapon_percent=weapon;
            const float sy=1.f/std::tan(fov*float(3.14159265358979323846/360));
            const float sx=sy/(wide?16.f/9:4.f/3);
            const std::array<float,16> m={sx,0,0,0,0,sy,0,0,0,0,1,1,0,0,0,0};
            std::memcpy(b+matrix,m.data(),64);Put(b,point,x);Put(b,point+4,30);Put(b,point+8,400);
            X86Context project{};project.esp=0x80000;
            W(b,project.esp,0x12345);W(b,project.esp+4,0x77000);W(b,project.esp+8,point);
            W(b,project.esp+12,screen);W(b,project.esp+16,matrix);
            __imp__sub_000470A0(project,b);
            assert(project.eax&255);assert(project.esp==0x80004);
            const float expected_x=(x*sx/400+1)*320,expected_y=(1-30*sy/400)*240;
            assert(std::fabs(Real(b,screen)-expected_x)<.0001f);
            assert(std::fabs(Real(b,screen+4)-expected_y)<.0001f);
            X86Context c{};c.esp=0x80000;c.ecx=owner;
            const unsigned d=c.esp+0x10;
            Put(b,d,0);Put(b,d+4,0);Put(b,d+8,640);Put(b,d+12,480);
            W(b,c.esp+4,a);W(b,c.esp+8,z);
            Put(b,a,expected_x-20-.5f);Put(b,z,expected_x+20-.5f);
            Put(b,a+4,expected_y-10-.5f);Put(b,z+4,expected_y+10-.5f);
            std::array<uint8_t,32> before;std::memcpy(before.data(),b+a,before.size());
            TrialHudLayoutScope h(c,b);TrialHudLayout(c,b);
            {
                TrialProjectedHudScope projected(c,b);
                {TrialProjectedHudScope nested(c,b);TrialHudQuadScope q(c,b);
                    // This is the coordinate copied into the GPU queue by 9D9B0.
                    // The old wide-HUD rule incorrectly moved a centered marker to x240.
                    assert(std::fabs((Real(b,a)+Real(b,z))*.5f+.5f-expected_x)<.0001f);
                    assert(!std::memcmp(before.data(),b+a,before.size()));
                }
                assert(hud==(wide?nullptr:&h));
            }
            assert(hud==&h);
            {TrialHudQuadScope ordinary(c,b);
                assert(Real(b,a)==(wide?trial_view::HudX(expected_x-20-.5f,.75f):expected_x-20-.5f));
            }
            assert(!std::memcmp(before.data(),b+a,before.size()));
            // An unrelated owner cannot disable the regular HUD correction.
            W(b,owner,0x453160);
            {TrialProjectedHudScope unrelated(c,b);assert(hud==&h);}
            W(b,owner,0x45312c);++checks;
        }
    trial_graphics::wide=true;trial_graphics::fov=53;trial_graphics::weapon_percent=100;
    assert(!hud);
    std::printf("PASS %u retail 470A0 projected HUD placements: centered/off-center, FOV/aspect, weapon independence, nested restoration and static anchors\n",checks);
}

static void CheckWeaponProjection(uint8_t* b){
    W(b,0x519f60,0x70000);W(b,0x70000,0x73000);W(b,0x73050,0xa2650);
    W(b,view,0x456d88);W(b,0x456d98,0xade90);W(b,0x456d9c,0x214e90);W(b,0x456db0,0xadf50);
    W(b,model,0x457880);W(b,model+0x60,0x180);Put(b,model+0x110,.1f);Put(b,model+0x114,.15f);
    Put(b,0x44bf94,1);
    const uint32_t destinations[]={0xbae4a,0xbaedb,0xbae9d,0xbaecc,0xbaeaa,0xbaebb,0xbae66};
    std::memcpy(b+0xbaf54,destinations,sizeof(destinations));
    unsigned checks=0;
    for(float aspect:{4.f/3,16.f/9})for(float degrees:{38.f,48.f,53.f})
        for(unsigned percent=50;percent<=150;++percent)for(unsigned op=0;op<7;++op){
            trial_graphics::weapon_percent=percent;
            // Independent world FOV: changing it must never enter this path.
            trial_graphics::fov=(percent&1)?40:90;
            const float y=1.f/std::tan(degrees*float(3.14159265358979323846/360));
            std::array<float,16> alternate{y/aspect,0,0,0,0,y,0,0,0,0,1.0005f,1,0,0,-25.0125f,0};
            std::array<float,16> world{1,0,0,0,0,2,0,0,0,0,1.0005f,1,0,0,-25.0125f,0};
            std::memcpy(b+view+0xd0,world.data(),64);std::memcpy(b+view+0x110,alternate.data(),64);
            std::array<uint8_t,0x118> model_before;std::memcpy(model_before.data(),b+model,model_before.size());
            X86Context c{};c.ecx=model;c.esp=0x80000;c.esi=11;c.edi=12;c.ebx=13;c.ebp=14;
            W(b,c.esp,0x12345);W(b,c.esp+4,0x74000);W(b,0x74000,op);W(b,0x74004,0x75000);
            projections.clear();model_draws=0;__imp__sub_000BAC10(c,b);
            assert(c.esp==0x80008&&c.esi==11&&c.edi==12&&c.ebx==13&&c.ebp==14);
            assert(model_draws==(op==1?0:1)&&projections.size()==2);
            auto expected=alternate;
            expected[0]*=100.f/percent;expected[5]*=100.f/percent;
            for(unsigned row=0;row<4;++row)expected[row*4+2]=alternate[row*4+3]*.1f+alternate[row*4+2]*(.15f-.1f);
            for(unsigned i=0;i<16;++i)assert(std::fabs(projections[0][i]-expected[i])<.00001f);
            assert(!std::memcmp(projections[1].data(),world.data(),64));
            assert(!std::memcmp(b+view+0xd0,world.data(),64)&&!std::memcmp(b+view+0x110,alternate.data(),64));
            assert(!std::memcmp(b+model,model_before.data(),model_before.size()));
            ++checks;
        }
    // Invalid or unrelated copies must stay byte-identical, including FP state.
    X86Context c{};c.esi=model;c.edi=view;c.ecx=view;c.esp=0x80000;
    const unsigned matrix=c.esp+0x54;W(b,c.esp,matrix);
    std::array<float,16> matrix_data{1,0,0,0,0,2,0,0,0,0,1,1,0,0,-25,0};
    auto unchanged=[&]{
        std::array<uint8_t,64> before;std::memcpy(before.data(),b+matrix,64);
        const unsigned csr=_mm_getcsr();_mm_setcsr(0xffc0);
        TrialWeaponProjection(c,b);assert(_mm_getcsr()==0xffc0);_mm_setcsr(csr);
        assert(!std::memcmp(before.data(),b+matrix,64));
    };
    std::memcpy(b+matrix,matrix_data.data(),64);
    for(unsigned percent:{0u,49u,100u,151u,0xffffffffu}){trial_graphics::weapon_percent=percent;unchanged();}
    trial_graphics::weapon_percent=150;
    for(unsigned flags:{0u,0x80u,0x100u}){W(b,model+0x60,flags);unchanged();}W(b,model+0x60,0x180);
    W(b,model,0x453160);unchanged();W(b,model,0x457880);
    Put(b,model+0x110,.2f);unchanged();Put(b,model+0x110,.1f);
    Put(b,model+0x114,.3f);unchanged();Put(b,model+0x114,.15f);
    W(b,view,0);unchanged();W(b,view,0x456d88);
    c.ecx=0;unchanged();c.ecx=view;
    W(b,c.esp,view+0x110);unchanged();W(b,c.esp,matrix);
    for(unsigned offset:{0u,20u,44u}){
        const unsigned before=Word(b,matrix+offset);W(b,matrix+offset,0x7fc01234);unchanged();W(b,matrix+offset,before);
    }
    Put(b,matrix,0);unchanged();Put(b,matrix,-1);unchanged();Put(b,matrix,1);
    c.esp=0xfffffffcu;TrialWeaponProjection(c,b);c.esp=0x80000;
    c.esi=0xfffffffcu;TrialWeaponProjection(c,b);c.esi=model;
    const unsigned csr=_mm_getcsr();_mm_setcsr(0xffc0);
    TrialWeaponProjection(c,b);assert(_mm_getcsr()==0xffc0);_mm_setcsr(csr);
    assert(Real(b,matrix)==1.f*(100.f/150)&&Real(b,matrix+20)==2.f*(100.f/150));
    std::printf("PASS %u original BAC10 weapon projections: original/zoomed framing, all draw operations, depth/aspect and world restoration\n",checks);
}
int main(){
    auto*b=kernel::memory.data();X86Context c{};c.esi=0x20000;c.eax=17;c.ecx=0x30000;c.esp=0x40000;
    W(b,c.esi+0x54,12);W(b,c.esi+0x308,17);TrialPlayerCameraCreated(c,b);
    W(b,c.ecx+0x54,17);W(b,c.ecx+0x58,12);W(b,c.ecx+0x22c,12);W(b,c.ecx+0x210,2);
    Put(b,c.ecx+0x228,53);
    assert(TrialCameraTarget(c,b)==53);
    trial_graphics::fov=70;
    assert(std::fabs(TrialCameraTarget(c,b)-70)<.0001f);
    assert(Real(b,c.ecx+0x228)==53);
    Put(b,c.ecx+0x228,30);
    for(int i=0;i<100;++i){assert(TrialCameraTarget(c,b)>30&&TrialCameraTarget(c,b)<70);}
    assert(Real(b,c.ecx+0x228)==30); // no cumulative widening
    W(b,c.ecx+0x54,18);
    assert(TrialCameraTarget(c,b)==30);
    W(b,c.ecx+0x54,17);W(b,c.ecx+0x210,1);
    assert(TrialCameraTarget(c,b)==30);
    for(unsigned fov=40;fov<=90;++fov){
        float out=trial_view::Zoom(53,fov);assert(std::fabs(out-fov)<.0001f);
        assert(trial_view::Zoom(20,fov)<out);
    }
    const unsigned d=c.esp+0x10,a=0x50000,z=0x50010;
    W(b,c.esp+4,a);W(b,c.esp+8,z);
    auto rect=[&](float w=640){Put(b,d,0);Put(b,d+4,0);Put(b,d+8,w);Put(b,d+12,480);};
    rect();Put(b,a,48);Put(b,z,112);
    {TrialHudQuadScope q(c,b);assert(Real(b,a)==48&&Real(b,z)==112);} // menus untouched
    {
        TrialHudLayoutScope h(c,b);TrialHudLayout(c,b);
        assert(Real(b,d+8)==640/.75f);
        {TrialHudQuadScope q(c,b);assert(std::fabs((Real(b,z)-Real(b,a))*(16.f/9)/(4.f/3)-64)<.0001f);}
        assert(Real(b,a)==48&&Real(b,z)==112);
        // Right edge stays 48 authored pixels from the physical screen edge.
        Put(b,a,640/.75f-48-.5f);Put(b,z,640/.75f-.5f);
        {TrialHudQuadScope q(c,b);assert(Real(b,z)==639.5f);assert(std::fabs(Real(b,a)-603.5f)<.0001f);}
        const uint32_t center_bits=TrialHudCenterBits();float center;std::memcpy(&center,&center_bits,4);
        assert(std::fabs(center*.75f-320)<.0001f);
        rect(320);{TrialHudLayoutScope split(c,b);TrialHudLayout(c,b);assert(split.scale_==1);}
        assert(hud==&h); // nested drawing restores the enclosing scope
    }
    assert(hud==nullptr);
    trial_graphics::wide=false;rect();
    {TrialHudLayoutScope h(c,b);TrialHudLayout(c,b);assert(Real(b,d+8)==640&&h.scale_==1);}

    // Use the actual retail subtree/sprite identities and final quad caller.
    const unsigned owner=0x60000,tree=0x61000,sprite=0x62000;
    c.ecx=owner;c.esi=sprite;
    W(b,owner,0x453160);W(b,owner+0x90,tree);W(b,sprite,0x453b50);
    W(b,c.esp,0x7b149);W(b,c.esp+4,a);W(b,c.esp+8,z);
    for(unsigned i=0;i<8;++i)W(b,sprite+i*4,0x12340000+i);
    W(b,sprite,0x453b50);
    const auto sprite_before=std::array<uint32_t,8>{Word(b,sprite),Word(b,sprite+4),Word(b,sprite+8),Word(b,sprite+12),Word(b,sprite+16),Word(b,sprite+20),Word(b,sprite+24),Word(b,sprite+28)};
    unsigned reticle_checks=0;
    for(bool wide:{false,true})for(unsigned percent=50;percent<=200;++percent)
        for(float extent:{16.f,32.f,64.f,96.f}){
            trial_graphics::wide=wide;trial_graphics::reticle_percent=percent;rect();
            TrialHudLayoutScope h(c,b);TrialHudLayout(c,b);
            // Include off-center and already-expanded geometry: the game owns
            // placement/spread, while this setting scales its current extent.
            const float cx=wide?426.25f:319.5f,cy=241.5f;
            Put(b,a,cx-extent/2);Put(b,z,cx+extent/2);Put(b,a+4,cy-extent/2);Put(b,z+4,cy+extent/2);
            const uint32_t saved[]={Word(b,a),Word(b,a+4),Word(b,z),Word(b,z+4)};
            const unsigned csr=_mm_getcsr();_mm_setcsr(0xffc0);
            {
                TrialReticleScope r(c,b);assert(_mm_getcsr()==0xffc0);
                TrialHudQuadScope q(c,b);assert(_mm_getcsr()==0xffc0);
                // Expected-value arithmetic can set SSE sticky status bits.
                // Keep it separate from the exact scope-restoration checks.
                const unsigned validation_csr=_mm_getcsr();_mm_setcsr(0x1f80);
                const float scale=percent*.01f;
                assert(std::fabs((Real(b,z)-Real(b,a))-extent*scale*h.scale_)<.0002f);
                assert(std::fabs((Real(b,z+4)-Real(b,a+4))-extent*scale)<.0002f);
                assert(std::fabs((Real(b,a)+Real(b,z))*.5f-trial_view::HudX(cx,h.scale_))<.0002f);
                assert(std::fabs((Real(b,a+4)+Real(b,z+4))*.5f-cy)<.0002f);
                if(percent==100){
                    assert(Word(b,a+4)==saved[1]&&Word(b,z+4)==saved[3]);
                    assert(Real(b,a)==(wide?trial_view::HudX(cx-extent/2,h.scale_):cx-extent/2));
                }
                _mm_setcsr(validation_csr);
            }
            assert(_mm_getcsr()==0xffc0);_mm_setcsr(csr);
            assert(Word(b,a)==saved[0]&&Word(b,a+4)==saved[1]&&Word(b,z)==saved[2]&&Word(b,z+4)==saved[3]);
            for(unsigned i=0;i<8;++i)assert(Word(b,sprite+i*4)==sprite_before[i]);
            assert(!reticle);++reticle_checks;
        }
    trial_graphics::wide=false;trial_graphics::reticle_percent=200;rect();
    Put(b,a,287.5f);Put(b,z,351.5f);Put(b,a+4,207.5f);Put(b,z+4,271.5f);
    auto unchanged=[&]{TrialHudQuadScope q(c,b);assert(Real(b,a)==287.5f&&Real(b,z)==351.5f&&Real(b,a+4)==207.5f&&Real(b,z+4)==271.5f);};
    {TrialReticleScope r(c,b);unchanged();} // outside player HUD
    {
        TrialHudLayoutScope h(c,b);
        unchanged(); // ordinary HUD sprites / weapon prompts
        {TrialReticleScope r(c,b);
            W(b,c.esp,0x12345);unchanged();W(b,c.esp,0x7b149);
            W(b,sprite,0x453b80);unchanged();W(b,sprite,0x453b50);
            W(b,owner,0x4530bc);{TrialReticleScope sniper_mask(c,b);unchanged();} // scope-mask class
            W(b,owner,0x4530f8);{TrialReticleScope brackets(c,b);unchanged();}
            W(b,owner,0x453160);assert(reticle==&r);
            {TrialHudQuadScope q(c,b);assert(Real(b,a)==255.5f&&Real(b,z)==383.5f);}
        }
        for(unsigned invalid:{0u,49u,201u,0xffffffffu}){trial_graphics::reticle_percent=invalid;TrialReticleScope r(c,b);unchanged();}
        trial_graphics::reticle_percent=200;
        W(b,owner+0x90,0);{TrialReticleScope r(c,b);unchanged();}W(b,owner+0x90,tree);
        {TrialReticleScope r(c,b);
            // Reject overlapping points and nonfinite coordinates without writes.
            W(b,c.esp+8,a+4);unchanged();W(b,c.esp+8,z);
            W(b,a+4,0x7fc01234);{TrialHudQuadScope q(c,b);assert(Word(b,a+4)==0x7fc01234&&Real(b,a)==287.5f);}Put(b,a+4,207.5f);
            W(b,c.esp+8,0xfffffffcu);unchanged();W(b,c.esp+8,z);
            W(b,a,0x7f800000);{TrialHudQuadScope q(c,b);assert(Word(b,a)==0x7f800000&&Real(b,z)==351.5f);}Put(b,a,287.5f);
        }
    }
    assert(!hud&&!reticle);
    CheckWeaponProjection(b);
    CheckProjectedHud(b);
    std::printf("PASS player FOV/zoom, HUD aspect/anchors; %u reticle size/center/restoration cases, scope/menu/invalid exclusions\n",reticle_checks);
}
