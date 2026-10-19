// Links the actual GDI/keycap/controller artwork with a synthetic prepared strip.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "xbe_pc_prompts.h"
#include "xbe_hd_ui.h"
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort(); } } while(0)
static unsigned checks;
namespace uc_hd_ui {
static Image buttons{128,4096,std::vector<uint8_t>(size_t(128)*4096*4)};
static bool available=true;
const Image* Buttons(){return available?&buttons:nullptr;}
}
static std::vector<uint8_t> Art(const TrialPromptInput& input,bool hd,unsigned id=1){
    static std::vector<uint8_t> compressed(32768,0);
    std::vector<uint8_t> out;unsigned w=0,h=0;
    CHECK(TrialPromptArtwork(id,compressed.data(),input,out,w,h,hd));
    CHECK(w==(id==1?128u:256u)&&h==(id==1?4096u:256u)&&out.size()==size_t(w)*h*4);
    return out;
}
static bool RowEqual(const std::vector<uint8_t>& a,unsigned ar,const std::vector<uint8_t>& b,unsigned br){
    return !std::memcmp(a.data()+size_t(ar)*128*128*4,b.data()+size_t(br)*128*128*4,128*128*4);
}
int main(){
    // Each pixel carries a cell and local-position witness, including partial alpha.
    for(unsigned y=0;y<4096;++y)for(unsigned x=0;x<128;++x){
        auto* p=uc_hd_ui::buttons.bgra.data()+(size_t(y)*128+x)*4;
        p[0]=uint8_t(y/128+1);p[1]=uint8_t(x);p[2]=uint8_t(y%128);p[3]=uint8_t(1+(x+y)%254);
    }
    TrialPromptInput keyboard;for(unsigned i=0;i<12;++i){keyboard.bindings[i]='A'+i;
        std::snprintf(keyboard.labels[i],64,"%c",'A'+i);}
    const auto off=Art(keyboard,false),on=Art(keyboard,true);
    static constexpr int actions[32]={-1,0,3,2,1,-1,7,4,6,5,9,8,-1,-1,-1,10,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    for(unsigned row=0;row<32;++row){
        if(actions[row]>=0)CHECK(RowEqual(off,row,on,row)); // bindings stay native
        else CHECK(RowEqual(on,row,uc_hd_ui::buttons.bgra,row));
    }
    auto orders=keyboard;orders.bindings[12]=4;std::strcpy(orders.labels[12],"Middle mouse");
    const auto orders_off=Art(orders,false),orders_on=Art(orders,true);
    CHECK(RowEqual(orders_off,20,orders_on,20)&&!RowEqual(orders_on,20,on,20));
    for(unsigned row=0;row<32;++row)if(row!=20)CHECK(RowEqual(orders_on,row,on,row));
    TrialPromptInput controller=keyboard;controller.controller=true;
    static constexpr unsigned rows[12]={1,4,3,2,7,9,8,6,11,10,15,16};
    for(unsigned i=0;i<12;++i)controller.bindings[i]=12-i;
    const auto xbox=Art(controller,true);
    for(unsigned row=0;row<32;++row)if(actions[row]>=0)
        CHECK(RowEqual(xbox,row,uc_hd_ui::buttons.bgra,rows[controller.bindings[actions[row]]-1]));
    // PS/Nintendo symbols remain generated and keep their bound action ordering.
    for(unsigned family:{1u,2u,3u}){controller.family=family;
        auto a=Art(controller,false),b=Art(controller,true);
        for(unsigned row=0;row<32;++row)if(actions[row]>=0)CHECK(RowEqual(a,row,b,row));}
    controller.family=0;uc_hd_ui::available=false;CHECK(Art(controller,true)==Art(controller,false));
    uc_hd_ui::available=true;uc_hd_ui::buttons.width=127;
    CHECK(Art(controller,true)==Art(controller,false));uc_hd_ui::buttons.width=128;
    auto truncated=uc_hd_ui::buttons.bgra.back();uc_hd_ui::buttons.bgra.pop_back();
    CHECK(Art(controller,true)==Art(controller,false));uc_hd_ui::buttons.bgra.push_back(truncated);
    keyboard.held=true;keyboard.direction=8;keyboard.x=34;keyboard.y=-22;
    CHECK(Art(keyboard,true,2)==Art(keyboard,false,2)); // order gesture unchanged
    CHECK(Art(keyboard,false)==off); // repeated toggle restores original artwork
    std::printf("PASS %u HD prompt pixels/bindings/family/gesture/toggle checks\n",checks);
}
