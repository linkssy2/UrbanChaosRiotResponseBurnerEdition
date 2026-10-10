#include "heartbeat_widget.h"
#include "character_dialogue.h"
extern "C" {
#include "../font.h"
#include "../texture_asset/texture_asset.h"
}
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
uint32_t color32(const float c[4]){
    uint32_t v[4];for(unsigned i=0;i<4;i++)v[i]=uint32_t(c[i]*255.f);
    return (v[3]<<24)|(v[0]<<16)|(v[1]<<8)|v[2];
}
void valid_color(const float*c){
    if(!c)throw std::invalid_argument("Heartbeat color missing");
    for(unsigned i=0;i<4;i++)if(!std::isfinite(c[i])||c[i]<0||c[i]>1)throw std::invalid_argument("Heartbeat color invalid");
}
}
float native_heartbeat_label_width(const UCFont&font,const char*text){
    if(!text||!font.mapping||!font.glyphs||!font.count)throw std::invalid_argument("Heartbeat label font missing");
    auto u16=[](const uint8_t*p){return uint16_t(p[0]|uint16_t(p[1])<<8);};
    float width=0;size_t count=0;
    for(auto*p=(const unsigned char*)text;*p;p++){
        if(*p<32||*p>126||++count>4096)throw std::invalid_argument("Heartbeat label character");
        unsigned index=*p>font.maximum?0:u16(font.mapping+2**p);if(index>=font.count)throw std::invalid_argument("Heartbeat label glyph");
        const auto*g=font.glyphs+24*index;float advance=float(int16_t(u16(g+20)))+float(int16_t(u16(g+16)));width=advance+width;
    }return width;
}
NativeHeartbeatWidgetPlan native_heartbeat_widget_plan(const UCHudHeartbeat&h,int32_t actor,float x,float y,const float color[4],float order_label_width){
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(h.elapsed)||h.elapsed<0)throw std::invalid_argument("Heartbeat placement invalid");
    valid_color(color);valid_color(h.damage_color);valid_color(h.alert_color);
    const float parent_x=x,parent_y=y;
    if(!std::isfinite(order_label_width))throw std::invalid_argument("Heartbeat order label extent");
    NativeHeartbeatWidgetPlan p;if(actor==-1&&h.elapsed>=.44999998807907104f)return p;
    float t=std::clamp(h.elapsed/.44999998807907104f,0.f,1.f);if(actor!=-1)t=1-t;t=1-t;
    const float ease=t>=1?0:t<=0?1:float(1.0-std::sin(double(t)*1.5707963705062866));
    x+=0.f-640.f*ease;
    auto sprite=[&](float sx,float sy,float w,float ht,float u,float v,float ue,float ve,unsigned texture,const float*c){
        if(c[3]<=0)return;p.shapes.quads[p.shapes.count++]={std::floor(sx)-.5f,std::floor(sy)-.5f,w,ht,u,v,ue,ve,texture,color32(c)};
    };
    // 79770 visits the second child before the first: background, two damage
    // sprites, alert label, then71D70 draws the53-column waveform.
    sprite(x,y,57,21,0,0,57.f/64,21.f/32,0,color);
    float damage[4],alert[4];for(unsigned i=0;i<4;i++){damage[i]=color[i]*h.damage_color[i];alert[i]=color[i]*h.alert_color[i];}
    const float dx=x-8,dy=y-4;
    sprite(dx,dy,10,32,22.f/32,0,1,1,1,damage);
    sprite(dx+10,dy,60,32,0,0,60.f/64,1,2,damage);
    p.label_after=p.shapes.count;p.label_x=x+59;p.label_y=(y+3)-2;
    p.label_color=color32(alert);p.label_visible=alert[3]>=.009999999776482582f;
    for(unsigned i=0;i<53;i++){if(!std::isfinite(h.samples[i]))throw std::invalid_argument("Heartbeat sample invalid");valid_color(h.colors[i]);}
    y+=14;
    for(unsigned i=1;i<53;i++){
        float rgba[4];for(unsigned j=0;j<4;j++)rgba[j]=color[j]*h.colors[i][j];
        if(rgba[3]<=0)continue;
        float a=y+h.samples[i],b=y+h.samples[i-1];if(a>b)std::swap(a,b);
        const float left=std::floor(float(double(x)+i)),right=std::floor(float(double(x)+i)+2),top=std::floor(a-1),bottom=std::floor(b+2);
        p.shapes.quads[p.shapes.count++]={left-.5f,top-.5f,right-left,bottom-top,0,0,1,1,3,color32(rgba)};
    }
    if(order_label_width>=0){
        const float ix=((order_label_width+parent_x)+59.f)-3.f,iy=parent_y-2.f;
        const float left=std::floor(ix),top=std::floor(iy),right=std::floor(ix+24.f),bottom=std::floor(iy+24.f);
        const float tint[4]={1,1,1,color[3]};
        p.shapes.quads[p.shapes.count++]={left-.5f,top-.5f,right-left,bottom-top,0,0,1,1,5,color32(tint)};
    }
    return p;
}
struct NativeHeartbeatWidget::Impl {
    UCHudHeartbeat history{};int32_t actor=-1;NativeDialogueText*text=nullptr;std::string label;
    BytesOwner metrics;UCFont font{};ComPtr<ID3D11ShaderResourceView> textures[6];NativeGpuMesh mesh;float label_width=0;
    Impl(){uc_hud_heartbeat_initialize(&history);}
};
NativeHeartbeatWidget::NativeHeartbeatWidget():p_(std::make_unique<Impl>()){}
NativeHeartbeatWidget::~NativeHeartbeatWidget()=default;
void NativeHeartbeatWidget::load(const std::filesystem::path&game,NativeRenderer&r,NativeDialogueText&text,const UCArchive*level){
    auto p=std::make_unique<Impl>();p->text=&text;
    static const char*names[]={"hud\\hud_heartbeatbg","hud\\hud_teamhealth_a_dam","hud\\hud_teamhealth_b_dam","font\\font_untextured_alpha"};
    static const char*sh[]={"7f79787c9e0dcb23a96b0442e735ef20120cf459b8e5e37ffae6596e707e1c15","e7118fe12bcdbba7d844692419b36540a013e994d72accaa6f2d024c71cf3f4e","a152011c61c7bf0dd0ff03337ab449d1f1578f28d7aa92f33009ed2b4f860efc","d57a561a9c9165f64278fe7123f02d2983067888a00b402fafb382dd6cafc214"};
    static const char*th[]={"151a723d2ba718dbbd5cccf657bfb2c2b81831ff717db07f6db8c72902b8b9b5","98ab51129705435d2f34b4f0a92a7efd4d5d9fe2f4ea690633caa27fedf6a3f2","bb48e89abeec12e376e40378a68c1b62e0167c99098d59a0889e6820ec82e002","8edc66b1627428cd92209b97d42e9b3d0e392d7a5c09bc48dcea49751f1e326f"};
    auto texture=[&](const UCArchive*a,const char*name,const char*hash){
        BytesOwner bytes;checked_resource(a,name,hash,bytes);
        struct T{UCTextureAsset v{};~T(){uc_texture_free(&v);}} t;
        if(uc_texture_decode(bytes.value.data,bytes.value.size,&t.v))throw std::runtime_error("Heartbeat texture invalid");return r.upload(t.v);
    };
    for(unsigned i=0;i<4;i++){
        BytesOwner shader;checked_resource(level,(std::string(names[i])+".shader").c_str(),sh[i],shader);
        const auto name=i==3?std::string("font\\font_untextured.tga"):std::string(names[i])+".tga=nomipmap";
        p->textures[i]=texture(level,name.c_str(),th[i]);
    }
    uint8_t hash[32];auto fonts=read_archive(game/"dialoguefonts.lst",hash);
    checked_resource(fonts.get(),"fonts\\small.abc","afb8cc9e0083ee773ee61057c2b5e27fd89c8adf3870d63b900d1dcb14cd01ad",p->metrics);
    if(uc_font_read(p->metrics.value.data,p->metrics.value.size,&p->font))throw std::runtime_error("Heartbeat font invalid");
    auto art=read_archive(game/"PersistantFilesXBox.lst",hash);BytesOwner shader;
    checked_resource(art.get(),"fonts\\small.shader","0c00c756df416d5d7312fe168a3132d7f1152423c58a633bb399e6a70579e208",shader);
    p->textures[4]=texture(art.get(),"fonts\\small%4b%.tga","19496ff6ff46c36474827d94087e70ef8a496020f439b8e301c1ccfcc25ae8a0");
    NativeVertex vertices[4]{};const uint32_t indices[]={0,1,2,0,2,3};p->mesh=r.upload(vertices,4,indices,6,true);p_=std::move(p);
}
int NativeHeartbeatWidget::bind(int32_t actor,const char*resource,float health){
    if(actor<0||!resource||!p_->text||!std::isfinite(health)||health<0)return 1;
    const auto*line=p_->text->find(resource);std::string label=line&&!line->variants.empty()?line->variants[0].text:resource;
    std::vector<UCFontQuad> quads(label.size()+1);size_t n;float height;
    if(uc_font_layout_flags(&p_->font,label.c_str(),0,0,1,0,640,4,0,quads.data(),quads.size(),&n,&height))return 1;
    const float width=native_heartbeat_label_width(p_->font,label.c_str());
    p_->label_width=width;p_->label=std::move(label);p_->actor=actor;p_->history.previous_health=health;p_->history.alarms=3;p_->history.elapsed=0;return 0;
}
void NativeHeartbeatWidget::clear(){p_->actor=-1;p_->history.elapsed=0;}
bool NativeHeartbeatWidget::active()const{return p_->actor!=-1||p_->history.elapsed<.44999998807907104f;}
int32_t NativeHeartbeatWidget::actor()const{return p_->actor;}
const UCHudHeartbeat&NativeHeartbeatWidget::history()const{return p_->history;}
void NativeHeartbeatWidget::step(const UCHudHeartbeatActor&actor,float delta,UCGameRandom&rng,void(*sound)(void*),void*owner){
    auto a=actor;a.external_color=0;if(p_->actor==-1)a.present=0;
    if(!uc_hud_heartbeat_step(&p_->history,&a,delta,&rng,sound,owner))throw std::runtime_error("NPC heartbeat update invalid");
}
void NativeHeartbeatWidget::action_feedback(ID3D11ShaderResourceView*image){p_->textures[5]=image;}
bool NativeHeartbeatWidget::has_feedback()const{return p_->textures[5]!=nullptr;}
void NativeHeartbeatWidget::draw(NativeRenderer&r,float x,float y,const float color[4],bool order_available){
    const auto plan=native_heartbeat_widget_plan(p_->history,p_->actor,x,y,color,order_available&&p_->textures[5]?p_->label_width:-1);if(!plan.shapes.count&&!plan.label_visible)return;
    if(!p_->mesh.vertices)throw std::runtime_error("NPC heartbeat resources missing");
    const float scale=std::min(r.width/640.f,r.height/480.f),oy=(r.height-480*scale)*.5f;
    const float matrix[16]={2*scale/r.width,0,0,0,0,-2*scale/r.height,0,0,0,0,1,0,-1,1-2*oy/r.height,0,1},white[4]={1,1,1,1};
    auto quad=[&](const NativeHudQuad&q){
        const float xy[4][2]={{q.x+.5f,q.y+.5f},{q.x+q.width+.5f,q.y+.5f},{q.x+q.width+.5f,q.y+q.height+.5f},{q.x+.5f,q.y+q.height+.5f}},uv[4][2]={{q.u0,q.v0},{q.u1,q.v0},{q.u1,q.v1},{q.u0,q.v1}};
        NativeVertex v[4]{};for(unsigned j=0;j<4;j++){std::memcpy(v[j].position,xy[j],8);std::memcpy(v[j].uv,uv[j],8);v[j].color=q.color;}
        NativeDrawState s;s.depth_disabled=true;s.write_alpha=false;s.blend_mode=1;s.vertex_exposure=true;
        s.point_filter=s.clamp_u=s.clamp_v=q.texture<3||q.texture==5;
        r.update(p_->mesh,v,4);r.draw(p_->mesh,matrix,white,p_->textures[q.texture].Get(),0,6,s);
    };
    auto label=[&](){if(!plan.label_visible||p_->label.empty())return;
        std::vector<UCFontQuad> quads(p_->label.size()+1);size_t n;float height;
        if(uc_font_layout_flags(&p_->font,p_->label.c_str(),plan.label_x,plan.label_y,1,0,640,4,0,quads.data(),quads.size(),&n,&height))throw std::runtime_error("NPC heartbeat label layout");
        for(size_t i=0;i<n;i++){const auto&q=quads[i];quad({q.x0,q.y0,q.x1-q.x0,q.y1-q.y0,q.u0,q.v0,q.u1,q.v1,4,plan.label_color});}
    };
    for(size_t i=0;i<=plan.shapes.count;i++){if(i==plan.label_after)label();if(i<plan.shapes.count)quad(plan.shapes.quads[i]);}
}
