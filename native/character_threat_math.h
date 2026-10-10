#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void uc_character_threat_direction(const float forward[3],int right,float out[3]);
int uc_character_threat_line_near(const float position[3],const float target[3],const float forward[3],float radius);
#ifdef __cplusplus
}
#endif
