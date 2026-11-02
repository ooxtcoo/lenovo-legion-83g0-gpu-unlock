#pragma once
#include "port.h"
#define GU_EMBEDDED_BYTES 84436020U
typedef struct {const UINT8 *begin,*end;} GU_EMBEDDED_ASSET;
extern const UINT8 GuEmbeddedStart[],GuEmbeddedEnd[];
extern const GU_EMBEDDED_ASSET GuEmbeddedAssets[5];
