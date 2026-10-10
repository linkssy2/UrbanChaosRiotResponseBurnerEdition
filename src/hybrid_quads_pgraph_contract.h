#ifndef UC_HYBRID_QUADS_CONTRACT_H
#define UC_HYBRID_QUADS_CONTRACT_H
/* Exact fixed original shaders: authored decal EXPOSURE_VERTEX and ordinary
 * 4EAFE0/4EB270 textured quad. Source selection/lifecycle is kept by CONTROL. */
static const uint32_t uc_quad_decal[][3]={{0xcd001bu,0x8371800u,0x20b08800u},{0xcd201bu,0x8373800u,0x20904800u},{0xcd401bu,0x8375800u,0x20a02800u},{0xcd601bu,0x8377800u,0x20901800u},{0x4ee41bu,0x7c365000u,0x2fb00000u},{0x647401au,0xc4355bffu,0x10a8e800u},{0x4fa01bu,0xdc376800u,0x20a0f818u},{0x2000600u,0x8001068u,0xe0a0e84cu},{0x87601au,0xc400286au,0xf0b0e801u}};
static const uint32_t uc_quad_textured[][3]={{0xed001bu,0x8371800u,0x20b08800u},{0xed201bu,0x8373800u,0x20904800u},{0xed401bu,0x8375800u,0x20902800u},{0xed601bu,0x8377800u,0x20b01800u},{0x47401au,0xc4355800u,0x20b0e800u},{0x620041bu,0x280013ffu,0x10a8f818u},{0x2000600u,0x800106cu,0xe0a0f84cu},{0x87601au,0xc400286au,0xf0b0e801u}};
static int uc_quad_code(const uint32_t p[136][4],unsigned start,const uint32_t (*code)[3],unsigned count){
 if(start>136-count)return 0;
 for(unsigned i=0;i<count;i++)if(((p[start+i][1]^code[i][0])&0x0fffffffu)||p[start+i][2]!=code[i][1]||((p[start+i][3]^code[i][2])&0xfffff87fu))return 0;
 return 1;
}
static unsigned uc_quad_kind(const uint32_t p[136][4],unsigned start,const uint32_t m[2048]){
 if((m[0x1e70/4]&0xfffffu)!=1)return 0;
 if(uc_quad_code(p,start,uc_quad_decal,9)&&(m[0x1e60/4]&255)==1&&
    m[0x260/4]==0xd4d81010u&&m[0xac0/4]==0xc4c80000u&&
    (m[0xaa0/4]&0xfffffu)==0xc0&&(m[0x1e40/4]&0xfffffu)==0xc0&&
    m[0x288/4]==0x200f0000u&&(m[0x28c/4]&0xffffff00u)==0x200c1c00u)return 1;
 if(uc_quad_code(p,start,uc_quad_textured,8)&&(m[0x1e60/4]&255)==2&&
    m[0x260/4]==0xd4d81010u&&m[0x264/4]==0&&m[0xac0/4]==0xc8200000u&&m[0xac4/4]==0xccc40000u&&
    (m[0xaa0/4]&0xfffffu)==0xc0&&!(m[0xaa4/4]&0xfffffu)&&
    (m[0x1e40/4]&0xfffffu)==0xc0&&(m[0x1e44/4]&0xfffffu)==0xc0&&
    m[0x288/4]==0x130c0300u&&(m[0x28c/4]&0xffffff00u)==0x1c00u)return 2;
 return 0;
}
#endif
