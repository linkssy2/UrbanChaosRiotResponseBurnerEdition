#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include "../environment_view.h"

/* Explicit PC exposure experiment, not the full original graphics owner.
 * One view, full RGBA8/UNORM single-sample target, dimensions divisible by40/30.
 * Open once on an immediate context. Black history and retained0 deliberately
 * replace the source's uninitialized meter seed. No per-frame allocation.
 * Calls are serial; GPU failure is fatal, not a retryable frame transaction. */
class NativeEnvironment {
    template<class T>using ComPtr=Microsoft::WRL::ComPtr<T>;
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11Texture2D> history_,curve_;
    ComPtr<ID3D11ShaderResourceView> history_view_,curve_view_;
    ComPtr<ID3D11ComputeShader> gather_;
    ComPtr<ID3D11Buffer> samples_,readback_;
    ComPtr<ID3D11UnorderedAccessView> samples_view_;
    std::vector<uint8_t> sparse_;
    UCExposureProfile profile_{};
    UCEnvironmentExposure exposure_{};
    UCExposureToneProvider tone_{};
    UCEnvironmentView view_{};
    int32_t retained_=0;
    unsigned width_=0,height_=0,count_=0;
public:
    NativeEnvironment()=default;
    NativeEnvironment(const NativeEnvironment&)=delete;
    NativeEnvironment &operator=(const NativeEnvironment&)=delete;
    void open(ID3D11Device*,ID3D11DeviceContext*,unsigned width,unsigned height,
              const UCExposureProfile&);
    UCExposurePublication begin(const UCExposureInput&,uint32_t manager_flags=0);
    /* Call AFTER the admitted render. Measure the previous completed target,
     * then capture this completed target for the NEXT end. Current tone scale
     * is read by the shared CPU leaf. Logical glow presence publishes original
     * parameters only; this experiment does not render glow/history suffixes.
     * Restores every CS slot it uses; other pipeline stages are untouched. */
    void end(ID3D11Texture2D *completed,unsigned sample_enabled,
             unsigned glow_present,unsigned guard_enabled,int32_t guard_count);
    /* A curve is drawable only inside a successfully begun view phase. */
    ID3D11ShaderResourceView *lookup()const{return view_.pending?curve_view_.Get():nullptr;}
    const UCEnvironmentExposure &exposure()const{return exposure_;}
    const UCExposureToneProvider &tone()const{return tone_;}
    const UCEnvironmentView &view()const{return view_;}
    int32_t retained()const{return retained_;}
    unsigned sample_count()const{return count_;}
};
