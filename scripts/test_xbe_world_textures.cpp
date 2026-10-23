#undef NDEBUG
#include "xbe_world_textures.h"
#include "xbe_ledge_uv.h"
#include "xbe_arm_mesh.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 using namespace uc_world_textures;
 Entry e{};std::string hash(64,'a');
 assert(Parse("64 32 12 1024 "+hash+" "+hash,e));
 for(auto line:{std::string("64 32 12 1024 ../../x x"),"0 32 12 1024 "+hash+" "+hash,"64 32 11 1024 "+hash+" "+hash,"64 32 12 1024 "+hash+" "+hash+" extra"})assert(!Parse(line,e));
 std::vector<uint8_t> b(20+16*3,0);std::memcpy(b.data(),"UCWT0001",8);
 uint32_t w=4,h=4,n=3;std::memcpy(b.data()+8,&w,4);std::memcpy(b.data()+12,&h,4);std::memcpy(b.data()+16,&n,4);
 Image im;assert(Decode(b,im)&&im.mips.size()==3&&im.width==4);
 b.pop_back();assert(!Decode(b,im)&&im.mips.empty());b.push_back(0);b.push_back(0);assert(!Decode(b,im));
 b.resize(68);b[16]=2;assert(!Decode(b,im));b[16]=3;b[8]=3;assert(!Decode(b,im));b[8]=4;b[0]=0;assert(!Decode(b,im));
 auto directory=std::filesystem::temp_directory_path()/("burner-world-test-"+std::to_string(GetCurrentProcessId()));
 std::filesystem::create_directory(directory);assert(ReadEntries(directory).empty());
 {std::ofstream f(directory/"manifest.txt");f<<"4 4 12 8 "<<hash<<" "<<hash<<"\n";}
 assert(ReadEntries(directory).empty());
 {std::ofstream f(directory/(hash+".uctex"),std::ios::binary);std::string bytes(68,'x');f.write(bytes.data(),bytes.size());}
 assert(ReadEntries(directory).size()==1);
 const std::string ledge="b019a89fc10d2e69a8af8d205a6ed1f2f7822a0fe62e60cfcff8bc6eb6fb0322";
 const std::string replacement="845ef41e2600785b1cc341a142e6a7a5eb89ed5f78167979994f1b943be04500";
 std::filesystem::copy_file(directory/(hash+".uctex"),directory/(replacement+".uctex"));
 {std::ofstream f(directory/"manifest.txt",std::ios::app);
  f<<"128 128 12 10920 "<<ledge<<" "<<replacement<<"\n";
  f<<"128 128 12 10920 "<<ledge<<" "<<hash<<"\n";}
 auto rows=ReadEntries(directory);
 assert(rows.size()==3&&rows[0].source==hash&&rows[1].target==replacement&&rows[2].target==hash);
 std::filesystem::remove(directory/(replacement+".uctex"));
 std::filesystem::remove(directory/(hash+".uctex"));assert(ReadEntries(directory).empty());
 std::filesystem::remove(directory/"manifest.txt");std::filesystem::remove(directory);
 std::vector<uint8_t> unknown(32);assert(!uc_ledge_uv::Apply(unknown,unknown));
 for(int arg=1;arg<argc;++arg){
  std::ifstream f(argv[arg],std::ios::binary);assert(f);
  if(std::filesystem::path(argv[arg]).extension()==".positions") {
   std::vector<uint8_t> original((std::istreambuf_iterator<char>(f)),{}),positions=original;
   assert(positions.size()==23870||positions.size()==16140);
   std::ifstream uf(std::filesystem::path(argv[arg]).replace_extension(".uv"),std::ios::binary);assert(uf);
   std::vector<uint8_t> original_uv((std::istreambuf_iterator<char>(uf)),{}),uv=original_uv;
   assert(uv.size()==positions.size()/10*8);
   positions[4]^=1;auto invalid=positions;
   assert(!uc_arm_mesh::Apply(positions,uv)&&positions==invalid&&uv==original_uv);positions=original;
   assert(!uc_arm_mesh::Apply(std::span<uint8_t>(positions).first(positions.size()-1),uv)&&positions==original);
   assert(!uc_arm_mesh::Apply(positions,std::span<uint8_t>(uv).first(uv.size()-1))&&positions==original&&uv==original_uv);
   if(positions.size()==23870){
    uv[0]^=1;auto invalid_uv=uv;assert(!uc_arm_mesh::Apply(positions,uv)&&positions==original&&uv==invalid_uv);uv=original_uv;
   }
   assert(uc_arm_mesh::Apply(positions,uv));unsigned changed=0;
   for(size_t i=0;i<positions.size();i+=10) {
    int16_t before,after;std::memcpy(&before,original.data()+i,2);std::memcpy(&after,positions.data()+i,2);
    const bool ring=std::abs(int(before))>=15980&&std::abs(int(before))<=16000;
    if(ring){assert(after==(before<0?-5000:5000));++changed;}
    else assert(before==after);
    assert(!std::memcmp(original.data()+i+2,positions.data()+i+2,8));
    const size_t offset=i/10*8;
    assert(!std::memcmp(uv.data()+offset,original_uv.data()+offset,4));
    if(ring&&positions.size()==23870){
     float old_v,new_v;std::memcpy(&old_v,original_uv.data()+offset+4,4);std::memcpy(&new_v,uv.data()+offset+4,4);
     assert(new_v>=0&&new_v<=1);
     // Independently locate the adjacent forearm ring and compare texels per unit of length.
     int distance=INT_MAX,neighbor=0;float neighbor_v=0;
     int16_t y,z;std::memcpy(&y,original.data()+i+2,2);std::memcpy(&z,original.data()+i+4,2);
     for(size_t j=0;j<original.size();j+=10){
      int16_t q[3];std::memcpy(q,original.data()+j,6);float v;std::memcpy(&v,original_uv.data()+j/10*8+4,4);
      if((q[0]<0)!=(before<0)||std::abs(int(q[0]))<17500||std::abs(int(q[0]))>17700||v<.99f||v>1.01f)continue;
      const int d=(q[1]-y)*(q[1]-y)+(q[2]-z)*(q[2]-z);
      if(d<distance){distance=d;neighbor=std::abs(int(q[0]));neighbor_v=v;}
     }
     assert(neighbor);
     const float old_density=(neighbor_v-old_v)/(neighbor-std::abs(int(before)));
     const float new_density=(neighbor_v-new_v)/(neighbor-std::abs(int(after)));
     assert(std::abs(new_density/old_density-1)<.002f);
    }else assert(!std::memcmp(uv.data()+offset+4,original_uv.data()+offset+4,4));
   }
   assert(changed==(positions.size()==23870?36u:30u));
   auto corrected=positions,corrected_uv=uv;assert(!uc_arm_mesh::Apply(positions,uv)&&positions==corrected&&uv==corrected_uv);
   std::printf("arm mesh: %u open-ring vertices extended; sleeve texel density checked, hands and foreign streams unchanged\n",changed);
   continue;
  }
  uint32_t count=0;f.read(reinterpret_cast<char*>(&count),4);assert(count&&count<=1000000);
  std::vector<uint8_t> positions(size_t(count)*16),uv(size_t(count)*8),expected(uv.size());
  f.read(reinterpret_cast<char*>(positions.data()),positions.size());
  f.read(reinterpret_cast<char*>(uv.data()),uv.size());
  f.read(reinterpret_cast<char*>(expected.data()),expected.size());assert(f&&f.peek()==EOF);
  auto original=uv;positions[0]^=1;assert(!uc_ledge_uv::Apply(positions,uv)&&uv==original);positions[0]^=1;
  uv[0]^=1;auto changed=uv;assert(!uc_ledge_uv::Apply(positions,uv)&&uv==changed);uv=original;
  assert(!uc_ledge_uv::Apply(positions,std::span<uint8_t>(uv).first(uv.size()-1))&&uv==original);
  assert(uc_ledge_uv::Apply(positions,uv)&&uv==expected);
  assert(!uc_ledge_uv::Apply(positions,uv)&&uv==expected);
  for(size_t i=0;i<uv.size();i+=8)assert(std::memcmp(uv.data()+i,original.data()+i,4)==0);
 }
 std::puts("world textures: bounded manifest, exact identities, complete BC3 mips, malformed input fallback passed");
}
