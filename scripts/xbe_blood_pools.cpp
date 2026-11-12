#include "xbe_blood_pools.h"
#include "cpu/x86_context.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <cstring>
#include <xmmintrin.h>

namespace {
thread_local uint64_t submissions=0;
uint32_t Read(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
void Write(uint8_t* b,uint32_t a,uint32_t v){std::memcpy(b+a,&v,4);}
}
uint64_t TrialDecalSubmissions(){return submissions;}
void TrialDecalSubmitted(){++submissions;}

void TrialGrowDecalRing(const X86Context& original,uint8_t* b){
    const uint32_t ring=original.ecx;
    if(!b||!TrialGuestReadable(ring,0x90)||Read(b,ring)!=0x456ae0)return;
    const uint32_t data=Read(b,ring+0x80),first=Read(b,ring+0x84),next=Read(b,ring+0x88),capacity=Read(b,ring+0x8c);
    if(capacity<2||capacity>UINT32_MAX/152||first>=capacity||next>=capacity||
       (next+1)%capacity!=first||!TrialGuestReadable(data,capacity*76)||
       original.esp<0x11000||!TrialGuestReadable(original.esp-4096,4096))return;
    // Keep the guest allocator and destructor paired; retain ring order when wrapped.
    struct Restore {
        uint8_t* b;uint32_t low;std::array<uint8_t,4096> stack;unsigned csr=_mm_getcsr();
        Restore(uint8_t* p,uint32_t a):b(p),low(a){std::memcpy(stack.data(),b+low,stack.size());}
        ~Restore(){std::memcpy(b+low,stack.data(),stack.size());_mm_setcsr(csr);}
    } restore(b,original.esp-4096);
    X86Context call=original;call.esp-=512;
    const uint32_t grown=kernel::CallGuest(call,0x2d589f,{capacity*152});
    if(!grown)return;
    if(!TrialGuestReadable(grown,capacity*152)){
        kernel::CallGuest(call,0x2d631d,{grown});return;
    }
    const uint32_t count=capacity-1,tail=capacity-first;
    const uint32_t first_count=count<tail?count:tail;
    std::memcpy(b+grown,b+data+first*76,first_count*76);
    std::memcpy(b+grown+first_count*76,b+data,(count-first_count)*76);
    Write(b,ring+0x80,grown);Write(b,ring+0x84,0);Write(b,ring+0x88,count);Write(b,ring+0x8c,capacity*2);
    kernel::CallGuest(call,0x2d631d,{data});
}

#include "xbe_blood_clip.h"
#include <map>
namespace {
thread_local unsigned blood_depth=0;
using BloodKey=std::array<uint32_t,13>;
thread_local std::map<BloodKey,std::vector<blood_clip::Polygon>> blood_surfaces;
BloodKey BloodIdentity(const uint8_t* b,uint32_t record){
 BloodKey key{};key[0]=Read(b,record);std::memcpy(key.data()+1,b+record+16,48);return key;
}
struct BloodScratch {
 uint8_t* b;uint32_t low;std::array<uint8_t,8192> saved;unsigned csr=_mm_getcsr();X86Context call;
 BloodScratch(const X86Context& c,uint8_t* p):b(p),low(c.esp-8192),call(c){std::memcpy(saved.data(),p+low,saved.size());call.esp-=512;}
 ~BloodScratch(){std::memcpy(b+low,saved.data(),saved.size());_mm_setcsr(csr);}
};
bool BloodStack(const X86Context& c){return c.esp>=0x12000&&TrialGuestReadable(c.esp-8192,8192);}
uint32_t Bits(float f){uint32_t v;std::memcpy(&v,&f,4);return v;}
}
TrialBloodPoolScope::TrialBloodPoolScope(){++blood_depth;}
TrialBloodPoolScope::~TrialBloodPoolScope(){--blood_depth;}
void TrialClearBloodSupport(){blood_surfaces.clear();}
void TrialBloodSupport(const X86Context& c,uint8_t* b){
 if(!blood_depth||!b||c.esi<4||!TrialGuestReadable(c.esi-4,76)||!BloodStack(c))return;
 const uint32_t record=c.esi-4;auto key=BloodIdentity(b,record);
 float corners[4][3];std::memcpy(corners,b+record+16,48);
 for(auto& corner:corners)for(float v:corner)if(!std::isfinite(v))return;
 BloodScratch scratch(c,b);const uint32_t start=c.esp-128,end=c.esp-112;
 auto probe=[&](blood_clip::Point p){
  float a[3],z[3];for(int k=0;k<3;++k)a[k]=z[k]=corners[0][k]+p.u*(corners[1][k]-corners[0][k])+p.v*(corners[3][k]-corners[0][k]);
  // 223450 lifts the quad by its unit normal; match 223BC0's two-unit support rays.
  a[1]+=2.f;z[1]-=2.f;std::memcpy(b+start,a,12);std::memcpy(b+end,z,12);
  scratch.call=c;scratch.call.esp=c.esp-512;
  return kernel::CallGuest(scratch.call,0x214230,{start,end,1,0xffff,0x4000,0})!=0;
 };
 blood_surfaces[key]=blood_clip::support(probe);
}
bool TrialBloodVertex(const X86Context& c,uint8_t* b){
 if(!b||c.esi>=4||!TrialGuestReadable(c.ebx,76)||!TrialGuestReadable(c.esp+0x20,12))return false;
 auto found=blood_surfaces.find(BloodIdentity(b,c.ebx));if(found==blood_surfaces.end())return false;
 const auto& mesh=found->second;
 if(mesh.size()==1&&mesh[0].size()==4&&mesh[0][0].u==0&&mesh[0][0].v==0&&mesh[0][2].u==1&&mesh[0][2].v==1)return false;
 struct Vertex {float x,y,z;};static thread_local std::array<Vertex,4> vertices;
 std::memcpy(&vertices[c.esi].x,b+c.esp+0x20,4);std::memcpy(&vertices[c.esi].y,&c.ebp,4);std::memcpy(&vertices[c.esi].z,b+c.esp+0x28,4);
 if(c.esi!=3)return true;
 if(!BloodStack(c))return true;
 float corners[4][3];std::memcpy(corners,b+c.ebx+16,48);
 const float ux=corners[1][0]-corners[0][0],uz=corners[1][2]-corners[0][2],vx=corners[3][0]-corners[0][0],vz=corners[3][2]-corners[0][2],det=ux*vz-uz*vx;
 if(!std::isfinite(det)||std::abs(det)<1e-8f)return true;
 blood_clip::Polygon current;current.reserve(4);
 for(auto v:vertices){float dx=v.x-corners[0][0],dz=v.z-corners[0][2];current.push_back({(dx*vz-dz*vx)/det,(ux*dz-uz*dx)/det});}
 const float du=current[1].u-current[0].u,dv=current[3].v-current[0].v;
 if(!std::isfinite(du)||!std::isfinite(dv)||du<=1e-6f||dv<=1e-6f)return true;
 BloodScratch scratch(c,b);
 auto emit=[&](blood_clip::Point p){
  const float u=(p.u-current[0].u)/du,v=(p.v-current[0].v)/dv;
  float xyz[3];for(int k=0;k<3;++k)xyz[k]=corners[0][k]+p.u*(corners[1][k]-corners[0][k])+p.v*(corners[3][k]-corners[0][k]);
  scratch.call=c;scratch.call.esp=c.esp-512;
  kernel::CallGuest(scratch.call,0x3ef5c0,{3,Bits(u),Bits(v)});
  scratch.call=c;scratch.call.esp=c.esp-512;
  kernel::CallGuest(scratch.call,0x3ef600,{0xffffffff,Bits(xyz[0]),Bits(xyz[1]),Bits(xyz[2]),0x3f800000});
 };
 // Reuse clipping storage across support cells; arithmetic and emission order stay original.
 blood_clip::Polygon poly,scratch_poly;poly.reserve(8);scratch_poly.reserve(8);
 for(const auto& surface:found->second){poly.assign(current.begin(),current.end());blood_clip::intersect(poly,surface,scratch_poly);
  for(size_t i=1;i+1<poly.size();++i){emit(poly[0]);emit(poly[i]);emit(poly[i+1]);emit(poly[i+1]);}
 }
 return true;
}
