#ifndef UCRR_PRESENT_RESIZE_H
#define UCRR_PRESENT_RESIZE_H
/* Render-owner only, after guest draws drain and before scanout/menu drawing. */
static HRESULT ucrr_present_resize(IDXGISwapChain *chain,ID3D11Device *device,ID3D11DeviceContext *context,
 ID3D11RenderTargetView **rtv,ID3D11Texture2D **depth,ID3D11DepthStencilView **dsv,
 UINT *width,UINT *height,UINT requested_width,UINT requested_height){
 if(!requested_width||!requested_height||requested_width>16384||requested_height>16384)return S_FALSE;
 if(*width==requested_width&&*height==requested_height)return S_FALSE;
 D3D11_TEXTURE2D_DESC desc={0};desc.Width=requested_width;desc.Height=requested_height;
 desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;
 desc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
 ID3D11Texture2D *next_depth=NULL,*buffer=NULL;ID3D11DepthStencilView *next_dsv=NULL;
 HRESULT hr=ID3D11Device_CreateTexture2D(device,&desc,NULL,&next_depth);
 if(FAILED(hr))return hr;
 hr=ID3D11Device_CreateDepthStencilView(device,(ID3D11Resource*)next_depth,NULL,&next_dsv);
 if(FAILED(hr)){ID3D11Texture2D_Release(next_depth);return hr;}
 ID3D11DeviceContext_OMSetRenderTargets(context,0,NULL,NULL);
 if(*rtv){ID3D11RenderTargetView_Release(*rtv);*rtv=NULL;}
 hr=IDXGISwapChain_ResizeBuffers(chain,0,requested_width,requested_height,DXGI_FORMAT_UNKNOWN,0);
 if(SUCCEEDED(hr)){
  if(*dsv)ID3D11DepthStencilView_Release(*dsv);if(*depth)ID3D11Texture2D_Release(*depth);
  *depth=next_depth;*dsv=next_dsv;*width=requested_width;*height=requested_height;
 }else{ID3D11DepthStencilView_Release(next_dsv);ID3D11Texture2D_Release(next_depth);}
 /* Even a refused resize releases our old view: reacquire the retained buffer. */
 HRESULT view_hr=IDXGISwapChain_GetBuffer(chain,0,&IID_ID3D11Texture2D,(void**)&buffer);
 if(SUCCEEDED(view_hr)){view_hr=ID3D11Device_CreateRenderTargetView(device,(ID3D11Resource*)buffer,NULL,rtv);ID3D11Texture2D_Release(buffer);}
 return FAILED(view_hr)?view_hr:hr;
}
#endif
