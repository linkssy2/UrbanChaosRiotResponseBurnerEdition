#pragma once
#include <windows.h>
#include <array>
#include <algorithm>
#include <filesystem>
#include <string>

// PC controls share the package's settings file, never Xbox profiles or saves.
class NativeInputBindings {
public:
    enum class Action : unsigned {
        forward,back,left,right,crouch,fire,shield_hold,reload,pistol,
        shotgun,taser,shield_select,molotov,weapon_next,weapon_previous,orders,secondary,aim,count
    };
    static constexpr unsigned action_count=static_cast<unsigned>(Action::count);
    static constexpr unsigned wheel_up=256,wheel_down=257;
    unsigned sensitivity=100; // Percent of the existing native mouse rotation.

    NativeInputBindings(){reset_defaults();}
    void reset_defaults(){
        for(unsigned i=0;i<action_count;++i)keys_[i]=entries_[i].default_key;
        sensitivity=100;
    }
    unsigned key(Action action)const{
        const auto i=static_cast<unsigned>(action);return i<action_count?keys_[i]:0;
    }
    static unsigned normalize_key(unsigned code){
        if(code==VK_LSHIFT||code==VK_RSHIFT)return VK_SHIFT;
        if(code==VK_LCONTROL||code==VK_RCONTROL)return VK_CONTROL;
        if(code==VK_LMENU||code==VK_RMENU)return VK_MENU;
        return code;
    }
    static bool continuous(Action action){
        return action<=Action::crouch||action==Action::shield_hold||action==Action::orders||action==Action::aim;
    }
    static bool valid_key(Action action,unsigned code){
        if(static_cast<unsigned>(action)>=action_count)return false;
        code=normalize_key(code);
        if(code==wheel_up||code==wheel_down)return !continuous(action);
        if(!code||code>254||code==VK_ESCAPE||code==VK_F5||code==VK_F6||
           code==VK_F7||code==VK_F8||code==VK_F9||code==VK_F11)return false;
        if(code==VK_LBUTTON||code==VK_RBUTTON||code==VK_MBUTTON||code==VK_XBUTTON1||code==VK_XBUTTON2)return true;
        return MapVirtualKeyW(code,MAPVK_VK_TO_VSC_EX)!=0;
    }
    // A duplicate swaps bindings. Reject the whole change if that would move
    // a wheel impulse onto a held action; neither binding changes on failure.
    bool bind(Action action,unsigned code){
        code=normalize_key(code);
        if(!valid_key(action,code))return false;
        const auto index=static_cast<unsigned>(action);
        for(unsigned i=0;i<action_count;++i)if(i!=index&&keys_[i]==code){
            if(!valid_key(static_cast<Action>(i),keys_[index]))return false;
            keys_[i]=keys_[index];break;
        }
        keys_[index]=code;return true;
    }
    static const char *action_name(Action action){
        const auto i=static_cast<unsigned>(action);return i<action_count?entries_[i].name:"";
    }
    static const char *action_description(Action action){
        const auto i=static_cast<unsigned>(action);return i<action_count?entries_[i].description:"";
    }
    std::string label(Action action)const{return key_name(key(action));}
    static std::string key_name(unsigned code){
        code=normalize_key(code);
        switch(code){
        case VK_LBUTTON:return "Left Mouse";
        case VK_RBUTTON:return "Right Mouse";
        case VK_MBUTTON:return "Middle Mouse";
        case VK_XBUTTON1:return "Mouse 4";
        case VK_XBUTTON2:return "Mouse 5";
        case wheel_up:return "Wheel Up";
        case wheel_down:return "Wheel Down";
        case VK_PAUSE:return "Pause";
        case VK_SNAPSHOT:return "Print Screen";
        default:break;
        }
        const UINT scan=MapVirtualKeyW(code,MAPVK_VK_TO_VSC_EX);
        LONG parameter=static_cast<LONG>((scan&255u)<<16);
        if((scan&0xff00u)==0xe000u||code==VK_NUMLOCK)parameter|=1L<<24;
        wchar_t wide[128]{};
        const int length=scan?GetKeyNameTextW(parameter,wide,128):0;
        if(length>0){
            const int bytes=WideCharToMultiByte(CP_UTF8,0,wide,length,nullptr,0,nullptr,nullptr);
            if(bytes>0){
                std::string result(static_cast<size_t>(bytes),'\0');
                WideCharToMultiByte(CP_UTF8,0,wide,length,result.data(),bytes,nullptr,nullptr);
                return result;
            }
        }
        return code?"Key "+std::to_string(code):"Unassigned";
    }
    void load(const std::filesystem::path &directory){
        path_=directory/L"native-settings.ini";reset_defaults();
        for(unsigned i=0;i<action_count;++i){
            const unsigned code=GetPrivateProfileIntW(L"Controls",entries_[i].setting,0,path_.c_str());
            if(code)bind(static_cast<Action>(i),code);
        }
        const unsigned value=GetPrivateProfileIntW(L"Controls",L"MouseSensitivity",100,path_.c_str());
        sensitivity=value>=25&&value<=200?value:100;
    }
    bool save()const{
        if(path_.empty()||sensitivity<25||sensitivity>200)return false;
        bool ok=true;
        auto put=[&](const wchar_t *name,unsigned code){
            const auto value=std::to_wstring(code);
            if(!WritePrivateProfileStringW(L"Controls",name,value.c_str(),path_.c_str()))ok=false;
        };
        for(unsigned i=0;i<action_count;++i)put(entries_[i].setting,keys_[i]);
        put(L"MouseSensitivity",sensitivity);return ok;
    }
private:
    struct Entry {const wchar_t *setting;const char *name,*description;unsigned default_key;};
    inline static constexpr Entry entries_[action_count]={
        {L"Forward","Move forward","Hold to move forward.",'W'},
        {L"Back","Move backward","Hold to move backward.",'S'},
        {L"Left","Strafe left","Hold to move left.",'A'},
        {L"Right","Strafe right","Hold to move right.",'D'},
        {L"Crouch","Crouch","Hold to crouch.",'C'},
        {L"Fire","Fire","Press or hold to fire the selected weapon.",VK_LBUTTON},
        {L"ShieldHold","Raise shield","Hold to raise the shield.",VK_CONTROL},
        {L"Reload","Reload","Reload the selected weapon.",'R'},
        {L"Pistol","Select pistol","Equip the pistol.",'1'},
        {L"Shotgun","Select shotgun","Equip the shotgun.",'2'},
        {L"Taser","Select taser","Equip the taser.",'3'},
        {L"ShieldSelect","Select shield","Equip the shield.",'4'},
        {L"Molotov","Select molotov","Equip a molotov.",'5'},
        {L"WeaponNext","Next weapon","Select the next weapon.",wheel_up},
        {L"WeaponPrevious","Previous weapon","Select the previous weapon.",wheel_down},
        {L"Orders","NPC orders","Hold and drag toward an available order; release to select.",VK_MBUTTON},
        {L"Secondary","Weapon secondary","Toggle secondary fire mode, or the flashlight on compatible one-handed weapons.",'T'},
        {L"Aim","Aim down sights","Hold to use the weapon's aimed position.",VK_RBUTTON}
    };
    std::array<unsigned,action_count> keys_{};
    std::filesystem::path path_;
};

// PC input adaptation: same24-count dead zone and directions as the tested
// hybrid. Queries are read-only; all recipients see one simulation-tick pulse.
struct NativeOrdersInput {
    bool held=false,opening=false;long x=0,y=0;int released=-1;
    void cancel(){*this={};}
    int direction()const{
        const long ax=std::abs(x),ay=std::abs(y);
        return ax<24&&ay<24?-1:ax>ay?(x<0?2:3):(y<0?0:1);
    }
    void press(){if(!held){held=opening=true;x=y=0;released=-1;}}
    void release(){if(held){released=direction();held=false;}}
    bool motion(long dx,long dy){
        if(!held)return false;
        x=static_cast<long>(std::clamp<int64_t>(int64_t(x)+dx,-65536,65536));
        y=static_cast<long>(std::clamp<int64_t>(int64_t(y)+dy,-65536,65536));return true;
    }
    double query(unsigned code,unsigned mode)const{
        if(mode==0)return code==31&&(held||opening)?1:0;
        return mode==2&&released>=0&&code==30u+static_cast<unsigned>(released)?1:0;
    }
    void finish_tick(){opening=false;released=-1;}
};

// Optional standalone offline check: compile this header as C++ with
// NATIVE_INPUT_BINDINGS_SELFCHECK defined and link user32. Never runs in the game.
#ifdef NATIVE_INPUT_BINDINGS_SELFCHECK
#include <cassert>
int main(){
    using B=NativeInputBindings;using A=B::Action;B bindings;
    assert(bindings.key(A::secondary)=='T'&&bindings.bind(A::secondary,VK_MBUTTON));
    assert(bindings.key(A::secondary)==VK_MBUTTON);
    assert(bindings.bind(A::forward,'S'));
    assert(bindings.key(A::forward)=='S'&&bindings.key(A::back)=='W');
    assert(!bindings.bind(A::forward,B::wheel_up));
    assert(!bindings.bind(A::weapon_next,'S'));
    assert(bindings.key(A::weapon_next)==B::wheel_up&&bindings.key(A::forward)=='S');
    assert(bindings.bind(A::fire,B::wheel_up));
    assert(bindings.key(A::weapon_next)==VK_LBUTTON&&bindings.key(A::fire)==B::wheel_up);
    assert(bindings.bind(A::reload,VK_RCONTROL));
    assert(bindings.key(A::reload)==VK_CONTROL&&bindings.key(A::shield_hold)=='R');
    for(unsigned code:{0u,static_cast<unsigned>(VK_ESCAPE),static_cast<unsigned>(VK_F5),258u})
        assert(!bindings.bind(A::reload,code));
    bindings.reset_defaults();
    for(unsigned i=0;i<B::action_count;++i)for(unsigned j=i+1;j<B::action_count;++j)
        assert(bindings.key(static_cast<A>(i))!=bindings.key(static_cast<A>(j)));
    assert(bindings.sensitivity==100&&bindings.key(A::forward)=='W');
    assert(bindings.key(A::aim)==VK_RBUTTON&&bindings.key(A::secondary)=='T');
    assert(bindings.key(A::shield_hold)==VK_CONTROL&&bindings.key(A::crouch)=='C');
    assert(!bindings.bind(A::aim,B::wheel_up));
    assert(bindings.key(A::orders)==VK_MBUTTON&&!bindings.bind(A::orders,B::wheel_up));
    NativeOrdersInput orders;
    for(int slot=0;slot<4;slot++){
        orders.press();assert(orders.query(31,0)==1&&orders.query(31,2)==0);
        orders.motion(slot==2?-24:slot==3?24:0,slot==0?-24:slot==1?24:0);
        assert(orders.direction()==slot);orders.release();
        assert(orders.query(30+slot,2)==1&&orders.query(30+slot,2)==1);
        orders.finish_tick();assert(!orders.query(30+slot,2)&&!orders.query(31,0));
    }
    orders.press();orders.motion(23,23);orders.release();assert(orders.released==-1);
    orders.press();orders.motion(LONG_MAX,LONG_MIN);assert(orders.x==65536&&orders.y==-65536);
    orders.cancel();assert(!orders.held&&!orders.opening&&orders.released==-1);
}
#endif
