"""Stage an offline production-ImGui layout and input fixture; no game launch."""
from pathlib import Path
import hashlib


def prepare(area):
    area=Path(area);local=Path(__file__).parent
    ui=(local/'xbe_trial_graphics_ui.cpp').read_text()
    for name in ("xbe_controls_ui.inc","xbe_language_ui.inc"):
        marker='#include "'+name+'"'
        assert ui.count(marker)==1
        ui=ui.replace(marker,(local/name).read_text())
    marker='return ImGui::InvisibleButton(id,{width*sx,height*sy});'
    assert ui.count(marker)==1
    # Observation only: the real production InvisibleButton still owns input.
    observed=ui.replace(marker,'RecordMenuHit(id,Point(x,y),{width*sx,height*sy});\n        '+marker,1)
    marker='    c.Text("Revive Partner",x+key_width+15,y+6,14,white,232-key_width,true);'
    assert observed.count(marker)==1
    observed=observed.replace(marker,'    RecordRevivePrompt(slot,source,binding,trial_language::Ui("Revive Partner"));\n'+marker,1)
    marker='    c.Center(timer,245,y-8,150,28,number_color,true);'
    assert observed.count(marker)==1
    observed=observed.replace(marker,marker+'\n    RecordReviveCountdown(slot,seconds,c.Point(320,y),trial_language::Ui("Revive time remaining"));',1)
    fixture=(local/'test_xbe_menu_style.cpp').read_text()
    assert fixture.count('// PRODUCTION_MENU_UI')==1
    fixture=fixture.replace('// PRODUCTION_MENU_UI',observed)
    fixture=fixture.replace('/* GAME_PATH */','L"'+(area/'assets').as_posix()+'"')
    (area/'src/compat/test_menu_style.cpp').write_text(fixture)
    return {'ui_sha256':hashlib.sha256(ui.encode()).hexdigest(),
            'sizes':[[640,480],[1920,1080],[2560,1440]],
            'scope':'actual ImGui geometry/input with user font archives; no scene or GPU parity claim'}
