#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <vector>

namespace uc_hd_ui::filter {
namespace detail {
inline bool Extent(uint32_t w, uint32_t h, size_t& bytes) {
    // UI preparation is deliberately smaller than the hardware texture limit.
    if (!w || !h || w > 4096 || h > 4096) return false;
    const uint64_t size = uint64_t(w) * h * 4;
    if (size > std::numeric_limits<size_t>::max()) return false;
    bytes = size_t(size); return true;
}
inline uint16_t U16(const uint8_t* p) { return uint16_t(p[0]) | uint16_t(p[1]) << 8; }
inline uint32_t U32(const uint8_t* p) { return uint32_t(U16(p)) | uint32_t(U16(p+2)) << 16; }
inline std::array<uint8_t,3> Bgr565(uint16_t c) {
    return {uint8_t(((c & 31) << 3) | ((c & 31) >> 2)), uint8_t((((c >> 5) & 63) << 2) | (((c >> 5) & 63) >> 4)), uint8_t(((c >> 11) << 3) | ((c >> 11) >> 2))};
}
inline float Cubic(float a, float b, float c, float d, float t) {
    const float value = b + 0.5f*t*((c-a) + t*((2*a-5*b+4*c-d) + t*(3*(b-c)+d-a)));
    // Limit to the two bracketing samples: no negative coverage, ringing, or
    // opacity spreading across a transparent atlas gutter from distant taps.
    return std::clamp(value, std::min(b,c), std::max(b,c));
}
inline uint8_t Byte(float f) { return uint8_t(std::clamp(std::floor(f+0.5f),0.f,255.f)); }
} // namespace detail

// Exact BC1/BC3 blocks to straight-alpha BGRA; integer-floor interpolation
// matches the existing Urban menu decoder. Source mip-chain matching is separate.
inline bool DecodeNative(uint32_t width, uint32_t height, uint32_t format,
                         std::span<const uint8_t> payload,
                         std::vector<uint8_t>& bgra) {
    size_t bytes=0;
    if (!detail::Extent(width,height,bytes)) return false;
    const bool bc3=format==15;
    if ((format!=12 && !bc3) || payload.size()!=uint64_t((width+3)/4)*((height+3)/4)*(bc3?16:8)) return false;
    std::vector<uint8_t> result;
    try { result.resize(bytes); } catch (const std::bad_alloc&) { return false; }
    {
        const uint32_t blocks_x=(width+3)/4,blocks_y=(height+3)/4;
        for (uint32_t by=0;by<blocks_y;++by) for (uint32_t bx=0;bx<blocks_x;++bx) {
            const auto* block=payload.data()+(size_t(by)*blocks_x+bx)*(bc3?16:8);
            std::array<uint8_t,8> alpha{};uint64_t alpha_bits=0;
            if (bc3) {
                alpha[0]=block[0];alpha[1]=block[1];
                if (alpha[0]>alpha[1]) for(unsigned i=1;i<7;++i)alpha[i+1]=uint8_t(((7-i)*alpha[0]+i*alpha[1])/7);
                else {for(unsigned i=1;i<5;++i)alpha[i+1]=uint8_t(((5-i)*alpha[0]+i*alpha[1])/5);alpha[6]=0;alpha[7]=255;}
                for(unsigned i=0;i<6;++i)alpha_bits|=uint64_t(block[2+i])<<(8*i);
            }
            const auto* rgb=block+(bc3?8:0);
            const uint16_t c0=detail::U16(rgb),c1=detail::U16(rgb+2);
            const uint32_t bits=detail::U32(rgb+4);
            std::array<std::array<uint8_t,3>,4> colors{detail::Bgr565(c0),detail::Bgr565(c1),{}, {}};
            for(unsigned c=0;c<3;++c) {
                if(bc3 || c0>c1) {colors[2][c]=uint8_t((2*colors[0][c]+colors[1][c])/3);colors[3][c]=uint8_t((colors[0][c]+2*colors[1][c])/3);}
                else colors[2][c]=uint8_t((colors[0][c]+colors[1][c])/2);
            }
            for(unsigned i=0;i<16;++i) {
                const uint32_t x=bx*4+i%4,y=by*4+i/4;
                if(x>=width || y>=height)continue;
                const unsigned index=(bits>>(i*2))&3;auto* p=result.data()+(size_t(y)*width+x)*4;
                std::copy_n(colors[index].data(),3,p);
                p[3]=bc3?alpha[(alpha_bits>>(i*3))&7]:uint8_t(c0<=c1 && index==3?0:255);
            }
        }
    }
    bgra.swap(result);return true;
}

// Existing atlas layout and UVs stay unchanged. The cubic is limited in each
// axis; fully transparent central neighborhoods remain transparent. It cannot
// isolate touching atlas cells without cell metadata, just as linear sampling
// cannot. Fonts with validated cells receive the separate contour pass below.
inline std::vector<uint8_t> Upscale(uint32_t width, uint32_t height, const std::vector<uint8_t>& bgra, unsigned scale) {
    size_t bytes=0;
    if((scale!=2 && scale!=4) || !detail::Extent(width,height,bytes) || bgra.size()!=bytes || bytes>std::numeric_limits<size_t>::max()/16)return {};
    using Pixel=std::array<float,4>;
    const uint32_t out_width=width*scale,out_height=height*scale;
    std::vector<uint8_t> output;
    std::array<std::vector<Pixel>,4> rows;
    try {output.resize(bytes*scale*scale);for(auto& row:rows)row.resize(out_width);}catch(const std::bad_alloc&){return {};}
    std::array<int,4> loaded{-1,-1,-1,-1};
    auto load=[&](int y)->const std::vector<Pixel>& {
        auto& row=rows[unsigned(y)%4];
        if(loaded[unsigned(y)%4]==y)return row;
        loaded[unsigned(y)%4]=y;
        for(uint32_t x=0;x<out_width;++x) {
            const float source=(float(x)+.5f)/float(scale)-.5f;const int first=int(std::floor(source));const float t=source-first;
            Pixel samples[4];
            for(int k=0;k<4;++k) {
                const int sx=std::clamp(first+k-1,0,int(width)-1);
                const auto* p=bgra.data()+(size_t(y)*width+unsigned(sx))*4;
                for(unsigned c=0;c<3;++c)samples[k][c]=float(p[c])*p[3]/255.f;
                samples[k][3]=p[3];
            }
            for(unsigned c=0;c<4;++c)row[x][c]=detail::Cubic(samples[0][c],samples[1][c],samples[2][c],samples[3][c],t);
        }
        return row;
    };
    for(uint32_t y=0;y<out_height;++y) {
        const float source=(float(y)+.5f)/float(scale)-.5f;const int first=int(std::floor(source));const float t=source-first;
        std::array<const std::vector<Pixel>*,4> samples;
        for(int k=0;k<4;++k)samples[k]=&load(std::clamp(first+k-1,0,int(height)-1));
        for(uint32_t x=0;x<out_width;++x) {
            Pixel p;for(unsigned c=0;c<4;++c)p[c]=detail::Cubic((*samples[0])[x][c],(*samples[1])[x][c],(*samples[2])[x][c],(*samples[3])[x][c],t);
            auto* out=output.data()+(size_t(y)*out_width+x)*4;out[3]=detail::Byte(p[3]);
            for(unsigned c=0;c<3;++c)out[c]=out[3]?detail::Byte(std::clamp(p[c],0.f,p[3])*255.f/p[3]):0;
        }
    }
    return output;
}
namespace font_detail {
struct Cell { float x0, y0, x1, y1; };
struct Point { float x, y; };
struct Edge { Point a, b; };

// An alpha-derived vector boundary, not a replacement alphabet. The original
// cell's peak retains intentionally faint glyphs. Saddles use the bilinear
// asymptotic decision; holes follow this coverage contour, not a guessed font.
inline std::vector<Edge> Contour(uint32_t width, std::span<const uint8_t> pixels,
                                 Cell cell, uint8_t peak, float level=.5f) {
    std::vector<Edge> edges;
    if (!peak) return edges;
    const float iso = float(peak) * std::max(level,.000001f);
    const int left=int(std::floor(cell.x0)),top=int(std::floor(cell.y0));
    const int right=int(std::ceil(cell.x1)),bottom=int(std::ceil(cell.y1));
    auto alpha = [&](int x, int y) -> float {
        if (x < left || x >= right || y < top || y >= bottom) return 0;
        return pixels[(size_t(y) * width + unsigned(x)) * 4 + 3];
    };
    for (int y = top-1; y < bottom; ++y) {
        for (int x = left-1; x < right; ++x) {
            const float v[4]{alpha(x,y), alpha(x+1,y), alpha(x+1,y+1), alpha(x,y+1)};
            unsigned bits = 0;
            for (unsigned i=0;i<4;++i) if(v[i]>=iso) bits |= 1u<<i;
            if (!bits || bits == 15) continue;
            const Point p[4]{{x+.5f,y+.5f},{x+1.5f,y+.5f},{x+1.5f,y+1.5f},{x+.5f,y+1.5f}};
            auto cross = [&](unsigned e) {
                const unsigned n=(e+1)&3;
                const float t=(iso-v[e])/(v[n]-v[e]);
                return Point{p[e].x+t*(p[n].x-p[e].x),p[e].y+t*(p[n].y-p[e].y)};
            };
            auto add = [&](unsigned a,unsigned b) { edges.push_back({cross(a),cross(b)}); };
            switch(bits) {
            case 1: case 14: add(3,0);break;
            case 2: case 13: add(0,1);break;
            case 3: case 12: add(3,1);break;
            case 4: case 11: add(1,2);break;
            case 6: case 9: add(0,2);break;
            case 7: case 8: add(3,2);break;
            case 5: case 10:
                if ((v[0]-iso)*(v[2]-iso) >= (v[1]-iso)*(v[3]-iso)) {add(0,1);add(2,3);}
                else {add(3,0);add(1,2);}break;
            }
        }
    }
    return edges;
}

inline uint64_t DrawCell(uint32_t width, std::span<const uint8_t> source,
                        Cell cell, std::vector<uint8_t>& output, float level=.5f) {
    uint8_t peak=0;
    for(unsigned y=unsigned(std::floor(cell.y0));y<unsigned(std::ceil(cell.y1));++y)
      for(unsigned x=unsigned(std::floor(cell.x0));x<unsigned(std::ceil(cell.x1));++x)
        peak=std::max(peak,source[(size_t(y)*width+x)*4+3]);
    const auto edges=Contour(width,source,cell,peak,level);
    const unsigned first=unsigned(cell.x0*4),last=unsigned(cell.x1*4),out_width=width*4;
    std::vector<float> coverage(last-first), crossings;
    // Eight exact horizontal interval integrations per output row. This samples
    // vector edges at the new resolution; it does not enlarge source antialiasing.
    constexpr unsigned samples=8;
    uint64_t area=0;
    for(unsigned y=unsigned(cell.y0*4);y<unsigned(cell.y1*4);++y) {
        std::fill(coverage.begin(),coverage.end(),0.f);
        for(unsigned s=0;s<samples;++s) {
            const float sy=(float(y)+(s+.5f)/samples)*.25f;
            crossings.clear();
            for(const auto& e:edges) {
                if ((e.a.y<=sy && sy<e.b.y) || (e.b.y<=sy && sy<e.a.y))
                    crossings.push_back(4*(e.a.x+(sy-e.a.y)*(e.b.x-e.a.x)/(e.b.y-e.a.y)));
            }
            std::sort(crossings.begin(),crossings.end());
            for(size_t i=0;i+1<crossings.size();i+=2) {
                const float lo=std::clamp(crossings[i],float(first),float(last));
                const float hi=std::clamp(crossings[i+1],float(first),float(last));
                for(unsigned x=unsigned(std::floor(lo));x<std::min(last,unsigned(std::ceil(hi)));++x)
                    coverage[x-first]+=std::max(0.f,std::min(hi,float(x+1))-std::max(lo,float(x)))/samples;
            }
        }
        for(unsigned x=first;x<last;++x) {
            auto* p=output.data()+(size_t(y)*out_width+x)*4;
            p[3]=detail::Byte(coverage[x-first]*peak);
            area+=p[3];
        }
    }
    return area;
}

inline void RestoreCell(uint32_t width, std::span<const uint8_t> source,
                        Cell cell, std::vector<uint8_t>& output) {
    const unsigned first_x=unsigned(cell.x0*4),first_y=unsigned(cell.y0*4);
    const unsigned last_x=unsigned(cell.x1*4),last_y=unsigned(cell.y1*4);
    std::vector<uint8_t> was_clear(size_t(last_x-first_x)*(last_y-first_y));
    for(unsigned y=first_y;y<last_y;++y)for(unsigned x=first_x;x<last_x;++x)
        was_clear[size_t(y-first_y)*(last_x-first_x)+x-first_x]=!output[(size_t(y)*width*4+x)*4+3];
    double source_area=0;
    for(unsigned y=unsigned(std::floor(cell.y0));y<unsigned(std::ceil(cell.y1));++y)
      for(unsigned x=unsigned(std::floor(cell.x0));x<unsigned(std::ceil(cell.x1));++x) {
        const double wx=std::min(cell.x1,float(x+1))-std::max(cell.x0,float(x));
        const double wy=std::min(cell.y1,float(y+1))-std::max(cell.y0,float(y));
        source_area+=wx*wy*source[(size_t(y)*width+x)*4+3];
    }
    const double target=source_area*16;
    float lo=0,hi=1,best=.5f,above=.5f;
    double error=std::numeric_limits<double>::max();
    uint64_t above_area=std::numeric_limits<uint64_t>::max();
    // Match the original integrated coverage; a fixed 50% contour can inflate
    // stroke weight. This derives a bounded silhouette, not an unknown font.
    for(unsigned i=0;i<13;++i) {
        const float level=(lo+hi)*.5f;
        const uint64_t area=DrawCell(width,source,cell,output,level);
        const double difference=std::abs(double(area)-target);
        if(difference<error){error=difference;best=level;}
        if(area>=target && area<above_area){above_area=area;above=level;}
        if(area>target)lo=level;else hi=level;
    }
    const uint64_t area=DrawCell(width,source,cell,output,
        above_area!=std::numeric_limits<uint64_t>::max()?above:best);
    // A constant-alpha patch can make the contour area jump at one level. Keep
    // the closest covering contour and lower its opacity, never inflate weight.
    if(area>target) {
        const double ratio=target/area;
        for(unsigned y=unsigned(cell.y0*4);y<unsigned(cell.y1*4);++y)
          for(unsigned x=unsigned(cell.x0*4);x<unsigned(cell.x1*4);++x) {
            auto& a=output[(size_t(y)*width*4+x)*4+3];
            a=detail::Byte(float(a*ratio));
        }
    }
    // New contour coverage may enter a pixel whose filtered alpha rounded to
    // zero. Recover its original premultiplied colour instead of adding black.
    for(unsigned y=first_y;y<last_y;++y)for(unsigned x=first_x;x<last_x;++x) {
        auto* p=output.data()+(size_t(y)*width*4+x)*4;
        if(!p[3]){p[0]=p[1]=p[2]=0;continue;}
        if(!was_clear[size_t(y-first_y)*(last_x-first_x)+x-first_x])continue;
        const float sx=(x+.5f)*.25f-.5f,sy=(y+.5f)*.25f-.5f;
        const int ix=int(std::floor(sx)),iy=int(std::floor(sy));
        float alpha=0,colour[3]{};
        for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx) {
            const int px=std::clamp(ix+int(dx),int(cell.x0),int(cell.x1)-1);
            const int py=std::clamp(iy+int(dy),int(cell.y0),int(cell.y1)-1);
            const auto* q=source.data()+(size_t(py)*width+unsigned(px))*4;
            const float weight=(dx?sx-ix:1-(sx-ix))*(dy?sy-iy:1-(sy-iy))*q[3];
            alpha+=weight;for(unsigned c=0;c<3;++c)colour[c]+=weight*q[c];
        }
        if(alpha>0)for(unsigned c=0;c<3;++c)p[c]=detail::Byte(colour[c]/alpha);
        else {
            float nearest=std::numeric_limits<float>::max();
            for(unsigned py=unsigned(cell.y0);py<unsigned(cell.y1);++py)
                for(unsigned px=unsigned(cell.x0);px<unsigned(cell.x1);++px) {
                    const auto* q=source.data()+(size_t(py)*width+px)*4;
                    const float distance=(px-sx)*(px-sx)+(py-sy)*(py-sy);
                    if(q[3]&&distance<nearest){nearest=distance;std::copy_n(q,3,p);}
                }
        }
    }
}

} // namespace font_detail
} // namespace uc_hd_ui::filter
