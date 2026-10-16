#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>

namespace trial_controller {
enum Family : unsigned { Xbox, PS4, PS5, Nintendo };
// SDL owns detection; rendering only reads this small, independent snapshot.
inline std::atomic<unsigned> family{Xbox};
inline const char* Label(unsigned source,unsigned style) {
    static const char* xbox[]={"Unbound","A","B","X","Y","RB","LB","LT","RT","Start","Back","LS click","RS click","D-pad Up","D-pad Down","D-pad Left","D-pad Right","LS Left","LS Right","LS Up","LS Down","RS Left","RS Right","RS Up","RS Down"};
    static const char* ps[]={"Unbound","Cross","Circle","Square","Triangle","R1","L1","L2","R2","Options","Share","L3","R3"};
    static const char* nintendo[]={"Unbound","B","A","Y","X","R","L","ZL","ZR","+","-","LS click","RS click"};
    if(source>24)return "Invalid";
    if(source>12||style==Xbox)return xbox[source];
    if(style==Nintendo)return nintendo[source];
    if(style==PS5&&source==10)return "Create";
    return ps[source];
}
inline const char* Label(unsigned source){return Label(source,family.load());}
inline const char* CancelHint(){
    switch(family.load()){
    case PS4:return "Release first. Escape or Options + Share cancels.";
    case PS5:return "Release first. Escape or Options + Create cancels.";
    case Nintendo:return "Release first. Escape or + and - cancels.";
    default:return "Release first. Escape or Start + Back cancels.";
    }
}
}

int native_hud_keycap(uint32_t*,unsigned,const char*,const char*);
// Same keycap frame as our PC prompts; PlayStation face symbols are vectors.
inline void TrialControllerGlyph(uint32_t* pixels,unsigned source,unsigned family){
    const bool shape=(family==trial_controller::PS4||family==trial_controller::PS5)&&source>=1&&source<=4;
    native_hud_keycap(pixels,0,shape?"":trial_controller::Label(source,family),nullptr);
    if(!shape)return;
    const uint32_t colors[]={0,0xff9bb9f5,0xffed9292,0xffe5a6d8,0xff8be0b0};
    auto line=[&](float ax,float ay,float bx,float by){
        const float dx=bx-ax,dy=by-ay;
        for(int y=28;y<100;++y)for(int x=28;x<100;++x){
            float t=((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy);t=t<0?0:t>1?1:t;
            const float px=x-ax-t*dx,py=y-ay-t*dy;
            if(px*px+py*py<=6.25f)pixels[y*128+x]=colors[source];
        }
    };
    if(source==1){line(42,42,86,86);line(86,42,42,86);}
    if(source==2)for(int y=35;y<94;++y)for(int x=35;x<94;++x){
        float d=std::sqrt(float((x-64)*(x-64)+(y-64)*(y-64)));
        if(d>=23&&d<=28)pixels[y*128+x]=colors[source];
    }
    if(source==3){line(40,40,88,40);line(88,40,88,88);line(88,88,40,88);line(40,88,40,40);}
    if(source==4){line(64,36,92,88);line(92,88,36,88);line(36,88,64,36);}
}
