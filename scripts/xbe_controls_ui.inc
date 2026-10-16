// Same original frame/fonts/selection strips as Graphics. Only drafts live
// here: the input backend publishes a complete profile after a successful save.
trial_controls::Draft controls_draft{};
unsigned controls_device=0,controls_page=0,controls_slot=0;
int controls_capture_action=-1,controls_conflict_action=-1;
unsigned controls_candidate=0;
bool controls_original_requested=false;
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
    "Left stick left: moves left with Standard thumbsticks. Game options controls the Legacy layout.",
    "Left stick right: moves right with Standard thumbsticks. Game options controls the Legacy layout.",
    "Move forward. Opposite directions cancel each other.",
    "Move backward. Opposite directions cancel each other.",
    "Right stick left: turns left with Standard thumbsticks. Mouse movement also aims during gameplay.",
    "Right stick right: turns right with Standard thumbsticks. Mouse movement also aims during gameplay.",
    "Look up. The game's invert-look setting can reverse this direction.",
    "Look down. The game's invert-look setting can reverse this direction.",
    "Hold and drag toward a teammate order or help option, then release. Mouse wheel cannot be held."};

unsigned ControlsCount(){return controls_device?24:25;}
unsigned& ControlsBinding(unsigned action,unsigned slot){
    return controls_device?controls_draft.controller[action][slot]:controls_draft.keyboard[action][slot];
}
void ControlsCancelCapture(){
    trial_controls::CancelCapture();controls_capture_action=controls_conflict_action=-1;controls_candidate=0;
}
void ControlsReadDraft(){
    ControlsCancelCapture();controls_draft=trial_controls::Snapshot();controls_page=0;controls_original_requested=false;
}
bool ControlsDirty(){const auto live=trial_controls::Snapshot();return std::memcmp(&controls_draft,&live,sizeof(live))!=0;}
bool ControlsApply(){
    if(!trial_controls::Apply(controls_draft,message))return false;
    controls_draft=trial_controls::Snapshot();message="Controls applied and saved.";TrialGraphicsInputChanged();return true;
}
void ControlsFinish(){
    const bool original=controls_original_requested;controls_original_requested=false;Close();
    if(original)FrontendAction(controls_origin,UC_HF_ACTIVATE,controls_origin_row);
}
void ControlsBack(bool original=false){
    if(controls_capture_action>=0||controls_conflict_action>=0){ControlsCancelCapture();}
    else if(confirm_back){confirm_back=false;controls_original_requested=false;}
    else if(ControlsDirty()){confirm_back=true;controls_original_requested=original;}
    else{controls_original_requested=original;ControlsFinish();return;}
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
    if(replace)for(unsigned i=0;i<ControlsCount();++i)for(unsigned slot=0;slot<2;++slot)
        if((i!=action||slot!=controls_slot)&&ControlsOverlap(ControlsBinding(i,slot),controls_candidate))ControlsBinding(i,slot)=0;
    ControlsBinding(action,controls_slot)=controls_candidate;
    ControlsCancelCapture();message="Binding changed. Select Apply to save.";
    ++navigation_generation;frontend_events=0;
}
void ControlsCandidate(unsigned source){
    if(controls_capture_action<0)return;
    if(!controls_device&&(source>257||source==VK_ESCAPE||source==VK_LWIN||source==VK_RWIN||
       (controls_capture_action==24&&source>=256))){
        message=controls_capture_action==24&&source>=256?"Mouse orders needs a button or key that can be held.":"That key is reserved. Choose another input.";
        trial_controls::BeginCapture(controls_device);return;
    }
    controls_candidate=source;controls_conflict_action=-1;
    if(source)for(unsigned action=0;action<ControlsCount();++action)if(action!=unsigned(controls_capture_action))
        for(unsigned slot=0;slot<2;++slot)if(ControlsOverlap(source,ControlsBinding(action,slot)))controls_conflict_action=int(action);
    if(controls_conflict_action<0)ControlsAssign(false);
    else{++navigation_generation;frontend_events=0;}
}
void ControlsBegin(unsigned action,unsigned slot){
    controls_capture_action=int(action);controls_slot=slot;controls_conflict_action=-1;controls_candidate=0;message.clear();
    trial_controls::BeginCapture(controls_device);++navigation_generation;frontend_events=0;
}
void ControlsDescription(MenuCanvas& canvas,const char* text){
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
    const bool accept=ready&&((events&4)||ImGui::IsKeyPressed(ImGuiKey_Enter,false)||ImGui::IsKeyPressed(ImGuiKey_Space,false));
    const bool back=ready&&((events&8)||ImGui::IsKeyPressed(ImGuiKey_Escape,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false)||ImGui::IsMouseClicked(1,false));
    ImGui::BeginDisabled(!ready);
    if(confirm_back||controls_conflict_action>=0){
        const bool conflict=controls_conflict_action>=0;
        canvas.Center(conflict?"This input is already assigned":"Apply your changes?",35,155,570,22,white,true);
        if(conflict){
            char label[160];std::snprintf(label,sizeof(label),"Also used by %s.",control_names[controls_conflict_action]);
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
        canvas.Center(control_names[controls_capture_action],40,133,560,23,white,true);
        canvas.Center(controls_device?"Press a controller button or move a stick":"Press a key, mouse button or mouse wheel",40,188,560,16,white);
        canvas.Center("Release the opening input first. Escape cancels.",50,219,540,12,muted);
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
        if(ready){
            if((events&1)||ImGui::IsKeyPressed(ImGuiKey_UpArrow))focus=focus<=2?0:focus<=16?focus-2:focus-1;
            if((events&2)||ImGui::IsKeyPressed(ImGuiKey_DownArrow))focus=focus==0?1:focus<15?focus+2:std::min(23u,focus+1);
            if(ImGui::IsKeyPressed(ImGuiKey_Tab))focus=(focus+1)%24;
            if(focus>=1&&focus<=16&&(left||right))focus=1+((focus-1)/2)*2+(right?1:0);
            else if(focus>=17&&focus<=18&&(left||right))focus=left?17:18;
            else if(focus>=19&&(left||right))focus=19+(focus-19+(left?4:1))%5;
        }
        const unsigned device_before_row=controls_device;
        SettingChoice(canvas,"Input device",controls_device,devices,names,0,focus,focus==0?(left?-1:right||accept?1:0):0);
        if(controls_device!=device_before_row){controls_page=0;focus=0;message.clear();}
        const unsigned pages=(ControlsCount()+7)/8;controls_page=std::min(controls_page,pages-1);
        canvas.Text("ACTION",61,118,11,muted);canvas.Center("PRIMARY",337,118,115,11,muted);canvas.Center("SECONDARY",465,118,115,11,muted);
        const unsigned start=controls_page*8,count=std::min(8u,ControlsCount()-start);
        if(focus>count*2&&focus<=16)focus=17;
        for(unsigned row=0;row<count;++row){
            const unsigned action=start+row;const float y=138+row*20.f;
            if(focus==1+row*2||focus==2+row*2)canvas.Selection(50,y,540,20);
            canvas.Text(control_names[action],61,y+3,12,white,264);
            for(unsigned slot=0;slot<2;++slot){
                const unsigned index=1+row*2+slot;const float x=slot?465:337;ImGui::PushID(int(index));
                if(canvas.Hit("binding",x,y,115,20)||(accept&&focus==index))ControlsBegin(action,slot);
                if(ImGui::IsItemHovered()&&PointerMoved())focus=index;
                const unsigned source=ControlsBinding(action,slot);
                canvas.Center(controls_device?xbox_ControllerSourceName(source):xbox_KeyboardSourceName(source),x+4,y+3,107,12,focus==index?white:muted);
                ImGui::PopID();
            }
        }
        auto page=[&](int delta){controls_page=(controls_page+pages+delta)%pages;focus=delta<0?17:18;message.clear();};
        if(canvas.Hit("previous controls",60,308,110,20)||(accept&&focus==17))page(-1);
        if(ImGui::IsItemHovered()&&PointerMoved())focus=17;
        canvas.Text("Previous",70,312,12,focus==17?white:muted);
        char caption[32];std::snprintf(caption,sizeof(caption),"Page %u / %u",controls_page+1,pages);canvas.Center(caption,240,312,160,12,muted);
        if(canvas.Hit("next controls",470,308,110,20)||(accept&&focus==18))page(1);
        if(ImGui::IsItemHovered()&&PointerMoved())focus=18;
        canvas.Text("Next",490,312,12,focus==18?white:muted);
        const unsigned selected=focus>=1&&focus<=count*2?start+(focus-1)/2:start;
        ControlsDescription(canvas,focus==0?"Choose a device. Each action accepts two inputs; menu navigation keeps its standard controls.":
            focus==23?"Original vibration, invert look, auto-centre, crouch, Standard/Legacy thumbsticks and stick sensitivity.":control_help[selected]);
        const char* labels[]={"Apply","Discard changes","Defaults","Back","Game options"};
        auto activate=[&](unsigned i){
            if(i==0)ControlsApply();
            else if(i==1){ControlsReadDraft();message="Changes discarded.";}
            else if(i==2){
                const auto defaults=trial_controls::Defaults();
                if(controls_device){std::memcpy(controls_draft.controller,defaults.controller,sizeof(defaults.controller));std::memcpy(controls_draft.deadzone,defaults.deadzone,sizeof(defaults.deadzone));}
                else{std::memcpy(controls_draft.keyboard,defaults.keyboard,sizeof(defaults.keyboard));controls_draft.sensitivity=defaults.sensitivity;}
                message="Defaults selected. Apply to save.";
            }else ControlsBack(i==4);
        };
        if(accept&&focus>=19)activate(focus-19);
        RowStrip(canvas,labels,nullptr,5,395,focus>=19?focus-19:5,[&](unsigned i){focus=19+i;},activate);
        if(back)ControlsBack();
    }
    canvas.Center(!message.empty()?message.c_str():ControlsDirty()?"Changes not applied.":"",30,429,580,11,IM_COL32(207,217,219,255));
    if(controls_capture_action>=0)FooterPrompts(canvas,{{VK_ESCAPE,"Cancel"}});
    else if(xbox_KeyboardMode())FooterPrompts(canvas,{{13,"Select"},{VK_RBUTTON,confirm_back?"Keep editing":"Back"}});
    else FooterPrompts(canvas,{{0x103,"Navigate"},{0x101,"Select"},{0x102,"Back"}});
    ImGui::EndDisabled();EndMenu();
}
