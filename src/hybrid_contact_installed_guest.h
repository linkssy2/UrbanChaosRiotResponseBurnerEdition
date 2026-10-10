#ifndef UC_HYBRID_CONTACT_INSTALLED_GUEST_H
#define UC_HYBRID_CONTACT_INSTALLED_GUEST_H
#ifdef __cplusplus
extern "C" {
#endif
/* Returns 0 without guest effects; 1 means the complete owner returned.
 * Registered callbacks retain the original valid lifetime obligation. */
int uc_hybrid_contact_installed_destroy(void);
#ifdef __cplusplus
}
#endif
#endif
