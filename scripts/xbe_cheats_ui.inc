void CheatsUi(const ImGuiViewport* view){
    BeginMenu("Main menu cheats",view);MenuCanvas canvas(view);canvas.SettingsFrame(false,"CHEATS");
    static TrialFrontendReleaseGate gate;static uint64_t epoch=0,identity=0;static unsigned focus=0;
    const auto generation=navigation_generation.load();
    if(epoch!=generation){epoch=generation;focus=0;identity=0;}
    const bool ready=gate.Accept(generation,UiHeld());
    const unsigned events=frontend_events.exchange(0);
    const bool accept=TakeNativeConfirm(ready)||(ready&&(events&4));
    const bool back=ready&&((events&8)||ImGui::IsKeyPressed(ImGuiKey_Escape,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false)||ImGui::IsMouseClicked(1,false));
    const auto state=trial_cheats::Snapshot();
    const bool available=state.valid&&state.pause_epoch==cheats_epoch;
    if(identity&&(!available||state.identity!=identity)){Close();EndMenu();return;}
    if(available&&!identity)identity=state.identity;
    int delta=0;
    if(ready){
        if((events&1)||ImGui::IsKeyPressed(ImGuiKey_UpArrow))focus=(focus+trial_cheats::Count)%(trial_cheats::Count+1);
        if((events&2)||ImGui::IsKeyPressed(ImGuiKey_DownArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab))focus=(focus+1)%(trial_cheats::Count+1);
        if((events&16)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow))delta=-1;
        if((events&32)||ImGui::IsKeyPressed(ImGuiKey_RightArrow)||accept)delta=1;
    }
    const unsigned keyboard_focus=focus;
    ImGui::BeginDisabled(!ready||!available||state.pending);
    for(unsigned i=0;i<trial_cheats::Count;++i){
        if(SettingRow(canvas,trial_cheats::Names[i],state.enabled&(1u<<i)?"On":"Off",i,focus,i==keyboard_focus?delta:0,22,94)&&ready&&available&&!state.pending)
            if(!trial_cheats::Toggle(i,state))message="Cheat state changed. Try again.";
    }
    ImGui::EndDisabled();
    canvas.Text("Original cheats apply to the whole game, including both co-op players.",61,325,12,muted,518);
    canvas.Text("Weapon changes may require re-equipping or restarting the mission.",61,346,12,muted,518);
    ImGui::BeginDisabled(!ready);
    const char* labels[]={"Back"};
    RowStrip(canvas,labels,nullptr,1,398,focus==trial_cheats::Count?0:1,
        [&](unsigned){focus=trial_cheats::Count;},[&](unsigned){Close();});
    if(back||(accept&&focus==trial_cheats::Count))Close();
    canvas.Center(!available?"Cheats are unavailable here.":state.pending?"Applying cheat...":message.c_str(),30,429,580,11,muted);
    if(xbox_KeyboardMode())FooterPrompts(canvas,{{37,"Change"},{39,"Change"},{VK_RETURN,"Select"},{VK_RBUTTON,"Back"}});
    else FooterPrompts(canvas,{{0x103,"Navigate"},{0x101,"Select"},{0x102,"Back"}});
    ImGui::EndDisabled();EndMenu();
}
