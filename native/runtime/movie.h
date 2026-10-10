#pragma once
#include "renderer.h"
#include <filesystem>
#include <memory>

// Native Windows decoding of the derived movie assets. No Xbox decoder state
// and no runtime FFmpeg process. Construct/use/destroy on the owning UI thread.
class NativeMovie {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    enum class Framing {fit,cover,stretch};
    NativeMovie();~NativeMovie();
    void open(NativeRenderer&,const std::filesystem::path&,bool loop=false,float volume=1.f);
    bool step(); // Upload the newest frame due on the audio playback clock.
    void draw(NativeRenderer&,Framing=Framing::fit); // Stretch retains full UVs for CONTROL's title presentation.
    void pause(bool);
    void set_volume(float); // Preserve playback position and paused state.
    void close(); // Skip and release decoder, voices and texture immediately.
    bool finished()const;
    ID3D11ShaderResourceView*texture()const;
    double position()const;
    double duration()const;
    double video_timestamp()const;
    uint64_t decoded_frames()const;
};
