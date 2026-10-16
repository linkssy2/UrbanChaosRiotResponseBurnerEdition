// This draft changes the next startup only; original loaded banks stay owned
// by the game until exit. No profile or save data is rewritten here.
uint32_t language_draft=0;
void LanguageReadDraft(){language_draft=trial_language::Snapshot();}
bool LanguageDirty(){return language_draft!=trial_language::Snapshot();}
bool LanguageApply(){
    if(!trial_language::Apply(language_draft,message))return false;
    LanguageReadDraft();
    message=trial_language::PendingRestart()?"Language saved. Restart the game to use it.":"Language applied and saved.";
    TrialGraphicsInputChanged();return true;
}
void LanguageBack(){
    if(confirm_back)confirm_back=false;
    else if(LanguageDirty())confirm_back=true;
    else{Close();return;}
    ++navigation_generation;frontend_events=0;
}
void LanguageUi(const ImGuiViewport* view,bool paused){
    BeginMenu("Burner language",view);MenuCanvas canvas(view);canvas.SettingsFrame(paused,"LANGUAGE");
    static TrialFrontendReleaseGate gate;static uint64_t epoch=0;
    static unsigned focus=0,confirmation=2;
    const auto generation=navigation_generation.load();
    if(epoch!=generation){epoch=generation;focus=0;confirmation=2;}
    const bool ready=gate.Accept(generation,UiHeld());
    const unsigned events=frontend_events.exchange(0);
    const bool accept=ready&&((events&4)||ImGui::IsKeyPressed(ImGuiKey_Enter,false)||ImGui::IsKeyPressed(ImGuiKey_Space,false));
    const bool back=ready&&((events&8)||ImGui::IsKeyPressed(ImGuiKey_Escape,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false)||ImGui::IsMouseClicked(1,false));
    ImGui::BeginDisabled(!ready);
    if(confirm_back){
        canvas.Center("Apply your changes?",40,163,560,24,white,true);
        canvas.Center("Save the language for the next time you start the game.",40,204,560,14,white);
        const char* labels[]={"Apply","Discard","Keep editing"};
        auto activate=[&](unsigned i){if(i==0){if(LanguageApply())Close();}else if(i==1)Close();else LanguageBack();};
        if(ready){
            if((events&(1|16))||ImGui::IsKeyPressed(ImGuiKey_LeftArrow))confirmation=(confirmation+2)%3;
            if((events&(2|32))||ImGui::IsKeyPressed(ImGuiKey_RightArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab))confirmation=(confirmation+1)%3;
            if(accept)activate(confirmation);
        }
        RowStrip(canvas,labels,nullptr,3,257,confirmation,[&](unsigned i){confirmation=i;},activate);
    }else{
        int delta=0;
        if(ready){
            if((events&1)||ImGui::IsKeyPressed(ImGuiKey_UpArrow))focus=(focus+3)%4;
            if((events&2)||ImGui::IsKeyPressed(ImGuiKey_DownArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab))focus=(focus+1)%4;
            const bool left=(events&16)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow);
            const bool right=(events&32)||ImGui::IsKeyPressed(ImGuiKey_RightArrow);
            if(focus==0)delta=left?-1:right||accept?1:0;
            else if(left||right)focus=1+(focus-1+(left?2:1))%3;
        }
        const int change=SettingRow(canvas,"Original game language",trial_language::Names[language_draft],0,focus,delta);
        if(change)language_draft=(language_draft+trial_language::Count+(change>0?1:-1))%trial_language::Count;
        canvas.Text("Uses the original language files supplied by your game.",61,144,14,white,518);
        canvas.Text("Restart the game after applying a different language.",61,174,14,white,518);
        canvas.Text("PC-specific settings and controls descriptions remain in English.",61,214,12,muted,518);
        const char* labels[]={"Apply","Discard changes","Back"};
        auto activate=[&](unsigned i){if(i==0)LanguageApply();else if(i==1){LanguageReadDraft();message="Changes discarded.";}else LanguageBack();};
        RowStrip(canvas,labels,nullptr,3,398,focus?focus-1:3,[&](unsigned i){focus=i+1;},activate);
        if(accept&&focus)activate(focus-1);
    }
    if(back)LanguageBack();
    canvas.Center(!message.empty()?message.c_str():LanguageDirty()?"Changes not applied.":
        trial_language::PendingRestart()?"Restart the game to use the saved language.":"",30,429,580,11,muted);
    if(xbox_KeyboardMode())FooterPrompts(canvas,{{1,"Select"},{37,"Change"},{39,"Change"},{VK_RBUTTON,"Back"}});
    else FooterPrompts(canvas,{{0x103,"Navigate"},{0x101,"Select"},{0x102,"Back"}});
    ImGui::EndDisabled();EndMenu();
}
