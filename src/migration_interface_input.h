#ifndef MIGRATION_INTERFACE_INPUT_H
#define MIGRATION_INTERFACE_INPUT_H
/* 0:55BF0 binding expansion, 1:55CD0 physical record, 2:567F0 title query,
 * 3:274740 original menu press conversion (after existing movie/mouse hooks).
 * 4:55880 presets, 5:55BA0 get, 6:55BB0 set, 7:55BD0/55BE0 reset.
 * Exact flag1 admits; a decline leaves CPU, RAM and host FP state untouched. */
int migration_interface_input(unsigned operation);
#endif
