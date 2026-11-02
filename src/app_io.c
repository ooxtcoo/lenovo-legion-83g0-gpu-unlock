/* Modified 2026-10-21: experimental AD103 32GB GA102-pair port; see PORT-NOTES.txt. */
#include "app_io.h"
#include "compat/compat.h"
#include "format.h"
#include "sha256.h"
#include <Protocol/LoadedImage.h>
#include "embedded.h"
static EFI_GUID LoadedGuid=EFI_LOADED_IMAGE_PROTOCOL_GUID,PathGuid=EFI_DEVICE_PATH_PROTOCOL_GUID;
static EFI_SYSTEM_TABLE *Table;
/* Embedded assets own no filesystem handles. Windows discovery is separate. */
static BOOLEAN LogFault,IoReady;
static EFI_STATUS LogStatus;
typedef struct {BOOLEAN seen;EFI_STATUS raw,result;} IO_STEP;
static IO_STEP IoSteps[6];
static const char *IoNames[]={"LoadedImage","DevicePath","SimpleFS","OpenVolume","Root.Open","GetInfo"};
static EFI_HANDLE IoDevice;
static UINTN IoPathBytes,IoPathNodes;
static BOOLEAN IoEligible,IoUsb,IoEnd;
typedef struct {UINT8 type,subtype;UINT16 len;} PATH_NODE;
static PATH_NODE IoNodes[64];static UINTN IoNodeCount;
static int IoFirst;
static EFI_STATUS IoRecord(UINTN Step,EFI_STATUS Raw,EFI_STATUS Result) {
  IoSteps[Step]=(IO_STEP){TRUE,Raw,Result};
  if(Result!=EFI_SUCCESS && IoFirst<0)IoFirst=(int)Step;
  return Result;
}
VOID GuIoDescribe(VOID) {
  GuLog("IO first=%s device=%#llx eligible=%u path=%zuB/%zu nodes\n",
    IoFirst<0?"none":IoNames[IoFirst],(unsigned long long)(UINTN)IoDevice,IoEligible,(size_t)IoPathBytes,(size_t)IoPathNodes);
  for(UINTN i=0;i<6;i++) {
    if(IoSteps[i].seen)GuLog("IO %s raw=%#llx result=%#llx\n",IoNames[i],(unsigned long long)IoSteps[i].raw,(unsigned long long)IoSteps[i].result);
    else GuLog("IO %s SKIPPED\n",IoNames[i]);
  }
  if(LogFault)GuLog("SECONDARY logging raw=%#llx (output may be incomplete)\n",(unsigned long long)LogStatus);
}
VOID GuIoFinalDescribe(VOID) {
  if(IoFirst!=1)return;
  GuLog("\nPATH REFUSAL v5-auto-v2\n");
  GuLog("raw=%016llx result=%016llx eligible=%u\n",(unsigned long long)IoSteps[1].raw,(unsigned long long)IoSteps[1].result,IoEligible);
  GuLog("path=%zuB/%zu valid nodes; captured headers=%zu (max64)\n",(size_t)IoPathBytes,(size_t)IoPathNodes,(size_t)IoNodeCount);
  GuLog("need USB/SD exact-size=%u AND EndEntire/len4=%u; bounds64/4096\n",IoUsb,IoEnd);
  GuLog("Nodes: index:type/subtype/length (hex); original traversal only\n");
  for(UINTN i=0;i<IoNodeCount;i++) {
    GuLog("%02x:%02x/%02x/%04x%s",(unsigned)i,IoNodes[i].type,IoNodes[i].subtype,IoNodes[i].len,
      i%4==3 || i+1==IoNodeCount?"\n":" ");
  }
  GuLog("NO DETACH/ATTEMPT. Release keys; Space/Q/Esc returns. No auto-return.\n");
}
static char DetachedTail[8192];
static UINTN TailBytes;
BOOLEAN GuScreenDetached;
typedef struct {const char *name;CHAR16 *file;UINTN size;const char *hash;} ASSET_SPEC;
static const ASSET_SPEC Specs[]={
 {"gpu-unlock/610.43.03/booter_load_ad102.bin",L"ASSETS\\booter_load_ad102.bin",57672,"6ee05b20e1dc244c08c052f36ef28469b8d09f4955d1fe33e48a179e11a7fd49"},
 {"gpu-unlock/610.43.03/v67-ad102-610.43.03-four-plm.bin",L"ASSETS\\v67-ad102-610.43.03-four-plm.bin",64000,"44c36b2d6b3424e53e18f3e55db85c833655de071ce66ce42749588a4a7dc8cb"},
 {"gpu-unlock/610.43.03/gsp_ga10x.bin",L"ASSETS\\gsp_ga10x.bin",84277400,"572373620a37418f24dc16b5031c39338778c3257e48e8408de9a57291b24f3a"},
 {"gpu-unlock/610.43.03/gsp_boot_ad102.bin",L"ASSETS\\gsp_boot_ad102.bin",36864,"45d6335a8185d6ff13e8d1484cea73f68d3d13aa1c19dc080ed1c3482dd5bac0"},
 {"gpu-unlock/610.43.03/gsp_boot_desc_ad102.bin",L"ASSETS\\gsp_boot_desc_ad102.bin",84,"83f8f3f1f5f9496e0ea8fc7103c2beb64d0b6de2f5ffe725f13514de5a95fa42"}
};
static VOID LogError(VOID){if(!LogFault && LogStatus==EFI_SUCCESS)LogStatus=EFI_DEVICE_ERROR;LogFault=TRUE;if(Gu.armed)Gu.fault=TRUE;}
VOID GuConsole(const char *Text) {
  if(GuScreenDetached)return;
  if(!Table || !Table->ConOut || !Table->ConOut->OutputString){LogError();return;}
  while(*Text) {
    CHAR16 line[128];UINTN n=0;
    while(*Text && n<126){if(*Text=='\n')line[n++]='\r';line[n++]=(UINT8)*Text++;}
    line[n]=0;EFI_STATUS status=Table->ConOut->OutputString(Table->ConOut,line);if(status!=EFI_SUCCESS){if(!LogFault)LogStatus=status;LogError();return;}
  }
}
VOID GuLogText(const char *Text) {
  if(GuScreenDetached) {
    UINTN n=strlen(Text),cap=sizeof(DetachedTail)-1;
    if(n>cap){Text+=n-cap;n=cap;}
    if(TailBytes+n>cap){UINTN drop=TailBytes+n-cap;for(UINTN i=drop;i<TailBytes;i++)DetachedTail[i-drop]=DetachedTail[i];TailBytes-=drop;}
    memcpy(DetachedTail+TailBytes,Text,n);TailBytes+=n;DetachedTail[TailBytes]=0;
  } else GuConsole(Text);
}
VOID GuReplayDetached(VOID) {
  if(!GuScreenDetached&&TailBytes){GuConsole("LAST bounded transaction log tail (not persistent):\n");GuConsole(DetachedTail);TailBytes=0;}
}
VOID GuLog(const char *Format,...) {
  char text[1024];va_list args;va_start(args,Format);
  size_t n=GuFormatV(text,sizeof(text),Format,args);va_end(args);
  if(n>=sizeof(text)){LogError();GuConsole("Log format or length error.\n");return;}
  GuLogText(text);
}
EFI_STATUS GuConsoleStatus(VOID){return LogStatus;}
BOOLEAN GuLogReady(VOID){return IoReady && !LogFault;}
static EFI_STATUS UsbPath(EFI_HANDLE Handle) {
  EFI_DEVICE_PATH_PROTOCOL *path=NULL;
  EFI_STATUS status=Table->BootServices->HandleProtocol(Handle,&PathGuid,(VOID **)&path);
  if(status!=EFI_SUCCESS || !path)return IoRecord(1,status,status!=EFI_SUCCESS?status:EFI_UNSUPPORTED);
  UINTN total=0;BOOLEAN usb=FALSE;
  for(UINTN nodes=0;nodes<64 && total+4<=4096;nodes++) {
    UINTN n=path->Length[0]+((UINTN)path->Length[1]<<8);
    IoNodes[IoNodeCount++]=(PATH_NODE){path->Type,path->SubType,(UINT16)n};
    if(n<4 || n>4096-total)return IoRecord(1,status,EFI_UNSUPPORTED);
    IoPathBytes=total+n;IoPathNodes=nodes+1;
    if(path->Type==MESSAGING_DEVICE_PATH &&
      ((path->SubType==MSG_USB_DP && n==sizeof(USB_DEVICE_PATH)) ||
       (path->SubType==MSG_SD_DP && n==sizeof(SD_DEVICE_PATH)))){usb=TRUE;IoUsb=TRUE;}
    if(path->Type==END_DEVICE_PATH_TYPE){IoEnd=n==4 && path->SubType==END_ENTIRE_DEVICE_PATH_SUBTYPE;IoEligible=usb && n==4 && path->SubType==END_ENTIRE_DEVICE_PATH_SUBTYPE;
      return IoRecord(1,status,IoEligible?EFI_SUCCESS:EFI_UNSUPPORTED);}
    total+=n;path=(VOID *)((UINT8 *)path+n);
  }
  return IoRecord(1,status,EFI_UNSUPPORTED);
}
EFI_STATUS GuIoOpen(EFI_HANDLE Image,EFI_SYSTEM_TABLE *St) {
  Table=St;LogFault=FALSE;LogStatus=EFI_SUCCESS;IoReady=FALSE;TailBytes=0;
  memset(IoSteps,0,sizeof(IoSteps));IoFirst=-1;IoDevice=NULL;IoEligible=FALSE;IoPathBytes=IoPathNodes=0;
  IoNodeCount=0;IoUsb=IoEnd=FALSE;memset(IoNodes,0,sizeof(IoNodes));
  memset(GuAssets,0,sizeof(GuAssets));
  GuConsole("Lenovo AD103M v5-auto-v2 AUTO one-shot unlock. SCREEN logging only.\n");
  GuConsole("No report writes; photograph errors. Detached log tail is volatile.\n");
  EFI_LOADED_IMAGE_PROTOCOL *loaded=NULL;
  EFI_STATUS status=Table->BootServices->HandleProtocol(Image,&LoadedGuid,(VOID **)&loaded);
  IoRecord(0,status,status!=EFI_SUCCESS?status:!loaded||!loaded->DeviceHandle?EFI_UNSUPPORTED:EFI_SUCCESS);
  if(IoFirst>=0)return IoSteps[IoFirst].result;
  IoDevice=loaded->DeviceHandle;
  UsbPath(IoDevice);
  if(IoFirst>=0)return IoSteps[IoFirst].result;
  GuConsole("Five embedded assets; no asset filesystem access. Windows preload remains separate.\n");
  IoReady=TRUE;return GuLogReady()?EFI_SUCCESS:LogStatus;
}
/* No private PROM dump API in the runtime. */
EFI_STATUS GuLoadAssets(VOID) {
  for(UINTN i=0;i<5;i++) {
    const ASSET_SPEC *spec=&Specs[i];
    const GU_EMBEDDED_ASSET *asset=&GuEmbeddedAssets[i];
    UINTN base=(UINTN)GuEmbeddedStart,limit=(UINTN)GuEmbeddedEnd;
    UINTN begin=(UINTN)asset->begin,end=(UINTN)asset->end;
    /* Integer bounds before subtraction/dereference; no external fallback. */
    if(limit<base || limit-base!=GU_EMBEDDED_BYTES || begin<base ||
      end<begin || end>limit || end-begin!=spec->size)return EFI_COMPROMISED_DATA;
    UINT8 *data=GuAlloc(spec->size);if(!data)return EFI_OUT_OF_RESOURCES;
    memcpy(data,asset->begin,spec->size);
    if(!GuHashMatches(data,spec->size,spec->hash)){GuFree(data);return EFI_CRC_ERROR;}
    GuAssets[i].name=spec->name;GuAssets[i].firmware.data=data;GuAssets[i].firmware.size=spec->size;
    GuLog("asset verified: %s bytes=%zu sha256=%s\n",spec->name,(size_t)spec->size,spec->hash);
    if(!GuLogReady())return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}
VOID GuReleaseAssets(VOID) {
  if(!GuSafeToReturn())return;
  for(UINTN i=0;i<5;i++){GuFree((VOID *)GuAssets[i].firmware.data);if(Gu.fault)return;memset(&GuAssets[i],0,sizeof(GuAssets[i]));}
}
EFI_STATUS GuIoClose(VOID) {
  return GuSafeToReturn()?EFI_SUCCESS:EFI_ACCESS_DENIED;
}
