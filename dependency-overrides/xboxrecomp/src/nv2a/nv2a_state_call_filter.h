#ifndef NV2A_STATE_CALL_FILTER_H
#define NV2A_STATE_CALL_FILTER_H
/* Rejection only. This classifies a freshly read header, not guest payload or
 * mutable state. A positive result still needs every normal state-block guard.
 * Draw/index/inline packets cannot be pure state plans in any existing mode. */
static int nv_state_call_possible(uint32_t header)
{
    unsigned kind=header&0xE0030003u,count=(header>>18)&0x7ffu;
    if((kind!=0&&kind!=0x40000000u)||!count||count>32)return 0;
    switch(header&0x1ffcu){
    case NV097_SET_BEGIN_END:case NV097_ARRAY_ELEMENT16:
    case NV097_ARRAY_ELEMENT32:case NV097_DRAW_ARRAYS:
    case NV097_INLINE_ARRAY:return 0;
    default:return 1;
    }
}
#endif
