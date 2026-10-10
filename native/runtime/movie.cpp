#include "movie.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>
#include <xaudio2.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
void check(HRESULT h){if(FAILED(h)){char text[64];sprintf_s(text,"Native movie error 0x%08lX",static_cast<unsigned long>(h));throw std::runtime_error(text);}}
using Clock=std::chrono::steady_clock;
constexpr DWORD video=static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),audio=static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);
ComPtr<IMFSourceReader> reader(const std::filesystem::path&file,bool pixels){
    ComPtr<IMFAttributes>a;check(MFCreateAttributes(&a,1));if(pixels)check(a->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING,TRUE));
    ComPtr<IMFSourceReader>r;check(MFCreateSourceReaderFromURL(file.c_str(),a.Get(),&r));check(r->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS),FALSE));return r;
}
void format(IMFSourceReader*r,DWORD stream,const GUID&major,const GUID&subtype){
    ComPtr<IMFMediaType>t;check(MFCreateMediaType(&t));check(t->SetGUID(MF_MT_MAJOR_TYPE,major));check(t->SetGUID(MF_MT_SUBTYPE,subtype));
    check(r->SetCurrentMediaType(stream,nullptr,t.Get()));check(r->SetStreamSelection(stream,TRUE));
}
}
struct NativeMovie::Impl {
    NativeRenderer&r;bool com=false,mf=false,loop=false,done=false,paused=false,video_end=false;
    ComPtr<IMFSourceReader>decoder;ComPtr<IMFSample>next;LONGLONG next_stamp=0;
    ComPtr<ID3D11Texture2D>pixels;ComPtr<ID3D11ShaderResourceView>view;NativeGpuMesh quad;
    UINT32 width=0,height=0;LONG stride=0;double length=0,last_stamp=0,last_end=0;
    uint64_t frames=0;float volume=1;
    ComPtr<IXAudio2>engine;IXAudio2MasteringVoice*master=nullptr;IXAudio2SourceVoice*voice=nullptr;
    WAVEFORMATEX wave{};std::vector<uint8_t>pcm;Clock::time_point started{};double paused_time=0;
    explicit Impl(NativeRenderer&renderer):r(renderer){const HRESULT h=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(h!=RPC_E_CHANGED_MODE){check(h);com=true;}const HRESULT startup=MFStartup(MF_VERSION);if(FAILED(startup)){if(com)CoUninitialize();com=false;check(startup);}mf=true;}
    ~Impl(){if(voice)voice->DestroyVoice();if(master)master->DestroyVoice();engine.Reset();next.Reset();decoder.Reset();if(mf)MFShutdown();if(com)CoUninitialize();}
    double clock()const{
        if(paused)return paused_time;
        if(voice){XAUDIO2_VOICE_STATE state{};voice->GetState(&state);if(state.BuffersQueued)return double(state.SamplesPlayed)/wave.nSamplesPerSec;}
        return std::chrono::duration<double>(Clock::now()-started).count();
    }
    void sound(const std::filesystem::path&file){
        auto source=reader(file,false);ComPtr<IMFMediaType>native;
        const HRESULT result=source->GetNativeMediaType(audio,0,&native);if(result==MF_E_INVALIDSTREAMNUMBER)return;check(result);
        format(source.Get(),audio,MFMediaType_Audio,MFAudioFormat_PCM);ComPtr<IMFMediaType>type;check(source->GetCurrentMediaType(audio,&type));
        WAVEFORMATEX*allocated=nullptr;UINT32 bytes=0;check(MFCreateWaveFormatExFromMFMediaType(type.Get(),&allocated,&bytes));
        if(bytes<sizeof(WAVEFORMATEX)){CoTaskMemFree(allocated);throw std::runtime_error("Movie PCM format is truncated");}
        wave=*allocated;CoTaskMemFree(allocated);
        if(wave.wFormatTag!=WAVE_FORMAT_PCM||wave.wBitsPerSample!=16||wave.nChannels!=2||!wave.nSamplesPerSec||wave.nBlockAlign!=4)
            throw std::runtime_error("Movie conversion requires stereo PCM16");
        // ponytail: preload bounded intro/menu PCM; stream audio if future
        // movies exceed512MiB. Video remains decoded one frame ahead.
        for(;;){DWORD flags=0;LONGLONG stamp=0;ComPtr<IMFSample>sample;check(source->ReadSample(audio,0,nullptr,&flags,&stamp,&sample));
            if(flags&(MF_SOURCE_READERF_ERROR|MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))throw std::runtime_error("Movie audio format changed");
            if(sample){ComPtr<IMFMediaBuffer>buffer;check(sample->ConvertToContiguousBuffer(&buffer));BYTE*data=nullptr;DWORD count=0;check(buffer->Lock(&data,nullptr,&count));
                if(count%wave.nBlockAlign||pcm.size()+count>512u*1024u*1024u){buffer->Unlock();throw std::runtime_error("Movie audio exceeds bounded PCM extent");}
                // ASF PCM packets are contiguous; verify their timestamps to
                // avoid silently discarding a discontinuity or mixing tracks.
                const double expected=double(pcm.size()/wave.nBlockAlign)/wave.nSamplesPerSec;
                if(std::abs(double(stamp)*1e-7-expected)>.002){buffer->Unlock();throw std::runtime_error("Movie PCM timestamp discontinuity");}
                try{pcm.insert(pcm.end(),data,data+count);}catch(...){buffer->Unlock();throw;}check(buffer->Unlock());
            }
            if(flags&MF_SOURCE_READERF_ENDOFSTREAM)break;
        }
        if(pcm.empty())return;check(XAudio2Create(&engine,0,XAUDIO2_DEFAULT_PROCESSOR));check(engine->CreateMasteringVoice(&master));
    }
    void start_sound(){
        if(voice){voice->DestroyVoice();voice=nullptr;}
        if(engine&&!pcm.empty()){check(engine->CreateSourceVoice(&voice,&wave));check(voice->SetVolume(volume));XAUDIO2_BUFFER b{};b.AudioBytes=UINT32(pcm.size());b.pAudioData=pcm.data();b.Flags=XAUDIO2_END_OF_STREAM;check(voice->SubmitSourceBuffer(&b));check(voice->Start());}
        started=Clock::now();paused_time=0;
    }
    void read(){
        next.Reset();if(video_end)return;
        for(;;){DWORD flags=0;check(decoder->ReadSample(video,0,nullptr,&flags,&next_stamp,&next));
            if(flags&(MF_SOURCE_READERF_ERROR|MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))throw std::runtime_error("Movie video format changed");
            if(flags&MF_SOURCE_READERF_ENDOFSTREAM)video_end=true;
            if(next){if(next_stamp<0||double(next_stamp)*1e-7<last_stamp)throw std::runtime_error("Movie video timestamp order");++frames;return;}
            if(video_end)return;
        }
    }
    void upload(){
        ComPtr<IMFMediaBuffer>buffer;check(next->ConvertToContiguousBuffer(&buffer));ComPtr<IMF2DBuffer>two;
        BYTE*data=nullptr;LONG pitch=stride;DWORD count=0;const bool is2d=SUCCEEDED(buffer.As(&two));
        if(is2d)check(two->Lock2D(&data,&pitch));else{
            check(buffer->Lock(&data,nullptr,&count));
            if(size_t(std::abs(pitch))*height>count){buffer->Unlock();throw std::runtime_error("Movie pixel extent");}
            if(pitch<0)data+=size_t(-pitch)*(height-1);
        }
        D3D11_MAPPED_SUBRESOURCE mapped{};HRESULT status=r.context->Map(pixels.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped);
        if(SUCCEEDED(status)){
            for(UINT32 y=0;y<height;y++){auto*dst=static_cast<uint8_t*>(mapped.pData)+size_t(y)*mapped.RowPitch;std::memcpy(dst,data+ptrdiff_t(y)*pitch,size_t(width)*4);for(UINT32 x=0;x<width;x++)dst[x*4+3]=255;}
            r.context->Unmap(pixels.Get(),0);
        }
        if(is2d)check(two->Unlock2D());else check(buffer->Unlock());check(status);
        last_stamp=double(next_stamp)*1e-7;LONGLONG duration=0;if(SUCCEEDED(next->GetSampleDuration(&duration)))last_end=last_stamp+double(duration)*1e-7;else last_end=last_stamp;
    }
    void rewind(){
        next.Reset();PROPVARIANT zero{};zero.vt=VT_I8;zero.hVal.QuadPart=0;check(decoder->SetCurrentPosition(GUID_NULL,zero));
        video_end=false;last_stamp=last_end=0;read();start_sound();
    }
};
NativeMovie::NativeMovie()=default;
NativeMovie::~NativeMovie()=default;
void NativeMovie::open(NativeRenderer&r,const std::filesystem::path&file,bool loop,float volume){
    if(!std::isfinite(volume)||volume<0||volume>1)throw std::invalid_argument("Movie volume");close();auto p=std::make_unique<Impl>(r);p->loop=loop;p->volume=volume;
    p->sound(file);p->decoder=reader(file,true);format(p->decoder.Get(),video,MFMediaType_Video,MFVideoFormat_RGB32);
    ComPtr<IMFMediaType>type;check(p->decoder->GetCurrentMediaType(video,&type));check(MFGetAttributeSize(type.Get(),MF_MT_FRAME_SIZE,&p->width,&p->height));
    if(!p->width||!p->height||p->width>4096||p->height>4096)throw std::runtime_error("Movie frame size");
    UINT32 stride=0;if(SUCCEEDED(type->GetUINT32(MF_MT_DEFAULT_STRIDE,&stride)))p->stride=LONG(stride);else check(MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1,p->width,&p->stride));
    if(std::abs(p->stride)<LONG(p->width*4))throw std::runtime_error("Movie frame stride");
    PROPVARIANT duration{};const HRESULT duration_hr=p->decoder->GetPresentationAttribute(static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE),MF_PD_DURATION,&duration);
    const bool duration_valid=SUCCEEDED(duration_hr)&&duration.vt==VT_UI8;
    if(duration_valid)p->length=double(duration.uhVal.QuadPart)*1e-7;PropVariantClear(&duration);check(duration_hr);
    if(!duration_valid||p->length<=0)throw std::runtime_error("Movie duration");
    D3D11_TEXTURE2D_DESC desc{};desc.Width=p->width;desc.Height=p->height;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DYNAMIC;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    check(r.device->CreateTexture2D(&desc,nullptr,&p->pixels));check(r.device->CreateShaderResourceView(p->pixels.Get(),nullptr,&p->view));
    NativeVertex vertices[4]{};const uint32_t indices[]={0,1,2,0,2,3};p->quad=r.upload(vertices,4,indices,6,true);p->read();if(!p->next)throw std::runtime_error("Movie has no decoded frame");p->upload();p->read();p->start_sound();p_=std::move(p);
}
bool NativeMovie::step(){
    if(!p_||p_->done||p_->paused)return false;const double time=p_->clock();bool changed=false;
    while(p_->next&&double(p_->next_stamp)*1e-7<=time){p_->upload();p_->read();changed=true;}
    const double end=std::max(p_->length,p_->last_end);
    if(p_->video_end&&!p_->next&&time>=end){if(p_->loop){p_->rewind();if(p_->next){p_->upload();p_->read();}return true;}p_->done=true;if(p_->voice)check(p_->voice->Stop());}
    return changed;
}
void NativeMovie::pause(bool pause){
    if(!p_||p_->done||p_->paused==pause)return;
    if(pause){p_->paused_time=p_->clock();if(p_->voice)check(p_->voice->Stop());p_->paused=true;}
    else{p_->started=Clock::now()-std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(p_->paused_time));if(p_->voice)check(p_->voice->Start());p_->paused=false;}
}
void NativeMovie::set_volume(float volume){
    if(!std::isfinite(volume)||volume<0||volume>1)throw std::invalid_argument("Movie volume");
    if(!p_)return;if(p_->voice)check(p_->voice->SetVolume(volume));p_->volume=volume;
}
void NativeMovie::draw(NativeRenderer&r,Framing framing){
    if(!p_)return;const float sx=float(r.width)/p_->width,sy=float(r.height)/p_->height,scale=framing==Framing::cover?std::max(sx,sy):std::min(sx,sy);
    const float w=framing==Framing::stretch?float(r.width):p_->width*scale,h=framing==Framing::stretch?float(r.height):p_->height*scale;
    const float x=(r.width-w)*.5f,y=(r.height-h)*.5f,xy[4][2]={{x,y},{x+w,y},{x+w,y+h},{x,y+h}},uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
    NativeVertex vertices[4]{};for(unsigned i=0;i<4;i++){std::memcpy(vertices[i].position,xy[i],8);std::memcpy(vertices[i].uv,uv[i],8);}
    r.update(p_->quad,vertices,4);const float matrix[16]={2.f/r.width,0,0,0,0,-2.f/r.height,0,0,0,0,1,0,-1,1,0,1},white[4]={1,1,1,1};
    NativeDrawState state;state.depth_disabled=true;state.clamp_u=state.clamp_v=state.vertex_exposure=true;state.write_alpha=false;r.draw(p_->quad,matrix,white,p_->view.Get(),0,6,state);
}
void NativeMovie::close(){p_.reset();}
bool NativeMovie::finished()const{return !p_||p_->done;}
ID3D11ShaderResourceView*NativeMovie::texture()const{return p_?p_->view.Get():nullptr;}
double NativeMovie::position()const{return p_?p_->clock():0;}
double NativeMovie::duration()const{return p_?p_->length:0;}
double NativeMovie::video_timestamp()const{return p_?p_->last_stamp:0;}
uint64_t NativeMovie::decoded_frames()const{return p_?p_->frames:0;}
