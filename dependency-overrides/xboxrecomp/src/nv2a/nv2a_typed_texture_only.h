/* Diagnostic removal, NOT a fidelity certificate. Later typed lighting upload,
 * validation and draw remain unchanged. Missing legacy 2D state can alter fallback
 * rendering, effect classification, residency and mip behavior. Default OFF. */
#ifdef UCRR_TYPED_TEXTURE_ONLY
static int nv_typed_texture_option=-1;
static uint64_t nv_typed_texture_skipped[4];
static void typed_texture_report(void){
    if(nv_typed_texture_option==1)fprintf(stderr,
        "[TYPED-TEXTURE-ONLY] diagnostic=1 skipped=%llu,%llu,%llu,%llu\n",
        (unsigned long long)nv_typed_texture_skipped[0],
        (unsigned long long)nv_typed_texture_skipped[1],
        (unsigned long long)nv_typed_texture_skipped[2],
        (unsigned long long)nv_typed_texture_skipped[3]);
}
static IDirect3DTexture8 *update_draw_texture(unsigned stage,int direct_gpu){
    if(nv_typed_texture_option<0){const char*s=getenv("UCRR_TYPED_TEXTURE_ONLY");
        nv_typed_texture_option=s&&!strcmp(s,"1");}
    if(nv_typed_texture_option&&direct_gpu&&stage<4&&g_pg.tex[stage].enabled&&
       nv_option_lighting_textures()&&!raw_draw_retain_enabled()&&!nv_removal.enabled){
        unsigned mode=(g_pg.shader_stage_program>>(5*stage))&31;
        if(mode==2||mode==3){
            ++nv_typed_texture_skipped[stage];
            nv_effect_stage[stage]=0;g_pg.tex[stage].host_levels=0;
            return NULL;
        }
    }
    return update_live_texture(stage);
}
#else
#define update_draw_texture(stage,direct_gpu) update_live_texture(stage)
#endif
