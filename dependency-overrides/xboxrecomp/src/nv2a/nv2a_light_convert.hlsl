Buffer<uint> source : register(t0);
cbuffer Layout : register(b0) { uint width,height,depth,signs; };
#ifdef LIGHT_CUBE
RWTexture2DArray<float4> destination : register(u0);
#else
RWTexture3D<float4> destination : register(u0);
#endif
uint morton(uint3 p){
    uint result=0,place=1;
    for(uint bit=1;bit<width||bit<height||bit<depth;bit<<=1){
        if(bit<width){if(p.x&bit)result|=place;place<<=1;}
        if(bit<height){if(p.y&bit)result|=place;place<<=1;}
        if(bit<depth){if(p.z&bit)result|=place;place<<=1;}
    }
    return result;
}
float component(uint v,uint mask){
    return (signs&mask)?float(int(v)-(v>=128?256:0))/128.0f:float(v)/255.0f;
}
[numthreads(8,8,1)]
void main(uint3 p:SV_DispatchThreadID){
#ifdef LIGHT_CUBE
    if(p.x>=width||p.y>=height||p.z>=6)return;
    uint v=source.Load(p.z*width*height+morton(uint3(p.xy,0)));
    destination[p]=float4(component((v>>16)&255,2),component((v>>8)&255,4),
                          component(v&255,8),component(v>>24,1));
#else
    if(p.x>=width||p.y>=height||p.z>=depth)return;
    uint address=morton(p);
    uint v=(source.Load(address>>2)>>((address&3)*8))&255;
    destination[p]=float4(1,1,1,float(v)/255.0f);
#endif
}
