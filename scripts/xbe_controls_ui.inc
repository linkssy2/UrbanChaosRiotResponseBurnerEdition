// Same original frame/fonts/selection strips as Graphics. Only drafts live
// here: the input backend publishes a complete profile after a successful save.
trial_controls::Draft controls_draft{};
unsigned controls_device=0,controls_scroll=0,controls_slot=0;
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
unsigned ControlsProfileStart(){return ControlsBase()+(controls_device?2:5);}
unsigned ControlsCount(){return ControlsProfileStart()+7;}
bool ControlsProfile(unsigned action){return action>=ControlsProfileStart();}
const char* profile_names[]={"Vibration","Invert stick look","Auto-centre","Crouch mode","Thumbstick layout","Horizontal stick sensitivity","Vertical stick sensitivity"};
const char* profile_help[]={"Controller vibration.","Reverse vertical stick aiming; raw mouse aiming is unchanged.","Use the original automatic camera centring.","Choose hold or toggle for crouching.","Standard or Legacy movement and aiming sticks.","Original horizontal stick sensitivity; mouse sensitivity is separate.","Original vertical stick sensitivity; mouse sensitivity is separate."};
unsigned ControlsAction(unsigned row){
    static constexpr unsigned keyboard[]={0,28,7,11,6,1,2,29,3,4,5,25,26,18,19,16,17,10,27,22,23,20,21,24,12,13,14,15,8,9};
    static constexpr unsigned controller[]={0,7,11,6,1,2,3,4,5,24,25,18,19,16,17,10,22,23,20,21,12,13,14,15,8,9};
    return row<ControlsProfileStart()?(controls_device?controller[row]:keyboard[row]):row;
}
const char* ControlsGroup(unsigned row){
    if(row>=ControlsProfileStart())return "CONTROLLER SETTINGS";
    if(row>=(controls_device?24u:28u))return "MENU CONTROLS";
    if(row>=(controls_device?20u:23u))return "TEAMMATE ORDERS";
    if(row>=(controls_device?11u:13u))return "MOVEMENT / AIMING";
    return "COMBAT / INTERACTION";
}
bool ControlsInteract(unsigned action){return !controls_device&&action==ControlsBase()+3;}
bool ControlsPreviousFirearm(unsigned action){return !controls_device&&action==ControlsBase()+4;}
unsigned ControlsSlots(unsigned action){if(ControlsProfile(action))return 1;return !controls_device&&(action==0||ControlsInteract(action))?1:2;}
bool ControlsMouseSensitivity(unsigned action);
const char* ControlsName(unsigned action){if(ControlsProfile(action))return profile_names[action-ControlsProfileStart()];if(ControlsInteract(action))return "Interact / dialogue skip";if(ControlsPreviousFirearm(action))return "Previous Firearm";if(!controls_device&&action==2)return "Next Firearm";if(!controls_device&&action==0)return "Reload";if(ControlsMouseSensitivity(action))return "Mouse sensitivity";return action<ControlsBase()?control_names[action]:action==ControlsBase()?"Pistol spin mode":"Pistol spin button";}
const char* ControlsHelp(unsigned action){if(ControlsProfile(action))return profile_help[action-ControlsProfileStart()];if(ControlsInteract(action))return "Use nearby objects and skip dialogue. This key is displayed in interaction prompts.";if(ControlsPreviousFirearm(action))return "Equip or select the previous available firearm.";if(!controls_device&&action==2)return "Equip or select the next available firearm.";if(!controls_device&&action==0)return "Reload your weapon without activating nearby objects.";if(ControlsMouseSensitivity(action))return "Raw mouse sensitivity: 1 to 1600%. Lower is slower; independent of Windows pointer speed and stick sensitivity.";return action<ControlsBase()?control_help[action]:action==ControlsBase()?"Original gesture: rotate the stick or move the mouse in two full circles. Button press: use your assigned input.":"Used in Button press mode. Runs the original pistol spin and next-kill slow-motion reward.";}
bool ControlsMouseSensitivity(unsigned action){return !controls_device&&action==ControlsBase()+2;}
unsigned& ControlsBinding(unsigned action,unsigned slot){
    // Keep existing profiles and prompt consumers compatible with the two retail-action slots.
    if(ControlsInteract(action))return controls_draft.keyboard[0][1];
    if(ControlsPreviousFirearm(action))return controls_draft.previous_firearm[slot];
    if(action>=ControlsBase())return controls_draft.spin_binding[controls_device][slot];
    return controls_device?controls_draft.controller[action][slot]:controls_draft.keyboard[action][slot];
}
void ControlsCancelCapture(){
    trial_controls::CancelCapture();controls_capture_action=controls_conflict_action=-1;controls_candidate=0;
}
void ControlsReadDraft(){
    ControlsCancelCapture();controls_draft=trial_controls::Snapshot();controls_scroll=0;
    std::lock_guard lock(context_lock);std::memcpy(profile_draft,profile_live,sizeof(profile_draft));
}
bool ControlsDirty(){const auto live=trial_controls::Snapshot();std::lock_guard lock(context_lock);return std::memcmp(&controls_draft,&live,sizeof(live))!=0||std::memcmp(profile_draft,profile_live,sizeof(profile_draft))!=0;}
bool ControlsApply(){
    {std::lock_guard lock(context_lock);if(!profile_valid||!SameMenu(context,opened)){message="Controls context changed. Reopen Controls.";return false;}}
    if(!trial_controls::Apply(controls_draft,message))return false;
    {std::lock_guard lock(context_lock);
        if(!profile_valid||!SameMenu(context,opened)){message="Bindings saved; profile changed. Reopen Controls.";return false;}
        std::memcpy(profile_expected,profile_live,sizeof(profile_expected));
        std::memcpy(profile_pending,profile_draft,sizeof(profile_pending));profile_origin=context;profile_apply=true;}
    controls_draft=trial_controls::Snapshot();message="Controls applied and saved.";TrialGraphicsInputChanged();return true;
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
    if(replace)for(unsigned i=0;i<ControlsCount();++i)for(unsigned slot=0;slot<ControlsSlots(i);++slot)
        if(i!=ControlsBase()&&!ControlsMouseSensitivity(i)&&!ControlsProfile(i)&&(i!=action||slot!=controls_slot)&&ControlsOverlap(ControlsBinding(i,slot),controls_candidate))ControlsBinding(i,slot)=0;
    ControlsBinding(action,controls_slot)=controls_candidate;
    ControlsCancelCapture();message="Binding changed. Select Apply to save.";
    ++navigation_generation;frontend_events=0;
}
void ControlsCandidate(unsigned source){
    if(controls_capture_action<0)return;
    if(!controls_device&&(source>257||source==VK_ESCAPE||source==VK_LWIN||source==VK_RWIN||
       ((controls_capture_action==24||unsigned(controls_capture_action)==ControlsBase()+1)&&source>=256))){
        message=controls_capture_action==24&&source>=256?"Mouse orders needs a button or key that can be held.":"That key is reserved. Choose another input.";
        trial_controls::BeginCapture(controls_device);return;
    }
    controls_candidate=source;controls_conflict_action=-1;
    if(source)for(unsigned action=0;action<ControlsCount();++action)if(action!=ControlsBase()&&!ControlsMouseSensitivity(action)&&!ControlsProfile(action)&&action!=unsigned(controls_capture_action))
        for(unsigned slot=0;slot<ControlsSlots(action);++slot)if(ControlsOverlap(source,ControlsBinding(action,slot)))controls_conflict_action=int(action);
    if(controls_conflict_action<0)ControlsAssign(false);
    else{++navigation_generation;frontend_events=0;}
}
void ControlsBegin(unsigned action,unsigned slot){
    if(ControlsProfile(action))return;
    if(slot>=ControlsSlots(action))slot=0;
    if(ControlsMouseSensitivity(action)){
        controls_draft.sensitivity=unsigned(std::clamp(int(controls_draft.sensitivity)+(slot?1:-1),1,1600));
        message="Mouse sensitivity changed. Apply to save.";return;
    }
    if(action==ControlsBase()){controls_draft.spin_mode^=1;message="Pistol spin mode changed. Apply to save.";return;}
    controls_capture_action=int(action);controls_slot=slot;controls_conflict_action=-1;controls_candidate=0;message.clear();
    trial_controls::BeginCapture(controls_device);++navigation_generation;frontend_events=0;
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
    BeginMenu("Burner controls",view);MenuCanvas canvas(view);canvas.SettingsFrame(paused,"CONTROLS");
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
                if(i)ControlsBack();else{trial_controls::CancelCapture();ControlsCandidate(0);}
            }
            ImGui::PopID();
        }
        if(ImGui::IsKeyPressed(ImGuiKey_Escape,false))ControlsBack();
        if(controls_capture_action>=0){
            if(const unsigned source=trial_controls::TakeCapture())ControlsCandidate(source);
            else if(!trial_controls::CaptureWaiting()){ControlsCancelCapture();++navigation_generation;}
        }
        ImGui::BeginDisabled(!ready);
    }else{
        const unsigned devices[]={0,1};const char* names[]={"Keyboard and mouse","Controller"};
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
                if(focus>=1&&focus<=16&&ControlsMouseSensitivity(ControlsAction(controls_scroll+(focus-1)/2)))focus=19;
                else if(focus==16){if(controls_scroll<max_scroll){++controls_scroll;focus=15;}else focus=19;}
                else focus=focus>=22?0:focus+1;
            }
            if(focus>=1&&focus<=16&&(left||right)&&ControlsMouseSensitivity(ControlsAction(controls_scroll+(focus-1)/2)))ControlsBegin(ControlsAction(controls_scroll+(focus-1)/2),right?1:0);
            else if(focus>=1&&focus<=16&&(left||right))focus=1+((focus-1)/2)*2+(right?1:0);
            else if(focus>=19&&(left||right))focus=19+(focus-19+(left?3:1))%4;
        }
        const unsigned device_before_row=controls_device;
        SettingChoice(canvas,"Input device",controls_device,devices,names,0,focus,focus==0?(left?-1:right||accept?1:0):0);
        if(controls_device!=device_before_row){controls_scroll=0;focus=0;message.clear();}

        canvas.Text(ControlsGroup(controls_scroll),61,118,11,muted);canvas.Center("PRIMARY",337,118,115,11,muted);canvas.Center("SECONDARY",465,118,115,11,muted);
        const unsigned start=controls_scroll,count=std::min(8u,ControlsCount()-start);
        if(ready&&(events&64)&&focus>=1&&focus<=count*2){
            const auto action=ControlsAction(start+(focus-1)/2);
            if(action!=ControlsBase()&&!ControlsMouseSensitivity(action)&&!ControlsProfile(action)){
                ControlsBinding(action,(focus-1)%2)=0;message="Binding cleared. Apply to save.";
            }
        }
        if(focus>count*2&&focus<=16)focus=19;
        for(unsigned row=0;row<count;++row){
            const unsigned action=ControlsAction(start+row);const float y=138+row*20.f;
            if(ControlsSlots(action)==1&&focus==2+row*2)focus=1+row*2;
            if(ControlsProfile(action)){
                const unsigned i=action-ControlsProfileStart(),index=1+row*2;
                if(focus==index+1)focus=index;
                if(i>=5)SettingSlider(canvas,profile_names[i],profile_draft[i],0,100,5,"%",index,focus,focus==index?(left?-1:right?1:0):0,y-index*16);
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
                SettingSlider(canvas,"Mouse sensitivity",controls_draft.sensitivity,1,1600,1,"%",index,focus,0,y-index*16);
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
            const float y=(ImGui::GetIO().MousePos.y-view->Pos.y)/canvas.sy;
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
                else{std::memcpy(controls_draft.keyboard,defaults.keyboard,sizeof(defaults.keyboard));std::memcpy(controls_draft.previous_firearm,defaults.previous_firearm,sizeof(defaults.previous_firearm));controls_draft.sensitivity=defaults.sensitivity;}
                controls_draft.spin_mode=0;std::fill(std::begin(controls_draft.spin_binding[controls_device]),std::end(controls_draft.spin_binding[controls_device]),0u);
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
