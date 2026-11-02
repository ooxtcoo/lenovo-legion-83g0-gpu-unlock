/* Diagnostic snapshot only. Each original ColdCheck access is performed once.
 * A failed read and later suppressed reads are UNKNOWN, never measured all-ones. */
static struct {
  UINT32 v[14],valid,called;BOOLEAN rejected,fault,first;int assessment;
  UINT32 offset;EFI_STATUS raw,status,console;const char *stage;
} Cold;
static UINT32 ColdRead(UINTN i,UINT32 offset) {
  BOOLEAN before=Gu.fault;UINT32 v=GuRead32(offset);
  Cold.called|=1U<<i;Cold.v[i]=v;
  if(!before&&!Gu.fault)Cold.valid|=1U<<i;
  return v;
}
static EFI_STATUS ColdResult(EFI_STATUS status) {
  Cold.status=status;Cold.rejected=status!=EFI_SUCCESS;Cold.fault=Gu.fault;
  Cold.first=Gu.read_fault_seen;Cold.offset=Gu.read_fault_offset;
  Cold.raw=Gu.read_fault_status;Cold.console=GuConsoleStatus();return status;
}
static const char *ColdGate(UINTN i,BOOLEAN pass) {
  return !(Cold.valid&(1U<<i))?"UNKNOWN":pass?"PASS":"FAIL";
}
static VOID ColdDescribe(VOID) {
  if(!Cold.rejected)return;
  GuLog("\nCOLD REFUSAL v5-auto-v2\n");
  GuLog("stage=%s status=%016llx\n",Cold.stage,(unsigned long long)Cold.status);
  GuLog("%s CHIP raw=%08x expected=%08x\n",ColdGate(0,Cold.v[0]==GU_BOARD_CHIP),Cold.v[0],GU_BOARD_CHIP);
  BOOLEAN gvalid=(Cold.valid&14)==14;
  GuLog("%s GFW assess=%d expected=0 config_ok=1\n",!gvalid?"UNKNOWN":Cold.assessment?"FAIL":"PASS",Cold.assessment);
  GuLog("%s GFW_PLM raw=%08x need bit0=1, not ffffffff\n",ColdGate(1,Cold.v[1]!=0xffffffffU&&(Cold.v[1]&1)),Cold.v[1]);
  GuLog("%s GFW_BOOT raw=%08x need low8=ff, not ffffffff\n",ColdGate(2,Cold.v[2]!=0xffffffffU&&(Cold.v[2]&255)==255),Cold.v[2]);
  GuLog("%s GFW_CPUCTL raw=%08x need bit4=1, not ffffffff\n",ColdGate(3,Cold.v[3]!=0xffffffffU&&(Cold.v[3]&16)),Cold.v[3]);
  GuLog("%s WPR2_HI raw=%08x expected >>4=0\n",ColdGate(4,!(Cold.v[4]>>4)),Cold.v[4]);
  GuLog("%s FEAT_PLM raw=%08x expected=ffffff8f\n",ColdGate(5,Cold.v[5]==0xffffff8f),Cold.v[5]);
  GuLog("%s SS0 raw=%08x expected=53422157/53402157 or fresh C\n",ColdGate(6,KnownSs0(Cold.v[6]) || (ExceptionalSs0Confirmed && Cold.v[6]==ColdEntrySs0)),Cold.v[6]);
  GuLog("%s SS1 raw=%08x expected=00000006\n",ColdGate(7,Cold.v[7]==6),Cold.v[7]);
  GuLog("%s VRAM raw=%08x expected=00003ff8\n",ColdGate(13,Cold.v[13]==0x3ff8),Cold.v[13]);
  GuLog("%s fault=%u expected=0; valid-mask=%04x called=%04x\n",Cold.fault?"FAIL":"PASS",Cold.fault,Cold.valid,Cold.called);
  GuLog("first-read-fault=%u offset=%08x raw-status=%016llx\n",Cold.first,Cold.offset,(unsigned long long)Cold.raw);
  GuLog("DIAG WPR_CFG=%08x FBPA=%08x WPR=%08x\n",Cold.v[8],Cold.v[9],Cold.v[10]);
  GuLog("DIAG 823b00=%08x 8403c4=%08x (not value gates)\n",Cold.v[11],Cold.v[12]);
  GuLog("UNKNOWN=not read successfully; raw field is NOT measured.\n");
  GuLog("mask bits: chip,gfw3,hi,plm,ss0,ss1,cfg,fbpa,wpr,raw2,vram\n");
  GuLog("GFW -11=running; -19=inaccessible; UNKNOWN assess not usable.\n");
  GuLog("%s console-at-check=%016llx now=%016llx\n",Cold.console?"FAIL":"PASS",(unsigned long long)Cold.console,(unsigned long long)GuConsoleStatus());
  GuLog(Gu.retained?"STOP: hold physical POWER OFF. Do not boot Windows.\n":
    "NO DETACH/ATTEMPT. Release keys; Space/Q/Esc returns. No auto-return.\n");
}
