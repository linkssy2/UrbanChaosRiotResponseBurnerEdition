// Compare production packing with the previous float4 assembler.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "core/log.h"
#include "gpu/vertex.h"
#include "kernel/kernel.h"

namespace kernel { uint8_t* g_base=nullptr; }
namespace mem { uint32_t VirtualFromPhysical(uint32_t address){return address;} }
namespace logx { void Write(Level,const char*,...){} }

/* ORIGINAL_VERTEX */
/* PRODUCTION_VERTEX */
/* PREVIOUS_INDEX_DISPATCH */
/* PREVIOUS_STRIP_PAIRS */

namespace {
constexpr unsigned kHeader=0x100,kData=0x1000,kStride=64,kVertices=65536+32;
std::vector<uint8_t> memory(0xB00000);
unsigned checks=0;

gpu::State State() {
    gpu::State s{};s.vs_handle=1;s.vs=std::make_shared<gpu::VertexShader>();
    s.streams[0]={kHeader,kStride};
    // Misaligned position forces the real converted path; other attributes vary.
    s.vs->inputs[0]={0,1,0x32};s.vs->inputs[2]={0,16,0x31};
    s.vs->inputs[3]={0,24,0x40};s.vs->inputs[9]={0,28,0x22};
    return s;
}

gpu::State PackedState(unsigned layout) {
    gpu::State s{};s.vs_handle=1;s.vs=std::make_shared<gpu::VertexShader>();
    for(unsigned stream:{0u,1u,2u,4u})s.streams[stream]={kHeader+32*(stream+1),stream==0?10u:stream==1?4u:8u};
    s.vs->inputs[0]={0,0,0x31};
    if(layout>=2)s.vs->inputs[1]={0,6,0x16};
    if(layout>=3)s.vs->inputs[2]={1,0,0x40};
    if(layout>=1&&layout<=3)s.vs->inputs[3]={2,0,0x22};
    if(layout==1||layout==2)s.vs->inputs[4]={2,8,0x22};
    s.vs->inputs[7]={4,0,0x35};s.vs->inputs[8]={4,6,0x24};
    return s;
}

gpu::State ParticleState() {
    gpu::State s{};s.vs_handle=1;s.vs=std::make_shared<gpu::VertexShader>();
    // Original 9A225/9B4D1 inline quad declaration, verified by test_inline_draws.
    s.streams[0]={kHeader,52};
    s.vs->inputs[0]={0,0,0x32};s.vs->inputs[1]={0,12,0x40};
    s.vs->inputs[2]={0,16,0x32};s.vs->inputs[3]={0,28,0x32};s.vs->inputs[4]={0,40,0x32};
    return s;
}

std::vector<float> ConvertedValues(const gpu::State& s,const gpu::Assembled& a) {
    if(!a.repacked)return a.vertices;
    std::vector<float> values;
    for(unsigned v=0;v<a.vertex_count;++v)for(unsigned r=0;r<16;++r)if(a.regs>>r&1){
        const auto& e=a.elements[r];const auto& buffer=a.buffers[e.buffer];
        const auto type=s.vs->inputs[r].type;
        const auto host=gpu::HostFormat(type);
        assert(e.format==host.format && ((a.decode>>(r*4))&15)==host.decode);
        assert(e.offset%std::min<unsigned>(host.bytes,4)==0 && buffer.stride%4==0);
        const auto* source=buffer.src+size_t(v)*buffer.stride+e.offset;
        float decoded[4];original_vertex::OriginalConvert(type,source,decoded);
        values.insert(values.end(),decoded,decoded+4);
        if(e.buffer==0)for(unsigned i=gpu::TypeBytes(type);i<((host.bytes+3u)&~3u);++i)assert(source[i]==0);
    }
    return values;
}

std::vector<uint32_t> VertexOrder(const gpu::Assembled& a) {
    const uint32_t count=a.index_count?a.index_count:a.draw_count;
    std::vector<uint32_t> order(count);
    for(uint32_t i=0;i<count;++i){
        uint32_t v=i;
        if(a.index_count){
            if(a.index16){uint16_t x;std::memcpy(&x,static_cast<const uint8_t*>(a.index_src)+2*i,2);v=x;}
            else std::memcpy(&v,static_cast<const uint8_t*>(a.index_src)+4*i,4);
            v+=a.base_vertex;
        }
        assert(v<a.vertex_count);order[i]=v;
    }
    if(!a.strip && a.topology==gpu::Topology::Lines)order.resize(count/2*2);
    if(!a.strip && a.topology==gpu::Topology::Triangles)order.resize(count/3*3);
    return order;
}

void Check(gpu::State& s,uint32_t prim,uint32_t start,uint32_t count,const uint16_t* indices) {
    // Renderer reuses these outputs across raw, converted, and rejected draws.
    static gpu::Assembled before,after;
    std::fill(after.vertices.begin(),after.vertices.end(),-98765.f);
    std::fill(after.converted_storage.begin(),after.converted_storage.end(),-98765.f);
    const auto* data=reinterpret_cast<const uint8_t*>(indices);
    bool a=original_vertex::Assemble(s,prim,start,count,data,before);
    bool b=gpu::Assemble(s,prim,start,count,data,after);
    assert(a==b);++checks;
    const auto decoded=ConvertedValues(s,after);
    const auto reference=ConvertedValues(s,before);
    assert(reference.size()==decoded.size());
    if(!reference.empty())assert(!std::memcmp(reference.data(),decoded.data(),reference.size()*sizeof(float)));
    assert(after.vertices.empty()||after.converted_storage.empty());
    if(!a)return;
    assert(before.regs==after.regs);
    if(!after.repacked)assert(before.decode==after.decode);
    assert(before.pretransformed==after.pretransformed && before.vertex_count==after.vertex_count);
    assert(before.topology==after.topology && before.strip==after.strip);
    if(!before.vertices.empty())
        assert(before.index_count+before.draw_count==after.index_count+after.draw_count);
    if(!after.repacked){
        assert(before.buffer_count==after.buffer_count);
        for(int r=0;r<16;++r)if(before.regs>>r&1)
            assert(!std::memcmp(&before.elements[r],&after.elements[r],sizeof(before.elements[r])));
        for(unsigned i=0;i<before.buffer_count;++i){
            assert(before.buffers[i].bytes==after.buffers[i].bytes && before.buffers[i].stride==after.buffers[i].stride);
            assert(!std::memcmp(before.buffers[i].src,after.buffers[i].src,before.buffers[i].bytes));
        }
    }else{
        assert(after.vertices.empty()&&after.buffer_count>=1);
        assert(after.buffers[0].bytes==after.vertex_count*after.buffers[0].stride);
        for(unsigned r=0;r<16;++r)if(after.regs>>r&1){
            const auto& e=after.elements[r];const auto& b=after.buffers[e.buffer];
            assert(uint64_t(after.vertex_count-1)*b.stride+e.offset+gpu::HostFormat(s.vs->inputs[r].type).bytes<=b.bytes);
        }
    }
    assert(VertexOrder(before)==VertexOrder(after));
    // Fallbacks keep their index bytes, including strip winding and degenerates.
    if(prim!=1 && prim!=2 && prim!=5)assert(before.indices==after.indices);
}

void CheckDispatch(gpu::State& s,uint32_t prim,uint32_t start,uint32_t count,const uint8_t* ip) {
    Check(s,prim,start,count,reinterpret_cast<const uint16_t*>(ip));
    if(!count)return;
    const auto bounds=ip?gpu::IndexRange(ip,count):gpu::IndexBounds{start,start+count-1};
    gpu::DrawRange draw{prim,start,count,ip,bounds.first,
        ip?bounds.first+s.base_vertex:start,bounds.last-bounds.first+1};
    gpu::Assembled before,after;
    before.indices=after.indices={91,82,73};
    int regs[16];
    const int nregs=gpu::CollectRegs(s,*s.vs,regs,before);
    after.regs=before.regs;
    gpu::Inputs inputs{*s.vs,regs,nregs};
    const bool a=previous_index_dispatch::AssembleConverted(s,inputs,draw,before);
    const bool b=gpu::AssembleConverted(s,inputs,draw,after);
    assert(a==b&&before.indices==after.indices&&before.topology==after.topology);
    assert(before.indices.size()>=3&&before.indices[0]==91&&before.indices[1]==82&&before.indices[2]==73);
    ++checks;
}

void CheckIndexDispatch() {
    const uint16_t dense[]={7,8,9,7,9,10,10,10,8,9,7,10};
    const uint16_t sparse[]={1,3000,6000,1,6000,9000,9000,9000,1,6000,3000,9000};
    const uint16_t high[]={65533,65534,65535,65535,65534,65533,65533,65533,65535,65534,65533,65535};
    for(auto s:{State(),PackedState(2)})for(unsigned prim=1;prim<=10;++prim)for(unsigned count=0;count<=12;++count){
        for(uint32_t base:{0u,3u,0xfffffff9u}){
            s.base_vertex=base;
            CheckDispatch(s,prim,5,count,nullptr);
            std::array<uint8_t,sizeof(dense)+1> unaligned{};
            std::memcpy(unaligned.data()+1,dense,sizeof(dense));
            CheckDispatch(s,prim,0,count,unaligned.data()+1);
        }
        s.base_vertex=0;CheckDispatch(s,prim,0,count,reinterpret_cast<const uint8_t*>(sparse));
        s.base_vertex=uint32_t(0)-65530;CheckDispatch(s,prim,0,count,reinterpret_cast<const uint8_t*>(high));
    }
    // Long odd/even strips retain degenerates, 65535 and incomplete-quad behavior.
    std::vector<uint16_t> indices(65536);
    for(unsigned i=0;i<indices.size();++i)indices[i]=uint16_t(i/2*31);
    auto s=State();
    for(unsigned prim:{6u,8u})for(unsigned count:{65535u,65536u})
        CheckDispatch(s,prim,0,count,reinterpret_cast<const uint8_t*>(indices.data()));
#ifdef _WIN32
    auto* guarded=static_cast<uint8_t*>(VirtualAlloc(nullptr,0x2000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    assert(guarded);DWORD previous=0;
    assert(VirtualProtect(guarded+0x1000,0x1000,PAGE_NOACCESS,&previous));
    for(unsigned count:{1u,2u,3u,4u,7u,12u}){
        auto* ip=guarded+0x1000-count*2;
        std::memcpy(ip,dense,count*2);
        for(unsigned prim=1;prim<=10;++prim)CheckDispatch(s,prim,0,count,ip);
    }
    assert(VirtualFree(guarded,0,MEM_RELEASE));
#endif
}

void CheckParticleQuads() {
    auto state=ParticleState();std::vector<uint16_t> indices(65536);
    for(unsigned i=0;i<indices.size();++i)indices[i]=uint16_t(i*31);
    for(unsigned count:{1u,2u,3u,4u,5u,7u,8u,9u,11u,12u,15u,16u,156u,157u,65532u,65535u,65536u}){
        Check(state,8,0,count,nullptr);Check(state,8,0,count,indices.data());
        auto converted=state;converted.vs=std::make_shared<gpu::VertexShader>(*state.vs);converted.vs->inputs[0].offset=1;
        Check(converted,8,0,count,nullptr);Check(converted,8,0,count,indices.data());
    }
    for(uint32_t count=0;count<1024;++count){
        std::vector<uint32_t> expected{91,82,73},actual=expected;
        auto local=[](uint32_t v){return(v*1789)&65535;};
        for(uint32_t q=0;q<count/4;++q)for(uint32_t corner:{0u,1u,2u,0u,2u,3u})expected.push_back(local(q*4+corner));
        gpu::Topology topology;gpu::ListIndices(8,count,local,actual,topology);
        assert(actual==expected&&topology==gpu::Topology::Triangles);++checks;
    }
#ifdef _WIN32
    auto* guarded=static_cast<uint8_t*>(VirtualAlloc(nullptr,0x2000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    assert(guarded);DWORD previous=0;
    assert(VirtualProtect(guarded+0x1000,0x1000,PAGE_NOACCESS,&previous));
    for(unsigned count:{3u,4u,7u,156u,157u}){
        uint8_t* ip=guarded+0x1000-count*2;
        for(unsigned i=0;i<count;++i){uint16_t value=uint16_t(i);std::memcpy(ip+i*2,&value,2);}
        Check(state,8,0,count,reinterpret_cast<const uint16_t*>(ip));
        std::vector<uint8_t> unaligned(count*2+1);std::memcpy(unaligned.data()+1,ip,count*2);
        Check(state,8,0,count,reinterpret_cast<const uint16_t*>(unaligned.data()+1));
    }
    assert(VirtualFree(guarded,0,MEM_RELEASE));
#endif
}

using AssembleFn=bool(*)(const gpu::State&,uint32_t,uint32_t,uint32_t,const uint8_t*,gpu::Assembled&);
double Bench(AssembleFn assemble,const gpu::State& s,uint32_t prim,const std::vector<uint16_t>& indices,unsigned rounds,bool mixed) {
    gpu::Assembled output;
    auto raw=s;raw.vs=std::make_shared<gpu::VertexShader>(*s.vs);raw.vs->inputs[0].offset=0;
    const auto* ip=reinterpret_cast<const uint8_t*>(indices.data());
    assert(assemble(s,prim,0,unsigned(indices.size()),ip,output));
    volatile uint64_t witness=0;
    const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<rounds;++i){
        bool made=assemble(s,prim,0,unsigned(indices.size()),ip,output);
        witness=witness+made+output.vertex_count+output.index_count+output.draw_count;
        if(mixed){
            made=assemble(raw,prim,0,unsigned(indices.size()),ip,output);
            witness=witness+made+output.vertex_count+output.vertices.size();
        }
    }
    assert(witness);
    return std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/rounds;
}

void BenchIndexDispatch() {
    for(bool packed:{false,true})for(unsigned vertices:{34u,329u,1000u}){
        auto s=packed?PackedState(2):State();std::vector<uint16_t> indices(vertices*2-2);
        for(unsigned i=0;i<indices.size();++i)indices[i]=uint16_t(i%vertices);
        std::array<double,7> before{},after{};
        for(unsigned pass=0;pass<before.size();++pass){
            if(pass%2){after[pass]=Bench(gpu::Assemble,s,6,indices,20000,false);before[pass]=Bench(previous_index_dispatch::Assemble,s,6,indices,20000,false);}
            else{before[pass]=Bench(previous_index_dispatch::Assemble,s,6,indices,20000,false);after[pass]=Bench(gpu::Assemble,s,6,indices,20000,false);}
        }
        std::sort(before.begin(),before.end());std::sort(after.begin(),after.end());
        std::printf("BENCH index dispatch only %s: %u vertices, %zu indices | median before %.3f us after %.3f us (%.1f%%); synthetic CPU assembly, not FPS\n",
            packed?"packed":"converted",vertices,indices.size(),before[3],after[3],100*(before[3]-after[3])/before[3]);
    }
}

void CheckStripPairs() {
    std::vector<uint16_t> indices(65536);
    for(unsigned i=0;i<indices.size();++i)indices[i]=uint16_t(i/2*31);
    for(unsigned count:{0u,1u,2u,3u,4u,5u,6u,7u,34u,329u,1000u,65535u,65536u}) {
        std::vector<uint32_t> before{91,82,73},after=before;
        gpu::Topology a{},b{};
        unsigned reads=0;
        const auto local=[&](uint32_t i){assert(i<count);return uint32_t(indices[i])-37u;};
        previous_strip_pairs::PreviousListIndices(6,count,local,before,a);
        gpu::ListIndices(6,count,[&](uint32_t i){++reads;return local(i);},after,b);
        assert(before==after&&a==b);
        assert(reads==(count>=3?count:0));
        ++checks;
    }
}

void BenchStripPairs() {
    // Same full production assembler before and after only the strip loop change.
    for(unsigned layout=0;layout<3;++layout)for(unsigned vertices:{34u,329u,1000u}) {
        auto s=layout==0?PackedState(2):layout==1?State():ParticleState();
        std::vector<uint16_t> indices(vertices*2-2);
        for(unsigned i=0;i<indices.size();++i)indices[i]=uint16_t(i%vertices);
        std::array<double,7> before{},after{};
        for(unsigned pass=0;pass<7;++pass){
            if(pass%2){after[pass]=Bench(gpu::Assemble,s,6,indices,10000,false);before[pass]=Bench(previous_strip_pairs::Assemble,s,6,indices,10000,false);}
            else{before[pass]=Bench(previous_strip_pairs::Assemble,s,6,indices,10000,false);after[pass]=Bench(gpu::Assemble,s,6,indices,10000,false);}
        }
        std::sort(before.begin(),before.end());std::sort(after.begin(),after.end());
        std::printf("BENCH paired strips %s: %u vertices, %zu indices | median before %.3f us after %.3f us (%.1f%%); synthetic CPU assembly, not FPS\n",
            layout==0?"packed":layout==1?"converted":"raw control",vertices,indices.size(),before[3],after[3],100*(before[3]-after[3])/before[3]);
    }
}
}

int main(int argc,char** argv) {
    kernel::g_base=memory.data();kernel::W32(kHeader+4,kData);
    for(unsigned i=0;i<kVertices;++i){
        auto* p=memory.data()+kData+i*kStride;
        for(unsigned j=0;j<kStride;++j)p[j]=uint8_t(i*29+j*17);
        float position[3]={float(i),float(i%97),float(i%53)};
        std::memcpy(p+1,position,sizeof(position));
        const uint16_t shorts[]={uint16_t(i),uint16_t(~i),uint16_t(i*73),uint16_t(i*257)};
        std::memcpy(p+16,shorts,sizeof(shorts));
    }
    auto s=State();
    const uint16_t dense[]={7,8,9,7,9,10,10,10,8,9,7,10};
    const uint16_t sparse[]={1,3000,6000,1,6000,9000,9000,9000,1,6000,3000,9000};
    const uint16_t high[]={65533,65534,65535,65535,65534,65533};
    for(unsigned prim=0;prim<=11;++prim)for(unsigned count=0;count<=12;++count){
        for(unsigned base:{0u,3u}){
            s.base_vertex=base;
            Check(s,prim,5,count,nullptr);Check(s,prim,0,count,dense);Check(s,prim,0,count,sparse);
        }
    }
    for(unsigned type:{0x12u,0x22u,0x32u,0x42u,0x72u,0x40u,0x11u,0x21u,0x31u,0x41u,
                       0x14u,0x24u,0x34u,0x44u,0x15u,0x25u,0x35u,0x45u,0x16u}){
        s.vs->inputs[2].type=type;
        for(unsigned prim=1;prim<=10;++prim){Check(s,prim,0,12,dense);Check(s,prim,0,6,high);}
        // All signed/unsigned 16-bit values, including normalization endpoints.
        Check(s,5,0,65536,nullptr);
    }
    for(unsigned type:{0x01u,0x02u,0x03u,0x10u,0x52u,0xFFFFu}) {
        s.vs->inputs[2].type=type;
        Check(s,5,0,12,dense);Check(s,6,0,12,sparse);
    }
    // Float payloads, including NaN/infinity/signed zero, remain byte-identical.
    const uint32_t unusual[]={0x7fc12345,0x7f800000,0x80000000};
    std::memcpy(memory.data()+kData+7*kStride+1,unusual,sizeof(unusual));
    s.base_vertex=0;Check(s,5,0,12,dense);
    // Raw and fixed-function callers retain their existing assembly route.
    s=State();s.vs->inputs[0].offset=0;Check(s,5,0,12,dense);
    gpu::Assembled raw;
    assert(gpu::Assemble(s,5,0,12,reinterpret_cast<const uint8_t*>(dense),raw)&&raw.vertices.empty());
    s.vs_handle=0x144;Check(s,5,0,12,dense);
    // Independent streams and declaration stride fallback take the same bytes.
    s=State();s.vs->inputs[2].stream=1;s.streams[1]={kHeader,0};s.vs->stream_stride[1]=kStride;
    Check(s,5,0,12,dense);Check(s,6,0,12,sparse);
    // Raw/rejected draws must stay empty without discarding the last conversion's storage.
    for(unsigned count:{4u,3u,7u,3u,12u,2u,1u,0u}){
        s=State();Check(s,5,0,count,nullptr);
        s.vs->inputs[0].offset=0;Check(s,5,0,12,dense);
        Check(s,0,0,12,dense);Check(s,5,0,0,nullptr);
        s.vs.reset();Check(s,5,0,12,dense);
        s=State();s.streams[0].vb=0;Check(s,5,0,12,dense);
        s=State();Check(s,6,0,count,sparse);
    }
    // Two live outputs own independent buffers, even across interleaved raw draws.
    gpu::Assembled first,second;s=State();
    assert(gpu::Assemble(s,5,0,12,reinterpret_cast<const uint8_t*>(dense),first));
    const auto saved=first.vertices;
    assert(gpu::Assemble(s,5,0,12,reinterpret_cast<const uint8_t*>(sparse),second));
    s.vs->inputs[0].offset=0;
    assert(gpu::Assemble(s,5,0,12,reinterpret_cast<const uint8_t*>(dense),second)&&second.vertices.empty());
    assert(first.vertices.size()==saved.size()&&!std::memcmp(first.vertices.data(),saved.data(),saved.size()*sizeof(float)));
    uint32_t random=0x12345678;
    for(unsigned stream:{0u,1u,2u,4u}){
        const unsigned address=0x500000+stream*0x100000;
        kernel::W32(kHeader+32*(stream+1)+4,address);
        const unsigned stride=stream==0?10:stream==1?4:8;
        for(unsigned i=0;i<kVertices*stride+16;++i){random^=random<<13;random^=random>>17;random^=random<<5;memory[address+i]=uint8_t(random);}
    }
    for(unsigned layout=0;layout<5;++layout){
        for(unsigned prim=0;prim<=11;++prim)for(unsigned count=0;count<=12;++count)for(unsigned base:{0u,3u}){
            s=PackedState(layout);s.base_vertex=base;
            Check(s,prim,5,count,nullptr);Check(s,prim,0,count,dense);Check(s,prim,0,count,sparse);
        }
        s=PackedState(layout);Check(s,6,0,6,high);Check(s,5,0,65536,nullptr);
        assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(dense),raw)&&raw.repacked);
        // Exact position/normal bytes and padding complement the decoded-value comparison.
        const bool has_normal=layout>=2;
        const auto& packed=raw.buffers[0];
        assert(packed.stride==(has_normal?12u:8u));
        assert(raw.elements[0].buffer==0&&raw.elements[0].offset==0);
        if(has_normal)assert(raw.elements[1].buffer==0&&raw.elements[1].offset==8);
        for(unsigned v=0;v<raw.vertex_count;++v){
            std::array<uint8_t,12> expected{};
            const auto* source=memory.data()+0x500000+(7+v)*10;
            std::memcpy(expected.data(),source,6);
            if(has_normal)std::memcpy(expected.data()+8,source+6,4);
            assert(!std::memcmp(packed.src+v*packed.stride,expected.data(),packed.stride));
        }
        // A fresh source mutation must be visible; this is scratch storage, not a cache.
        memory[0x500000+7*10]^=0x57;Check(s,6,0,12,dense);
        // Declaration stride fallback still admits the same observed layout.
        s.streams[0].stride=0;s.vs->stream_stride[0]=10;Check(s,6,0,12,dense);
        for(unsigned type:{0x12u,0x03u,0xFFFFu}){
            s=PackedState(layout);s.vs->inputs[0].type=type;Check(s,6,0,12,dense);
            assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(dense),raw)&&!raw.repacked);
        }
        for(unsigned stride:{0u,11u,12u}){
            s=PackedState(layout);s.streams[0].stride=stride;Check(s,6,0,12,dense);
            assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(dense),raw)&&!raw.repacked);
        }
        s=PackedState(layout);s.vs->inputs[7].type=0x42;Check(s,6,0,12,dense);
        assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(dense),raw)&&!raw.repacked);
        s=PackedState(layout);s.streams[4].stride=9;Check(s,6,0,12,dense);
        assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(dense),raw)&&!raw.repacked);
        // Reusing the same output after packing clears the route and its decoder.
        s=State();Check(s,6,0,12,dense);Check(s,0,0,12,dense);
    }
    s=PackedState(2);assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(dense),first));
    const std::vector<uint8_t> packed_saved(first.buffers[0].src,first.buffers[0].src+first.buffers[0].bytes);
    assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(sparse),second));
    assert(!std::memcmp(first.buffers[0].src,packed_saved.data(),packed_saved.size()));
    for(unsigned layout=0;layout<5;++layout){
        s=PackedState(layout);
        assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(dense),raw));
        assert(raw.buffer_count>1);
        assert(raw.buffers[raw.elements[7].buffer].bytes==(raw.vertex_count-1)*8+8);
        if(layout==1||layout==2)assert(raw.buffers[raw.elements[4].buffer].bytes==(raw.vertex_count-1)*8+16);
        // A SHORT3 host tail uncovered by another authored attribute stays fully packed.
        s.vs->inputs[8].type=0;Check(s,6,0,12,dense);
        assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(dense),raw)&&raw.buffer_count==1);
        s=PackedState(layout);Check(s,6,0,12,sparse);
        assert(gpu::Assemble(s,6,0,12,reinterpret_cast<const uint8_t*>(sparse),raw)&&raw.buffer_count==1);
    }
    // Duplicate or relocated stream0 attributes retain the generic copier.
    for(unsigned layout=0;layout<5;++layout)for(unsigned variant=0;variant<4;++variant){
        s=PackedState(layout);
        if(variant==0)s.vs->inputs[5]={0,0,0x31};
        if(variant==1)s.vs->inputs[6]={0,6,0x16};
        if(variant==2){s.vs->inputs[1].type=0;s.vs->inputs[6]={0,6,0x16};}
        if(variant==3){s.vs->inputs[0].type=0;s.vs->inputs[6]={0,0,0x31};}
        for(unsigned prim:{5u,6u}){Check(s,prim,0,12,dense);Check(s,prim,0,12,sparse);}
    }
#ifdef _WIN32
    // A position-only last vertex exposes six bytes, not eight. Guard the next
    // page so a widened source load fails even when decoded values would match.
    auto* guarded=static_cast<uint8_t*>(VirtualAlloc(nullptr,0x3000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    assert(guarded);
    DWORD old_protect=0;
    assert(VirtualProtect(guarded+0x2000,0x1000,PAGE_NOACCESS,&old_protect));
    kernel::g_base=guarded;
    for(bool normal:{false,true})for(unsigned count:{3u,34u}){
        s={};s.vs_handle=1;s.vs=std::make_shared<gpu::VertexShader>();
        s.streams[0]={kHeader,10};s.vs->inputs[0]={0,0,0x31};
        if(normal)s.vs->inputs[1]={0,6,0x16};
        const unsigned bytes=(count-1)*10+(normal?10:6),address=0x2000-bytes;
        kernel::W32(kHeader+4,address);
        for(unsigned i=0;i<bytes;++i)guarded[address+i]=uint8_t(i*31+7);
        Check(s,6,0,count,nullptr);
    }
    kernel::g_base=memory.data();
    assert(VirtualFree(guarded,0,MEM_RELEASE));
#endif
    CheckParticleQuads();
    CheckIndexDispatch();
    CheckStripPairs();
    std::printf("PASS vertex assembly: %u previous/new cases, exact authored values and primitive order; packed padding, fallback, mutation, particle quads and independent storage preserved (GPU decode tested separately)\n",checks);
    if(argc>1&&!std::strcmp(argv[1],"strip-pairs")){BenchStripPairs();return 0;}
    if(argc>1&&!std::strcmp(argv[1],"dispatch")){BenchIndexDispatch();return 0;}
    if(argc>1){
        s=State();
        for(unsigned vertices:{24u,200u,1000u})for(unsigned prim:{5u,6u})for(bool mixed:{false,true}){
            std::vector<uint16_t> indices(vertices*3);
            for(unsigned i=0;i<indices.size();++i)indices[i]=uint16_t(i%vertices);
            Check(s,prim,0,unsigned(indices.size()),indices.data());
            std::array<double,7> before{},after{};
            for(unsigned pass=0;pass<before.size();++pass){
                if(pass%2){after[pass]=Bench(gpu::Assemble,s,prim,indices,20000,mixed);before[pass]=Bench(original_vertex::Assemble,s,prim,indices,20000,mixed);}
                else {before[pass]=Bench(original_vertex::Assemble,s,prim,indices,20000,mixed);after[pass]=Bench(gpu::Assemble,s,prim,indices,20000,mixed);}
            }
            std::sort(before.begin(),before.end());std::sort(after.begin(),after.end());
            std::printf("BENCH actual Assemble %s %s (specialized baseline): %u converted vertices, %zu indices, 4 attributes | median before %.3f us after %.3f us (%.1f%%); synthetic per iteration, not FPS\n",
                prim==5?"triangle-list":"strip fallback",mixed?"converted+raw":"converted",vertices,indices.size(),before[3],after[3],100*(before[3]-after[3])/before[3]);
        }
        for(unsigned layout=0;layout<5;++layout)for(unsigned vertices:{34u,329u,1000u}){
            s=PackedState(layout);std::vector<uint16_t> indices(vertices*2-2);
            for(unsigned i=0;i<indices.size();++i)indices[i]=uint16_t(i%vertices);
            Check(s,6,0,unsigned(indices.size()),indices.data());
            std::array<double,7> before{},after{};
            for(unsigned pass=0;pass<before.size();++pass){
                if(pass%2){after[pass]=Bench(gpu::Assemble,s,6,indices,10000,false);before[pass]=Bench(original_vertex::Assemble,s,6,indices,10000,false);}
                else{before[pass]=Bench(original_vertex::Assemble,s,6,indices,10000,false);after[pass]=Bench(gpu::Assemble,s,6,indices,10000,false);}
            }
            std::sort(before.begin(),before.end());std::sort(after.begin(),after.end());
            std::printf("BENCH packed layout %u: %u vertices | median before %.3f us after %.3f us (%.1f%%); synthetic CPU assembly, not FPS\n",layout,vertices,before[3],after[3],100*(before[3]-after[3])/before[3]);
        }
    }
}
