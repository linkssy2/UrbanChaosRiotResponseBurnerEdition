#pragma once
#include "model.h"
extern "C" {
#include "../gunflare.h"
#include "../firstperson_projection.h"
}

/* Concrete model endpoints for the first-person187680 resource recipe.
 * Two exact one-group, unanimated AMFs; source construction order is handgun,
 * rifle, while linked-list traversal is reversed. Entity registration/events,
 * parent/world ownership and particle services remain with the caller.
 * Resources are loaded once. Transform/tint/visibility change retained state;
 * drawing neither advances GunFlare nor consumes its shared random stream. */
struct NativeGunFlareModels {
    struct Instance {
        NativeAnimatedModel model;
        float world[16]{},tint[4]{1,1,1,1};
        bool visible=false;
    } instances[2];
    void *handles[2]{};
    UCGunFlareModels groups[2]{};
    bool ready=false;
    NativeGunFlareModels()=default;
    NativeGunFlareModels(const NativeGunFlareModels&)=delete;
    NativeGunFlareModels& operator=(const NativeGunFlareModels&)=delete;
    void load(NativeRenderer &,const UCArchive *,const uint8_t hash[32],
              const std::filesystem::path &catalog,const float world[16]);
    int tint(void *model,const float rgba[4]);
    int visible(void *model,unsigned enabled);
    int transform(void *model,const float world[16]);
    /* Caller supplies the actual camera view/projection, before model depth
     * slicing, and positive inverse exposure. The .1/.15 range is draw-local.
     * Existing preview renderer parent state is retained; no collector/sort,
     * original full-frame depth ancestry or first-person placement claim. */
    void draw(NativeRenderer &,const float view[16],const float projection[16],float inverse_exposure);
private:
    Instance *find(void *);
};
