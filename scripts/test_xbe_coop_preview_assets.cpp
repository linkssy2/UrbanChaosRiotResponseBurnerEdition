#include "xbe_coop_preview_assets.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <xmmintrin.h>

#define CHECK(value) do{if(!(value)){std::cerr<<"Failed line "<<__LINE__<<": " #value "\n";std::exit(1);}}while(0)
int main(int argc,char** argv){
    using namespace trial_coop_preview;
    CHECK(argc==2||argc==3);const std::filesystem::path game=argv[1];
    if(argc==3){
        CHECK(std::string(argv[2])=="--reject");Model model;model.radius=123;model.vertices.resize(1);std::string error;
        const unsigned previous=_mm_getcsr();_mm_setcsr(previous|0x6000);const unsigned requested=_mm_getcsr();
        CHECK(!Load(game,0,model,error));CHECK(_mm_getcsr()==requested);_mm_setcsr(previous);
        CHECK(model.radius==123&&model.vertices.size()==1&&!error.empty());std::cout<<"REJECT "<<error<<"\n";return 0;
    }
    constexpr unsigned expected_triangles[]={6011,5924,6136,6771,6285};
    constexpr unsigned expected_images[]={11,13,11,12,13};
    constexpr unsigned expected_alpha_low[]={1302,0,0,0,2778},expected_alpha_high[]={9510,0,0,0,4044};
    size_t total_bytes=0;
    for(unsigned character=0;character<5;++character){
        Model model;std::string error;const unsigned fp=_mm_getcsr();
        if(!Load(game,character,model,error)){std::cerr<<error<<"\n";return 1;}CHECK(error.empty()&&_mm_getcsr()==fp);
        CHECK(model.triangles.size()==expected_triangles[character]);
        CHECK(model.vertices.size()==model.triangles.size()*3);
        CHECK(model.images.size()==expected_images[character]);
        std::vector<size_t> material_triangles(model.images.size());
        size_t alpha_low=0,alpha_high=0,colors=0,pixels=0;float low[3]{INFINITY,INFINITY,INFINITY},high[3]{-INFINITY,-INFINITY,-INFINITY};
        for(const auto& v:model.vertices){
            for(unsigned k=0;k<3;++k){CHECK(std::isfinite(v.position[k]));low[k]=std::min(low[k],v.position[k]);high[k]=std::max(high[k],v.position[k]);}
            float norm=0;for(float n:v.normal){CHECK(std::isfinite(n));norm+=n*n;}CHECK(std::abs(norm-1)<.001f);
            for(float uv:v.uv)CHECK(std::isfinite(uv)&&uv>=0&&uv<=1);
        }
        // The combined standing/lowered idle must not retain the bind T-pose.
        CHECK(high[1]-low[1]>(high[0]-low[0])*1.4f);
        for(const auto& t:model.triangles){
            CHECK(t.image<model.images.size());++material_triangles[t.image];
            for(auto i:t.indices)CHECK(i<model.vertices.size());
        }
        for(auto count:material_triangles)CHECK(count>0);
        for(const auto& image:model.images){
            CHECK(image.width&&image.height&&image.width<=MaxImageDimension&&image.height<=MaxImageDimension);
            CHECK(image.rgba.size()==size_t(image.width)*image.height*4);pixels+=image.rgba.size();
            for(size_t i=0;i<image.rgba.size();i+=4){
                colors+=image.rgba[i]!=image.rgba[i+1]||image.rgba[i+1]!=image.rgba[i+2];
                const auto alpha=image.rgba[i+3];alpha_low+=alpha>0&&alpha<127;alpha_high+=alpha>=127&&alpha<255;
            }
        }
        // Original wrapped textures retain fractional alpha on both sides of127.
        CHECK(alpha_low==expected_alpha_low[character]&&alpha_high==expected_alpha_high[character]);
        CHECK(colors>1000);total_bytes+=pixels;
        const auto old_vertices=model.vertices.size();const auto old_radius=model.radius;
        CHECK(!Load(game,5,model,error)&&!error.empty());CHECK(model.vertices.size()==old_vertices&&model.radius==old_radius);
        CHECK(!Load(game/"missing-preview-archive",0,model,error));CHECK(model.vertices.size()==old_vertices&&model.radius==old_radius);
        std::cout<<"character="<<character<<" vertices="<<model.vertices.size()<<" triangles="<<model.triangles.size()
                 <<" fractional_alpha_low="<<alpha_low<<" fractional_alpha_high="<<alpha_high<<" image_bytes="<<pixels<<" extent="<<high[0]-low[0]<<","<<high[1]-low[1]<<","<<high[2]-low[2]<<"\n";
    }
    CHECK(total_bytes<8*1024*1024*4);std::cout<<"PASS all five original bodies; normalized wrapped UVs; exact-key Stand; total_image_bytes="<<total_bytes<<"\n";
}
