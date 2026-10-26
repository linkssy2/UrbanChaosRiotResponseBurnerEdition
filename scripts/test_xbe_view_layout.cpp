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
static unsigned fov=53,fov2=53,reticle_percent=100,weapon_percent=100,weapon_percent2=100;static bool wide=true;
uint32_t FieldOfView(){return fov;}bool Widescreen(){return wide;}
uint32_t FieldOfView(unsigned player){return player==1?fov2:fov;}
uint32_t ReticleScale(){return reticle_percent;}
uint32_t WeaponFovScale(){return weapon_percent;}
uint32_t WeaponFovScale(unsigned player){return player==1?weapon_percent2:weapon_percent;}
}
static uint32_t coop_players[2]{};static unsigned presentation=0;
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?coop_players[slot]:0;}
unsigned TrialCoopPresentation(){return presentation;}
#include "../common/xbe_view_layout.cpp"
static void W(uint8_t*b,unsigned a,unsigned v){std::memcpy(b+a,&v,4);}
static void RegisterPlayer(uint8_t* b,uint32_t owner,uint32_t port,uint32_t id,uint32_t camera){
    W(b,owner+0x368,port);W(b,owner+0x54,id);W(b,owner+0x308,camera);
    X86Context c{};c.esi=owner;c.eax=camera;TrialPlayerCameraCreated(c,b);
}
static constexpr unsigned view=0x71000,model=0x72000;
static std::vector<std::array<float,16>> projections;
static unsigned model_draws;
static std::vector<std::array<float,5>> scope_borders;
static std::array<float,4> inset_rect{},notice_rect{};
static bool sample_notice=false,reward_notice=false;
static constexpr uint32_t notice_tween=0x6d000,notice_owner=0x6d100;
static constexpr uint32_t help_queue=0x62000,help_tween=0x62100,help_head=0x62300,help_node=0x62400;
static bool sample_help=false,throw_help=false;
X86_FUNC(sub_00077450);
X86_FUNC(sub_00072200);
X86_FUNC(sub_002739A0);
X86_FUNC(sub_0007AAE0);
X86_FUNC(sub_000656D0);
X86_FUNC(sub_000731E0);
X86_FUNC(sub_00273860);
X86_FUNC(NativeHelpQueue);
static bool retail_borders=false;
static std::vector<std::array<float,5>> queued_borders;
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t target,const uint32_t* p,size_t count){
    assert(target==0x656d0&&count==3);
    scope_borders.push_back({Real(memory.data(),p[0]),Real(memory.data(),p[0]+4),
        Real(memory.data(),p[1]),Real(memory.data(),p[1]+4),float(Word(memory.data(),p[2])>>24)});
    if(retail_borders){
        const X86Context saved=c;
        for(size_t i=count;i;--i){c.esp-=4;W(memory.data(),c.esp,p[i-1]);}
        c.esp-=4;W(memory.data(),c.esp,0);sub_000656D0(c,memory.data());c=saved;
    }else{c.eax=0xbad;c.esp-=40;}
    return 0;
}
}
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    if(target==0x701012){c.eax=1;c.esp+=8;return;} // HUD visibility service.
    if(target==0x273860){sub_00273860(c,b);return;}
    if(target==0x701007){
        const auto d=Word(b,c.esp+4);std::memcpy(inset_rect.data(),b+d,16);
        if(sample_notice){
            // Native callers discard the descriptor before fixed-position tween drawing.
            X86Context q=c;q.esp=0x6e000;q.ecx=notice_owner;
            W(b,q.esp,0);W(b,q.esp+4,d);const size_t before=queued_borders.size();
            if(reward_notice)sub_00077450(q,b);else sub_00072200(q,b);
            assert(q.esp==0x6e008&&queued_borders.size()==before+1);
            const auto& r=queued_borders.back();notice_rect={r[0],r[1],r[2],r[3]};
            assert(Real(b,notice_tween+0x20)==210&&Real(b,notice_tween+0x24)==83);
        }
        c.esp+=8;return;
    }
    if(target==0x731e0){sub_000731E0(c,b);return;}
    if(target==0x7aae0){sub_0007AAE0(c,b);return;}
    if(target==0x701008){c.eax=0x60f00;c.esp+=4;return;} // Scope mask size interface.
    if(target==0x701009){const auto out=Word(b,c.esp+4);Put(b,out,640);Put(b,out+4,480);c.eax=out;c.esp+=8;return;}
    if(target==0x701010){
        // Native centering is complete; intercept only mask artwork publication.
        const auto d=Word(b,c.esp+4);X86Context q=c;q.esp=0x6c800;
        W(b,q.esp,0);W(b,q.esp+4,d);W(b,q.esp+8,d+8);W(b,q.esp+12,0x6d200);
        sub_000656D0(q,b);assert(q.esp==0x6c804);c.esp+=8;return;
    }
    if(target==0x701004){c.eax=42;c.esp+=8;return;} // Untextured material lookup.
    if(target==0x701005){c.eax=0x6a200;c.esp+=4;return;} // Sprite queue.
    if(target==0x701006){
        TrialHudQuadScope quad(c,b);
        const auto a=Word(b,c.esp+4),z=Word(b,c.esp+8);
        queued_borders.push_back({Real(b,a),Real(b,a+4),Real(b,z),Real(b,z+4),float(Word(b,c.esp+12)>>24)});
        c.esp+=36;return;
    }
    if(target==0x701003){c.esp+=8;return;} // Scope's child mask renderer.
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
// GENERATED_TASER_DRAW
X86_FUNC(sub_000BE210){ctx.eax=0x69000;ctx.esp+=4;}
X86_FUNC(sub_000658C0);
bool TrialFrontendHideListSprites(){return false;}
X86_FUNC(sub_0022B210){ctx.eax=0x6a300;ctx.esp+=4;}
X86_FUNC(sub_002E0E00){ctx.eax=0x6a300;ctx.esp+=4;}
X86_FUNC(sub_002A7B50){ctx.eax=ctx.ecx;ctx.esp+=12;}
X86_FUNC(sub_002D666E){double value;std::memcpy(&value,base+ctx.esp+4,8);x87_push(ctx,std::floor(value));ctx.esp+=4;}
// List/animation advancement are service boundaries. Native callers and final
// fixed-coordinate tween drawing execute unchanged through the sprite queue.
X86_FUNC(sub_00273730){assert(ctx.ecx==(sample_help?help_tween:notice_tween)&&Real(base,ctx.esp+4)==.016f);ctx.esp+=8;}
X86_FUNC(sub_00274320){
    if(sample_help){NativeHelpQueue(ctx,base);return;}
    assert(ctx.ecx==notice_owner+(reward_notice?0x10:4)&&Real(base,ctx.esp+4)==.016f);
    ctx.ecx=notice_tween;sub_002739A0(ctx,base);
}
X86_FUNC(sub_00087100){W(base,ctx.ecx,0xffffffffu);ctx.eax=ctx.ecx;ctx.esp+=20;}
X86_FUNC(sub_00065760){assert(false);}
X86_FUNC(sub_0004A840){ctx.eax=0x62500;ctx.esp+=4;}
X86_FUNC(sub_002D5CCD){assert(false);}
X86_FUNC(sub_00017060){ctx.esp+=8;}
X86_FUNC(sub_00017160){ctx.esp+=12;}
X86_FUNC(sub_00017730){ctx.esp+=8;}
X86_FUNC(sub_00017180){ctx.esp+=12;}
X86_FUNC(sub_00018CE0){
    // Font shaping is a service boundary: use the native text draw's actual
    // X/Y arguments to emit a glyph-sized quad through the real sprite path.
    assert(sample_help);
    if(throw_help)throw 1;
    const float x=Real(base,ctx.esp+8),y=Real(base,ctx.esp+12);
    Put(base,0x62600,x);Put(base,0x62604,y);Put(base,0x62608,x+80);Put(base,0x6260c,y+12);
    W(base,0x62610,0xffffffffu);
    X86Context q=ctx;q.esp=0x6c000;
    W(base,q.esp,0);W(base,q.esp+4,0x62600);W(base,q.esp+8,0x62608);W(base,q.esp+12,0x62610);
    sub_000656D0(q,base);
    assert(Real(base,0x62600)==x&&Real(base,0x62604)==y&&Real(base,0x62608)==x+80&&Real(base,0x6260c)==y+12);
    ctx.esp+=32;
}
// GENERATED_REWARD_DRAW
// GENERATED_OBJECTIVE_DRAW
// GENERATED_RECT_QUEUE
// GENERATED_RECT_ROUND
// GENERATED_RECT_DRAW
// GENERATED_FULLSCREEN_RECT
// GENERATED_SCOPE_TINT
// GENERATED_SCOPE_COLOR
// GENERATED_SCOPE_DRAW
// GENERATED_DESCRIPTOR_COPY
// GENERATED_HUD_INSET
// GENERATED_NOTICE_TWEEN_DRAW
// GENERATED_CENTERED_SCOPE_CHILD
// GENERATED_HELP_QUEUE_DRAW
// GENERATED_HELP_TEXT_DRAW
// GENERATED_HELP_DRAW
// GENERATED_ORIGINAL_HELP_DRAW

static void CheckCoopHelp(uint8_t* b){
    const uint32_t root=0x61000,main=0x61100,six=0x61200;
    W(b,root,0x4531a0);W(b,root+8,main);W(b,main,0x453bd0);W(b,main+8,six);
    W(b,six,0x453c20);W(b,six+4,help_queue);W(b,help_queue,0x453580);
    W(b,help_queue+8,help_head);W(b,help_queue+12,1);Put(b,help_queue+16,.016f);
    W(b,help_head,help_node);W(b,help_node,help_head);W(b,help_node+4,help_head);W(b,help_node+8,help_tween);
    W(b,help_tween,0x48905c);W(b,0x489060,0x273860);Put(b,help_tween+8,1);b[help_tween+0x1c]=0;
    Put(b,help_tween+0x20,450);Put(b,help_tween+0x24,220);Put(b,help_tween+0x38,1);
    W(b,help_tween+0x60,0xffffffffu);Put(b,help_tween+0xac,50);Put(b,help_tween+0xb0,590);
    W(b,0x62500,0x62540);W(b,0x62574,0x701012);
    for(unsigned slot=0;slot<2;++slot){
        const auto actor=0x64000+slot*0x1000;coop_players[slot]=actor;
        RegisterPlayer(b,actor,slot,100+slot,200+slot);W(b,actor,0x46fbe8);W(b,actor+0x300,root);
    }
    sample_help=true;unsigned cases=0;
    for(bool reverse:{false,true})for(unsigned index=0;index<2;++index)for(bool wide:{false,true})for(unsigned mode:{0u,1u,2u}){
        const unsigned slot=reverse?1-index:index;const float top=float(slot)*240;
        trial_coop::active=mode!=0;presentation=mode;trial_graphics::wide=wide;
        W(b,root+4,coop_players[slot]);
        X86Context c{};c.ecx=root;c.esp=0x70000;const uint32_t d=c.esp+16;
        Put(b,d,0);Put(b,d+4,mode==1?top:0);Put(b,d+8,640);Put(b,d+12,mode==1?top+240:480);
        TrialHudLayoutScope h(c,b);TrialHudLayout(c,b);
        const auto descriptor=std::array<uint32_t,4>{Word(b,d),Word(b,d+4),Word(b,d+8),Word(b,d+12)};
        const auto tween_before=std::array<uint32_t,2>{Word(b,help_tween+0x20),Word(b,help_tween+0x24)};
        auto draw=[&](bool fixed){X86Context q{};q.ecx=help_queue;q.esp=0x71000;W(b,q.esp,0);W(b,q.esp+4,d);
            queued_borders.clear();if(fixed)sub_00077570(q,b);else OriginalHelpDraw(q,b);
            assert(q.esp==0x71008&&queued_borders.size()==1&&!h.help_);return queued_borders.back();};
        const auto original=draw(false),fixed=draw(true);
        assert(original[1]==219.5f&&original[3]==231.5f); // Negative witness: P2 is still in P1's half.
        const float dy=mode==1?top-120:0;
        assert(fixed[1]==original[1]+dy&&fixed[3]==original[3]+dy);
        assert(fixed[0]==original[0]&&fixed[2]==original[2]);
        if(mode==1)assert(fixed[1]>=top&&fixed[3]<top+240);
        assert(Word(b,help_tween+0x20)==tween_before[0]&&Word(b,help_tween+0x24)==tween_before[1]);
        for(unsigned i=0;i<4;++i)assert(Word(b,d+i*4)==descriptor[i]);
        if(mode==1){
            X86Context q{};q.ecx=help_queue;q.esp=0x71000;W(b,q.esp+4,d);
            {TrialHudHelpScope good(q,b);assert(h.help_);
                q.ecx=help_queue+0x10;{TrialHudHelpScope unrelated(q,b);assert(!h.help_);}assert(h.help_);
                X86Context quad{};quad.esp=0x6b000;W(b,quad.esp+4,0x62600);W(b,quad.esp+8,0x62604);
                Put(b,0x62600,450);Put(b,0x62604,220);Put(b,0x62608,530);
                {TrialHudQuadScope overlap(quad,b);assert(Real(b,0x62600)==450&&Real(b,0x62604)==220);}
                W(b,quad.esp+8,0x62608);W(b,0x62604,0x7fc01234);
                {TrialHudQuadScope invalid(quad,b);assert(Real(b,0x62600)==450&&Word(b,0x62604)==0x7fc01234);}
            }assert(!h.help_);
            W(b,six+4,help_queue+0x10);assert(draw(true)[1]==original[1]);W(b,six+4,help_queue);
            W(b,d+4,0x7fc01234);assert(draw(true)[1]==original[1]);W(b,d+4,descriptor[1]);
            Put(b,d+4,top+240);assert(draw(true)[1]==original[1]);W(b,d+4,descriptor[1]);
            throw_help=true;bool threw=false;try{draw(true);}catch(int){threw=true;}
            throw_help=false;assert(threw&&!h.help_&&hud==&h);
        }
        ++cases;
    }
    assert(!hud);
    X86Context q{};q.ecx=help_queue;q.esp=0x71000;W(b,q.esp,0);W(b,q.esp+4,0x70010);
    queued_borders.clear();sub_00077570(q,b);
    assert(q.esp==0x71008&&queued_borders.size()==1&&queued_borders[0][1]==219.5f);
    sample_help=false;trial_coop::active=false;presentation=0;trial_graphics::wide=true;
    std::printf("PASS %u native help queue/text-draw cases; per-view glyph placement/restoration, original negative witness; font shaping/artwork remain live verification\n",cases);
}

static void CheckCoopHud(uint8_t* b){
    const uint32_t root=0x60000,main=0x60100,pair=0x60200,corner=0x60300,notices=0x60400,
        notice_pair=0x60500,list=0x60600,rewards=0x60700,child=0x60800,vt=0x60900;
    W(b,root,0x4531a0);W(b,root+8,main);W(b,root+0x10,notices);
    W(b,main,0x453bd0);W(b,main+4,pair);W(b,pair,0x453bd0);W(b,pair+4,corner);
    W(b,corner,0x453cd0);W(b,corner+8,child);W(b,child,vt);W(b,vt+8,0x701007);
    W(b,notices,0x453cd0);W(b,notices+8,notice_pair);W(b,notice_pair,0x453bd0);
    W(b,notice_pair+4,list);W(b,notice_pair+8,rewards);W(b,list,0x453018);W(b,rewards,0x45356c);
    W(b,0x453bd0+8,0x701007);
    for(auto node:{corner,notices}){Put(b,node+12,48);Put(b,node+16,48);Put(b,node+20,36);Put(b,node+24,36);}
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t actor=0x64000+slot*0x1000;coop_players[slot]=actor;
        RegisterPlayer(b,actor,slot,100+slot,200+slot);W(b,actor,0x46fbe8);W(b,actor+0x300,root);
    }
    W(b,0x5150b0,0x6a000);W(b,0x6a000,0x6a100);W(b,0x6a1b4,0x701004);
    W(b,0x6a148,0x701005);W(b,0x6a200,0x6a210);W(b,0x6a214,0x701006);
    Put(b,0x44c1f8,.5f);Put(b,0x44bf94,1);Put(b,0x44c174,0);
    Put(b,notice_owner+0x10,.016f);Put(b,notice_owner+0x1c,.016f);
    Put(b,notice_tween+0x20,210);Put(b,notice_tween+0x24,83);
    Put(b,notice_tween+0x40,300);Put(b,notice_tween+0x44,44);
    Put(b,notice_tween+0x34,1);Put(b,notice_tween+0x38,1);
    W(b,notice_tween+0x3c,42);W(b,notice_tween+0x48,0xffffffffu);b[notice_tween+0x4c]=1;
    const uint32_t scope_owner=0x60b00,centered=0x60c00,mask=0x60d00,mask_vt=0x60e00,size=0x60f00;
    W(b,scope_owner,0x4530bc);W(b,scope_owner+8,centered);
    W(b,centered,0x453ae0);W(b,0x453ae8,0x7aae0);W(b,centered+4,mask);W(b,centered+8,0);
    W(b,mask,mask_vt);W(b,mask_vt+8,0x701010);W(b,mask_vt+12,0x701008);
    W(b,size,size+0x20);W(b,size+0x34,0x701009);W(b,0x6d200,0xffffffffu);
    Put(b,0x4505cc,255);Put(b,0x4523d4,640);Put(b,0x4523d0,480);
    for(unsigned i=0;i<4;++i)Put(b,0x690d0+i*4,1);
    unsigned cases=0;
    for(unsigned slot=0;slot<2;++slot)for(bool wide:{false,true})for(unsigned mode:{0u,1u,2u})for(bool reward:{false,true}){
        reward_notice=reward;
        trial_coop::active=mode!=0;presentation=mode;trial_graphics::wide=wide;
        const uint32_t actor=coop_players[slot];W(b,root+4,actor);
        X86Context c{};c.ecx=root;c.esp=0x70000;const uint32_t d=c.esp+0x10;
        const float top=float(slot)*240;
        Put(b,d,0);Put(b,d+4,top);Put(b,d+8,640);Put(b,d+12,top+240);
        TrialHudLayoutScope scope(c,b);TrialHudLayout(c,b);
        const bool changed=mode==1;
        assert(scope.split_==changed);assert(scope.scale_==(changed&&wide?.75f:1.f));
        const auto descriptor=std::array<float,4>{Real(b,d),Real(b,d+4),Real(b,d+8),Real(b,d+12)};
        auto draw=[&](uint32_t node){X86Context q{};q.ecx=node;q.esp=0x71000;W(b,q.esp,0);W(b,q.esp+4,d);sub_0007C1E0(q,b);};
        sample_notice=false;draw(corner);
        assert(inset_rect[0]==(changed?16:48)&&inset_rect[1]==top+(changed?12:36));
        assert(inset_rect[2]==descriptor[2]-(changed?16:48)&&inset_rect[3]==top+240-(changed?12:36));
        assert(Real(b,corner+12)==48&&Real(b,corner+20)==36);
        sample_notice=true;draw(notices);sample_notice=false;
        const float scale=changed&&wide?.75f:1.f;
        if(changed){
            assert(std::abs((notice_rect[0]+notice_rect[2])*.5f-319.5f)<.001f);
            assert(notice_rect[1]==top+41.5f&&notice_rect[3]==top+85.5f);
            assert(std::abs(notice_rect[2]-notice_rect[0]-300*scale)<.001f);
        }else assert(notice_rect[0]==209.5f&&notice_rect[1]==82.5f&&notice_rect[2]==509.5f);
        assert(!scope.notification_);
        for(unsigned i=0;i<4;++i)assert(Real(b,d+i*4)==descriptor[i]);
        // Other inset nodes, projected brackets and scope masks retain their native coordinates.
        W(b,0x60a00,0x453cd0);W(b,0x60a08,child);Put(b,0x60a0c,48);Put(b,0x60a10,48);Put(b,0x60a14,36);Put(b,0x60a18,36);
        draw(0x60a00);assert(inset_rect[0]==48&&inset_rect[1]==top+36);
        if(changed){
            // Execute native731E0 then native7AAE0: the fixed640-wide mask
            // remains centered in either half, despite the wider HUD canvas.
            W(b,scope_owner+4,actor);W(b,actor+0x35c,1);Put(b,actor+0x4dc,0);
            for(unsigned i=0;i<4;++i)Put(b,d+16+i*4,1);
            X86Context q{};q.ecx=scope_owner;q.esp=0x71000;W(b,q.esp,0);W(b,q.esp+4,d);
            queued_borders.clear();scope_borders.clear();sub_000731E0(q,b);
            assert(q.esp==0x71008&&queued_borders.size()==2&&scope_borders.empty());
            const auto& mask_rect=queued_borders.back();
            assert(mask_rect[0]==-.5f&&mask_rect[2]==639.5f);
            assert(mask_rect[1]==top-.5f&&mask_rect[3]==top+239.5f);
            for(unsigned i=0;i<4;++i)assert(Real(b,d+i*4)==descriptor[i]);
        }
        assert(hud==&scope);++cases;
    }
    trial_coop::active=false;presentation=0;trial_graphics::wide=true;
    std::printf("PASS %u original inset + notice caller/tween + scope centering cases; GPU text and artwork remain live verification\n",cases);
}

static void CheckScopeDraw(uint8_t* b){
    const unsigned owner=0x68000,descriptor=0x68100,player=0x68200,tree=0x68800;
    W(b,owner,0x4530bc);W(b,owner+4,player);W(b,owner+8,tree);
    W(b,tree,tree+16);W(b,tree+24,0x701003);W(b,player+0x35c,1);
    Put(b,0x44bf94,1);Put(b,0x4505cc,255);Put(b,player+0x4dc,0);
    for(unsigned i=0;i<4;++i){Put(b,descriptor+16+i*4,1);Put(b,0x690d0+i*4,1);}
    // Live scope uses 30% for the fullscreen dimmer, not for its opaque mask.
    Put(b,0x690dc,.3f);
    Put(b,descriptor,0);Put(b,descriptor+4,0);Put(b,descriptor+8,640/.75f);Put(b,descriptor+12,480);
    X86Context c{};c.ecx=owner;c.esp=0x70000;W(b,c.esp+4,descriptor);
    scope_borders.clear();
    W(b,0x5150b0,0x6a000);W(b,0x6a000,0x6a100);W(b,0x6a1b4,0x701004);
    W(b,0x6a148,0x701005);W(b,0x6a200,0x6a210);W(b,0x6a214,0x701006);
    Put(b,0x44c1f8,.5f);Put(b,0x4523d4,640);Put(b,0x4523d0,480);
    retail_borders=true;queued_borders.clear();
    TrialHudLayoutScope h(c,b);h.scale_=.75f;
    sub_000731E0(c,b);
    assert(c.esp==0x70008);
    assert(scope_borders.size()==2);
    assert(queued_borders.size()==3);
    assert(queued_borders[0][0]==-.5f&&queued_borders[0][2]==639.5f);
    assert(queued_borders[1][4]==255&&queued_borders[2][4]==255);
    assert(queued_borders[1][0]==-.5f&&queued_borders[2][2]>639);
    // The retail safe-area container passes an inset descriptor to its child.
    const unsigned inset=0x6b000;
    W(b,0x4530bc+8,0x731e0);W(b,inset+8,owner);
    Put(b,inset+12,48);Put(b,inset+16,48);Put(b,inset+20,36);Put(b,inset+24,36);
    unsigned cases=0;
    for(unsigned fov:{53u,90u})for(unsigned weapon:{100u,150u})for(unsigned reticle:{50u,100u})
        for(float dim:{.3f,1.f})for(float zoom_fade:{0.f,.5f,1.f})
        for(float fade:{0.f,.5f,1.f})for(float margin:{0.f,48.f}){
        trial_graphics::fov=fov;trial_graphics::weapon_percent=weapon;trial_graphics::reticle_percent=reticle;
        Put(b,inset+12,margin);Put(b,inset+16,margin);Put(b,player+0x4dc,zoom_fade);
        Put(b,0x690dc,dim);Put(b,descriptor+28,1-fade);
        c.ecx=inset;c.esp=0x70000;W(b,c.esp+4,descriptor);
        scope_borders.clear();queued_borders.clear();sub_0007C1E0(c,b);
        assert(scope_borders.size()==(fade==1?0:2)&&queued_borders.size()==scope_borders.size()+1);
        assert(queued_borders[0][0]==-.5f&&queued_borders[0][2]==639.5f);
        if(fade!=1){
            assert(queued_borders[1][0]==-.5f&&queued_borders[2][2]>=639.5f);
            assert(queued_borders[1][4]==(fade==0?255:128));
            assert(queued_borders[1][2]==79&&queued_borders[2][0]==559);
        }
        assert(c.esp==0x70008);++cases;
    }
    Put(b,player+0x4dc,0);Put(b,descriptor+28,1);
    for(bool off_center:{false,true}){
        h.scale_=off_center?.75f:1.f;Put(b,inset+12,off_center?47.f:48.f);Put(b,inset+16,48);
        c.ecx=inset;c.esp=0x70000;W(b,c.esp+4,descriptor);
        scope_borders.clear();queued_borders.clear();sub_0007C1E0(c,b);
        assert(scope_borders.empty()&&queued_borders.size()==1);
        assert(queued_borders[0][0]==-.5f&&queued_borders[0][2]==639.5f);
    }
    std::printf("PASS %u retail inset/scope/rectangle draw cases; fades, reported FOV/reticle settings, 4:3/off-center exclusions\n",cases);
    retail_borders=false;
}

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
    RegisterPlayer(b,0x79000,0,500,600);
    RegisterPlayer(b,0x7a000,1,501,601);
    W(b,0x79000,0x46fbe8);W(b,0x7a000,0x46fbe8);
    coop_players[0]=0x79000;coop_players[1]=0x7a000;
    TrialPlayerViewScope player_view;X86Context selected{};selected.ecx=0x79000;player_view.Select(selected,b);
    W(b,0x519f60,0x70000);W(b,0x70000,0x73000);W(b,0x73050,0xa2650);
    W(b,view,0x456d88);W(b,0x456d98,0xade90);W(b,0x456d9c,0x214e90);W(b,0x456db0,0xadf50);
    W(b,model,0x457880);W(b,model+0x60,0x180);Put(b,model+0x110,.1f);Put(b,model+0x114,.15f);
    Put(b,0x44bf94,1);
    const uint32_t destinations[]={0xbae4a,0xbaedb,0xbae9d,0xbaecc,0xbaeaa,0xbaebb,0xbae66};
    std::memcpy(b+0xbaf54,destinations,sizeof(destinations));
    unsigned checks=0;
    for(unsigned mode:{0u,1u,2u}){
    trial_coop::active=mode!=0;
    const unsigned slot=mode==2?1:0;selected.ecx=coop_players[slot];player_view.Select(selected,b);
    for(bool climbing:{false,true})for(float aspect:{4.f/3,16.f/9})for(float degrees:{38.f,48.f,53.f})
        for(unsigned percent=50;percent<=150;++percent)for(unsigned op=0;op<7;++op){
            trial_graphics::weapon_percent=slot?200-percent:percent;
            trial_graphics::weapon_percent2=slot?percent:200-percent;
            // Independent world FOV: changing it must never enter this path.
            trial_graphics::fov=(percent&1)?40:90;
            trial_graphics::fov2=(percent&1)?90:40;
            const float y=1.f/std::tan(degrees*float(3.14159265358979323846/360));
            std::array<float,16> alternate{y/aspect,0,0,0,0,y,0,0,0,0,1.0005f,1,0,0,-25.0125f,0};
            std::array<float,16> world{1,0,0,0,0,2,0,0,0,0,1.0005f,1,0,0,-25.0125f,0};
            world[5]=1.f/std::tan(trial_graphics::FieldOfView(slot)*float(3.14159265358979323846/360));
            world[0]=world[5]/aspect;
            Put(b,model+0x110,climbing?0.f:.1f);Put(b,model+0x114,climbing?1.f:.15f);
            X86Context ladder{};ladder.esi=coop_players[slot];ladder.eax=model;
            W(b,ladder.esi+0x170,climbing?11:0);TrialLadderHands(ladder,b);
            std::memcpy(b+view+0xd0,world.data(),64);std::memcpy(b+view+0x110,alternate.data(),64);
            std::array<uint8_t,0x118> model_before;std::memcpy(model_before.data(),b+model,model_before.size());
            X86Context c{};c.ecx=model;c.esp=0x80000;c.esi=11;c.edi=12;c.ebx=13;c.ebp=14;
            W(b,c.esp,0x12345);W(b,c.esp+4,0x74000);W(b,0x74000,op);W(b,0x74004,0x75000);
            projections.clear();model_draws=0;__imp__sub_000BAC10(c,b);
            assert(c.esp==0x80008&&c.esi==11&&c.edi==12&&c.ebx==13&&c.ebp==14);
            assert(model_draws==(op==1?0:1)&&projections.size()==2);
            auto expected=alternate;
            if(climbing){expected[0]=world[0];expected[5]=world[5];}
            else{
                expected[0]*=100.f/percent;expected[5]*=100.f/percent;
                for(unsigned row=0;row<4;++row)expected[row*4+2]=alternate[row*4+3]*.1f+alternate[row*4+2]*(.15f-.1f);
            }
            for(unsigned i=0;i<16;++i)assert(std::fabs(projections[0][i]-expected[i])<.00001f);
            assert(!std::memcmp(projections[1].data(),world.data(),64));
            assert(!std::memcmp(b+view+0xd0,world.data(),64)&&!std::memcmp(b+view+0x110,alternate.data(),64));
            assert(!std::memcmp(b+model,model_before.data(),model_before.size()));
            ++checks;
        }
    }
    trial_coop::active=false;selected.ecx=0x79000;player_view.Select(selected,b);
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
    // Leaving the ladder must immediately stop matching the retained arm model.
    trial_graphics::weapon_percent=150;W(b,0x79000+0x170,0);unchanged();
    W(b,0x79000+0x170,11);W(b,model,0x453160);unchanged();W(b,model,0x457880);
    X86Context reset{};reset.esi=0x79000;TrialLadderHands(reset,b);unchanged();
    Put(b,model+0x110,.1f);Put(b,model+0x114,.15f);
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
    std::printf("PASS %u original BAC10 weapon projections: SP/each co-op owner, independent weapon/world FOV, ladder, all draw operations and restoration\n",checks);
}
static void CheckTaser(uint8_t* b){
    constexpr uint32_t owner=0x40000,beam=0x42000,camera=0x43000,output=0x44000;
    RegisterPlayer(b,owner,0,501,601);W(b,owner+0x4fc,1);
    TrialPlayerViewScope player_view;X86Context selected{};selected.ecx=owner;player_view.Select(selected,b);
    for(unsigned i=0;i<3;++i)W(b,owner+0x814+i*4,101+i);
    W(b,beam,0x470058);const uint32_t ribbon=beam+0x80;
    for(unsigned i=0;i<21;++i){
        Put(b,ribbon+0x38+i*12,float(i+1));Put(b,ribbon+0x3c+i*12,2);Put(b,ribbon+0x40+i*12,100);
        Put(b,ribbon+0x134+i*12,1);Put(b,ribbon+0x138+i*12,0);Put(b,ribbon+0x13c+i*12,0);
    }
    Put(b,ribbon+0x234,1);Put(b,ribbon+0x240,21);Put(b,ribbon+0x254,1);
    Put(b,0x4700b8,.05f);Put(b,0x453424,1);Put(b,0x44bf94,1);Put(b,0x44c1f8,.5f);
    auto draw=[&]{
        X86Context c{};c.ecx=ribbon;c.esp=0x80000;W(b,c.esp,0);
        const uint32_t args[]={0,40,camera,output,0x45000,0x46000};
        std::memcpy(b+c.esp+4,args,sizeof(args));sub_00175820(c,b);assert(c.esp==0x8001c);
        std::array<float,360> result;std::memcpy(result.data(),b+output,sizeof(result));return result;
    };
    unsigned cases=0;
    for(float angle:{0.f,.7f,1.5f})for(unsigned fov:{40u,53u,70u,90u})for(unsigned percent:{50u,100u,150u}){
        const float cs=std::cos(angle),sn=std::sin(angle);
        const float cam[]={cs,0,-sn,0,0,1,0,0,sn,0,cs,0,3,4,5,1};std::memcpy(b+camera,cam,64);
        W(b,beam+0x54,101);players[0].weapon_effect_scale=0;auto original=draw();
        X86Context c{};c.esi=model;c.edi=view;c.ecx=view;c.esp=0x80000;
        W(b,model,0x457880);W(b,model+0x60,0x180);Put(b,model+0x110,.1f);Put(b,model+0x114,.15f);
        W(b,view,0x456d88);W(b,c.esp,c.esp+0x54);
        const float wy=1.f/std::tan(fov*float(3.14159265358979323846/360));
        const float gy=1.f/std::tan(53*float(3.14159265358979323846/360));
        Put(b,view+0xd0,wy*.75f);Put(b,view+0xe4,wy);
        std::memset(b+c.esp+0x54,0,64);Put(b,c.esp+0x54,gy*.75f);Put(b,c.esp+0x68,gy);
        trial_graphics::weapon_percent=percent;TrialWeaponProjection(c,b);
        const float ratio=gy*(100.f/percent)/wy;
        for(unsigned id:{101u,102u,103u,104u}){
            W(b,beam+0x54,id);std::array<uint8_t,0x300> before;std::memcpy(before.data(),b+beam,before.size());
            auto corrected=draw();assert(!std::memcmp(before.data(),b+beam,before.size()));
            for(unsigned i=0;i<120;++i){
                float old_view[3]{},new_view[3]{};
                for(unsigned row=0;row<3;++row)for(unsigned j=0;j<3;++j){
                    old_view[row]+=(original[i*3+j]-cam[12+j])*cam[row*4+j];
                    new_view[row]+=(corrected[i*3+j]-cam[12+j])*cam[row*4+j];
                }
                for(unsigned j=0;j<3;++j)assert(std::abs(new_view[j]-old_view[j]*(id<104&&j<2?ratio:1.f))<.0002f);
            }
            ++cases;
        }
        W(b,owner+0x4fc,2);W(b,beam+0x54,101);assert(draw()==original);W(b,owner+0x4fc,1);
    }
    players[0].owner=0;players[0].weapon_effect_scale=0;
    std::printf("PASS %u original taser ribbon draws: camera/weapon FOV alignment, rotated cameras, foreign effects and simulation unchanged\n",cases);
}
static void CheckPlayerViews(uint8_t* b){
    const uint32_t owners[]={0x48000,0x49000},cameras[]={0x4a000,0x4b000},models[]={0x4c000,0x4d000};
    trial_coop::active=true;
    for(unsigned i=0;i<2;++i){
        RegisterPlayer(b,owners[i],i,700+i,800+i);
        W(b,owners[i],0x46fbe8);W(b,owners[i]+0x5c,0);coop_players[i]=owners[i];
        W(b,cameras[i]+0x54,800+i);W(b,cameras[i]+0x58,700+i);W(b,cameras[i]+0x22c,700+i);
        W(b,cameras[i]+0x210,2);Put(b,cameras[i]+0x228,53);
        W(b,owners[i]+0x4fc,1);W(b,owners[i]+0x170,11);
        for(unsigned j=0;j<3;++j)W(b,owners[i]+0x814+j*4,100+i*100+j);
        W(b,models[i],0x457880);W(b,models[i]+0x60,0x180);
        X86Context ladder{};ladder.esi=owners[i];ladder.eax=models[i];TrialLadderHands(ladder,b);
    }
    trial_graphics::fov=80;trial_graphics::fov2=40;
    // Creating the second camera must not replace the first player's FOV owner.
    for(unsigned i:{1u,0u,1u,0u}){X86Context c{};c.ecx=cameras[i];assert(std::abs(TrialCameraTarget(c,b)-(i?40:80))<.0001f);}
    // A default P1 camera cannot bypass a nondefault P2 setting or alter zoom events.
    trial_graphics::fov=53;trial_graphics::fov2=90;
    for(unsigned i=0;i<2;++i){
        X86Context c{};c.ecx=cameras[i];Put(b,c.ecx+0x228,30);
        assert(std::abs(TrialCameraTarget(c,b)-trial_view::Zoom(30,i?90:53))<.0001f);
        assert(Real(b,c.ecx+0x228)==30);Put(b,c.ecx+0x228,53);
    }
    trial_graphics::fov=80;trial_graphics::fov2=40;
    X86Context second{};second.ecx=cameras[1];
    for(unsigned guard=0;guard<3;++guard){
        if(guard==0)trial_coop::active=false;
        if(guard==1)coop_players[1]=0;
        if(guard==2)W(b,owners[1]+0x5c,1);
        assert(std::abs(TrialCameraTarget(second,b)-80)<.0001f);
        trial_coop::active=true;coop_players[1]=owners[1];W(b,owners[1]+0x5c,0);
    }
    X86Context c{};c.esp=0x80000;c.edi=view;c.ecx=view;W(b,view,0x456d88);W(b,c.esp,c.esp+0x54);
    Put(b,view+0xd0,1);Put(b,view+0xe4,2);trial_graphics::weapon_percent=100;trial_graphics::weapon_percent2=50;
    auto matrix=[&](float scale){std::memset(b+c.esp+0x54,0,64);Put(b,c.esp+0x54,scale);Put(b,c.esp+0x68,scale*2);};
    TrialPlayerViewScope outer;X86Context selected{};selected.ecx=owners[0];outer.Select(selected,b);
    assert(view_player==owners[0]);
    for(unsigned i=0;i<2;++i){
        TrialPlayerViewScope inner;assert(!view_player);selected.ecx=owners[i];inner.Select(selected,b);
        for(unsigned j=0;j<2;++j){
            c.esi=models[j];Put(b,models[j]+0x110,0);Put(b,models[j]+0x114,1);matrix(4);
            TrialWeaponProjection(c,b);assert(Real(b,c.esp+0x54)==(i==j?1.f:4.f));
        }
        c.esi=models[i];Put(b,c.esi+0x110,.1f);Put(b,c.esi+0x114,.15f);matrix(float(i+2));
        TrialWeaponProjection(c,b);
        float scales[2];const uint64_t bits=players[i].weapon_effect_scale.load();std::memcpy(scales,&bits,8);
        const float expected=i?6.f:2.f;
        assert(scales[0]==expected&&scales[1]==expected);inner.Clear();assert(!view_player);
    }
    assert(view_player==owners[0]);
    {
        TrialPlayerViewScope inner;selected.ecx=owners[1];inner.Select(selected,b);
        c.esi=models[1];
        for(unsigned guard=0;guard<5;++guard){
            if(guard==0)trial_coop::active=false;
            if(guard==1)coop_players[1]=0;
            if(guard==2)W(b,owners[1],0x46fbe9);
            if(guard==3)W(b,owners[1]+0x5c,1);
            if(guard==4)W(b,owners[1]+0x54,0xffffffffu);
            matrix(3);TrialWeaponProjection(c,b);assert(Real(b,c.esp+0x54)==3);
            trial_coop::active=true;coop_players[1]=owners[1];W(b,owners[1],0x46fbe8);
            W(b,owners[1]+0x5c,0);W(b,owners[1]+0x54,701);
        }
        matrix(3);TrialWeaponProjection(c,b);assert(Real(b,c.esp+0x54)==6);
    }
    constexpr uint32_t beam=0x4e000,cam=0x50000,vertices=0x51000;
    W(b,beam,0x470058);const float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::memcpy(b+cam,identity,64);
    X86Context effect{};effect.ecx=beam+0x80;effect.esp=0x81000;
    W(b,effect.esp+4,0);W(b,effect.esp+8,1);W(b,effect.esp+12,cam);W(b,effect.esp+16,vertices);
    auto beam_draw=[&](unsigned owner,float expected){
        W(b,beam+0x54,100+owner*100);
        for(unsigned j=0;j<3;++j){Put(b,vertices+j*12,1);Put(b,vertices+j*12+4,2);Put(b,vertices+j*12+8,3);}
        {TrialTaserVertices corrected(effect,b);}
        for(unsigned j=0;j<3;++j){assert(Real(b,vertices+j*12)==expected);assert(Real(b,vertices+j*12+4)==expected*2);assert(Real(b,vertices+j*12+8)==3);}
    };
    beam_draw(0,2);beam_draw(1,1); // Remote beam remains in world projection.
    {TrialPlayerViewScope inner;selected.ecx=owners[1];inner.Select(selected,b);beam_draw(0,1);beam_draw(1,6);}
    beam_draw(0,2); // The nested second view restores the first view's ratio.
    // Reused or retired actor/camera IDs cannot inherit a previous player's correction.
    for(unsigned offset:{0x54u,0x308u,0x368u}){
        const uint32_t saved=Word(b,owners[0]+offset);W(b,owners[0]+offset,0xffffffffu);
        X86Context camera{};camera.ecx=cameras[0];assert(TrialCameraTarget(camera,b)==53);beam_draw(0,1);
        W(b,owners[0]+offset,saved);
    }
    outer.Clear();beam_draw(0,1);beam_draw(1,1);
    trial_coop::active=false;coop_players[0]=coop_players[1]=0;
    std::printf("PASS two original player/view identities: independent FOV, ladder models, taser ratios, foreign effects and nested/retired owner isolation\n");
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
    CheckTaser(b);
    CheckProjectedHud(b);
    CheckCoopHud(b);
    CheckCoopHelp(b);
    CheckScopeDraw(b);scope_borders.clear();
    {
        TrialHudLayoutScope h(c,b);h.scale_=.75f;c.esi=0x68000;c.edi=0x68100;c.esp=0x70000;
        W(b,c.esi,0x4530bc);Put(b,c.edi,0);Put(b,c.edi+4,0);Put(b,c.edi+8,640/.75f);Put(b,c.edi+12,480);
        Put(b,c.edi+28,.5f);Put(b,c.esp+28,.3f);const X86Context saved=c;TrialScopeBorders(c,b);
        assert(!std::memcmp(&saved,&c,sizeof(c))&&scope_borders.size()==2);
        assert(scope_borders[0][0]==0&&std::abs(scope_borders[0][2]-106.6667f)<.01f&&scope_borders[0][4]==128);
        assert(std::abs(scope_borders[1][0]-746.6667f)<.01f&&scope_borders[1][2]==std::ceil(640/.75f));
        h.scale_=1;TrialScopeBorders(c,b);assert(scope_borders.size()==2);
        h.scale_=.75f;Put(b,c.edi+28,0);TrialScopeBorders(c,b);assert(scope_borders.size()==2);
    }
    std::printf("PASS player FOV/zoom, HUD aspect/anchors; %u reticle size/center/restoration cases, scope/menu/invalid exclusions\n",reticle_checks);
    CheckPlayerViews(b);
}


