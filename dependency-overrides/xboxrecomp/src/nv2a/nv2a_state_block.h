#ifndef NV_STATE_BLOCK_H
#define NV_STATE_BLOCK_H
#include <stdint.h>
typedef struct NvStateBlockResult {unsigned words,packets,methods,method,subchannel,incrementing;} NvStateBlockResult;
NvStateBlockResult pgraph_d3d11_state_block(const uint8_t *source,unsigned words);
#endif
