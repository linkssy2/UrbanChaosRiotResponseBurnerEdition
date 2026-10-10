#include "integration_shadows.h"
#include "integration_lighting.h"
#include "assets.h"
#include "../scene_asset.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
using namespace DirectX;
namespace {
void check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("PC shadow depth resource failed");}
constexpr unsigned resolution=512;
struct Caster {uint32_t first,count;BoundingBox bounds;};
struct DynamicCaster {Caster geometry;uint32_t light;};
struct WorldPosition {float xyz[3];};
struct Map {
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> view;
    std::array<ComPtr<ID3D11DepthStencilView>,6> depth;
    uint64_t frame=0;uint32_t index=0,source_index=UINT32_MAX;float position[3]{},radius=0;
    bool valid=false;
};
}
struct NativeIntegrationShadows::State {
    NativeRenderer &renderer;const NativeGpuMesh &mesh;
    std::vector<Caster> casters;std::vector<Map> maps;
    std::vector<DynamicCaster> dynamic;
    std::vector<WorldPosition> dynamic_positions,scratch_positions;
    std::vector<uint32_t> dynamic_indices,scratch_indices;
    std::vector<DynamicCaster> scratch_casters;
    ComPtr<ID3D11Buffer> dynamic_vertices,dynamic_elements;
    uint32_t position_capacity=0,index_capacity=0;
    ComPtr<ID3D11VertexShader> vertex;ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> matrix;ComPtr<ID3D11RasterizerState> raster;
    ComPtr<ID3D11DepthStencilState> depth_state;
    bool batch_ranges=false;
    uint64_t draws=0,frame=1,rendered=0,hits=0,indices=0;uint32_t excluded=0;
    State(NativeRenderer &r,const NativeGpuMesh &m):renderer(r),mesh(m){}
    void allocate(Map &map){
        if(map.texture)return;
        D3D11_TEXTURE2D_DESC d{};d.Width=d.Height=resolution;d.MipLevels=1;d.ArraySize=6;
        d.Format=DXGI_FORMAT_R32_TYPELESS;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;
        d.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;d.MiscFlags=D3D11_RESOURCE_MISC_TEXTURECUBE;
        check(renderer.device->CreateTexture2D(&d,nullptr,&map.texture));
        D3D11_SHADER_RESOURCE_VIEW_DESC s{};s.Format=DXGI_FORMAT_R32_FLOAT;s.ViewDimension=D3D11_SRV_DIMENSION_TEXTURECUBE;s.TextureCube.MipLevels=1;
        check(renderer.device->CreateShaderResourceView(map.texture.Get(),&s,&map.view));
        D3D11_DEPTH_STENCIL_VIEW_DESC v{};v.Format=DXGI_FORMAT_D32_FLOAT;v.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2DARRAY;v.Texture2DArray.ArraySize=1;
        for(unsigned i=0;i<6;i++){v.Texture2DArray.FirstArraySlice=i;check(renderer.device->CreateDepthStencilView(map.texture.Get(),&v,&map.depth[i]));}
    }
    void render(Map &map,const NativeLightingPoint &light,float near_z){
        allocate(map);auto *c=renderer.context.Get();
        ComPtr<ID3D11RenderTargetView> target;ComPtr<ID3D11DepthStencilView> prior_depth;
        c->OMGetRenderTargets(1,&target,&prior_depth);
        D3D11_VIEWPORT prior_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
        UINT viewport_count=D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        c->RSGetViewports(&viewport_count,prior_viewports);
        ID3D11ShaderResourceView *none=nullptr;c->PSSetShaderResources(2,1,&none);
        UINT stride=sizeof(NativeVertex),offset=0;auto *vb=mesh.vertices.Get();
        c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetIndexBuffer(mesh.indices.Get(),DXGI_FORMAT_R32_UINT,0);
        c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);c->IASetInputLayout(layout.Get());
        c->VSSetShader(vertex.Get(),nullptr,0);c->PSSetShader(nullptr,nullptr,0);
        auto *cb=matrix.Get();c->VSSetConstantBuffers(0,1,&cb);
        c->RSSetState(raster.Get());c->OMSetDepthStencilState(depth_state.Get(),0);
        D3D11_VIEWPORT viewport{0,0,float(resolution),float(resolution),0,1};c->RSSetViewports(1,&viewport);
        static const float forward[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        static const float up[6][3]={{0,1,0},{0,1,0},{0,0,-1},{0,0,1},{0,1,0},{0,1,0}};
        const auto eye=XMVectorSet(light.position[0],light.position[1],light.position[2],1);
        const auto projection=XMMatrixPerspectiveFovLH(XM_PIDIV2,1,near_z,light.radius);
        for(unsigned face=0;face<6;face++){
            c->OMSetRenderTargets(0,nullptr,map.depth[face].Get());c->ClearDepthStencilView(map.depth[face].Get(),D3D11_CLEAR_DEPTH,1,0);
            const auto view=XMMatrixLookToLH(eye,XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(forward[face])),XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(up[face])));
            XMFLOAT4X4 rows;XMStoreFloat4x4(&rows,view*projection);
            c->UpdateSubresource(matrix.Get(),0,nullptr,&rows,0,0);
            BoundingFrustum frustum(projection),world;frustum.Transform(world,XMMatrixInverse(nullptr,view));
            stride=sizeof(NativeVertex);vb=mesh.vertices.Get();
            c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetIndexBuffer(mesh.indices.Get(),DXGI_FORMAT_R32_UINT,0);
            // Same ordered triangles and depth state, fewer API submissions.
            // Do not bridge invisible gaps or change buffers with a pending range.
            uint32_t first=0,count=0;
            auto flush=[&](){if(count){c->DrawIndexed(count,first,0);indices+=count;++draws;count=0;}};
            auto append=[&](const Caster&caster){
                if(!batch_ranges||uint64_t(first)+count!=caster.first)flush();
                if(!count)first=caster.first;count+=caster.count;
            };
            for(const auto &caster:casters)if(world.Intersects(caster.bounds))append(caster);
            flush();
            if(!dynamic.empty()){
                stride=sizeof(WorldPosition);vb=dynamic_vertices.Get();c->IASetVertexBuffers(0,1,&vb,&stride,&offset);
                c->IASetIndexBuffer(dynamic_elements.Get(),DXGI_FORMAT_R32_UINT,0);
                for(const auto &caster:dynamic)if(caster.light==light.source_index&&world.Intersects(caster.geometry.bounds)){
                    append(caster.geometry);
                }
                flush();
            }
        }
        auto *rt=target.Get();c->OMSetRenderTargets(1,&rt,prior_depth.Get());c->RSSetViewports(viewport_count,prior_viewports);
        memcpy(map.position,light.position,12);map.radius=light.radius;map.valid=true;++rendered;
    }
};
NativeIntegrationShadows::NativeIntegrationShadows(NativeRenderer &r,const UCScene &scene,
    const NativeGpuMesh &mesh,const std::vector<NativeMaterial> &materials,uint32_t capacity,bool batch_ranges):state_(std::make_unique<State>(r,mesh)){
    if(!capacity||capacity>128)throw std::invalid_argument("Shadow cache capacity must be1..128");
    if(!mesh.vertices||!mesh.indices||mesh.vertex_count!=scene.vertex_count||mesh.index_count!=scene.index_count)
        throw std::invalid_argument("Shadow mesh must be the same retained scene mesh");
    auto &s=*state_;s.maps.resize(capacity);s.batch_ranges=batch_ranges;
    for(uint32_t i=0;i<scene.batch_count;i++){
        const auto &b=scene.batches[i];
        if(b.material_index>=materials.size())throw std::invalid_argument("Shadow material index");
        const auto &m=materials[b.material_index];
        if(b.light_index!=0xffff||!b.flags[1]||!b.index_count||!m.supported()||m.state.blend_mode||m.state.depth_disabled){++s.excluded;continue;}
        if(b.first_index>scene.index_count||b.index_count>scene.index_count-b.first_index||b.index_count%3)
            throw std::invalid_argument("Shadow triangle range");
        XMVECTOR low=XMVectorReplicate(std::numeric_limits<float>::max()),high=XMVectorNegate(low);
        for(uint32_t j=0;j<b.index_count;j++){
            auto index=scene.indices[b.first_index+j];if(index>=scene.vertex_count)throw std::invalid_argument("Shadow vertex index");
            auto p=scene.vertices[index].position;
            if(!std::isfinite(p[0])||!std::isfinite(p[1])||!std::isfinite(p[2]))throw std::invalid_argument("Shadow position");
            auto v=XMVectorSet(p[0],p[1],p[2],0);low=XMVectorMin(low,v);high=XMVectorMax(high,v);
        }
        Caster caster{b.first_index,b.index_count,{}};BoundingBox::CreateFromPoints(caster.bounds,low,high);
        caster.bounds.Extents.x+=1;caster.bounds.Extents.y+=1;caster.bounds.Extents.z+=1;s.casters.push_back(caster);
    }
    const char *source="cbuffer Matrix:register(b0){row_major float4x4 rows;} float4 main(float3 p:POSITION):SV_Position{return mul(float4(p,1),rows);}";
    ComPtr<ID3DBlob> code,error;check(D3DCompile(source,strlen(source),"PC shadow map",nullptr,nullptr,"main","vs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error));
    check(r.device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&s.vertex));
    D3D11_INPUT_ELEMENT_DESC input={"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};
    check(r.device->CreateInputLayout(&input,1,code->GetBufferPointer(),code->GetBufferSize(),&s.layout));
    D3D11_BUFFER_DESC cb{};cb.ByteWidth=64;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    check(r.device->CreateBuffer(&cb,nullptr,&s.matrix));
    D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;
    check(r.device->CreateRasterizerState(&rs,&s.raster));
    D3D11_DEPTH_STENCIL_DESC ds{};ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    check(r.device->CreateDepthStencilState(&ds,&s.depth_state));
}
NativeIntegrationShadows::~NativeIntegrationShadows()=default;
void NativeIntegrationShadows::set_dynamic_casters(const NativeShadowCaster *input,uint32_t count){
    auto &s=*state_;if(count>4096||(count&&!input))throw std::invalid_argument("Invalid dynamic shadow caster list");
    size_t vertices=0,elements=0;
    for(uint32_t i=0;i<count;i++){
        const auto &v=input[i];
        if(!v.vertex_count||!v.index_count||v.index_count%3||!v.positions||!v.indices||v.source_light_index==0xffffffffu||
           v.vertex_count>1048576||v.index_count>3145728)throw std::invalid_argument("Invalid dynamic shadow caster");
        vertices+=v.vertex_count;elements+=v.index_count;
        if(vertices>1048576||elements>3145728)throw std::invalid_argument("Dynamic shadow geometry bound");
        for(uint32_t j=0;j<v.vertex_count;j++)for(float f:v.positions[j])if(!std::isfinite(f))throw std::invalid_argument("Invalid dynamic shadow position");
        for(uint32_t j=0;j<v.index_count;j++)if(v.indices[j]>=v.vertex_count)throw std::invalid_argument("Invalid dynamic shadow index");
    }
    auto &p=s.scratch_positions;auto &e=s.scratch_indices;auto &b=s.scratch_casters;
    p.clear();e.clear();b.clear();p.reserve(vertices);e.reserve(elements);b.reserve(count);
    for(uint32_t i=0;i<count;i++){
        const auto &v=input[i];uint32_t base=(uint32_t)p.size();DynamicCaster caster{{(uint32_t)e.size(),v.index_count,{}},v.source_light_index};
        for(uint32_t j=0;j<v.vertex_count;j++){WorldPosition w;memcpy(w.xyz,v.positions[j],12);p.push_back(w);}
        XMVECTOR low=XMVectorReplicate(std::numeric_limits<float>::max()),high=XMVectorNegate(low);
        for(uint32_t j=0;j<v.index_count;j++){const auto k=v.indices[j];e.push_back(base+k);auto q=XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(v.positions[k]));low=XMVectorMin(low,q);high=XMVectorMax(high,q);}
        BoundingBox::CreateFromPoints(caster.geometry.bounds,low,high);
        const auto &bounds=caster.geometry.bounds;for(float f:{bounds.Center.x,bounds.Center.y,bounds.Center.z,bounds.Extents.x,bounds.Extents.y,bounds.Extents.z})if(!std::isfinite(f))throw std::invalid_argument("Invalid dynamic shadow bounds");
        caster.geometry.bounds.Extents.x+=1;caster.geometry.bounds.Extents.y+=1;caster.geometry.bounds.Extents.z+=1;b.push_back(caster);
    }
    bool same=p.size()==s.dynamic_positions.size()&&e.size()==s.dynamic_indices.size()&&b.size()==s.dynamic.size();
    if(same&&(!p.empty()&&memcmp(p.data(),s.dynamic_positions.data(),p.size()*sizeof p[0])))same=false;
    if(same&&(!e.empty()&&memcmp(e.data(),s.dynamic_indices.data(),e.size()*sizeof e[0])))same=false;
    if(same)for(size_t i=0;i<b.size();i++)if(b[i].light!=s.dynamic[i].light||b[i].geometry.first!=s.dynamic[i].geometry.first||b[i].geometry.count!=s.dynamic[i].geometry.count){same=false;break;}
    if(same)return;
    // Resources grow only when necessary. Validate all input before GPU writes.
    auto grow=[&](ComPtr<ID3D11Buffer>&buffer,uint32_t &capacity,size_t required,unsigned stride,unsigned bind){
        if(required<=capacity)return;uint32_t next=(uint32_t)required;D3D11_BUFFER_DESC d{};d.ByteWidth=next*stride;d.Usage=D3D11_USAGE_DYNAMIC;d.BindFlags=bind;d.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Buffer> replacement;check(s.renderer.device->CreateBuffer(&d,nullptr,&replacement));buffer=std::move(replacement);capacity=next;
    };
    try{
    grow(s.dynamic_vertices,s.position_capacity,p.size(),sizeof(WorldPosition),D3D11_BIND_VERTEX_BUFFER);
    grow(s.dynamic_elements,s.index_capacity,e.size(),sizeof(uint32_t),D3D11_BIND_INDEX_BUFFER);
    // Compare actual indexed silhouettes per source light. A moving actor must
    // not evict unrelated static light maps. Global merged-buffer offsets may
    // change without changing a light's world geometry.
    auto same_light=[&](uint32_t light){
        size_t old_at=0,new_at=0;
        for(;;){
            while(old_at<s.dynamic.size()&&s.dynamic[old_at].light!=light)++old_at;
            while(new_at<b.size()&&b[new_at].light!=light)++new_at;
            if(old_at==s.dynamic.size()||new_at==b.size())return old_at==s.dynamic.size()&&new_at==b.size();
            const auto &old=s.dynamic[old_at++].geometry,&next=b[new_at++].geometry;
            if(old.count!=next.count)return false;
            for(uint32_t k=0;k<old.count;k++)if(memcmp(&s.dynamic_positions[s.dynamic_indices[old.first+k]],&p[e[next.first+k]],sizeof(WorldPosition)))return false;
        }
    };
    for(auto &m:s.maps)if(m.valid&&!same_light(m.source_index))m.valid=false;
    if(!p.empty()){
        D3D11_MAPPED_SUBRESOURCE map{};check(s.renderer.context->Map(s.dynamic_vertices.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map));memcpy(map.pData,p.data(),p.size()*sizeof p[0]);s.renderer.context->Unmap(s.dynamic_vertices.Get(),0);
        check(s.renderer.context->Map(s.dynamic_elements.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map));memcpy(map.pData,e.data(),e.size()*sizeof e[0]);s.renderer.context->Unmap(s.dynamic_elements.Get(),0);
    }
    }catch(...){s.dynamic.clear();s.dynamic_positions.clear();s.dynamic_indices.clear();for(auto &m:s.maps)m.valid=false;throw;}
    s.dynamic_positions.swap(p);s.dynamic_indices.swap(e);s.dynamic.swap(b);
}
void NativeIntegrationShadows::begin_frame(){auto &s=*state_;if(++s.frame==0){s.frame=1;for(auto &m:s.maps)m.frame=0;}}
bool NativeIntegrationShadows::prepare(void *owner,uint32_t index,const NativeLightingPoint &light,NativeLightingShadow &out){
    if(!owner)return false;auto &s=*static_cast<NativeIntegrationShadows*>(owner)->state_;
    if(!light.shadows||(s.casters.empty()&&s.dynamic.empty())||!std::isfinite(light.radius)||light.radius<=1||
       !std::isfinite(light.position[0])||!std::isfinite(light.position[1])||!std::isfinite(light.position[2]))return false;
    Map *map=nullptr;
    for(auto &m:s.maps)if(m.valid&&m.index==index){map=&m;break;}
    if(!map){for(auto &m:s.maps)if(m.frame!=s.frame&&(!map||m.frame<map->frame))map=&m;if(!map)return false;map->valid=false;map->index=index;}
    const float near_z=std::max(1.f,light.radius*.001f);
    if(!map->valid||map->source_index!=light.source_index||map->radius!=light.radius||memcmp(map->position,light.position,12)){map->source_index=light.source_index;s.render(*map,light,near_z);}else ++s.hits;
    map->frame=s.frame;out={map->view.Get(),near_z,light.radius,.00001f};return true;
}
uint32_t NativeIntegrationShadows::caster_batches()const{return (uint32_t)state_->casters.size();}
uint32_t NativeIntegrationShadows::excluded_batches()const{return state_->excluded;}
uint32_t NativeIntegrationShadows::capacity()const{return (uint32_t)state_->maps.size();}
uint64_t NativeIntegrationShadows::maps_rendered()const{return state_->rendered;}
uint64_t NativeIntegrationShadows::cache_hits()const{return state_->hits;}
uint64_t NativeIntegrationShadows::indices_submitted()const{return state_->indices;}

uint64_t NativeIntegrationShadows::draw_calls()const{return state_->draws;}
