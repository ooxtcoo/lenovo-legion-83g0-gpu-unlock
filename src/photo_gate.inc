/* Post-recovery consent only. Never reachable from an earlier STOP.
 * Firmware calls are not interruptible; no claim of unlimited safe wall time.
 * No Reset(input), text-mode switch, automatic timeout start or PCI write. */
static BOOLEAN PhotoStarted;
static BOOLEAN PhotoConsent(VOID) {
  EFI_GUID inputGuid=EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *in=NULL;EFI_INPUT_KEY key;EFI_STATUS s;
  PhotoStarted=TRUE;
  GuConsole("\n============================================================\n");
  GuConsole("RESULT PHOTO - Windows NOT started\n");
  GuLog("POST-GOP SS0=%08x SS1=%08x unlock_matched=%u\n",
    HandoffSs0,HandoffSs1,HandoffSs0==0x88888888U && HandoffSs1==8);
  GuConsole("Recovery checks PASS: FLR, DMA cleanup, GOP, graph, PCI256.\n");
  GuConsole("Selector match is separate; Windows boot is NOT yet tested.\n");
  GuConsole("Take photo. Release all keys; wait for fresh S prompt.\n");
  GuConsole("No automatic start. Q/Esc -> terminal STOP; physical POWER OFF.\n");
  GuConsole("Firmware can still hang/reset; no unlimited safe-time promise.\n");
  GuConsole("============================================================\n");
  if(!GuLogReady())return FALSE;
  if(!Table->ConsoleInHandle || !Table->ConIn)return FALSE;
  s=Gu.bs->HandleProtocol(Table->ConsoleInHandle,&inputGuid,(VOID **)&in);
  if(s!=EFI_SUCCESS || !in || in!=Table->ConIn || !in->ReadKeyStroke)return FALSE;
  unsigned quiet=0;
  for(unsigned i=0;i<64 && quiet<5;i++) {
    s=in->ReadKeyStroke(in,&key);
    if(s==EFI_NOT_READY)quiet++;
    else if(s==EFI_SUCCESS) {
      if(key.ScanCode==23 || (!key.ScanCode && (key.UnicodeChar=='Q'||key.UnicodeChar=='q')))return FALSE;
      quiet=0;
    } else return FALSE;
    if(Gu.bs->Stall(100000)!=EFI_SUCCESS)return FALSE;
  }
  if(quiet<5)return FALSE;
  GuConsole("Press fresh S to recheck and START WINDOWS. Q/Esc = STOP.\n");
  if(!GuLogReady())return FALSE;
  /* Discard keys queued before/during the prompt; a stuck buffer refuses. */
  for(unsigned i=0;i<64;i++) {
    s=in->ReadKeyStroke(in,&key);
    if(s==EFI_NOT_READY)break;
    if(s!=EFI_SUCCESS || i==63)return FALSE;
    if(key.ScanCode==23 || (!key.ScanCode && (key.UnicodeChar=='Q'||key.UnicodeChar=='q')))return FALSE;
  }
  for(;;) {
    if(!GuLogReady() || Table->ConIn!=in || !in->ReadKeyStroke)return FALSE;
    s=in->ReadKeyStroke(in,&key);
    if(s==EFI_SUCCESS) {
      if(key.ScanCode==23 || (!key.ScanCode && (key.UnicodeChar=='Q'||key.UnicodeChar=='q')))return FALSE;
      if(!key.ScanCode && (key.UnicodeChar=='S'||key.UnicodeChar=='s'))return TRUE;
    } else if(s!=EFI_NOT_READY)return FALSE;
    if(Gu.bs->Stall(100000)!=EFI_SUCCESS)return FALSE;
  }
}
