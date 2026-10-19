#pragma once
#include "xbe_hd_ui.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#include <bcrypt.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <fstream>
#include <span>
#include <string_view>

// Only approved art for an exact retail texture identity is loaded at startup.
namespace uc_hd_ui::restored {
enum class AlphaPolicy { Png, Source };
enum class Placement { Canvas, SourceBounds };
enum class ColorPolicy { Png, SourceChroma };
struct Crop { uint32_t x=0,y=0,width=0,height=0; };
struct Artwork {
    const char* name;
    uint32_t width,height,format;
    size_t source_bytes;
    const char* source_sha256;
    const char* png;
    const char* png_sha256;
    Crop crop{};
    AlphaPolicy alpha=AlphaPolicy::Png;
    Placement placement=Placement::Canvas;
    ColorPolicy color=ColorPolicy::Png;
};
constexpr size_t kPngLimit=32*1024*1024, kPixelLimit=64*1024*1024;
using Microsoft::WRL::ComPtr;

inline bool Digest(std::span<const uint8_t> bytes,std::array<uint8_t,32>& result) {
    if(bytes.size()>ULONG_MAX)return false;
    struct Hash {
        BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE handle=nullptr;
        ~Hash(){if(handle)BCryptDestroyHash(handle);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);}
    } hash;
    std::array<uint8_t,32> value{};
    if(BCryptOpenAlgorithmProvider(&hash.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0||
       BCryptCreateHash(hash.algorithm,&hash.handle,nullptr,0,nullptr,0,0)<0||
       BCryptHashData(hash.handle,const_cast<PUCHAR>(bytes.data()),ULONG(bytes.size()),0)<0||
       BCryptFinishHash(hash.handle,value.data(),ULONG(value.size()),0)<0)return false;
    result=value;return true;
}
inline bool MatchesDigest(std::span<const uint8_t> bytes,std::string_view hex) {
    if(hex.size()!=64)return false;
    std::array<uint8_t,32> expected{},actual{};
    auto digit=[](char c)->int{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;};
    for(size_t i=0;i<expected.size();++i) {
        const int a=digit(hex[i*2]),b=digit(hex[i*2+1]);if(a<0||b<0)return false;
        expected[i]=uint8_t(a*16+b);
    }
    return Digest(bytes,actual)&&actual==expected;
}
inline bool FileName(std::string_view name) {
    if(name.empty()||name.size()>128||!name.ends_with(".png")||name.find("..")!=name.npos)return false;
    for(char c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;
    return true;
}
inline const Artwork* Find(std::span<const Artwork> rows,std::string_view name,
                           uint32_t width,uint32_t height,uint32_t format,std::span<const uint8_t> bytes) {
    for(const auto& row:rows)
        if(row.name&&row.source_sha256&&row.png&&row.png_sha256&&name==row.name&&
           width==row.width&&height==row.height&&format==row.format&&bytes.size()==row.source_bytes&&
           FileName(row.png)&&MatchesDigest(bytes,row.source_sha256))return &row;
    return nullptr;
}
inline std::filesystem::path Directory() {
    std::array<wchar_t,32768> path{};
    const DWORD count=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
    return count&&count<path.size()?std::filesystem::path(path.data()).parent_path()/L"hd-ui":std::filesystem::path{};
}
inline bool Bounds(const Artwork& row,uint32_t width,uint32_t height,Crop& crop) {
    if(!row.width||!row.height||row.width>1024||row.height>1024||!width||!height||
       width>8192||height>8192||uint64_t(width)*height*4>kPixelLimit)return false;
    crop=row.crop;
    if(!crop.x&&!crop.y&&!crop.width&&!crop.height)crop={0,0,width,height};
    if(!crop.width||!crop.height||crop.x>width||crop.y>height||
       crop.width>width-crop.x||crop.height>height-crop.y)return false;
    // Bound sprites validate the visible silhouette, not transparent canvas margins.
    return crop.width>=row.width*2&&crop.height>=row.height*2&&
        (row.placement==Placement::SourceBounds||
         uint64_t(crop.width)*row.height==uint64_t(crop.height)*row.width);
}

inline bool ContentBounds(uint32_t width,uint32_t height,std::span<const uint8_t> bgra,Crop& output) {
    if(!width||!height||uint64_t(width)*height*4!=bgra.size())return false;
    uint32_t left=width,top=height,right=0,bottom=0;
    for(uint32_t y=0;y<height;++y)for(uint32_t x=0;x<width;++x)
        if(bgra[(size_t(y)*width+x)*4+3]>16) {
            left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);
        }
    if(left==width)return false;
    output={left,top,right-left,bottom-top};return true;
}
inline bool Pixels(IWICImagingFactory* factory,IWICBitmapSource* source,uint32_t width,uint32_t height,
                   std::vector<uint8_t>& output) {
    if(!width||!height||uint64_t(width)*height*4>kPixelLimit)return false;
    ComPtr<IWICFormatConverter> converted;
    std::vector<uint8_t> pixels(size_t(width)*height*4);
    if(FAILED(factory->CreateFormatConverter(&converted))||
       FAILED(converted->Initialize(source,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))||
       FAILED(converted->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data())))return false;
    output=std::move(pixels);return true;
}

inline bool BoundSprite(std::string_view name) {
    if(name.starts_with("hud\\"))return true;
    for(const char* leaf:{"lc_medal01.tga","lc_medal02.tga","lc_medal03.tga","lc_medal04.tga",
                         "lc_medal01smg.tga","reward_star.tga"})
        if(name.starts_with("frontend\\frt_content\\")&&name.substr(21)==leaf)return true;
    return false;
}
inline bool BlueThumbnail(const Artwork& row) {
    if(!row.name||row.width!=32||row.height!=32||row.alpha!=AlphaPolicy::Source)return false;
    const std::string_view name=row.name;
    return name.ends_with("smg.tga")&&(name.starts_with("frontend\\frt_content\\level_image_")||
                                     name.starts_with("frontend\\frt_content\\reward_image_"));
}
inline void SourceChroma(const uint8_t* source,uint8_t* pixel) {
    const unsigned maximum=std::max({source[0],source[1],source[2]});
    // Preserve almost-black authored pixels instead of magnifying compression
    // noise. Ratios below are bounded channel proportions, never 1/tiny luma.
    if(maximum<=4){std::copy_n(source,3,pixel);return;}
    const unsigned luminance=(unsigned(pixel[2])*54+unsigned(pixel[1])*183+unsigned(pixel[0])*19+128)/256;
    for(unsigned c=0;c<3;++c)pixel[c]=uint8_t((unsigned(source[c])*luminance+maximum/2)/maximum);
}

// Validate the approved file before WIC sees it; output changes only on success.
inline bool Load(const Artwork& row,const std::filesystem::path& directory,
                 std::span<const uint8_t> source_alpha,Image& output,std::span<const uint8_t> original={}) {
    if(directory.empty()||!row.png||!row.png_sha256||!FileName(row.png)||
       (row.alpha!=AlphaPolicy::Png&&row.alpha!=AlphaPolicy::Source)||
       (row.placement!=Placement::Canvas&&row.placement!=Placement::SourceBounds)||
       (row.color!=ColorPolicy::Png&&row.color!=ColorPolicy::SourceChroma)||
       (row.color==ColorPolicy::SourceChroma&&!BlueThumbnail(row))||
       (row.placement==Placement::SourceBounds&&
        (row.alpha!=AlphaPolicy::Png||!row.name||!BoundSprite(row.name))))return false;
    std::ifstream file(directory/row.png,std::ios::binary|std::ios::ate);
    if(!file)return false;
    const auto size=file.tellg();if(size<=0||size>std::streamoff(kPngLimit))return false;
    std::vector<uint8_t> bytes(static_cast<size_t>(size));file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))||
       !MatchesDigest(bytes,row.png_sha256))return false;
    struct Apartment {
        HRESULT status=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
        ~Apartment(){if(SUCCEEDED(status))CoUninitialize();}
    } apartment;
    if(FAILED(apartment.status)&&apartment.status!=RPC_E_CHANGED_MODE)return false;
    ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;
    GUID container{};UINT frames=0,width=0,height=0;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))||
       FAILED(factory->CreateStream(&stream))||FAILED(stream->InitializeFromMemory(bytes.data(),DWORD(bytes.size())))||
       FAILED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnDemand,&decoder))||
       FAILED(decoder->GetContainerFormat(&container))||container!=GUID_ContainerFormatPng||
       FAILED(decoder->GetFrameCount(&frames))||frames!=1||FAILED(decoder->GetFrame(0,&frame))||
       FAILED(frame->GetSize(&width,&height)))return false;
    Crop crop{};if(!Bounds(row,width,height,crop))return false;
    const UINT target_width=row.width*4,target_height=row.height*4;
    const size_t pixels=size_t(target_width)*target_height*4;
    if(row.alpha==AlphaPolicy::Source&&source_alpha.size()!=pixels)return false;
    ComPtr<IWICBitmapClipper> clip;
    WICRect rect{INT(crop.x),INT(crop.y),INT(crop.width),INT(crop.height)};
    if(FAILED(factory->CreateBitmapClipper(&clip))||FAILED(clip->Initialize(frame.Get(),&rect)))return false;
    ComPtr<IWICBitmapSource> source=clip;
    Crop target{0,0,target_width,target_height};
    if(row.placement==Placement::SourceBounds) {
        Crop authored{},restored{};std::vector<uint8_t> decoded;
        if(!ContentBounds(row.width,row.height,original,authored)||
           !Pixels(factory.Get(),source.Get(),crop.width,crop.height,decoded)||
           !ContentBounds(crop.width,crop.height,decoded,restored))return false;
        const uint64_t a=uint64_t(restored.width)*authored.height,b=uint64_t(restored.height)*authored.width;
        // Major silhouette changes need new art, not aggressive runtime distortion.
        if(a*4<b*3||a*4>b*5)return false;
        ComPtr<IWICBitmapClipper> content;
        WICRect content_rect{INT(restored.x),INT(restored.y),INT(restored.width),INT(restored.height)};
        if(FAILED(factory->CreateBitmapClipper(&content))||FAILED(content->Initialize(source.Get(),&content_rect)))return false;
        source=content;crop=restored;
        target={authored.x*4,authored.y*4,authored.width*4,authored.height*4};
    }
    if(crop.width!=target.width||crop.height!=target.height) {
        // Resize premultiplied pixels so transparent RGB cannot bleed into edges.
        ComPtr<IWICFormatConverter> premultiplied;ComPtr<IWICBitmapScaler> scaler;
        if(FAILED(factory->CreateFormatConverter(&premultiplied))||
           FAILED(premultiplied->Initialize(source.Get(),GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))||
           FAILED(factory->CreateBitmapScaler(&scaler))||
           FAILED(scaler->Initialize(premultiplied.Get(),target.width,target.height,WICBitmapInterpolationModeFant)))return false;
        source=scaler;
    }
    Image image{target_width,target_height,std::vector<uint8_t>(pixels)};
    std::vector<uint8_t> fitted;
    if(!Pixels(factory.Get(),source.Get(),target.width,target.height,fitted))return false;
    for(uint32_t y=0;y<target.height;++y)
        std::memcpy(image.bgra.data()+(size_t(target.y+y)*target_width+target.x)*4,
                    fitted.data()+size_t(y)*target.width*4,size_t(target.width)*4);
    for(size_t i=0;i<pixels;i+=4) {
        if(row.alpha==AlphaPolicy::Source) {
            // Restored RGB may be absent outside its own mask. Preserve authored
            // edge colour wherever the source still has coverage.
            if(!image.bgra[i+3]&&source_alpha[i+3])
                std::memcpy(image.bgra.data()+i,source_alpha.data()+i,3);
            image.bgra[i+3]=source_alpha[i+3];
        }
        if(row.color==ColorPolicy::SourceChroma&&image.bgra[i+3])SourceChroma(source_alpha.data()+i,image.bgra.data()+i);
        if(!image.bgra[i+3])image.bgra[i]=image.bgra[i+1]=image.bgra[i+2]=0;
    }
    output=std::move(image);return true;
}
}
