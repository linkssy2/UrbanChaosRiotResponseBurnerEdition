#ifndef D3D8_HALF_EFFECT_H
#define D3D8_HALF_EFFECT_H
#include <d3dcompiler.h>
#ifndef HE_GROUP_SERIAL
#define HE_GROUP_SERIAL() 0
#endif
/* Read-only grouping upper bound. A query boundary can split these runs further;
 * this counter never authorizes reordering or deferred compositing. */
static struct {int enabled,active;uint64_t last,length,draws,groups,max,hist[8];
 uint64_t depth_epoch,prior_depth,depth_reuses,depth_misses;uintptr_t prior_dsv;
 uintptr_t rtv,dsv;unsigned mask;} hg;
static void hg_depth_dirty(void){if(hg.enabled)++hg.depth_epoch;}
static void hg_depth_draw(ID3D11DeviceContext*c){if(!hg.enabled)return;ID3D11DepthStencilState*d=NULL;UINT ref=0;ID3D11DeviceContext_OMGetDepthStencilState(c,&d,&ref);int writes=1;if(d){D3D11_DEPTH_STENCIL_DESC dd;ID3D11DepthStencilState_GetDesc(d,&dd);writes=dd.DepthEnable&&dd.DepthWriteMask!=D3D11_DEPTH_WRITE_MASK_ZERO;ID3D11DepthStencilState_Release(d);}if(writes)hg_depth_dirty();}
static void hg_depth_record(uintptr_t dsv){if(!hg.active)return;if(hg.prior_dsv==dsv&&hg.prior_depth==hg.depth_epoch)++hg.depth_reuses;else ++hg.depth_misses;hg.prior_dsv=dsv;hg.prior_depth=hg.depth_epoch;}
static void hg_finish(void){if(!hg.length)return;unsigned b=0;uint64_t n=hg.length;while(n>1&&b<7){n>>=1;++b;}++hg.hist[b];++hg.groups;if(hg.length>hg.max)hg.max=hg.length;hg.length=0;}
static void hg_record(uint64_t serial,uintptr_t rtv,uintptr_t dsv,unsigned mask){if(!hg.active)return;if(!serial||serial!=hg.last+1||rtv!=hg.rtv||dsv!=hg.dsv||mask!=hg.mask)hg_finish();++hg.length;++hg.draws;hg.last=serial;hg.rtv=rtv;hg.dsv=dsv;hg.mask=mask;}
/* Experimental additive-quads-only adapter. Caller certifies no active guest
 * sample query and quad ownership. This is NOT a soft-particle asset allowlist.
 * Each accepted draw is composited immediately, preserving submission order. */
typedef struct HalfEffectSaved {
 ID3D11RenderTargetView*rtv;ID3D11DepthStencilView*dsv;
 ID3D11VertexShader*vs;ID3D11PixelShader*ps;ID3D11InputLayout*layout;
 ID3D11ShaderResourceView*srv;ID3D11SamplerState*sampler;
 ID3D11BlendState*blend;ID3D11DepthStencilState*depth;ID3D11RasterizerState*raster;
 FLOAT factor[4];UINT mask,stencil;D3D11_VIEWPORT viewport;D3D11_PRIMITIVE_TOPOLOGY topology;
} HalfEffectSaved;
static struct {int initialized,enabled,failed;unsigned width,height,samples;
 ID3D11VertexShader*vs;ID3D11PixelShader*depth_ps,*depth_ms_ps,*composite_ps;
 ID3D11SamplerState*sampler;ID3D11RasterizerState*raster;
 ID3D11DepthStencilState*depth_write,*depth_off;ID3D11BlendState*add[16];
 ID3D11Texture2D*color,*depth,*source;ID3D11RenderTargetView*rtv;
 ID3D11DepthStencilView*dsv;ID3D11ShaderResourceView*srv,*source_srv;
 HalfEffectSaved saved;uint64_t attempts,accepted,rejected,errors;
} he;
static uint64_t he_start_attempts,he_start_accepted,he_start_rejected;
static struct {int enabled;uint64_t serial,reductions,reuses,start_reductions,start_reuses;} hc;
static void he_capture_start(void){he_start_attempts=he.attempts;he_start_accepted=he.accepted;he_start_rejected=he.rejected;hc.start_reductions=hc.reductions;hc.start_reuses=hc.reuses;int enabled=hg.enabled;memset(&hg,0,sizeof(hg));hg.enabled=enabled;hg.active=1;}
#define HE_RELEASE(p) do{if(p){IUnknown_Release((IUnknown*)(p));(p)=NULL;}}while(0)
static void he_release_saved(void){HalfEffectSaved*s=&he.saved;
 HE_RELEASE(s->rtv);HE_RELEASE(s->dsv);HE_RELEASE(s->vs);HE_RELEASE(s->ps);HE_RELEASE(s->layout);HE_RELEASE(s->srv);HE_RELEASE(s->sampler);HE_RELEASE(s->blend);HE_RELEASE(s->depth);HE_RELEASE(s->raster);
}
static void he_targets_free(void){hc.serial=0;HE_RELEASE(he.source_srv);HE_RELEASE(he.source);HE_RELEASE(he.srv);HE_RELEASE(he.rtv);HE_RELEASE(he.dsv);HE_RELEASE(he.color);HE_RELEASE(he.depth);he.width=he.height=he.samples=0;}
static void he_shutdown(void){he_release_saved();he_targets_free();HE_RELEASE(he.vs);HE_RELEASE(he.depth_ps);HE_RELEASE(he.depth_ms_ps);HE_RELEASE(he.composite_ps);HE_RELEASE(he.sampler);HE_RELEASE(he.raster);HE_RELEASE(he.depth_write);HE_RELEASE(he.depth_off);for(unsigned i=0;i<16;++i)HE_RELEASE(he.add[i]);memset(&he,0,sizeof(he));}
static ID3DBlob*he_compile(const char*src,const char*entry,const char*profile){ID3DBlob*b=NULL,*error=NULL;HRESULT hr=D3DCompile(src,strlen(src),NULL,NULL,NULL,entry,profile,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&error);HE_RELEASE(error);if(FAILED(hr)){HE_RELEASE(b);return NULL;}return b;}
static int he_init(ID3D11Device*d){
 if(he.initialized)return he.enabled&&!he.failed;he.initialized=1;const char*p=getenv("UCRR_EFFECT_GROUP_PROBE");hg.enabled=p&&!strcmp(p,"1");const char*v=getenv("UCRR_HALF_ADDITIVE");if(!hg.enabled&&(!v||strcmp(v,"1")))return 0;he.enabled=1;if(hg.enabled)return 1;const char*cache=getenv("UCRR_HALF_DEPTH_REUSE");hc.enabled=cache&&!strcmp(cache,"1");
 const char*shader="struct V {float4 p:SV_Position;float2 uv:TEXCOORD0;}; V vs(uint id:SV_VertexID){V o;o.uv=float2((id<<1)&2,id&2);o.p=float4(o.uv*float2(2,-2)+float2(-1,1),0,1);return o;} Texture2D<float4> image:register(t0);SamplerState linear_sampler:register(s0);float4 composite(V i):SV_Target{return image.SampleLevel(linear_sampler,i.uv,0);}";
 const char*depth="Texture2D<float> z:register(t0);float main(float4 p:SV_Position):SV_Depth{uint w,h;z.GetDimensions(w,h);int2 q=int2(p.xy)*2;float d=1;[unroll]for(int y=0;y<2;y++)[unroll]for(int x=0;x<2;x++)d=min(d,z.Load(int3(min(q+int2(x,y),int2(w-1,h-1)),0)));return d;}";
 const char*depthms="Texture2DMS<float> z:register(t0);float main(float4 p:SV_Position):SV_Depth{uint w,h,n;z.GetDimensions(w,h,n);int2 q=int2(p.xy)*2;float d=1;[unroll]for(int y=0;y<2;y++)[unroll]for(int x=0;x<2;x++)for(uint s=0;s<n;s++)d=min(d,z.Load(min(q+int2(x,y),int2(w-1,h-1)),s));return d;}";
 ID3DBlob*b=he_compile(shader,"vs","vs_5_0");if(!b)goto fail;HRESULT hr=ID3D11Device_CreateVertexShader(d,ID3D10Blob_GetBufferPointer(b),ID3D10Blob_GetBufferSize(b),NULL,&he.vs);HE_RELEASE(b);if(FAILED(hr))goto fail;
 b=he_compile(shader,"composite","ps_5_0");if(!b)goto fail;hr=ID3D11Device_CreatePixelShader(d,ID3D10Blob_GetBufferPointer(b),ID3D10Blob_GetBufferSize(b),NULL,&he.composite_ps);HE_RELEASE(b);if(FAILED(hr))goto fail;
 b=he_compile(depth,"main","ps_5_0");if(!b)goto fail;hr=ID3D11Device_CreatePixelShader(d,ID3D10Blob_GetBufferPointer(b),ID3D10Blob_GetBufferSize(b),NULL,&he.depth_ps);HE_RELEASE(b);if(FAILED(hr))goto fail;
 b=he_compile(depthms,"main","ps_5_0");if(!b)goto fail;hr=ID3D11Device_CreatePixelShader(d,ID3D10Blob_GetBufferPointer(b),ID3D10Blob_GetBufferSize(b),NULL,&he.depth_ms_ps);HE_RELEASE(b);if(FAILED(hr))goto fail;
 D3D11_SAMPLER_DESC sd={0};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;sd.ComparisonFunc=D3D11_COMPARISON_ALWAYS;
 if(FAILED(ID3D11Device_CreateSamplerState(d,&sd,&he.sampler)))goto fail;
 D3D11_RASTERIZER_DESC rd={0};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;if(FAILED(ID3D11Device_CreateRasterizerState(d,&rd,&he.raster)))goto fail;
 D3D11_DEPTH_STENCIL_DESC dd={0};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;dd.DepthFunc=D3D11_COMPARISON_ALWAYS;if(FAILED(ID3D11Device_CreateDepthStencilState(d,&dd,&he.depth_write)))goto fail;dd.DepthEnable=FALSE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;if(FAILED(ID3D11Device_CreateDepthStencilState(d,&dd,&he.depth_off)))goto fail;
 return 1;
fail:he.failed=1;++he.errors;return 0;
}
static int he_targets(ID3D11Device*d,ID3D11Texture2D*source,const D3D11_TEXTURE2D_DESC*desc){
 if(he.source==source&&he.width==desc->Width&&he.height==desc->Height&&he.samples==desc->SampleDesc.Count)return 1;
 he_targets_free();D3D11_TEXTURE2D_DESC td={0};td.Width=desc->Width/2;td.Height=desc->Height/2;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
 if(FAILED(ID3D11Device_CreateTexture2D(d,&td,NULL,&he.color))||FAILED(ID3D11Device_CreateRenderTargetView(d,(ID3D11Resource*)he.color,NULL,&he.rtv))||FAILED(ID3D11Device_CreateShaderResourceView(d,(ID3D11Resource*)he.color,NULL,&he.srv)))goto fail;
 td.Format=DXGI_FORMAT_D32_FLOAT;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;if(FAILED(ID3D11Device_CreateTexture2D(d,&td,NULL,&he.depth))||FAILED(ID3D11Device_CreateDepthStencilView(d,(ID3D11Resource*)he.depth,NULL,&he.dsv)))goto fail;
 D3D11_SHADER_RESOURCE_VIEW_DESC sv={0};sv.Format=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;sv.ViewDimension=desc->SampleDesc.Count>1?D3D11_SRV_DIMENSION_TEXTURE2DMS:D3D11_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
 if(FAILED(ID3D11Device_CreateShaderResourceView(d,(ID3D11Resource*)source,&sv,&he.source_srv)))goto fail;
 he.source=source;ID3D11Texture2D_AddRef(source);he.width=desc->Width;he.height=desc->Height;he.samples=desc->SampleDesc.Count;return 1;
fail:he_targets_free();++he.errors;return 0;
}
static void he_restore_shaders(ID3D11DeviceContext*c){HalfEffectSaved*s=&he.saved;ID3D11DeviceContext_VSSetShader(c,s->vs,NULL,0);ID3D11DeviceContext_PSSetShader(c,s->ps,NULL,0);ID3D11DeviceContext_IASetInputLayout(c,s->layout);ID3D11DeviceContext_IASetPrimitiveTopology(c,s->topology);ID3D11DeviceContext_PSSetShaderResources(c,0,1,&s->srv);ID3D11DeviceContext_PSSetSamplers(c,0,1,&s->sampler);ID3D11DeviceContext_RSSetState(c,s->raster);ID3D11DeviceContext_OMSetBlendState(c,s->blend,s->factor,s->mask);ID3D11DeviceContext_OMSetDepthStencilState(c,s->depth,s->stencil);}
static int he_begin(ID3D11Device*d,ID3D11DeviceContext*c,int allowed){
 if(!he_init(d))return 0;++he.attempts;if(!allowed){++he.rejected;return 0;}HalfEffectSaved*s=&he.saved;
 ID3D11DeviceContext_OMGetRenderTargets(c,1,&s->rtv,&s->dsv);ID3D11DeviceContext_OMGetBlendState(c,&s->blend,s->factor,&s->mask);ID3D11DeviceContext_OMGetDepthStencilState(c,&s->depth,&s->stencil);ID3D11DeviceContext_RSGetState(c,&s->raster);
 if(!s->rtv||!s->dsv||!s->blend||!s->depth||!s->raster||s->mask!=0xFFFFFFFFu)goto reject;
 D3D11_BLEND_DESC bd;D3D11_DEPTH_STENCIL_DESC dd;D3D11_RASTERIZER_DESC rd;ID3D11BlendState_GetDesc(s->blend,&bd);ID3D11DepthStencilState_GetDesc(s->depth,&dd);ID3D11RasterizerState_GetDesc(s->raster,&rd);
 D3D11_RENDER_TARGET_BLEND_DESC*b=&bd.RenderTarget[0];
 if(bd.AlphaToCoverageEnable||bd.IndependentBlendEnable||!b->BlendEnable||b->DestBlend!=D3D11_BLEND_ONE||b->BlendOp!=D3D11_BLEND_OP_ADD||(b->SrcBlend!=D3D11_BLEND_ONE&&b->SrcBlend!=D3D11_BLEND_SRC_ALPHA)||((b->RenderTargetWriteMask&8)&&(b->DestBlendAlpha!=D3D11_BLEND_ONE||b->BlendOpAlpha!=D3D11_BLEND_OP_ADD))||!dd.DepthEnable||dd.DepthWriteMask!=D3D11_DEPTH_WRITE_MASK_ZERO||dd.StencilEnable||(dd.DepthFunc!=D3D11_COMPARISON_LESS&&dd.DepthFunc!=D3D11_COMPARISON_LESS_EQUAL)||rd.ScissorEnable)goto reject;
 UINT n=1;ID3D11DeviceContext_RSGetViewports(c,&n,&s->viewport);if(n!=1)goto reject;
 /* Auxiliary geometry/tessellation stages are unsupported; the runtime normally
  * leaves these empty. Never let a fullscreen helper pass through a guest stage. */
 ID3D11GeometryShader*gs=NULL;ID3D11HullShader*hs=NULL;ID3D11DomainShader*ds=NULL;
 ID3D11DeviceContext_GSGetShader(c,&gs,NULL,NULL);ID3D11DeviceContext_HSGetShader(c,&hs,NULL,NULL);ID3D11DeviceContext_DSGetShader(c,&ds,NULL,NULL);
 int extra=gs||hs||ds;HE_RELEASE(gs);HE_RELEASE(hs);HE_RELEASE(ds);if(extra)goto reject;
 if(s->viewport.MinDepth!=0||s->viewport.MaxDepth!=1)goto reject;
 ID3D11Resource*resource=NULL;ID3D11DepthStencilView_GetResource(s->dsv,&resource);ID3D11Texture2D*source=NULL;HRESULT hr=IUnknown_QueryInterface((IUnknown*)resource,&IID_ID3D11Texture2D,(void**)&source);HE_RELEASE(resource);if(FAILED(hr))goto reject;
 D3D11_TEXTURE2D_DESC td;ID3D11Texture2D_GetDesc(source,&td);
 int valid=td.Format==DXGI_FORMAT_R24G8_TYPELESS&&td.ArraySize==1&&td.MipLevels==1&&td.Width>=640&&td.Height>=480&&!(td.Width&1)&&!(td.Height&1)&&s->viewport.TopLeftX==0&&s->viewport.TopLeftY==0&&s->viewport.Width==td.Width&&s->viewport.Height==td.Height;
 ID3D11Resource*color_resource=NULL;ID3D11Texture2D*color_texture=NULL;ID3D11RenderTargetView_GetResource(s->rtv,&color_resource);
 hr=IUnknown_QueryInterface((IUnknown*)color_resource,&IID_ID3D11Texture2D,(void**)&color_texture);HE_RELEASE(color_resource);
 if(FAILED(hr))valid=0;else{D3D11_TEXTURE2D_DESC color_desc;ID3D11Texture2D_GetDesc(color_texture,&color_desc);valid=valid&&color_desc.Width==td.Width&&color_desc.Height==td.Height&&color_desc.ArraySize==1&&color_desc.MipLevels==1&&color_desc.SampleDesc.Count==td.SampleDesc.Count&&(color_desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM||color_desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM);HE_RELEASE(color_texture);}
 if(valid&&hg.enabled){hg_record(HE_GROUP_SERIAL(),(uintptr_t)s->rtv,(uintptr_t)s->dsv,b->RenderTargetWriteMask&15);hg_depth_record((uintptr_t)s->dsv);HE_RELEASE(source);++he.accepted;he_release_saved();return 0;}
 if(valid)valid=he_targets(d,source,&td);HE_RELEASE(source);if(!valid)goto reject;
 unsigned mask=b->RenderTargetWriteMask&15;if(!he.add[mask]){D3D11_BLEND_DESC add={0};add.RenderTarget[0].BlendEnable=TRUE;add.RenderTarget[0].SrcBlend=add.RenderTarget[0].DestBlend=add.RenderTarget[0].SrcBlendAlpha=add.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ONE;add.RenderTarget[0].BlendOp=add.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;add.RenderTarget[0].RenderTargetWriteMask=(UINT8)mask;if(FAILED(ID3D11Device_CreateBlendState(d,&add,&he.add[mask])))goto reject;}
 ID3D11DeviceContext_VSGetShader(c,&s->vs,NULL,NULL);ID3D11DeviceContext_PSGetShader(c,&s->ps,NULL,NULL);ID3D11DeviceContext_IAGetInputLayout(c,&s->layout);ID3D11DeviceContext_IAGetPrimitiveTopology(c,&s->topology);ID3D11DeviceContext_PSGetShaderResources(c,0,1,&s->srv);ID3D11DeviceContext_PSGetSamplers(c,0,1,&s->sampler);
 D3D11_VIEWPORT half_viewport=s->viewport;half_viewport.Width*=.5f;half_viewport.Height*=.5f;ID3D11DeviceContext_RSSetViewports(c,1,&half_viewport);uint64_t serial=HE_GROUP_SERIAL();int reuse=hc.enabled&&serial&&hc.serial&&serial==hc.serial+1;if(!reuse){ID3D11DeviceContext_RSSetState(c,he.raster);ID3D11DeviceContext_OMSetRenderTargets(c,0,NULL,he.dsv);ID3D11DeviceContext_OMSetDepthStencilState(c,he.depth_write,0);ID3D11DeviceContext_PSSetShaderResources(c,0,1,&he.source_srv);ID3D11DeviceContext_VSSetShader(c,he.vs,NULL,0);ID3D11DeviceContext_PSSetShader(c,he.samples>1?he.depth_ms_ps:he.depth_ps,NULL,0);ID3D11DeviceContext_IASetInputLayout(c,NULL);ID3D11DeviceContext_IASetPrimitiveTopology(c,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);ID3D11DeviceContext_Draw(c,3,0);
++hc.reductions;}else ++hc.reuses;hc.serial=serial;
 ID3D11ShaderResourceView*none=NULL;ID3D11DeviceContext_PSSetShaderResources(c,0,1,&none);const FLOAT clear[4]={0,0,0,0};ID3D11DeviceContext_ClearRenderTargetView(c,he.rtv,clear);ID3D11DeviceContext_OMSetRenderTargets(c,1,&he.rtv,he.dsv);he_restore_shaders(c);++he.accepted;return (int)mask+1;
reject:++he.rejected;he_release_saved();return 0;
}
static void he_end(ID3D11DeviceContext*c,int token){if(!token)return;HalfEffectSaved*s=&he.saved;
 ID3D11DeviceContext_OMSetRenderTargets(c,1,&s->rtv,NULL);ID3D11DeviceContext_RSSetViewports(c,1,&s->viewport);ID3D11DeviceContext_RSSetState(c,he.raster);ID3D11DeviceContext_OMSetDepthStencilState(c,he.depth_off,0);ID3D11DeviceContext_OMSetBlendState(c,he.add[token-1],NULL,0xFFFFFFFFu);ID3D11DeviceContext_VSSetShader(c,he.vs,NULL,0);ID3D11DeviceContext_PSSetShader(c,he.composite_ps,NULL,0);ID3D11DeviceContext_IASetInputLayout(c,NULL);ID3D11DeviceContext_IASetPrimitiveTopology(c,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);ID3D11DeviceContext_PSSetShaderResources(c,0,1,&he.srv);ID3D11DeviceContext_PSSetSamplers(c,0,1,&he.sampler);ID3D11DeviceContext_Draw(c,3,0);
 ID3D11ShaderResourceView*none=NULL;ID3D11DeviceContext_PSSetShaderResources(c,0,1,&none);ID3D11DeviceContext_OMSetRenderTargets(c,1,&s->rtv,s->dsv);he_restore_shaders(c);he_release_saved();
}
static void he_dump(const wchar_t*base){if(!he.enabled)return;wchar_t path[MAX_PATH];
 if(hg.enabled){hg_finish();hg.active=0;swprintf(path,MAX_PATH,L"%ls.effect-groups.csv",base);FILE*g=_wfopen(path,L"w");if(g){fprintf(g,"eligible_draws,optimistic_groups,longest,run1,run2_3,run4_7,run8_15,run16_31,run32_63,run64_127,run128plus,depth_reuses,depth_misses\n%llu,%llu,%llu",(unsigned long long)hg.draws,(unsigned long long)hg.groups,(unsigned long long)hg.max);for(unsigned i=0;i<8;++i)fprintf(g,",%llu",(unsigned long long)hg.hist[i]);fprintf(g,",%llu,%llu\n",(unsigned long long)hg.depth_reuses,(unsigned long long)hg.depth_misses);fclose(g);}}
 if(hc.enabled){swprintf(path,MAX_PATH,L"%ls.depth-reuse.csv",base);FILE*g=_wfopen(path,L"w");if(g){fprintf(g,"window_reductions,window_reuses\n%llu,%llu\n",(unsigned long long)(hc.reductions-hc.start_reductions),(unsigned long long)(hc.reuses-hc.start_reuses));fclose(g);}}
 swprintf(path,MAX_PATH,L"%ls.half-additive.csv",base);FILE*f=_wfopen(path,L"w");if(!f)return;fprintf(f,"lifetime_attempts,accepted,rejected,errors,window_attempts,window_accepted,window_rejected\n%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",(unsigned long long)he.attempts,(unsigned long long)he.accepted,(unsigned long long)he.rejected,(unsigned long long)he.errors,(unsigned long long)(he.attempts-he_start_attempts),(unsigned long long)(he.accepted-he_start_accepted),(unsigned long long)(he.rejected-he_start_rejected));fclose(f);}
#endif
