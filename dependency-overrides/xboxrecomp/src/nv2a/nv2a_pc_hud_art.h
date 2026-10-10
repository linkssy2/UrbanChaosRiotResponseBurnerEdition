/* PC HUD artwork, rasterized only when bindings/drag state change. */
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../../include/host_controller.h"
#include "nv2a_pc_hud_assets.h"
static const int pc_hud_actions[32]={-1,0,3,2,1,-1,7,4,6,5,9,8,-1,-1,-1,10,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static void pc_hud_line(HDC dc,int x,int y,int a,int b){MoveToEx(dc,x,y,NULL);LineTo(dc,a,b);}
static void pc_hud_icon(HDC dc,int left,int top,unsigned source,const char*label){
 SaveDC(dc);SetViewportOrgEx(dc,left,top,NULL);
 HPEN pen=CreatePen(PS_SOLID,3,RGB(218,218,197));HGDIOBJ op=SelectObject(dc,pen);
 HBRUSH fill=CreateSolidBrush(RGB(29,35,37));HGDIOBJ ob=SelectObject(dc,fill);
 if(source==1||source==2||source==4||source==5||source==6||source>=0x100){
  RoundRect(dc,32,9,96,119,30,30);pc_hud_line(dc,33,48,95,48);pc_hud_line(dc,64,10,64,46);
  HBRUSH hi=CreateSolidBrush(RGB(192,192,153));RECT r={source==2?68:38,17,source==2?88:60,42};
  if(source==1||source==2)FillRect(dc,&r,hi);
  if(source==4||source>=0x100){r=(RECT){58,23,70,43};FillRect(dc,&r,hi);}
  if(source>=0x100){int y=source==0x100?60:97;pc_hud_line(dc,64,60,64,98);pc_hud_line(dc,64,y,54,source==0x100?71:86);pc_hud_line(dc,64,y,74,source==0x100?71:86);}
  DeleteObject(hi);
 }else{
  RoundRect(dc,7,16,121,112,10,10);pc_hud_line(dc,13,104,114,104);pc_hud_line(dc,114,25,114,104);
 }
 if(!(source==1||source==2||source==4||source>=0x100)){
  char text[64];snprintf(text,sizeof(text),"%s",label);
  if(strstr(text,"Control")||strstr(text,"Ctrl"))strcpy(text,"CTRL");
  else if(strstr(text,"Shift"))strcpy(text,"SHIFT");
  else if(strstr(text,"Alt"))strcpy(text,"ALT");
  else if(!strcmp(text,"Backspace"))strcpy(text,"BACK");
  else if(!strcmp(text,"Unbound"))strcpy(text,"--");
  if(source==5||source==6)strcpy(text,source==5?"4":"5");
  int size=strlen(text)<=2?66:strlen(text)<=5?29:19;
  HFONT font=CreateFontA(-size,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Arial");HGDIOBJ of=SelectObject(dc,font);
  SetTextColor(dc,RGB(237,237,220));SetBkMode(dc,TRANSPARENT);RECT r={12,26,113,100};DrawTextA(dc,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
  SelectObject(dc,of);DeleteObject(font);
 }
 SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(fill);DeleteObject(pen);RestoreDC(dc,-1);
}
static uint32_t*pc_hud_pixels(unsigned id,const unsigned*bindings,char labels[13][64],long dx,long dy,unsigned direction,int held,unsigned*w,unsigned*h){
 *w=id==1?128:256;*h=id==1?4096:256;unsigned sw=id==1?32:64,sh=id==1?1024:64;
 const uint32_t*base=(const uint32_t*)(id==1?pc_hud_buttons_pixels:pc_hud_orders_pixels);
 BITMAPINFO info={0};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=*w;info.bmiHeader.biHeight=-(int)*h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 uint32_t*pixels=NULL;HDC dc=CreateCompatibleDC(NULL);HBITMAP bmp=CreateDIBSection(dc,&info,DIB_RGB_COLORS,(void**)&pixels,NULL,0);
 if(!dc||!bmp||!pixels){if(bmp)DeleteObject(bmp);if(dc)DeleteDC(dc);return NULL;}HGDIOBJ old=SelectObject(dc,bmp);
 for(unsigned y=0;y<*h;++y)for(unsigned x=0;x<*w;++x)pixels[y*(*w)+x]=id==1?base[(y*sh/(*h))*sw+x*sw/(*w)]:0;
 if(id==1){
  for(unsigned row=0;row<32;++row){int a=pc_hud_actions[row];if(a<0)continue;
   memset(pixels+row*128*128,0,128*128*4);pc_hud_icon(dc,0,row*128,bindings[a],labels[a]);GdiFlush();
   for(unsigned j=0;j<128*128;++j){uint32_t*q=pixels+row*128*128+j;if(*q&0xffffff)*q|=0xff000000;}
  }
 }else{
  /* Replace the controller art completely, keeping the source sprite's native
   * position, scale and alpha. The rebound mouse/key icon owns the center. */
  pc_hud_icon(dc,64,64,bindings[12],labels[12]);
  HPEN pen=CreatePen(PS_SOLID,5,RGB(231,230,179));HGDIOBJ op=SelectObject(dc,pen);HGDIOBJ ob=SelectObject(dc,GetStockObject(NULL_BRUSH));
  pc_hud_line(dc,111,39,128,22);pc_hud_line(dc,128,22,145,39);
  pc_hud_line(dc,111,217,128,234);pc_hud_line(dc,128,234,145,217);
  pc_hud_line(dc,39,111,22,128);pc_hud_line(dc,22,128,39,145);
  pc_hud_line(dc,217,111,234,128);pc_hud_line(dc,234,128,217,145);
  double x=dx,y=dy,extent=fmax(fabs(x),fabs(y));if(extent>60){x*=60/extent;y*=60/extent;}
  int tx=128+(int)x,ty=128+(int)y;
  if(held){pc_hud_line(dc,128,128,tx,ty);Ellipse(dc,tx-8,ty-8,tx+9,ty+9);}
  if(direction){int ax=direction==4?56:direction==8?200:128,ay=direction==1?56:direction==2?200:128;Ellipse(dc,ax-13,ay-13,ax+14,ay+14);}
  SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(pen);GdiFlush();
  for(unsigned y=0;y<*h;++y)for(unsigned x=0;x<*w;++x){uint32_t*q=&pixels[y*(*w)+x];if(*q&0xffffff)*q|=0xff000000;}
 }
 uint32_t*copy=malloc((size_t)*w**h*4);if(copy)memcpy(copy,pixels,(size_t)*w**h*4);
 SelectObject(dc,old);DeleteObject(bmp);DeleteDC(dc);return copy;
}
