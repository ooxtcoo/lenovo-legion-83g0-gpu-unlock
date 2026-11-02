/* V5_CODE_CLOSURE: tested experimental manual executor. */
/* Lenovo83G0 v5-auto-v2 experimental executor; admission is not safety proof. */
#include "app.h"
#include "app_io.h"
#include "compat/compat.h"
#include "checks.h"
#include "sha256.h"
#include "prom_profile.h"
#include "ownership.h"
#include "lifecycle.h"
#include "vendor/unlock_610.h"
#include "vendor/gfw_eval.h"
#include <Protocol/GraphicsOutput.h>
#include <IndustryStandard/Acpi.h>
#include <Guid/GlobalVariable.h>
static EFI_GUID PciGuid=EFI_PCI_IO_PROTOCOL_GUID,GopGuid=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
  PathGuid=EFI_DEVICE_PATH_PROTOCOL_GUID,GlobalGuid=EFI_GLOBAL_VARIABLE;
static EFI_SYSTEM_TABLE *Table;
static UINT32 InitialConfig[64];
static const char *Stage;
static BOOLEAN ExpectDetached;
static GU_RELEASE_OWNER PendingOwner;
static EFI_STATUS Step(EFI_STATUS Status) {
  GuLog("CHECK stage=%s status=%#llx\n",Stage,(unsigned long long)Status);
  return !GuLogReady() && !GU_FAILED(Status)?EFI_DEVICE_ERROR:Status;
}
static VOID ConfigLog(const UINT32 *Config) {
  for(UINTN i=0;i<64;i+=4)GuLog("config[%02x]=%08x %08x %08x %08x\n",
    (unsigned)(i*4),Config[i],Config[i+1],Config[i+2],Config[i+3]);
}
static BOOLEAN DisplayCheck(VOID) {
 return ExpectDetached?GuGraphDetachedDisplay():GuGraphAttached(FALSE);
}
#include "handoff.inc"
static BOOLEAN PhotoStarted; /* Marks automatic handoff for terminal STOP wording. */
static EFI_STATUS ReadConfig(UINT32 *Config) {
  for(UINTN i=0;i<64;i++)if(GU_FAILED(GuConfigRead(EfiPciIoWidthUint32,(UINT32)i*4,&Config[i]))) {
    GuLog("PCI config read failed: offset=%#x valid_bytes=%u; snapshot incomplete\n",(unsigned)(i*4),(unsigned)(i*4));
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}
static EFI_STATUS ConfigCheck(BOOLEAN Save) {
  UINT32 config[64];GU_CAPS caps;UINTN segment,bus,device,function;
  EFI_STATUS status=Gu.pci->GetLocation(Gu.pci,&segment,&bus,&device,&function);
  GuLog("target location: status=%#llx\n",(unsigned long long)status);
  if(GU_FAILED(status))return status;
  status=ReadConfig(config);if(GU_FAILED(status))return status;
  BOOLEAN checked=GuCheckConfig(config,segment,bus,device,function,&caps);
  BOOLEAN valid=checked && config[2]==0x030000a1 && config[0xc8/4]==0x00050011 &&
    config[0x10/4]==0x85000000 && !(config[(caps.pcie+8)/4]&0x00200000U);
  BOOLEAN same=Save || !memcmp(config,InitialConfig,sizeof(config));
  if(Save || !valid || !same)ConfigLog(config);
  GuLog("target BDF=%zu:%zu:%zu.%zu checks=%u pm=%#x pcie=%#x msi=%#x msix=%#x\n",
    (size_t)segment,(size_t)bus,(size_t)device,(size_t)function,checked,
    (unsigned)caps.pm,(unsigned)caps.pcie,(unsigned)caps.msi,(unsigned)caps.msix);
  if(!valid)return EFI_UNSUPPORTED;
  if(!same)return EFI_NOT_READY;
  if(Save)memcpy(InitialConfig,config,sizeof(config));
  Gu.pm_cap=caps.pm;Gu.pcie_cap=caps.pcie;Gu.pcie_version=caps.pcie_version;Gu.msi_cap=caps.msi;Gu.msix_cap=caps.msix;
  return EFI_SUCCESS;
}
static EFI_STATUS RestoreDecode(VOID) {
  UINT32 current[64],normalized[64],after[64];UINTN segment,bus,device,function;
  if(!Gu.pci || !Gu.owned || !Gu.armed || !Gu.display_ok || Gu.started || Gu.allocations ||
    Gu.dma_count || Gu.fault || Gu.retained)return EFI_ACCESS_DENIED;
  EFI_STATUS status=Gu.pci->GetLocation(Gu.pci,&segment,&bus,&device,&function);
  if(GU_FAILED(status))return status;
  status=ReadConfig(current);if(GU_FAILED(status))return status;
  GuLogText("config before decode restore (256 valid bytes):\n");ConfigLog(current);
  if(!GuTargetConfig(current,segment,bus,device,function))return EFI_UNSUPPORTED;
  UINT32 missing=InitialConfig[1]&~current[1]&3U;
  memcpy(normalized,current,sizeof(normalized));normalized[1]|=missing;
  /* Restore only cleared, previously enabled IO/MEM bits. Every other bit,
     including BME, status, BAR, ROM and PM/capability state must be unchanged. */
  if(memcmp(normalized,InitialConfig,sizeof(normalized))) {
    GuLogText("decode restore refused: config differs beyond cleared original IO/MEM bits\n");
    return EFI_UNSUPPORTED;
  }
  if(!missing){GuLogText("decode restore not needed\n");return EFI_SUCCESS;}
  if(!Gu.pci->Attributes)return EFI_UNSUPPORTED;
  UINT64 needed=(missing&1?EFI_PCI_IO_ATTRIBUTE_IO:0)|(missing&2?EFI_PCI_IO_ATTRIBUTE_MEMORY:0);
  UINT64 supported=0,attributes=0;
  status=Gu.pci->Attributes(Gu.pci,EfiPciIoAttributeOperationSupported,0,&supported);
  GuLog("decode supported attributes: status=%#llx mask=%#llx\n",(unsigned long long)status,(unsigned long long)supported);
  if(GU_FAILED(status))return status;
  if((supported&needed)!=needed)return EFI_UNSUPPORTED;
  status=Gu.pci->Attributes(Gu.pci,EfiPciIoAttributeOperationGet,0,&attributes);
  GuLog("decode current attributes: status=%#llx mask=%#llx\n",(unsigned long long)status,(unsigned long long)attributes);
  if(GU_FAILED(status))return status;
  GuLog("decode restore intent: saved_command=%#x current_command=%#x missing=%#x attributes=%#llx\n",
    InitialConfig[1]&0xffff,current[1]&0xffff,missing,(unsigned long long)needed);
  if(!GuLogReady() || Gu.fault)return EFI_DEVICE_ERROR;
  /* Never use EFI_PCI_DEVICE_ENABLE: it would also enable bus mastering. */
  EFI_STATUS enabled=Gu.pci->Attributes(Gu.pci,EfiPciIoAttributeOperationEnable,needed,NULL);
  GuLog("decode enable status=%#llx\n",(unsigned long long)enabled);
  if(!GuLogReady() || Gu.fault)return GU_FAILED(enabled)?enabled:EFI_DEVICE_ERROR;
  /* Even an error may follow a partial hardware effect. Capture it if readable,
     but never turn an API error into success merely because readback matches. */
  status=ReadConfig(after);
  BOOLEAN matched=FALSE;
  if(!GU_FAILED(status)) {
    GuLogText("config after decode restore (256 valid bytes):\n");ConfigLog(after);
    matched=!memcmp(after,InitialConfig,sizeof(after));GuLog("decode readback matched=%u\n",matched);
  }
  if(GU_FAILED(enabled))return enabled;
  if(GU_FAILED(status))return status;
  if(!GuLogReady() || Gu.fault)return EFI_DEVICE_ERROR;
  return matched?EFI_SUCCESS:EFI_COMPROMISED_DATA;
}
static EFI_STATUS FindTarget(VOID) {
  EFI_HANDLE *handles=NULL;UINTN count=0,found=0;
  EFI_STATUS status=Gu.bs->LocateHandleBuffer(ByProtocol,&PciGuid,NULL,&count,&handles);
  GuLog("PCI enumeration: status=%#llx handles=%zu\n",(unsigned long long)status,(size_t)count);
  if(GU_FAILED(status) || !handles || !count || count>256){if(handles)GuFree(handles);return EFI_NOT_FOUND;}
  for(UINTN i=0;i<count;i++) {
    EFI_PCI_IO_PROTOCOL *pci=NULL;UINT32 id=0;
    status=Gu.bs->HandleProtocol(handles[i],&PciGuid,(VOID **)&pci);
    if(!GU_FAILED(status) && pci && !pci->Pci.Read)status=EFI_UNSUPPORTED;
    if(!GU_FAILED(status) && pci)status=pci->Pci.Read(pci,EfiPciIoWidthUint32,0,1,&id);
    if(GU_FAILED(status) || !pci){GuLog("PCI scan failed: index=%zu status=%#llx\n",(size_t)i,(unsigned long long)status);status=EFI_DEVICE_ERROR;break;}
    if(id==GU_BOARD_ID){found++;Gu.handle=handles[i];Gu.pci=pci;}
  }
  GuFree(handles);GuLog("PCI target matches=%zu status=%#llx\n",(size_t)found,(unsigned long long)status);
  if(GU_FAILED(status) || found!=1)return EFI_UNSUPPORTED;
  if(!Gu.pci->GetLocation||!Gu.pci->Pci.Read||!Gu.pci->Pci.Write||!Gu.pci->Mem.Read||!Gu.pci->Mem.Write||
     !Gu.pci->GetBarAttributes||!Gu.pci->Attributes||!Gu.pci->AllocateBuffer||!Gu.pci->FreeBuffer||
     !Gu.pci->Map||!Gu.pci->Unmap||!Gu.pci->Flush)return EFI_UNSUPPORTED;
  return ConfigCheck(TRUE);
}
static EFI_STATUS BarCheck(VOID) {
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *r=NULL;
  EFI_STATUS status=Gu.pci->GetBarAttributes(Gu.pci,0,NULL,(VOID **)&r);
  GuLog("bar0 query: status=%#llx\n",(unsigned long long)status);
  if(!GU_FAILED(status)) {
    if(r && r->Desc==ACPI_ADDRESS_SPACE_DESCRIPTOR && r->Len==sizeof(*r)-3)
      GuLog("bar0 descriptor: type=%u granularity=%llu min=%#llx max=%#llx length=%#llx translation=%#llx\n",
        (unsigned)r->ResType,(unsigned long long)r->AddrSpaceGranularity,(unsigned long long)r->AddrRangeMin,
        (unsigned long long)r->AddrRangeMax,(unsigned long long)r->AddrLen,(unsigned long long)r->AddrTranslationOffset);
    if(!r || r->Desc!=ACPI_ADDRESS_SPACE_DESCRIPTOR || r->Len!=sizeof(*r)-3 || r->ResType!=0 ||
      r->AddrRangeMin!=0x85000000 || r->AddrLen!=0x1000000 || r->AddrTranslationOffset ||
      /* UEFI describes an inclusive end; pinned EDK2 PciBusDxe instead returns
         PciBar.Alignment. Both representations describe this exact 16 MiB BAR. */
      (r->AddrRangeMax!=0x85ffffff && r->AddrRangeMax!=0x00ffffff) ||
      r->AddrSpaceGranularity!=32 || ((UINT8 *)(r+1))[0]!=ACPI_END_TAG_DESCRIPTOR)status=EFI_UNSUPPORTED;
    else {
      UINT8 *raw=(UINT8 *)r,sum=0;
      for(UINTN i=0;i<sizeof(*r)+2;i++)sum=(UINT8)(sum+raw[i]);
      if(raw[sizeof(*r)+1] && sum)status=EFI_COMPROMISED_DATA;
      else {Gu.bar_base=r->AddrRangeMin;Gu.bar_size=r->AddrLen;}
    }
  }
  if(r)GuFree(r);return status;
}
static UINT32 ColdEntrySs0;
static BOOLEAN ColdEntrySet,ExceptionalSs0Confirmed;
static BOOLEAN KnownSs0(UINT32 value) {return value==0x53422157U || value==0x53402157U;}
#include "cold_report.inc"
#include "final_config_report.inc"
static EFI_STATUS ColdCheck(VOID) {
  memset(&Cold,0,sizeof(Cold));Cold.stage=Stage;Cold.assessment=-19;
  UINT32 chip=ColdRead(0,0);GuLog("chip: actual=%#x expected=%#x\n",chip,GU_BOARD_CHIP);
  if(chip!=GU_BOARD_CHIP)return ColdResult(EFI_UNSUPPORTED);
  struct gpu_unlock_gfw_sample s={.config_ok=true,.plm=ColdRead(1,GU_GFW_PLM_OFFSET),
    .boot=ColdRead(2,GU_GFW_BOOT_OFFSET),.cpuctl=ColdRead(3,GU_GFW_CPUCTL_OFFSET)};
  UINT32 hi=ColdRead(4,0x1fa828),plm=ColdRead(5,GU_FEAT_PLM_OFFSET),ss0=ColdRead(6,GU_FEAT_SS0_OFFSET),ss1=ColdRead(7,GU_FEAT_SS1_OFFSET);
  GuLog("cold state: chip=%#x plm=%#x gfw=%#x/%#x/%#x WPR2_HI=%#x SS0=%#x SS1=%#x\n",
    GU_BOARD_CHIP,plm,s.plm,s.boot,s.cpuctl,hi,ss0,ss1);
  UINT32 wpr_cfg=ColdRead(8,0x1fa7cc),fbpa=ColdRead(9,0x9a0148),wpr=ColdRead(10,0x1fa7c4);
  GuLog("cold PLMs: WPR_CFG=%#x FBPA=%#x WPR=%#x FEAT=%#x\n",wpr_cfg,fbpa,wpr,plm);
  /* Diagnostic reads requested for RUN07 follow-up. The meaning/bit map of
     0x823b00 is unverified on AD103: do not decode it or write it.
     0x8403c4 is the reset-PLM candidate at the standard Falcon +0x3c4.
     Neither raw value is a new execution gate. Transport faults still latch. */
  UINT32 source_raw=ColdRead(11,0x823b00),reset_plm=ColdRead(12,0x8403c4);
  GuLog("cold diagnostic: reg=0x823b00 name=SOURCE_ENABLE_CANDIDATE_RAW value=%#x decoded=0\n",source_raw);
  GuLog("cold diagnostic: reg=0x8403c4 name=RESET_PLM_CANDIDATE_RAW value=%#x decoded=0\n",reset_plm);
  UINT32 vram=ColdRead(13,0x1183a4);
  /* Lenovo83G0 photographed successful collector session, not inferred from
     product VRAM capacity. Exact measured cold selectors are admission pins,
     not proof that a firmware transaction or Windows recovery is viable. */
  GuLog("VRAM register=%#x (%u MiB); accepted=0x3ff8; layout uses actual register\n",vram,vram);
  Cold.assessment=gpu_unlock_gfw_assess(&s);
  if(Gu.fault || Cold.assessment || hi>>4 || plm!=0xffffff8f ||
    !ss0 || ss0==0xffffffffU || ss1!=6 || vram!=0x3ff8)return ColdResult(EFI_NOT_READY);
  /* First pass only stages the exact observation; it cannot authorize detach.
   * Unknown SS0 has no known-safe interpretation. C is experimental consent,
   * never permission to ignore any other predicate or a changed observation. */
  if(!ColdEntrySet){ColdEntrySs0=ss0;ColdEntrySet=TRUE;}
  else if(!KnownSs0(ColdEntrySs0)){
    if(!ExceptionalSs0Confirmed || ss0!=ColdEntrySs0){ExceptionalSs0Confirmed=FALSE;return ColdResult(EFI_NOT_READY);}
  }else if(!KnownSs0(ss0))return ColdResult(EFI_NOT_READY);
  return ColdResult(Gu.fault?EFI_DEVICE_ERROR:EFI_SUCCESS);
}
static VOID LogHash(const char *Name,const VOID *Data,UINTN Bytes) {
  static const char digits[]="0123456789abcdef";
  UINT8 digest[32];char hex[65];GuSha256(Data,Bytes,digest);
  for(UINTN i=0;i<32;i++){hex[2*i]=digits[digest[i]>>4];hex[2*i+1]=digits[digest[i]&15];}
  hex[64]=0;GuLog("%s bytes=%zu SHA256=%s\n",Name,(size_t)Bytes,hex);
}
static EFI_STATUS PromCheck(VOID) {
  /* Measured PCI_IO exposed prefix, NOT a complete option-ROM chain. The
     exact 0x24e00 bytes equal the private normalized PROM prefix; both PCIR
     images have last=0, so do not assert a complete chain or signature trust.
     PROM transport remains independently checked below. */
  if(!Gu.pci->RomImage || Gu.pci->RomSize!=0x24e00 ||
     !GuHashMatches(Gu.pci->RomImage,0x24e00,"e3f39eb2b2ed89e4e36fe8e28ed2b9e06c19c911086015fa03bca6b5f47020bd"))return EFI_CRC_ERROR;
  VOID *prom=GuAlloc(1048576);if(!prom)return EFI_OUT_OF_RESOURCES;
  GuCopyFromIo(prom,(VOID *)(UINTN)(Gu.bar_base+GU_PROM_BAR_OFFSET),1048576);
  BOOLEAN good=!Gu.fault && GuHashMatches(prom,GU_PROM_VIEW_BYTES,GU_PROM_VIEW_SHA256);
  GuLog("PCI RomImage hash matched=1; PROM read width=32 io_fault=%u\n",(unsigned)Gu.fault);
  LogHash("live PROM",prom,1048576);
  LogHash("PROM verified view [0,0xa2c00)",prom,GU_PROM_VIEW_BYTES);
  GuLog("live PROM view hash matched=%u bar_offset=%#x bytes=%#x expected=%s\n",
    (unsigned)good,GU_PROM_BAR_OFFSET,GU_PROM_VIEW_BYTES,GU_PROM_VIEW_SHA256);
  /* Never persist private PROM bytes, including on mismatch or partial read. */
  if(!good)GuLog("PROM mismatch/transport fault; no private dump written\n");
  GuFree(prom);return good?EFI_SUCCESS:Gu.fault?EFI_DEVICE_ERROR:EFI_CRC_ERROR;
}
static EFI_STATUS Arm(VOID) {
  if(!GuLogReady() || !ColdEntrySet || Gu.fault)return EFI_DEVICE_ERROR;
  EFI_STATUS s=GuGraphConsent();if(s!=EFI_SUCCESS)return s;
  if(KnownSs0(ColdEntrySs0))return EFI_SUCCESS;
  s=GuGraphSs0Consent(ColdEntrySs0);
  ExceptionalSs0Confirmed=s==EFI_SUCCESS && GuLogReady() && !Gu.fault;
  return ExceptionalSs0Confirmed?EFI_SUCCESS:s==EFI_SUCCESS?EFI_DEVICE_ERROR:s;
}
static VOID FinalRefusalReport(VOID) {if(Cold.rejected)ColdDescribe();else GuIoFinalDescribe();}
static VOID PausePreflightFailure(EFI_STATUS Status) {GuIoDescribe();GuGraphSetRefusalReport(FinalRefusalReport);GuGraphRefuseAt(Status,Stage);}
static VOID Halt(VOID) {
  Gu.retained=TRUE;
  if(GuGraphFreshConsole()){GuScreenDetached=FALSE;GuReplayDetached();}
  GuLog("STOP stage=%s\n",Stage);
  GuLogText(FinalConfig.rejected?
    "STOP: final PCI invariant not established. Windows NOT started.\nPOWER OFF using the physical power button; do not continue to Windows.\n":
    PhotoStarted?"STOP: Windows handoff refused. Do not return to launcher.\nPOWER OFF using the physical power button.\n":
    "STOP: cleanup cannot be proven. Resources remain reserved.\nPOWER OFF using the physical power button; do not continue to Windows.\n");
  ColdDescribe();FinalConfigDescribe();
  /* No ResetSystem, ExitBootServices, retry, protocol close or memory release. */
  for(;;)Gu.bs->Stall(1000000);
}
static VOID LogContainment(VOID) {
  /* Called only after any TPL_NOTIFY containment section has ended. */
  GuLog("BME containment: attribute_status=%#llx hardware_status=%#llx command=%#x cached=%#llx pending=%u\n",
    (unsigned long long)Gu.master_attribute_status,(unsigned long long)Gu.master_hardware_status,
    Gu.master_last_command,(unsigned long long)Gu.master_last_attributes,Gu.master_attributes_pending);
}
static VOID ReleaseHalt(EFI_STATUS Status,BOOLEAN Attempted) {
  /* Never refresh into a different PCI interface and then write its command
   * register. Core transaction containment owns its original interface; an
   * uncertain disconnect/reconnect is terminal with resources retained. */
  GuLog("FINAL status=%#llx stage=%s attempted=%u driver_release=1 clean=0; no further PCI cleanup\n",
    (unsigned long long)Status,Stage,Attempted);
  Halt();
}
EFI_STATUS EFIAPI EfiMain(EFI_HANDLE Image,EFI_SYSTEM_TABLE *St) {
  if(!St || !St->BootServices || !St->RuntimeServices || !St->RuntimeServices->GetVariable)return EFI_INVALID_PARAMETER;
  WindowsImage=NULL;WindowsVolume=NULL;WindowsDevicePath=NULL;GuScreenDetached=FALSE;ExpectDetached=FALSE;
  memset(&Cold,0,sizeof(Cold));memset(&FinalConfig,0,sizeof(FinalConfig));
  PhotoStarted=FALSE;HandoffSs0=HandoffSs1=0;
  ColdEntrySet=ExceptionalSs0Confirmed=FALSE;ColdEntrySs0=0;
  Table=St;GuInit(St->BootServices,NULL);Gu.image=Image;Gu.display_check=DisplayCheck;
  memset(&PendingOwner,0,sizeof(PendingOwner));
  Stage="graph-init";EFI_STATUS status=GuGraphInit(Image,St);BOOLEAN attempted=FALSE,release_attempted=FALSE;
  if(status!=EFI_SUCCESS)return GuGraphRefuse(status);
  Stage="usb-assets";status=GuIoOpen(Image,St);if(GU_FAILED(status))goto finish;
  UINT8 secure=0xff;UINTN bytes=1;
  Stage="secure-boot";
  status=St->RuntimeServices->GetVariable(L"SecureBoot",&GlobalGuid,NULL,&bytes,&secure);
  if(GU_FAILED(status) || bytes!=1 || secure){status=EFI_SECURITY_VIOLATION;goto finish;}
  status=Step(status);if(GU_FAILED(status))goto finish;
  Stage="watchdog";status=Step(Gu.bs->SetWatchdogTimer(0,0,0,NULL));if(GU_FAILED(status))goto finish;
  Stage="target";status=Step(FindTarget());if(GU_FAILED(status))goto finish;
  Stage="display";Gu.display_ok=DisplayCheck();status=Step(Gu.display_ok?EFI_SUCCESS:EFI_UNSUPPORTED);if(GU_FAILED(status))goto finish;
  Stage="ownership";status=GuClaimController();
  if(status==EFI_ACCESS_DENIED) {
    PendingOwner.agent=GuGraphOwner();status=PendingOwner.agent?EFI_SUCCESS:EFI_UNSUPPORTED;
    if(!GU_FAILED(status))GuLogText("Exclusive ownership deferred until AUTO admission and explicit driver release.\n");
  }
  if(!GU_FAILED(status) && !PendingOwner.agent)status=EFI_UNSUPPORTED;
  status=Step(status);if(GU_FAILED(status))goto finish;
  Stage="config-recheck";status=Step(ConfigCheck(FALSE));if(GU_FAILED(status))goto finish;
  Stage="bar0";status=Step(BarCheck());if(GU_FAILED(status))goto finish;
  Stage="cold-state";status=Step(ColdCheck());if(GU_FAILED(status))goto finish;
  Stage="prom";status=Step(PromCheck());if(GU_FAILED(status))goto finish;
  Stage="assets";status=Step(GuLoadAssets());if(GU_FAILED(status))goto finish;
  Stage="windows-preload";status=Step(PreloadWindows());if(GU_FAILED(status))goto finish;
  Stage="arming";status=Step(Arm());if(GU_FAILED(status))goto finish;
  Stage="armed-config";status=Step(ConfigCheck(FALSE));if(GU_FAILED(status))goto finish;
  Stage="armed-display";Gu.display_ok=DisplayCheck();status=Step(Gu.display_ok?EFI_SUCCESS:EFI_UNSUPPORTED);if(GU_FAILED(status))goto finish;
  Stage="armed-cold-state";status=Step(ColdCheck());if(GU_FAILED(status))goto finish;
  if(PendingOwner.agent) {
    GU_RELEASE_OWNER current;
    Stage="owner-recheck";
    GuLogText("AUTO authorized: release RTX firmware driver once, then revalidate.\n");
    if(!GuLogReady()){status=EFI_DEVICE_ERROR;goto finish;}
    /* Durable intent precedes the last owner snapshot. No file IO between a
       successful final comparison and DisconnectController. */
    status=GuGraphAttached(FALSE)?EFI_SUCCESS:EFI_ABORTED;
    current.agent=GuGraphOwner();
    if(current.agent!=PendingOwner.agent)status=EFI_ABORTED;
    if(GU_FAILED(status)){status=Step(status);goto finish;}
    /* Final exceptional-value reread after all prompt/output/graph callbacks.
     * No output/input callback between this comparison and driver release. */
    if(!KnownSs0(ColdEntrySs0)) {
      UINT32 ss0=GuRead32(GU_FEAT_SS0_OFFSET),ss1=GuRead32(GU_FEAT_SS1_OFFSET);
      if(!ExceptionalSs0Confirmed || ss0!=ColdEntrySs0 || ss1!=6 || Gu.fault || !GuLogReady()) {
        ExceptionalSs0Confirmed=FALSE;status=EFI_NOT_READY;goto finish;
      }
    }
    EFI_PCI_IO_PROTOCOL *prior=Gu.pci;
    Stage="driver-release";release_attempted=TRUE;Gu.armed=TRUE;Gu.pci=NULL;
    GuScreenDetached=TRUE;ExpectDetached=TRUE;
    EFI_STATUS released=Gu.bs->DisconnectController(Gu.handle,current.agent,NULL);
    EFI_TPL refresh_tpl=Gu.bs->RaiseTPL(TPL_NOTIFY);
    EFI_STATUS refreshed=GuRefreshTargetPci();
    Gu.bs->RestoreTPL(refresh_tpl);
    GuLog("driver release: status=%#llx refresh=%#llx same_pci=%u\n",
      (unsigned long long)released,(unsigned long long)refreshed,(unsigned)(Gu.pci==prior));
    status=Step(released!=EFI_SUCCESS?EFI_DEVICE_ERROR:refreshed!=EFI_SUCCESS?EFI_DEVICE_ERROR:Gu.pci!=prior?EFI_COMPROMISED_DATA:EFI_SUCCESS);
    if(status==EFI_SUCCESS&&!GuGraphDetached())status=EFI_ACCESS_DENIED;
    if(GU_FAILED(status))goto finish;
    Stage="ownership-after-release";status=Step(GuClaimController());if(GU_FAILED(status))goto finish;
    Stage="display-after-release";Gu.display_ok=DisplayCheck();status=Step(Gu.display_ok?EFI_SUCCESS:EFI_UNSUPPORTED);if(GU_FAILED(status))goto finish;
    Stage="decode-after-release";status=Step(RestoreDecode());if(GU_FAILED(status))goto finish;
    Stage="config-after-release";status=Step(ConfigCheck(FALSE));if(GU_FAILED(status))goto finish;
    Stage="bar0-after-release";status=Step(BarCheck());if(GU_FAILED(status))goto finish;
    Stage="cold-after-release";status=Step(ColdCheck());if(GU_FAILED(status))goto finish;
    Stage="prom-after-release";status=Step(PromCheck());if(GU_FAILED(status))goto finish;
  }
  GuLogText("ARMED: one transaction is starting. Do not reset or remove the USB.\n");
  if(!GuLogReady()){status=EFI_DEVICE_ERROR;goto finish;}
  Stage="transaction";Gu.armed=TRUE;attempted=TRUE;
  UINT64 mask=0xffffffffULL;
  struct pci_dev device={.dev={.dma_mask=&mask,.coherent_dma_mask=0xffffffffULL},.pm_cap=Gu.pm_cap,.refs=1};
  struct gpu_unlock_610_shared shared={0};
  int result=gpu_unlock_610_load_shared(&device,&shared);
  if(!result)result=gpu_unlock_610_unlock(&device,&shared);
  /* Also covers failures before GuStarted, including BME-enable readback. */
  EFI_STATUS contained=GuClearMaster();
  LogContainment();
  GuLog("transaction result=%d containment=%#llx started=%u quiesced=%u fault=%u retained=%u maps=%u FLR=%u\n",
    result,(unsigned long long)contained,Gu.started,Gu.quiesced,Gu.fault,Gu.retained,Gu.dma_count,Gu.flrs);
  if(GU_FAILED(contained) || !GuSafeToReturn() || Gu.allocations || (Gu.started && result))Halt();
  gpu_unlock_610_release_shared(&shared);Gu.armed=FALSE;
  status=result?EFI_DEVICE_ERROR:EFI_SUCCESS;
finish:
  /* Step can refuse successful register gates after a console failure. */
  if(status!=EFI_SUCCESS && Cold.stage==Stage && Cold.called){Cold.rejected=TRUE;Cold.status=status;}
  if(!GU_FAILED(status) && attempted && release_attempted) {
    Stage="post-unlock-handoff";status=RestoreDisplayForWindows(PendingOwner.agent);
  }
  if(release_attempted && (GU_FAILED(status) || Gu.fault))ReleaseHalt(status,attempted);
  if(attempted && !GuSafeToReturn())Halt();
  GuLog("FINAL status=%#llx stage=%s attempted=%u driver_release=%u clean=%u\n",(unsigned long long)status,Stage,attempted,release_attempted,GuSafeToReturn());
  if(!GuLogReady()){GuLog("SECONDARY logging/not-ready; primary retained\n");if(status==EFI_SUCCESS)status=EFI_DEVICE_ERROR;if(release_attempted)ReleaseHalt(status,attempted);if(Gu.started)Halt();}
  if(attempted && !GuSafeToReturn())Halt();
  EFI_STATUS closed=GuIoClose();
  if(GU_FAILED(closed)){if(release_attempted)ReleaseHalt(closed,attempted);if(Gu.started)Halt();GuLog("SECONDARY cleanup status=%#llx\n",(unsigned long long)closed);if(status==EFI_SUCCESS)status=closed;}
  if(Gu.owned) {
    closed=Gu.bs->CloseProtocol(Gu.handle,&PciGuid,Image,NULL);
    if(GU_FAILED(closed)){if(release_attempted)ReleaseHalt(closed,attempted);if(Gu.started)Halt();GuLog("SECONDARY cleanup status=%#llx\n",(unsigned long long)closed);if(status==EFI_SUCCESS)status=closed;}else Gu.owned=FALSE;
  }
  GuReleaseAssets();
  if(release_attempted&&(Gu.fault||!GuSafeToReturn()))Halt();
  if(!GU_FAILED(status) && attempted) {
    GuConsole("Checking final PCI for AUTO handoff; Windows NOT started.\n");
    if(!GuLogReady())Halt();
    /* Fresh role-matched graph after all output/file callbacks; no cached GOP. */
    if(!GuGraphAttached(TRUE))Halt();
    Stage="final-config-before-StartImage";
    if(!FinalConfigCheck())Halt();
    FinalConfigAccepted();
    PhotoStarted=TRUE;
    GuConsole("AUTO: final fresh graph/PCI check, then START WINDOWS. No retry.\n");
    /* Output may change state: repeat strict graph and original PCI256.
     * No console/input callbacks between this comparison and StartImage. */
    Stage="post-AUTO-final-graph";
    if(!GuLogReady() || !GuSafeToReturn() || Gu.fault || !GuGraphAttached(TRUE))Halt();
    Stage="post-AUTO-final-config-before-StartImage";
    if(!FinalConfigCheck())Halt();
    UINTN bytes=0;CHAR16 *data=NULL;
    status=Gu.bs->StartImage(WindowsImage,&bytes,&data);
    GuScreenDetached=TRUE;
    if(GuGraphFreshConsole()){GuScreenDetached=FALSE;GuLog("StartImage returned status=%#llx; never retry or return to launcher\n",(unsigned long long)status);}
    /* ExitData is retained: Windows may have changed platform state. */
    /* Returning after a hardware attempt could permit accidental rerun. */
    GuScreenDetached=TRUE;Halt();
  }
  else GuConsole("Attempt cancelled or rejected. Returning without another attempt.\n");
  if(WindowsImage)Gu.bs->UnloadImage(WindowsImage);
  GuFree(WindowsDevicePath);WindowsDevicePath=NULL;
  if(status!=EFI_SUCCESS && !release_attempted)PausePreflightFailure(status);
  return status!=EFI_SUCCESS&&!EFI_ERROR(status)?EFI_DEVICE_ERROR:status;
}
