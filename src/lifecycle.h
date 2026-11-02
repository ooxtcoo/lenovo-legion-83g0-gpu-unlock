#pragma once
#include <Uefi.h>
EFI_STATUS GuGraphInit(EFI_HANDLE,EFI_SYSTEM_TABLE*);
BOOLEAN GuGraphAttached(BOOLEAN);
BOOLEAN GuGraphDetached(VOID);
BOOLEAN GuGraphDetachedDisplay(VOID);
EFI_HANDLE GuGraphOwner(VOID);
EFI_STATUS GuGraphConsent(VOID);
EFI_STATUS GuGraphSs0Consent(UINT32 ss0);
EFI_STATUS GuGraphRefuse(EFI_STATUS);
BOOLEAN GuGraphFreshConsole(VOID);

EFI_STATUS GuGraphRefuseAt(EFI_STATUS,const char *);

VOID GuGraphSetRefusalReport(VOID (*Report)(VOID));
