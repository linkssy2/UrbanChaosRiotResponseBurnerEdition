"""Remove hooks left by the retired brightness-readback experiment."""
from pathlib import Path

def restore_generated(area):
    """Keep old generated trees compatible with the current renderer."""
    path=Path(area)/'generated/game/x86_recomp.14.cpp'
    text=path.read_text()
    text=text.replace('extern "C" void TrialBrightnessComplete(X86Context&);\n','').replace('\tTrialBrightnessComplete(ctx);\n','')
    path.write_text(text)
