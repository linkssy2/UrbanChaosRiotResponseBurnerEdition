#undef NDEBUG
#include "xbe_world_textures.h"
#include <cassert>
#include <cstdio>
int main(){
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
 assert(ReadEntries(directory).size()==1);std::filesystem::remove(directory/(hash+".uctex"));assert(ReadEntries(directory).empty());
 std::filesystem::remove(directory/"manifest.txt");std::filesystem::remove(directory);
 std::puts("world textures: bounded manifest, exact identities, complete BC3 mips, malformed input fallback passed");
}
