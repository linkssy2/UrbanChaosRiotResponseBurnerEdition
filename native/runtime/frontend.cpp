#include "frontend.h"
#include "input_bindings.h"
extern "C" {
#include "../texture_asset/texture_asset.h"
}
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
extern "C" int native_hud_keycap(uint32_t *,unsigned,const char *,const char *);

namespace {
using Action=NativeFrontendAction;
using Screen=NativeFrontend::Screen;
constexpr unsigned control_pages=(NativeInputBindings::action_count+7)/8;
constexpr unsigned glyph_capacity=4096;
constexpr float row_x=64,row_width=512,row_height=24;
// Original frontend palette:3A8960 white,3A8900 grey,3A89C0 red220/40/0.
const float white[]={1,1,1,1},shade[]={0,0,0,.4f},selected[]={.6f,.6f,.6f,.35f};
std::string uppercase(std::string value){for(char &c:value)if(c>='a'&&c<='z')c=char(c-'a'+'A');return value;}
std::string printable(std::string value){
    // Existing font layout admits printable ASCII. Keep authored English text;
    // normalize punctuation that falls outside that recovered font policy.
    std::string result;
    for(unsigned char c:value){
        if(c==0xa9)result+="(c)";
        else if(c==0x91||c==0x92)result+='\'';
        else if(c==0x96||c==0x97)result+='-';
        else if(c>=32&&c<=126)result+=char(c);
        else if(c==9||c==10||c==13)result+=' ';
        else result+='?';
    }
    while(!result.empty()&&result.back()==' ')result.pop_back();
    return result;
}
unsigned cycle(unsigned current,std::initializer_list<unsigned> values,int direction){
    auto it=std::find(values.begin(),values.end(),current);
    size_t index=it==values.end()?0:size_t(it-values.begin());
    index=direction<0?(index+values.size()-1)%values.size():(index+1)%values.size();
    return values.begin()[index];
}
}

std::string NativeFrontend::localized(const char *key,const char *fallback)const{
    const auto *line=text_.find(key);return printable(line&&!line->variants.empty()?line->variants.front().text:fallback);
}
void NativeFrontend::load(const std::filesystem::path &game,NativeRenderer &r){
    if(loaded_)throw std::logic_error("Frontend assets already loaded");
    uint8_t hash[32];auto menu=read_archive(game/"99-01.ent",hash);
    auto global=read_archive(game/"globallanguage.lst",hash);
    auto fonts=read_archive(game/"dialoguefonts.lst",hash);
    auto art=read_archive(game/"PersistantFilesXBox.lst",hash);
    auto read=[](const UCArchive *a,const char *name,BytesOwner &out){
        const size_t i=find_resource(a,name);
        if(i==SIZE_MAX||uc_archive_read(a,i,UC_AR_LOCAL_PAYLOAD,&out.value))
            throw std::runtime_error(std::string("Frontend resource: ")+name);
    };
    for(unsigned i=0;i<2;i++){BytesOwner bytes;read(i?menu.get():global.get(),i?"99-01.exttxt":"globals.exttxt",bytes);
        text_.load(bytes.value.data,bytes.value.size,0,i!=0);
        if(i){
            // load() above validates the56-byte records and complete language
            // extent. Preserve actual authored credit order, without guessed IDs.
            uint32_t count;std::memcpy(&count,bytes.value.data+20,4);
            for(uint32_t j=0;j<count;j++){
                const auto *name=bytes.value.data+24+size_t(j)*56;
                if(std::memcmp(name,"credits-",8))continue;
                const std::string key(reinterpret_cast<const char*>(name),size_t(std::find(name,name+40,uint8_t(0))-name));
                const auto *line=text_.find(key.c_str());
                if(line&&!line->variants.empty()&&!line->variants.front().text.empty())credits_.push_back(printable(line->variants.front().text));
            }
        }
    }
    read(fonts.get(),"fonts\\small.abc",metrics_);
    if(uc_font_read(metrics_.value.data,metrics_.value.size,&font_))throw std::runtime_error("Frontend font metrics");
    read(fonts.get(),"fonts\\large.abc",title_metrics_);
    if(uc_font_read(title_metrics_.value.data,title_metrics_.value.size,&title_font_))throw std::runtime_error("Frontend title font metrics");
    auto texture=[&](const UCArchive *a,const char *name){
        BytesOwner bytes;read(a,name,bytes);
        struct Texture {UCTextureAsset value{};~Texture(){uc_texture_free(&value);}} decoded;
        if(uc_texture_decode(bytes.value.data,bytes.value.size,&decoded.value))throw std::runtime_error(std::string("Frontend texture: ")+name);
        return r.upload(decoded.value);
    };
    font_texture_=texture(art.get(),"fonts\\small%4b%.tga");
    title_texture_=texture(art.get(),"fonts\\large%4b%.tga");
    bar_=texture(menu.get(),"frontend\\frt_content\\grey_bar.tga");
    left_=texture(menu.get(),"frontend\\frt_content\\arrow_left.tga");
    right_=texture(menu.get(),"frontend\\frt_content\\arrow_right.tga");
    logo_=texture(menu.get(),"frontend\\frt_content\\fe_ztlogo%24b%.tga");
    backdrop_=texture(menu.get(),"frontend\\frt_backdrops\\frt_backdrop_memory.tga");
    thumbnail_=texture(menu.get(),"frontend\\frt_content\\level_image_02%24b%.tga");
    auto material_texture=[&](const std::string &shader){
        const UCArchive *archive=find_resource(menu.get(),shader.c_str())!=SIZE_MAX?menu.get():art.get();
        BytesOwner bytes;read(archive,shader.c_str(),bytes);
        struct Material{UCMaterialAsset value{};~Material(){uc_material_free(&value);}} material;
        if(uc_material_decode(bytes.value.data,bytes.value.size,&material.value))throw std::runtime_error("Frontend image material");
        const char *name=nullptr;
        for(unsigned d=0;d<material.value.dependency_count;d++)if(material.value.dependencies[d].kind){
            const char *candidate=material.value.dependencies[d].name;const size_t n=strlen(candidate);
            if(n>=4&&!_stricmp(candidate+n-4,".tga")){if(name)throw std::runtime_error("Ambiguous frontend image texture");name=candidate;}
        }
        if(!name)throw std::runtime_error("Missing frontend image texture");
        return texture(find_resource(archive,name)!=SIZE_MAX?archive:art.get(),name);
    };
    static const char *medal_names[]={"lc_medal01smg","lc_medal01","lc_medal02","lc_medal03","lc_medal04"};
    for(unsigned i=0;i<5;i++)medals_[i]=material_texture(std::string("frontend\\frt_content\\")+medal_names[i]+".shader");
    // Original500A98 has11 main images and one shared emergency image.
    for(unsigned i=0;i<12;i++){
        const auto number=i==11?std::string("es"):(i<9?"0":"")+std::to_string(i+1);
        const std::string stem=std::string("frontend\\frt_content\\level_image_")+number;
        mission_art_[i]=material_texture(stem+".shader");
        mission_small_[0][i]=material_texture(stem+"smg.shader");
        mission_small_[1][i]=material_texture(stem+"sm.shader");
    }
    static const char *ticks[]={"tick","tick_gold","tick_bronze"};
    for(unsigned i=0;i<3;i++)mission_complete_[i]=material_texture(std::string("frontend\\frt_content\\")+ticks[i]+".shader");
    NativeVertex vertices[4]{};const uint32_t order[]={0,1,2,0,2,3};quad_=r.upload(vertices,4,order,6,true);
    text_vertices_.reserve(2048);loaded_=true;rows();
}

void NativeFrontend::show(Screen next){
    if(next==Screen::controls)controls_parent_=screen_==Screen::pause?Screen::pause:screen_==Screen::options?Screen::options:Screen::main;
    if((next==Screen::graphics||next==Screen::options)&&screen_!=Screen::controls)parent_=screen_==Screen::pause?Screen::pause:Screen::main;
    else if(next==Screen::main||next==Screen::pause)parent_=next;
    screen_=next;selected_=0;pointer_valid_=false;status_.clear();rows();
    if(next==Screen::overwrite)selected_=1;
}
void NativeFrontend::set_external(std::string title,std::vector<NativeFrontendRow> rows,unsigned selection,
    bool horizontal,bool over_game,std::string status,std::string footer,bool level_browser,NativeFrontendLevelDetails details){
    if(rows.size()>20||selection!=UINT32_MAX&&selection>=rows.size())throw std::invalid_argument("External menu rows/selection");
    title=localized(title.c_str(),title.c_str());
    for(auto &row:rows){row.label=localized(row.label.c_str(),row.label.c_str());row.value=localized(row.value.c_str(),row.value.c_str());}
    if(screen_!=Screen::external)pointer_valid_=false;
    external_rows_=std::move(rows);external_title_=std::move(title);external_footer_=std::move(footer);
    external_horizontal_=horizontal;external_over_game_=over_game;external_level_browser_=level_browser;level_details_=std::move(details);status_=std::move(status);screen_=Screen::external;
    if(selection!=UINT32_MAX)selected_=selection;
    this->rows();
}
std::string NativeFrontend::detail_text(const NativeFrontendDetailText &detail)const{
    const auto *line=(detail.flags&0x80)?nullptr:text_.find(detail.prefix.c_str());
    const std::string source=line&&!line->variants.empty()?line->variants.front().text:detail.prefix;
    std::string result;for(unsigned char c:source)if(c==0x81)result+=detail.suffix;else result+=char(c);
    return printable(result);
}
void NativeFrontend::draw_level_details(NativeRenderer &r,float width){
    if(level_details_.texts.size()<2)return;
    const float left=314,right=width-30,description_right=right-134;
    if(selected_<external_rows_.size()&&external_rows_[selected_].artwork_id<12)image(r,mission_art_[external_rows_[selected_].artwork_id].Get(),right-120,94,120,120,white);
    const std::string title=uppercase(detail_text(level_details_.texts[0])),description=detail_text(level_details_.texts[1]);
    const float title_scale=std::min(1.f,(description_right-left)/std::max(1.f,measure(title)));
    text(title,left,94,title_scale,0xffffffff,description_right);
    float description_scale=.74f,height=0;size_t count=0;
    std::vector<UCFontQuad> quads(description.size());
    for(unsigned i=0;i<5;i++){
        if(uc_font_layout(&font_,description.c_str(),left,123,description_scale,left,description_right,0,quads.data(),quads.size(),&count,&height))throw std::runtime_error("Mission description layout");
        if(height<=102)break;description_scale*=.88f;
    }
    text(description,left,123,description_scale,0xffe0e0d8,description_right);
    std::vector<float> ys,medal_rows,icon_rows;float minimum_x=1e9f,maximum_x=-1e9f;
    auto row_y=[&](float y){for(unsigned i=0;i<ys.size();i++)if(std::fabs(ys[i]-y)<2.f)return i;ys.push_back(y);return unsigned(ys.size()-1);};
    for(unsigned i=2;i<level_details_.texts.size();i++){const auto &t=level_details_.texts[i];row_y(t.y);minimum_x=std::min(minimum_x,t.x);maximum_x=std::max(maximum_x,t.x);}
    //26F6A0 emits each medal label then its2/3 slots, with icons2px
    //above the text. Associate those ordered groups instead of making a
    //second native row for the source icon offset.
    for(const auto &t:level_details_.texts)if(t.prefix.compare(0,17,"HUD-MedalScreen-3")==0)medal_rows.push_back(t.y);
    unsigned medal_group=0;
    for(unsigned i=0;i<level_details_.icons.size();i++){
        if(i&&level_details_.icons[i].y!=level_details_.icons[i-1].y)++medal_group;
        if(medal_group>=medal_rows.size())throw std::runtime_error("Mission medal row mismatch");
        icon_rows.push_back(medal_rows[medal_group]);
    }
    std::sort(ys.begin(),ys.end());
    const float top=std::max(232.f,133.f+height+font_.height*description_scale),step=std::min(25.f,(371-top)/std::max(1.f,float(ys.size())));
    auto y_for=[&](float y){unsigned nearest=0;for(unsigned i=1;i<ys.size();i++)if(std::fabs(ys[i]-y)<std::fabs(ys[nearest]-y))nearest=i;return top+nearest*step;};
    const float text_scale=std::min(.78f,step/22.f),mid=left+(right-left)*.60f;
    for(unsigned i=2;i<level_details_.texts.size();i++){
        const auto &t=level_details_.texts[i];const bool value=maximum_x-minimum_x>1&&t.x>(minimum_x+maximum_x)*.5f;
        text(detail_text(t),value?mid:left,y_for(t.y),text_scale,t.color,value?right:mid-12);
    }
    for(unsigned row=0;row<ys.size();row++){
        unsigned count_on_row=0;for(float y:icon_rows)if(std::fabs(y_for(y)-(top+row*step))<1)count_on_row++;
        unsigned index=0;const float size=std::min(24.f,step);
        for(unsigned k=0;k<level_details_.icons.size();k++)if(std::fabs(y_for(icon_rows[k])-(top+row*step))<1){
            const auto &i=level_details_.icons[k];
            const float tint[]={((i.color>>16)&255)/255.f,((i.color>>8)&255)/255.f,(i.color&255)/255.f,(i.color>>24)/255.f};
            if(i.kind<5)image(r,medals_[i.kind].Get(),right-count_on_row*(size+5)+index*(size+5),top+row*step,size,size,tint);index++;
        }
    }
}
void NativeFrontend::set_save_slot(unsigned slot,bool available,std::string summary,bool occupied){
    if(slot<1||slot>save_summaries_.size())throw std::out_of_range("Native save slot");
    save_slot_=slot;save_available_=available;save_occupied_=available||occupied;
    save_summary_=summary.empty()?"Empty":std::move(summary);save_summaries_[slot-1]=save_summary_;rows();
}
void NativeFrontend::rows(){
    rows_.clear();
    auto add=[&](std::string label,Action action=Action::none,Screen destination=Screen::closed,bool enabled=true,std::string value={}){
        rows_.push_back({std::move(label),std::move(value),action,destination,enabled});
    };
    auto go=[&](const char *key,const char *fallback,Screen destination){add(localized(key,fallback),Action::none,destination);};
    auto setting=[&](const char *label,Action action,std::string value){add(label,action,Screen::closed,true,std::move(value));};
    auto toggle=[&](const char *label,Action action,bool value){setting(label,action,value?"On":"Off");};
    switch(screen_){
    case Screen::external:
        for(const auto &row:external_rows_)add(row.label+(external_horizontal_&&!row.value.empty()?": "+row.value:""),
            Action::external_activate,Screen::closed,row.enabled,external_horizontal_?"":row.value);
        break;
    case Screen::splash:add("Click or press Enter",Action::none,Screen::main);break;
    case Screen::main:
        go("NewFrontend-Singleplayer-1","Single player",Screen::profiles);
        add(localized("NewFrontEnd-LocalNetwork-2","System Link"),Action::none,Screen::closed,false);
        add(localized("NewFrontEnd-Internet-2","Xbox Live"),Action::none,Screen::closed,false);
        go("NewFrontEnd-Options-1","Options",Screen::options);
        go("NewFrontEnd-Credits-1","Credits",Screen::credits);
        add("Graphics",Action::none,Screen::graphics);
        add("Controls",Action::none,Screen::controls);
        add("Quit",Action::quit);break;
    case Screen::profiles:
        for(unsigned i=0;i<save_summaries_.size();i++)add("Slot "+std::to_string(i+1)+": "+save_summaries_[i],Action::select_save_slot);
        go("NewFrontEnd-Back-1","Back",Screen::main);break;
    case Screen::singleplayer:
        add("Continue",Action::resume_mission,Screen::closed,save_available_);
        add("Start new mission",save_occupied_?Action::none:Action::start_mission,save_occupied_?Screen::overwrite:Screen::closed);
        add("Other missions",Action::none,Screen::closed,false);
        go("NewFrontEnd-Back-1","Back",Screen::profiles);break;
    case Screen::overwrite:
        add("Yes, start new mission",Action::start_mission);
        add("No, go back",Action::none,Screen::singleplayer);break;
    case Screen::graphics:
        setting("Resolution",Action::resolution,std::to_string(settings_.width)+" x "+std::to_string(settings_.height));
        toggle("Fullscreen",Action::fullscreen,settings_.fullscreen);
        setting("Anti-aliasing (MSAA)",Action::msaa,settings_.msaa==1?"Off":std::to_string(settings_.msaa)+"x");
        setting("Anisotropic filtering",Action::anisotropy,std::to_string(settings_.anisotropy)+"x");
        toggle("Vertical sync",Action::vsync,settings_.vsync);
        setting("Frame limit",Action::frame_limit,settings_.frame_limit?std::to_string(settings_.frame_limit)+" FPS":"Unlimited");
        toggle("FPS counter",Action::fps_counter,settings_.fps_counter);
        toggle("Dynamic lighting",Action::lighting,settings_.lighting);
        toggle("Shadows",Action::shadows,settings_.shadows);
        toggle("Color pass",Action::color,settings_.color);
        go("NewFrontEnd-Back-1","Back",parent_);break;
    case Screen::options:add("Controls",Action::none,Screen::controls);go("NewFrontEnd-Back-1","Back",parent_);break;
    case Screen::controls:
        for(unsigned i=0;i<8;i++){
            const auto action=static_cast<NativeInputBindings::Action>(controls_page_*8+i);
            const bool valid=controls_page_*8+i<NativeInputBindings::action_count;
            add(NativeInputBindings::action_name(action),Action::rebind,Screen::closed,valid&&bindings_!=nullptr,valid?(bindings_?bindings_->label(action):"Unavailable"):"");
        }
        add("Mouse sensitivity",Action::mouse_sensitivity,Screen::closed,bindings_!=nullptr,std::to_string(bindings_?bindings_->sensitivity:100)+"%");
        add("Previous page",Action::none,Screen::closed,controls_page_>0);
        add("Next page",Action::none,Screen::closed,controls_page_+1<control_pages);
        add("Reset defaults",Action::controls_reset,Screen::closed,bindings_!=nullptr);
        go("NewFrontEnd-Back-1","Back",controls_parent_);break;
    case Screen::credits:
        add("Previous page",Action::none,Screen::closed,credits_page_>0);
        add("Next page",Action::none,Screen::closed,(credits_page_+1)*5<credits_.size());
        go("NewFrontEnd-Back-1","Back",Screen::main);break;
    case Screen::pause:
        add("Resume",Action::resume);add("Graphics",Action::none,Screen::graphics);
        add("Controls",Action::none,Screen::controls);
        go("NewFrontEnd-Options-1","Options",Screen::options);
        add("Save checkpoint",Action::save_checkpoint);
        add("Main menu",Action::main_menu);add("Quit",Action::quit);break;
    default:break;
    }
    if(selected_>=rows_.size())selected_=0;
    if(!rows_.empty()&&!rows_[selected_].enabled)select(1);
}
NativeFrontend::Bounds NativeFrontend::bounds(unsigned index)const{
    if(horizontal()){
        const float scale=menu_scale();float total=0,x=0;
        for(unsigned i=0;i<rows_.size();i++)total+=measure(rows_[i].label,scale)+18;
        x=(canvas_width()-total)*.5f;
        for(unsigned i=0;i<index;i++)x+=measure(rows_[i].label,scale)+18;
        return {x,390,measure(rows_[index].label,scale)+18,26};
    }
    if(screen_==Screen::external){
        if(external_level_browser_){const float step=std::min(24.f,276.f/std::max(1.f,float(rows_.size())));return {12,94+index*step,(level_details_.actions&4)?188.f:60.f,step};}
        const float step=std::min(26.f,268.f/std::max(1.f,float(rows_.size())));
        return {row_x,100+index*step,row_width,step};
    }
    if(screen_==Screen::controls){
        if(index<9)return {row_x,107+index*23.f,row_width,22};
        if(index<11)return {index==9?64.f:332.f,365,244,row_height};
        return {row_x,index==11?392.f:420.f,row_width,row_height};
    }
    float y=160+index*38.f;
    if(screen_==Screen::splash)y=370;
    if(screen_==Screen::options)y=index?416.f:365.f;
    if(screen_==Screen::credits)y=365+index*26.f;
    if(screen_==Screen::graphics)y=index==10?416:111+index*26.f;
    if(screen_==Screen::overwrite)y=300+index*38.f;
    if(screen_==Screen::pause)y=142+index*34.f;
    return {row_x,y,row_width,row_height};
}
NativeFrontend::Bounds NativeFrontend::level_button(unsigned index)const{
    const float width=(canvas_width()-96)/4;
    return {30+index*(width+12),399,width,33};
}
float NativeFrontend::canvas_width()const{
    if(!settings_.width||!settings_.height)return 640;
    return settings_.width/std::min(settings_.width/640.f,settings_.height/480.f);
}
float NativeFrontend::measure(const std::string &value,float scale,const UCFont *font)const{
    if(!loaded_)return float(value.size())*8*scale;
    if(value.empty())return 0;
    std::vector<UCFontQuad> quads(value.size());size_t count=0;float height=0;
    if(uc_font_layout(font?font:&font_,value.c_str(),0,0,scale,0,10000,0,quads.data(),quads.size(),&count,&height))throw std::runtime_error("Frontend text measurement");
    return count?quads[count-1].x1:0;
}
bool NativeFrontend::horizontal()const{return screen_==Screen::main||screen_==Screen::pause||screen_==Screen::singleplayer||screen_==Screen::external&&external_horizontal_;}
float NativeFrontend::menu_scale()const{
    float width=0;for(const auto &row:rows_)width+=measure(row.label);
    return std::min(1.f,(canvas_width()-40-18*rows_.size())/std::max(1.f,width));
}
void NativeFrontend::select(int direction){
    if(rows_.empty())return;
    const auto previous=selected_;
    for(size_t i=0;i<rows_.size();i++){
        selected_=direction<0?(selected_+unsigned(rows_.size())-1)%unsigned(rows_.size()):(selected_+1)%unsigned(rows_.size());
        if(rows_[selected_].enabled){if(selected_!=previous)status_.clear();return;}
    }
}
NativeFrontendEvent NativeFrontend::back(){
    if(screen_==Screen::external)return {Action::external_back};
    if(screen_==Screen::pause)return {Action::resume};
    if(screen_==Screen::controls)show(controls_parent_);
    else if(screen_==Screen::graphics||screen_==Screen::options)show(parent_);
    else if(screen_==Screen::singleplayer)show(Screen::profiles);
    else if(screen_==Screen::overwrite)show(Screen::singleplayer);
    else if(screen_==Screen::profiles||screen_==Screen::credits)show(Screen::main);
    else if(screen_==Screen::splash)show(Screen::main);
    return {};
}
NativeFrontendEvent NativeFrontend::activate(int direction){
    if(selected_>=rows_.size()||!rows_[selected_].enabled)return {};
    if(screen_==Screen::external)return {Action::external_activate,external_rows_[selected_].id,direction};
    const auto row=rows_[selected_];
    if(row.action==Action::select_save_slot){const unsigned slot=selected_+1;show(Screen::singleplayer);return {Action::select_save_slot,slot};}
    if(screen_==Screen::credits&&selected_<2){credits_page_+=selected_?1u:unsigned(-1);rows();return {};}
    if(screen_==Screen::controls&&(selected_==9||selected_==10)){controls_page_+=selected_==9?unsigned(-1):1u;selected_=0;status_.clear();rows();return {};}
    if(row.destination!=Screen::closed){show(row.destination);return {};}
    unsigned value=0;
    switch(row.action){
    case Action::rebind:value=controls_page_*8+selected_;break;
    case Action::mouse_sensitivity:{const unsigned current=bindings_?bindings_->sensitivity:100;
        value=direction<0?(current<=25?200:(current-1)/25*25):(current>=200?25:(current/25+1)*25);break;}
    case Action::resolution:value=cycle(settings_.height,{720,1080,1440},direction);break;
    case Action::msaa:value=cycle(settings_.msaa,{1,2,4,8},direction);break;
    case Action::anisotropy:value=cycle(settings_.anisotropy,{1,2,4,8,16},direction);break;
    case Action::frame_limit:value=cycle(settings_.frame_limit,{0,30,60,120},direction);break;
    case Action::fullscreen:value=!settings_.fullscreen;break;
    case Action::vsync:value=!settings_.vsync;break;
    case Action::fps_counter:value=!settings_.fps_counter;break;
    case Action::lighting:value=!settings_.lighting;break;
    case Action::shadows:value=!settings_.shadows;break;
    case Action::color:value=!settings_.color;break;
    default:break;
    }
    return {row.action,value};
}
NativeFrontendEvent NativeFrontend::mouse(float x,float y,unsigned width,unsigned height,bool click){
    if(!active()||screen_==Screen::loading||!width||!height||!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=width||y>=height)return {};
    if(!settings_.width||!settings_.height)return {};
    // DXGI presents the fixed render size into the actual client. Reverse that
    // scaling first, including user-resized or work-area-clamped windows.
    x=x*settings_.width/width;y=y*settings_.height/height;
    width=settings_.width;height=settings_.height;
    const float scale=std::min(width/640.f,height/480.f),ox=screen_==Screen::splash?(width-640*scale)*.5f:0,oy=(height-480*scale)*.5f;
    x=(x-ox)/scale;y=(y-oy)/scale;
    if(x<0||x>=(screen_==Screen::splash?640:canvas_width())||y<0||y>=480)return {};
    const bool moved=!pointer_valid_||std::fabs(x-pointer_x_)>=4||std::fabs(y-pointer_y_)>=4;
    if(!click&&!moved)return {};
    pointer_valid_=true;pointer_x_=x;pointer_y_=y;rows();
    if(screen_==Screen::splash&&click){show(Screen::main);return {};}
    if(screen_==Screen::external&&external_level_browser_&&!(level_details_.actions&4)&&click){
        for(unsigned i=0;i<4;i++){const auto r=level_button(i);if(x>=r.x&&x<r.x+r.width&&y>=r.y&&y<r.y+r.height){
            if(i==0)return back();if(i==3)return activate(0);
            if(level_details_.actions&(1u<<(i-1)))return {i==1?Action::external_difficulty:Action::external_rewards};
            return {};
        }}
    }
    for(unsigned i=0;i<rows_.size();i++){
        const auto r=bounds(i);if(!rows_[i].enabled||x<r.x||x>=r.x+r.width||y<r.y||y>=r.y+r.height)continue;
        if(selected_!=i)status_.clear();selected_=i;
        if(click){
            if(screen_==Screen::external&&external_level_browser_)return activate(0);
            if(screen_==Screen::external)return activate(external_rows_[i].adjustable&&x>=380&&x<414?-1:external_rows_[i].adjustable&&x>=548&&x<564?1:0);
            return activate(((screen_==Screen::graphics&&i<10)||(screen_==Screen::controls&&i==8))&&x>=380&&x<414?-1:1);
        }break;
    }
    return {};
}
NativeFrontendEvent NativeFrontend::key(unsigned vk){
    if(!active()||screen_==Screen::loading)return {};rows();
    if(vk==VK_ESCAPE||vk==VK_BACK||(screen_==Screen::external&&vk=='Q'))return back();
    if(vk==VK_UP||vk==VK_DOWN){select(vk==VK_UP?-1:1);return {};}
    if(screen_==Screen::external&&external_level_browser_&&(vk==VK_LEFT||vk==VK_RIGHT)){select(vk==VK_LEFT?-1:1);return {};}
    if(horizontal()&&(vk==VK_LEFT||vk==VK_RIGHT)){select(vk==VK_LEFT?-1:1);return {};}
    if(screen_==Screen::external&&external_level_browser_){
        if(vk=='D'&&(level_details_.actions&1))return {Action::external_difficulty};
        if(vk=='R'&&(level_details_.actions&2))return {Action::external_rewards};
    }
    if(vk==VK_RETURN||vk==VK_SPACE)return activate(screen_==Screen::external?0:1);
    if(screen_==Screen::external&&!horizontal()&&(vk==VK_LEFT||vk==VK_RIGHT)&&selected_<external_rows_.size()&&external_rows_[selected_].adjustable)return activate(vk==VK_LEFT?-1:1);
    if((vk==VK_LEFT||vk==VK_RIGHT)&&((screen_==Screen::graphics&&selected_<10)||(screen_==Screen::controls&&selected_==8)))return activate(vk==VK_LEFT?-1:1);
    if(screen_==Screen::controls&&(vk==VK_PRIOR||vk==VK_NEXT)){selected_=vk==VK_PRIOR?9:10;return activate();}
    if(screen_==Screen::credits&&(vk==VK_LEFT||vk==VK_RIGHT)){
        selected_=vk==VK_LEFT?0:1;return activate();
    }
    return {};
}
ID3D11ShaderResourceView *NativeFrontend::keycap(NativeRenderer &r,unsigned key){
    auto found=keycaps_.find(key);if(found!=keycaps_.end())return found->second.Get();
    std::vector<uint32_t> pixels(128*128);const auto label=NativeInputBindings::key_name(key);
    if(!native_hud_keycap(pixels.data(),key,label.c_str(),nullptr))throw std::runtime_error("Control key artwork");
    UCTextureAsset texture{};texture.format=UC_TEXTURE_BGRA8_UNORM;texture.width=texture.height=128;texture.mip_count=1;
    texture.storage=reinterpret_cast<uint8_t*>(pixels.data());texture.storage_bytes=pixels.size()*4;
    texture.mips[0]={128,128,128*4,128*128*4,texture.storage};
    auto uploaded=r.upload(texture);auto *result=uploaded.Get();keycaps_.emplace(key,std::move(uploaded));return result;
}

void NativeFrontend::image(NativeRenderer &r,ID3D11ShaderResourceView *texture,float x,float y,float width,float height,const float *tint,bool full,bool opaque){
    const float xy[4][2]={{x,y},{x+width,y},{x+width,y+height},{x,y+height}};
    float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
    if(full&&texture){
        ComPtr<ID3D11Resource> resource;texture->GetResource(&resource);ComPtr<ID3D11Texture2D> source;
        if(FAILED(resource.As(&source)))throw std::runtime_error("Frontend background requires a2D texture");
        D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);
        const float source_aspect=float(desc.Width)/desc.Height,destination_aspect=width/height;
        // Cover the viewport with a centered crop: original640x480 footage is
        // never stretched into16:9. Menu art retains the separate safe transform.
        if(destination_aspect>source_aspect){const float edge=(1-source_aspect/destination_aspect)*.5f;uv[0][1]=uv[1][1]=edge;uv[2][1]=uv[3][1]=1-edge;}
        else{const float edge=(1-destination_aspect/source_aspect)*.5f;uv[0][0]=uv[3][0]=edge;uv[1][0]=uv[2][0]=1-edge;}
    }
    NativeVertex vertices[4]{};for(unsigned i=0;i<4;i++){std::memcpy(vertices[i].position,xy[i],8);std::memcpy(vertices[i].uv,uv[i],8);}
    r.update(quad_,vertices,4);
    const float scale=full?1.f:std::min(r.width/640.f,r.height/480.f),ox=full||screen_!=Screen::loading&&screen_!=Screen::splash?0:(r.width-640*scale)*.5f,oy=full?0:(r.height-480*scale)*.5f;
    const float matrix[16]={2*scale/r.width,0,0,0,0,-2*scale/r.height,0,0,0,0,1,0,-1+2*ox/r.width,1-2*oy/r.height,0,1};
    NativeDrawState state;state.depth_disabled=true;state.blend_mode=opaque?0:1;state.write_alpha=false;state.clamp_u=state.clamp_v=true;state.vertex_exposure=texture!=nullptr;
    r.draw(quad_,matrix,tint,texture,0,0,state);
}
void NativeFrontend::text(const std::string &value,float x,float y,float scale,uint32_t color,float right,const UCFont *font){
    const std::string clean=printable(value);if(clean.empty())return;
    std::vector<UCFontQuad> quads(clean.size());size_t count=0;float height=0;
    if(uc_font_layout(font?font:&font_,clean.c_str(),x,y,scale,x,right,0,quads.data(),quads.size(),&count,&height)||count>glyph_capacity-text_vertices_.size()/4)
        throw std::runtime_error("Frontend text layout: "+clean);
    for(size_t i=0;i<count;i++){
        const auto &q=quads[i];const float xy[4][2]={{q.x0,q.y0},{q.x1,q.y0},{q.x1,q.y1},{q.x0,q.y1}},uv[4][2]={{q.u0,q.v0},{q.u1,q.v0},{q.u1,q.v1},{q.u0,q.v1}};
        for(unsigned j=0;j<4;j++){NativeVertex v;v.position[0]=xy[j][0]+.5f;v.position[1]=xy[j][1]+.5f;v.position[2]=0;std::memcpy(v.uv,uv[j],8);v.color=color;text_vertices_.push_back(v);}
    }
}
void NativeFrontend::draw(NativeRenderer &r,const NativeFrontendSettings &settings,ID3D11ShaderResourceView *background){
    if(!active())return;if(!loaded_)throw std::logic_error("Frontend assets not loaded");settings_=settings;
    if(screen_==Screen::loading){draw_loading(r,0);return;}rows();
    const bool paused=screen_==Screen::external&&external_over_game_||screen_==Screen::pause||((screen_==Screen::graphics||screen_==Screen::options)&&parent_==Screen::pause)||
        (screen_==Screen::controls&&(controls_parent_==Screen::pause||(controls_parent_==Screen::options&&parent_==Screen::pause)));
    const bool original_levels=screen_==Screen::external&&external_level_browser_&&background;
    const float width=canvas_width(),black[]={0,0,0,1},frame[]={.45f,.47f,.47f,1};
    if(original_levels){const float side=(level_details_.actions&4)?212.f:80.f;image(r,nullptr,0,0,width,480,black);image(r,background,side,0,width-side,480,white,false,true);}
    else if(!paused)image(r,background?background:backdrop_.Get(),0,0,float(r.width),float(r.height),white,true);
    if(!original_levels&&(paused||screen_!=Screen::main&&screen_!=Screen::singleplayer&&screen_!=Screen::splash))
        image(r,nullptr,0,0,float(r.width),float(r.height),shade,true);
    if(screen_==Screen::splash){
        // The authored gamelogo/splash movie already contains the logo.
        if(!background)image(r,logo_.Get(),192,76,256,256,white);
    }else{
        // CONTROL259020/259CD0: black title/footer, framed moving panel,
        // uppercase large font and horizontal footer choices. Widen the panel
        // for PC aspect ratios; keep text and art at their authored proportions.
        image(r,nullptr,0,0,width,70,black);
        image(r,nullptr,0,382,width,98,black);
        image(r,nullptr,0,70,width,3,frame);image(r,nullptr,0,379,width,3,frame);
        image(r,nullptr,0,73,3,306,frame);image(r,nullptr,width-3,73,3,306,frame);
    }
    text_vertices_.clear();
    std::string title;
    switch(screen_){
    case Screen::external:title=external_title_;break;
    case Screen::main:title=localized("NewFrontEnd-MainMenu-1","Main menu");break;
    case Screen::profiles:title="Single player: Saved games";break;
    case Screen::singleplayer:title=localized("NewFrontend-Singleplayer-1","Single player")+": "+localized("NewFrontEnd-SelectLevel-1","Select Level");break;
    case Screen::overwrite:title="Start a new mission?";break;
    case Screen::graphics:title="Graphics";break;
    case Screen::controls:title="Controls";break;
    case Screen::options:title=localized("NewFrontEnd-Options-1","Options");break;
    case Screen::credits:title=localized("NewFrontEnd-Credits-1","Credits");break;
    case Screen::pause:title="Paused";break;
    default:break;
    }
    title=uppercase(title);
    const float title_scale=std::min(.8f,(width-100)/std::max(1.f,measure(title,1,&title_font_)));
    const auto colon=title.find(':');
    if(colon==std::string::npos)text(title,50,36,title_scale,0xffffffff,width-50,&title_font_);
    else{
        const auto prefix=title.substr(0,colon+1);
        text(prefix,50,36,title_scale,0xffffffff,width-50,&title_font_);
        text(title.substr(colon+2),50+measure(prefix,title_scale,&title_font_)+10,36,title_scale,0xff68776a,width-50,&title_font_);
    }
    flush_text(r,title_texture_.Get());
    if(screen_==Screen::main)text("Slot "+std::to_string(save_slot_),width-112,49,.7f,0xffdddddd,width-30);
    if(screen_==Screen::profiles)text("Choose a save slot.",64,116,.95f,0xffdddddd);
    if(screen_==Screen::singleplayer){
        const float right=width-54,preview=240,description_right=right-preview-25;
        image(r,nullptr,right-preview-2,93,preview+4,244,frame);
        image(r,thumbnail_.Get(),right-preview,95,preview,240,white);
        text(uppercase(localized("LevelName-101-3","Westside Canals")),50,98,1,0xffffffff,description_right);
        text(localized("NewFrontEnd-LevelDesc-2","Gang warfare has exploded in this industrial district. Emergency teams are trapped after being lured to the docks."),50,123,1,0xfffedaab,description_right);
        text("Slot "+std::to_string(save_slot_)+": "+save_summary_,50,345,.8f,0xffdddddd,width-50);
    }
    if(screen_==Screen::overwrite){
        text("Start Westside Canals from the beginning?",64,145,1.1f,0xffffffff);
        text("Your existing checkpoint in slot "+std::to_string(save_slot_)+" will be replaced when the new mission saves.",64,205,.95f,0xffdddddd);
    }
    if(screen_==Screen::options){
        text("Mouse and keyboard",64,112,1.1f,0xffffffff);
        text("Open Controls to change keys, mouse buttons and sensitivity.",64,154,.9f,0xffdddddd);
        text("F5: FPS counter    F11: Fullscreen    Escape: Pause / back",64,218,.85f,0xffdddddd);
        text("Graphics settings are available from the main and pause menus.",64,282,.85f,0xffdddddd,width-64);
    }
    if(screen_==Screen::credits){
        const unsigned begin=credits_page_*5,end=std::min(begin+5,unsigned(credits_.size()));
        for(unsigned i=begin;i<end;i++)text(credits_[i],64,116+(i-begin)*46.f,.95f,0xffeeeeee);
        text(std::to_string(credits_page_+1)+" / "+std::to_string(std::max(1u,(unsigned(credits_.size())+4)/5)),465,337,.8f,0xffbbbbbb);
    }
    if(screen_==Screen::controls){
        const std::string description=!status_.empty()?status_:selected_<8?NativeInputBindings::action_description(static_cast<NativeInputBindings::Action>(controls_page_*8+selected_)):
            selected_==8?"Adjust how quickly the view turns with mouse movement.":"Click a binding to change it. Press Escape to cancel.";
        text(description,64,325,.72f,0xffdddddd);
        text(std::to_string(controls_page_+1)+" / "+std::to_string(control_pages),432,73,.7f,0xffbbbbbb);
    }
    if(screen_==Screen::external&&external_level_browser_&&!(level_details_.actions&4)){
        if(!original_levels)draw_level_details(r,width);
        image(r,nullptr,78,73,2,306,frame);
        const std::string difficulty=localized(("NewFrontEnd-EMH-"+std::to_string(level_details_.difficulty+2)).c_str(),"Difficulty");
        const std::string labels[]={"Back","Difficulty: "+difficulty+" [D]","Rewards [R]","Deploy"};
        text("Medals:  "+std::to_string(level_details_.medals)+" / 204",width-178,50,.7f,0xffdddddd,width-30);
        for(unsigned i=0;i<4;i++){const auto b=level_button(i);const bool enabled=i==0||i==3||(level_details_.actions&(1u<<(i-1)));
            image(r,bar_.Get(),b.x,b.y,b.width,b.height,selected);
            text(labels[i],b.x+12,b.y+7,std::min(.85f,(b.width-24)/std::max(1.f,measure(labels[i]))),enabled?0xffffffff:0xff606060,b.x+b.width-8);
        }
    }
    for(unsigned i=0;i<rows_.size();i++){
        const auto rect=bounds(i);const float y=rect.y;const auto &row=rows_[i];const bool chosen=i==selected_&&row.enabled;
        if(chosen&&!horizontal())image(r,bar_.Get(),rect.x,y,rect.width,rect.height,selected);
        float scale=horizontal()?menu_scale():screen_==Screen::external?std::min(.85f,rect.height/27.f):screen_==Screen::graphics||screen_==Screen::controls?.85f:1.f;
        float text_x=rect.x+(horizontal()?9:12),text_right=row.value.empty()?rect.x+rect.width-3:378.f;
        if(screen_==Screen::external&&external_level_browser_&&!(level_details_.actions&4)){
            const auto &source=external_rows_[i];const float size=std::min(24.f,rect.height-1);
            if(source.artwork_id<12)image(r,mission_small_[source.completed?1:0][source.artwork_id].Get(),rect.x+3,y,size,size,white);
            if(source.completed&&source.completed<=4)image(r,mission_complete_[source.completed>2?source.completed-2:0].Get(),rect.x+rect.width-size-3,y,size,size,white);
            text_x=rect.x+27;text_right=rect.x+rect.width-24;
            scale=std::min(scale,(text_right-text_x)/std::max(1.f,measure(row.label)));
        }
        if(!(screen_==Screen::external&&external_level_browser_&&!(level_details_.actions&4)))text(row.label,text_x,y+3,scale,chosen?0xffffffff:row.enabled?0xff787878:0xff505050,text_right);
        if(horizontal()&&i+1<rows_.size())text("|",rect.x+rect.width-1,y+3,scale,0xff787878,width);
        if(!row.value.empty()){
            if(screen_==Screen::external&&external_rows_[i].keycap){const float size=std::min(22.f,rect.height-2);image(r,keycap(r,external_rows_[i].keycap),386,y+1,size,size,white);}
            else if(screen_==Screen::controls&&i<8&&bindings_)image(r,keycap(r,bindings_->key(static_cast<NativeInputBindings::Action>(controls_page_*8+i))),386,y+1,22,22,white);
            else{image(r,left_.Get(),386,y+4,16,16,white);image(r,right_.Get(),548,y+4,16,16,white);}
            text(row.value,414,y+3,screen_==Screen::external?scale:.85f,0xffffffff,546);
        }
    }
    if(screen_==Screen::external&&status_.empty()&&selected_<external_rows_.size())
        text(external_rows_[selected_].help,50,horizontal()?417.f:387.f,.72f,0xffd6e9ff,width-50);
    if(!status_.empty()&&screen_!=Screen::controls)text(status_,50,horizontal()?417.f:387.f,.72f,0xffd6e9ff,width-50);
    text(screen_==Screen::external?external_footer_:screen_==Screen::graphics?"Click arrows or use Left / Right to change.  Esc: back":screen_==Screen::controls?"Click or Enter: change binding    PgUp / PgDn: page    Esc: back":"Mouse / arrows: navigate    Enter: select    Esc: back",std::max(50.f,width-450),447,.75f,0xffbbbbbb,width-30);
    flush_text(r);
}
void NativeFrontend::flush_text(NativeRenderer &r,ID3D11ShaderResourceView *texture){
    if(!text_vertices_.empty()){
        if(text_mesh_.vertex_count!=text_vertices_.size()){
            const uint32_t order[]={0,1,2,0,2,3};std::vector<uint32_t> indices(text_vertices_.size()/4*6);
            for(unsigned i=0;i<text_vertices_.size()/4;i++)for(unsigned j=0;j<6;j++)indices[i*6+j]=i*4+order[j];
            text_mesh_=r.upload(text_vertices_.data(),text_vertices_.size(),indices.data(),indices.size(),true);
        }else r.update(text_mesh_,text_vertices_.data(),text_vertices_.size());
        const float scale=std::min(r.width/640.f,r.height/480.f),ox=screen_==Screen::loading||screen_==Screen::splash?(r.width-640*scale)*.5f:0,oy=(r.height-480*scale)*.5f;
        const float matrix[16]={2*scale/r.width,0,0,0,0,-2*scale/r.height,0,0,0,0,1,0,-1+2*ox/r.width,1-2*oy/r.height,0,1};
        NativeDrawState state;state.depth_disabled=true;state.blend_mode=1;state.write_alpha=false;state.vertex_exposure=true;
        r.draw(text_mesh_,matrix,white,texture?texture:font_texture_.Get(),0,uint32_t(text_vertices_.size()/4*6),state);
        text_vertices_.clear();
    }
}

void NativeFrontend::begin_loading(const std::filesystem::path &game,NativeRenderer &r){
    if(!loaded_)throw std::logic_error("Frontend assets not loaded");
    if(!loading_art_){
        uint8_t hash[32];auto loading=read_archive(game/"loadingscreen13.lst",hash);
        auto art=read_archive(game/"PersistantFilesXBox.lst",hash);
        auto fonts=read_archive(game/"dialoguefonts.lst",hash);
        auto read=[](const UCArchive *a,const char *name,BytesOwner &out){
            const size_t i=find_resource(a,name);
            if(i==SIZE_MAX||uc_archive_read(a,i,UC_AR_LOCAL_PAYLOAD,&out.value))throw std::runtime_error(std::string("Loading resource: ")+name);
        };
        auto texture=[&](const UCArchive *a,const char *name){
            BytesOwner bytes;read(a,name,bytes);
            struct Texture {UCTextureAsset value{};~Texture(){uc_texture_free(&value);}} decoded;
            if(uc_texture_decode(bytes.value.data,bytes.value.size,&decoded.value))throw std::runtime_error(std::string("Loading texture: ")+name);
            return r.upload(decoded.value);
        };
        // Original59860, first profile tip counter0: table4E75E0 row0=(1,13,1).
        // Keep this fresh-profile presentation until native profile history exists.
        for(unsigned i=0;i<2;i++){
            read(fonts.get(),i?"fonts\\large.abc":"fonts\\medium.abc",loading_metrics_[i]);
            if(uc_font_read(loading_metrics_[i].value.data,loading_metrics_[i].value.size,&loading_fonts_[i]))throw std::runtime_error("Loading font metrics");
            loading_font_textures_[i]=texture(art.get(),i?"fonts\\large%4b%.tga":"fonts\\medium%4b%.tga");
        }
        for(unsigned i=0;i<4;i++){
            const auto name="frontend\\frt_loadingbar\\frt_loading_flag_000"+std::to_string(i)+".tga";
            loading_flags_[i]=texture(art.get(),name.c_str());
        }
        loading_mask_=texture(art.get(),"frontend\\frt_content\\mask_solid.tga");
        loading_fade_=texture(art.get(),"frontend\\frt_content\\mask_fade_left.tga");
        loading_title_=localized("LevelName-101-3","Westside Canals");
        for(char &c:loading_title_)if(c>='a'&&c<='z')c=char(c-'a'+'A');
        loading_tip_=localized("Loading-Info-1","");loading_label_=localized("Loading-Bar-1","");
        if(loading_tip_.empty()||loading_label_.empty())throw std::runtime_error("Original loading text missing");
        std::vector<UCFontQuad> quads(loading_tip_.size());size_t count=0;float height=0;
        if(uc_font_layout(&loading_fonts_[0],loading_tip_.c_str(),50,0,1,50,430,0,quads.data(),quads.size(),&count,&height))throw std::runtime_error("Loading tip layout");
        //5A10D/5A190: medium font, x50..430, last line ending at368.
        loading_tip_y_=368-height-loading_fonts_[0].line_height;
        quads.resize(loading_label_.size());
        if(uc_font_layout(&font_,loading_label_.c_str(),0,0,1,0,640,0,quads.data(),quads.size(),&count,&height)||!count)throw std::runtime_error("Loading label layout");
        loading_label_x_=557-(quads[count-1].x1-quads[0].x0)*.5f;
        //65910 maps the packed512-square art to its authored640x480 canvas.
        loading_art_=texture(loading.get(),"frontend\\frt_backdrops\\frt_loading13%32b%.tga");
    }
    show(Screen::loading);settings_.width=r.width;settings_.height=r.height;
}
void NativeFrontend::draw_loading(NativeRenderer &r,float elapsed_seconds){
    if(screen_!=Screen::loading||!loading_art_)throw std::logic_error("Loading presentation not prepared");
    const float black[]={0,0,0,1};image(r,nullptr,0,0,float(r.width),float(r.height),black,true);
    image(r,loading_art_.Get(),0,0,640,480,white);
    //5A305 solid/fade backing, original textures; reverse the fade's UV direction.
    image(r,loading_mask_.Get(),0,loading_tip_y_,330,368-loading_tip_y_,white);
    image(r,loading_fade_.Get(),410,loading_tip_y_,-80,368-loading_tip_y_,white);
    text_vertices_.clear();
    text(loading_title_,50,69-loading_fonts_[1].height,1,0xffffffff,590,&loading_fonts_[1]);
    flush_text(r,loading_font_textures_[1].Get());
    text(loading_tip_,50,loading_tip_y_,1,0xffffffff,430,&loading_fonts_[0]);
    flush_text(r,loading_font_textures_[0].Get());
    //64BA0/64BF0: x525,y400,64x16; four authored frames every0.2 seconds.
    const double seconds=std::isfinite(elapsed_seconds)?std::max(0.f,elapsed_seconds):0;
    const auto frame=unsigned(std::fmod(std::floor(seconds*5),4));
    image(r,loading_flags_[frame].Get(),525,400,64,16,white);
    text(loading_label_,loading_label_x_,412,1,0xffffffff,590);flush_text(r);
}

bool NativeFrontend::self_check(){
    NativeFrontend f;f.show(Screen::splash);f.key(VK_RETURN);if(f.screen()!=Screen::main)return false;
    f.key(VK_RETURN);if(f.screen()!=Screen::profiles)return false;
    auto slot=f.key(VK_RETURN);if(slot.action!=Action::select_save_slot||slot.value!=1||f.screen()!=Screen::singleplayer)return false;
    if(f.selected_!=1)return false;
    f.key(VK_DOWN);if(f.selected_!=3)return false;f.key(VK_UP);if(f.key(VK_RETURN).action!=Action::start_mission)return false;
    f.set_save_slot(2,true,"Westside Canals / checkpoint1");f.show(Screen::singleplayer);
    if(f.key(VK_RETURN).action!=Action::resume_mission||f.save_slot()!=2)return false;
    f.key(VK_DOWN);f.key(VK_RETURN);if(f.screen()!=Screen::overwrite||f.selected_!=1)return false;
    f.key(VK_RETURN);if(f.screen()!=Screen::singleplayer)return false;
    f.key(VK_DOWN);f.key(VK_RETURN);f.key(VK_UP);if(f.key(VK_RETURN).action!=Action::start_mission)return false;
    f.set_save_slot(3,false,"Unreadable",true);f.show(Screen::singleplayer);f.key(VK_RETURN);
    if(f.screen()!=Screen::overwrite)return false;
    f.show(Screen::main);for(unsigned i=0;i<3;i++)f.key(VK_RIGHT);f.key(VK_RETURN);if(f.screen()!=Screen::graphics)return false;
    if(f.key(VK_LEFT).value!=1440)return false;f.key(VK_DOWN);if(f.key(VK_RETURN).action!=Action::fullscreen)return false;
    f.show(Screen::pause);f.key(VK_DOWN);f.key(VK_RETURN);f.key(VK_ESCAPE);if(f.screen()!=Screen::pause||f.key(VK_ESCAPE).action!=Action::resume)return false;
    f.set_save_slot(1,false,"Empty");f.show(Screen::singleplayer);if(f.mouse(10,200,1920,1080,true).action!=Action::none)return false;
    // Widescreen menu fills the viewport; input uses the same transform.
    const auto start=f.bounds(1);const float click_x=start.x+start.width*.5f,click_y=start.y+12;
    if(f.mouse(click_x*2.25f,click_y*2.25f,1920,1080,true).action!=Action::start_mission)return false;
    if(f.mouse(click_x/f.canvas_width()*1000,click_y/480*1000,1000,1000,true).action!=Action::start_mission)return false;
    f.show(Screen::graphics);f.selected_=2;if(f.key(VK_RIGHT).value!=2)return false;f.settings_.msaa=8;if(f.key(VK_RIGHT).value!=1)return false;
    NativeInputBindings bindings;f.set_bindings(bindings);f.show(Screen::controls);
    if(f.key(VK_RETURN).action!=Action::rebind||f.key(VK_RETURN).value!=0)return false;
    f.key(VK_NEXT);if(f.key(VK_RETURN).value!=8)return false;
    f.key(VK_NEXT);auto flashlight_binding=f.key(VK_RETURN);
    if(f.controls_page_!=2||flashlight_binding.action!=Action::rebind||flashlight_binding.value!=16)return false;
    f.key(VK_NEXT);if(f.controls_page_!=2)return false;
    for(unsigned i=NativeInputBindings::action_count-16;i<8;i++){f.selected_=i;if(f.activate().action!=Action::none)return false;}
    f.selected_=0;const auto flashlight_row=f.bounds(0);
    if(f.mouse((flashlight_row.x+15)*2.25f,(flashlight_row.y+12)*2.25f,1920,1080,true).value!=16)return false;
    f.key(VK_PRIOR);if(f.controls_page_!=1)return false;
    f.selected_=8;if(f.key(VK_RIGHT).value!=125)return false;
    f.show(Screen::pause);f.selected_=3;f.key(VK_RETURN);f.key(VK_RETURN);f.key(VK_ESCAPE);f.key(VK_ESCAPE);
    if(f.screen()!=Screen::pause)return false;
    f.selected_=4;if(f.key(VK_RETURN).action!=Action::save_checkpoint)return false;
    if(f.detail_text({0x80,0,0,0,0,"Completed: \x81 / 12","7"})!="Completed: 7 / 12")return false;
    if(f.detail_text({0x80,0,0,0,0,"Literal name","Unused suffix"})!="Literal name")return false;
    std::vector<NativeFrontendRow> levels;
    for(unsigned i=0;i<17;i++)levels.push_back({i,0,true,false,"Mission "+std::to_string(i+1),{}, {}});
    f.set_external("Select assignment",levels,0,false,false,{}, {},true);
    f.level_details_.actions=3;
    if(f.key('D').action!=Action::external_difficulty||f.key('R').action!=Action::external_rewards)return false;
    f.level_details_.actions=0;if(f.key('D').action!=Action::none||f.key('R').action!=Action::none)return false;
    const auto mission=f.bounds(4);
    if(f.mouse((mission.x+20)*2.25f,(mission.y+mission.height*.5f)*2.25f,1920,1080,true).action!=Action::external_activate||f.selected_index()!=4)return false;
    if(f.key(VK_RETURN).action!=Action::external_activate||f.key(VK_RETURN).value!=4)return false;
    if(f.key(VK_DOWN).action!=Action::none||f.selected_index()!=5)return false;
    if(f.mouse((f.canvas_width()-80)*2.25f,410*2.25f,1920,1080,true).action!=Action::external_activate)return false;
    if(f.mouse(60*2.25f,410*2.25f,1920,1080,true).action!=Action::external_back)return false;
    f.show(Screen::loading);
    for(unsigned key:{VK_ESCAPE,VK_BACK,VK_RETURN,VK_SPACE,VK_UP,VK_DOWN})if(f.key(key).action!=Action::none||f.screen()!=Screen::loading)return false;
    if(f.mouse(640,360,1280,720,true).action!=Action::none||f.screen()!=Screen::loading)return false;
    return f.mouse(NAN,0,1280,720,true).action==Action::none;
}
