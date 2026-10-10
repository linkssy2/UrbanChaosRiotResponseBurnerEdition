#include "integration_scene.h"
#include "environment.h"
#include "glow.h"
#include "depth_blur.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

static void check(HRESULT result){if(FAILED(result))throw std::runtime_error("Scene integration GPU operation failed");}
struct NativeIntegrationScene::State {
    struct Frame {
        NativeEnvironment environment;
        NativeGlow glow;
        ComPtr<ID3D11Texture2D> snapshot,meter;
        ComPtr<ID3D11ShaderResourceView> curve;
        ID3D11Texture2D *target=nullptr; // Identity only; renderer owns it.
        unsigned width=0,height=0,meter_width=0,meter_height=0;
        UCExposurePublication publication{};
        bool pending=false;
    };
    UCExposureProfile profile{};
    std::string shader;
    std::unique_ptr<Frame> frame;
    bool present=false;
    uint64_t frames=0;
    UCDepthBlurRequest blur_pending{};UCDepthBlurSettings blur_settings{5000,5000,0,0};
    std::unique_ptr<NativeDepthBlur> blur;
};
NativeIntegrationScene::NativeIntegrationScene(const UCLevel &level,const std::filesystem::path &shaders):state_(std::make_unique<State>()) {
    uc_depth_blur_reset(&state_->blur_pending);
    static_assert(sizeof(UCExposureProfile)==36);
    for(uint32_t i=0;i<level.entity_count;i++)if(!strcmp(level.entities[i].class_name,"ExposureSettings")) {
        const auto &bytes=level.entities[i].properties;
        if(state_->present||bytes.size!=sizeof state_->profile||!bytes.data)
            throw std::runtime_error("Scene preview requires one complete authored exposure profile");
        memcpy(&state_->profile,bytes.data,sizeof state_->profile);state_->present=true;
    }
    if(state_->present) {
        UCEnvironmentExposure validated{};
        if(uc_environment_exposure_reset(&state_->profile,&validated))
            throw std::runtime_error("Unsupported authored scene exposure profile");
        std::ifstream input(shaders/"environment_postprocess.hlsl",std::ios::binary);
        if(!input)throw std::runtime_error("Cannot read native environment shader");
        state_->shader.assign(std::istreambuf_iterator<char>(input),{});
        if(state_->shader.empty()||state_->shader.size()>65536)throw std::runtime_error("Invalid native environment shader size");
    }
}
NativeIntegrationScene::~NativeIntegrationScene()=default;
bool NativeIntegrationScene::available()const{return state_->present;}
void NativeIntegrationScene::begin(NativeRenderer &r,float delta) {
    if(!std::isfinite(delta)||delta<0||delta>1)throw std::invalid_argument("Invalid scene frame delta");
    if(!available())return;
    if(state_->frame&&state_->frame->pending)throw std::logic_error("Scene exposure frame already begun");
    if(!state_->frame||state_->frame->target!=r.color.Get()||state_->frame->width!=r.width||state_->frame->height!=r.height) {
        if(r.width<40||r.height<30||r.width>4096||r.height>4096)throw std::invalid_argument("Unsupported scene postprocess viewport");
        auto f=std::make_unique<State::Frame>();f->width=r.width;f->height=r.height;f->target=r.color.Get();
        f->meter_width=std::min(r.width,3840u)/40*40;f->meter_height=std::min(r.height,2160u)/30*30;
        f->environment.open(r.device.Get(),r.context.Get(),f->meter_width,f->meter_height,state_->profile);
        D3D11_TEXTURE2D_DESC td{};r.color->GetDesc(&td);td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        check(r.device->CreateTexture2D(&td,nullptr,&f->snapshot));
        td.Width=f->meter_width;td.Height=f->meter_height;check(r.device->CreateTexture2D(&td,nullptr,&f->meter));
        f->glow.open(r.device.Get(),r.context.Get(),f->snapshot.Get(),r.width/2,r.height/2,state_->shader);
        state_->frame=std::move(f);
    }
    auto &f=*state_->frame;UCExposureInput input{};input.delta=delta;input.glow_present=1;
    f.publication=f.environment.begin(input);f.curve=f.environment.lookup();f.pending=true;
}
void NativeIntegrationScene::finish(NativeRenderer &r,bool advance) {
    if(!available())return;
    if(!state_->frame||!state_->frame->curve)throw std::logic_error("Scene postprocess has no begun frame");
    auto &f=*state_->frame;
    if(f.target!=r.color.Get()||r.width!=f.width||r.height!=f.height||(advance&&!f.pending))
        throw std::logic_error("Scene postprocess target or frame changed");
    ID3D11ShaderResourceView *empty[2]{};r.context->PSSetShaderResources(0,2,empty);r.context->OMSetRenderTargets(0,nullptr,nullptr);
    r.context->CopyResource(f.snapshot.Get(),r.color.Get());
    const UCEnvironmentGlowOuterInput glow{r.width,r.height,r.width/2,r.height/2,r.width,r.height,
        {0,0,(float)r.width,(float)r.height},f.publication.glow_threshold,f.publication.glow_multiplier};
    f.glow.apply(glow);r.context->CopyResource(r.color.Get(),f.glow.output());r.tone(f.curve.Get());
    if(advance) {
        r.context->OMSetRenderTargets(0,nullptr,nullptr);
        D3D11_BOX box{0,0,0,f.meter_width,f.meter_height,1};r.context->CopySubresourceRegion(f.meter.Get(),0,0,0,0,r.color.Get(),0,&box);
        f.environment.end(f.meter.Get(),1,1,0,0);f.pending=false;++state_->frames;
    }
    // Restore main's geometry/HUD pipeline without clearing color or depth.
    ID3D11RenderTargetView *target=r.target.Get();r.context->OMSetRenderTargets(1,&target,r.depth_view.Get());
    D3D11_VIEWPORT viewport{0,0,(float)r.width,(float)r.height,0,1};r.context->RSSetViewports(1,&viewport);
    r.context->IASetInputLayout(r.layout.Get());r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);
    ID3D11Buffer *constants=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&constants);r.context->PSSetConstantBuffers(0,1,&constants);
}
float NativeIntegrationScene::exposure()const{return state_->frame?state_->frame->environment.tone().settings.input_scale:1.f;}
float NativeIntegrationScene::adapted_exposure()const{return state_->frame?state_->frame->environment.exposure().adapted_exposure:1.f;}
float NativeIntegrationScene::inverse_exposure()const{return 1.f/exposure();}
uint64_t NativeIntegrationScene::frames()const{return state_->frames;}
unsigned NativeIntegrationScene::meter_samples()const{return state_->frame?state_->frame->environment.sample_count():0;}
int NativeIntegrationScene::request_blur(float a,float b,float v){return uc_depth_blur_request(&state_->blur_pending,a,b,v);}
void NativeIntegrationScene::publish_blur(){uc_depth_blur_publish(&state_->blur_pending,&state_->blur_settings);}
void NativeIntegrationScene::draw_blur(NativeRenderer&r,float m22,float m32){
    if(!state_->blur_settings.enabled)return;
    if(!available())throw std::logic_error("Depth blur requires the active native renderobject");
    if(!state_->blur)state_->blur=std::make_unique<NativeDepthBlur>(state_->shader);
    state_->blur->draw(r,m22,m32,state_->blur_settings);
}
bool NativeIntegrationScene::shot(const UCCollisionAsset &world,uint32_t item,const float origin[3],const float direction[3]) {
    if(!origin||!direction||(item!=2&&item!=17))return false;
    float length=0;for(unsigned i=0;i<3;i++){if(!std::isfinite(origin[i])||!std::isfinite(direction[i]))return false;length+=direction[i]*direction[i];}
    if(!std::isfinite(length)||length<1e-12f)return false;
    float from[3],to[3];length=50000.f/std::sqrt(length);
    for(unsigned i=0;i<3;i++)to[i]=origin[i]+direction[i]*length;
    uc_collision_to_physics(origin,from);uc_collision_to_physics(to,to);UCCollisionRayHit hit{};
    int result=uc_collision_raycast(&world,from,to,&hit);if(result<0)return false;
    ++shots;hits+=result!=0;last_hit=result!=0;last_item=item;last_query=hit;return true;
}
