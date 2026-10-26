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
   const auto count=positions.size()/10;
   assert(count==1614||count==2387||count==3103);
   auto read=[&](const char* suffix){
    std::ifstream stream(std::filesystem::path(argv[arg]).replace_extension(suffix),std::ios::binary);assert(stream);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(stream)),{});
   };
   auto uv=read(".uv"),skin=read(".skin");const auto original_uv=uv,original_skin=skin;
   assert(uv.size()==count*8&&skin.size()==count*8);
   // Each missing, truncated or foreign stream rejects the entire transaction.
   for(int stream=0;stream<3;++stream){
    positions=original;uv=original_uv;skin=original_skin;
    auto& bytes=stream==0?positions:stream==1?uv:skin;bytes[4]^=1;
    const auto p=positions,u=uv,k=skin;
    assert(!uc_arm_mesh::Apply(positions,uv,skin)&&positions==p&&uv==u&&skin==k);
    positions=original;uv=original_uv;skin=original_skin;
    assert(!uc_arm_mesh::Apply(std::span<uint8_t>(positions).first(positions.size()-(stream==0)),
       std::span<uint8_t>(uv).first(uv.size()-(stream==1)),std::span<uint8_t>(skin).first(skin.size()-(stream==2))));
    assert(positions==original&&uv==original_uv&&skin==original_skin);
   }
   assert(!uc_arm_mesh::Apply(positions,uv,{})&&positions==original&&uv==original_uv&&skin==original_skin);
   assert(uc_arm_mesh::Apply(positions,uv,skin));unsigned changed=0;
   for(size_t vertex=0;vertex<count;++vertex){
    const size_t i=vertex*10,offset=vertex*8;
    int16_t before,after;std::memcpy(&before,original.data()+i,2);std::memcpy(&after,positions.data()+i,2);
    const bool ring=std::abs(int(before))>=15980&&std::abs(int(before))<=16000;
    if(ring){
     const int shoulder=count==3103?8000:5000;
     assert(after==(before<0?-shoulder:shoulder));++changed;
     assert(skin[offset+6]==0&&skin[offset+7]==255);
     float v;std::memcpy(&v,uv.data()+offset+4,4);assert(v>=0&&v<=1);
    }else{
     assert(before==after);
     assert(!std::memcmp(uv.data()+offset,original_uv.data()+offset,8));
     assert(!std::memcmp(skin.data()+offset,original_skin.data()+offset,8));
    }
    assert(!std::memcmp(original.data()+i+2,positions.data()+i+2,8));
    assert(!std::memcmp(uv.data()+offset,original_uv.data()+offset,4));
    assert(!std::memcmp(skin.data()+offset,original_skin.data()+offset,6));
    assert(skin[offset+6]+skin[offset+7]<=255);
   }
   assert(changed==(count==2387?36u:30u));
   const auto p=positions,u=uv,k=skin;
   assert(!uc_arm_mesh::Apply(positions,uv,skin)&&positions==p&&uv==u&&skin==k);
   for(const auto& item:{std::pair{std::string(".patched.positions"),&positions},
       std::pair{std::string(".patched.uv"),&uv},std::pair{std::string(".patched.skin"),&skin}}){
    std::ofstream stream(std::filesystem::path(argv[arg]).replace_extension(item.first),std::ios::binary);
    stream.write(reinterpret_cast<const char*>(item.second->data()),item.second->size());assert(stream);
   }
   std::printf("arm mesh: %u shoulder-edge vertices follow upper-arm joints; exact atomic streams and unrelated vertices checked\n",changed);
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
