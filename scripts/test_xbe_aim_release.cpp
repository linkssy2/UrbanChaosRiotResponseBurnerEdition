// Compile the actual recorder source without any guest/render/file dependencies.
// Release hooks must remain inert even with the private environment flag set.
#undef NDEBUG
#ifndef UC_BURNER_RELEASE
#error This fixture requires the production release policy
#endif
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <xmmintrin.h>
#include "cpu/x86_context.h"
#include "xbe_coop.h"
#include "../common/xbe_aim_diagnostic.cpp"

int main(){
    constexpr const char* path="release-aim-must-not-create.log";
    assert(!std::filesystem::exists(path));
    _putenv_s("BURNER_AIM_LOG",path);
    _putenv_s("UCRR_COOP_TEST","1");
    trial_coop::SetAvailable(true);assert(trial_coop::Request());trial_coop::SetActive(true);
    X86Context context{};context.ecx=0xffffffffu;context.esp=0xfffffffcu;
    const auto saved=context;const auto csr=_mm_getcsr();
    const auto* invalid=reinterpret_cast<const uint8_t*>(1);
    for(unsigned iteration=0;iteration<2;++iteration){
        TrialAimFireScope fire(context,invalid,iteration!=0);
        TrialAimShotScope shot(context,invalid);
        TrialAimReticleScope reticle(context,invalid);
        TrialAimImpact(context,invalid);TrialAimBullet(context,invalid);
        TrialAimRender(context,invalid);TrialAimQuad(context,invalid);
    }
    assert(!std::memcmp(&context,&saved,sizeof context)&&_mm_getcsr()==csr);
    assert(!std::filesystem::exists(path));
    std::puts("release aim hooks: no dependencies, guest reads, state writes or output despite diagnostic environment");
}
