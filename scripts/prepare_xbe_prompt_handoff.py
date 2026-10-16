"""Stage the actual threaded renderer's POD state transport regression."""
from pathlib import Path


def between(text,begin,end):
    start=text.index(begin)
    return text[start:text.index(end,start)]


def prepare(sdk,area):
    sdk,area=Path(sdk),Path(area)
    source=(sdk/'runtime/gpu/render_thread.cpp').read_text()
    state=(area/'src/gpu/state.h').read_text()
    field='    unsigned uc_prompt_variant = 0; // original queued text identity\n'
    assert state.count(field)==1 and state.index(field)>state.index('    std::array<float, 17> material')
    layout=between(state,'struct State {','\n};')+'\n};\n'
    old_layout=layout.replace(field,'').replace('struct State {\n','struct State {\n'+field,1)
    # Both layouts run exactly the same source-selected transport methods. The
    # only negative-witness change is the old, excluded first-field placement.
    types=between(source,'enum class Op :','// the plain data in State')
    ranges=between(source,'struct Range {','using Clock =')
    methods=between(source,'    // a field of the render thread\'s State,','    // resources by guest header:')
    replay=between(source,'            case Op::State: {','            case Op::Draw:')
    template=(Path(__file__).parent/'test_xbe_prompt_handoff.cpp').read_text()
    for marker,value in [('/* ORIGINAL_COMMAND_TYPES */',types),
                         ('/* ORIGINAL_STATE_RANGES */',ranges),
                         ('/* ORIGINAL_SEND_METHODS */',methods),
                         ('/* ORIGINAL_STATE_REPLAY */',replay)]:
        assert template.count(marker)==1
        template=template.replace(marker,value,1)
    begin=template.index('// TRANSPORT_BEGIN\n')+len('// TRANSPORT_BEGIN\n')
    end=template.index('// TRANSPORT_END\n')
    transport=template[begin:end]
    template=template[:begin]+('namespace fixed {\nusing State=gpu::State;\n'+transport+'}\n'
                              'namespace prior {\n'+old_layout+transport+'}\n')+template[end:]
    out=area/'src/compat/test_prompt_handoff.cpp'
    out.write_text(template)
