#pragma once
#include "assets.h"
#include "order_manager.h"
#include <map>
#include <string>
/* Presentation only. Source manager owns availability, alpha/color and actions.
 * PC center/direction layout replaces the controller pad; never selects/orders. */
class NativeOrderHud {
 struct Image{ComPtr<ID3D11ShaderResourceView> texture;float width=0,height=0;};
 std::map<std::string,Image> images_;ComPtr<ID3D11ShaderResourceView> key_,white_;
 NativeGpuMesh mesh_;unsigned key_code_=0;size_t capacity_=0;
 std::vector<NativeVertex> vertices_;std::vector<ID3D11ShaderResourceView*> views_;
public:
 void load(const UCArchive*,NativeRenderer&);
 // Qualified retained image for original70D70/71750 heartbeat feedback.
 ID3D11ShaderResourceView*image(const char*material)const;
 void update_key(NativeRenderer&,unsigned key);
 void draw(NativeRenderer&,const std::vector<NativeOrderSprite>&,bool held,float drag_x,float drag_y,int selected_slot);
};
