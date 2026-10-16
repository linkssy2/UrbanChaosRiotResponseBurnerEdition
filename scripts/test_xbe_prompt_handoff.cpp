// The former guest-only fixture observed State before the real renderer queue.
// This executes its actual range selection, 64-byte deltas, state publication
// and Op::State replay, with delayed draw capture instead of a GPU device.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <smmintrin.h>
#include "gpu/state.h"
using namespace gpu;

// TRANSPORT_BEGIN
/* ORIGINAL_COMMAND_TYPES */
/* ORIGINAL_STATE_RANGES */
class Replay {
public:
    std::vector<State> draws;
    void Queue(const State& state) {
        SendState(state);
        cur_->cmds.push_back(Cmd{Op::Draw});
    }
    bool CopiesPrompt() const {
        const size_t offset=At(sent_,&sent_.uc_prompt_variant);
        for(const auto& range:ranges_)
            if(offset>=range.off&&offset+sizeof(sent_.uc_prompt_variant)<=range.off+range.len)return true;
        return false;
    }
    size_t States() const {
        return std::count_if(cur_->cmds.begin(),cur_->cmds.end(),[](const Cmd& c){return c.op==Op::State;});
    }
    void Drain() {
        for(const auto& command:cur_->cmds)Execute(*cur_,command);
        cur_->cmds.clear();cur_->bytes.clear();cur_->vs.clear();cur_->ps.clear();
    }
private:
    // This test uses no guest resource headers. This is the same null-header
    // fast path as production Header; texture uploads are independently tested.
    uint32_t Header(uint32_t header){assert(header==0);return header;}
    /* ORIGINAL_SEND_METHODS */
    void Execute(const Batch& b,const Cmd& c) {
        const uint8_t* data=b.bytes.empty()?nullptr:b.bytes.data()+c.data;
        switch(c.op) {
            /* ORIGINAL_STATE_REPLAY */
            case Op::Draw:draws.push_back(state_);break;
            default:assert(false&&"only queued state and draws in this fixture");
        }
    }
    std::vector<Range> ranges_=StateRanges();
    State sent_{},state_{};
    Batch batch_{};
    Batch* cur_=&batch_;
};
// TRANSPORT_END

template<class State> static State Scene() {
    State s{};
    s.simple[17]=0x11223344;s.rs[143]=1;s.tss[2][0]=0x55667788;
    s.index_data=0x12345678;s.base_vertex=19;s.vs_handle=0x55;
    s.vsc[104][2]=3.25f;s.psc[3][1]=-.25f;
    s.viewport.x=12;s.viewport.y=34;s.viewport.w=1280;s.viewport.h=720;
    s.scissor_count=1;s.scissors[0]=9;s.scissors[1]=11;s.scissors[2]=777;s.scissors[3]=555;
    s.transforms[2][5]=2;s.transforms[3][5]=3;s.transforms[16][5]=16;s.transforms[256][5]=256;
    s.material[7]=.75f;s.lights[3][5]=71;s.light_enable=8;s.ss_offset_x=.5f;
    s.uc_fog_pass=2;s.uc_fog_params_valid=1;s.uc_fog_params[3]=10000;
    s.vs=std::make_shared<VertexShader>();s.ps=std::make_shared<PixelShader>();
    return s;
}
template<class State> static void Unrelated(const State& s,const State& expected) {
    assert(s.simple==expected.simple&&s.rs==expected.rs&&s.tss==expected.tss);
    assert(s.index_data==expected.index_data&&s.base_vertex==19&&s.vs_handle==expected.vs_handle);
    assert(s.vsc==expected.vsc&&s.psc==expected.psc);
    assert(s.viewport.x==12&&s.viewport.y==34&&s.viewport.w==1280&&s.viewport.h==720);
    assert(s.scissor_count==expected.scissor_count&&s.scissors==expected.scissors);
    for(unsigned index:{2u,3u,16u,256u})assert(s.transforms[index]==expected.transforms[index]);
    assert(s.material==expected.material&&s.lights==expected.lights&&s.light_enable==8&&s.ss_offset_x==.5f);
    assert(s.uc_fog_pass==2&&s.uc_fog_params_valid==1&&s.uc_fog_params[3]==10000);
    assert(s.vs==expected.vs&&s.ps==expected.ps);
}
template<class Pipeline,class State> static void Sequence(bool retained) {
    Pipeline pipeline;assert(pipeline.CopiesPrompt()==retained);
    State source=Scene<State>(),expected=source;
    const unsigned variants[]={1,0,1,1,0,0,1};
    for(unsigned variant:variants){source.uc_prompt_variant=variant;pipeline.Queue(source);}
    // Repeated variant with identical state creates no redundant state packet.
    assert(pipeline.States()==(retained?5u:1u));
    // Rendering is deliberately delayed until after the guest scopes reset and
    // another draw has changed the source data. Queued snapshots must survive.
    source.uc_prompt_variant=0;source.material[7]=0;source.vs.reset();source.ps.reset();
    assert(pipeline.draws.empty());pipeline.Drain();assert(pipeline.draws.size()==7);
    for(unsigned i=0;i<7;++i){
        assert(pipeline.draws[i].uc_prompt_variant==(retained?variants[i]:0));
        Unrelated(pipeline.draws[i],expected);
    }
    // The final sent variant was1; a new batch must transmit its later reset0,
    // then permit interaction1 again without overwriting the previous draws.
    source=expected;source.uc_prompt_variant=0;pipeline.Queue(source);
    source.uc_prompt_variant=1;pipeline.Queue(source);pipeline.Drain();
    assert(pipeline.draws.size()==9&&pipeline.draws[7].uc_prompt_variant==0);
    assert(pipeline.draws[8].uc_prompt_variant==unsigned(retained));
    Unrelated(pipeline.draws[7],expected);Unrelated(pipeline.draws[8],expected);
}
int main() {
    Sequence<prior::Replay,prior::State>(false);
    Sequence<fixed::Replay,gpu::State>(true);
    std::puts("prompt handoff: original leading field loses all F variants; actual range/delta/send/replay retains trailing metadata across delayed draws, resets and batches");
}
