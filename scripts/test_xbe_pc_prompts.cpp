// Runs production snapshot, decoder/artwork and the actual translated medic gate.
// OS input, NPC eligibility and final D3D12 submission remain outside this fixture.
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include "cpu/x86_ops.h"
#include "xbe_pc_prompts.h"
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

static std::recursive_mutex input_lock;
static bool keyboard_mode=true,gameplay=true,menu_mode=false,captured=true,focus=true,graphics=false,order_held=false;
static bool conversation_prompt=false;
bool TrialConversationPromptActive(){return conversation_prompt;}
static long order_x=0,order_y=0;
static unsigned order_phase=3,order_buttons=0,order_selection=0;
static unsigned bindings[25][2]{};
static unsigned controller_bindings[24][2]{};
static LONG controller_custom=0;
static char labels[25][64]{};
int xbox_KeyboardMode(){return keyboard_mode;}
static bool focused(){return focus;}
static bool TrialGraphicsOpen(){return graphics;}
static unsigned xbox_KeyboardBinding(unsigned action,unsigned slot){return bindings[action][slot];}
static void controller_init(){}
static unsigned xbox_ControllerBinding(unsigned action,unsigned slot){return controller_bindings[action][slot];}
static const char* xbox_ControllerSourceName(unsigned source){
    static const char* names[]={"Unbound","A","B","X","Y","RB","LB","LT","RT","Start","Back","LS click","RS click","D-pad Up","D-pad Down","D-pad Left","D-pad Right","LS Left","LS Right","LS Up","LS Down","RS Left","RS Right","RS Up","RS Down"};
    return source<=24?names[source]:"Invalid";
}
static const char* xbox_KeyboardSourceName(unsigned source){static char key[8];std::snprintf(key,sizeof(key),"%c",source);return key;}
static void xbox_KeyboardActionLabel(unsigned action,char* dst,size_t cap){std::snprintf(dst,cap,"%s",labels[action]);}
static unsigned OrderDirection(){return std::max(std::abs(order_x),std::abs(order_y))<24?0:std::abs(order_x)>std::abs(order_y)?(order_x<0?4:8):(order_y<0?1:2);}
/* INPUT_IMPLEMENTATION */
namespace PromptKeyboard {
struct XBOX_GAMEPAD {unsigned short wButtons=0;unsigned char bAnalogButtons[8]{};SHORT sThumbLX=0,sThumbLY=0,sThumbRX=0,sThumbRY=0;};
struct UCRRKeyboardConfig {unsigned binding[25][2]{};};
static UCRRKeyboardConfig keyboard_config;
static SRWLOCK keyboard_lock=SRWLOCK_INIT,keyboard_wheel_lock=SRWLOCK_INIT;
static unsigned keyboard_wheel_active=0;
static LONG keyboard_consumed_click=0;
static constexpr unsigned UCRR_KEYBOARD_WHEEL_UP=256;
static constexpr unsigned controller_digital[8]={16,32,64,128,1,2,4,8};
static bool down[256]{};
static void keyboard_init(){}
static bool keyboard_key_down(unsigned key){return down[key];}
static bool keyboard_is_mouse(unsigned key){return key==VK_LBUTTON||key==VK_RBUTTON||key==VK_MBUTTON||key==VK_XBUTTON1||key==VK_XBUTTON2;}
static void xbox_KeyboardNoteInput(){}
static void xbox_KeyboardConsumeMouseClick(int value){keyboard_consumed_click=value;}
/* ACTUAL_KEYBOARD_APPLY */
static bool ActionA(unsigned source,bool capture){
    keyboard_config={};std::memcpy(keyboard_config.binding,bindings,sizeof(bindings));
    std::memset(down,0,sizeof(down));keyboard_wheel_active=0;
    if(source<256)down[source]=true;else keyboard_wheel_active=source;
    XBOX_GAMEPAD pad{};keyboard_apply_bindings(&pad,capture);return pad.bAnalogButtons[0]!=0;
}
}
/* ORIGINAL_AND_CANDIDATE_GATES */
/* PROMPT_CONTEXT_FIXTURE */

static std::vector<uint8_t> Read(const wchar_t* path){
    std::ifstream in(path,std::ios::binary);CHECK(in.good());
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
static void SnapshotChecks(){
    for(unsigned a=0;a<25;++a){bindings[a][0]='A'+a%26;std::snprintf(labels[a],64,"%c",'A'+a%26);}
    bindings[8][0]=VK_RETURN;std::strcpy(labels[8],"Enter");
    bindings[24][0]=VK_MBUTTON;std::strcpy(labels[24],"Middle mouse");
    bindings[0][0]=0;bindings[0][1]='R';std::strcpy(labels[0],"R");
    TrialPromptInput out{};CHECK(TrialPromptSnapshot(1,out));
    CHECK(out.bindings[0]=='R'&&out.bindings[8]==VK_ESCAPE&&!std::strcmp(out.labels[8],"Esc"));
    menu_mode=true;CHECK(TrialPromptSnapshot(1,out)&&out.bindings[8]==VK_RETURN&&!std::strcmp(out.labels[8],"Enter"));
    CHECK(out.bindings[1]==VK_RBUTTON&&!std::strcmp(out.labels[1],"Mouse Right"));
    menu_mode=false;CHECK(TrialPromptSnapshot(1,out)&&out.bindings[1]==bindings[1][0]&&!std::strcmp(out.labels[1],labels[1]));
    conversation_prompt=true;CHECK(TrialPromptSnapshot(1,out)&&out.bindings[0]=='R');
    bindings[0][0]='R';bindings[0][1]='F';
    CHECK(TrialPromptSnapshot(1,out,1)&&out.bindings[0]=='F'&&!std::strcmp(out.labels[0],"F"));
    bindings[0][1]='G';CHECK(TrialPromptSnapshot(1,out,1)&&out.bindings[0]=='G'&&!std::strcmp(out.labels[0],"G"));
    bindings[0][1]=0;CHECK(TrialPromptSnapshot(1,out,1)&&out.bindings[0]=='R');
    bindings[0][1]='F';
    menu_mode=true;CHECK(TrialPromptSnapshot(1,out)&&out.bindings[0]=='R');menu_mode=false;
    // Actual regular A-input assembly accepts exactly the displayed binding,
    // including rebound keys. Keep R as the ordinary Reload label/binding.
    for(unsigned key:{unsigned('F'),unsigned('G'),unsigned(VK_MBUTTON),256u}){
        bindings[0][1]=key;CHECK(TrialPromptSnapshot(1,out,1)&&out.bindings[0]==key);
        CHECK(PromptKeyboard::ActionA(out.bindings[0],true));
        if(key=='F'||key=='G')CHECK(PromptKeyboard::ActionA(out.bindings[0],false));
        CHECK(TrialPromptSnapshot(1,out)&&out.bindings[0]=='R'&&PromptKeyboard::ActionA('R',true));
    }
    bindings[0][1]='F';
    conversation_prompt=false;CHECK(TrialPromptSnapshot(1,out)&&out.bindings[0]=='R');
    CHECK(TrialPromptSnapshot(2,out)&&!out.held&&out.bindings[12]==VK_MBUTTON);
    CHECK(TrialPromptSnapshot(1,out,2)&&out.bindings[12]==VK_MBUTTON&&!out.held);
    CHECK(TrialPromptSnapshot(1,out)&&!out.bindings[12]&&!out.labels[12][0]);
    bindings[24][0]=0;bindings[24][1]='O';std::strcpy(labels[24],"O");
    CHECK(TrialPromptSnapshot(1,out,2)&&out.bindings[12]=='O'&&!std::strcmp(out.labels[12],"O"));
    bindings[24][1]=0;std::strcpy(labels[24],"Unbound");
    CHECK(TrialPromptSnapshot(1,out,2)&&!out.bindings[12]&&!std::strcmp(out.labels[12],"Unbound"));
    bindings[24][0]=VK_MBUTTON;bindings[24][1]=0;std::strcpy(labels[24],"Middle mouse");
    order_held=true;order_x=-61;order_y=3;
    for(unsigned i=0;i<100;++i){CHECK(TrialPromptSnapshot(2,out)&&out.held&&out.x==-60&&out.y==2&&out.direction==4);}
    CHECK(order_phase==3&&order_buttons==0&&order_selection==0&&order_x==-61&&order_y==3);
    order_held=false;CHECK(TrialPromptSnapshot(2,out)&&!out.held&&!out.x&&!out.y&&!out.direction);
    order_held=true;captured=false;CHECK(TrialPromptSnapshot(2,out)&&!out.held);captured=true;
    focus=false;CHECK(TrialPromptSnapshot(2,out)&&!out.held);focus=true;
    graphics=true;CHECK(TrialPromptSnapshot(2,out)&&!out.held);graphics=false;
    keyboard_mode=false;CHECK(!TrialPromptSnapshot(2,out)&&!out.held);
    CHECK(!TrialPromptSnapshot(1,out,1)&&!out.bindings[0]);keyboard_mode=true;
    bindings[24][0]='O';std::strcpy(labels[24],"O");CHECK(TrialPromptSnapshot(2,out)&&out.bindings[12]=='O'&&!std::strcmp(out.labels[12],"O"));
    order_held=false;
    struct Pad {unsigned buttons=0;int lx=0,ly=0,rx=0,ry=0;unsigned char analog[8]{};} pad;
    CHECK(!uc_pc_prompts::ControllerActive(pad));
    for(unsigned i=0;i<8;++i){pad.analog[i]=30;CHECK(!uc_pc_prompts::ControllerActive(pad));pad.analog[i]=31;CHECK(uc_pc_prompts::ControllerActive(pad));pad.analog[i]=0;}
    pad.buttons=1;CHECK(uc_pc_prompts::ControllerActive(pad));pad={};
    for(auto* axis:{&pad.lx,&pad.ly,&pad.rx,&pad.ry}){
        int threshold=(axis==&pad.lx||axis==&pad.ly)?7849:8689;
        *axis=threshold;CHECK(!uc_pc_prompts::ControllerActive(pad));
        *axis=threshold+1;CHECK(uc_pc_prompts::ControllerActive(pad));
        *axis=-threshold-1;CHECK(uc_pc_prompts::ControllerActive(pad));*axis=0;
    }
    uc_pc_prompts::PointerActivity pointer;
    CHECK(!pointer.Move(123,456)&&!pointer.Move(123,456));
    CHECK(pointer.Move(124,456)&&!pointer.Move(124,456));
    pointer.Reset();CHECK(!pointer.Move(200,200)&&pointer.Move(200,201));
}
static void ControllerSnapshotChecks(){
    keyboard_mode=false;gameplay=true;menu_mode=false;
    for(unsigned action=0;action<24;++action)controller_bindings[action][0]=action+1;
    TrialPromptInput out;CHECK(!TrialPromptSnapshot(1,out));
    // A custom deadzone/secondary binding alone does not require new artwork.
    controller_custom=1;CHECK(!TrialPromptSnapshot(1,out));
    controller_bindings[0][1]=4;CHECK(!TrialPromptSnapshot(1,out));
    controller_bindings[0][0]=4;CHECK(TrialPromptSnapshot(1,out)&&out.controller&&out.bindings[0]==4&&!std::strcmp(out.labels[0],"Y"));
    auto original=out;CHECK(TrialPromptSnapshot(1,out,1)&&uc_pc_prompts::Equal(original,out));
    CHECK(TrialPromptSnapshot(1,out,2)&&uc_pc_prompts::Equal(original,out));
    controller_bindings[0][0]=0;CHECK(TrialPromptSnapshot(1,out)&&out.bindings[0]==4&&!std::strcmp(out.labels[0],"Y"));
    for(unsigned source=0;source<=24;++source){
        controller_bindings[0][1]=source;
        const bool changed=TrialPromptSnapshot(1,out);
        CHECK(changed==(source!=1));
        CHECK(out.bindings[0]==source&&!std::strcmp(out.labels[0],xbox_ControllerSourceName(source)));
    }
    original=out;out.controller=false;CHECK(!uc_pc_prompts::Equal(original,out));
    out=original;out.labels[0][0]^=1;CHECK(!uc_pc_prompts::Equal(original,out));
    menu_mode=true;CHECK(!TrialPromptSnapshot(1,out)&&!out.controller&&!out.bindings[0]);
    menu_mode=false;gameplay=false;CHECK(!TrialPromptSnapshot(1,out));gameplay=true;
    CHECK(!TrialPromptSnapshot(2,out));
    keyboard_mode=true;CHECK(TrialPromptSnapshot(1,out)&&!out.controller&&out.bindings[0]=='R');
    controller_custom=0;
}
static void FamilyChecks(){
    keyboard_mode=false;menu_mode=true;controller_custom=0;
    TrialPromptInput prior{};
    for(unsigned family=1;family<=3;++family){
        trial_controller::family=family;TrialPromptInput out;
        CHECK(TrialPromptSnapshot(1,out)&&out.controller&&out.family==family);
        CHECK(out.bindings[0]==1&&!std::strcmp(out.labels[0],trial_controller::Label(1,family)));
        CHECK(!uc_pc_prompts::Equal(prior,out));prior=out;
        CHECK(!TrialPromptSnapshot(2,out));
        uint32_t glyph[128*128]{};TrialControllerGlyph(glyph,1,family);
        CHECK(std::count_if(std::begin(glyph),std::end(glyph),[](uint32_t c){return c!=0;})>100);
    }
    CHECK(!std::strcmp(trial_controller::Label(5,trial_controller::PS5),"R1"));
    CHECK(!std::strcmp(trial_controller::Label(10,trial_controller::PS5),"Create"));
    CHECK(!std::strcmp(trial_controller::Label(3,trial_controller::Nintendo),"Y"));
    trial_controller::family=0;keyboard_mode=true;menu_mode=false;
}
static void GateChecks(){
    std::vector<uint8_t> mem(512);uint8_t* base=mem.data();
    for(bool pc:{false,true})for(unsigned available:{0u,1u,2u})for(uint32_t target:{UINT32_MAX,0u,42u}){
        keyboard_mode=pc;X86Context ref{},got{};ref.esi=got.esi=128;ref.eax=got.eax=0xaabbccdd;
        ref.ebx=got.ebx=123;ref.edi=got.edi=456;ref.esp=got.esp=400;
        X86_MEM8(128+0xd5)=uint8_t(available);X86_MEM32(128+0xd8)=target;auto before=mem;
        OriginalGate(ref,base);CandidateGate(got,base);
        CHECK((ref.eax&255)==unsigned(available&&target==UINT32_MAX));
        CHECK((got.eax&255)==unsigned(available&&(target==UINT32_MAX||pc)));
        CHECK((got.eax&0xffffff00)==0xaabbcc00&&got.esi==128&&got.ebx==123&&got.edi==456&&got.esp==400&&mem==before);
    }
    keyboard_mode=true;
}
static void ArtworkChecks(){
    auto source=Read(/* PATH_SOURCE */),reference=Read(/* PATH_PIXELS */);auto before=source;
    CHECK(source.size()==32768&&reference.size()==32*1024*4);
    std::vector<uint32_t> decoded(32*1024);uc_pc_prompts::DecodeButtons(source.data(),decoded.data());
    CHECK(!std::memcmp(decoded.data(),reference.data(),reference.size()));
    struct Desc {unsigned width=32,height=1024,depth=1,format=15;bool cube=false,linear=false;} desc;
    CHECK(uc_pc_prompts::Match(desc,source.data(),source.size())==1);
    CHECK(!uc_pc_prompts::Match(desc,source.data(),source.size()-1));
    source[20]^=1;CHECK(!uc_pc_prompts::Match(desc,source.data(),source.size()));source[20]^=1;
    desc.cube=true;CHECK(!uc_pc_prompts::Match(desc,source.data(),source.size()));desc.cube=false;
    desc.linear=true;CHECK(!uc_pc_prompts::Match(desc,source.data(),source.size()));desc.linear=false;
    auto orders=Read(/* PATH_ORDERS */);desc.width=desc.height=64;
    CHECK(orders.size()==4096&&uc_pc_prompts::Match(desc,orders.data(),orders.size())==2);
    orders[20]^=1;CHECK(!uc_pc_prompts::Match(desc,orders.data(),orders.size()));orders[20]^=1;
    CHECK(!uc_pc_prompts::Match(desc,orders.data(),orders.size()-1));
    TrialPromptInput input;CHECK(TrialPromptSnapshot(1,input));std::vector<uint8_t> pixels;unsigned w=0,h=0;
    CHECK(TrialPromptArtwork(1,source.data(),input,pixels,w,h)&&w==128&&h==4096&&pixels.size()==w*h*4);
    // Atlas rows with no button replacement must retain every original pixel.
    for(unsigned row:{0u,5u,12u,13u,14u,17u,18u,19u,20u,21u,22u,23u,24u,25u,26u,27u,28u,29u,30u,31u})
        for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x)
            CHECK(!std::memcmp(&pixels[((row*128+y)*128+x)*4],&reference[((row*32+y/4)*32+x/4)*4],4));
    auto old=pixels;input.bindings[0]='F';std::strcpy(input.labels[0],"F");
    CHECK(TrialPromptArtwork(1,source.data(),input,pixels,w,h));
    CHECK(std::memcmp(&old[128*128*4],&pixels[128*128*4],128*128*4));
    CHECK(!std::memcmp(&old[2*128*128*4],&pixels[2*128*128*4],30*128*128*4));
    CHECK(TrialPromptSnapshot(1,input,2));
    CHECK(TrialPromptArtwork(1,source.data(),input,pixels,w,h));
    CHECK(std::memcmp(&old[20*128*128*4],&pixels[20*128*128*4],128*128*4));
    // An Orders glyph uses the same bound keycap as the other buttons.
    input.bindings[0]=input.bindings[12];std::strcpy(input.labels[0],input.labels[12]);
    CHECK(TrialPromptArtwork(1,source.data(),input,pixels,w,h));
    CHECK(!std::memcmp(&pixels[128*128*4],&pixels[20*128*128*4],128*128*4));
    CHECK(TrialPromptSnapshot(1,input));CHECK(TrialPromptArtwork(1,source.data(),input,pixels,w,h)&&pixels==old);
    CHECK(TrialPromptSnapshot(2,input));input.bindings[12]=VK_MBUTTON;std::strcpy(input.labels[12],"Middle mouse");
    CHECK(TrialPromptArtwork(2,nullptr,input,pixels,w,h)&&w==256&&h==256);auto idle=pixels;
    for(auto direction:{1u,2u,4u,8u}){
        input.held=true;input.direction=direction;input.x=direction==4?-60:direction==8?60:0;input.y=direction==1?-60:direction==2?60:0;
        CHECK(TrialPromptArtwork(2,nullptr,input,pixels,w,h)&&pixels!=idle);
    }
    input.held=false;input.direction=0;input.x=input.y=0;
    CHECK(TrialPromptArtwork(2,nullptr,input,pixels,w,h)&&pixels==idle);
    unsigned opaque=0;for(size_t i=0;i<pixels.size();i+=4){if(pixels[i]||pixels[i+1]||pixels[i+2]){++opaque;CHECK(pixels[i+3]==255);}else CHECK(!pixels[i+3]);}
    CHECK(opaque>1000&&opaque<18000&&source==before);
    input.bindings[12]='O';std::strcpy(input.labels[12],"O");CHECK(TrialPromptArtwork(2,nullptr,input,pixels,w,h)&&pixels!=idle);
    // Each logical button can point at every original physical button row.
    // Verify pixel-for-pixel source copies, including alpha, after swaps.
    static constexpr unsigned rows[12]={1,4,3,2,7,9,8,6,11,10,15,16};
    input={};input.controller=true;
    for(unsigned shift=0;shift<12;++shift){
        for(unsigned a=0;a<12;++a){input.bindings[a]=(a+shift)%12+1;std::strcpy(input.labels[a],xbox_ControllerSourceName(input.bindings[a]));}
        const auto snapshot=input;CHECK(TrialPromptArtwork(1,source.data(),input,pixels,w,h)&&w==128&&h==4096);
        for(unsigned a=0;a<12;++a)for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x)
            CHECK(!std::memcmp(&pixels[((rows[a]*128+y)*128+x)*4],&reference[((rows[input.bindings[a]-1]*32+y/4)*32+x/4)*4],4));
        CHECK(uc_pc_prompts::Equal(input,snapshot));
        for(unsigned row:{0u,5u,12u,13u,14u,17u,18u,19u,20u,21u,22u,23u,24u,25u,26u,27u,28u,29u,30u,31u})
            for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x)
                CHECK(!std::memcmp(&pixels[((row*128+y)*128+x)*4],&reference[((row*32+y/4)*32+x/4)*4],4));
    }
    // Direction/unbound badges use the complete source label and never treat
    // controller IDs as mouse virtual keys. The existing rasterizer is oracle.
    std::vector<uint8_t> fallback,previous;
    for(unsigned value=0;value<=24;++value){
        if(value&&value<=12)continue;
        input.bindings[0]=value;std::strcpy(input.labels[0],xbox_ControllerSourceName(value));
        CHECK(TrialPromptArtwork(1,source.data(),input,pixels,w,h));
        TrialPromptInput badge=input;badge.controller=false;badge.bindings[0]=0;
        CHECK(TrialPromptArtwork(1,source.data(),badge,fallback,w,h));
        const size_t begin=128*128*4,bytes=128*128*4;
        CHECK(!std::memcmp(pixels.data()+begin,fallback.data()+begin,bytes));
        if(!previous.empty())CHECK(std::memcmp(pixels.data()+begin,previous.data()+begin,bytes));
        previous=pixels;
    }
    CHECK(source==before);
}
int main(){
    SnapshotChecks();ControllerSnapshotChecks();FamilyChecks();GateChecks();ArtworkChecks();PromptContextChecks();
    std::puts("PASS PC prompt production snapshot/input binding assembly, exact BC3 atlas conversion, untouched original rows, binding/hold/drag/release artwork, controller remap row copies/direction badges/default and menu bypass, original medic gate, cinematic Skip producer, reward Continue literal footer and queued R/F glyph identity through original record selector/emitter; no live GPU claim");
}
