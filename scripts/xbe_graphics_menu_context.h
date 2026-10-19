#pragma once
// Host copies this identity only; it must not read guest memory. Kind 0 revokes
// the PC graphics entry, 1 is Main, 2 is Pause, and 3 is the Options hub.
extern void TrialGraphicsMenuContext(uint32_t manager,uint32_t screen,uint32_t owner,uint32_t kind);
struct TrialMenuContext { uint32_t manager=0,screen=0,owner=0,kind=0; };
static TrialMenuContext graphics_pending,graphics_published;
static uint32_t GraphicsMenuKind(uint32_t owner){
    if(!menu_pointer_readable(owner,0xE8)||MEM32(owner)!=0x4877B4)return 0;
    const uint32_t selection=MEM32(owner+4),count=MEM32(owner+0x58);
    if(!count||count>20||!menu_pointer_readable(selection,4)||MEM32(selection)>=count)return 0;
    // Original factories: Main 2722A5, Pause 2677F4/267F9B, Options 276DFC.
    // 51D300 is Controller Options, not the Options hub (51D310).
    return selection==0x51D29C?1:selection==0x51CFA4?2:selection==0x51D310?3:
        selection==0x51D304?4:selection==0x51D30C?5:0;
}
static void GraphicsMenuPublish(TrialMenuContext context={}){
    graphics_published=context;
    TrialGraphicsMenuContext(context.manager,context.screen,context.owner,context.kind);
}
static void GraphicsMenuBegin(){
    graphics_pending={};
    if(!hf_settled())hf_reset();
    if(!hf_settled()||graphics_published.manager!=menu_manager||graphics_published.screen!=menu_screen)
        GraphicsMenuPublish();
}
static void GraphicsMenuCapture(){
    if(menu_special_active()||!hf_settled()||hf_eligible_owner!=menu_owner||!menu_render_count)return;
    if(const uint32_t kind=GraphicsMenuKind(menu_owner))
        graphics_pending={menu_manager,menu_screen,menu_owner,kind};
}
static void GraphicsMenuEnd(){
    // 272870 can replace/delete the rendered screen before it returns. Never
    // expose that outgoing list, a modal, or a list whose input gate was closed.
    if(!menu_seen||menu_special_active()||!hf_settled()||
       graphics_pending.manager!=menu_manager||graphics_pending.screen!=menu_screen||
       graphics_pending.owner!=hf_eligible_owner||
       GraphicsMenuKind(graphics_pending.owner)!=graphics_pending.kind||!graphics_pending.kind){
        GraphicsMenuPublish();return;
    }
    GraphicsMenuPublish(graphics_pending);
}
static void GraphicsMenuDestroy(uint32_t manager){
    if(manager==graphics_published.manager||manager==graphics_pending.manager){
        graphics_pending={};GraphicsMenuPublish();
    }
}
static void GraphicsMenuSpecial(){
    if(menu_special_active()){graphics_pending={};GraphicsMenuPublish();}
}
