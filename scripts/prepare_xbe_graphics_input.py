"""Exercise the staged input adapter without ImGui, SDL or a visible window."""
from pathlib import Path
import re


def function(source, name, prefix='void '):
    start = source.index(prefix + name + '(')
    end = source.index('\n}', start) + 2
    return source[start:end]


def prepare(source, area):
    """Call after cursor preparation so accidental helper deletion is caught."""
    state_start = source.index('std::recursive_mutex input_lock;')
    state_end = source.index('\nstruct XBOX_GAMEPAD', state_start)
    state = source[state_start:state_end]
    pad_type = re.search(r'struct XBOX_GAMEPAD \{[^\n]+\};', source)
    digital = re.search(r'const WORD controller_digital\[\]=\{[^\n]+\};', source)
    assert pad_type and digital
    key_down = re.search(r'BOOL keyboard_key_down\(int key\)\{[^\n]+\}', source)
    assert key_down
    fixture = Path(__file__).with_name('test_xbe_graphics_input.cpp').read_text()
    control = (Path(__file__).resolve().parents[1] / 'src/recomp_manual.c').read_text()
    control_stick = '\n'.join(function(control, name, 'static float ') for name in
                              ('ucrr_host_stick_value', 'ucrr_host_stick_axis'))
    helpers_start = source.index('namespace {\ntrial_controls::CaptureGate controls_capture;')
    helpers_end = source.index('\nvoid TrialMenuRightButton', helpers_start)
    for marker, code in (
        ('// GENERATED_STATE', state + '\n' + pad_type[0] + '\n' + digital[0]),
        ('// GENERATED_KEY_DOWN', key_down[0].replace('GetAsyncKeyState', 'TestAsyncKeyState')),
        ('// GENERATED_RIGHT_BACK', function(source, 'TrialMenuRightButton') + '\n' +
         function(source, 'TrialMenuRightBack', 'bool ')),
        ('// GENERATED_CHANGED', function(source, 'TrialGraphicsInputChanged')),
        ('// GENERATED_CONTROLS', source[helpers_start:helpers_end]),
        ('// GENERATED_PUBLISH', function(source, 'TrialInputPublish')),
        ('// GENERATED_MENU_KEYS', '\n'.join(function(source, name) for name in
            ('TrialMenuKeysBegin', 'TrialMenuKeysEnd', 'TrialGameplay', 'ucrr_host_menu_mode')) + '\nvoid TrialMenuKeysReset(){menu_key_taps.Clear();}'),
        ('// GENERATED_PAD', function(source, 'TrialInputPad')),
        ('// GENERATED_CONTROL_STICK', control_stick),
    ):
        assert fixture.count(marker) == 1, marker
        fixture = fixture.replace(marker, code)
    out = Path(area) / 'src/compat/test_graphics_input.cpp'
    out.write_text(fixture)
    return source
