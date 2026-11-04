// Adapted from Darkwatch's xberecomp surround path. Keep Urban Chaos's existing
// cone/rolloff, source ownership and complete MIXIN chain; no XACT addresses.
static void SurroundLevelsDirect(Voice& v,bool surround71,float g[2][8]) {
    std::memset(g,0,sizeof(float)*16);
    if(v.is3d&&v.mode!=2){
        float stereo[2][2];LevelsDirect(v,stereo);
        float d[3]={v.p[0],v.p[1],v.p[2]};
        for(int i=0;i<3&&v.mode!=1;++i)d[i]-=g_listener[i];
        const float distance=std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
        const float forward=v.mode==1?d[2]:d[0]*g_front[0]+d[1]*g_front[1]+d[2]*g_front[2];
        const float angle=std::isfinite(distance)&&std::isfinite(forward)&&distance>1e-3f?
            std::clamp(forward/distance,-1.f,1.f):1.f;
        const float front=std::sqrt((1+angle)*.5f),rear=std::sqrt((1-angle)*.5f);
        std::array<float,2> spread{1,0};
        if(surround71&&std::isfinite(distance)&&distance>1e-3f){
            const float right[3]={g_top[1]*g_front[2]-g_top[2]*g_front[1],
                g_top[2]*g_front[0]-g_top[0]*g_front[2],g_top[0]*g_front[1]-g_top[1]*g_front[0]};
            const float side=v.mode==1?d[0]:d[0]*right[0]+d[1]*right[1]+d[2]*right[2];
            const float up=v.mode==1?d[1]:d[0]*g_top[0]+d[1]*g_top[1]+d[2]*g_top[2];
            constexpr float degrees=57.29577951308232f;
            spread=trial_audio::RearSpread(std::atan2(side,forward)*degrees,std::atan2(up,std::hypot(side,forward))*degrees);
        }
        for(unsigned c=0;c<std::min<unsigned>(v.channels,2);++c){
            g[c][0]=stereo[c][0]*front;g[c][1]=stereo[c][1]*front;
            g[c][4]=stereo[c][0]*rear*spread[0];g[c][5]=stereo[c][1]*rear*spread[0];
            g[c][6]=stereo[c][0]*rear*spread[1];g[c][7]=stereo[c][1]*rear*spread[1];
        }
        return;
    }
    static constexpr uint8_t speaker[]={0,1,2,3,4,5,0,1,4,5},defaults[]={0,1};
    const uint8_t* bins=v.nbins?v.bin:defaults;
    const unsigned n=v.nbins?v.nbins:2,voices=(std::max<unsigned>(v.channels,1)+1)/2;
    const unsigned per=std::max(n/voices,2u),step=v.channels==1?1:2;
    bool speakers=false;
    for(unsigned s=0;s<n;++s)speakers|=bins[s]<10;
    for(unsigned c=0;c<std::min<unsigned>(v.channels,2);++c)
        for(unsigned s=c;s<per&&s<n;s+=step){
            const unsigned bin=bins[s];const float gain=Db(v.bin_mb[bin])*v.volume*v.headroom;
            if(bin<10)g[c][speaker[bin]]=std::max(g[c][speaker[bin]],gain);
            else if(!speakers&&!v.output)g[c][0]=g[c][1]=std::max(g[c][0],gain);
        }
}

static void SurroundLevels(Voice& source,bool surround71,float g[2][8]) {
    std::memset(g,0,sizeof(float)*16);
    if(!source.output){if(!source.bins_empty)SurroundLevelsDirect(source,surround71,g);return;}
    if(source.nbins)SurroundLevelsDirect(source,surround71,g);
    float gain=AudioMonoSendGain(source);
    uint32_t output=source.output;size_t remaining=g_voices.size();
    while(output&&remaining--){
        auto it=g_voices.find(output);if(it==g_voices.end())break;
        Voice& bus=it->second;
        if(!bus.output||bus.nbins){
            float parent[2][8]{};
            if(!bus.bins_empty)SurroundLevelsDirect(bus,surround71,parent);
            for(unsigned c=0;c<std::min<unsigned>(source.channels,2);++c)
                for(unsigned side=0;side<8;++side)g[c][side]+=gain*parent[0][side];
            if(!bus.output)return;
        }
        gain*=AudioMonoSendGain(bus);output=bus.output;
    }
    std::memset(g,0,sizeof(float)*16);
}
