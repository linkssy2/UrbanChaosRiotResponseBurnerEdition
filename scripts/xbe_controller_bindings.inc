// Urban Chaos profile remapping over TGP482's input::Pad. Device discovery,
// polling and rumble stay in the maintained XboxXBERecomp input backend.
struct UCRRControllerConfig {unsigned binding[24][2]{},deadzone[2]{};};
static UCRRControllerConfig controller_config;
static SRWLOCK controller_lock=SRWLOCK_INIT;
static INIT_ONCE controller_once=INIT_ONCE_STATIC_INIT;
static LONG controller_custom=0;

static void controller_defaults(UCRRControllerConfig* profile){
    *profile={};
    for(unsigned action=0;action<24;++action)profile->binding[action][0]=action+1;
}
static bool controller_identity(const UCRRControllerConfig& profile){
    if(profile.deadzone[0]||profile.deadzone[1])return false;
    for(unsigned action=0;action<24;++action)
        if(profile.binding[action][0]!=action+1||profile.binding[action][1])return false;
    return true;
}
static void controller_publish(){InterlockedExchange(&controller_custom,!controller_identity(controller_config));}
static BOOL CALLBACK controller_load(PINIT_ONCE,PVOID,PVOID*){
    UCRRControllerConfig loaded;controller_defaults(&loaded);
    wchar_t path[MAX_PATH];
    if(controller_path(path)){
        for(unsigned action=0;action<24;++action)for(unsigned slot=0;slot<2;++slot){
            wchar_t key[32];swprintf(key,32,L"Binding%u_%u",action,slot);
            loaded.binding[action][slot]=trial_input_profile::Read(path,L"Controller",key,loaded.binding[action][slot],24);
        }
        for(unsigned stick=0;stick<2;++stick){
            wchar_t key[32];swprintf(key,32,L"Deadzone%u",stick);
            loaded.deadzone[stick]=trial_input_profile::Read(path,L"Controller",key,0,30);
        }
    }
    controller_config=loaded;controller_publish();return TRUE;
}
static void controller_init(){InitOnceExecuteOnce(&controller_once,controller_load,nullptr,nullptr);}
unsigned xbox_ControllerBinding(unsigned action,unsigned slot){
    if(action>=24||slot>=2)return 0;
    controller_init();AcquireSRWLockShared(&controller_lock);
    const unsigned source=controller_config.binding[action][slot];
    ReleaseSRWLockShared(&controller_lock);return source;
}

static int controller_strength(const input::Pad& pad,unsigned source){
    if(!source||source>24)return 0;
    if(source<=8)return (int(pad.analog[source-1])*32768+127)/255;
    if(source<=16)return (pad.buttons&controller_digital[source-9])?32768:0;
    const int axes[]={pad.lx,pad.ly,pad.rx,pad.ry};
    const unsigned direction=source-17,axis=direction/2;
    const bool positive=bool(direction%2)!=bool(axis%2);
    return std::max(positive?axes[axis]:-axes[axis],0);
}
static void controller_strengths(const input::Pad* pad,int values[24]){
    for(unsigned source=1;source<=24;++source)values[source-1]=controller_strength(*pad,source);
}
static void controller_apply_profile(input::Pad* pad,const UCRRControllerConfig* profile){
    if(controller_identity(*profile))return;
    const auto original=*pad;
    auto strength=[&](unsigned source){
        int value=controller_strength(original,source);
        if(source>=17&&source<=24){
            const int threshold=int(profile->deadzone[(source-17)/4])*32768/100;
            if(threshold)value=value<=threshold?0:(value-threshold)*32768/(32768-threshold);
        }
        return value;
    };
    int mapped[24]{};
    for(unsigned action=0;action<24;++action)
        mapped[action]=std::max(strength(profile->binding[action][0]),strength(profile->binding[action][1]));
    pad->buttons=0;
    for(unsigned action=0;action<8;++action){
        pad->analog[action]=uint8_t((mapped[action]*255+16384)/32768);
        if(mapped[action+8]>4096)pad->buttons|=controller_digital[action];
    }
    int16_t* axes[]={&pad->lx,&pad->ly,&pad->rx,&pad->ry};
    for(unsigned axis=0;axis<4;++axis){
        const unsigned pair=16+axis*2;
        const int difference=mapped[pair+1]-mapped[pair];
        *axes[axis]=int16_t(std::clamp(axis%2?-difference:difference,-32768,32767));
    }
}
static void controller_apply(input::Pad* pad){
    if(!InterlockedCompareExchange(&controller_custom,0,0))return;
    AcquireSRWLockShared(&controller_lock);const auto profile=controller_config;
    ReleaseSRWLockShared(&controller_lock);
    controller_apply_profile(pad,&profile);
}
