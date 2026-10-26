// Retail registration, command publication/selection/completion, property reads
// and orders HUD dispatch. Allocation, sprite resources and event delivery are
// explicit fixture boundaries; no claim of live NPC animation or pixels.
// Native 156E60 also executes its own actor look transform, ray construction,
// occlusion decision and target publication. Actor property/scene ray queries
// remain boundaries with distinct camera transforms and geometry for each view.
// HelpPrompt publication/cancellation and medic eligibility execute native code;
// text-resource construction and list allocation/destruction are boundaries.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
constexpr uint32_t P[]={0x600000,0x602000},M[]={0x604000,0x605000},H[]={0x606000,0x607000};
constexpr uint32_t W=0x608000,STATE=0x609000,C=0x60c000,VT=0x60d000,DATA=0x60e000,SP=0x780000;
constexpr uint32_t NPC[]={C,0x60f000},HELP[]={0x610000,0x611000};
static uint32_t queues[2];static float health[2];static bool medic_present,orders_available[2],throw_text;
static unsigned clears[2];
static uint32_t admitted[2]={P[0],P[1]},heap;static bool candidate,fail_copy;
static unsigned attached,chosen,complete[2],drawn[2],updated[2],made_sprites[2];
static uint32_t control_owner;
static std::array<std::array<float,16>,2> look_matrices;
static std::array<std::array<float,3>,2> target_positions;
static unsigned look_queries[2],filter_queries[2],ray_queries[2],marks[2],target_types[2];
static bool look_available[2],target_available[2],blocked[2],selectable_available;
uint8_t* B(){return kernel::g_base;}
uint32_t Read(uint32_t a){uint32_t v;std::memcpy(&v,B()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(B()+a,&v,4);}
float Real(uint32_t a){float v;std::memcpy(&v,B()+a,4);return v;}
void Float(uint32_t a,float v){std::memcpy(B()+a,&v,4);}
uint32_t TrialCoopPlayer(unsigned s){return s<2?admitted[s]:0;}
#include "../common/xbe_coop_orders.cpp"
// ORIGINAL_DECLARATIONS
static uint32_t Allocate(uint32_t bytes){const uint32_t p=heap;heap+=(bytes+15)&~15u;assert(heap<0x700000);return p;}
static uint32_t Find(uint32_t m,uint32_t id){const uint32_t h=Read(m+0xac);for(uint32_t n=Read(h);n!=h;n=Read(n))if(Read(n+8)==id)return n;return 0;}
static unsigned Owner(uint32_t m){assert(m==M[0]||m==M[1]);return m==M[1];}
static void RemoveSprite(uint32_t m,uint32_t id){const auto h=Read(m+0xbc);for(auto n=Read(h);n!=h;n=Read(n))if(Read(n+8)==id){Put(Read(n+4),Read(n));Put(Read(n)+4,Read(n+4));Put(m+0xc0,Read(m+0xc0)-1);return;}}
static void RetireClient(uint32_t m,uint32_t id){const auto n=Find(m,id);if(!n)return;for(auto a=Read(n+0x10);a<Read(n+0x14);a+=0x1c)RemoveSprite(m,Read(a+0x14));Put(Read(n+4),Read(n));Put(Read(n)+4,Read(n+4));Put(m+0xb0,Read(m+0xb0)-1);}
static void Invoke(uint32_t fn,uint32_t self,std::initializer_list<uint32_t> args);

// Help-list construction/insert/clear execute the pinned original callers;
// allocator and text-resource creation are explicit boundary stubs.
X86_FUNC(sub_002C3740){const auto n=Allocate(12);Put(n,n);Put(n+4,n);ctx.eax=n;ctx.esp+=4;}
X86_FUNC(sub_002BCFA0){const auto n=Allocate(12);Put(n,Read(ctx.esp+4));Put(n+4,Read(ctx.esp+8));Put(n+8,Read(Read(ctx.esp+12)));ctx.eax=n;ctx.esp+=16;}
X86_FUNC(sub_000722A0){Put(ctx.ecx+8,Read(ctx.ecx+8)+Read(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_00274300){const auto q=ctx.ecx-4;assert(q==queues[0]||q==queues[1]);++clears[q==queues[1]];const auto h=Read(q+8);Put(h,h);Put(h+4,h);Put(q+12,0);ctx.esp+=4;}
X86_FUNC(sub_00072A80){if(candidate)Coop_00072A80(ctx,base);else Original_00072A80(ctx,base);}
X86_FUNC(sub_00073090){if(candidate)Coop_00073090(ctx,base);else Original_00073090(ctx,base);}
X86_FUNC(sub_000643E0){ctx.eax=DATA+0x500;ctx.esp+=4;}
X86_FUNC(sub_001F9010){x87_push(ctx,.016);ctx.esp+=4;}
X86_FUNC(sub_001F7F00){ctx.eax=medic_present&&Read(ctx.esp+4)==0x46b548?300:0xffffffff;ctx.esp+=4;}
X86_FUNC(sub_00140680){
    if(throw_text)throw std::runtime_error("text allocation boundary");
    const auto text=Read(ctx.esp+4);const float y=Real(ctx.esp+8);
    if(text){auto nested=ctx;nested.esp-=128;Put(nested.esp,0xbeef);Put(nested.esp+4,text);sub_00073090(nested,base);assert(nested.esp==ctx.esp-124);}
    x87_push(ctx,y+16);ctx.esp+=12;
}
X86_FUNC(sub_001407F0){if(candidate)Coop_001407F0(ctx,base);else Original_001407F0(ctx,base);}
X86_FUNC(sub_00140E80){if(candidate)Coop_00140E80(ctx,base);else Original_00140E80(ctx,base);}
X86_FUNC(sub_00140EB0){if(candidate)Coop_00140FB0(ctx,base);else Original_00140FB0(ctx,base);}
X86_FUNC(sub_001411A0){if(candidate)Coop_001411A0(ctx,base);else Original_001411A0(ctx,base);}

X86_FUNC(sub_00158E60){
    const uint32_t n=Allocate(0x20),next=Read(ctx.esp+4),prev=Read(ctx.esp+8),data=Read(ctx.esp+12);
    Put(n,next);Put(n+4,prev);Put(n+8,Read(data));const auto actions=Allocate(0x400);
    Put(n+0x10,actions);Put(n+0x14,actions);Put(n+0x18,actions+0x400);ctx.eax=n;ctx.esp+=16;
}
X86_FUNC(sub_001578E0){Put(ctx.ecx+8,Read(ctx.ecx+8)+Read(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_001572A0){
    const auto cmd=Read(ctx.esp+4);ctx.eax=0;
    for(auto a=Read(ctx.ecx+8);a<Read(ctx.ecx+12);a+=0x1c)if(Read(a+4)==cmd)ctx.eax=1;
    ctx.esp+=8;
}
X86_FUNC(sub_001582F0){
    const auto m=ctx.ecx-0xb4;const unsigned slot=Owner(m);const auto id=++made_sprites[slot];
    const auto head=Read(m+0xbc),node=Allocate(0x18),sprite=Allocate(0x10);
    Put(node,head);Put(node+4,Read(head+4));Put(Read(head+4),node);Put(head+4,node);Put(node+8,id);Put(node+0x10,sprite);
    Put(sprite,VT+0x100);Put(sprite+4,slot);Put(sprite+8,Read(Read(ctx.esp+4)));Put(m+0xc0,Read(m+0xc0)+1);
    ctx.eax=id;ctx.esp+=16;
}
X86_FUNC(sub_00158480){const auto end=Read(ctx.ecx+8);assert(end+0x1c<=Read(ctx.ecx+12));std::memcpy(B()+end,B()+Read(ctx.esp+4),0x1c);Put(ctx.ecx+8,end+0x1c);ctx.esp+=8;}
X86_FUNC(sub_001581C0){
    const auto node=Find(ctx.ecx,Read(ctx.esp+4)),command=Read(ctx.esp+8);assert(node);
    const auto end=Read(node+0x14);for(auto a=Read(node+0x10);a<end;a+=0x1c)if(Read(a+4)==command){RemoveSprite(ctx.ecx,Read(a+0x14));std::memmove(B()+a,B()+a+0x1c,end-a-0x1c);Put(node+0x14,end-0x1c);break;}ctx.esp+=12;
}
X86_FUNC(sub_00158FB0){RetireClient(ctx.ecx,Read(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_0004A840){ctx.eax=DATA;ctx.esp+=4;}
X86_FUNC(sub_0004ACD0){ctx.eax=100;ctx.esp+=8;}
X86_FUNC(sub_001F7AA0){
    const auto id=Read(ctx.esp+4),name=Read(ctx.esp+8);ctx.eax=0;
    if(id==100||id==101){
        if(name==0x45819c)ctx.eax=orders_available[id-100];
        else if(name!=0x458184){control_owner=id;ctx.eax=!B()[P[id-100]+0x202];}
    }else{
        assert(id==300&&(name==0x46b53c||name==0x46b530));
        auto nested=ctx;nested.ecx=C;nested.esp-=128;Put(nested.esp,0xbeef);
        if(name==0x46b53c){if(candidate)Coop_00168210(nested,base);else Original_00168210(nested,base);}
        else {if(candidate)Coop_00168170(nested,base);else Original_00168170(nested,base);}
        ctx.eax=nested.eax;
    }
    ctx.esp+=4;
}
X86_FUNC(sub_001F7A70){
    const auto id=Read(ctx.esp+4),name=Read(ctx.esp+8),out=Read(ctx.esp+12);
    if(id==100||id==101){
        const unsigned slot=id-100;
        if(name==0x4581c0){Put(out,2);Float(out+4,health[slot]);ctx.eax=1;}else if(name==0x44d94c){
            assert(!std::strcmp(reinterpret_cast<const char*>(B()+name),"Look point"));
            ++look_queries[slot];ctx.eax=look_available[slot];
            if(ctx.eax)std::memcpy(B()+out+0x20,look_matrices[slot].data(),64);
        }else{
            assert(name==0x46ca34&&!std::strcmp(reinterpret_cast<const char*>(B()+name),"Collision filter"));
            ++filter_queries[slot];Put(out,1);Put(out+4,0x1110+slot);ctx.eax=1;
        }
        ctx.esp+=4;return;
    }
    if(id==300||id==301){
        assert(name==0x465b00&&!std::strcmp(reinterpret_cast<const char*>(B()+name),"object type"));
        ctx.eax=target_available[id-300];if(ctx.eax){Put(out,1);Put(out+4,target_types[id-300]);}
        ctx.esp+=4;return;
    }
    assert(id==200||id==201);
    auto nested=ctx;nested.ecx=M[id-200];nested.esp-=32;Put(nested.esp,0xbeef);Put(nested.esp+4,name);Put(nested.esp+8,out);
    if(candidate)Coop_00156C50(nested,base);else Original_00156C50(nested,base);ctx.eax=nested.eax;ctx.esp+=4;
}
// Scene boundaries consume the native routine's actual ray, never a seeded
// manager target. Their two camera origins deliberately identify different rays.
static unsigned RayOwner(uint32_t origin){
    const float x=Real(origin);assert(x==0.f||x==1000.f);return x==1000.f;
}
X86_FUNC(sub_00214230){
    const auto origin=Read(ctx.esp+4),end=Read(ctx.esp+8);const unsigned slot=RayOwner(origin);
    assert(Read(ctx.esp+12)==9&&Read(ctx.esp+16)==0x1110+slot&&Read(ctx.esp+20)==0x200&&Read(ctx.esp+24)==0);
    ++ray_queries[slot];const auto& matrix=look_matrices[slot];const float reach=Real(0x44db94);
    for(unsigned axis=0;axis<3;++axis){
        assert(Real(origin+4*axis)==matrix[12+axis]);
        assert(std::fabs(Real(end+4*axis)-(matrix[12+axis]+reach*matrix[8+axis]))<0.01f);
    }
    ctx.eax=0;
    if(blocked[slot]){
        ctx.eax=DATA+0x300;for(unsigned axis=0;axis<3;++axis)Float(ctx.eax+4*axis,matrix[12+axis]+50.f*matrix[8+axis]);
        Put(ctx.eax+0x2c,0); // A wall, not the selectable NPC behind it.
    }
    ctx.esp+=4;
}
X86_FUNC(sub_001F7EA0){
    assert(Read(ctx.esp+4)==0x465ae0&&!std::strcmp(reinterpret_cast<const char*>(B()+0x465ae0),"SELECTABLEITEMS"));
    Put(ctx.ecx,0x123abcd);ctx.eax=selectable_available;ctx.esp+=8;
}
X86_FUNC(sub_00045490){
    const auto origin=Read(ctx.esp+4),direction=Read(ctx.esp+8),list=Read(ctx.esp+12),distance=Read(ctx.esp+16);
    const unsigned slot=RayOwner(origin);assert(Read(list)==0x123abcd);
    for(unsigned axis=0;axis<3;++axis)assert(std::fabs(Real(direction+axis*4)-look_matrices[slot][8+axis])<0.00001f);
    float nearest=10000.f;ctx.eax=0;
    for(unsigned target=0;target<2;++target){
        float along=0,square=0;for(unsigned axis=0;axis<3;++axis){const float delta=target_positions[target][axis]-Real(origin+axis*4);along+=delta*Real(direction+axis*4);square+=delta*delta;}
        if(along>0&&along<nearest&&square-along*along<1.f){nearest=along;ctx.eax=NPC[target];}
    }
    Float(distance,nearest);ctx.esp+=4;
}
X86_FUNC(sub_0020A140){assert(ctx.ecx==NPC[0]||ctx.ecx==NPC[1]);assert(Read(ctx.esp+4)==0x1000000);++marks[ctx.ecx==NPC[1]];ctx.esp+=8;}
X86_FUNC(sub_00159150){if(candidate)Coop_00159150(ctx,base);else Original_00159150(ctx,base);}
X86_FUNC(sub_00158EF0){Coop_00158EF0(ctx,base);}
X86_FUNC(sub_00158D90){Coop_00158D90(ctx,base);}
X86_FUNC(sub_001F90A0){
    const auto id=Read(ctx.esp+4),event=Read(ctx.esp+8),payload=Read(ctx.esp+12),sender=Read(ctx.esp+20);
    const auto keep=ctx;X86Context delivery=ctx;delivery.esp-=128;Put(delivery.esp,0xbeef);Put(delivery.esp+4,payload);Put(delivery.esp+8,sender);
    if(event==130){assert(id==300);delivery.ecx=C;Coop_00040D40(delivery,base);}
    else{assert(event==136&&(id==200||id==201));++complete[id-200];}
    ctx=keep;ctx.esp+=24;
}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    switch(fn){
    case 0x140860:if(candidate)Coop_00140860(c,b);else Original_00140860(c,b);return;
    case 0x140e10:if(candidate)Coop_00140E10(c,b);else Original_00140E10(c,b);return;
    case 0x1407f0:sub_001407F0(c,b);return;
    case 0x77500:Coop_00077500(c,b);return;
    case 0x159150:if(fail_copy)throw std::runtime_error("allocation boundary");sub_00159150(c,b);return;
    case 0x158f30:if(candidate)Coop_00158F30(c,b);else Original_00158F30(c,b);return;
    case 0x1583e0:if(candidate)Coop_001583E0(c,b);else Original_001583E0(c,b);return;
    case 0x159120:if(candidate)Coop_00159120(c,b);else Original_00159120(c,b);return;
    case 0x157640:Coop_00157640(c,b);return;
    case 0x157670:Coop_00157670(c,b);return;
    case 0xf001:{ // Subclass attached: publish one original action, once.
        assert(c.ecx==C);++attached;const auto saved=c;
        const auto scratch=c.esp-256;Put(scratch,7);Put(scratch+4,0x465074);Put(scratch+8,0);Put(scratch+12,0x123abc);Put(scratch+16,1);Put(scratch+20,300);
        c.esp-=32;Put(c.esp,0xbeef);Put(c.esp+4,scratch);Put(c.esp+8,scratch+20);c.ecx=M[Read(C+0x98)-200];
        if(candidate)Coop_00158F30(c,b);else Original_00158F30(c,b);
        c=saved;c.esp+=4;return;
    }
    case 0xf002:{
        assert(c.ecx==C&&Read(c.esp+4)==7);++chosen;
        auto selected=c;selected.esp-=32;Put(selected.esp,0xbeef);Coop_0019BEA0(selected,b);
        c.esp+=8;return;
    }
    case 0xf003:assert(Read(c.ecx+4)<2);++updated[Read(c.ecx+4)];c.esp+=8;return;
    case 0xf004:assert(Read(c.ecx+4)<2);++drawn[Read(c.ecx+4)];c.esp+=8;return;
    default:std::fprintf(stderr,"Unexpected guest call %08X ecx=%08X eax=%08X sp=%08X return=%08X\n",fn,c.ecx,c.eax,c.esp,Read(c.esp));std::abort();
    }
}
namespace kernel {
// PRODUCTION_CALL_GUEST
}
// ORIGINAL_BODIES

static X86Context Context(uint32_t self){X86Context c{};c.ecx=self;c.esp=SP;c.ebp=0x1111;c.esi=0x2222;c.edi=0x3333;c.ebx=0x4444;Put(SP,0xbeef);return c;}
static void Identity(uint32_t p,uint32_t id){Put(p+0x54,id);const auto e=STATE+0x3c+(id&1023)*8;Put(e,id);Put(e+4,p);}
static void Seed(){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::SetAvailable(true);assert(trial_coop::Request());trial_coop::SetActive(true);
    candidate=true;fail_copy=false;throw_text=medic_present=false;attached=chosen=0;heap=0x620000;
    selectable_available=true;target_positions={{{0,0,100},{1000,0,-100}}};
    for(unsigned i=0;i<2;++i){look_queries[i]=filter_queries[i]=ray_queries[i]=marks[i]=0;target_types[i]=3;
        look_available[i]=target_available[i]=true;blocked[i]=false;look_matrices[i]={};
        look_matrices[i][0]=look_matrices[i][5]=look_matrices[i][15]=1;look_matrices[i][10]=i?-1.f:1.f;look_matrices[i][12]=i?1000.f:0.f;}
    Put(0x51c4dc,W);Put(W+4,STATE);Put(0x51c398,DATA);Put(DATA+8,DATA+0x20);Put(0x4f9ed4,130);Put(0x4f9eec,136);
    Put(C,VT);Identity(C,300);Put(C+0x98,0xffffffff);B()[C+0xa0]=1;Put(VT+0x64,0xf001);Put(VT+0x68,0xf002);
    Identity(NPC[1],301);
    Put(VT+0x104,0xf003);Put(VT+0x108,0xf004);Put(0x46ca64+4,0x157640);Put(0x46ca64+8,0x157670);
    for(unsigned slot=0;slot<2;++slot){
        admitted[slot]=P[slot];complete[slot]=drawn[slot]=updated[slot]=made_sprites[slot]=0;
        Put(P[slot],0x46fbe8);Identity(P[slot],100+slot);Put(P[slot]+0x368,slot);Put(P[slot]+0x7ec,200+slot);Put(P[slot]+0x300,H[slot]);
        Put(M[slot],0x46caa8);Identity(M[slot],200+slot);Put(M[slot]+0x58,100+slot);Put(M[slot]+0x98,slot);
        const auto head=Allocate(0x10);Put(head,head);Put(head+4,head);Put(M[slot]+0xac,head);
        const auto sprites=Allocate(0x10);Put(sprites,sprites);Put(sprites+4,sprites);Put(M[slot]+0xbc,sprites);Put(M[slot]+0xb4,0x46ca64);
        Put(H[slot],0x4531a0);Put(H[slot]+4,P[slot]);
        const auto pair=Allocate(12),six=Allocate(0x1c);queues[slot]=Allocate(0x20);
        Put(H[slot]+8,pair);Put(pair,0x453bd0);Put(pair+8,six);Put(six,0x453c20);Put(six+4,queues[slot]);
        auto ctor=Context(queues[slot]);Coop_00077500(ctor,B());assert(ctor.esp==SP+4);
        Put(HELP[slot],0x46b4d0);Identity(HELP[slot],500+slot);Put(HELP[slot]+0x58,100+slot);Put(HELP[slot]+0x80,0x25);
        Put(P[slot]+0x7f0,500+slot);health[slot]=100;orders_available[slot]=false;clears[slot]=0;
        Put(M[slot]+0xec,400+slot);Put(M[slot]+0xe8,3);
    }
    Put(0x516124,M[1]+0xb4);
    // Resource IDs normally come from 3A7D20/string interning. Text construction
    // is a fixture boundary; use distinct IDs for each native table index.
    for(unsigned i=0;i<0x25;++i)Put(0x51bf50+i*8,0x46b660+i);
    // ORIGINAL_CONSTANTS
}
static void Invoke(uint32_t fn,uint32_t self,std::initializer_list<uint32_t> args){
    auto c=Context(self);uint32_t a=SP+4;for(auto value:args){Put(a,value);a+=4;}
    const unsigned csr=_mm_getcsr();x86_icall(c,B(),fn);
    assert(c.esp==SP+4+args.size()*4&&c.ebp==0x1111&&c.esi==0x2222&&c.edi==0x3333&&c.ebx==0x4444&&_mm_getcsr()==csr);
}
static void Register(unsigned owner){Invoke(0x159150,M[owner],{300});}
static void Hud(unsigned owner,bool fixed){
    auto root=Context(H[owner]);TrialCoopOrdersHudScope scope(root,B());
    auto c=Context(0);Put(SP+4,DATA);
    if(fixed)Coop_00071920(c,B());else Original_00071920(c,B());assert(c.esp==SP+8);
    c=Context(0);Put(SP+4,DATA);if(fixed)Coop_00071900(c,B());else Original_00071900(c,B());assert(c.esp==SP+8);
}
static uint32_t Look(unsigned query_owner,unsigned stored_owner,bool fixed){
    auto update=Context(M[query_owner]);TrialCoopOrdersScope scope(update,B());
    auto c=Context(M[stored_owner]);Put(SP+4,0x465074);Put(SP+8,DATA+0x100);
    if(fixed)Coop_00156C50(c,B());else Original_00156C50(c,B());assert(c.esp==SP+12&&Read(DATA+0x100)==1);
    return Read(DATA+0x104);
}
static void ComputeLook(unsigned slot){
    const uint32_t other_id=Read(M[1-slot]+0xec),other_type=Read(M[1-slot]+0xe8);
    auto c=Context(M[slot]);{
        TrialCoopOrdersScope scope(c,B());Coop_00156E60(c,B());
    }
    assert(c.esp==SP+4&&c.ebp==0x1111&&c.esi==0x2222&&c.edi==0x3333&&c.ebx==0x4444);
    assert(Read(M[1-slot]+0xec)==other_id&&Read(M[1-slot]+0xe8)==other_type);
}
static bool Eligible(unsigned slot){
    auto update=Context(M[slot]);TrialCoopOrdersScope scope(update,B());
    auto c=Context(C);Coop_00168170(c,B());
    assert(c.esp==SP+4&&c.ebp==0x1111&&c.esi==0x2222&&c.edi==0x3333&&c.ebx==0x4444);
    return uint8_t(c.eax)!=0;
}
static void NativeLookSelection(){
    for(unsigned first=0;first<2;++first){
        Seed();Register(0);ComputeLook(first);ComputeLook(1-first);
        for(unsigned slot=0;slot<2;++slot){
            assert(look_queries[slot]==1&&filter_queries[slot]==1&&ray_queries[slot]==1&&marks[slot]==1);
            assert(Read(M[slot]+0xec)==300+slot&&Read(M[slot]+0xe8)==3&&Eligible(slot));
        }
        // Original client properties see its saved owner's ray, even when the
        // other manager is selecting. The scoped hook sees the actual new ray.
        assert(Look(1,0,false)==300&&Look(1,0,true)==301&&Read(C+0x98)==200);
        // An outstanding action for one target must not steal/reject the other
        // player's different target. This executes the native busy predicate.
        Put(C+0xb4,3);Put(C+0xb8,300);assert(!Eligible(0)&&Eligible(1));
        Put(C+0xb8,301);assert(Eligible(0)&&!Eligible(1));Put(C+0xb4,0);
        target_types[1]=1;ComputeLook(1);assert(!Eligible(1)&&Eligible(0));
        target_types[1]=3;ComputeLook(1);assert(Eligible(1));
        // Looking away computes a miss and clears only that viewer's result.
        look_matrices[0][8]=1;look_matrices[0][10]=0;ComputeLook(0);
        assert(Read(M[0]+0xec)==0xffffffff&&Read(M[0]+0xe8)==0&&!Eligible(0));
        assert(Read(M[1]+0xec)==301&&Eligible(1));
        look_matrices[0][8]=0;look_matrices[0][10]=1;blocked[0]=true;ComputeLook(0);
        assert(Read(M[0]+0xec)==0xffffffff&&!Eligible(0)&&Eligible(1));
        blocked[0]=false;ComputeLook(0);assert(Read(M[0]+0xec)==300&&Eligible(0));
        // Failed actor/target queries and an unavailable selectable list cannot
        // retain a previous prompt target in that manager.
        look_available[0]=false;const auto rays=ray_queries[0];ComputeLook(0);
        assert(ray_queries[0]==rays&&Read(M[0]+0xec)==0xffffffff&&!Eligible(0));
        look_available[0]=true;target_available[0]=false;ComputeLook(0);
        assert(Read(M[0]+0xec)==0xffffffff&&!Eligible(0));target_available[0]=true;
        selectable_available=false;ComputeLook(0);assert(Read(M[0]+0xec)==0xffffffff&&!Eligible(0));
        selectable_available=true;ComputeLook(0);assert(Eligible(0)&&Eligible(1));
    }
    Seed();ComputeLook(0);assert(!look_queries[0]&&!ray_queries[0]&&Read(M[0]+0xec)==0xffffffff);
    std::puts("orders look: original actor transform/ray construction, separate targets, both update orders, occlusion/misses, native busy/type eligibility and stale-target clearing passed (scene queries are fixture boundaries)");
}
static unsigned HelpCount(unsigned slot){return Read(queues[slot]+12);}
static void HelpUpdate(unsigned slot){Invoke(0x140e10,HELP[slot],{});}
static bool HealPlayer(unsigned slot,bool scoped=true,bool fixed=true){
    auto c=Context(C);const auto saved=c;
    if(scoped){auto helper=Context(HELP[slot]);TrialCoopHelpScope scope(helper,B());if(fixed)Coop_00168210(c,B());else Original_00168210(c,B());}
    else {if(fixed)Coop_00168210(c,B());else Original_00168210(c,B());}
    assert(c.esp==SP+4&&c.esi==saved.esi&&c.edi==saved.edi&&c.ebp==saved.ebp&&c.ebx==saved.ebx);return uint8_t(c.eax)!=0;
}
static void HelpRouting(){
    // The native constructor leaves the global pointing to the second HUD.
    // Original P1 publication goes to P2, and P2's no-help update erases it.
    Seed();candidate=false;orders_available[0]=true;HelpUpdate(0);
    assert(HelpCount(0)==0&&HelpCount(1)>0);HelpUpdate(1);assert(HelpCount(1)==0);
    for(unsigned first=0;first<2;++first){
        Seed();orders_available[0]=orders_available[1]=true;
        HelpUpdate(first);assert(HelpCount(first)>0&&HelpCount(1-first)==0);
        HelpUpdate(1-first);const unsigned other=HelpCount(1-first);assert(other>0);
        orders_available[first]=false;HelpUpdate(first);assert(!HelpCount(first)&&HelpCount(1-first)==other);
        assert(Read(HELP[first]+0x80)==0x25&&Read(HELP[1-first]+0x80)==0x15);
        // Explicit event publication uses the same owner outside update.
        Put(DATA+0x200,0x46b660);Put(DATA+0x204,0);Float(DATA+0x208,3);Put(DATA+0x20c,900);
        Invoke(0x140860,HELP[first],{DATA+0x200,DATA+0x20c});
        assert(HelpCount(first)==1&&HelpCount(1-first)==other&&Read(HELP[first]+0x88)==900);
        // A blocked/invalidated owner cannot clear a live partner's queue.
        B()[P[first]+0x202]=1;HelpUpdate(first);assert(!HelpCount(first)&&HelpCount(1-first)==other);
    }
    // Full native look computation -> real medic eligibility -> helper table
    // selection -> clear/enqueue. Looking away clears only the actor's hint.
    Seed();Register(0);medic_present=true;health[0]=health[1]=100;
    for(unsigned slot=0;slot<2;++slot){ComputeLook(slot);HelpUpdate(slot);assert(HelpCount(slot)>0&&Read(HELP[slot]+0x80)==0x19);}
    look_matrices[0][8]=1;look_matrices[0][10]=0;ComputeLook(0);const auto partner=HelpCount(1);HelpUpdate(0);
    assert(!HelpCount(0)&&HelpCount(1)==partner&&!Eligible(0)&&Eligible(1)&&!chosen);
    // Native 'heal player' threshold/target/busy rules use viewer health.
    for(unsigned viewer=0;viewer<2;++viewer){
        Seed();Register(1-viewer);Put(M[viewer]+0xe8,4);Put(M[1-viewer]+0xe8,4);
        health[viewer]=25;health[1-viewer]=100;assert(HealPlayer(viewer));
        assert(!HealPlayer(1-viewer));health[viewer]=100;assert(!HealPlayer(viewer));
        health[viewer]=25;Put(C+0xb4,4);assert(!HealPlayer(viewer));Put(C+0xb4,0);
        Put(M[viewer]+0xe8,0);assert(!HealPlayer(viewer));Put(M[viewer]+0xe8,4);
        assert(Read(C+0x98)==201-viewer); // Look/health queries never take over an issued command.
    }
    Seed();Register(1);Put(M[1]+0xe8,4);health[0]=100;health[1]=25;
    assert(!HealPlayer(1,true,false)&&HealPlayer(1)&&!HealPlayer(1,false));
    trial_coop::SetActive(false);assert(!HealPlayer(1));
    // Scope and identity failures do not publish into the last singleton.
    for(unsigned damage=0;damage<5;++damage){
        Seed();Invoke(0x1407f0,HELP[1],{0x46b660,0});const auto n=HelpCount(1);
        if(damage==0)Put(P[0]+0x7f0,999);
        if(damage==1)Put(STATE+0x3c+500*8,999);
        if(damage==2)admitted[0]=0;
        if(damage==3)Put(H[0]+4,P[1]);
        if(damage==4)Put(queues[0],0);
        Invoke(0x1407f0,HELP[0],{0x46b660,0});assert(!HelpCount(0)&&HelpCount(1)==n);
    }
    Seed();auto c=Context(HELP[0]);{
        TrialCoopHelpScope outer(c,B());assert(TrialCoopHelpHud(B(),queues[1])==queues[0]);
        try{throw_text=true;Invoke(0x1407f0,HELP[1],{0x46b660,0});assert(false);}catch(const std::runtime_error&){}
        throw_text=false;assert(TrialCoopHelpHud(B(),queues[1])==queues[0]);
        auto actor=Context(0);TrialCoopOrdersPlayer(actor,B());assert(actor.eax==100);
    }
    assert(TrialCoopHelpHud(B(),queues[1])==queues[1]);
    Seed();trial_coop::SetActive(false);Invoke(0x1407f0,HELP[0],{0x46b660,0});assert(!HelpCount(0)&&HelpCount(1)==1);
    std::puts("help prompts: native singleton regression, own publication/event/clear, native look-away/medic eligibility, stale ownership, nested unwind and SP fallback passed (text resources are boundary stubs)");
}
int main(){
    NativeLookSelection();HelpRouting();
    // Negative witness: P1 registers commands, but both HUDs dispatch P2's
    // last-constructed empty list. Merely replacing key glyphs cannot fix it.
    Seed();candidate=false;Register(0);assert(attached==1&&Find(M[0],300)&&!Find(M[1],300));Hud(0,false);Hud(1,false);assert(drawn[0]==0&&drawn[1]==0);
    for(unsigned owner=0;owner<2;++owner){
        Seed();Register(owner);assert(attached==1&&Find(M[0],300)&&Find(M[1],300));
        assert(Read(M[0]+0xb0)==1&&Read(M[1]+0xb0)==1&&made_sprites[0]==1&&made_sprites[1]==1);
        Register(1-owner);Register(owner);assert(attached==1&&made_sprites[0]==1&&made_sprites[1]==1);
        Hud(0,true);assert(drawn[0]==1&&drawn[1]==0&&updated[0]==1&&updated[1]==0);
        Hud(1,true);assert(drawn[0]==1&&drawn[1]==1&&updated[1]==1);
        for(unsigned viewer=0;viewer<2;++viewer){
            assert(Look(viewer,owner,true)==400+viewer);assert(Read(C+0x98)==200+owner);
            assert(Look(viewer,owner,false)==400+owner);
        }
        // Select and finish through the original client callbacks. Only the
        // selected command changes its owner; eligibility queries never do.
        for(unsigned initiator=0;initiator<2;++initiator){
            auto c=Context(C);Put(DATA+0x200,7);Put(DATA+0x204,200+initiator);Put(SP+4,DATA+0x200);Put(SP+8,DATA+0x204);
            Coop_00040D60(c,B());assert(c.esp==SP+12&&Read(C+0x98)==200+initiator);
            assert(Read(C+0xb8)==400+initiator);
            assert(Look(1-initiator,initiator,true)==401-initiator&&Read(C+0x98)==200+initiator);
            // The retail busy/target eligibility leaf rejects an already
            // active order for that same NPC in both managers' look contexts.
            Put(C+0xb4,3);Put(M[1-initiator]+0xec,400+initiator);
            auto update=Context(M[1-initiator]);{
                TrialCoopOrdersScope scope(update,B());auto busy=Context(C);Coop_00168170(busy,B());assert((busy.eax&255)==0&&Read(C+0x98)==200+initiator);
            }
            Put(C+0xb4,0);Put(M[1-initiator]+0xec,401-initiator);
            c=Context(C);Put(SP+4,7);Coop_00040DD0(c,B());assert(c.esp==SP+8&&complete[initiator]==1);
        }
        assert(chosen==2);
        Put(DATA+0x200,7);Put(DATA+0x204,300);Invoke(0x1583e0,M[owner],{DATA+0x200,DATA+0x204});
        assert(Read(M[0]+0xc0)==0&&Read(M[1]+0xc0)==0);
        Invoke(0x159120,M[owner],{0,DATA+0x204});assert(!Find(M[0],300)&&!Find(M[1],300));
    }
    // A secondary actor admitted after the primary NPC registration needs
    // existing commands copied, without replaying NPC attached side effects.
    Seed();candidate=false;Register(0);candidate=true;auto c=Context(M[1]);{TrialCoopOrdersScope scope(c,B());}
    assert(attached==1&&Find(M[1],300)&&made_sprites[1]==1);{TrialCoopOrdersScope scope(c,B());}assert(made_sprites[1]==1);
    // Do not leak context after nested scopes, exceptions or stale identities.
    Seed();fail_copy=true;bool caught=false;try{Register(0);}catch(const std::runtime_error&){caught=true;}assert(caught);fail_copy=false;
    Seed();candidate=false;Register(0);candidate=true;fail_copy=true;c=Context(M[1]);
    std::array<uint8_t,4096> stack_copy;std::memcpy(stack_copy.data(),B()+SP-4096,4096);
    caught=false;try{TrialCoopOrdersScope scope(c,B());}catch(const std::runtime_error&){caught=true;}assert(caught);
    assert(std::memcmp(stack_copy.data(),B()+SP-4096,4096)==0);fail_copy=false;
    auto property=Context(M[0]);TrialCoopOrdersProperty(property,B());assert(property.ecx==M[0]);
    {TrialCoopOrdersScope outer(c,B());auto first=Context(M[0]);{TrialCoopOrdersScope inner(first,B());property=Context(M[1]);TrialCoopOrdersProperty(property,B());assert(property.ecx==M[0]);}property=Context(M[0]);TrialCoopOrdersProperty(property,B());assert(property.ecx==M[1]);}
    Seed();Register(0);B()[M[0]+0x5c]=1;Hud(0,true);assert(drawn[0]==0&&drawn[1]==0);
    Seed();Register(0);Put(STATE+0x3c+(200&1023)*8,999);Hud(0,true);assert(drawn[0]==0&&drawn[1]==0);
    // P1's conversation control lock must not hide P2's orders menu.
    Seed();B()[P[0]+0x202]=1;c=Context(M[1]);c.esi=M[1];c.ebx=0;Original_Control(c,B());assert((c.eax&255)==0&&control_owner==100);
    c=Context(M[1]);c.esi=M[1];c.ebx=0;Coop_Control(c,B());assert((c.eax&255)==1&&control_owner==101);
    Seed();trial_coop::SetActive(false);Register(0);assert(attached==1&&!Find(M[1],300));
    std::puts("co-op orders: original registration/attachment, independent HUD dispatch and look queries, native command owner/completion, late creation, duplicate and removal paths passed");
}
