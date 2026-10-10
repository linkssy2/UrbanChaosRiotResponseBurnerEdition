#pragma once
#include "assets.h"
#include "character_dialogue.h"
#include <array>
extern "C" {
#include "../font.h"
}

enum class NativeFrontendAction {
    none,start_mission,resume,main_menu,quit,resolution,fullscreen,msaa,
    anisotropy,vsync,frame_limit,fps_counter,lighting,shadows,color,
    rebind,controls_reset,mouse_sensitivity,select_save_slot,resume_mission,save_checkpoint,
    external_activate,external_back,external_difficulty,external_rewards
};
struct NativeFrontendEvent {NativeFrontendAction action=NativeFrontendAction::none;unsigned value=0;int direction=1;};
struct NativeFrontendRow {
    unsigned id=0,keycap=0;bool enabled=true,adjustable=false;
    std::string label,value,help;
    unsigned artwork_id=0,completed=0;
};
struct NativeFrontendDetailText {
    unsigned flags=0,color=0xffffffff,font=0;float x=0,y=0;
    std::string prefix,suffix;
};
struct NativeFrontendDetailIcon {unsigned kind=0,color=0xffffffff;float x=0,y=0;};
struct NativeFrontendLevelDetails {
    std::vector<NativeFrontendDetailText> texts;
    std::vector<NativeFrontendDetailIcon> icons;
    unsigned actions=0,difficulty=0,medals=0;
};
struct NativeFrontendSettings {
    unsigned width=1280,height=720,msaa=1,anisotropy=1,frame_limit=60;
    bool fullscreen=false,vsync=true,fps_counter=true,lighting=true,shadows=true,color=true;
};

// PC interaction over the original frontend art/font/text. This does not own
// save profiles, mission state or graphics settings; actions request the owner.
class NativeInputBindings;
class NativeFrontend {
public:
    enum class Screen {closed,splash,main,profiles,singleplayer,overwrite,graphics,options,credits,pause,controls,loading,external};
    void load(const std::filesystem::path &game,NativeRenderer &);
    void show(Screen);
    Screen screen()const{return screen_;}
    bool active()const{return screen_!=Screen::closed;}
    void set_settings(const NativeFrontendSettings &s){settings_=s;}
    void set_bindings(const NativeInputBindings &bindings){bindings_=&bindings;}
    void set_status(std::string message){status_=std::move(message);}
    // A borrowed gameplay owner supplies presentation rows only. No native
    // mission/save/settings actions are synthesized by this mode.
    void set_external(std::string title,std::vector<NativeFrontendRow> rows,unsigned selected,
        bool horizontal,bool over_game,std::string status,std::string footer,bool level_browser=false,NativeFrontendLevelDetails details={});
    unsigned selected_index()const{return selected_;}
    unsigned selected_id()const{return selected_<external_rows_.size()?external_rows_[selected_].id:0;}
    void set_save_slot(unsigned slot,bool available,std::string summary,bool occupied=false);
    void set_save_summaries(const std::array<std::string,3> &summaries){save_summaries_=summaries;}
    unsigned save_slot()const{return save_slot_;}
    void draw(NativeRenderer &,const NativeFrontendSettings &,ID3D11ShaderResourceView *background=nullptr);
    void begin_loading(const std::filesystem::path &game,NativeRenderer &);
    void draw_loading(NativeRenderer &,float elapsed_seconds);
    NativeFrontendEvent mouse(float client_x,float client_y,unsigned client_width,unsigned client_height,bool click);
    NativeFrontendEvent key(unsigned virtual_key);
    // Runnable, window-free policy check, retained for later authorized tests.
    static bool self_check();
private:
    struct Row {std::string label,value;NativeFrontendAction action;Screen destination;bool enabled;};
    struct Bounds {float x,y,width,height;};
    Screen screen_=Screen::closed,parent_=Screen::main;
    Screen controls_parent_=Screen::main;
    const NativeInputBindings *bindings_=nullptr;
    NativeFrontendSettings settings_{};
    std::vector<Row> rows_;
    std::vector<NativeFrontendRow> external_rows_;
    std::string external_title_,external_footer_;
    NativeFrontendLevelDetails level_details_;
    std::string detail_text(const NativeFrontendDetailText &)const;
    void draw_level_details(NativeRenderer &,float width);
    bool external_horizontal_=false,external_over_game_=false,external_level_browser_=false;
    std::string status_;
    std::array<std::string,3> save_summaries_{{"Empty","Empty","Empty"}};
    std::string save_summary_="Empty";
    unsigned save_slot_=1;
    bool save_available_=false,save_occupied_=false;
    unsigned selected_=0,credits_page_=0,controls_page_=0;
    float pointer_x_=0,pointer_y_=0;
    bool pointer_valid_=false,loaded_=false;
    NativeDialogueText text_;
    std::vector<std::string> credits_;
    BytesOwner metrics_,title_metrics_;UCFont font_{},title_font_{};
    BytesOwner loading_metrics_[2];UCFont loading_fonts_[2]{};
    ComPtr<ID3D11ShaderResourceView> loading_font_textures_[2],loading_flags_[4],loading_art_,loading_mask_,loading_fade_;
    std::string loading_title_,loading_tip_,loading_label_;
    float loading_tip_y_=0,loading_label_x_=0;
    ComPtr<ID3D11ShaderResourceView> font_texture_,title_texture_,bar_,left_,right_,logo_,backdrop_,thumbnail_;
    ComPtr<ID3D11ShaderResourceView> medals_[5],mission_art_[12],mission_small_[2][12],mission_complete_[3];
    std::map<unsigned,ComPtr<ID3D11ShaderResourceView>> keycaps_;
    NativeGpuMesh quad_,text_mesh_;
    std::vector<NativeVertex> text_vertices_;
    std::string localized(const char *,const char *)const;
    void rows();
    void select(int);
    NativeFrontendEvent activate(int direction=1);
    NativeFrontendEvent back();
    Bounds bounds(unsigned)const;
    Bounds level_button(unsigned)const;
    float canvas_width()const;
    float measure(const std::string &,float scale=1,const UCFont *font=nullptr)const;
    bool horizontal()const;
    float menu_scale()const;
    ID3D11ShaderResourceView *keycap(NativeRenderer &,unsigned);
    void image(NativeRenderer &,ID3D11ShaderResourceView *,float,float,float,float,const float *,bool full=false,bool opaque=false);
    void text(const std::string &,float,float,float,uint32_t,float right=576,const UCFont *font=nullptr);
    void flush_text(NativeRenderer &,ID3D11ShaderResourceView *texture=nullptr);
};
