#pragma once
#include "character_navigation.h"
extern "C" {
#include "../fire_damage_sphere.h"
}
/* Actual236D0/24630 box groups. add retains raw geometry; refresh
 * publishes groups at the existing navigation update phase. All admitted
 * navigators currently use radius40 and detour margin101. Source23860 leaves
 * merged byte9 uninitialized; this PC owner retains the equal input flag.
 * This owner is the shared raw-box registry, not one registry per flame. */
class NativeNavigationBoxes {
 struct Group;
 struct Entry {UCNavigationBox box{};Group *group=nullptr;};
 NativeCharacterNavigationManager&manager_;std::vector<std::unique_ptr<Entry>>boxes_;
 std::vector<std::unique_ptr<Group>>groups_;bool failed_=false;
 int publish(std::unique_ptr<Group>);void erase(Group*);
public:
 explicit NativeNavigationBoxes(NativeCharacterNavigationManager&);
 ~NativeNavigationBoxes();
 NativeNavigationBoxes(const NativeNavigationBoxes&)=delete;
 NativeNavigationBoxes&operator=(const NativeNavigationBoxes&)=delete;
 int add(int32_t id,const float center[3],const float size[3],uint8_t allow_inside);
 int remove(int32_t id);int refresh();
 size_t size()const{return boxes_.size();}
 const NativeCharacterDetour::Obstacle*group(int32_t id)const;
};
/* Source polygon values only; container allocation is native. 22E60 preserves
 * original insertion/removal order, including collinear and wrap cases. */
struct NativeNavigationPolygon {
 using Point=std::array<float,3>;
 std::vector<Point> points;Point center{};float radius=0,height=0;
 uint32_t members=1;uint8_t allow_inside=0;
};
int native_navigation_insert(NativeNavigationPolygon&,const NativeNavigationPolygon::Point&);
int native_navigation_merge(const NativeNavigationPolygon&,const NativeNavigationPolygon&,NativeNavigationPolygon&);
int native_navigation_overlap(const NativeNavigationPolygon&,const NativeNavigationPolygon&);
