// Same original frame/fonts/selection strips as Graphics. Only drafts live
// here: the input backend publishes a complete profile after a successful save.
trial_controls::Draft controls_draft{};
unsigned controls_device=0,controls_player=0,controls_scroll=0,controls_slot=0;
int controls_capture_action=-1,controls_conflict_action=-1;
unsigned controls_candidate=0;
const char* const control_names[]={
    "Reload / interact","Melee weapons","Firearms","Thrown weapons","Secondary / flashlight",
    "Thermal breather","Raise shield","Fire / attack","Pause / start","Objectives","Crouch","Zoom",
    "Context order","Teammate orders","Follow / accept help","Cover / decline help",
    "Move left (Standard)","Move right (Standard)","Move forward","Move backward","Look left (Standard)","Look right (Standard)","Look up","Look down","Mouse orders"};
const char* const control_help[]={
    "Reload or use nearby objects. The secondary binding is shown for interactions and skips conversations.",
    "Select or cycle melee weapons and the stun gun. Hold for quick use.",
    "Equip a firearm; press again to cycle available firearms.",
    "Select or cycle thrown weapons. Hold for a quick throw.",
    "Use the weapon's alternate action: flashlight, fire mode, grenade roll or close attack.",
    "Put on or remove the thermal breather when it is available.",
    "Hold to raise the riot shield. Release to lower it; Fire attacks with the shield raised.",
    "Fire or attack. Hold to use the stun gun or charge a throw, then release to throw.",
    "Pause gameplay. Escape always pauses; a custom key also works. Enter stays menu-only.",
    "View mission objectives. Scroll with the menu directions and use Back to close.",
    "Crouch using the game's selected hold or toggle behavior.",
    "Zoom with supported weapons, including while using the shield.",
    "Issue the available context action to a teammate. Original Xbox D-pad up.",
    "Display the available teammate orders. Original Xbox D-pad down.",
    "Ask a teammate to follow, or accept healing when offered.",
    "Ask a teammate to take cover, or decline healing when offered.",
    "Left stick left: moves left with Standard thumbsticks. Thumbstick layout selects Legacy controls.",
    "Left stick right: moves right with Standard thumbsticks. Thumbstick layout selects Legacy controls.",
    "Move forward. Opposite directions cancel each other.",
    "Move backward. Opposite directions cancel each other.",
    "Right stick left: turns left with Standard thumbsticks. Mouse movement also aims during gameplay.",
    "Right stick right: turns right with Standard thumbsticks. Mouse movement also aims during gameplay.",
    "Look up. The game's invert-look setting can reverse this direction.",
    "Look down. The game's invert-look setting can reverse this direction.",
    "Hold and drag toward a teammate order or help option, then release. Mouse wheel cannot be held."};

unsigned ControlsBase(){return controls_device?24:25;}
unsigned ControlsProfileStart(){return ControlsBase()+(controls_device?10:15);}
bool ControlsMixedInput(unsigned action){return action==47;}
unsigned ControlsCount(){return (controls_device?43:41)+(controls_player?0:1);}
bool ControlsProfile(unsigned action){return !ControlsMixedInput(action)&&action>=ControlsProfileStart();}
bool ControlsDeadzone(unsigned action){return controls_device&&ControlsProfile(action)&&action>=ControlsProfileStart()+7;}
const char* profile_names[]={"Vibration","Invert stick look","Auto-centre","Crouch mode","Thumbstick layout","Horizontal stick sensitivity","Vertical stick sensitivity"};
const char* profile_help[]={"Controller vibration.","Reverse vertical stick aiming; raw mouse aiming is unchanged.","Use the original automatic camera centring.","Choose hold or toggle for crouching.","Standard or Legacy movement and aiming sticks.","Original horizontal stick sensitivity; mouse sensitivity is separate.","Original vertical stick sensitivity; mouse sensitivity is separate."};
unsigned ControlsAction(unsigned row){
    // Reorder only the display; saved action and native profile IDs stay stable.
    if(!controls_player){if(!row)return 47;--row;}
    static constexpr unsigned keyboard[]={27,35,33,31,37,39,7,0,28,11,32,6,30,34,2,29,1,3,4,5,25,26,18,19,16,17,10,43,36,20,21,22,23,24,12,13,14,15,8,9,38};
    static constexpr unsigned controller[]={38,39,40,41,42,35,36,34,37,29,27,31,33,7,0,11,28,6,26,2,1,3,4,5,24,25,18,19,16,17,10,30,20,21,22,23,12,13,14,15,8,9,32};
    return controls_device?controller[row]:keyboard[row];
}
const char* ControlsGroup(unsigned row){
    if(!controls_player){if(!row)return "INPUT SETTINGS";--row;}
    if(row>=(controls_device?40u:38u))return "MENU CONTROLS";
    if(row>=(controls_device?36u:33u))return "TEAMMATE ORDERS";
    if(row>=(controls_device?26u:22u))return "MOVEMENT / AIMING";
    if(row<(controls_device?13u:6u))return "INPUT SETTINGS";
    return "COMBAT / INTERACTION";
}
bool ControlsInteract(unsigned action){return !controls_device&&action==ControlsBase()+3;}
bool ControlsPreviousFirearm(unsigned action){return !controls_device&&action==ControlsBase()+4;}
bool ControlsHealPartner(unsigned action){return !controls_device&&action==ControlsBase()+9;}
bool ControlsHealthPack(unsigned action){return action==ControlsBase()+(controls_device?2:5);}
bool ControlsAimMode(unsigned action){return action==ControlsBase()+(controls_device?3:6);}
bool ControlsScopeZoom(unsigned action){return action==ControlsBase()+(controls_device?4:7);}
bool ControlsZoomSensitivity(unsigned action){return action==ControlsBase()+(controls_device?5:10);}
bool ControlsWalk(unsigned action){return action==ControlsProfileStart()-4;}
bool ControlsWalkMode(unsigned action){return action==ControlsProfileStart()-3;}
bool ControlsHud(unsigned action){return action==ControlsProfileStart()-2;}
bool ControlsMovementInertia(unsigned action){return action==ControlsProfileStart()-1;}
bool ControlsMouseInvert(unsigned action){return !controls_device&&action==ControlsBase()+8;}
unsigned ControlsSlots(unsigned action){if(ControlsMixedInput(action)||ControlsProfile(action)||ControlsAimMode(action)||ControlsWalkMode(action)||ControlsMovementInertia(action)||ControlsMouseInvert(action))return 1;return !controls_device&&(action==0||ControlsInteract(action))?1:2;}
bool ControlsMouseSensitivity(unsigned action);
bool ControlsLookSlider(unsigned action){return ControlsMouseSensitivity(action)||ControlsZoomSensitivity(action);}
const char* ControlsName(unsigned action){if(ControlsMixedInput(action))return "Controller + mouse";if(ControlsDeadzone(action))return action==ControlsProfileStart()+7?"Left stick deadzone":"Right stick deadzone";if(ControlsMovementInertia(action))return "Movement inertia";if(ControlsWalk(action))return "Walk";if(ControlsWalkMode(action))return "Walking mode";if(ControlsHud(action))return "Toggle HUD";if(ControlsZoomSensitivity(action))return "Zoom sensitivity";if(ControlsHealPartner(action))return "Heal Partner";if(ControlsMouseInvert(action))return "Invert mouse look";if(ControlsAimMode(action))return "Aiming mode";if(ControlsScopeZoom(action))return "Cycle scope zoom";if(ControlsHealthPack(action))return "Use health pack";if(ControlsProfile(action))return profile_names[action-ControlsProfileStart()];if(ControlsInteract(action))return "Interact / dialogue skip";if(ControlsPreviousFirearm(action))return "Previous Firearm";if(!controls_device&&action==2)return "Next Firearm";if(!controls_device&&action==0)return "Reload";if(ControlsMouseSensitivity(action))return "Mouse sensitivity";return action<ControlsBase()?control_names[action]:action==ControlsBase()?"Pistol spin mode":"Pistol spin button";}
const char* ControlsHelp(unsigned action){if(ControlsMixedInput(action))return "Use controller and mouse together in single-player gameplay. Menus use the last active device.";if(ControlsDeadzone(action))return "Ignore small stick movements. 0% responds immediately; Original keeps the original controller response. Applies to this player only.";if(ControlsMovementInertia(action))return "Keep the original movement build-up. Off makes movement input respond immediately.";if(ControlsWalk(action))return "Move slowly using this input. Walking mode chooses Hold or Toggle.";if(ControlsWalkMode(action))return "Hold Walk to move slowly, or press once to toggle walking. Opening a menu cancels walking.";if(ControlsHud(action))return "Show or hide gameplay HUD for this player. Menus remain visible.";if(ControlsZoomSensitivity(action))return "Look speed while zoomed, for this player only. 100% keeps the current speed; lower values aim more slowly.";if(ControlsHealPartner(action))return "Use your pack to heal or revive the nearby partner you are facing. Living partners have no healing prompt.";if(ControlsMouseInvert(action))return "Reverse vertical mouse aiming. Stick aiming and mouse orders are unchanged.";if(ControlsAimMode(action))return "Toggle preserves the original zoom cycle. Hold needs a held key or button. Use Cycle scope zoom to change rifle magnification.";if(ControlsScopeZoom(action))return "In Hold mode, change rifle magnification while aiming. Assign a separate input; does not zoom out.";if(ControlsHealthPack(action))return controls_device?"Controller: double-tap D-pad Down to heal yourself; hold Down while facing your partner to heal or revive them. Collect more packs from medics.":"Use a stored health pack, or revive a nearby downed partner. Collect more from a medic while supplies last.";if(ControlsProfile(action))return profile_help[action-ControlsProfileStart()];if(ControlsInteract(action))return "Use nearby objects and skip dialogue. This key is displayed in interaction prompts.";if(ControlsPreviousFirearm(action))return "Equip or select the previous available firearm.";if(!controls_device&&action==2)return "Equip or select the next available firearm.";if(!controls_device&&action==0)return "Reload your weapon without activating nearby objects.";if(ControlsMouseSensitivity(action))return "Mouse look speed: 100% is the default. Lower is slower; independent of Windows pointer speed and stick sensitivity.";return action<ControlsBase()?control_help[action]:action==ControlsBase()?"Original gesture: rotate the stick or move the mouse in two full circles. Button press: use your assigned input.":"Used in Button press mode. Runs the original pistol spin and next-kill slow-motion reward.";}
bool ControlsMouseSensitivity(unsigned action){return !controls_device&&action==ControlsBase()+2;}
bool ControlsBindingAction(unsigned action){
    return !ControlsMixedInput(action)&&action!=ControlsBase()&&!ControlsAimMode(action)&&!ControlsWalkMode(action)&&
        !ControlsMovementInertia(action)&&!ControlsMouseInvert(action)&&!ControlsLookSlider(action)&&!ControlsProfile(action);
}
unsigned& ControlsBinding(unsigned action,unsigned slot){
    if(ControlsWalk(action))return controls_draft.walk_binding[controls_device][slot];
    if(ControlsHud(action))return controls_draft.hud_binding[controls_device][slot];
    // Keep existing profiles and prompt consumers compatible with the two retail-action slots.
    if(ControlsInteract(action))return controls_draft.keyboard[0][1];
    if(ControlsPreviousFirearm(action))return controls_draft.previous_firearm[slot];
    if(ControlsHealthPack(action))return controls_draft.health_pack[controls_device][slot];
    if(ControlsHealPartner(action))return controls_draft.heal_partner[slot];
    if(ControlsScopeZoom(action))return controls_draft.scope_zoom[controls_device][slot];
    if(action>=ControlsBase())return controls_draft.spin_binding[controls_device][slot];
    return controls_device?controls_draft.controller[action][slot]:controls_draft.keyboard[action][slot];
}
void ControlsCancelCapture(){
    trial_controls::CancelCapture(controls_player);controls_capture_action=controls_conflict_action=-1;controls_candidate=0;
}
void ControlsReadDraft(){
    ControlsCancelCapture();controls_draft=trial_controls::Snapshot(controls_player);controls_scroll=0;
    std::lock_guard lock(context_lock);std::memcpy(profile_draft,controls_player?controls_draft.native_profile:profile_live,sizeof(profile_draft));
}
void ControlsOpen(int player){
    ControlsCancelCapture();
    if(player>=0){controls_player=unsigned(player);controls_device=player?1u:0u;}
    ControlsReadDraft();
}
bool ControlsDirty(){const auto live=trial_controls::Snapshot(controls_player);std::lock_guard lock(context_lock);return std::memcmp(&controls_draft,&live,sizeof(live))!=0||std::memcmp(profile_draft,controls_player?live.native_profile:profile_live,sizeof(profile_draft))!=0;}
bool ControlsApply(){
    {std::lock_guard lock(context_lock);if((!controls_player&&!profile_valid)||!SameMenu(context,opened)){message="Controls context changed. Reopen Controls.";return false;}}
    if(controls_player)std::memcpy(controls_draft.native_profile,profile_draft,sizeof(profile_draft));
    if(!trial_controls::Apply(controls_draft,message,controls_player))return false;
    if(!controls_player){std::lock_guard lock(context_lock);
        if(!profile_valid||!SameMenu(context,opened)){message="Bindings saved; profile changed. Reopen Controls.";return false;}
        std::memcpy(profile_expected,profile_live,sizeof(profile_expected));
        std::memcpy(profile_pending,profile_draft,sizeof(profile_pending));profile_origin=context;profile_apply=true;}
    controls_draft=trial_controls::Snapshot(controls_player);message="Controls applied and saved.";TrialGraphicsInputChanged();return true;
}
void ControlsFinish(){Close();}
void ControlsBack(){
    if(controls_capture_action>=0||controls_conflict_action>=0)ControlsCancelCapture();
    else if(confirm_back)confirm_back=false;
    else if(ControlsDirty())confirm_back=true;
    else{ControlsFinish();return;}
    ++navigation_generation;frontend_events=0;
}
bool ControlsOverlap(unsigned a,unsigned b){
    if(!a||!b)return false;if(a==b)return true;if(controls_device)return false;
    auto family=[](unsigned v){return v==VK_LSHIFT||v==VK_RSHIFT?VK_SHIFT:
        v==VK_LCONTROL||v==VK_RCONTROL?VK_CONTROL:v==VK_LMENU||v==VK_RMENU?VK_MENU:0u;};
    return family(a)==b||family(b)==a;
}
void ControlsAssign(bool replace){
    const unsigned action=unsigned(controls_capture_action);
    if(replace)for(unsigned i=0;i<ControlsProfileStart();++i)for(unsigned slot=0;slot<ControlsSlots(i);++slot)
        if(ControlsBindingAction(i)&&(i!=action||slot!=controls_slot)&&ControlsOverlap(ControlsBinding(i,slot),controls_candidate))ControlsBinding(i,slot)=0;
    ControlsBinding(action,controls_slot)=controls_candidate;
    ControlsCancelCapture();message="Binding changed. Select Apply to save.";
    ++navigation_generation;frontend_events=0;
}
void ControlsCandidate(unsigned source){
    if(controls_capture_action<0)return;
    if(!controls_device&&(source>257||source==VK_ESCAPE||source==VK_LWIN||source==VK_RWIN||
       ((controls_capture_action==24||unsigned(controls_capture_action)==ControlsBase()+1)&&source>=256))){
        message=controls_capture_action==24&&source>=256?"Mouse orders needs a button or key that can be held.":"That key is reserved. Choose another input.";
        trial_controls::BeginCapture(controls_device,controls_player);return;
    }
    controls_candidate=source;controls_conflict_action=-1;
    if(source)for(unsigned action=0;action<ControlsProfileStart();++action)if(action!=ControlsBase()&&!ControlsAimMode(action)&&!ControlsWalkMode(action)&&!ControlsMovementInertia(action)&&!ControlsMouseInvert(action)&&!ControlsLookSlider(action)&&!ControlsProfile(action)&&action!=unsigned(controls_capture_action))
        for(unsigned slot=0;slot<ControlsSlots(action);++slot)if(ControlsOverlap(source,ControlsBinding(action,slot)))controls_conflict_action=int(action);
    if(controls_conflict_action<0)ControlsAssign(false);
    else{++navigation_generation;frontend_events=0;}
}
void ControlsBegin(unsigned action,unsigned slot){
    if(ControlsMixedInput(action)){
        if(!controls_player){controls_draft.mixed_input^=1;message="Controller and mouse mode changed. Apply to save.";}
        return;
    }
    if(ControlsProfile(action))return;
    if(ControlsWalkMode(action)){controls_draft.walk_mode^=1;message="Walking mode changed. Apply to save.";return;}
    if(ControlsMovementInertia(action)){controls_draft.movement_inertia^=1;message="Movement inertia changed. Apply to save.";return;}
    if(ControlsMouseInvert(action)){controls_draft.invert_mouse^=1;message="Mouse look inversion changed. Apply to save.";return;}
    if(ControlsAimMode(action)){controls_draft.aim_mode^=1;message="Aiming mode changed. Apply to save.";return;}
    if(slot>=ControlsSlots(action))slot=0;
    if(ControlsZoomSensitivity(action)){
        controls_draft.zoom_sensitivity=unsigned(std::clamp(int(controls_draft.zoom_sensitivity)+(slot?1:-1),1,200));
        message="Zoom sensitivity changed. Apply to save.";return;
    }
    if(ControlsMouseSensitivity(action)){
        controls_draft.sensitivity=unsigned(std::clamp(int(controls_draft.sensitivity)+(slot?1:-1),1,1600));
        message="Mouse sensitivity changed. Apply to save.";return;
    }
    if(action==ControlsBase()){controls_draft.spin_mode^=1;message="Pistol spin mode changed. Apply to save.";return;}
    controls_capture_action=int(action);controls_slot=slot;controls_conflict_action=-1;controls_candidate=0;message.clear();
    trial_controls::BeginCapture(controls_device,controls_player);++navigation_generation;frontend_events=0;
}
void ControlsDescription(MenuCanvas& canvas,const char* text){
    text=trial_language::Ui(text);
    // Two readable original-font lines, rather than shrinking a paragraph.
    std::string first=text,second;size_t split=first.size();
    while(canvas.Width(first.c_str(),12)>518&&split){
        split=first.rfind(' ',split-1);if(split==std::string::npos){split=0;break;}
        first=std::string(text,split);
    }
    if(split&&split<std::strlen(text))second=text+split+1;
    canvas.Text(first.c_str(),61,337,12,muted,518);canvas.Text(second.c_str(),61,354,12,muted,518);
}
void ControlsUi(const ImGuiViewport* view,bool paused){
    BeginMenu("Burner controls",view);MenuCanvas canvas(view);
    canvas.SettingsFrame(paused,settings_pause_player<0?"CONTROLS":controls_player?"CONTROLS - PLAYER 2":"CONTROLS - PLAYER 1");
    static unsigned focus=0,confirmation=0;static TrialFrontendReleaseGate gate;
    const bool ready=gate.Accept(navigation_generation.load(),UiHeld());
    const unsigned events=frontend_events.exchange(0);
    const bool accept=TakeNativeConfirm(ready)||(ready&&(events&4));
    const bool back=ready&&((events&8)||ImGui::IsKeyPressed(ImGuiKey_Escape,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false)||ImGui::IsMouseClicked(1,false));
    ImGui::BeginDisabled(!ready);
    if(confirm_back||controls_conflict_action>=0){
        const bool conflict=controls_conflict_action>=0;
        canvas.Center(conflict?"This input is already assigned":"Apply your changes?",35,155,570,22,white,true);
        if(conflict){
            char label[160];std::snprintf(label,sizeof(label),trial_language::Ui("Also used by %s."),trial_language::Ui(ControlsName(controls_conflict_action)));
            canvas.Center(label,45,197,550,14,white);
            canvas.Center("Replace clears its other bindings; shared inputs perform both actions.",45,222,550,12,muted);
        }else canvas.Center("Save your controls before going back.",50,204,540,14,white);
        const char* normal[]={"Apply","Discard","Keep editing"};const char* clashes[]={"Replace","Keep shared","Cancel"};
        auto activate=[&](unsigned i){
            if(conflict){if(i<2)ControlsAssign(i==0);else ControlsBack();}
            else if(i==0){if(ControlsApply())ControlsFinish();}
            else if(i==1)ControlsFinish();else ControlsBack();
        };
        if(ready){
            if((events&(1|16))||ImGui::IsKeyPressed(ImGuiKey_LeftArrow))confirmation=(confirmation+2)%3;
            if((events&(2|32))||ImGui::IsKeyPressed(ImGuiKey_RightArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab))confirmation=(confirmation+1)%3;
            if(accept)activate(confirmation);
        }
        RowStrip(canvas,conflict?clashes:normal,nullptr,3,263,confirmation,[&](unsigned i){confirmation=i;},activate);
        if(back)ControlsBack();
    }else if(controls_capture_action>=0){
        canvas.Center(ControlsName(controls_capture_action),40,133,560,23,white,true);
        canvas.Center(controls_device?"Press a controller button or move a stick":"Press a key, mouse button or mouse wheel",40,188,560,16,white);
        canvas.Center(trial_controller::CancelHint(),50,219,540,12,muted);
        // Act on mouse-down here, before consuming a captured left click. This
        // keeps the visible Cancel/Clear targets usable while binding Mouse Left.
        ImGui::EndDisabled();
        for(unsigned i=0;i<2;++i){
            ImGui::PushID(int(i));const float x=150+i*180.f;
            const bool clicked=canvas.Hit("capture choice",x,270,160,27);
            if(ImGui::IsItemHovered())canvas.Selection(x,272,160,23);
            canvas.Center(i?"Cancel":"Clear binding",x,275,160,16);
            if(clicked||ImGui::IsItemActivated()){
                if(i)ControlsBack();else{trial_controls::CancelCapture(controls_player);ControlsCandidate(0);}
            }
            ImGui::PopID();
        }
        if(ImGui::IsKeyPressed(ImGuiKey_Escape,false))ControlsBack();
        if(controls_capture_action>=0){
            if(const unsigned source=trial_controls::TakeCapture(controls_player))ControlsCandidate(source);
            else if(!trial_controls::CaptureWaiting(controls_player)){ControlsCancelCapture();++navigation_generation;}
        }
        ImGui::BeginDisabled(!ready);
    }else{
        const unsigned devices[]={0,1,2};const char* names[]={"Keyboard and mouse","Controller - Player 1","Controller - Player 2"};
        const bool left=ready&&((events&16)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow));
        const bool right=ready&&((events&32)||ImGui::IsKeyPressed(ImGuiKey_RightArrow));
        const unsigned max_scroll=ControlsCount()>8?ControlsCount()-8:0;
        controls_scroll=std::min(controls_scroll,max_scroll);
        const auto mouse=ImGui::GetIO().MousePos;
        const bool over_list=mouse.x>=view->Pos.x+50*canvas.sx&&mouse.x<view->Pos.x+615*canvas.sx&&
            mouse.y>=view->Pos.y+132*canvas.sy&&mouse.y<view->Pos.y+320*canvas.sy;
        if(ready&&over_list&&ImGui::GetIO().MouseWheel!=0)
            controls_scroll=unsigned(std::clamp(int(controls_scroll)-int(std::round(ImGui::GetIO().MouseWheel*3)),0,int(max_scroll)));
        if(ready){
            if((events&1)||ImGui::IsKeyPressed(ImGuiKey_UpArrow)){
                if(focus>=19)focus=15;
                else if(focus>2)focus-=2;
                else if(focus&&controls_scroll)--controls_scroll;
                else focus=0;
            }
            if((events&2)||ImGui::IsKeyPressed(ImGuiKey_DownArrow)){
                if(!focus)focus=1;
                else if(focus<15)focus+=2;
                else if(focus<=16&&controls_scroll<max_scroll)++controls_scroll;
                else focus=19;
            }
            if(ImGui::IsKeyPressed(ImGuiKey_Tab)){
                if(focus>=1&&focus<=16){
                    const unsigned row=(focus-1)/2,action=ControlsAction(controls_scroll+row);
                    if((focus&1)&&ControlsSlots(action)>1&&!ControlsLookSlider(action))++focus;
                    else if(row<7)focus=3+row*2;
                    else if(controls_scroll<max_scroll){++controls_scroll;focus=15;}
                    else focus=19;
                }else focus=focus>=22?0:focus+1;
            }
            if(focus>=1&&focus<=16&&(left||right)&&ControlsLookSlider(ControlsAction(controls_scroll+(focus-1)/2)))ControlsBegin(ControlsAction(controls_scroll+(focus-1)/2),right?1:0);
            else if(focus>=1&&focus<=16&&(left||right))focus=1+((focus-1)/2)*2+(right?1:0);
            else if(focus>=19&&(left||right))focus=19+(focus-19+(left?3:1))%4;
        }
        const unsigned device_before_row=controls_player?2:controls_device;
        unsigned selected_device=device_before_row;
        ImGui::BeginDisabled(settings_pause_player>=0);
        SettingChoice(canvas,"Input device",selected_device,devices,names,0,focus,
            settings_pause_player<0&&focus==0?(left?-1:right||accept?1:0):0);
        ImGui::EndDisabled();
        if(selected_device!=device_before_row){
            const unsigned player=selected_device==2?1:0;
            if(player!=controls_player&&ControlsDirty())message="Apply or discard changes before changing player.";
            else{
                controls_device=selected_device?1:0;
                if(player!=controls_player){controls_player=player;ControlsReadDraft();}
                controls_scroll=0;focus=0;message.clear();
            }
        }

        const unsigned start=controls_scroll,count=std::min(8u,ControlsCount()-start);
        const char* group=ControlsGroup(start+(focus>=1&&focus<=count*2?(focus-1)/2:0));
        canvas.Text(group,61,118,11,muted);canvas.Center("PRIMARY",337,118,115,11,muted);canvas.Center("SECONDARY",465,118,115,11,muted);
        if(ready&&(events&64)&&focus>=1&&focus<=count*2){
            const auto action=ControlsAction(start+(focus-1)/2);
            if(ControlsBindingAction(action)){
                ControlsBinding(action,(focus-1)%2)=0;message="Binding cleared. Apply to save.";
            }
        }
        if(focus>count*2&&focus<=16)focus=19;
        for(unsigned row=0;row<count;++row){
            const unsigned action=ControlsAction(start+row);const float y=138+row*20.f;
            if(ControlsSlots(action)==1&&focus==2+row*2)focus=1+row*2;
            if(ControlsMixedInput(action)){
                const unsigned index=1+row*2,values[]={0,1};const char* names[]={"Off","On"};
                SettingChoice(canvas,"Controller + mouse",controls_draft.mixed_input,values,names,index,focus,
                    focus==index?(left?-1:right||accept?1:0):0,y-index*16,16);
                continue;
            }
            if(ControlsMouseInvert(action)){
                const unsigned index=1+row*2,values[]={0,1};const char* names[]={"Off","On"};
                SettingChoice(canvas,"Invert mouse look",controls_draft.invert_mouse,values,names,index,focus,
                    focus==index?(left?-1:right||accept?1:0):0,y-index*16,16);
                continue;
            }
            if(ControlsWalkMode(action)){
                const unsigned index=1+row*2,values[]={0,1};const char* names[]={"Hold","Toggle"};
                SettingChoice(canvas,"Walking mode",controls_draft.walk_mode,values,names,index,focus,
                    focus==index?(left?-1:right||accept?1:0):0,y-index*16,16);
                continue;
            }
            if(ControlsMovementInertia(action)){
                const unsigned index=1+row*2,values[]={0,1};const char* names[]={"Off","On"};
                SettingChoice(canvas,"Movement inertia",controls_draft.movement_inertia,values,names,index,focus,
                    focus==index?(left?-1:right||accept?1:0):0,y-index*16,16);
                continue;
            }
            if(ControlsAimMode(action)){
                const unsigned index=1+row*2,values[]={0,1};const char* names[]={"Toggle","Hold"};
                SettingChoice(canvas,"Aiming mode",controls_draft.aim_mode,values,names,index,focus,
                    focus==index?(left?-1:right||accept?1:0):0,y-index*16,16);
                continue;
            }
            if(ControlsZoomSensitivity(action)){
                const unsigned index=1+row*2;if(focus==index+1)focus=index;
                SettingSlider(canvas,"Zoom sensitivity",controls_draft.zoom_sensitivity,1,200,1,"%",index,focus,0,y-index*16);
                continue;
            }
            if(ControlsDeadzone(action)){
                const unsigned i=action-ControlsProfileStart()-7,index=1+row*2;
                SettingSlider(canvas,ControlsName(action),controls_draft.deadzone[i],0,31,1,"%",index,focus,
                    focus==index?(left?-1:right?1:0):0,y-index*16,16,1,controls_draft.deadzone[i]==31?"Original":nullptr);
                continue;
            }
            if(ControlsProfile(action)){
                const unsigned i=action-ControlsProfileStart(),index=1+row*2;
                if(focus==index+1)focus=index;
                if(i>=5)SettingSlider(canvas,profile_names[i],profile_draft[i],0,100,5,"%",index,focus,focus==index?(left?-1:right?1:0):0,y-index*16,16,2);
                else{
                    const unsigned values[]={0,1};
                    const char* toggles[]={"Off","On"};const char* crouch[]={"Toggle","Hold"};const char* sticks[]={"Legacy","Standard"};
                    SettingChoice(canvas,profile_names[i],profile_draft[i],values,i==3?crouch:i==4?sticks:toggles,index,focus,
                        focus==index?(left?-1:right||accept?1:0):0,y-index*16,16);
                }
                continue;
            }
            if(ControlsMouseSensitivity(action)){
                const unsigned index=1+row*2;if(focus==index+1)focus=index;
                SettingSlider(canvas,"Mouse sensitivity",controls_draft.sensitivity,1,1600,1,"%",index,focus,0,y-index*16,16,.25f);
                continue;
            }
            if(focus==1+row*2||focus==2+row*2)canvas.Selection(50,y,540,20);
            canvas.Text(ControlsName(action),61,y+3,12,white,264);
            for(unsigned slot=0;slot<ControlsSlots(action);++slot){
                const unsigned index=1+row*2+slot;const float x=slot?465:337;ImGui::PushID(int(index));
                if(canvas.Hit("binding",x,y,115,20)||(accept&&focus==index))ControlsBegin(action,slot);
                if(ImGui::IsItemHovered()&&PointerMoved())focus=index;
                const unsigned source=ControlsBinding(action,slot);
                canvas.Center(action==ControlsBase()?(slot?"Change":controls_draft.spin_mode?"Button press":"Original gesture"):controls_device?trial_controller::Label(source):xbox_KeyboardSourceName(source),x+4,y+3,107,12,focus==index?white:muted);
                ImGui::PopID();
            }
        }
        // The same continuous list is scrolled by wheel, thumb or pad focus.
        const float track_top=138,track_height=160,thumb_height=track_height*count/ControlsCount();
        canvas.Rect(600,track_top,5,track_height,IM_COL32(80,90,85,140));
        canvas.Hit("controls scrollbar",594,track_top,17,track_height);
        if(ImGui::IsItemActive()&&ImGui::IsMouseDown(0)){
            const float y=(ImGui::GetIO().MousePos.y-canvas.origin.y)/canvas.sy;
            controls_scroll=unsigned(std::clamp(int(std::round((y-track_top-thumb_height/2)*max_scroll/(track_height-thumb_height))),0,int(max_scroll)));
        }
        const float thumb_top=track_top+(track_height-thumb_height)*controls_scroll/std::max(1u,max_scroll);
        canvas.Rect(597,thumb_top,11,thumb_height,IM_COL32(173,184,177,210));
        const unsigned selected=focus>=1&&focus<=count*2?ControlsAction(start+(focus-1)/2):ControlsAction(start);
        ControlsDescription(canvas,focus==0?"Choose a device. Each action accepts two inputs; menu navigation keeps its standard controls.":
            ControlsHelp(selected));
        const char* labels[]={"Apply","Discard changes","Defaults","Back"};
        auto activate=[&](unsigned i){
            if(i==0)ControlsApply();
            else if(i==1){ControlsReadDraft();message="Changes discarded.";}
            else if(i==2){
                const auto defaults=trial_controls::Defaults();
                if(controls_device){std::memcpy(controls_draft.controller,defaults.controller,sizeof(defaults.controller));std::memcpy(controls_draft.deadzone,defaults.deadzone,sizeof(defaults.deadzone));}
                else{std::memcpy(controls_draft.keyboard,defaults.keyboard,sizeof(defaults.keyboard));std::memcpy(controls_draft.previous_firearm,defaults.previous_firearm,sizeof(defaults.previous_firearm));controls_draft.sensitivity=defaults.sensitivity;controls_draft.invert_mouse=defaults.invert_mouse;}
                controls_draft.spin_mode=0;std::fill(std::begin(controls_draft.spin_binding[controls_device]),std::end(controls_draft.spin_binding[controls_device]),0u);
                controls_draft.zoom_sensitivity=defaults.zoom_sensitivity;
                controls_draft.walk_mode=defaults.walk_mode;
                std::memcpy(controls_draft.walk_binding[controls_device],defaults.walk_binding[controls_device],sizeof(defaults.walk_binding[controls_device]));
                std::memcpy(controls_draft.hud_binding[controls_device],defaults.hud_binding[controls_device],sizeof(defaults.hud_binding[controls_device]));
                controls_draft.movement_inertia=defaults.movement_inertia;
                if(!controls_player)controls_draft.mixed_input=defaults.mixed_input;
                controls_draft.aim_mode=0;std::fill(std::begin(controls_draft.scope_zoom[controls_device]),std::end(controls_draft.scope_zoom[controls_device]),0u);
                std::memcpy(controls_draft.health_pack[controls_device],defaults.health_pack[controls_device],sizeof(defaults.health_pack[controls_device]));
                if(!controls_device)std::memcpy(controls_draft.heal_partner,defaults.heal_partner,sizeof(defaults.heal_partner));
                if(controls_device)std::memcpy(profile_draft,defaults.native_profile,sizeof(profile_draft));
                else profile_draft[3]=defaults.native_profile[3];
                message="Defaults selected. Apply to save.";
            }else ControlsBack();
        };
        if(accept&&focus>=19)activate(focus-19);
        RowStrip(canvas,labels,nullptr,4,395,focus>=19?focus-19:4,[&](unsigned i){focus=19+i;},activate);
        if(back)ControlsBack();
    }
    canvas.Center(!message.empty()?message.c_str():ControlsDirty()?"Changes not applied.":"",30,429,580,11,IM_COL32(207,217,219,255));
    if(controls_capture_action>=0){
        if(xbox_KeyboardMode())FooterPrompts(canvas,{{VK_ESCAPE,"Cancel"}});
        else canvas.Center("START + BACK TO CANCEL",40,452,560,12,muted);
    }
    else if(xbox_KeyboardMode())FooterPrompts(canvas,{{13,"Select"},{VK_RBUTTON,confirm_back?"Keep editing":"Back"}});
    else FooterPrompts(canvas,{{0x103,"Navigate"},{0x101,"Select"},{0x104,"Clear binding"},{0x102,"Back"}});
    ImGui::EndDisabled();EndMenu();
}

void ViewControlsUi(const TrialFrontendFrame& f,const ImGuiViewport* view,bool interactive=true){
    static TrialFrontendReleaseGate gate;
    static uint64_t epoch=0;
    static unsigned device=0,focus=0,scroll=0;
    const int owner=trial_coop::PauseOwner();
    if(epoch!=f.source.epoch){
        epoch=f.source.epoch;device=owner>=0?(owner?2u:0u):xbox_KeyboardMode()?0u:1u;
        focus=scroll=0;
    }
    const bool ready=interactive&&gate.Accept(navigation_generation.load(),UiHeld());
    const unsigned events=interactive?frontend_events.exchange(0):0;
    const bool accept=TakeNativeConfirm(ready)||(ready&&(events&4));
    const bool back=ready&&((events&8)||ImGui::IsMouseClicked(1,false)||
        ImGui::IsKeyPressed(ImGuiKey_Escape,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false));
    BeginMenu("View controls",view);MenuCanvas c(view);
    c.SettingsFrame((f.source.source_kind&0x80000000u)!=0,"VIEW CONTROLS");
    ImGui::BeginDisabled(!ready);
    const unsigned devices[]={0,1,2};
    const char* names[]={"Keyboard and mouse","Controller - Player 1","Controller - Player 2"};
    const unsigned previous=device;
    const bool left=ready&&((events&16)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow));
    const bool right=ready&&((events&32)||ImGui::IsKeyPressed(ImGuiKey_RightArrow));
    ImGui::BeginDisabled(owner>=0);
    SettingChoice(c,"Input device",device,devices,names,0,focus,
        owner<0&&focus==0?(left?-1:right||accept?1:0):0);
    ImGui::EndDisabled();
    if(device!=previous){scroll=0;focus=0;}
    // The reference page and editor share action IDs; only the editor saves drafts.
    controls_player=device==2?1:0;controls_device=device?1:0;
    controls_draft=trial_controls::Snapshot(controls_player);
    unsigned rows[43]{},count=0;
    for(unsigned row=0;row<ControlsCount();++row)
        if(ControlsBindingAction(ControlsAction(row)))rows[count++]=row;
    const unsigned visible=std::min(count,8u),maximum=count-visible;
    scroll=std::min(scroll,maximum);focus=std::min(focus,count);
    auto reveal=[&](){if(focus){scroll=std::min(scroll,focus-1);if(focus>scroll+visible)scroll=focus-visible;}};
    if(ready){
        if((events&1)||ImGui::IsKeyPressed(ImGuiKey_UpArrow)){if(focus)--focus;reveal();}
        if((events&2)||ImGui::IsKeyPressed(ImGuiKey_DownArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab)){focus=std::min(focus+1,count);reveal();}
        const auto p=ImGui::GetIO().MousePos,a=c.Point(50,132),b=c.Point(615,318);
        if(p.x>=a.x&&p.x<b.x&&p.y>=a.y&&p.y<b.y)
            scroll=unsigned(std::clamp(int(scroll)-int(std::round(ImGui::GetIO().MouseWheel*3)),0,int(maximum)));
        if(back||(accept&&focus))FrontendAction(f,UC_HF_BACK);
    }
    if(focus)focus=std::clamp(focus,scroll+1,scroll+visible);
    c.Text(ControlsGroup(rows[focus?focus-1:scroll]),61,118,11,muted);
    c.Center("PRIMARY",337,118,115,11,muted);c.Center("SECONDARY",465,118,115,11,muted);
    for(unsigned i=0;i<visible;++i){
        const unsigned index=scroll+i,action=ControlsAction(rows[index]);const float y=138+i*20.f;
        ImGui::PushID(int(index));
        if(c.Hit("view control",50,y,540,20)||(ImGui::IsItemHovered()&&PointerMoved()))focus=index+1;
        if(focus==index+1)c.Selection(50,y,540,20);
        c.Text(ControlsName(action),61,y+3,12,focus==index+1?white:muted,264);
        for(unsigned slot=0;slot<ControlsSlots(action);++slot){
            const unsigned source=ControlsBinding(action,slot);
            const bool heal=controls_device&&ControlsHealthPack(action)&&!slot&&(source==14||(!source&&!ControlsBinding(action,1)));
            const char* label=heal?"Down x2 / hold":controls_device?trial_controller::Label(source):xbox_KeyboardSourceName(source);
            c.Center(label,slot?469:341,y+3,107,12,focus==index+1?white:muted);
        }
        ImGui::PopID();
    }
    const float top=138,height=160,thumb=height*visible/count;
    c.Rect(600,top,5,height,IM_COL32(80,90,85,140));
    c.Hit("view controls scrollbar",594,top,17,height);
    if(ImGui::IsItemActive()&&ImGui::IsMouseDown(0)){
        const float y=(ImGui::GetIO().MousePos.y-c.origin.y)/c.sy;
        scroll=unsigned(std::clamp(int(std::round((y-top-thumb/2)*maximum/(height-thumb))),0,int(maximum)));
    }
    c.Rect(597,top+(height-thumb)*scroll/std::max(1u,maximum),11,thumb,IM_COL32(173,184,177,210));
    ControlsDescription(c,focus?ControlsHelp(ControlsAction(rows[focus-1])):"Change bindings in Options > Controls.");
    const char* labels[]={"Scroll up","Back","Scroll down"};
    const bool enabled[]={scroll>0,true,scroll<maximum};
    RowStrip(c,labels,enabled,3,398,3,[](unsigned){},[&](unsigned i){
        if(i==1)FrontendAction(f,UC_HF_BACK);
        else if(i==0&&scroll)--scroll;else if(i==2&&scroll<maximum)++scroll;
    });
    if(xbox_KeyboardMode())FooterPrompts(c,{{38,"Scroll up"},{40,"Scroll down"},{VK_RBUTTON,"Back"}});
    else FooterPrompts(c,{{0x103,"Navigate"},{0x102,"Back"}});
    ImGui::EndDisabled();EndMenu();
}
