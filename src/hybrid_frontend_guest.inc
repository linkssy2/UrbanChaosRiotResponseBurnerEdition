/* Included by menu_pointer_guest.h. Original menu updates remain authoritative;
 * the bridge never calls constructors, writes saves or invokes a callback. */
#include "hybrid_frontend_guest.h"
static UcHybridFrontendSnapshot hf_view,hf_pending;
static uint64_t hf_epoch;
static uint32_t hf_back_owner,hf_back_kind,hf_eligible_owner,hf_level_action;
/* Pause children (including modal confirmations) do not all carry 0x10.
 * Keep the presentation scope with the original manager, not the child. */
static uint32_t hf_pause_manager;
/* Confirmed original Quit callback267400 tears down this manager before main. */
static int hf_quit_handoff;
static int hf_text(uint32_t address,char text[128]){
    for(unsigned i=0;i<128;i++){
        if(!menu_pointer_readable(address+i,1)){text[0]=0;return 0;}
        text[i]=(char)MEM8(address+i);if(!text[i])return 1;
    }
    text[0]=0;return 0;
}
static int hf_inline(uint32_t address,char *text,unsigned capacity){
    if(!menu_pointer_readable(address,capacity))return 0;
    for(unsigned i=0;i<capacity;i++){text[i]=(char)MEM8(address+i);if(!text[i])return 1;}
    return 0;
}
static int hf_settled(void){
    return menu_pointer_readable(menu_manager,0x15)&&MEM32(menu_manager+4)==menu_screen&&
        menu_pointer_readable(menu_screen,4)&&!MEM8(menu_manager+0x14);
}
static void hf_reset(void){
    hf_back_owner=hf_back_kind=hf_eligible_owner=hf_level_action=0;memset(&hf_pending,0,sizeof(hf_pending));
    if(!ucrr_native_frontend_enabled())return;
    int transition=hf_quit_handoff||((hf_view.kind||hf_view.source_kind&UC_HF_TRANSITION)&&
        menu_pointer_readable(menu_manager,0x15)&&
        (MEM8(menu_manager+0x14)||(MEM32(menu_manager+4)!=menu_screen&&
         menu_pointer_readable(MEM32(menu_manager+4),4))));
    memset(&hf_view,0,sizeof(hf_view));hf_view.epoch=++hf_epoch;
    if(hf_quit_handoff||(hf_pause_manager&&menu_pointer_readable(hf_pause_manager,8)&&MEM32(hf_pause_manager+4)))hf_view.source_kind=0x80000000u;
    else hf_pause_manager=0;
    if(transition)hf_view.source_kind|=UC_HF_TRANSITION;
    ucrr_native_frontend_publish(&hf_view);
}
static void hf_begin(void){memset(&hf_pending,0,sizeof(hf_pending));hf_back_owner=hf_back_kind=hf_eligible_owner=hf_level_action=0;}
/* Original 263C20 / 264570 / 264F10 / 263680 row layouts. Values are
 * copied for display only; adjustments still enter the original row query. */
static int hf_read_row(UcHybridFrontendRow *r){
    uint32_t p=r->row;if(!menu_pointer_readable(p,0x20))return 0;
    r->table=MEM32(p);r->enabled=!!MEM8(p+4);r->callback=MEM32(p+0xC);
    if(r->table==0x487668){
        if(!menu_pointer_readable(p,0xDC)||!hf_inline(p+8,r->label,64))return 0;
        uint32_t count=MEM32(p+0xD0),index=MEM32(p+0xD4);
        if(!count||count>16||index>=count||!hf_text(MEM32(p+0x54+8*index),r->value))return 0;
        r->callback=MEM32(p+0x48);
    }else{
        if(!hf_text(MEM32(p+8),r->label))return 0;
        if(r->table==0x487608){
            if(r->callback&&r->callback!=0xffffffffu)r->actions=UC_HF_ROW_ACTIVATE;
            return 1;
        }
        if(r->table==0x487684){
            if(!menu_pointer_readable(r->callback,1)||!hf_text(MEM32(p+(MEM8(r->callback)?0x14:0x18)),r->value))return 0;
        }else if(r->table==0x487730){
            if(!menu_pointer_readable(r->callback,4))return 0;
            snprintf(r->value,sizeof(r->value),"%d",(int32_t)MEM32(r->callback));
        }else if(r->table==0x48774C){
            if(!menu_pointer_readable(r->callback,1))return 0;
            snprintf(r->value,sizeof(r->value),"%u",MEM8(r->callback));
        }else return 0;
    }
    r->actions=UC_HF_ROW_LEFT|UC_HF_ROW_RIGHT;return 1;
}
static void hf_capture_list(uint32_t owner){
    if(menu_special_active()||!ucrr_native_frontend_enabled()||hf_eligible_owner!=owner||!hf_settled()||!menu_pointer_readable(owner,0xE8))return;
    uint32_t count=MEM32(owner+0x58),selection=MEM32(owner+4);
    if(!count||count>20||!menu_pointer_readable(selection,4))return;
    UcHybridFrontendSnapshot s={0};s.owner=owner;s.kind=UC_HF_LIST;s.count=count;s.selected=MEM32(selection);
    if(s.selected>=count)return;
    if(selection==0x51D29C){strcpy(s.title,"NewFrontEnd-MainMenu-1");hf_pause_manager=0;hf_quit_handoff=0;}
    if(selection==0x51CFA4){strcpy(s.title,"NewFrontEnd-Paused-1");hf_pause_manager=menu_manager;}
    if(selection==0x51D300)hf_text(0x4855A8,s.title);
    if(selection==0x51D304)hf_text(0x489AAC,s.title);
    if(selection==0x51D308)hf_text(0x489B38,s.title);
    if(selection==0x51D30C)hf_text(0x489BA0,s.title);
    if(selection==0x51D310)hf_text(0x485910,s.title);
    if(MEM32(owner+0xAC)&&MEM32(owner+0xAC)!=0xffffffffu)s.back_action=MEM32(0x485A18);
    for(unsigned i=0;i<count;i++){
        UcHybridFrontendRow*r=&s.rows[i];r->row=MEM32(owner+8+4*i);r->index=i;
        if(!hf_read_row(r))return;
        for(unsigned k=0;k<menu_target_count;k++){
            const UCRR_MENU_TARGET*t=&menu_targets[k];
            if(t->owner==owner&&t->row==r->row&&t->table==r->table)
                r->actions|=t->action==0x20?UC_HF_ROW_LEFT:t->action==0x21?UC_HF_ROW_RIGHT:0;
        }
    }
    hf_pending=s;
}
static void hf_capture_levels(uint32_t owner){
    memset(&hf_pending,0,sizeof(hf_pending));
    if(menu_special_active()||!ucrr_native_frontend_enabled()||hf_eligible_owner!=owner||!hf_settled()||
       !menu_pointer_readable(owner,0x118)||MEM32(owner)!=0x4885FC)return;
    UcHybridFrontendSnapshot s={0};s.kind=UC_HF_LEVELS;s.owner=owner;
    s.count=MEM32(owner+0xEC);s.selected=MEM32(owner+0x1C);
    if(!s.count||s.count>17||s.selected>=s.count)return;
    if(MEM32(owner+0x14)&&MEM32(owner+0x14)!=0xffffffffu)s.back_action=MEM32(0x485A18);
    s.level_actions=1; /*2707AF builds the original difficulty modal. */
    if(MEM32(owner+0xC)&&MEM32(owner+0xC)!=0xffffffffu)s.level_actions|=2;
    /*643E0 returns this profile;63C20/63D40 define its completion and medal
     *records. Availability comes only from26EF70's filtered triplets. */
    const uint32_t profile=0x5152B0;
    if(!menu_pointer_readable(profile,0x110))return;
    s.level_difficulty=MEM32(profile+0x48);if(s.level_difficulty>3)return;
    for(unsigned i=0;i<17;i++)for(unsigned j=0;j<4;j++)s.level_medals+=MEM8(profile+0x8C+i*8+j);
    /* The original26EAB0 names, selected from the original26EF70 row list.
     * Never add locked missions or reconstruct profile availability here. */
    static const uint32_t names[]={0x488328,0x488318,0x488308,0x4882F8,0x4882E8,0x4882D8,
        0x4882C8,0x4882B8,0x4882A8,0x488294,0x488280};
    static const uint32_t bonus[]={0x48826C,0x488258,0x488244,0x488230,0x48821C,0x488208};
    for(unsigned i=0;i<s.count;i++){
        UcHybridFrontendRow*r=&s.rows[i];r->index=i;r->row=MEM32(owner+0x20+12*i);
        r->table=MEM32(owner+0x24+12*i);r->enabled=!!MEM8(owner+0x28+12*i);r->actions=UC_HF_ROW_ACTIVATE;
        if(!r->enabled)return; /*unknown/locked rows must never become deployable. */
        uint32_t name,record;
        if(r->row<11){name=names[r->row];r->artwork_id=r->row;record=profile+0x88+r->row*8;}
        else if(r->row==0xffffffffu&&r->table<6){name=bonus[r->table];r->artwork_id=11;record=profile+0xE0+r->table*8;}
        else return;
        uint32_t completed=MEM32(record);r->completed=completed<4?completed+1:0;
        if(!hf_text(name,r->label))return;
    }
    /*26F6A0 owns these widgets.274300 marks the old selection for fade-out;
     * copying only the new list entries prevents old progress leaking through.
     *32 active widgets cover the source title/description/values+4 medal rows.
     *The larger traversal bound permits old fading entries without allocation. */
    uint32_t head=MEM32(owner+0xF8);if(!menu_pointer_readable(head,12))return;
    uint32_t node=MEM32(head),last=head;unsigned visited=0,medal_row=0,slots=0,medal_kinds[32]={0};
    while(node!=head){
        if(++visited>256||!menu_pointer_readable(node,12)||MEM32(node+4)!=last)return;
        uint32_t widget=MEM32(node+8);if(!menu_pointer_readable(widget,0x3C))return;
        if(!MEM8(widget+0x1C)){
            float x=MEMF(widget+4),y=MEMF(widget+0x24);
            if(!isfinite(x)||!isfinite(y)||fabsf(x)>4096||fabsf(y)>4096)return;
            if(MEM32(widget)==0x48905C){
                if(s.text_count==32||!menu_pointer_readable(widget,0xB4))return;
                UcHybridFrontendDetailText*t=&s.texts[s.text_count++];
                t->flags=MEM32(widget+0x64);t->color=MEM32(widget+0x60);t->font=MEM32(widget+0x5C);t->x=x;t->y=y;
                if(!hf_inline(widget+0x3C,t->prefix,sizeof(t->prefix))||!hf_inline(widget+0x6C,t->suffix,sizeof(t->suffix)))return;
                if(!strncmp(t->prefix,"HUD-MedalScreen-3",17)){if(++medal_row>4)return;slots=0;}
            }else if(MEM32(widget)==0x452F1C){
                if(!medal_row||++slots>3||s.icon_count==32||!menu_pointer_readable(widget,0x50))return;
                uint32_t color=MEM32(widget+0x48);
                /*26FE36/26FF90 use the gray01smg placeholder;26FF44 uses
                 *the row's01..04 art and51CF94 color. This is source policy,
                 *not a reconstruction of earned medals from save bytes. */
                if(color!=0x40ffffffu&&color!=MEM32(0x51CF94))return;
                medal_kinds[s.icon_count]=5-medal_row;
                UcHybridFrontendDetailIcon*i=&s.icons[s.icon_count++];
                i->kind=color==0x40ffffffu?0:5-medal_row;i->color=color;i->x=x;i->y=y;
            }else return;
        }
        last=node;node=MEM32(node);
    }
    if(MEM32(head+4)!=last||s.text_count<2)return;
    /* Newly earned medals are gray placeholders in F4 plus animated medal/
     *glow widgets in100 (25DB00/25D7C0/25D950). Preserve their final art too;
     *the original renderer still owns the award animation and sound timing. */
    head=MEM32(owner+0x104);if(!menu_pointer_readable(head,12))return;
    node=MEM32(head);last=head;visited=0;
    while(node!=head){
        if(++visited>256||!menu_pointer_readable(node,12)||MEM32(node+4)!=last)return;
        uint32_t widget=MEM32(node+8);if(!menu_pointer_readable(widget,0x3C))return;
        if(!MEM8(widget+0x1C)){
            if(!menu_pointer_readable(widget,0xE8)||MEM32(widget)!=0x4873A4)return;
            float x=MEMF(widget+4),y=MEMF(widget+0x24);unsigned i;
            for(i=0;i<s.icon_count;i++)if(fabsf(s.icons[i].x-x)<0.001f&&fabsf(s.icons[i].y-y)<0.001f)break;
            if(i==s.icon_count)return;
            s.icons[i].kind=medal_kinds[i];s.icons[i].color=MEM32(0x51CF94);
        }
        last=node;node=MEM32(node);
    }
    if(MEM32(head+4)!=last)return;
    hf_pending=s;
}
static void hf_publish(void){
    if(!ucrr_native_frontend_enabled())return;
    /* Quit can return to the movie/title container (manager kind0x0A),
     * before any main-menu list exists. Release the outgoing pause pixels
     * once that non-pause screen settles; waiting for a list hides it forever. */
    if(hf_quit_handoff&&hf_settled()&&(MEM32(menu_manager+8)&0x18u)!=0x10u){
        hf_quit_handoff=0;hf_pause_manager=0;
    }
    if(hf_quit_handoff||!hf_settled()){hf_reset();return;}
    hf_pending.screen=menu_screen;hf_pending.screen_vtable=MEM32(menu_screen);hf_pending.source_kind=MEM32(menu_manager+8);
    if(hf_pending.kind==UC_HF_LEVELS||hf_pending.kind==UC_HF_PROFILES)hf_pause_manager=0;
    if(hf_pause_manager==menu_manager)hf_pending.source_kind|=0x80000000u;
    UcHybridFrontendSnapshot previous=hf_view;previous.epoch=0;
    previous.selected=hf_pending.selected; // Selection alone does not replace a screen.
    if(memcmp(&previous,&hf_pending,sizeof(previous)))++hf_epoch;
    hf_view=hf_pending;hf_view.epoch=hf_epoch;
    ucrr_native_frontend_publish(&hf_view);
}
/* Returns ownership of native pointer handling; changed tells the original
 * carousel to run its own selected-level refresh. It is not an input event. */
static int hf_input(uint32_t owner,uint32_t kind,int*changed){
    *changed=0;
    if(!ucrr_native_frontend_enabled()||!hf_settled()||hf_view.kind!=kind||hf_view.owner!=owner||hf_view.screen!=menu_screen)return 0;
    UcHybridFrontendAction a={0};if(!ucrr_native_frontend_take_action(&a))return 1;
    if(a.epoch!=hf_view.epoch||a.screen!=menu_screen||a.owner!=owner)return 1;
    if(a.kind==UC_HF_BACK){
        if(hf_view.back_action&&hf_view.back_action==MEM32(0x485A18)){hf_back_owner=owner;hf_back_kind=kind;}return 1;
    }
    if(kind==UC_HF_LEVELS&&(a.kind==UC_HF_DIFFICULTY||a.kind==UC_HF_REWARDS)){
        unsigned mask=a.kind==UC_HF_DIFFICULTY?1:2;
        if(menu_pointer_readable(owner,0x118)&&MEM32(owner)==0x4885FC&&(hf_view.level_actions&mask)&&
           (mask==1||(MEM32(owner+0xC)&&MEM32(owner+0xC)!=0xffffffffu)))hf_level_action=a.kind;
        return 1;
    }
    if(a.index>=hf_view.count)return 1;
    const UcHybridFrontendRow*r=&hf_view.rows[a.index];
    if(!r->enabled||r->row!=a.row)return 1;
    uint32_t selection;
    if(kind==UC_HF_LIST){
        if(!menu_pointer_readable(owner,0xE8)||a.index>=MEM32(owner+0x58)||MEM32(owner+8+a.index*4)!=a.row||
           !menu_pointer_readable(a.row,0x20)||MEM32(a.row)!=r->table||MEM32(a.row+(r->table==0x487668?0x48:0xC))!=r->callback||!MEM8(a.row+4))return 1;
        char label[128];if(!(r->table==0x487668?hf_inline(a.row+8,label,64):hf_text(MEM32(a.row+8),label))||strcmp(label,r->label))return 1;
        selection=MEM32(owner+4);if(!menu_pointer_readable(selection,4))return 1;
        uint32_t action=a.kind==UC_HF_ACTIVATE?0x16:a.kind==UC_HF_LEFT?0x20:a.kind==UC_HF_RIGHT?0x21:0;
        unsigned mask=a.kind==UC_HF_ACTIVATE?UC_HF_ROW_ACTIVATE:a.kind==UC_HF_LEFT?UC_HF_ROW_LEFT:a.kind==UC_HF_RIGHT?UC_HF_ROW_RIGHT:0;
        if(a.kind!=UC_HF_SELECT&&!(r->actions&mask))return 1;
        if(action==0x16&&(!MEM32(a.row+0xC)||MEM32(a.row+0xC)==0xffffffffu))return 1;
        if(action){menu_click_row=a.row;menu_click_action=action;}
    }else{
        if(!menu_pointer_readable(owner,0x118)||a.index>=MEM32(owner+0xEC)||
           MEM32(owner+0x20+a.index*12)!=a.row||MEM32(owner+0x24+a.index*12)!=r->table||
           !MEM8(owner+0x28+a.index*12)||
           (a.kind!=UC_HF_SELECT&&a.kind!=UC_HF_ACTIVATE))return 1;
        selection=owner+0x1C;if(a.kind==UC_HF_ACTIVATE)level_click_owner=owner;
    }
    *changed=MEM32(selection)!=a.index;MEM32(selection)=a.index;return 1;
}
static int hf_back_query(void){
    if(hf_back_owner&&hf_back_kind==UC_HF_REWARD_BROWSER&&hf_settled()&&g_esi==hf_back_owner&&
       menu_pointer_readable(g_esp,8)&&MEM32(g_esp)==0x25CCEE&&MEM32(g_esp+4)==hf_view.back_action)return 1;
    if(hf_level_action&&hf_settled()&&g_esi==hf_view.owner&&hf_view.kind==UC_HF_LEVELS&&menu_pointer_readable(g_esp,8)){
        if(hf_level_action==UC_HF_DIFFICULTY&&MEM32(g_esp)==0x2707AF&&MEM32(g_esp+4)==0x19)return 1;
        if(hf_level_action==UC_HF_REWARDS&&MEM32(g_esp)==0x270AF0&&MEM32(g_esp+4)==0x18)return 1;
    }
    if(!hf_back_owner||!hf_settled()||g_esi!=hf_back_owner||!menu_pointer_readable(g_esp,8)||MEM32(g_esp+4)!=hf_view.back_action)return 0;
    return (hf_back_kind==UC_HF_LIST&&MEM32(g_esp)==0x266848)||
        (hf_back_kind==UC_HF_LEVELS&&MEM32(g_esp)==0x270AA5);
}
/* Original25C710's carousel records are filled by25B2A0, including locked
 * entries. Native selection only asks25B460 to rebuild the original panel. */
int ucrr_rewards_hover(void){
    uint32_t owner=g_esi;
    if(!ucrr_native_frontend_enabled()||!hf_settled()||!menu_pointer_readable(owner,0x34)||MEM32(owner)!=0x485F14)return 0;
    uint32_t count=MEM32(owner+0x18),base=MEM32(owner+0x2C);
    if(!count||count>55||MEM32(owner+4)>=count||!menu_pointer_readable(base,count*0x2C))return 0;
    UcHybridFrontendSnapshot s={0};s.kind=UC_HF_REWARD_BROWSER;s.owner=owner;s.count=count;s.selected=MEM32(owner+4);
    s.back_action=MEM32(owner+0x14)?MEM32(0x485A18):0;strcpy(s.title,"Rewards");
    for(unsigned i=0;i<count;i++){
        UcHybridFrontendRow*r=&s.rows[i];r->row=base+i*0x2C;r->index=i;r->enabled=1;r->actions=UC_HF_ROW_ACTIVATE;
        uint32_t label=MEM8(r->row+0x29)?0x485EF0:MEM32(r->row+(MEM8(r->row+0x28)?0x1C:0x18));
        if(!hf_text(label,r->label))return 0;
    }
    int changed=0;
    if(hf_view.kind==s.kind&&hf_view.owner==owner&&hf_view.screen==menu_screen){
        UcHybridFrontendAction a={0};
        if(ucrr_native_frontend_take_action(&a)&&a.epoch==hf_view.epoch&&a.screen==menu_screen&&a.owner==owner){
            if(a.kind==UC_HF_BACK&&s.back_action){hf_back_owner=owner;hf_back_kind=s.kind;}
            else if(a.index<count&&a.row==s.rows[a.index].row&&!strcmp(s.rows[a.index].label,hf_view.rows[a.index].label)&&
                    (a.kind==UC_HF_SELECT||a.kind==UC_HF_ACTIVATE)){
                changed=s.selected!=a.index;s.selected=a.index;MEM32(owner+4)=a.index;
            }
        }
    }
    menu_seen=1;menu_render_count=1;hf_pending=s;return changed;
}
