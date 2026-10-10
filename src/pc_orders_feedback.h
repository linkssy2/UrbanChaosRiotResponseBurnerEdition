/* Host UI thread only. An owned, click-through window leaves the game's input
 * capture and original NPC choices in charge. No GPU readback or game pause. */
#ifndef UCRR_PC_ORDERS_FEEDBACK_H
#define UCRR_PC_ORDERS_FEEDBACK_H
static HWND pc_orders_window;
static long pc_orders_x,pc_orders_y;
static unsigned pc_orders_direction;
static HFONT pc_orders_font;
#define UCRR_ORDERS_TIMER 73
static LRESULT CALLBACK pc_orders_proc(HWND w,UINT m,WPARAM a,LPARAM b){
 if(m==WM_NCHITTEST)return HTTRANSPARENT;
 if(m==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT){
  PAINTSTRUCT ps;HDC out=BeginPaint(w,&ps);RECT r;GetClientRect(w,&r);
  HDC dc=CreateCompatibleDC(out);HBITMAP bitmap=CreateCompatibleBitmap(out,r.right,r.bottom);
  HGDIOBJ previous=SelectObject(dc,bitmap);
  HBRUSH clear=CreateSolidBrush(RGB(1,0,1));FillRect(dc,&r,clear);DeleteObject(clear);
  SetMapMode(dc,MM_ANISOTROPIC);SetWindowExtEx(dc,300,250,NULL);SetViewportExtEx(dc,r.right,r.bottom,NULL);
  HPEN pen=CreatePen(PS_SOLID,2,RGB(161,187,99));HGDIOBJ oldPen=SelectObject(dc,pen);
  HBRUSH panel=CreateSolidBrush(RGB(21,26,19));HGDIOBJ oldBrush=SelectObject(dc,panel);
  RoundRect(dc,126,86,174,134,8,8); /* the actual 24-count dead zone */
  const POINT arrows[4][3]={{{144,50},{150,42},{156,50}},{{144,170},{150,178},{156,170}},{{90,104},{82,110},{90,116}},{{210,104},{218,110},{210,116}}};
  for(unsigned i=0;i<4;++i){HPEN arrow=CreatePen(PS_SOLID,pc_orders_direction==(1u<<i)?4:2,pc_orders_direction==(1u<<i)?RGB(196,231,115):RGB(111,121,104));HGDIOBJ saved=SelectObject(dc,arrow);Polyline(dc,arrows[i],3);SelectObject(dc,saved);DeleteObject(arrow);}
  double dx=pc_orders_x,dy=pc_orders_y,extent=fmax(fabs(dx),fabs(dy));
  if(extent>60){dx*=60/extent;dy*=60/extent;}
  int x=150+(int)dx,y=110+(int)dy;
  MoveToEx(dc,150,110,NULL);LineTo(dc,x,y);Ellipse(dc,x-5,y-5,x+6,y+6);
  SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);DeleteObject(panel);
  HGDIOBJ font=SelectObject(dc,pc_orders_font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(224,228,216));
  char binding[64],hint[110];xbox_KeyboardActionLabel(UCRR_KEYBOARD_ORDERS,binding,sizeof(binding));
  snprintf(hint,sizeof(hint),"HOLD %s + DRAG",binding);
  RECT text={0,192,300,217};DrawTextA(dc,hint,-1,&text,DT_CENTER|DT_SINGLELINE|DT_NOPREFIX);
  text.top=216;text.bottom=245;DrawTextA(dc,pc_orders_direction?"RELEASE TO SELECT":"DRAG TOWARD A CHOICE",-1,&text,DT_CENTER|DT_SINGLELINE|DT_NOPREFIX);
  SelectObject(dc,font);SetMapMode(dc,MM_TEXT);BitBlt(out,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);
  SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);EndPaint(w,&ps);return 0;
 }
 if(m==WM_NCDESTROY){if(pc_orders_font){DeleteObject(pc_orders_font);pc_orders_font=NULL;}pc_orders_window=NULL;}
 return DefWindowProcA(w,m,a,b);
}
static void pc_orders_feedback_tick(void){
 long x,y;unsigned direction;
 if(!pc_orders_window)return;
 if(!xbox_MouseOrdersFeedback(&x,&y,&direction)||pc_menu_active()||xbox_overlay_open||IsIconic(pc_mouse_window)){
  KillTimer(pc_mouse_window,UCRR_ORDERS_TIMER);
  if(IsWindowVisible(pc_orders_window))ShowWindow(pc_orders_window,SW_HIDE);return;
 }
 RECT r;GetClientRect(xbox_output_window,&r);POINT origin={0,0};ClientToScreen(xbox_output_window,&origin);
 int height=MulDiv(r.bottom,250,720);if(height<180)height=180;if(height>350)height=350;
 int width=MulDiv(height,300,250);
 pc_orders_x=x;pc_orders_y=y;pc_orders_direction=direction;
 SetWindowPos(pc_orders_window,HWND_TOP,origin.x+(r.right-width)/2,origin.y+(r.bottom-height)/2,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);
 InvalidateRect(pc_orders_window,NULL,FALSE);
}
static void pc_orders_feedback_initialize(HWND parent,HINSTANCE instance){
 WNDCLASSA c={0};c.lpfnWndProc=pc_orders_proc;c.hInstance=instance;c.lpszClassName="UCRRCommandDragFeedback";RegisterClassA(&c);
 pc_orders_font=CreateFontA(-16,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,"Arial");
 pc_orders_window=CreateWindowExA(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,c.lpszClassName,"",WS_POPUP,0,0,300,250,parent,NULL,instance,NULL);
 if(pc_orders_window){SetLayeredWindowAttributes(pc_orders_window,RGB(1,0,1),240,LWA_COLORKEY|LWA_ALPHA);}
 else if(pc_orders_font){DeleteObject(pc_orders_font);pc_orders_font=NULL;}
}
#endif
