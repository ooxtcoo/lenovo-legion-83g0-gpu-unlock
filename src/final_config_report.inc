/* Compare original PCI256 with only validated cleared CED/URD permitted.
 * No PCI status writes or baseline rebasing are added.
 * Snapshot before terminal console callbacks; never reread during reporting. */
static struct {
  UINT32 current[64];
  UINTN valid;
  EFI_STATUS read;
  BOOLEAN rejected,fault_before,cleared;
  GU_CAPS caps;
} FinalConfig;
static BOOLEAN FinalConfigCheck(VOID) {
  /* Each pass owns its valid prefix; never report stale first-pass dwords. */
  memset(&FinalConfig,0,sizeof(FinalConfig));
  FinalConfig.fault_before=Gu.fault;
  for(UINTN i=0;i<64;i++) {
    FinalConfig.read=GuConfigRead(EfiPciIoWidthUint32,(UINT32)i*4,&FinalConfig.current[i]);
    if(FinalConfig.read!=EFI_SUCCESS){FinalConfig.rejected=TRUE;return FALSE;}
    FinalConfig.valid=i+1;
  }
  FinalConfig.rejected=Gu.fault || !GuSafeToReturn() ||
    !GuCheckFinalConfig(InitialConfig,FinalConfig.current,&FinalConfig.caps,&FinalConfig.cleared);
  return !FinalConfig.rejected;
}
static VOID FinalConfigAccepted(VOID) {
  if(FinalConfig.rejected || !FinalConfig.cleared)return;
  UINTN offset=FinalConfig.caps.pcie+8U;
  GuLog("PCI EXCEPTION: only previously set CED/URD cleared; no status writes\n");
  GuLog("PCIe=%02x v%u off=%02x initial=%08x current=%08x\n",
    (unsigned)FinalConfig.caps.pcie,(unsigned)FinalConfig.caps.pcie_version,
    (unsigned)offset,InitialConfig[offset/4],FinalConfig.current[offset/4]);
}
static VOID FinalConfigDescribe(VOID) {
  if(!FinalConfig.rejected)return;
  UINTN differences=0,shown=0;
  for(UINTN i=0;i<FinalConfig.valid;i++)if(InitialConfig[i]!=FinalConfig.current[i])differences++;
  /* At most 24 lines, <=79 columns: two header lines + 22 difference rows.
   * valid is bytes in hex; on failure it also identifies the failed offset.
   * A preexisting fault suppresses the transport, explicitly marked below. */
  GuLog("FINAL PCI: read=%016llx valid=%03x/100 diffs=%u prior-fault=%u\n",
    (unsigned long long)FinalConfig.read,(unsigned)(FinalConfig.valid*4),
    (unsigned)differences,FinalConfig.fault_before);
  GuLog("off:initial>current HEX; unread UNKNOWN; STOP - physical POWER OFF\n");
  for(UINTN i=0;i<FinalConfig.valid;i++)if(InitialConfig[i]!=FinalConfig.current[i]) {
    GuLog("%02x:%08x>%08x%s",(unsigned)(i*4),InitialConfig[i],FinalConfig.current[i],
      (++shown%3==0 || shown==differences)?"\n":" ");
  }
}
