#include "objective_messages.h"
#include "objective_icon.h"
#include "character_dialogue.h"
extern "C" {
#include "../font.h"
#include "../ui_motion.h"
#include "../texture_asset/texture_asset.h"
}
#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <stdexcept>
namespace {
void need(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
ComPtr<ID3D11ShaderResourceView> texture(const UCArchive*a,const char*name,const char*hash,NativeRenderer&r){
    BytesOwner b;checked_resource(a,name,hash,b);
    struct Image{UCTextureAsset value{};~Image(){uc_texture_free(&value);}} image;
    need(!uc_texture_decode(b.value.data,b.value.size,&image.value),"Objective texture");return r.upload(image.value);
}
void quad(const UCFontQuad&q,uint32_t color,NativeVertex*out){
    const float xy[4][2]={{q.x0,q.y0},{q.x1,q.y0},{q.x1,q.y1},{q.x0,q.y1}};
    const float uv[4][2]={{q.u0,q.v0},{q.u1,q.v0},{q.u1,q.v1},{q.u0,q.v1}};
    for(unsigned j=0;j<4;j++){auto&v=out[j];v={};v.position[0]=xy[j][0]+.5f;v.position[1]=xy[j][1]+.5f;std::memcpy(v.uv,uv[j],8);v.color=color;}
}
NativeGpuMesh mesh(NativeRenderer&r,size_t quads){
    std::vector<NativeVertex> vertices(quads*4);std::vector<uint32_t> indices(quads*6);
    const uint32_t order[]={0,1,2,0,2,3};for(size_t i=0;i<quads;i++)for(unsigned j=0;j<6;j++)indices[i*6+j]=uint32_t(i)*4+order[j];
    return r.upload(vertices.data(),vertices.size(),indices.data(),indices.size(),true);
}
}
struct NativeObjectiveMessages::Impl {
    struct Font{BytesOwner metrics;UCFont value{};ComPtr<ID3D11ShaderResourceView> texture;} fonts[2];
    struct Row{int32_t type;std::string title,text;int32_t icon=-1;float age=0;bool active=false;};
    struct Presentation {
        std::string title,text;NativeGpuMesh letters[2];UCUIMotion motion[4]{};
        bool live[4]{true,true,true,true};float height=44;NativeObjectiveIcons icons;
    };
    std::deque<Row> rows;NativeRenderer*renderer=nullptr;NativeDialogueText*text=nullptr;
    std::deque<Presentation> views;
    ComPtr<ID3D11ShaderResourceView> masks[2];NativeGpuMesh panels[2];
    float delta=0,gap=0;
    std::unique_ptr<NativePlayerAudio::MissionSource> panel_audio,swoosh;
    NativeObjectiveIcons icons;
    const std::string&resolve(const char*key){const auto*line=text->find(key);
        need(line&&!line->variants.empty(),"Objective text missing");return line->variants[0].text;}
    void activate(Row&row){
        views.emplace_back();auto&v=views.back();v.title=row.title;v.text=row.text;v.icons=icons;auto*motion=v.motion;
        UCUINotificationPositions pos{};uc_ui_notification_positions(&pos);
        need(!uc_ui_motion_begin(motion,pos.panel,.25f,2,1)&&!uc_ui_motion_begin(motion+1,pos.edge,.25f,2,1)&&
            !uc_ui_motion_begin(motion+2,pos.title,.3f,2,1)&&!uc_ui_motion_begin(motion+3,pos.body,.45f,2,1),"Objective motion initialization");
        std::vector<UCFontQuad> quads(row.text.size()+1);size_t count=0;float extra=0;
        need(!uc_font_layout_flags(&fonts[1].value,row.text.c_str(),pos.body[0],pos.body[1],1,pos.body[0],pos.body[0]+280,16,0,
            quads.data(),quads.size(),&count,&extra),"Objective body height");
        //18130 returns glyph height20, while body baselines advance19.
        v.height=44+float(std::lround(extra/fonts[1].value.line_height))*fonts[1].value.height;
        v.letters[0]=mesh(*renderer,std::max(size_t{1},row.title.size()*4));v.letters[1]=mesh(*renderer,std::max(size_t{1},row.text.size()*4));
        v.icons.begin(row.type,0xffffffffu,row.icon);
        panel_audio->play(row.type<3?"Added":row.type==3?"Complete":"Failed");row.active=true;row.age=0;
    }
};
NativeObjectiveMessages::NativeObjectiveMessages():p_(new Impl){}
NativeObjectiveMessages::~NativeObjectiveMessages()=default;
void NativeObjectiveMessages::load(const std::filesystem::path&game,NativeRenderer&r,NativeDialogueText&text,NativePlayerAudio&audio){
    auto&s=*p_;need(!s.renderer,"Objective display already loaded");uint8_t hash[32];
    auto fonts=read_archive(game/"dialoguefonts.lst",hash),art=read_archive(game/"PersistantFilesXBox.lst",hash),level=read_archive(game/"1-01.ent",hash);
    static const char*names[]={"large","small"};
    static const char*metrics[]={"6e32bbc082dcec904a1fd6a58038a6076022aeaeb4d8bf09add18616b0288892","afb8cc9e0083ee773ee61057c2b5e27fd89c8adf3870d63b900d1dcb14cd01ad"};
    static const char*shaders[]={"5eb4cb2c8d9d5b59bc9223846b3db4c7d2fe27311f6dc4d57c666830f58eac7c","0c00c756df416d5d7312fe168a3132d7f1152423c58a633bb399e6a70579e208"};
    static const char*images[]={"92e62ec8844cc57571939189d6919abc17de25973b40f8ab4b3252f13359b0df","19496ff6ff46c36474827d94087e70ef8a496020f439b8e301c1ccfcc25ae8a0"};
    for(unsigned i=0;i<2;i++){std::string prefix=std::string("fonts\\")+names[i];BytesOwner shader;
        checked_resource(fonts.get(),(prefix+".abc").c_str(),metrics[i],s.fonts[i].metrics);
        need(!uc_font_read(s.fonts[i].metrics.value.data,s.fonts[i].metrics.value.size,&s.fonts[i].value),"Objective font metrics");
        checked_resource(art.get(),(prefix+".shader").c_str(),shaders[i],shader);
        s.fonts[i].texture=texture(art.get(),(prefix+"%4b%.tga").c_str(),images[i],r);
    }
    static const char*mask_names[]={"mask_solid","mask_fade"};
    static const char*mask_shaders[]={"51a8f6fc193f43fd86768be4fb37d14f41c857378e44a6d0164a0be2c435fbc2","05367d94a906438b50c7ddf8d30edb8328f8ea8c322e7849665bc711f43675a3"};
    static const char*mask_textures[]={"9cca27cd3a82cfb1e5b196924b8389d883386a04aba6318d44e34c072f83e579","bb62869c65ce8f5832b44da10877116f50cff527a57b7b450434b259fe3475a7"};
    for(unsigned i=0;i<2;i++){BytesOwner shader;std::string prefix=std::string("frontend\\frt_content\\")+mask_names[i];
        checked_resource(art.get(),(prefix+".shader").c_str(),mask_shaders[i],shader);
        s.masks[i]=texture(art.get(),(prefix+(i?"_left.tga":".tga")).c_str(),mask_textures[i],r);s.panels[i]=mesh(r,1);
    }
    s.icons.load(level.get(),r);
    s.panel_audio=std::make_unique<NativePlayerAudio::MissionSource>(audio,level.get(),find_resource(level.get(),"banks\\panel.amp"),game/"5.saf",true);
    s.swoosh=std::make_unique<NativePlayerAudio::MissionSource>(audio,level.get(),find_resource(level.get(),"banks\\levelglobals.amp"),game/"5.saf",true,3);
    s.renderer=&r;s.text=&text;
}
int NativeObjectiveMessages::add(int32_t type,const char*title,const char*text,const char*value,int32_t icon){
    auto&s=*p_;if(!s.renderer||type<0||type>4||!title||!text||(icon!=-1&&icon!=1))return 1;
    std::string body=s.resolve(text);
    //273590 text parameter0 is authored byte81, not a printf format.
    if(value){const size_t at=body.find('\x81');if(at==std::string::npos)return 1;body.replace(at,1,value);}
    s.rows.push_back({type,s.resolve(title),std::move(body),icon});return 0;
}
void NativeObjectiveMessages::step(float delta){
    auto&s=*p_;need(s.renderer&&std::isfinite(delta)&&delta>=0&&delta<=1,"Objective display delta");s.delta=delta;s.gap-=delta;
    s.panel_audio->step(delta);s.swoosh->step(delta);
    if(s.rows.empty()||s.gap>0)return;auto&row=s.rows.front();row.age+=delta;
    if(!row.active)s.activate(row);
    else if(row.age>2.5f){if(s.rows.size()==1)s.swoosh->play("Swoosh");
        //274300 starts every retained widget's outro; request deletion does
        //not destroy its display. The next request may overlap this exit.
        for(auto&v:s.views){for(auto&m:v.motion)m.finishing=1;v.icons.finish();}s.rows.pop_front();}
}
void NativeObjectiveMessages::draw(NativeRenderer&r){
    auto&s=*p_;if(s.views.empty())return;
    const float scale=std::min(r.width/640.f,r.height/480.f),ox=(r.width-640*scale)*.5f,oy=(r.height-480*scale)*.5f;
    const float matrix[16]={2.f*scale/r.width,0,0,0,0,-2.f*scale/r.height,0,0,0,0,1,0,-1+2.f*ox/r.width,1-2.f*oy/r.height,0,1};
    const float white[4]={1,1,1,1};NativeDrawState state;state.depth_disabled=true;state.blend_mode=1;state.write_alpha=false;state.vertex_exposure=true;
    for(auto&v:s.views){
    for(unsigned i=0;i<4;i++)if(v.live[i]){
        auto&m=v.motion[i];if(m.finishing&&m.phase<=0)v.live[i]=false;
        else need(!uc_ui_motion_step(&m,s.delta),"Objective display motion");}
    for(unsigned i=0;i<2;i++){if(!v.live[i])continue;const auto&m=v.motion[i];const float w=i?100.f:200.f;
        //2739A0 ->65550/65760 floors the rectangle, then subtracts half a pixel.
        UCFontQuad q{std::floor(m.x)-.5f,std::floor(m.y)-.5f,std::floor(m.x+w)-.5f,std::floor(m.y+v.height)-.5f,0,i?1.f:0.f,1,i?0.f:1.f};
        NativeVertex vertices[4];quad(q,(uint32_t(192*m.alpha)<<24)|0x808080,vertices);r.update(s.panels[i],vertices,4);r.draw(s.panels[i],matrix,white,s.masks[i].Get(),0,0,state);
    }
    for(unsigned i=0;i<2;i++){if(!v.live[i+2])continue;const auto&m=v.motion[i+2];const std::string&text=i?v.text:v.title;
        std::vector<UCFontQuad> quads(text.size()+1);size_t count=0;float height=0;
        need(!uc_font_layout_flags(&s.fonts[i].value,text.c_str(),m.x,m.y,i?1.f:.7f,m.x,m.x+280,i?16u:4u,0,quads.data(),quads.size(),&count,&height),"Objective text geometry");
        std::vector<NativeVertex> vertices;vertices.reserve(count*16);
        const uint32_t color=(uint32_t(255*m.alpha)<<24)|(i?0xfedaab:0xffffff);
        for(size_t j=0;j<count;j++){UCFontDraw draws[4];size_t n=0;need(!uc_font_blur_quad(&quads[j],m.blur,color,draws,&n),"Objective text blur");
            for(size_t k=0;k<n;k++){const auto at=vertices.size();vertices.resize(at+4);quad(draws[k].quad,draws[k].argb,vertices.data()+at);}}
        if(!vertices.empty()){const auto indices=uint32_t(vertices.size()/4*6);vertices.resize(v.letters[i].vertex_count);
            r.update(v.letters[i],vertices.data(),vertices.size());r.draw(v.letters[i],matrix,white,s.fonts[i].texture.Get(),0,indices,state);}
    }
    v.icons.draw(r,s.delta);
    }
    s.views.erase(std::remove_if(s.views.begin(),s.views.end(),[](const Impl::Presentation&v){return
        std::none_of(std::begin(v.live),std::end(v.live),[](bool live){return live;})&&v.icons.empty();}),s.views.end());
}
size_t NativeObjectiveMessages::size()const{return p_->rows.size();}
