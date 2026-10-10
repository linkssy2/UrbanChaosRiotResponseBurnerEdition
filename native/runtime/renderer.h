#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

/* The native engine submits owned vertices, indices and PC resources directly.
 * No command packets, GPU-register mirrors, guest addresses or shader VM. */
using Microsoft::WRL::ComPtr;
struct UCTextureAsset;
struct NativeVertex { float position[3], uv[2]; uint32_t color = 0xffffffffu; };
struct NativeGpuMesh {
    ComPtr<ID3D11Buffer> vertices, indices;
    uint32_t vertex_count = 0, index_count = 0;
    bool dynamic = false;
};
struct NativeDrawState {
    bool clamp_u=false,clamp_v=false,constant_color=false,vertex_exposure=false;
    bool vertex_alpha=false,write_alpha=true;
    float vertex_alpha_scale=1; // Actual pixel constant0 alpha for vertex-alpha materials.
    bool alpha_prepass=false,prepass_two_sided=false; /* Explicit8FF30 route, not ordinary material. */
    bool prepass_alpha_exempt=false; /* Model bit80 parent977A0: no cutoff or depth writes. */
    bool untextured_material=false; /* Authored vertex color * constant, no resource binding. */
    bool point_filter=false,depth_disabled=false; /* Explicit2D material/parent. */
    uint8_t blend_mode=0,depth_bias=0; /* 3: explicit DST_COLOR/ZERO parent. */
    float constant[4]{1,1,1,1},inverse_exposure=1;
    float uv_period[2]{}; /* Exact periodic ordinary material script. */
    bool rotor_uv=false;
};
struct NativeRenderer {
    bool profile_cpu=false;double profile_model_ms[4]{}; // Explicit diagnostic only.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapchain;
    /* Single-sample resolved resources; target/depth_view follow the active
     * scene until resolve_scene(), then point at these resources again. */
    ComPtr<ID3D11Texture2D> color, depth;
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<ID3D11DepthStencilView> depth_view;
    ComPtr<ID3D11ShaderResourceView> depth_resource;
    ComPtr<ID3D11Texture2D> tone_snapshot;
    ComPtr<ID3D11ShaderResourceView> tone_source;
    ComPtr<ID3D11VertexShader> vertex_shader,fog_vertex_shader;
    ComPtr<ID3D11PixelShader> pixel_shader,fog_pixel_shader,tone_pixel_shader;
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11RasterizerState> rasterizer,biased_rasterizer,glow_rasterizer,decal_rasterizer,prepass_rasterizer;
    ComPtr<ID3D11BlendState> blends[5][2]; /* opaque/alpha/additive/multiply/reverse-subtract, RGB/RGBA */
    ComPtr<ID3D11DepthStencilState> transparent_depth,fog_depth,prepass_depth;
    ComPtr<ID3D11SamplerState> samplers[4]; /* bit0 U-clamp, bit1 V-clamp */
    ComPtr<ID3D11SamplerState> point_sampler; /* HUD: clamp, point, base mip only. */
    unsigned width = 0, height = 0;
    float material_phase=0; /* Borrowed PC renderer clock; main advances once/frame. */

    void open(HWND window, unsigned w, unsigned h, bool software = false);
    /* Hybrid adapter: retain the existing immediate device/context and create
     * material samplers only. No window, swapchain, target or Present ownership. */
    void attach(ID3D11Device *,ID3D11DeviceContext *,bool draw_resources=false);
    void resize(unsigned w, unsigned h);
    bool supports_msaa(unsigned samples) const;
    void set_msaa(unsigned samples); /* 1 (off), 2, 4 or 8; throws without changing the mode on failure. */
    unsigned msaa_samples() const { return msaa_samples_; }
    void set_anisotropy(unsigned level); /* 1, 2, 4, 8 or 16; authored point samplers stay point. */
    unsigned anisotropy() const { return anisotropy_; }
    void set_vsync(bool enabled) { vsync_=enabled; }
    bool vsync() const { return vsync_; }
    NativeGpuMesh upload(const NativeVertex *vertices, size_t vertex_count,
                         const uint32_t *indices, size_t index_count, bool dynamic = false);
    /* Copies vertex_count packed NativeVertex records; accepts C-owned records
     * with the same verified layout without aliasing them as C++ objects. */
    void update(const NativeGpuMesh &mesh,const void *vertices,size_t vertex_count);
    ComPtr<ID3D11ShaderResourceView> upload(const UCTextureAsset &texture);
    ComPtr<ID3D11ShaderResourceView> upload_fog(const uint8_t *alpha,size_t count);
    /* Three contiguous 256-byte R/G/B curves, already resolved by their owner.
     * No curve generation, defaults, sRGB conversion or history composition. */
    ComPtr<ID3D11ShaderResourceView> upload_tone(const uint8_t *rgb,size_t count);
    void begin(const float clear[4]);
    /* End scene MSAA before heat haze, depth blur, scene history or 2D overlays.
     * Resolves color and nearest depth, and restores the single-sample targets. */
    void resolve_scene();
    void draw(const NativeGpuMesh &mesh, const float matrix[16], const float tint[4],
              ID3D11ShaderResourceView *texture = nullptr, uint32_t first_index = 0,
              uint32_t index_count = 0, const NativeDrawState &state = {});
    void fog(ID3D11ShaderResourceView *lookup,const float rgba[4]);
    /* Aligned RGBA8 point-lookup slice of A2C70. Fresh snapshot every call;
     * retail final combiner writes alpha ZERO. lookup comes from upload_tone. */
    void tone(ID3D11ShaderResourceView *lookup);
    void present();
    std::vector<uint8_t> read_rgba(); /* Verification only, never the frame loop. */
private:
    void initialize_draw_resources();
    struct SceneTargets {
        ComPtr<ID3D11Texture2D> color,depth;
        ComPtr<ID3D11RenderTargetView> target;
        ComPtr<ID3D11DepthStencilView> depth_view;
        ComPtr<ID3D11ShaderResourceView> depth_resource;
    } msaa_;
    SceneTargets make_targets(unsigned w,unsigned h,unsigned samples,bool with_color) const;
    void prepare_msaa_shaders();
    ComPtr<ID3D11RenderTargetView> single_target_;
    ComPtr<ID3D11DepthStencilView> single_depth_view_;
    ComPtr<ID3D11PixelShader> msaa_fog_shader_,resolve_depth_shader_;
    ComPtr<ID3D11DepthStencilState> resolve_depth_state_;
    unsigned msaa_samples_=1,anisotropy_=1;
    bool vsync_=true,msaa_active_=false;
};
