#ifndef UCRR_PC_MOUSE_HOST_H
#define UCRR_PC_MOUSE_HOST_H
extern void xbox_InputSetWindow(HWND);
extern BOOL xbox_InputIsBlocked(void);
extern void xbox_MouseSetActive(BOOL);
extern void xbox_MouseMotion(LONG,LONG);
extern void xbox_MouseOrdersButton(BOOL);
extern void xbox_KeyboardSourceEvent(unsigned,int);
extern void xbox_KeyboardWheel(int);
extern void xbox_KeyboardNoteInput(void);
static int pc_menu_active(void);
static void pc_menu_click(void);
static HWND pc_mouse_window;
static int pc_mouse_captured;
static WNDPROC pc_output_proc;
static void ucrr_host_release_mouse(void){
 pc_mouse_captured=0;xbox_MouseSetActive(FALSE);ClipCursor(NULL);
 if(GetCapture()==pc_mouse_window)ReleaseCapture();
 if(!hf_window_input(pc_mouse_window,WM_SETCURSOR,0,HTCLIENT))SetCursor(LoadCursor(NULL,IDC_ARROW));
}
static void pc_mouse_clip(void){RECT r;GetClientRect(pc_mouse_window,&r);MapWindowPoints(pc_mouse_window,NULL,(POINT*)&r,2);ClipCursor(&r);}
static void pc_mouse_capture(void){
 if(pc_mouse_captured||pc_menu_active()||GetForegroundWindow()!=pc_mouse_window||xbox_InputIsBlocked())return;
 pc_mouse_captured=1;xbox_MouseSetActive(TRUE);SetFocus(pc_mouse_window);SetCapture(pc_mouse_window);pc_mouse_clip();SetCursor(NULL);
}
static LRESULT CALLBACK pc_mouse_output_proc(HWND w,UINT m,WPARAM a,LPARAM b){
 if(hf_window_input(w,m,a,b))return m==WM_SETCURSOR?TRUE:0;
 if(m==WM_LBUTTONDOWN){xbox_KeyboardNoteInput();if(pc_menu_active())pc_menu_click();else pc_mouse_capture();return 0;}
 return CallWindowProcA(pc_output_proc,w,m,a,b);
}
static void pc_mouse_initialize(HWND parent,HWND output){
 pc_mouse_window=parent;xbox_InputSetWindow(parent);
 RAWINPUTDEVICE device={1,2,0,parent};
 if(!RegisterRawInputDevices(&device,1,sizeof(device)))fprintf(stderr,"[PC-INPUT] Raw mouse registration failed: %lu\n",GetLastError());
 pc_output_proc=(WNDPROC)SetWindowLongPtrA(output,GWLP_WNDPROC,(LONG_PTR)pc_mouse_output_proc);
}
static void pc_mouse_raw(LPARAM data){
 RAWINPUT input;UINT size=sizeof(input);
 if(GetRawInputData((HRAWINPUT)data,RID_INPUT,&input,&size,sizeof(RAWINPUTHEADER))==(UINT)-1)return;
 if(pc_mouse_captured&&GetForegroundWindow()==pc_mouse_window&&input.header.dwType==RIM_TYPEMOUSE&&!(input.data.mouse.usFlags&MOUSE_MOVE_ABSOLUTE))
 {
  static const unsigned down_flags[]={RI_MOUSE_LEFT_BUTTON_DOWN,RI_MOUSE_RIGHT_BUTTON_DOWN,RI_MOUSE_MIDDLE_BUTTON_DOWN,RI_MOUSE_BUTTON_4_DOWN,RI_MOUSE_BUTTON_5_DOWN};
  static const unsigned up_flags[]={RI_MOUSE_LEFT_BUTTON_UP,RI_MOUSE_RIGHT_BUTTON_UP,RI_MOUSE_MIDDLE_BUTTON_UP,RI_MOUSE_BUTTON_4_UP,RI_MOUSE_BUTTON_5_UP};
  static const unsigned sources[]={VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};
  for(unsigned i=0;i<5;++i)if(input.data.mouse.usButtonFlags&down_flags[i])xbox_KeyboardSourceEvent(sources[i],1);
  if(input.data.mouse.lLastX||input.data.mouse.lLastY)xbox_KeyboardNoteInput();
  if(input.data.mouse.usButtonFlags&RI_MOUSE_WHEEL)xbox_KeyboardWheel((SHORT)input.data.mouse.usButtonData);
  xbox_MouseMotion(input.data.mouse.lLastX,input.data.mouse.lLastY);
  for(unsigned i=0;i<5;++i)if(input.data.mouse.usButtonFlags&up_flags[i])xbox_KeyboardSourceEvent(sources[i],0);
 }
}
#endif
