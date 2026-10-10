#pragma once
#include <stdint.h>
/* Staged32D060 collector query. 0 leaves guest CPU/RAM untouched at this
 * boundary;1 publishes completed closest fields and consumes three call args.
 * Not production-connected. Only the pinned318EC0/3260E0 caller seam qualifies
 * dead traversal stack/volatile state; arbitrary callbacks remain translated. */
int uc_hybrid_scenery_query(uint32_t context,uint32_t stack,uint32_t frame);
