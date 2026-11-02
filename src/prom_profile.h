#ifndef GU_PROM_PROFILE_H
#define GU_PROM_PROFILE_H
/* Lenovo83G0 live collector photo: normalized [0,0xa2c00) matches the
 * independently parsed local VBIOS chain. Never upload the private bytes.
 * Matching bytes are not firmware signature authentication or execution proof. */
#define GU_PROM_BAR_OFFSET 0x300000U
#define GU_PROM_VIEW_BYTES 0xa2c00U
#define GU_PROM_VIEW_SHA256 "ea8dcfba333047f4f8b5d77891a77c3c5b0642e753fd718834c5501e5fccae14"
#endif
