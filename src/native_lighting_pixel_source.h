/* Fixed, source-qualified lighting formulas. Runtime never decodes a shader program here. */
typedef struct {unsigned stages;unsigned char digest[32];const char *name;const char *source;} UcNativeLightPixel;
static const UcNativeLightPixel uc_native_light_pixels[]={
{1,{0x80,0xec,0x97,0xa3,0xa6,0x20,0xd5,0x1e,0xd0,0x84,0x0c,0x85,0x98,0x58,0x31,0x67,0xae,0x0a,0x16,0x7a,0x24,0x4a,0x9c,0x55,0x7c,0x9f,0x0e,0xa7,0x14,0xeb,0xd1,0x28},"shadow_mask",
"float3 lit0=clamp((c0[0].rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float lit1=clamp(((1.0-saturate(saturate(input.fog)))*c1[0].a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(lit0,0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(lit0,0.0))*max(float3(0,0,0),0.0)),max(lit1,0.0));\n"},
{3,{0x58,0xe7,0x92,0xbb,0xe2,0x59,0xc6,0x31,0x52,0xf4,0x93,0xf4,0x7c,0x73,0x28,0x8e,0xde,0x6f,0x61,0x25,0xe7,0x7d,0xf3,0x42,0x12,0xde,0x84,0x57,0x92,0x32,0x5f,0x1a},"directional_diffuse",
"float3 lit0=clamp(((c0[0].rgb*(1.0-saturate(float3(0,0,0))))+((-c1[0].rgb)*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float3 lit2=clamp(dot(t2.rgb,lit0),-1.0,1.0);\n"
"float3 lit4=clamp((lit2*c0[2].rgb),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit4,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{3,{0xc8,0xfb,0x1b,0x7e,0x37,0x99,0x83,0x29,0xff,0x62,0x76,0x4f,0x52,0xd9,0x86,0xa5,0xe4,0x4c,0x88,0x68,0xce,0x53,0x7f,0xd5,0x8e,0x60,0x17,0xeb,0x0f,0x73,0x1e,0xbb},"directional_normal_fog0_specular0",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float lit3=clamp((0.0*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(lit2,0.0)+max(float3(0,0,0),0.0)*max(float3(0,0,0),0.0)+(1.0-max(float3(0,0,0),0.0))*max(float3(0,0,0),0.0)),max(lit3,0.0));\n"},
{3,{0x63,0x90,0xe2,0xec,0x65,0xcc,0x5e,0x47,0x2a,0x21,0x34,0x54,0x68,0x14,0x03,0xa6,0x90,0x34,0xc4,0x1a,0xe6,0xd3,0xd6,0x69,0x1c,0x46,0xff,0xda,0x2b,0x98,0x32,0xcc},"directional_normal_fog0_specular1",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float lit3=clamp((0.0*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(saturate((max(input.color1.rgb,0.0)+max(lit2,0.0))),0.0)+max(float3(0,0,0),0.0)*max(float3(0,0,0),0.0)+(1.0-max(float3(0,0,0),0.0))*max(float3(0,0,0),0.0)),max(lit3,0.0));\n"},
{3,{0x07,0x0f,0x96,0xa2,0xa9,0x9d,0xaf,0xab,0x2d,0xfc,0x54,0xa1,0x70,0x03,0x1f,0xf3,0x7f,0xf4,0x7f,0xdf,0x04,0x6d,0xdb,0x3c,0xe5,0x14,0x6c,0xa7,0xb1,0xda,0x7e,0x5b},"directional_normal_fog1_specular0",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float lit3=clamp((0.0*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(float3(saturate(input.fog),saturate(input.fog),saturate(input.fog)),0.0)*max(lit2,0.0)+(1.0-max(float3(saturate(input.fog),saturate(input.fog),saturate(input.fog)),0.0))*max(fog_color.rgb,0.0)),max(lit3,0.0));\n"},
{3,{0x24,0x12,0x27,0xff,0xe4,0x99,0x66,0xd5,0x99,0xfb,0x79,0x7a,0x37,0x03,0xc8,0xd2,0x83,0x7a,0x2f,0x36,0xb8,0xf6,0x7a,0xf3,0x4e,0x29,0xea,0x48,0xd5,0x95,0x8c,0xac},"directional_normal_fog1_specular1",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float lit3=clamp((0.0*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(float3(saturate(input.fog),saturate(input.fog),saturate(input.fog)),0.0)*max(saturate((max(input.color1.rgb,0.0)+max(lit2,0.0))),0.0)+(1.0-max(float3(saturate(input.fog),saturate(input.fog),saturate(input.fog)),0.0))*max(fog_color.rgb,0.0)),max(lit3,0.0));\n"},
{4,{0x24,0xc1,0xe0,0x08,0xe3,0xa5,0xef,0x75,0x2d,0x48,0x0c,0x35,0x54,0x57,0x81,0x97,0x4c,0x42,0x1f,0x59,0x42,0x3e,0x6c,0x96,0x4f,0xbd,0x58,0x9c,0xf0,0x50,0x7f,0x5f},"directional_masked",
"float3 lit0=clamp(dot(c0[0].rgb,t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float3 lit3=clamp((lit2*float3(t0.a,t0.a,t0.a)),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max((max(lit3,0.0)*max(t0.rgb,0.0)),0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{4,{0x4a,0x6a,0x06,0x22,0x15,0x4a,0x25,0xb7,0x15,0xb1,0x6c,0x15,0xce,0xb2,0x33,0x28,0x33,0xe0,0x74,0xc0,0xaf,0x47,0x25,0xc3,0xeb,0xf5,0x9d,0x05,0xb6,0x6d,0x12,0x17},"directional_specular",
"float3 lit0=clamp(((c0[0].rgb*(1.0-saturate(float3(0,0,0))))+((-c1[0].rgb)*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float3 lit2=clamp(dot(t2.rgb,lit0),-1.0,1.0);\n"
"float3 lit3=clamp(dot(t3.rgb,lit0),-1.0,1.0);\n"
"float3 lit4=clamp((((max(lit2,0.0)*(1.0-saturate(float3(0,0,0))))+(max(lit2,0.0)*(1.0-saturate(float3(0,0,0)))))*2.0),-1.0,1.0);\n"
"float lit5=clamp(((lit3).b*c0[2].a),-1.0,1.0);\n"
"float3 lit6=clamp((lit2*c0[3].rgb),-1.0,1.0);\n"
"float lit7=clamp((lit5*(lit4).b),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit6,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit7,0.0));\n"},
{4,{0x04,0xd8,0x5c,0x98,0x55,0xc9,0xda,0x2e,0x81,0x08,0x96,0x79,0xb9,0x73,0x5a,0x58,0x9a,0xd1,0x66,0x22,0x89,0x51,0x25,0x99,0x2a,0x98,0xfe,0xd3,0x87,0x82,0x3b,0x28},"directional_normal_specular",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t3.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((float3(t2.a,t2.a,t2.a)*c0[0].rgb),-1.0,1.0);\n"
"float lit2=clamp((t2.a*c1[0].a),-1.0,1.0);\n"
"float3 lit3=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float lit4=clamp((max((lit0).b,0.0)*max((lit0).b,0.0)),-1.0,1.0);\n"
"float lit5=clamp((lit4*lit4),-1.0,1.0);\n"
"float3 lit6=clamp((lit3*lit1),-1.0,1.0);\n"
"float lit7=clamp((lit5*lit2),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit6,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit7,0.0));\n"},
{6,{0x1d,0xe8,0xf2,0x82,0xea,0x9f,0x2e,0x0a,0x5e,0x72,0x95,0x5b,0xa4,0xeb,0x4e,0x78,0xc6,0x1a,0x3f,0xec,0x9a,0x6d,0xe0,0x60,0x0e,0x4f,0xab,0x14,0xf2,0x83,0xc1,0xe3},"directional_masked_specular",
"float3 lit0=clamp(dot(c0[0].rgb,t3.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((float3(t2.a,t2.a,t2.a)*c1[0].rgb),-1.0,1.0);\n"
"float lit2=clamp((t2.a*c0[1].a),-1.0,1.0);\n"
"float3 lit3=clamp(dot(c0[2].rgb,t2.rgb),-1.0,1.0);\n"
"float lit4=clamp((max((lit0).b,0.0)*max((lit0).b,0.0)),-1.0,1.0);\n"
"float lit5=clamp((lit4*lit4),-1.0,1.0);\n"
"float3 lit6=clamp((lit3*lit1),-1.0,1.0);\n"
"float lit7=clamp((lit5*lit2),-1.0,1.0);\n"
"float3 lit8=clamp((lit6*float3(t0.a,t0.a,t0.a)),-1.0,1.0);\n"
"float lit9=clamp((lit7*t0.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max((max(lit8,0.0)*max(t0.rgb,0.0)),0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit9,0.0));\n"},
{4,{0x94,0x83,0x5e,0x22,0x42,0x68,0xdd,0xff,0x33,0xb7,0x48,0xc6,0xd2,0xbf,0x3e,0x08,0x02,0x7a,0x28,0xc2,0x9a,0x2b,0x66,0xb4,0xa0,0x35,0xc2,0x34,0x0e,0xec,0xef,0x0e},"point_diffuse",
"float3 lit0=clamp(((c0[0].rgb*(1.0-saturate(float3(0,0,0))))+((-c1[0].rgb)*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float3 lit2=clamp(dot(t2.rgb,lit0),-1.0,1.0);\n"
"float3 lit4=clamp((lit2*c0[2].rgb),-1.0,1.0);\n"
"float3 lit5=clamp((lit4*t1.rgb),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit5,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{4,{0x3c,0xfb,0x54,0xb6,0x64,0xaa,0x3b,0x64,0x36,0x08,0x57,0xbc,0x13,0x65,0xb9,0x01,0xd6,0x71,0xb8,0x69,0xa2,0xb1,0xc4,0xea,0x65,0xd8,0x37,0x26,0x96,0x41,0x76,0xa0},"point_normal_fog0_specular0",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float lit3=clamp((0.0*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float3 lit4=clamp((lit2*t1.rgb),-1.0,1.0);\n"
"float lit5=clamp((lit3*t1.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(lit4,0.0)+max(float3(0,0,0),0.0)*max(float3(0,0,0),0.0)+(1.0-max(float3(0,0,0),0.0))*max(float3(0,0,0),0.0)),max(lit5,0.0));\n"},
{4,{0xfd,0xa6,0xe0,0xaa,0x8b,0xb2,0x98,0x5a,0x9b,0x75,0x64,0x1b,0xb2,0x77,0x01,0x69,0x73,0x65,0x2c,0xaf,0xeb,0x2b,0x53,0x42,0x0d,0xcb,0x74,0x59,0xda,0x29,0x00,0x7e},"point_normal_fog0_specular1",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float lit3=clamp((0.0*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float3 lit4=clamp((lit2*t1.rgb),-1.0,1.0);\n"
"float lit5=clamp((lit3*t1.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(saturate((max(input.color1.rgb,0.0)+max(lit4,0.0))),0.0)+max(float3(0,0,0),0.0)*max(float3(0,0,0),0.0)+(1.0-max(float3(0,0,0),0.0))*max(float3(0,0,0),0.0)),max(lit5,0.0));\n"},
{4,{0x08,0xbb,0x2d,0x9e,0x03,0x0a,0x51,0xaa,0xc1,0xa7,0xaf,0x13,0xac,0xaa,0xce,0xc8,0x9b,0xc2,0x44,0x51,0xdf,0xc3,0xb7,0xb3,0xb6,0x6a,0x61,0x67,0x34,0x3d,0xc6,0xaa},"point_normal_fog1_specular0",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float lit3=clamp((0.0*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float3 lit4=clamp((lit2*t1.rgb),-1.0,1.0);\n"
"float lit5=clamp((lit3*t1.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(float3(saturate(input.fog),saturate(input.fog),saturate(input.fog)),0.0)*max(lit4,0.0)+(1.0-max(float3(saturate(input.fog),saturate(input.fog),saturate(input.fog)),0.0))*max(fog_color.rgb,0.0)),max(lit5,0.0));\n"},
{4,{0xa0,0xe2,0x7c,0xcc,0x25,0xcf,0xae,0xef,0x84,0x18,0xe0,0xc5,0x21,0xc0,0x55,0x3f,0x53,0x4e,0xd1,0xdc,0x62,0xa8,0xd5,0x22,0x27,0xfd,0x36,0x2c,0xe6,0xcd,0x88,0x7c},"point_normal_fog1_specular1",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float lit3=clamp((0.0*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float3 lit4=clamp((lit2*t1.rgb),-1.0,1.0);\n"
"float lit5=clamp((lit3*t1.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(float3(saturate(input.fog),saturate(input.fog),saturate(input.fog)),0.0)*max(saturate((max(input.color1.rgb,0.0)+max(lit4,0.0))),0.0)+(1.0-max(float3(saturate(input.fog),saturate(input.fog),saturate(input.fog)),0.0))*max(fog_color.rgb,0.0)),max(lit5,0.0));\n"},
{5,{0x94,0x1e,0x97,0x12,0x4b,0xee,0x97,0xd7,0x3e,0x7e,0x49,0x01,0x64,0x93,0x55,0x8a,0xa6,0x22,0x19,0xa3,0x83,0x64,0x15,0x49,0x2f,0x67,0x49,0x32,0x30,0x06,0xaa,0xac},"point_masked",
"float3 lit0=clamp(dot(c0[0].rgb,t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit1*float3(t2.a,t2.a,t2.a)),-1.0,1.0);\n"
"float3 lit4=clamp((lit2*t1.rgb),-1.0,1.0);\n"
"float3 lit6=clamp((lit4*float3(t0.a,t0.a,t0.a)),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max((max(lit6,0.0)*max(t0.rgb,0.0)),0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{5,{0xd8,0xb8,0x81,0x5f,0xea,0x93,0x0a,0x56,0xc1,0x53,0x3b,0x73,0xa5,0x0d,0xf0,0x4f,0x83,0xf2,0xcd,0xe3,0x59,0xb3,0x62,0x80,0x58,0x48,0x39,0x3c,0x4d,0x34,0xae,0xcd},"point_specular",
"float3 lit0=clamp(((c0[0].rgb*(1.0-saturate(float3(0,0,0))))+((-c1[0].rgb)*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float3 lit2=clamp(dot(t2.rgb,lit0),-1.0,1.0);\n"
"float3 lit3=clamp(dot(t3.rgb,lit0),-1.0,1.0);\n"
"float3 lit4=clamp((((max(lit2,0.0)*(1.0-saturate(float3(0,0,0))))+(max(lit2,0.0)*(1.0-saturate(float3(0,0,0)))))*2.0),-1.0,1.0);\n"
"float lit5=clamp(((lit3).b*c0[2].a),-1.0,1.0);\n"
"float3 lit6=clamp((lit2*c0[3].rgb),-1.0,1.0);\n"
"float lit7=clamp((lit5*(lit4).b),-1.0,1.0);\n"
"float3 lit8=clamp((lit6*t1.rgb),-1.0,1.0);\n"
"float lit9=clamp((lit7*t1.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit8,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit9,0.0));\n"},
{5,{0xa4,0x15,0xdb,0xdf,0xfa,0x6f,0xbf,0xed,0x6f,0x6e,0x79,0x34,0x93,0x59,0x82,0x84,0xb8,0xe5,0xca,0x14,0x7c,0x1a,0x1b,0x85,0x8a,0xf4,0x7b,0x5d,0x58,0xb3,0x87,0x8f},"point_normal_specular",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t3.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((float3(t2.a,t2.a,t2.a)*c0[0].rgb),-1.0,1.0);\n"
"float lit2=clamp((t2.a*c1[0].a),-1.0,1.0);\n"
"float3 lit3=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float lit4=clamp((max((lit0).b,0.0)*max((lit0).b,0.0)),-1.0,1.0);\n"
"float lit5=clamp((lit4*lit4),-1.0,1.0);\n"
"float3 lit6=clamp((lit3*lit1),-1.0,1.0);\n"
"float lit7=clamp((lit5*lit2),-1.0,1.0);\n"
"float3 lit8=clamp((lit6*t1.rgb),-1.0,1.0);\n"
"float lit9=clamp((lit7*t1.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit8,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit9,0.0));\n"},
{7,{0x4d,0xfe,0xbe,0x26,0x07,0x38,0x7b,0x3f,0x5b,0xb7,0x39,0x96,0x65,0xce,0x83,0x02,0x1b,0x0c,0x21,0x07,0x5c,0x5f,0x3c,0x26,0x72,0xc2,0x51,0xe3,0xcc,0x46,0x46,0x72},"point_masked_specular",
"float3 lit0=clamp(dot(c0[0].rgb,t3.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((float3(t2.a,t2.a,t2.a)*c1[0].rgb),-1.0,1.0);\n"
"float lit2=clamp((t2.a*c0[1].a),-1.0,1.0);\n"
"float3 lit3=clamp(dot(c0[2].rgb,t2.rgb),-1.0,1.0);\n"
"float lit4=clamp((max((lit0).b,0.0)*max((lit0).b,0.0)),-1.0,1.0);\n"
"float lit5=clamp((lit4*lit4),-1.0,1.0);\n"
"float3 lit6=clamp((lit3*lit1),-1.0,1.0);\n"
"float lit7=clamp((lit5*lit2),-1.0,1.0);\n"
"float3 lit8=clamp((lit6*t1.rgb),-1.0,1.0);\n"
"float lit9=clamp((lit7*t1.a),-1.0,1.0);\n"
"float3 lit10=clamp((lit8*float3(t0.a,t0.a,t0.a)),-1.0,1.0);\n"
"float lit11=clamp((lit9*t0.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max((max(lit10,0.0)*max(t0.rgb,0.0)),0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit11,0.0));\n"},
{4,{0x0d,0x39,0x9b,0xe9,0x43,0xe5,0x82,0x21,0x95,0x55,0x90,0xbb,0xba,0x14,0xb6,0x7d,0x08,0x79,0x40,0x31,0xd3,0x85,0x9d,0x7f,0x4b,0x87,0x68,0xa7,0x11,0x2c,0x34,0x5a},"spot_diffuse",
"float3 lit0=clamp(dot(t2.rgb,t3.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((float3(t1.a,t1.a,t1.a)*c0[0].rgb),-1.0,1.0);\n"
"float3 lit3=clamp((lit0*lit1),-1.0,1.0);\n"
"float lit4=clamp((((lit0).b*(1.0-saturate(0.0)))+(c0[1].a*(2.0*max(0.0,0.0)-1.0))),-1.0,1.0);\n"
"float3 lit5=clamp((lit3*input.color0.rgb),-1.0,1.0);\n"
"float lit6=clamp((((max(lit4,0.0)*(1.0-saturate(0.0)))+(max(lit4,0.0)*(1.0-saturate(0.0))))*2.0),-1.0,1.0);\n"
"float3 lit7=clamp((lit5*float3(lit6,lit6,lit6)),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit7,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{4,{0x51,0x65,0xbf,0xbf,0x7d,0xb8,0xc0,0x78,0x70,0x39,0xa9,0x20,0x9a,0x61,0x74,0x37,0xed,0xbe,0xc3,0x96,0x86,0x14,0xd3,0xd1,0xd4,0x06,0xe5,0x96,0xab,0xc6,0xb6,0xdb},"spot_normal",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((float3(t1.a,t1.a,t1.a)*c0[0].rgb),-1.0,1.0);\n"
"float3 lit3=clamp((lit0*lit1),-1.0,1.0);\n"
"float lit4=clamp((((lit0).b*(1.0-saturate(0.0)))+(c0[1].a*(2.0*max(0.0,0.0)-1.0))),-1.0,1.0);\n"
"float3 lit5=clamp((lit3*input.color0.rgb),-1.0,1.0);\n"
"float lit6=clamp((((max(lit4,0.0)*(1.0-saturate(0.0)))+(max(lit4,0.0)*(1.0-saturate(0.0))))*2.0),-1.0,1.0);\n"
"float3 lit7=clamp((lit5*float3(lit6,lit6,lit6)),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit7,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{4,{0xfb,0x13,0x4d,0x82,0xbc,0x9b,0x7c,0x24,0xdd,0x33,0xad,0x9a,0x9c,0x84,0x40,0x17,0xda,0xfd,0x8e,0x6a,0x7e,0x11,0xca,0xf2,0x53,0xcb,0x9f,0x22,0x25,0x22,0x71,0x15},"spot_masked",
"float3 lit0=clamp(dot(c0[0].rgb,t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp((float3(t1.a,t1.a,t1.a)*c1[0].rgb),-1.0,1.0);\n"
"float3 lit3=clamp((lit0*lit1),-1.0,1.0);\n"
"float3 lit4=clamp((float3(t0.a,t0.a,t0.a)*input.color0.rgb),-1.0,1.0);\n"
"float lit5=clamp(((lit0.b*(1.0-saturate(0.0)))+(c0[1].a*(2.0*max(0.0,0.0)-1.0))),-1.0,1.0);\n"
"float3 lit6=clamp((lit3*lit4),-1.0,1.0);\n"
"float lit7=clamp((((max(lit5,0.0)*(1.0-saturate(0.0)))+(max(lit5,0.0)*(1.0-saturate(0.0))))*2.0),-1.0,1.0);\n"
"float3 lit8=clamp((lit6*float3(lit7,lit7,lit7)),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max((max(lit8,0.0)*max(t0.rgb,0.0)),0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{6,{0xe8,0x91,0xe8,0x33,0x5d,0x4a,0xac,0x4b,0xc9,0x1f,0x71,0xb5,0x98,0x94,0x29,0x61,0x08,0x1d,0x8c,0x60,0xea,0x04,0x11,0x65,0xab,0x69,0x4a,0x20,0x58,0x2a,0x72,0x76},"spot_specular",
"float3 lit0=clamp(dot(t2.rgb,t3.rgb),-1.0,1.0);\n"
"float3 lit1=clamp(dot(t0.rgb,t2.rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit3=clamp((lit1*lit1),-1.0,1.0);\n"
"float lit4=clamp((((lit0).b*(1.0-saturate(0.0)))+(c1[1].a*(2.0*max(0.0,0.0)-1.0))),-1.0,1.0);\n"
"float3 lit5=clamp((lit3*lit3),-1.0,1.0);\n"
"float lit6=clamp((((max(lit4,0.0)*(1.0-saturate(0.0)))+(max(lit4,0.0)*(1.0-saturate(0.0))))*2.0),-1.0,1.0);\n"
"float3 lit7=clamp((lit5*lit5),-1.0,1.0);\n"
"float lit8=clamp((t1.a*input.color0.a),-1.0,1.0);\n"
"float3 lit9=clamp((float3(lit8,lit8,lit8)*float3(lit6,lit6,lit6)),-1.0,1.0);\n"
"float lit10=clamp(((lit7).b*c0[4].a),-1.0,1.0);\n"
"float3 lit11=clamp((lit2*lit9),-1.0,1.0);\n"
"float lit12=clamp((lit10*(lit9).b),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit11,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit12,0.0));\n"},
{6,{0x51,0xa6,0x17,0x17,0x15,0xf5,0x7b,0x92,0xba,0xff,0xe0,0x60,0x7b,0x04,0xc9,0x4d,0xb4,0xb3,0x61,0x25,0xa8,0xaa,0x4b,0x8f,0xaf,0x65,0xf2,0xf7,0xf5,0x39,0xbf,0x0b},"spot_normal_specular",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),t3.rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit3=clamp((lit1*lit1),-1.0,1.0);\n"
"float lit4=clamp((((lit0).b*(1.0-saturate(0.0)))+(c1[1].a*(2.0*max(0.0,0.0)-1.0))),-1.0,1.0);\n"
"float3 lit5=clamp((lit3*lit3),-1.0,1.0);\n"
"float lit6=clamp((((max(lit4,0.0)*(1.0-saturate(0.0)))+(max(lit4,0.0)*(1.0-saturate(0.0))))*2.0),-1.0,1.0);\n"
"float3 lit7=clamp((lit5*lit5),-1.0,1.0);\n"
"float lit8=clamp((t1.a*input.color0.a),-1.0,1.0);\n"
"float3 lit9=clamp((float3(lit8,lit8,lit8)*float3(lit6,lit6,lit6)),-1.0,1.0);\n"
"float lit10=clamp(((lit7).b*c0[4].a),-1.0,1.0);\n"
"float3 lit11=clamp((lit2*lit9),-1.0,1.0);\n"
"float lit12=clamp((lit10*(lit9).b),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit11,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit12,0.0));\n"},
{6,{0xe5,0x89,0xb9,0x49,0x62,0x74,0x13,0xe5,0x6f,0x94,0xa6,0x58,0x3d,0x29,0x4d,0xac,0x48,0x23,0xa6,0xb2,0xc1,0xe2,0x2a,0x8a,0xb7,0x8f,0x00,0x32,0x84,0x46,0x3e,0xee},"spot_masked_specular",
"float3 lit0=clamp(dot(c0[0].rgb,t2.rgb),-1.0,1.0);\n"
"float3 lit1=clamp(dot(c0[0].rgb,t3.rgb),-1.0,1.0);\n"
"float3 lit2=clamp((lit0*c0[1].rgb),-1.0,1.0);\n"
"float3 lit3=clamp((lit1*lit1),-1.0,1.0);\n"
"float lit4=clamp((((lit0).b*(1.0-saturate(0.0)))+(c1[1].a*(2.0*max(0.0,0.0)-1.0))),-1.0,1.0);\n"
"float3 lit5=clamp((lit3*lit3),-1.0,1.0);\n"
"float lit6=clamp((((max(lit4,0.0)*(1.0-saturate(0.0)))+(max(lit4,0.0)*(1.0-saturate(0.0))))*2.0),-1.0,1.0);\n"
"float3 lit7=clamp((lit5*lit5),-1.0,1.0);\n"
"float lit8=clamp((t1.a*input.color0.a),-1.0,1.0);\n"
"float lit9=clamp((lit6*t0.a),-1.0,1.0);\n"
"float3 lit10=clamp((float3(lit8,lit8,lit8)*float3(lit9,lit9,lit9)),-1.0,1.0);\n"
"float lit11=clamp(((lit7).b*c0[4].a),-1.0,1.0);\n"
"float3 lit12=clamp((lit2*lit10),-1.0,1.0);\n"
"float lit13=clamp((lit11*(lit10).b),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max((max(lit12,0.0)*max(t0.rgb,0.0)),0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit13,0.0));\n"},
{2,{0x8b,0xe7,0xc6,0xef,0x65,0xc4,0xf3,0x4d,0x14,0xe9,0x88,0xfa,0x10,0x87,0x6b,0x35,0xd9,0x95,0x35,0x30,0xc9,0x2e,0xa2,0x1b,0x04,0x56,0x52,0x38,0xd7,0x8b,0x02,0x55},"emissive_ambient",
"float3 lit0=clamp((c0[0].rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float3 lit2=clamp(((lit0*(1.0-saturate(float3(0,0,0))))+(t0.rgb*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(lit2,0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(lit2,0.0))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{2,{0xe5,0xee,0x72,0xde,0xcf,0x4f,0xc7,0xef,0x25,0x93,0x30,0x72,0xea,0x44,0x22,0xc7,0xc2,0xec,0xb7,0xa9,0xf2,0x81,0x1e,0xa5,0x4f,0x41,0xaa,0x70,0x9f,0x17,0xb6,0xc6},"material_51a118",
"float3 lit0=clamp((c0[0].rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float3 lit2=clamp(((lit0*(1.0-saturate(float3(0,0,0))))+(t0.rgb*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(lit2,0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(lit2,0.0))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{1,{0x19,0xe3,0x2b,0x8d,0xbc,0x2a,0xb8,0xf1,0x76,0x52,0x02,0x0c,0x56,0x5f,0x15,0xc8,0xa4,0xff,0xfa,0x33,0xcd,0xa7,0x5e,0x95,0xf1,0x5e,0x52,0xe6,0xde,0x0b,0xab,0x65},"material_51a220",
"float3 lit0=clamp((c0[0].rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(lit0,0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(lit0,0.0))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{2,{0x2b,0x35,0x41,0xe7,0x62,0xac,0x4a,0x07,0x51,0x5a,0x32,0xca,0xca,0x18,0xca,0xab,0xbb,0x59,0xcd,0xca,0xb2,0x01,0x75,0x36,0xad,0x05,0x2b,0x14,0x11,0x62,0xba,0xfa},"material_51a1e0",
"float3 lit0=clamp(((t1.rgb*(1.0-saturate(float3(0,0,0))))+(c0[0].rgb*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float lit1=clamp(((t1.a*(1.0-saturate(0.0)))+(c0[0].a*(1.0-saturate(0.0)))),-1.0,1.0);\n"
"float3 lit2=clamp((lit0*t0.rgb),-1.0,1.0);\n"
"float lit3=clamp((lit1*t0.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(lit2,0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(lit2,0.0))*max(float3(0,0,0),0.0)),max(lit3,0.0));\n"},
{2,{0xe0,0xdf,0x9b,0xce,0x0c,0xd2,0x0a,0xdc,0x38,0x4e,0x78,0xd1,0xe1,0xb8,0x93,0x47,0x06,0x89,0x6e,0x71,0x8c,0x76,0xd0,0x5b,0xa1,0x89,0xbb,0x13,0xe9,0x33,0xda,0x2c},"material_51a048",
"float3 lit0=clamp((c0[0].rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float lit1=clamp((c0[0].a*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float3 lit2=clamp((lit0*t0.rgb),-1.0,1.0);\n"
"float lit3=clamp((lit1*t0.a),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(lit2,0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(lit2,0.0))*max(float3(0,0,0),0.0)),max(lit3,0.0));\n"},
{2,{0xf4,0x20,0x7d,0x4f,0xf7,0x63,0xf8,0xe0,0x73,0xe1,0xcc,0x5f,0xa8,0x35,0xa6,0x44,0x6a,0x92,0x2f,0x76,0xf9,0xbe,0x42,0x49,0x39,0x40,0xc9,0x1b,0x4a,0xf5,0xc2,0x00},"material_51a1b0",
"float3 lit0=clamp(dot((2.0*max(t0.rgb,0.0)-1.0),c0[0].rgb),-1.0,1.0);\n"
"float3 lit1=clamp(((lit0*c0[1].aaa)+((1.0-saturate(float3(0,0,0)))*(1.0-saturate(c0[1].aaa)))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(lit1,0.0)*max(fc0.rgb,0.0)+(1.0-max(lit1,0.0))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{2,{0xfa,0xb2,0x71,0x52,0x79,0x5f,0xfb,0x43,0x12,0xdb,0xd2,0xed,0xf1,0x8b,0x0a,0x2d,0xd5,0xd2,0xd4,0xde,0xe8,0xa8,0x30,0x0c,0xa9,0x81,0x1e,0x76,0x69,0xd8,0x2b,0x59},"material_51a154",
"float3 lit0=clamp(dot((2.0*max(t1.rgb,0.0)-1.0),c0[0].rgb),-1.0,1.0);\n"
"float3 lit1=clamp(((lit0*c0[1].aaa)+((1.0-saturate(float3(0,0,0)))*(1.0-saturate(c0[1].aaa)))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(t0.rgb,0.0)+max(lit1,0.0)*max(fc0.rgb,0.0)+(1.0-max(lit1,0.0))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{2,{0x5a,0xd4,0x2f,0x54,0x32,0xd5,0x90,0x2e,0x95,0xfc,0xa1,0xd3,0x97,0xb5,0xb1,0xc6,0x5d,0xec,0x0a,0xe3,0x98,0xa9,0xc8,0x27,0xbc,0xb1,0xbc,0x8e,0xd1,0x5e,0x9f,0x96},"environment_tone",
"float3 lit0=clamp(((t1.rgb*(1.0-saturate(float3(0,0,0))))+(t2.rgb*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float3 lit1=clamp(((lit0*(1.0-saturate(float3(0,0,0))))+(t3.rgb*(1.0-saturate(float3(0,0,0))))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(lit1,0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{2,{0x21,0xc3,0x15,0xa1,0xf7,0xa0,0x9d,0x1d,0x67,0xbc,0xd9,0xec,0x99,0xc6,0xe4,0x9a,0x36,0x13,0x78,0x59,0xb6,0xc9,0xef,0x12,0x2b,0x5b,0xbf,0x09,0x2e,0x8a,0xc1,0x67},"environment_exposure",
"float3 lit0=clamp(((float3(t0.a,t0.a,t0.a)*c0[0].rgb)+((1.0-saturate(float3(0,0,0)))*t0.rgb)),-1.0,1.0);\n"
"float3 lit2=clamp(dot(lit0,c0[1].rgb),-1.0,1.0);\n"
"float4 result=float4(saturate(max(lit2,0.0)+(1.0-saturate(float3(0,0,0)))*max((max(lit0,0.0)*(1.0-saturate(fc0.aaa))),0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{1,{0x60,0x12,0xc2,0x17,0x40,0x69,0x7b,0x47,0xe1,0x8c,0x41,0x03,0x32,0x51,0x64,0x57,0x18,0x7b,0x5f,0xad,0x28,0x4e,0xac,0xe2,0x16,0xda,0x4e,0xd7,0x43,0x3b,0x15,0xed},"environment_history",
"float3 lit0=clamp((t0.rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(fc0.rgb,0.0)*max(t1.rgb,0.0)+(1.0-max(fc0.rgb,0.0))*max(lit0,0.0)),max((float3(0,0,0)).b,0.0));\n"},
{2,{0xcf,0x95,0xcd,0xbc,0xcb,0xd5,0x0a,0x60,0x59,0xce,0xc1,0xee,0x9e,0x7b,0xc2,0x50,0xef,0xfe,0x71,0xd7,0xf7,0x17,0x9d,0x1d,0xa7,0xa0,0xf2,0x3e,0x0f,0x40,0x49,0xde},"material_51a180",
"float3 lit0=clamp(((t0.rgb*c0[0].rgb)+((1.0-saturate(float3(0,0,0)))*c0[0].aaa)),-1.0,1.0);\n"
"float lit2=clamp((t0.a*(1.0-saturate(0.0))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+(1.0-saturate(float3(0,0,0)))*max(((1.0-saturate(float3(0,0,0)))*max(lit0,0.0)),0.0)+(1.0-(1.0-saturate(float3(0,0,0))))*max(float3(0,0,0),0.0)),max(lit2,0.0));\n"},
{1,{0x8e,0xb6,0x6d,0x15,0xc6,0x94,0x09,0x27,0xa2,0x0e,0x38,0x18,0xca,0x90,0x4a,0xcc,0xa2,0xdb,0xa9,0xd5,0xc3,0xf1,0xc6,0x85,0x26,0x72,0x7e,0x10,0x70,0x23,0x54,0x7f},"environment_glow",
"float3 lit0=clamp((t1.rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(lit0,0.0)+max(float3(0,0,0),0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(float3(0,0,0),0.0))*max(float3(0,0,0),0.0)),max((float3(0,0,0)).b,0.0));\n"},
{1,{0x43,0xbc,0xe7,0x58,0xda,0x5a,0xd7,0x83,0x97,0x22,0x06,0x4e,0x97,0x43,0x1e,0xe6,0xb4,0xb4,0xf7,0x7a,0xca,0xe9,0x0f,0x48,0x55,0x43,0xfe,0x34,0xe3,0x83,0xe5,0xc7},"bloom_downsample",
"float3 lit0=clamp(((t0.rgb*(1.0-saturate(float3(0,0,0))))+(c0[0].rgb*(2.0*max(float3(0,0,0),0.0)-1.0))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(lit0,0.0)+max(float3(0,0,0),0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(float3(0,0,0),0.0))*max(float3(0,0,0),0.0)),(1.0-saturate((float3(0,0,0)).b)));\n"},
{1,{0xc3,0xfa,0xae,0x1b,0xad,0xca,0x95,0x91,0x8e,0x2b,0x8b,0x20,0x78,0xa4,0x3e,0x30,0x83,0x61,0xbc,0x0c,0x25,0x82,0xf0,0xb3,0x6c,0xa2,0x3d,0x70,0xbb,0x2b,0xd6,0x76},"environment_glow_history",
"float3 lit0=clamp(((t0.rgb*c0[0].rgb)*4.0),-1.0,1.0);\n"
"float lit1=clamp(((t0.a*c0[0].a)*4.0),-1.0,1.0);\n"
"float4 result=float4(saturate(max(float3(0,0,0),0.0)+max(lit0,0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(lit0,0.0))*max(float3(0,0,0),0.0)),max(lit1,0.0));\n"},
{2,{0x6e,0x76,0xd1,0xda,0x1d,0x52,0x89,0x49,0x8b,0xb1,0xde,0x0f,0x4b,0xd1,0xbf,0xa2,0x05,0x2b,0x4b,0xd1,0x64,0x92,0x65,0x9f,0xc8,0x99,0x02,0xab,0xd9,0x4a,0x99,0xdb},"gaussian_blur",
"float3 lit0=clamp(((t0.rgb*c0[0].rgb)+(t1.rgb*c0[0].rgb)),-1.0,1.0);\n"
"float3 lit2=clamp(((t2.rgb*c0[1].rgb)+(t3.rgb*c0[1].rgb)),-1.0,1.0);\n"
"float4 result=float4(saturate(max(lit2,0.0)+max(lit0,0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(lit0,0.0))*max(float3(0,0,0),0.0)),max(fc0.a,0.0));\n"},
{1,{0x4e,0xcf,0x5d,0x9a,0x85,0x92,0x39,0x0b,0xee,0x4a,0x88,0x15,0x08,0xf1,0xc8,0x2d,0xcd,0x0e,0xe2,0x8b,0x87,0xad,0xf9,0xd7,0x26,0xa5,0xa3,0x60,0x20,0x95,0x48,0xe2},"distance_fog",
"float3 lit0=clamp((c0[0].rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(lit0,0.0)+max(float3(0,0,0),0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(float3(0,0,0),0.0))*max(float3(0,0,0),0.0)),max(t1.a,0.0));\n"},
{1,{0x93,0xc4,0x0c,0xbd,0xb0,0x9e,0xaa,0x30,0x7b,0x41,0x3b,0x7a,0x75,0x42,0x12,0xe0,0x19,0x92,0x21,0xaa,0xcb,0x6d,0x45,0xdc,0x32,0xec,0x5f,0x0b,0xa6,0x99,0x78,0x5a},"bloom_composite",
"float3 lit0=clamp((t3.rgb*(1.0-saturate(float3(0,0,0)))),-1.0,1.0);\n"
"float4 result=float4(saturate(max(lit0,0.0)+max(float3(0,0,0),0.0)*(1.0-saturate(float3(0,0,0)))+(1.0-max(float3(0,0,0),0.0))*max(float3(0,0,0),0.0)),max(t1.a,0.0));\n"},
};
