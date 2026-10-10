/* Optional presentation counter: fixed storage, no file logging/readback/window. */
static LONG fps_enabled=-1;
static int fps_path(wchar_t*p){DWORD n=GetModuleFileNameW(NULL,p,MAX_PATH);if(!n||n>=MAX_PATH)return 0;wchar_t*q=wcsrchr(p,L'\\');if(!q||q-p+16>=MAX_PATH)return 0;wcscpy(q+1,L"xboxrecomp.ini");return 1;}
int d3d8_GetFpsCounter(void){
 if(InterlockedCompareExchange(&fps_enabled,0,0)<0){wchar_t p[MAX_PATH];LONG v=fps_path(p)&&GetPrivateProfileIntW(L"Graphics",L"ShowFPS",0,p)==1;InterlockedCompareExchange(&fps_enabled,v,-1);}
 return InterlockedCompareExchange(&fps_enabled,0,0)>0;
}
int d3d8_SaveFpsCounter(int enabled){wchar_t p[MAX_PATH];if(!fps_path(p)||!WritePrivateProfileStringW(L"Graphics",L"ShowFPS",enabled?L"1":L"0",p))return 0;InterlockedExchange(&fps_enabled,!!enabled);return 1;}
static LONGLONG fps_ticks[128],fps_frequency,fps_last_refresh;
static unsigned fps_head,fps_count;
static char fps_text[24]="--.- FPS";
static void fps_present(HRESULT result){
 if(!d3d8_GetFpsCounter()||result!=S_OK){fps_count=0;fps_last_refresh=0;strcpy(fps_text,"--.- FPS");return;}
 LARGE_INTEGER t;if(!fps_frequency){QueryPerformanceFrequency(&t);fps_frequency=t.QuadPart;}if(!fps_frequency||!QueryPerformanceCounter(&t))return;
 if(fps_count&&t.QuadPart-fps_ticks[(fps_head+127)%128]>fps_frequency*2){fps_count=0;fps_last_refresh=0;}
 fps_ticks[fps_head]=t.QuadPart;fps_head=(fps_head+1)%128;if(fps_count<128)++fps_count;
 while(fps_count>2&&t.QuadPart-fps_ticks[(fps_head+128-fps_count)%128]>fps_frequency)--fps_count;
 if(fps_count>=2&&t.QuadPart-fps_last_refresh>=fps_frequency/4){
  LONGLONG span=t.QuadPart-fps_ticks[(fps_head+128-fps_count)%128];
  if(span>0)snprintf(fps_text,sizeof(fps_text),"%.1f FPS",(double)(fps_count-1)*fps_frequency/span);
  fps_last_refresh=t.QuadPart;
 }
}
static unsigned fps_glyph(char c,unsigned row){
 static const unsigned char digits[10][7]={{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
 if(c>='0'&&c<='9')return digits[c-'0'][row];
 static const unsigned char f[7]={31,16,16,30,16,16,16},p[7]={30,17,17,30,16,16,16},s[7]={15,16,16,14,1,1,30};
 if(c=='F')return f[row];if(c=='P')return p[row];if(c=='S')return s[row];if(c=='.')return row==6?4:0;if(c=='-')return row==3?14:0;return 0;
}
static void fps_draw(D3D8DeviceState*state){
 if(!d3d8_GetFpsCounter()||!state->clear_context||!state->default_rtv)return;
 D3D11_RECT rects[512];unsigned n=0,len=(unsigned)strlen(fps_text);
 int scale=state->present_height>=1000?3:2;
 D3D11_RECT bg={10,10,18+(int)len*6*scale,18+7*scale};float dark[4]={0.025f,0.035f,0.025f,1},ink[4]={0.86f,0.93f,0.72f,1};
 ID3D11DeviceContext1_ClearView(state->clear_context,(ID3D11View*)state->default_rtv,dark,&bg,1);
 for(unsigned i=0;i<len;++i)for(unsigned y=0;y<7;++y){unsigned bits=fps_glyph(fps_text[i],y);for(unsigned x=0;x<5;++x)if(bits&(1u<<(4-x))){int px=14+(i*6+x)*scale,py=14+y*scale;if(n<512)rects[n++]=(D3D11_RECT){px,py,px+scale,py+scale};}}
 if(n)ID3D11DeviceContext1_ClearView(state->clear_context,(ID3D11View*)state->default_rtv,ink,rects,n);
}
