#include "control_frontend_bridge.h"
#include "frontend.h"
#include "movie.h"
#include "menu_cursor.h"
#include "graphics_settings.h"
#include <d3d11_1.h>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void check(HRESULT hr){require(SUCCEEDED(hr),"Native frontend graphics initialization failed");}
void error_text(char *out,size_t capacity,const char *text){
    if(out&&capacity){const size_t n=std::min(capacity-1,strlen(text));memcpy(out,text,n);out[n]=0;}
}
std::string bounded(const char *text,size_t limit){
    if(!text)return {};const size_t n=strnlen(text,limit+1);require(n<=limit,"Native frontend text exceeds bound");return {text,n};
}
struct ScopedPipeline {
    ID3D11DeviceContext1 *context;ComPtr<ID3DDeviceContextState> previous;
    ScopedPipeline(ID3D11DeviceContext1 *c,ID3DDeviceContextState *state):context(c){context->SwapDeviceContextState(state,&previous);}
    ~ScopedPipeline(){context->ClearState();context->SwapDeviceContextState(previous.Get(),nullptr);}
};
}
struct UCNativeControlFrontend {
    DWORD thread=GetCurrentThreadId();NativeRenderer renderer;NativeFrontend menu;NativeMovie movie;NativeFramePacer pacer;
    ComPtr<ID3D11DeviceContext1> context;ComPtr<ID3DDeviceContextState> pipeline;
    NativeFrontendSettings settings{};uint64_t identity=0;bool menu_ready=false,level_browser=false,over_game=false,pause_cached=false;
    ComPtr<ID3D11Texture2D> original_menu;ComPtr<ID3D11ShaderResourceView> original_menu_view;
    UCNativeControlFrontend(ID3D11Device *device,ID3D11DeviceContext *borrowed,const wchar_t *game){
        renderer.attach(device,borrowed,true);check(borrowed->QueryInterface(IID_PPV_ARGS(&context)));
        ComPtr<ID3D11Device1> device1;check(device->QueryInterface(IID_PPV_ARGS(&device1)));
        const auto feature=device->GetFeatureLevel();D3D_FEATURE_LEVEL selected{};
        check(device1->CreateDeviceContextState(0,&feature,1,D3D11_SDK_VERSION,__uuidof(ID3D11Device),&selected,&pipeline));
        menu.load(game,renderer);
    }
};
namespace {
void owner(UCNativeControlFrontend *state){require(state&&state->thread==GetCurrentThreadId(),"Native frontend owner thread mismatch");}
template<class F> int call(UCNativeControlFrontend *state,char *error,size_t capacity,F action){
    error_text(error,capacity,"");try{owner(state);action();return 1;}
    catch(const std::exception &e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown native frontend error");}return 0;
}
void target(UCNativeControlFrontend *state,ID3D11RenderTargetView *view,const D3D11_VIEWPORT *viewport,bool movie){
    require(view&&viewport,"Missing native frontend target");
    ComPtr<ID3D11Device> device;view->GetDevice(&device);require(device.Get()==state->renderer.device.Get(),"Native frontend target device mismatch");
    ComPtr<ID3D11Resource> resource;view->GetResource(&resource);ComPtr<ID3D11Texture2D> texture;check(resource.As(&texture));
    D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);D3D11_RENDER_TARGET_VIEW_DESC vd{};view->GetDesc(&vd);
    // Movies borrow the guest MSAA surface; menus use the resolved Present
    // target, whose cached backgrounds remain ordinary single-sample textures.
    const bool msaa=movie&&vd.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2DMS&&
        (desc.SampleDesc.Count==2||desc.SampleDesc.Count==4);
    require(msaa||(vd.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2D&&
        vd.Texture2D.MipSlice<desc.MipLevels&&desc.SampleDesc.Count==1),
        "Unsupported native frontend target view");
    const unsigned mip=msaa?0:vd.Texture2D.MipSlice;
    const unsigned w=std::max(1u,desc.Width>>mip),h=std::max(1u,desc.Height>>mip);
    require(viewport->TopLeftX==0&&viewport->TopLeftY==0&&viewport->Width==float(w)&&viewport->Height==float(h)&&
        viewport->MinDepth==0&&viewport->MaxDepth==1,"Unsupported native frontend target extent");
    ComPtr<ID3D11Predicate> predicate;BOOL value=FALSE;state->context->GetPredication(&predicate,&value);
    require(!predicate,"Native frontend cannot interrupt predication");
}
}
extern "C" UCNativeControlFrontend *uc_native_control_frontend_create(ID3D11Device *device,ID3D11DeviceContext *context,
    const wchar_t *game,char *error,size_t capacity){
    error_text(error,capacity,"");try{require(game&&*game,"Missing native frontend game assets");return new UCNativeControlFrontend(device,context,game);}
    catch(const std::exception &e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown native frontend initialization error");}return nullptr;
}
extern "C" void uc_native_control_frontend_destroy(UCNativeControlFrontend *state){delete state;}
extern "C" int uc_native_control_frontend_menu(UCNativeControlFrontend *state,const UCNativeFrontendMenu *source,char *error,size_t capacity){
    return call(state,error,capacity,[&]{
        require(source&&source->count<=20&&(!source->count||source->rows)&&!(source->flags&~15u)&&
            (source->selected==UINT32_MAX||source->selected<source->count),"Invalid native frontend menu");
        auto title=bounded(source->title,127),status=bounded(source->status,511),footer=bounded(source->footer,255);
        std::vector<NativeFrontendRow> rows;rows.reserve(source->count);
        for(unsigned i=0;i<source->count;i++){
            const auto &r=source->rows[i];require(r.enabled<=1&&r.adjustable<=1&&r.keycap<=257&&(!(source->flags&UC_NATIVE_FRONTEND_LEVEL_BROWSER)||(r.artwork_id<12&&r.completed<=4)),"Invalid native frontend row");
            rows.push_back({r.id,r.keycap,r.enabled!=0,r.adjustable!=0,bounded(r.label,127),bounded(r.value,127),bounded(r.help,511),r.artwork_id,r.completed});
        }
        NativeFrontendLevelDetails details;
        if(source->flags&UC_NATIVE_FRONTEND_REWARDS_BROWSER)details.actions=4;
        if(source->flags&UC_NATIVE_FRONTEND_LEVEL_BROWSER){
            require(source->level&&source->level->text_count>=2&&source->level->text_count<=32&&source->level->icon_count<=32&&
                source->level->texts&&(!source->level->icon_count||source->level->icons)&&!(source->level->actions&~3u)&&source->level->difficulty<4&&source->level->medals<=204,"Invalid mission details");
            details.actions=source->level->actions;details.difficulty=source->level->difficulty;details.medals=source->level->medals;
            for(unsigned i=0;i<source->level->text_count;i++){
                const auto &t=source->level->texts[i];
                require(std::isfinite(t.x)&&std::isfinite(t.y)&&t.font<=3,"Invalid mission text placement");
                details.texts.push_back({t.flags,t.color,t.font,t.x,t.y,bounded(t.prefix,31),bounded(t.suffix,63)});
            }
            for(unsigned i=0;i<source->level->icon_count;i++){
                const auto &v=source->level->icons[i];
                require(std::isfinite(v.x)&&std::isfinite(v.y)&&v.kind<=4,"Invalid mission icon placement");
                details.icons.push_back({v.kind,v.color,v.x,v.y});
            }
        }else require(!source->level,"Unexpected mission details");
        unsigned selected=source->selected;
        if(source->identity!=state->identity||!state->menu_ready){state->menu.show(NativeFrontend::Screen::closed);if(selected==UINT32_MAX&&!rows.empty())selected=0;}
        state->menu.set_external(std::move(title),std::move(rows),selected,(source->flags&1)!=0,(source->flags&2)!=0,std::move(status),std::move(footer),(source->flags&12)!=0,std::move(details));
        state->identity=source->identity;state->menu_ready=true;state->level_browser=(source->flags&12)!=0;
        // Empty snapshots revoke input during original screen transitions; they
        // must not discard the paused scene and recapture the outgoing page.
        state->over_game=(source->flags&UC_NATIVE_FRONTEND_OVER_GAME)!=0;
        if(!state->over_game)state->pause_cached=false;
    });
}
extern "C" int uc_native_control_frontend_input(UCNativeControlFrontend *state,const UCNativeFrontendInput *input,
    UCNativeFrontendEvent *out,char *error,size_t capacity){
    if(out)*out={};return call(state,error,capacity,[&]{
        require(input&&out&&input->click<=1&&state->menu_ready,"Invalid native frontend input");
        require((!input->client_width&&!input->client_height)||(input->client_width&&input->client_width<=16384&&input->client_height&&input->client_height<=16384),"Invalid native frontend input dimensions");
        if(input->client_width&&input->client_height){state->settings.width=input->client_width;state->settings.height=input->client_height;}
        state->menu.set_settings(state->settings);const unsigned prior=state->menu.selected_index();
        const auto result=input->key?state->menu.key(input->key):state->menu.mouse(input->x,input->y,input->client_width,input->client_height,input->click!=0);
        out->index=state->menu.selected_index();out->direction=result.direction;
        if(result.action==NativeFrontendAction::external_back)out->kind=UC_NATIVE_FRONTEND_BACK;
        else if(result.action==NativeFrontendAction::external_difficulty)out->kind=UC_NATIVE_FRONTEND_DIFFICULTY;
        else if(result.action==NativeFrontendAction::external_rewards)out->kind=UC_NATIVE_FRONTEND_REWARDS;
        else if(result.action==NativeFrontendAction::external_activate){out->kind=UC_NATIVE_FRONTEND_ACTIVATE;out->id=result.value;}
        else if(prior!=out->index){out->kind=UC_NATIVE_FRONTEND_HOVER;out->id=state->menu.selected_id();}
    });
}
extern "C" int uc_native_control_frontend_draw(UCNativeControlFrontend *state,ID3D11RenderTargetView *view,
    const D3D11_VIEWPORT *viewport,uint32_t client_width,uint32_t client_height,uint32_t draw_menu,char *error,size_t capacity){
    error_text(error,capacity,"");bool drawing=false;
    try{
        owner(state);require(draw_menu<=3&&((draw_menu!=1&&draw_menu!=3)||state->menu_ready)&&client_width&&client_width<=16384&&client_height&&client_height<=16384,
            "Native frontend presentation is not ready");target(state,view,viewport,draw_menu==0||draw_menu==2);
        ScopedPipeline saved(state->context.Get(),state->pipeline.Get());
        // UI and movie geometry use displayed client proportions. CONTROL may
        // stretch a4:3 physical target into a16:9 child; the viewport remains
        // physical while this NDC transform compensates that final stretch.
        auto &r=state->renderer;r.width=client_width;r.height=client_height;
        state->settings.width=r.width;state->settings.height=r.height;
        auto *c=r.context.Get();c->OMSetRenderTargets(1,&view,nullptr);c->RSSetViewports(1,viewport);
        c->IASetInputLayout(r.layout.Get());c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        c->VSSetShader(r.vertex_shader.Get(),nullptr,0);c->PSSetShader(r.pixel_shader.Get(),nullptr,0);
        auto *cb=r.constants.Get();c->VSSetConstantBuffers(0,1,&cb);c->PSSetConstantBuffers(0,1,&cb);
        state->movie.step();drawing=true;
        if(draw_menu==1||draw_menu==3){
            ID3D11ShaderResourceView *background=state->movie.texture();
            if(state->level_browser||state->over_game){
                // Reuse CONTROL's complete, animated mission information panel.
                // One GPU copy per menu frame; no readback or gameplay allocation.
                ComPtr<ID3D11Resource> resource;view->GetResource(&resource);
                ComPtr<ID3D11Texture2D> source;check(resource.As(&source));
                D3D11_TEXTURE2D_DESC desc{},prior{};source->GetDesc(&desc);
                if(state->original_menu)state->original_menu->GetDesc(&prior);
                if(!state->original_menu||prior.Width!=desc.Width||prior.Height!=desc.Height||prior.Format!=desc.Format){
                    state->original_menu_view.Reset();state->original_menu.Reset();state->pause_cached=false;
                    desc.MipLevels=desc.ArraySize=1;desc.Usage=D3D11_USAGE_DEFAULT;
                    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=desc.MiscFlags=0;
                    check(r.device->CreateTexture2D(&desc,nullptr,&state->original_menu));
                    check(r.device->CreateShaderResourceView(state->original_menu.Get(),nullptr,&state->original_menu_view));
                }
                c->OMSetRenderTargets(0,nullptr,nullptr);
                if((state->level_browser&&draw_menu!=3)||(!state->level_browser&&!state->pause_cached)){
                    c->CopySubresourceRegion(state->original_menu.Get(),0,0,0,0,source.Get(),0,nullptr);
                    state->pause_cached=state->over_game;
                }else c->CopySubresourceRegion(source.Get(),0,0,0,0,state->original_menu.Get(),0,nullptr);
                c->OMSetRenderTargets(1,&view,nullptr);background=state->original_menu_view.Get();
            }
            state->menu.draw(r,state->settings,background);
        }
        else{const float black[4]={0,0,0,1};c->ClearRenderTargetView(view,black);state->movie.draw(r,draw_menu==2?NativeMovie::Framing::stretch:NativeMovie::Framing::fit);}
        check(r.device->GetDeviceRemovedReason());return 1;
    }catch(const std::exception &e){error_text(error,capacity,e.what());}catch(...){error_text(error,capacity,"Unknown native frontend draw error");}
    return drawing?-1:0;
}
extern "C" int uc_native_control_frontend_movie_open(UCNativeControlFrontend *state,const wchar_t *file,uint32_t loop,float volume,char *error,size_t capacity){
    return call(state,error,capacity,[&]{require(file&&*file&&loop<=1,"Invalid native movie request");state->movie.open(state->renderer,file,loop!=0,volume);});
}
extern "C" int uc_native_control_frontend_movie_pause(UCNativeControlFrontend *state,uint32_t paused,char *error,size_t capacity){
    return call(state,error,capacity,[&]{require(paused<=1,"Invalid native movie pause state");state->movie.pause(paused!=0);});
}
extern "C" int uc_native_control_frontend_movie_volume(UCNativeControlFrontend *state,float volume,char *error,size_t capacity){
    return call(state,error,capacity,[&]{state->movie.set_volume(volume);});
}
extern "C" int uc_native_control_frontend_movie_tick(UCNativeControlFrontend *state,char *error,size_t capacity){
    int playing=0;return call(state,error,capacity,[&]{state->movie.step();playing=!state->movie.finished();})?playing:-1;
}
extern "C" int uc_native_control_frontend_movie_close(UCNativeControlFrontend *state,char *error,size_t capacity){
    return call(state,error,capacity,[&]{state->movie.close();});
}
extern "C" int uc_native_control_frontend_movie_finished(UCNativeControlFrontend *state){return !state||state->thread!=GetCurrentThreadId()?-1:state->movie.finished()?1:0;}
extern "C" HCURSOR uc_native_control_frontend_cursor(HWND window){
    try{thread_local NativeMenuCursor cursor;return cursor.get(window);}catch(...){return LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));}
}
extern "C" int uc_native_control_frontend_pace(UCNativeControlFrontend *state,uint32_t limit,char *error,size_t capacity){
    return call(state,error,capacity,[&]{require(limit==0||limit==30||limit==60||limit==120,"Invalid native frame limit");state->pacer.wait(limit);});
}
