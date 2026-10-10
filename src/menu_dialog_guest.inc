/* Original modal choices (485B78) and profile slots (48CB30). These screens
 * bypass the command-row list. Keep their pixels, callbacks and save policy. */
typedef struct {
    UCRR_MENU_TARGET rect;
    uint32_t label, callback, selection, count;
    uint64_t frame;
} UcMenuSpecialTarget;
static UcMenuSpecialTarget dialog_targets[8],profile_targets[4];
static uint64_t special_frame;
static uint32_t dialog_owner,profile_owner,dialog_click,profile_click;
static unsigned dialog_count,profile_count,dialog_drawn,profile_drawn;
static int dialog_running,profile_running,special_seen;
static int dialog_pointer,profile_pointer,profile_eligible;
static float dialog_x,dialog_y,profile_x,profile_y;

static int menu_special_active(void){return special_seen;}
static void menu_special_reset(void){
    dialog_owner=profile_owner=dialog_click=profile_click=0;
    dialog_count=profile_count=dialog_drawn=profile_drawn=0;
    dialog_running=profile_running=special_seen=dialog_pointer=profile_pointer=0;
    memset(dialog_targets,0,sizeof(dialog_targets));memset(profile_targets,0,sizeof(profile_targets));
}
static void menu_special_begin_frame(void){
    ++special_frame;special_seen=dialog_running=profile_running=0;
    dialog_click=profile_click=0;dialog_drawn=profile_drawn=0;
}
static void menu_special_reveal(void){
    special_seen=menu_seen=1;menu_render_count=1;
    /* Revoke an outgoing native parent immediately, before more host events. */
    if(hf_view.kind&&!(dialog_running&&hf_view.kind==UC_HF_DIALOG&&hf_view.owner==dialog_owner)&&
       !(profile_running&&hf_view.kind==UC_HF_PROFILES&&hf_view.owner==profile_owner))hf_reset();
    memset(&hf_pending,0,sizeof(hf_pending));
}
static int menu_dialog_valid(uint32_t owner){
    return menu_pointer_readable(owner,0x8C)&&MEM32(owner)==0x485B78u&&
        MEM32(owner+0xC)>0&&MEM32(owner+0xC)<=8&&MEM32(owner+8)<MEM32(owner+0xC);
}
void ucrr_menu_dialog_enter(void){
    dialog_running=0;dialog_click=0;
    if(!menu_dialog_valid(g_ecx))return;
    if(dialog_owner!=g_ecx){dialog_owner=g_ecx;dialog_count=0;dialog_pointer=0;}
    dialog_running=1;menu_special_reveal();
}
void ucrr_menu_dialog_hover(void){
    if(!dialog_running||g_ebx!=dialog_owner||!hf_settled()||!menu_dialog_valid(dialog_owner))return;
    if(ucrr_native_frontend_enabled()&&hf_view.kind==UC_HF_DIALOG&&hf_view.owner==dialog_owner&&hf_view.screen==menu_screen){
        UcHybridFrontendAction a={0};if(!ucrr_native_frontend_take_action(&a))return;
        if(a.epoch!=hf_view.epoch||a.owner!=dialog_owner||a.screen!=menu_screen)return;
        if(a.kind==UC_HF_BACK){if(MEM32(dialog_owner+0x50)&&hf_view.back_action)hf_back_owner=dialog_owner;return;}
        if(a.index>=hf_view.count||a.index>=MEM32(dialog_owner+0xC))return;
        const UcHybridFrontendRow *r=&hf_view.rows[a.index];char label[128];
        if(a.row!=r->row||MEM32(dialog_owner+0x30+4*a.index)!=r->callback||!r->enabled||
           !hf_text(MEM32(dialog_owner+0x10+4*a.index),label)||strcmp(label,r->label))return;
        if(a.kind==UC_HF_SELECT||a.kind==UC_HF_ACTIVATE){MEM32(dialog_owner+8)=a.index;if(a.kind==UC_HF_ACTIVATE){
            dialog_click=dialog_owner;
            if(r->callback==0x267400u)hf_quit_handoff=1;
        }}
        return;
    }
    float x,y;int click=0;if(!ucrr_host_menu_pointer(&x,&y,&click))return;
    if(dialog_pointer&&x==dialog_x&&y==dialog_y&&!click)return;
    dialog_pointer=1;dialog_x=x;dialog_y=y;
    for(unsigned i=0;i<dialog_count;i++){
        const UcMenuSpecialTarget*t=&dialog_targets[i];uint32_t n=t->rect.index;
        if(t->frame+1!=special_frame||t->rect.owner!=dialog_owner||n>=MEM32(dialog_owner+0xC)||
           t->count!=MEM32(dialog_owner+0xC)||t->label!=MEM32(dialog_owner+0x10+4*n)||
           t->callback!=MEM32(dialog_owner+0x30+4*n)||!t->callback||t->callback==0xffffffffu||
           !ucrr_menu_target_hit(&t->rect,x,y))continue;
        MEM32(dialog_owner+8)=n;if(click)dialog_click=dialog_owner;break;
    }
}
void ucrr_menu_dialog_bounds(void){
    if(!dialog_running||g_ebx!=dialog_owner||!menu_dialog_valid(dialog_owner)||!menu_pointer_readable(g_esp,0x44))return;
    unsigned n=MEM32(g_esp+0x20);if(n>=MEM32(dialog_owner+0xC)||g_esi!=dialog_owner+0x68+4*n)return;
    uint32_t font=MEM32(0x509704),face=MEM32(dialog_owner+0x5C);
    if(face>3||!menu_pointer_readable(font,0x24))return;
    uint32_t metrics=MEM32(font+0x14+4*face);if(!menu_pointer_readable(metrics,4))return;
    float x=MEMF(g_esp+0x18),y=MEMF(g_esp+0x40),w=MEMF(g_esi),h=MEMF(metrics);
    if(!isfinite(x)||!isfinite(y)||!isfinite(w)||!isfinite(h)||w<=0||w>640||h<=0||h>120)return;
    if(!dialog_drawn++)dialog_count=0;
    if(dialog_count<8){UcMenuSpecialTarget*t=&dialog_targets[dialog_count++];
        t->rect=(UCRR_MENU_TARGET){dialog_owner,dialog_owner+0x10+4*n,n,x,y,w,h};
        t->label=MEM32(dialog_owner+0x10+4*n);t->callback=MEM32(dialog_owner+0x30+4*n);
        t->count=MEM32(dialog_owner+0xC);t->frame=special_frame;}
}
void ucrr_menu_dialog_leave(void){
    if(dialog_running&&ucrr_native_frontend_enabled()&&hf_settled()&&menu_dialog_valid(dialog_owner)){
        UcHybridFrontendSnapshot s={0};s.kind=UC_HF_DIALOG;s.owner=dialog_owner;
        s.count=MEM32(dialog_owner+0xC);s.selected=MEM32(dialog_owner+8);
        s.back_action=MEM32(dialog_owner+0x50)?0x17:0;
        int valid=hf_text(MEM32(dialog_owner+4),s.title)&&s.title[0];
        for(unsigned i=0;i<s.count&&valid;i++){
            UcHybridFrontendRow *r=&s.rows[i];r->row=dialog_owner+0x10+4*i;r->index=i;
            r->callback=MEM32(dialog_owner+0x30+4*i);r->enabled=r->callback&&r->callback!=0xffffffffu;
            r->actions=UC_HF_ROW_ACTIVATE;valid=hf_text(MEM32(r->row),r->label);
        }
        if(valid)hf_pending=s;
    }
    dialog_running=0;dialog_click=0;if(!dialog_drawn)dialog_count=0;
}

static int menu_profile_valid(uint32_t owner){
    return menu_pointer_readable(owner,0x69C)&&MEM32(owner)==0x48CB30u&&MEM32(owner+0x688)<=4&&
        menu_pointer_readable(MEM32(owner+4),4)&&MEM32(MEM32(owner+4))<4;
}
/* 284330 draws the inline name at slot+1C; 2831A0 copies the
 * byte string "Nick Mason" here. Empty slots retain the localized label. */
static int menu_profile_label(uint32_t owner,unsigned index,char label[128]){
    return index>=MEM32(owner+0x688)?hf_text(0x48CB70,label):
        hf_inline(owner+0x1C+index*0x19C,label,20)&&label[0];
}
void ucrr_menu_profile_enter(void){
    profile_running=profile_eligible=0;profile_click=0;
    if(!menu_profile_valid(g_ecx))return;
    if(profile_owner!=g_ecx){profile_owner=g_ecx;profile_count=0;profile_pointer=0;}
    profile_running=1;menu_special_reveal();
}
/* Called only inside the source child-inactive branch. True enters28415D,
 * retaining the original selection sound,51D704 write and2835E0 refresh. */
int ucrr_menu_profile_hover(void){
    if(!profile_running||g_esi!=profile_owner||!hf_settled()||!menu_profile_valid(profile_owner))return 0;
    profile_eligible=1;
    if(ucrr_native_frontend_enabled()&&hf_view.kind==UC_HF_PROFILES&&hf_view.owner==profile_owner&&hf_view.screen==menu_screen){
        UcHybridFrontendAction a={0};if(!ucrr_native_frontend_take_action(&a))return 0;
        if(a.epoch!=hf_view.epoch||a.owner!=profile_owner||a.screen!=menu_screen)return 0;
        if(a.kind==UC_HF_BACK){if(MEM32(profile_owner+0x14)&&hf_view.back_action)hf_back_owner=profile_owner;return 0;}
        if(a.index>=4||a.row!=profile_owner+0x18+0x19C*a.index)return 0;
        const UcHybridFrontendRow*r=&hf_view.rows[a.index];char label[128];
        if(!r->enabled||r->callback!=MEM32(profile_owner+(a.index<MEM32(profile_owner+0x688)?8:0xC))||
           !menu_profile_label(profile_owner,a.index,label)||strcmp(label,r->label)||
           (a.kind!=UC_HF_SELECT&&a.kind!=UC_HF_ACTIVATE))return 0;
        uint32_t selection=MEM32(profile_owner+4);int changed=MEM32(selection)!=a.index;
        MEM32(selection)=a.index;if(a.kind==UC_HF_ACTIVATE)profile_click=profile_owner;return changed;
    }
    float x,y;int click=0;if(!ucrr_host_menu_pointer(&x,&y,&click))return 0;
    if(profile_pointer&&x==profile_x&&y==profile_y&&!click)return 0;
    profile_pointer=1;profile_x=x;profile_y=y;
    for(unsigned i=0;i<profile_count;i++){
        const UcMenuSpecialTarget*t=&profile_targets[i];unsigned n=t->rect.index;
        if(t->frame+1!=special_frame||t->rect.owner!=profile_owner||n>=4||
           t->selection!=MEM32(profile_owner+4)||t->count!=MEM32(profile_owner+0x688)||
           t->callback!=MEM32(profile_owner+(n<t->count?8:0xC))||!t->callback||t->callback==0xffffffffu||
           !ucrr_menu_target_hit(&t->rect,x,y))continue;
        uint32_t selection=MEM32(profile_owner+4);int changed=MEM32(selection)!=n;
        MEM32(selection)=n;if(click)profile_click=profile_owner;return changed;
    }
    return 0;
}
void ucrr_menu_profile_bounds(void){
    if(!profile_running||g_edi!=profile_owner||g_esi>=4||!menu_profile_valid(profile_owner)||!menu_pointer_readable(g_esp,16))return;
    float y=MEMF(g_esp),bottom=MEMF(g_esp+4),right=MEMF(g_esp+8),x=MEMF(g_esp+12);
    float w=right-x,h=bottom-y;if(!isfinite(x)||!isfinite(y)||!isfinite(w)||!isfinite(h)||w<=0||w>640||h<=0||h>480)return;
    if(!profile_drawn++)profile_count=0;
    if(profile_count<4){UcMenuSpecialTarget*t=&profile_targets[profile_count++];
        t->rect=(UCRR_MENU_TARGET){profile_owner,profile_owner+0x18+0x19C*g_esi,g_esi,x,y,w,h};
        t->selection=MEM32(profile_owner+4);t->count=MEM32(profile_owner+0x688);
        t->callback=MEM32(profile_owner+(g_esi<t->count?8:0xC));t->frame=special_frame;}
}
void ucrr_menu_profile_leave(void){
    if(profile_running&&profile_eligible&&ucrr_native_frontend_enabled()&&hf_settled()&&menu_profile_valid(profile_owner)){
        UcHybridFrontendSnapshot s={0};s.kind=UC_HF_PROFILES;s.owner=profile_owner;s.count=4;
        s.selected=MEM32(MEM32(profile_owner+4));s.back_action=MEM32(profile_owner+0x14)?MEM32(0x485A18):0;
        strcpy(s.title,"Single player: Saved games");int valid=1;
        for(unsigned i=0;i<4&&valid;i++){
            UcHybridFrontendRow*r=&s.rows[i];r->index=i;r->row=profile_owner+0x18+0x19C*i;
            r->callback=MEM32(profile_owner+(i<MEM32(profile_owner+0x688)?8:0xC));
            r->enabled=r->callback&&r->callback!=0xffffffffu;r->actions=UC_HF_ROW_ACTIVATE;
            valid=menu_profile_label(profile_owner,i,r->label);
        }
        if(valid)hf_pending=s;
    }
    profile_running=0;profile_click=0;if(!profile_drawn)profile_count=0;
}
static int menu_special_query(void){
    if(!hf_settled()||!menu_pointer_readable(g_esp,8))return 0;
    if(dialog_running&&hf_back_owner==dialog_owner&&g_ebx==dialog_owner&&MEM32(g_esp)==0x25A857u&&MEM32(g_esp+4)==0x17&&menu_dialog_valid(dialog_owner))return 1;
    if(profile_running&&hf_back_owner==profile_owner&&g_esi==profile_owner&&MEM32(g_esp)==0x284281u&&MEM32(g_esp+4)==MEM32(0x485A18)&&menu_profile_valid(profile_owner))return 1;
    if(MEM32(g_esp+4)!=0x16)return 0;
    if(dialog_running&&dialog_click==dialog_owner&&g_ebx==dialog_owner&&MEM32(g_esp)==0x25A825u&&menu_dialog_valid(dialog_owner))return 1;
    return profile_running&&profile_click==profile_owner&&g_esi==profile_owner&&MEM32(g_esp)==0x284221u&&menu_profile_valid(profile_owner);
}
