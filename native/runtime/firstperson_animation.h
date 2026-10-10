#pragma once
#include "model.h"
extern "C" {
#include "../mainplayer_presentation.h"
}

/* Resources outlive every controller and event reference, including events
 * emitted by a controller removed during the same update. The skeleton mask
 * is distinct from the model's pose-contribution mask. */
struct NativeFirstpersonClip {
    std::vector<uint8_t> bytes;
    UCAnimationAnnotations annotations{};
    ~NativeFirstpersonClip(){uc_animation_annotations_free(&annotations);}
};
struct NativeFirstpersonClips {
    UCAnimationTemplate catalog{},skeleton{};
    std::vector<std::unique_ptr<NativeFirstpersonClip>> clips;
    ~NativeFirstpersonClips();
    void load(const UCArchive *,const char *catalog_name,const char *skeleton_name);
    const NativeFirstpersonClip &find(const char *alias) const;
};
struct NativeFirstpersonController {
    UCAnimationSample sample{};
    UCAnimationController timing{};
    const NativeFirstpersonClip *resource=nullptr;
    ~NativeFirstpersonController(){uc_animation_sample_free(&sample);}
};
struct NativeFirstpersonAnimations {
    std::vector<std::unique_ptr<NativeFirstpersonController>> list;
    NativeFirstpersonController *current=nullptr;
    void apply(const NativeFirstpersonClips &,const UCMainplayerPresentationAction &);
    void update(NativeRenderer &,NativeAnimatedModel &,const NativeFirstpersonClips &,
                const UCAnimationBlendMath &,float delta,UCAnimationEventQueue &,NativeAnimatedModel *attachment=nullptr);
};
