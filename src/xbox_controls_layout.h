/* Road Rash/RecompFrontend-style controls organization, using native widgets.
 * Bindings retain the existing Xbox backend and two slots per guest input. */
static int xbox_bindings_split(void){return xbox_ui_width*65/100;}
static void xbox_bindings_layout(void){
 int visible=xbox_tab==2&&!xbox_close_pending;
 int split=xbox_bindings_split(),text=(split-64)*43/100,slot=(split-text-90)/2;
 xbox_control_rows=(xbox_ui_height-240)/80;if(xbox_control_rows<1)xbox_control_rows=1;if(xbox_control_rows>8)xbox_control_rows=8;
 if(xbox_control_page>xbox_control_action_count()-xbox_control_rows)xbox_control_page=xbox_control_action_count()-xbox_control_rows;
 if(xbox_control_help>=xbox_control_action_count())xbox_control_help=xbox_control_action_count()-1;
 for(int i=1902;i<=1903;++i)ShowWindow(GetDlgItem(xbox_overlay_window,i),SW_HIDE);
 xbox_place(1904,xbox_ui_width-210,xbox_ui_height-65,180,36);
 ShowWindow(GetDlgItem(xbox_overlay_window,1904),visible?SW_SHOW:SW_HIDE);
 for(int row=0;row<8;++row)for(int s=0;s<2;++s){int id=2000+row*4+s*2,y=145+row*80;
  xbox_place(id,text+28+s*slot,y,slot-29,48);xbox_place(id+1,text+28+s*slot+slot-27,y+10,24,28);
  ShowWindow(GetDlgItem(xbox_overlay_window,id),visible&&row<xbox_control_rows?SW_SHOW:SW_HIDE);
  ShowWindow(GetDlgItem(xbox_overlay_window,id+1),visible&&row<xbox_control_rows?SW_SHOW:SW_HIDE);
 }
 xbox_place(1920,split-12,134,10,xbox_control_rows*80);
 ShowWindow(GetDlgItem(xbox_overlay_window,1920),visible?SW_SHOW:SW_HIDE);
 InvalidateRect(GetDlgItem(xbox_overlay_window,1920),NULL,FALSE);
 for(int i=0;i<2;++i){xbox_place(1910+i,split+24,390+i*78,xbox_ui_width-split-48,28);ShowWindow(GetDlgItem(xbox_overlay_window,1910+i),visible&&!xbox_control_device?SW_SHOW:SW_HIDE);}
 xbox_place(1700,split+24,412,xbox_ui_width-split-48,30);ShowWindow(GetDlgItem(xbox_overlay_window,1700),visible&&xbox_control_device?SW_SHOW:SW_HIDE);
}
static void xbox_bindings_scroll(int value){
 if(xbox_bind_action>=0)return; /* Never recycle a row while capturing input. */
 int max=xbox_control_action_count()-xbox_control_rows;xbox_control_page=value<0?0:value>max?max:value;
 int focused=GetDlgCtrlID(GetFocus());xbox_control_help=xbox_control_page;if(focused>=2000&&focused<2000+xbox_control_rows*4)xbox_control_help=xbox_control_page+(focused-2000)/4;
 xbox_bindings_layout();xbox_overlay_labels();
}
static void xbox_bindings_navigate(int backwards){
 int id=GetDlgCtrlID(GetFocus()),row=(id-2000)/4;
 if(id>=2000&&id<2000+xbox_control_rows*4){
  if(backwards&&row==0&&xbox_control_page>0){xbox_bindings_scroll(xbox_control_page-1);return;}
  if(!backwards&&row==xbox_control_rows-1&&xbox_control_page<xbox_control_action_count()-xbox_control_rows){xbox_bindings_scroll(xbox_control_page+1);return;}
 }
 SetFocus(GetNextDlgTabItem(xbox_overlay_window,GetFocus(),backwards));
}
static LRESULT CALLBACK xbox_bindings_scroll_proc(HWND w,UINT m,WPARAM a,LPARAM b){
 RECT r;GetClientRect(w,&r);int maximum=xbox_control_action_count()-xbox_control_rows,thumb=r.bottom*xbox_control_rows/xbox_control_action_count(),travel=r.bottom-thumb;
 if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);SetDCBrushColor(dc,RGB(12,14,12));FillRect(dc,&r,GetStockObject(DC_BRUSH));r.top=MulDiv(xbox_control_page,travel,maximum);r.bottom=r.top+thumb;SetDCBrushColor(dc,RGB(50,74,38));FillRect(dc,&r,GetStockObject(DC_BRUSH));EndPaint(w,&ps);return 0;}
 if(m==WM_LBUTTONDOWN||(m==WM_MOUSEMOVE&&(a&MK_LBUTTON))){if(m==WM_LBUTTONDOWN)SetCapture(w);if(travel>0)xbox_bindings_scroll(MulDiv((short)HIWORD(b)-thumb/2,maximum,travel));return 0;}
 if(m==WM_LBUTTONUP){ReleaseCapture();return 0;}
 if(m==WM_MOUSEWHEEL)return SendMessage(GetParent(w),m,a,b);
 return DefWindowProcA(w,m,a,b);
}
static LRESULT CALLBACK xbox_binding_control_proc(HWND w,UINT m,WPARAM a,LPARAM b,UINT_PTR id,DWORD_PTR data){
 (void)data;
 if(m==WM_MOUSEMOVE||m==WM_SETFOCUS){int action=xbox_control_page+((int)id-2000)/4;if(action<xbox_control_action_count()&&xbox_control_help!=action){xbox_control_help=action;InvalidateRect(xbox_overlay_window,NULL,FALSE);}}
 if(m==WM_MOUSEWHEEL)return SendMessage(xbox_overlay_window,m,a,b);
 if(m==WM_KEYDOWN&&(a==VK_NEXT||a==VK_PRIOR)){xbox_bindings_scroll(xbox_control_page+(a==VK_NEXT?1:-1)*xbox_control_rows);return 0;}
 if(m==WM_KEYDOWN&&(a==VK_UP||a==VK_DOWN)){xbox_bindings_navigate(a==VK_UP);return 0;}
 if(m==WM_NCDESTROY)RemoveWindowSubclass(w,xbox_binding_control_proc,id);
 return DefSubclassProc(w,m,a,b);
}
static void xbox_bindings_paint(HDC dc){
 int split=xbox_bindings_split(),text=(split-64)*43/100;
 SelectObject(dc,xbox_button_font);SetTextColor(dc,RGB(129,222,65));
 xbox_text(dc,28,119,"GAME INPUT");xbox_text(dc,text+28,119,"BINDINGS");
 for(int row=0;row<xbox_control_rows;++row){int action=xbox_control_page+row,y=142+row*80;
  if(action==xbox_control_help){RECT active={xbox_px(18),xbox_px(y-4),xbox_px(split-20),xbox_px(y+74)};SetDCBrushColor(dc,RGB(19,27,17));FillRect(dc,&active,GetStockObject(DC_BRUSH));}
  SelectObject(dc,xbox_button_font);SetTextColor(dc,RGB(235,238,231));
  RECT title={xbox_px(28),xbox_px(y),xbox_px(text+18),xbox_px(y+24)};
  DrawTextA(dc,xbox_action_names[action],-1,&title,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
  RECT desc={xbox_px(28),xbox_px(y+25),xbox_px(text+18),xbox_px(y+73)};SetTextColor(dc,RGB(145,155,139));
  DrawTextA(dc,xbox_action_descriptions[action],-1,&desc,DT_WORDBREAK|DT_END_ELLIPSIS|DT_NOPREFIX);
 }
 HPEN pen=CreatePen(PS_SOLID,1,RGB(35,53,30));HGDIOBJ old=SelectObject(dc,pen);MoveToEx(dc,xbox_px(split),xbox_px(114),NULL);LineTo(dc,xbox_px(split),xbox_px(xbox_ui_height-78));SelectObject(dc,old);DeleteObject(pen);
 SelectObject(dc,xbox_button_title);SetTextColor(dc,RGB(223,231,220));
 RECT title={xbox_px(split+24),xbox_px(136),xbox_px(xbox_ui_width-24),xbox_px(194)};DrawTextA(dc,xbox_action_names[xbox_control_help],-1,&title,DT_WORDBREAK|DT_NOPREFIX);
 SelectObject(dc,xbox_button_font);SetTextColor(dc,RGB(180,190,175));
 RECT help={xbox_px(split+24),xbox_px(204),xbox_px(xbox_ui_width-24),xbox_px(345)};
 char first[64],second[64],description[640];
 snprintf(first,sizeof(first),"%s",xbox_control_device?xbox_KeyboardSourceName(xbox_binding(xbox_control_help,0)):xbox_ControllerSourceName(xbox_binding(xbox_control_help,0)));
 snprintf(second,sizeof(second),"%s",xbox_control_device?xbox_KeyboardSourceName(xbox_binding(xbox_control_help,1)):xbox_ControllerSourceName(xbox_binding(xbox_control_help,1)));
 snprintf(description,sizeof(description),"%s\n\nAssigned: %s / %s",xbox_action_descriptions[xbox_control_help],first,second);DrawTextA(dc,description,-1,&help,DT_WORDBREAK|DT_NOPREFIX);
 if(!xbox_control_device)for(int i=0;i<2;++i){char line[80];snprintf(line,sizeof(line),"%s stick deadzone: %u%%",i?"Right":"Left",xbox_ControllerDeadzone(i));xbox_text(dc,split+24,364+i*78,line);}
 else {char line[80];unsigned sensitivity=xbox_MouseGetSensitivity();snprintf(line,sizeof(line),"Mouse sensitivity: %u.%02ux",sensitivity/100,sensitivity%100);xbox_text(dc,split+24,380,line);
 if(xbox_ui_height>=580){RECT mouse={xbox_px(split+24),xbox_px(452),xbox_px(xbox_ui_width-24),xbox_px(xbox_ui_height-100)};char notice[320];xbox_binding_shared_notice(xbox_control_help,notice,sizeof(notice));if(notice[0])SetTextColor(dc,RGB(226,191,126));DrawTextA(dc,notice[0]?notice:"Wheel directions can select or cycle a weapon category. Teammate orders has its own hold binding at the end of this list.",-1,&mouse,DT_WORDBREAK|DT_NOPREFIX);}}
 xbox_text(dc,28,xbox_ui_height-60,xbox_bind_action>=0?(xbox_bind_armed?"Press a key, button or wheel. Esc cancels.":"Release inputs first. Esc cancels."):"Select a binding to change it. X clears. Saves immediately.");
}
