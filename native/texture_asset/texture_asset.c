#include "texture_asset.h"
#include <stdlib.h>
#include <string.h>

static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}

/* The existing proven xbox_unswizzle_rect masked-increment kernel, narrowed
 * to one/four-byte pixels with memcpy to avoid alignment/aliasing assumptions.
 * Provenance: third_party/xboxrecomp/src/d3d/d3d8_swizzle.h. */
static void linearize(uint8_t *dst, const uint8_t *src, uint32_t w, uint32_t h, uint32_t bytes) {
    uint32_t mx=0, my=0, slot=1, oy=0;
    for (uint32_t bit=1; bit<w || bit<h; bit<<=1) {
        if(bit<w) { mx|=slot; slot<<=1; }
        if(bit<h) { my|=slot; slot<<=1; }
    }
    for(uint32_t y=0; y<h; ++y) {
        uint32_t ox=0;
        for(uint32_t x=0; x<w; ++x) {
            memcpy(dst+((size_t)y*w+x)*bytes, src+(size_t)(ox+oy)*bytes, bytes);
            ox=(ox-mx)&mx;
        }
        oy=(oy-my)&my;
    }
}

UCTextureStatus uc_texture_decode(const void *bytes, size_t size, UCTextureAsset *out) {
    UCTextureAsset result={0};
    const uint8_t *p=(const uint8_t *)bytes;
    uint32_t w,h,levels,block=0,pixel_bytes=4,limit=1,largest;
    size_t total=0, offset=0;
    if(!out) return UC_TEXTURE_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(!p) return UC_TEXTURE_ARGUMENT;
    if(size<28) return UC_TEXTURE_TRUNCATED;
    if(read32(p+24)!=size-28) return UC_TEXTURE_FORMAT;
    result.source_selector=read32(p);
    result.source_flags=read32(p+20);
    /* 0x96090 selects cube construction by byte21, not by format selector.
     * Header+12 is retained only for the verified value1; no volume guess. */
    if(p[21] || read32(p+12)!=1) return UC_TEXTURE_UNSUPPORTED;
    switch(result.source_selector) {
    case 0: result.format=UC_TEXTURE_BC1_UNORM; block=8; break;
    case 1: result.format=UC_TEXTURE_BC2_UNORM; block=16; break;
    case 2: result.format=UC_TEXTURE_BC3_UNORM; block=16; break;
    case 3: result.format=UC_TEXTURE_P8_UINT; pixel_bytes=1; break;
    case 4: result.format=UC_TEXTURE_BGRA8_UNORM; break;
    default: return UC_TEXTURE_UNSUPPORTED; /* vertex/unknown */
    }
    w=read32(p+4); h=read32(p+8); levels=read32(p+16);
    if(!w || !h || (w&(w-1)) || (h&(h-1)) || !levels) return UC_TEXTURE_FORMAT;
    if(w>4096 || h>4096 || levels>UC_TEXTURE_MAX_MIPS) return UC_TEXTURE_LIMIT;
    largest=w>h?w:h;
    while(largest>1) { largest>>=1; ++limit; }
    if(levels>limit) return UC_TEXTURE_FORMAT;
    result.width=w; result.height=h; result.mip_count=levels;
    for(uint32_t i=0; i<levels; ++i) {
        UCTextureMip *m=&result.mips[i];
        m->width=w; m->height=h;
        m->row_pitch=block?((w+3)/4)*block:w*pixel_bytes;
        m->slice_pitch=m->row_pitch*(block?(h+3)/4:h);
        total+=m->slice_pitch;
        w=w>1?w/2:1; h=h>1?h/2:1;
    }
    /* Original 2D mip storage is packed. Audited asset exporter pads the end
     * to128 bytes. Accept tight data too; reject other unexplained layouts. */
    if(size-28!=total && size-28!=((total+127)&~(size_t)127)) return UC_TEXTURE_FORMAT;
    result.storage=(uint8_t *)malloc(total);
    if(!result.storage) return UC_TEXTURE_MEMORY;
    result.storage_bytes=total;
    for(uint32_t i=0; i<levels; ++i) {
        UCTextureMip *m=&result.mips[i];
        m->data=result.storage+offset;
        if(block) memcpy(result.storage+offset,p+28+offset,m->slice_pitch);
        else linearize(result.storage+offset,p+28+offset,m->width,m->height,pixel_bytes);
        offset+=m->slice_pitch;
    }
    *out=result;
    return UC_TEXTURE_OK;
}

UCTextureStatus uc_texture_load(const UCArchive *archive, size_t index,
    UCArchiveSource source, UCTextureAsset *out) {
    UCArchiveBytes bytes={0};
    UCTextureStatus status;
    if(!out) return UC_TEXTURE_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(uc_archive_read(archive,index,source,&bytes)!=UC_AR_OK) return UC_TEXTURE_ARCHIVE;
    status=uc_texture_decode(bytes.data,bytes.size,out);
    uc_archive_bytes_free(&bytes);
    return status;
}
void uc_texture_free(UCTextureAsset *texture) {
    if(texture) { free(texture->storage); memset(texture,0,sizeof(*texture)); }
}
const char *uc_texture_status(UCTextureStatus status) {
    static const char *const names[]={"ok","argument","truncated","format",
        "unsupported","limit","memory","archive"};
    return (unsigned)status<sizeof(names)/sizeof(names[0])?names[status]:"unknown";
}
