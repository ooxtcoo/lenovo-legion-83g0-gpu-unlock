/* Actual whole application + actual transaction + authentic assets.
 * ALL firmware services and register transitions are synthetic; never UEFI. */
#include "test_ad103_app.c"
#include <Guid/ConsoleOutDevice.h>
#include "../embedded.h"
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
__attribute__((noinline)) static void patch_memory(void *p,const void *data,size_t n){unsigned long old,ignored;CHECK(VirtualProtect(p,n,4,&old));for(size_t i=0;i<n;i++)((volatile UINT8*)p)[i]=((const UINT8*)data)[i];CHECK(VirtualProtect(p,n,old,&ignored));}
static GU_EMBEDDED_ASSET saved_embedded[5];
static unsigned asset_allocs,asset_frees,asset_fs_calls,windows_fs_calls;
static VOID *asset_pools[5];
static int corrupted=-1;
static UINT8 saved_byte;
static EFI_GUID cg=EFI_CONSOLE_OUT_DEVICE_GUID,tg=EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID,kg=EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID;
static EFI_DRIVER_BINDING_PROTOCOL aliases[2];
static EFI_LOADED_IMAGE_PROTOCOL alias_images[2];
static EFI_SIMPLE_TEXT_OUTPUT_MODE textmode;
static int auto_independent(unsigned c){switch(c){case 473:case 1:case 2:case 54:case 55:case 56:case 117:case 118:case 142:case 143:case 144:case 179:case 376:case 377:case 378:case 379:case 380:case 381:case 382:case 383:case 384:case 387:case 388:case 389:case 390:case 391:case 396:case 397:case 398:case 399:case 400:case 401:case 402:case 403:case 405:case 424:case 425:case 426:case 427:case 428:case 429:case 430:case 458:case 459:case 460:case 461:return 1;default:return 0;}}
static unsigned exceptional_phase,exceptional_reads,exceptional_keys,ss0_reads;
static unsigned vcase, input_phase, input_reads, writes, stop_calls, replacement_writes;
static unsigned report_seen,report_reads,mem_calls,cold_passes,report_keys;
static unsigned final_phase,final_reads,photo_phase,photo_reads,photo_quiet,s_accepted;
static unsigned output_calls,last_config_outputs,last_config_keys,photo_stalls;
static unsigned exit_phase,exit_reads,exit_stalls,exit_keys,refusal_count,cancel_keys;
/* 376=no key; final fixture interrupts only after >60 seconds of polling. */
static char screen[25][81];static unsigned sx,sy;
static void paint(const char*t){for(;*t;t++){if(*t=='\r')sx=0;else if(*t=='\n'){sy++;}else {if(sx==80){sx=0;sy++;}if(sy==25){memmove(screen,screen+1,24*81);memset(screen[24],0,81);sy=24;}screen[sy][sx++]=*t;}if(sy==25){memmove(screen,screen+1,24*81);memset(screen[24],0,81);sy=24;}}}
static void check_page(const char*tag){const char*t=strstr(fake.log,tag);CHECK(t);unsigned lines=0,col=0;for(const char*s=t;*s;s++){if(*s=='\n'){lines++;col=0;}else if(*s!='\r'){col++;CHECK(col<80);}}CHECK(lines<=24);int seen=0;for(unsigned i=0;i<25;i++)if(strstr(screen[i],tag))seen=1;CHECK(seen);CHECK(report_reads==mem_calls);CHECK(report_keys<=key_calls);}

static const UINT32 cold_offsets[]={0,0x118128,0x118234,0x110100,0x1fa828,0x823804,0x82381c,0x823820,0x1fa7cc,0x9a0148,0x1fa7c4,0x823b00,0x8403c4,0x1183a4};
static EFI_STATUS EFIAPI counted_replacement_write(EFI_PCI_IO_PROTOCOL*p,EFI_PCI_IO_PROTOCOL_WIDTH w,UINT32 o,UINTN n,VOID*d){replacement_writes++;return replacement_write(p,w,o,n,d);}
static EFI_HANDLE phy(void){return connect_calls?H(42):H(5);}
static EFI_STATUS EFIAPI voutput(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*p,CHAR16*s){
 char t[2048];narrow(t,sizeof(t),s);CHECK(p==&conout);
 output_calls++;
 if(strstr(t,"SS0 EXCEPTION:")){exceptional_phase=1;exceptional_reads=0;if(vcase==498)return EFI_WARN_UNKNOWN_GLYPH;if(vcase==499)return EFI_DEVICE_ERROR;}
 if(strstr(t,"Press fresh C")){exceptional_phase=2;exceptional_reads=0;if(vcase==500)return EFI_WARN_UNKNOWN_GLYPH;if(vcase==501)return EFI_DEVICE_ERROR;}
 if(strstr(t,"AUTO authorized:")){if(vcase==525)registers[0x82381c/4]^=1;if(vcase==526)registers[0x823820/4]=7;}
 if(strstr(t,"AUTO: launch authorizes")){if(vcase==3)return EFI_WARN_UNKNOWN_GLYPH;if(vcase==4)key_sent=TRUE;}
 if(strstr(t,"AUTO: final fresh")){final_phase=2;if(vcase==385)config[0xc8]^=1;if(vcase==386||vcase==404||vcase==464)infos[1].HorizontalResolution++;}
 if(strstr(t,"Attempt cancelled or rejected")){exit_phase=1;exit_reads=0;}
 if(strstr(t,"REFUSED - NO DETACH")){exit_phase=1;exit_reads=0;refusal_count++;}
 if(exit_phase&&(vcase==422||vcase==439))return vcase==422?EFI_DEVICE_ERROR:EFI_WARN_UNKNOWN_GLYPH;
 if(strstr(t,"COLD REFUSAL")||strstr(t,"PATH REFUSAL")){report_seen=1;report_reads=mem_calls;report_keys=(unsigned)key_calls;}
 if(strstr(t,"fresh U")||strstr(t,"Press U once")){input_phase=1;input_reads=0;}
 if(vcase==173&&strstr(t,"COLD REFUSAL"))return EFI_WARN_UNKNOWN_GLYPH;
 if(vcase==174&&report_seen)return EFI_WARN_UNKNOWN_GLYPH;
 if(vcase==175&&strstr(t,"FEAT_PLM raw="))return EFI_DEVICE_ERROR;
 if(vcase==178&&strstr(t,"IO first="))for(unsigned i=0;i<14;i++)registers[cold_offsets[i]/4]=0xdeadbeef;
 if(vcase==180&&strstr(t,"cold state:"))return EFI_WARN_UNKNOWN_GLYPH;
 if(vcase==3 && input_phase)return EFI_WARN_UNKNOWN_GLYPH;
 if(vcase==116 && strstr(t,"asset verified:"))return EFI_WARN_UNKNOWN_GLYPH;
 if(strstr(t,"PCI EXCEPTION:")){
  if(vcase==462)return EFI_WARN_UNKNOWN_GLYPH;
  if(vcase==463)return EFI_DEVICE_ERROR;
  if(vcase==477)Gu.fault=TRUE;
 }
 if(vcase==478&&strstr(t,"Checking final PCI"))Gu.quiesced=FALSE;
 if(strstr(t,"RESULT PHOTO")){photo_phase=1;photo_reads=0;if(vcase==379)return EFI_WARN_UNKNOWN_GLYPH;}
 if(strstr(t,"Press fresh S")){photo_phase=2;photo_reads=0;if(vcase==380)return EFI_DEVICE_ERROR;if(vcase==404)infos[1].HorizontalResolution++;}
 if(vcase==37 && (strstr(t,"Starting Windows")||strstr(t,"Checking final PCI")))return EFI_WARN_UNKNOWN_GLYPH;
 if((vcase==82||vcase==89)&&strstr(t,"SCREEN logging"))return EFI_WARN_UNKNOWN_GLYPH;
 if(vcase==58&&(strstr(t,"Starting Windows")||strstr(t,"Checking final PCI")))config[0xc8]^=1;
 if(strstr(t,"Starting Windows")||strstr(t,"Checking final PCI")) {
  final_phase=1;

 }
 if(vcase==374&&strstr(t,"FINAL PCI:"))return EFI_WARN_UNKNOWN_GLYPH;
 paint(t);EFI_STATUS out_status=app_output(p,s);halted=Gu.retained;return out_status;
}
static EFI_STATUS EFIAPI vinput(EFI_SIMPLE_TEXT_INPUT_PROTOCOL*p,EFI_INPUT_KEY*k){
 CHECK(p==&conin);key_calls++;
 if(vcase>=479&&vcase<=483){CHECK(0);return EFI_DEVICE_ERROR;}
 if(vcase>=484&&!exit_phase){
  CHECK(exceptional_phase);exceptional_reads++;k->ScanCode=0;k->UnicodeChar='C';
  if(exceptional_phase==1){
   if(vcase==490||vcase==545&&exceptional_reads%4==0||vcase==546&&exceptional_reads==1)return EFI_SUCCESS;
   if(vcase==492)return EFI_DEVICE_ERROR;if(vcase==493)return EFI_WARN_UNKNOWN_GLYPH;
   return EFI_NOT_READY;
  }
  if(vcase==491)return EFI_SUCCESS;
  if(exceptional_reads==1){if(vcase==494)return EFI_DEVICE_ERROR;if(vcase==495)return EFI_WARN_UNKNOWN_GLYPH;return EFI_SUCCESS;} /* stale C */
  if(exceptional_reads==2)return EFI_NOT_READY;
  if(vcase==485||vcase==489||vcase==546)return EFI_NOT_READY;
  if(vcase==496)return EFI_DEVICE_ERROR;if(vcase==497)return EFI_WARN_UNKNOWN_GLYPH;
  if(vcase==486)k->UnicodeChar='Q';if(vcase==487){k->UnicodeChar=0;k->ScanCode=23;}if(vcase==488)k->UnicodeChar=' ';
  if(vcase==502)registers[0x82381c/4]^=1;
  if(vcase==503)registers[0x82381c/4]=0x53422157;
  if(vcase==504)registers[0x823820/4]=7;
  if(vcase==505)config[0xc8]^=1;
  if(vcase==506)infos[1].HorizontalResolution++;
  if(vcase==507)Gu.fault=TRUE;
  if(vcase==530)k->UnicodeChar='c';
  if(vcase==531)k->ScanCode=1;
  if(vcase==532)k->UnicodeChar='U';if(vcase==533)k->UnicodeChar='S';if(vcase==534)k->UnicodeChar='\r';
  if(!k->ScanCode&&(k->UnicodeChar=='C'||k->UnicodeChar=='c'))exceptional_keys++;
  return EFI_SUCCESS;
 }
 if(exit_phase){
  exit_reads++;k->ScanCode=0;k->UnicodeChar=' ';
  if(vcase==417||vcase==429)return EFI_DEVICE_ERROR;
  if(vcase==418)return EFI_WARN_UNKNOWN_GLYPH;
  if(vcase==415||vcase==428)return EFI_SUCCESS;
  if(vcase==416)return exit_reads%4?EFI_NOT_READY:EFI_SUCCESS;
  /* Stale exit key is followed by five empty samples; it must never exit. */
  if(vcase==406)return EFI_NOT_READY;
  if(exit_reads<=3){if(vcase==436){k->UnicodeChar=exit_reads==1?' ':exit_reads==2?'Q':0;k->ScanCode=exit_reads==3?23:0;}return EFI_SUCCESS;}
  if(vcase==414)return EFI_NOT_READY;
  if(exit_reads<=8)return EFI_NOT_READY;
  if(vcase==440&&exit_reads<1009)return EFI_NOT_READY;
  if(exit_reads==(vcase==440?1009:9)){
   if(vcase==408)k->UnicodeChar='Q';if(vcase==409)k->UnicodeChar='q';
   if(vcase==410){k->ScanCode=23;k->UnicodeChar=0;}
   if(vcase==411)k->UnicodeChar='\r';if(vcase==412)k->UnicodeChar='U';if(vcase==413)k->UnicodeChar='S';
   if(vcase==438){k->ScanCode=23;k->UnicodeChar='U';}
   if(vcase!=411&&vcase!=412&&vcase!=413&&vcase!=438)exit_keys++;
   return EFI_SUCCESS;
  }
  if((vcase==411||vcase==412||vcase==413||vcase==438)&&exit_reads==10){exit_keys++;return EFI_SUCCESS;}
  if(vcase==437&&exit_reads<=19)return EFI_SUCCESS;
  return EFI_NOT_READY;
 }
 if(photo_phase){
  k->ScanCode=0;k->UnicodeChar='S';photo_reads++;
  if(photo_phase==1){photo_quiet++;if(vcase==378)return EFI_SUCCESS;if(vcase==389)return EFI_DEVICE_ERROR;if(vcase==390)return EFI_WARN_UNKNOWN_GLYPH;return photo_reads==1?EFI_SUCCESS:EFI_NOT_READY;}
  /* The first queued S after the visible prompt MUST be drained. */
  if(vcase==398)return EFI_SUCCESS;
  if(photo_reads==1)return vcase==376?EFI_NOT_READY:EFI_SUCCESS;
  if(photo_reads==2)return EFI_NOT_READY;
  if(vcase==376||vcase==377||vcase==387||vcase==388){if(photo_reads>1000)longjmp(escape_halt,4);if(vcase==387){k->UnicodeChar='U';return EFI_SUCCESS;}if(vcase==388){k->ScanCode=1;return EFI_SUCCESS;}return EFI_NOT_READY;}
  if(vcase==458||vcase==459||vcase==460){
   if(photo_reads>1000)longjmp(escape_halt,4);
   if(vcase==458)return EFI_NOT_READY;
   if(vcase==459)k->UnicodeChar='U';else k->ScanCode=1;
   return EFI_SUCCESS;
  }
  if(vcase==464)infos[1].HorizontalResolution++;
  if(vcase==461)k->UnicodeChar='Q';
  if(vcase==381)k->UnicodeChar='Q';
  if(vcase==382){k->ScanCode=23;k->UnicodeChar=0;}
  if(vcase==383)return EFI_DEVICE_ERROR;
  if(vcase==384)return EFI_WARN_UNKNOWN_GLYPH;
  if(vcase==385)config[0xc8]^=1;
  if(vcase==386)infos[1].HorizontalResolution++;
  if(vcase==396)k->UnicodeChar='s';
  if(vcase==397)k->UnicodeChar='x';
  if(vcase==397&&photo_reads>1000)longjmp(escape_halt,4);
  if(!k->ScanCode&&(k->UnicodeChar=='S'||k->UnicodeChar=='s')){s_accepted++;final_phase=2;}
  return EFI_SUCCESS;
 }
 if(vcase==54||vcase==55||vcase==118){k->ScanCode=0;k->UnicodeChar='U';return vcase==54&&key_calls%2?EFI_NOT_READY:EFI_SUCCESS;}
 if(!input_phase){if(vcase==430&&key_calls==1){k->ScanCode=0;k->UnicodeChar='U';return EFI_SUCCESS;}return EFI_NOT_READY;}
 k->ScanCode=0;k->UnicodeChar='U';
 if(input_reads++==0)return EFI_NOT_READY;
 if(vcase>=424&&vcase<=430){
  k->UnicodeChar=vcase==424?' ':vcase==427?'q':'Q';
  if(vcase==426){k->ScanCode=23;k->UnicodeChar=0;}
  cancel_keys++;
 }
 if(vcase==1||vcase==117||vcase>=142&&vcase<=144||vcase==179)k->UnicodeChar='Q';
 if(vcase==56)k->ScanCode=1;
 if(vcase==2)return EFI_NOT_READY;
 key_sent=TRUE;key_reset=TRUE;return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI vquery(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*p,UINTN m,UINTN*c,UINTN*r){(void)p;(void)m;*c=80;*r=25;return EFI_SUCCESS;}
static EFI_STATUS EFIAPI vsupported(EFI_DRIVER_BINDING_PROTOCOL*p,EFI_HANDLE h,EFI_DEVICE_PATH_PROTOCOL*d){(void)p;(void)h;(void)d;CHECK(0);return EFI_UNSUPPORTED;}
static EFI_STATUS EFIAPI vconfig(EFI_PCI_IO_PROTOCOL*p,EFI_PCI_IO_PROTOCOL_WIDTH w,UINT32 o,UINTN n,VOID*v){
 if(final_phase==2&&n==1&&w==EfiPciIoWidthUint32){
  if(vcase==392&&o==0)return EFI_TIMEOUT;
  if(vcase==393&&o==0x80)return EFI_WARN_UNKNOWN_GLYPH;
 }
 if(final_phase&&n==1){final_reads++;if(vcase>=245&&vcase<=372&&o==((vcase-245)%64)*4)return vcase<=308?EFI_TIMEOUT:EFI_WARN_UNKNOWN_GLYPH;}
 for(UINTN i=0;i<n;i++){EFI_STATUS s=app_read_config(p,w,o+(UINT32)(i*((UINTN)1<<w)),1,(UINT8*)v+i*((UINTN)1<<w));if(s!=EFI_SUCCESS)return s;}
 if(final_phase&&n==1&&w==EfiPciIoWidthUint32&&(vcase>=181&&vcase<=244&&o==(vcase-181)*4||vcase==373||vcase==374&&o==0xc8))*(UINT32*)v^=1;
 if(vcase>=441&&vcase<=478&&final_phase&&n==1&&w==EfiPciIoWidthUint32){
  if(o==0x80){
   if(vcase!=446)*(UINT32*)v&=~0x00090000U;
   if(vcase==447||vcase==449)*(UINT32*)v|=0x10000;
   if(vcase==448||vcase==450)*(UINT32*)v|=0x80000;
   if(vcase>=451&&vcase<=454)*(UINT32*)v|=(UINT32[]){0x20000,0x40000,0x200000,0x400000}[vcase-451];
   if(vcase==455)*(UINT32*)v^=1;
   if(vcase==457&&final_phase==2)*(UINT32*)v|=0x90000;
  }
  if(vcase==456&&o==0xfc)*(UINT32*)v^=1;
  if(vcase==465&&final_phase==2&&o==0x80)return EFI_TIMEOUT;
  if(vcase==466&&final_phase==2&&o==0xfc)return EFI_WARN_UNKNOWN_GLYPH;
  if(vcase==467&&o==0x78)*(UINT32*)v|=0x7800;
  if(vcase==468&&o==0xb4)*(UINT32*)v=0x00020010;
  if(vcase==469&&o==0x78)*(UINT32*)v=0x00028010;
  if(vcase==470&&o==0x68)*(UINT32*)v=0x00807c05;
  if(vcase==471&&o==0x78)*(UINT32*)v=0x0011b410;
  if(vcase==472&&o==0x78)*(UINT32*)v=0x003cb409;
  if(vcase==476&&o==4)*(UINT32*)v^=0x80000000U;
 }
 if(final_phase==2&&n==1&&w==EfiPciIoWidthUint32){
  if(vcase==394&&o==0xfc||vcase==395)*(UINT32*)v^=1;
  if(o==0xfc){last_config_outputs=output_calls;last_config_keys=(unsigned)key_calls;}
 }
 return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI vlocate(EFI_LOCATE_SEARCH_TYPE search,EFI_GUID*g,VOID*k,UINTN*n,EFI_HANDLE**h){
 if(guid(g,&fg)){windows_fs_calls++;for(unsigned i=0;i<5;i++)CHECK(GuAssets[i].firmware.data);if(vcase==115)return EFI_NOT_FOUND;return locate(search,g,k,n,h);}
 CHECK(search==ByProtocol&&!k);*n=guid(g,&pg)?1:guid(g,&gg)?(bound?2:1):1;
 CHECK(guid(g,&pg)||guid(g,&gg)||guid(g,&cg));CHECK(allocate_pool(EfiLoaderData,*n*sizeof(**h),(VOID**)h)==EFI_SUCCESS);
 (*h)[0]=guid(g,&pg)?H(3):guid(g,&gg)?(bound?phy():H(6)):phy();if(*n==2)(*h)[1]=H(6);return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI vhandle(EFI_HANDLE h,EFI_GUID*g,VOID**p){
 *p=NULL;
 if(guid(g,&kg)&&vcase==517)return EFI_DEVICE_ERROR;
 if(guid(g,&kg)&&vcase==518)return EFI_WARN_UNKNOWN_GLYPH;
 if(photo_phase&&guid(g,&kg)){if(vcase==401)return EFI_SUCCESS;if(vcase==403)conin.ReadKeyStroke=NULL;if(vcase==405)return EFI_TIMEOUT;}
 if(h==H(1)&&guid(g,&lg)){if(vcase==64)return EFI_TIMEOUT;if(vcase==65)return EFI_SUCCESS;}
 if(h==H(2)&&guid(g,&dg)&&vcase>=123&&vcase<=129){
  static UINT8 pathbytes[4096];memset(pathbytes,0,sizeof(pathbytes));*p=pathbytes;
  if(vcase==123){for(unsigned i=0;i<64;i++){pathbytes[i*4]=1;pathbytes[i*4+1]=1;pathbytes[i*4+2]=4;}}
  if(vcase==124){pathbytes[0]=3;pathbytes[1]=5;}
  if(vcase==125){pathbytes[0]=1;pathbytes[1]=1;pathbytes[2]=1;pathbytes[3]=16;}
  if(vcase==126){UINT8 b[]={3,5,6,0,0,0,0x7f,1,4,0};memcpy(pathbytes,b,sizeof(b));}
  if(vcase==127){pathbytes[0]=1;pathbytes[1]=1;pathbytes[2]=0xfc;pathbytes[3]=15;pathbytes[4092]=0x7f;pathbytes[4093]=0xff;pathbytes[4094]=8;}
  if(vcase==128){UINT8 b[]={3,5,6,0,0,0,0x7f,0xff,4,0};memcpy(pathbytes,b,sizeof(b));}
  if(vcase==129){UINT8 b[]={3,0x1a,5,0,0,0x7f,0xff,4,0};memcpy(pathbytes,b,sizeof(b));}
  return EFI_SUCCESS;
 }
 if(h==H(2)&&guid(g,&dg)&&vcase==122){static UINT8 shortpath[46]={4,4,42,0};shortpath[42]=0x7f;shortpath[43]=0xff;shortpath[44]=4;*p=shortpath;return EFI_SUCCESS;}
 if(h==H(2)&&guid(g,&dg)){if(vcase==66||vcase==88)return EFI_TIMEOUT;if(vcase==67)return EFI_SUCCESS;if(vcase==84){*p=(VOID*)rtx_path;return EFI_SUCCESS;}if(vcase==85){static UINT8 bad[]={3,5,0,0};*p=bad;return EFI_SUCCESS;}}
 if(h==H(2)&&guid(g,&fg)){asset_fs_calls++;CHECK(0);if(vcase==90)return EFI_UNSUPPORTED;if(vcase==88)return EFI_ACCESS_DENIED;if(vcase==69)return EFI_TIMEOUT;if(vcase==70)return EFI_SUCCESS;}
 if(guid(g,&tg)&&h==H(6)){if(vcase==30&&connect_calls)return EFI_NOT_FOUND;*p=&conout;}
 else if(guid(g,&kg)&&h==H(7))*p=&conin;
 else if(guid(g,&gg)&&h==phy()&&bound)*p=&gops[0];
 else if(guid(g,&gg)&&h==H(6))*p=&gops[1];
 else if(guid(g,&dg)&&(h==H(3)||h==phy()))*p=(VOID*)rtx_path;
 else if(guid(g,&dg)&&h==H(6))return EFI_UNSUPPORTED;
 else if(guid(g,&bg)&&h==H(20))*p=&aliases[0];
 else if(guid(g,&bg)&&h==H(22))*p=&aliases[1];
 else if(guid(g,&lg)&&h==H(21))*p=&alias_images[0];
 else if(guid(g,&lg)&&h==H(23))*p=&alias_images[1];
 else return handle(h,g,p);
 return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI vprotocols(EFI_HANDLE h,EFI_GUID***p,UINTN*n){
 *n=h==H(3)?2:h==phy()?3:2;
 CHECK(allocate_pool(EfiLoaderData,*n*sizeof(**p),(VOID**)p)==EFI_SUCCESS);
 if(h==H(3)){(*p)[0]=&pg;(*p)[1]=&dg;}
 else if(h==phy()){(*p)[0]=&gg;(*p)[1]=vcase==60?&dg:&cg;(*p)[2]=&tg;}
 else {CHECK(h==H(6));(*p)[0]=&gg;(*p)[1]=&tg;}return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI vinfo(EFI_HANDLE h,EFI_GUID*g,EFI_OPEN_PROTOCOL_INFORMATION_ENTRY**e,UINTN*n){
 *e=NULL;*n=0;
 if(h==H(3)&&exclusive&&guid(g,&pg)){*n=1;}
 else if(bound){if(h==H(3)&&(guid(g,&pg)||guid(g,&dg)))*n=1;
 if(h==phy()&&guid(g,&cg))*n=2;if(h==phy()&&guid(g,&tg))*n=1;}
 if(!*n)return EFI_SUCCESS;
 CHECK(allocate_pool(EfiLoaderData,*n*sizeof(**e),(VOID**)e)==EFI_SUCCESS);memset(*e,0,*n*sizeof(**e));
 (*e)[0].Attributes=EFI_OPEN_PROTOCOL_BY_DRIVER;(*e)[0].OpenCount=1;(*e)[0].ControllerHandle=h;
 (*e)[0].AgentHandle=h==H(3)?H(8):guid(g,&cg)?H(20):H(22);
 if(h==H(3)&&exclusive){(*e)[0].Attributes=EFI_OPEN_PROTOCOL_EXCLUSIVE;(*e)[0].AgentHandle=H(1);(*e)[0].ControllerHandle=NULL;}
 else if(h==H(3)&&guid(g,&dg)){(*e)[0].Attributes=EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER;(*e)[0].ControllerHandle=phy();}
 if(*n==2){(*e)[1]=(*e)[0];(*e)[1].Attributes=EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER;(*e)[1].ControllerHandle=H(6);}
 if(vcase==4&&key_sent&&h==H(3))(*e)[0].OpenCount=2;
 if(vcase==31&&connect_calls&&h==phy()&&guid(g,&tg))(*e)[0].AgentHandle=H(20);
 if(vcase==59&&h==phy()&&guid(g,&cg)&&*n==2)(*e)[1].AgentHandle=H(22);
 if(vcase==61&&h==H(3)&&guid(g,&dg))(*e)[0].OpenCount=2;
 if(vcase==62&&h==H(3)&&guid(g,&dg))(*e)[0].AgentHandle=H(20);
 return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI vdisconnect(EFI_HANDLE h,EFI_HANDLE d,EFI_HANDLE c){
 CHECK(windows_loaded&&GuScreenDetached&&h==H(3)&&d==H(8)&&!c&&!exclusive&&!disconnect_calls);
 CHECK(!fake.maps&&!fake.mmio_writes&&GuAssets[4].firmware.data);
 for(unsigned i=0;i<5;i++){CHECK(GuAssets[i].firmware.data!=GuEmbeddedAssets[i].begin);CHECK(GuAssets[i].firmware.data==asset_pools[i]);CHECK(GuAssets[i].firmware.size==originals[i].firmware.size);CHECK(!memcmp(GuAssets[i].firmware.data,originals[i].firmware.data,originals[i].firmware.size));}
 disconnect_calls++;
 if(vcase==5)return EFI_DEVICE_ERROR;
 if(vcase==6)return EFI_WARN_UNKNOWN_GLYPH;
 if(vcase!=7)bound=FALSE;
 modes[0].FrameBufferBase=modes[0].FrameBufferSize=0;
 if(vcase==8||vcase==34){replacement_pci=pci;replacement_pci.Pci.Read=replacement_read;replacement_pci.Pci.Write=counted_replacement_write;bs.HandleProtocol=handle;app_mode=APP_HANDOFF_REPLACED_PCI;if(vcase==34)config[4]|=4;}
 return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI vconnect(EFI_HANDLE h,EFI_HANDLE*d,EFI_DEVICE_PATH_PROTOCOL*p,BOOLEAN r){
 CHECK(h==H(3)&&d[0]==H(8)&&!d[1]&&!p&&r&&!exclusive&&Gu.quiesced&&!Gu.allocations&&!Gu.dma_count&&fake.flrs==1);
 connect_calls++;if(vcase==29)return EFI_DEVICE_ERROR;
 bound=TRUE;modes[0].FrameBufferBase=modes[1].FrameBufferBase;modes[0].FrameBufferSize=modes[1].FrameBufferSize;
 return EFI_SUCCESS;
}
static EFI_STATUS EFIAPI vstart(EFI_HANDLE h,UINTN*b,CHAR16**d){
 CHECK(!s_accepted&&!photo_phase&&(vcase>=484||!key_calls)&&final_reads==128);
 CHECK(disconnect_calls==1&&fake.gsp_starts==1);
 CHECK(output_calls==last_config_outputs&&key_calls==last_config_keys);
 CHECK(h==H(14)&&b&&d&&windows_loaded&&!exclusive&&!GuScreenDetached&&bound&&connect_calls==1);
 CHECK(Gu.quiesced&&fake.flrs==1&&!Gu.allocations&&!fake.live_maps&&!fake.live_buffers&&!(config[4]&4));
 windows_started++;if(vcase==32)return EFI_SUCCESS;if(vcase==33)return EFI_LOAD_ERROR;longjmp(escape_halt,2);
}
static EFI_STATUS EFIAPI vwrite(EFI_FILE_PROTOCOL*f,UINTN*n,VOID*d){writes++;return file_write(f,n,d);}
static EFI_STATUS EFIAPI vopen(EFI_FILE_PROTOCOL*f,EFI_FILE_PROTOCOL**out,CHAR16*name,UINT64 mode,UINT64 attrs){
 CHECK(((FILE_FAKE*)f)->kind==9 && mode==EFI_FILE_MODE_READ && attrs==0);
 if(((FILE_FAKE*)f)->kind==0){CHECK(mode==EFI_FILE_MODE_READ);
  if(vcase==75)return EFI_TIMEOUT;if(vcase==76){*out=NULL;return EFI_SUCCESS;}}
 if(vcase==35&&(mode&(EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE))){writes++;return EFI_WRITE_PROTECTED;}
 if(vcase==43&&((FILE_FAKE*)f)->kind==1&&name[0]=='A')return EFI_NOT_FOUND;
 return file_open(f,out,name,mode,attrs);
}
static EFI_STATUS EFIAPI vread(EFI_FILE_PROTOCOL*f,UINTN*n,VOID*d){
 EFI_STATUS s=file_read(f,n,d);if(((FILE_FAKE*)f)->kind==4){if(vcase==40)return EFI_WARN_UNKNOWN_GLYPH;if(vcase==44&&*n)(*n)--;}return s;
}
static EFI_STATUS EFIAPI vclose(EFI_FILE_PROTOCOL*f){EFI_STATUS s=file_close(f);if(vcase==81&&((FILE_FAKE*)f)->kind<9)return EFI_WRITE_PROTECTED;if(((FILE_FAKE*)f)->kind==4&&vcase==42)return EFI_DEVICE_ERROR;if(vcase==52&&connect_calls&&((FILE_FAKE*)f)->kind==1)return EFI_DEVICE_ERROR;return s;}
static EFI_STATUS EFIAPI vmetadata(EFI_FILE_PROTOCOL*f,EFI_GUID*g,UINTN*n,VOID*d){
 if(((FILE_FAKE*)f)->kind==1&&(vcase==78||vcase==81||vcase==89))return EFI_TIMEOUT;
 EFI_STATUS s=file_info(f,g,n,d);
 if(((FILE_FAKE*)f)->kind==1){if(vcase==79)((EFI_FILE_INFO*)d)->Attribute=0;if(vcase==80)return EFI_WARN_UNKNOWN_GLYPH;}
return vcase==41&&((FILE_FAKE*)f)->kind==4?EFI_WARN_UNKNOWN_GLYPH:s;}
static EFI_STATUS EFIAPI vmap(EFI_PCI_IO_PROTOCOL*p,EFI_PCI_IO_PROTOCOL_OPERATION o,VOID*c,UINTN*n,EFI_PHYSICAL_ADDRESS*d,VOID**m){EFI_STATUS s=map_buffer(p,o,c,n,d,m);return vcase==45&&s==EFI_SUCCESS?EFI_WARN_UNKNOWN_GLYPH:s;}
static EFI_STATUS EFIAPI vmread(EFI_PCI_IO_PROTOCOL*p,EFI_PCI_IO_PROTOCOL_WIDTH w,UINT8 bar,UINT64 o,UINTN n,VOID*d){
 mem_calls++;CHECK(!report_seen);if(o==0)cold_passes++;if(o==0x82381c)ss0_reads++;
 if(vcase>=547&&vcase<=574&&o==cold_offsets[(vcase-547)%14])return vcase<561?EFI_DEVICE_ERROR:EFI_WARN_UNKNOWN_GLYPH;
 if(o==0x82381c && (vcase==512||vcase==513))return vcase==512?EFI_DEVICE_ERROR:EFI_WARN_UNKNOWN_GLYPH;
 if(o==0x82381c && cold_passes==2 && (vcase==527||vcase==528))return vcase==527?EFI_DEVICE_ERROR:EFI_WARN_UNKNOWN_GLYPH;
 if(o==0x82381c && ss0_reads==3 && (vcase==541||vcase==542))return vcase==541?EFI_DEVICE_ERROR:EFI_WARN_UNKNOWN_GLYPH;
 if(o==0x82381c && cold_passes==2 && vcase==540)registers[o/4]=0x53412157;
 if(o==0x82381c && disconnect_calls && vcase==529)registers[o/4]^=1;
 if(vcase>=121&&vcase<=141||vcase>=145&&vcase<=175||vcase==178||vcase==180)CHECK(o==cold_offsets[mem_calls-1]);
 if(vcase>=145&&vcase<=172&&o==cold_offsets[(vcase-145)%14])return vcase<159?EFI_TIMEOUT:EFI_WARN_UNKNOWN_GLYPH;
 EFI_STATUS s=app_read_memory(p,w,bar,o,n,d);
 if(vcase==402&&connect_calls&&o==0x823820)*(UINT32*)d=7;
 if((vcase==176&&cold_passes==2||vcase==177&&disconnect_calls)&&o==0x823820)*(UINT32*)d=7;
 return vcase==46?EFI_WARN_UNKNOWN_GLYPH:s;
}
static EFI_STATUS EFIAPI vunmap(EFI_PCI_IO_PROTOCOL*p,VOID*m){if(vcase==47)return EFI_WARN_UNKNOWN_GLYPH;return unmap_buffer(p,m);}
static EFI_STATUS EFIAPI valloc(EFI_PCI_IO_PROTOCOL*p,EFI_ALLOCATE_TYPE a,EFI_MEMORY_TYPE t,UINTN n,VOID**h,UINT64 at){EFI_STATUS s=allocate_buffer(p,a,t,n,h,at);return vcase==48?EFI_WARN_UNKNOWN_GLYPH:s;}
static EFI_STATUS EFIAPI vfree(EFI_PCI_IO_PROTOCOL*p,UINTN n,VOID*h){if(vcase==49)return EFI_WARN_UNKNOWN_GLYPH;return free_buffer(p,n,h);}
static EFI_STATUS EFIAPI vpoolalloc(EFI_MEMORY_TYPE t,UINTN n,VOID**p){
 for(unsigned i=0;i<5;i++)if(n==originals[i].firmware.size && !windows_loaded && !Gu.started){
  asset_allocs++;
  if(vcase==105+i || vcase==110+i){*p=NULL;return vcase==105+i?EFI_OUT_OF_RESOURCES:EFI_WARN_UNKNOWN_GLYPH;}
  EFI_STATUS s=allocate_pool(t,n,p);if(s==EFI_SUCCESS)asset_pools[i]=*p;return s;
 }
 return allocate_pool(t,n,p);
}
static EFI_STATUS EFIAPI vpoolfree(VOID*p){
 if((vcase==50||vcase==53)&&connect_calls&&p==GuAssets[0].firmware.data)return vcase==50?EFI_DEVICE_ERROR:EFI_WARN_UNKNOWN_GLYPH;
 EFI_STATUS s=app_free_pool(p);if(s==EFI_SUCCESS)for(unsigned i=0;i<5;i++)if(p==asset_pools[i] && p){asset_frees++;asset_pools[i]=NULL;}
 return s;
}
static EFI_STATUS EFIAPI vcloseprotocol(EFI_HANDLE h,EFI_GUID*g,EFI_HANDLE a,EFI_HANDLE c){if(vcase==51)return EFI_DEVICE_ERROR;return close_protocol(h,g,a,c);}
static EFI_STATUS EFIAPI vvolume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*f,EFI_FILE_PROTOCOL**r){
 if(vcase==72)return EFI_TIMEOUT;if(vcase==73){*r=NULL;return EFI_SUCCESS;}return volume(f,r);
}
static EFI_STATUS EFIAPI vstall(UINTN n){
 if(exceptional_phase&&!exit_phase){if(vcase==519)return EFI_DEVICE_ERROR;if(vcase==520)return EFI_WARN_UNKNOWN_GLYPH;}
 if(vcase==479&&++stop_calls>1200)longjmp(escape_halt,5);
 if(Gu.retained)longjmp(escape_halt,1);
 if(exit_phase){exit_stalls++;if(exit_stalls>1200)longjmp(escape_halt,5);if(vcase==419)return EFI_DEVICE_ERROR;if(vcase==420)return EFI_WARN_UNKNOWN_GLYPH;}
 if(photo_phase){photo_stalls++;if(vcase==399)return EFI_DEVICE_ERROR;if(vcase==400)return EFI_WARN_UNKNOWN_GLYPH;}
 return stall(n);
}
static void vsetup(void){
 app_setup(APP_SUCCESS);
 exceptional_phase=exceptional_reads=exceptional_keys=ss0_reads=0;
 if(vcase>=484)registers[0x82381c/4]=0x53412157;
 if(vcase==508)registers[0x82381c/4]=0;
 if(vcase==509)registers[0x82381c/4]=0xffffffffU;
 if(vcase==510||vcase==511)registers[0x823820/4]=7;
 if(vcase==511||vcase==535||vcase==536)registers[0x82381c/4]=0x53402157;
 if(vcase==536)registers[0x823820/4]=0;
 if(vcase==521)registers[0x823804/4]=0xffffff0f;
 if(vcase==522)registers[0x1fa828/4]=16;
 if(vcase==523)registers[0x1183a4/4]=0;
 if(vcase==524)registers[0x118128/4]=0xffffffffU;
 if(vcase==540)registers[0x82381c/4]=0x53422157;
 if(vcase==543)registers[0x82381c/4]=1;
 if(vcase==544)registers[0x82381c/4]=0xfffffffeU;
 /* Old successful cases use healthy equal status; dedicated 441+ cases retain
    the photographed errors and deliberately exercise clearing/refusal. */
 if(vcase>=479 || vcase<441 || vcase==445 || vcase==449 || vcase==450)config[0x82]=0;
 if(vcase==442)config[0x81]=0x28;
 if(vcase==443)config[0x82]=1;
 if(vcase==444)config[0x82]=8;
 if(vcase==473||vcase==483)registers[0x82381c/4]=0x53402157;
 if(vcase==474)registers[0x82381c/4]=registers[0x823820/4]=0;
 if(vcase==475)*(UINT32*)(config+0x78)=0x003cb409;
 app_mode=vcase==9?APP_BAD_HASH:vcase==10?APP_BAD_SIZE:vcase==11?APP_BAD_PROM:APP_SUCCESS;
 input_phase=input_reads=writes=stop_calls=replacement_writes=0;final_phase=final_reads=photo_phase=photo_reads=photo_quiet=s_accepted=0;
 output_calls=last_config_outputs=last_config_keys=photo_stalls=0;bs.Stall=vstall;
 exit_phase=exit_reads=exit_stalls=exit_keys=refusal_count=cancel_keys=0;
 report_seen=report_reads=mem_calls=cold_passes=report_keys=sx=sy=0;memset(screen,0,sizeof(screen));
 asset_allocs=asset_frees=asset_fs_calls=windows_fs_calls=0;memset(asset_pools,0,sizeof(asset_pools));
 patch_memory((VOID*)GuEmbeddedAssets,saved_embedded,sizeof(saved_embedded));
 corrupted=(vcase>=91&&vcase<=95)?(int)vcase-91:(vcase==9||vcase==539)?0:-1;
 if(corrupted>=0){saved_byte=*GuEmbeddedAssets[corrupted].begin;UINT8 bad=saved_byte^1;patch_memory((VOID*)GuEmbeddedAssets[corrupted].begin,&bad,1);}
 GU_EMBEDDED_ASSET changed;
 if((vcase>=96&&vcase<=104)||vcase==10||vcase==119||vcase==120){
  unsigned i=vcase>=96&&vcase<=100?vcase-96:0;changed=saved_embedded[i];
  if(vcase<=100||vcase==10)changed.end--;
  if(vcase==101)changed.begin=(VOID*)((UINTN)GuEmbeddedStart-1);
  if(vcase==102)changed.end=GuEmbeddedEnd+1;
  if(vcase==103)changed.end=(VOID*)((UINTN)changed.begin-1);
  if(vcase==104){changed.begin=(VOID*)(UINTN)-16;changed.end=(VOID*)16;}
  if(vcase==119)changed.begin=NULL;
  if(vcase==120)changed.end++;
  patch_memory((VOID*)&GuEmbeddedAssets[i],&changed,sizeof(changed));
 }
 bs.AllocatePool=vpoolalloc;
 bs.HandleProtocol=vhandle;bs.LocateHandleBuffer=vlocate;bs.ProtocolsPerHandle=vprotocols;bs.OpenProtocolInformation=vinfo;
 bs.FreePool=vpoolfree;bs.CloseProtocol=vcloseprotocol;
 bs.DisconnectController=vdisconnect;bs.ConnectController=vconnect;bs.StartImage=vstart;
 pci.Pci.Read=vconfig;pci.RomSize=0x24e00;conout.QueryMode=vquery;conout.Mode=&textmode;
 conout.OutputString=voutput;conin.ReadKeyStroke=vinput;st.ConsoleInHandle=H(7);
 if(vcase==421||vcase==480||vcase==514||vcase==535)st.ConIn=NULL;
 if(vcase==515)conin.ReadKeyStroke=NULL;
 if(vcase==516)st.ConsoleInHandle=NULL;
 if(vcase==481)st.ConsoleInHandle=NULL;
 if(vcase==423||vcase==482)conin.ReadKeyStroke=NULL;
 owner_binding.Supported=owner_binding.Start=vsupported;
 for(unsigned i=0;i<2;i++){aliases[i]=owner_binding;aliases[i].DriverBindingHandle=H(20+i*2);aliases[i].ImageHandle=H(21+i*2);alias_images[i]=owner_image;alias_images[i].ImageBase=H(100+i);}
 modes[1]=modes[0];modes[1].Info=&infos[1];infos[1]=infos[0];
 files[3].protocol.Write=vwrite;for(unsigned i=0;i<12;i++){files[i].protocol.Open=vopen;files[i].protocol.Read=vread;files[i].protocol.Close=vclose;files[i].protocol.GetInfo=vmetadata;}
 pci.Map=vmap;pci.Mem.Read=vmread;pci.Unmap=vunmap;pci.AllocateBuffer=valloc;pci.FreeBuffer=vfree;
 fs.OpenVolume=vvolume;
 if(vcase==68)app_mode=APP_NO_USB;
 if(vcase==71)fs.OpenVolume=NULL;
 if(vcase==74)files[0].protocol.Open=NULL;
 if(vcase==77)files[1].protocol.GetInfo=NULL;
 if(vcase==86)files[0].protocol.Close=NULL;
 if(vcase==87)files[1].protocol.Close=NULL;
 if(vcase==83)app_mode=APP_SECUREBOOT;
 if(vcase==57)app_mode=APP_SECUREBOOT;
 if(vcase==63)infos[1].HorizontalResolution++;
 if(vcase==38)pci.Map=NULL;
 if(vcase==39)bs.StartImage=NULL;
 if(vcase==433)bs.StartImage=NULL;
 if(vcase>=406&&vcase<=423||vcase>=431&&vcase<=440)app_mode=APP_SECUREBOOT;
 if(vcase==434){app_mode=APP_SUCCESS;registers[0x823804/4]=0xffffff0f;}
 if(vcase==435)app_mode=APP_NO_USB;
 if(vcase==11||vcase==538)registers[0x300000/4]^=1;
 if(vcase==537)pci.RomSize--;
 if(vcase==128||vcase==129||vcase>=173&&vcase<=175||vcase==178)registers[0x823804/4]=0xffffff0f;
 if(vcase==130)registers[0]=0x193000a0;
 if(vcase==131)registers[0x1fa828/4]=16;
 if(vcase==132)registers[0x82381c/4]=0;
 if(vcase==133)registers[0x823820/4]=0;
 if(vcase==134)registers[0x1183a4/4]=0;
 if(vcase==135)registers[0x118128/4]=2;
 if(vcase==136)registers[0x118234/4]=0;
 if(vcase==137)registers[0x110100/4]=0;
 if(vcase>=138&&vcase<=140)registers[cold_offsets[1+vcase-138]/4]=0xffffffffU;
 if(vcase==141){for(unsigned i=1;i<8;i++)registers[cold_offsets[i]/4]=0xffffffffU;registers[0x1183a4/4]=0;}
 if(vcase==142)registers[0x823b00/4]=registers[0x8403c4/4]=0xffffffffU;
 if(vcase==143)registers[0x1fa7cc/4]=registers[0x9a0148/4]=registers[0x1fa7c4/4]=0xffffffffU;
 if(vcase==144)registers[0x1fa828/4]=15;
 if(vcase==121){registers[0x823804/4]=0xffffff0f;registers[0x8403c4/4]=0xffffffffU;}
 if(vcase==12)extra_case=1;
 if(vcase==13)extra_case=2;
 if(vcase==14)extra_case=3;
 if(vcase==15)extra_case=4;
 const enum scenario core[]={MAP_FAIL,MAP_PARTIAL,FWSEC_TIMEOUT,UNMAP_FAIL,FLR_WRITE_FAIL,RESTORE_FAIL,RESET_CHANGES_WPR_FIRST,RESET_CORRUPTS_PAYLOAD_FIRST,SEC2_RUN_TIMEOUT,POST_FLR_TIMEOUT,MMIO_WRITE_FAIL,FLUSH_FAIL,BME_READBACK_FAIL};
 if(vcase>=16&&vcase<=28)fake.mode=core[vcase-16];
}
int main(int argc,char**argv){
 CHECK(argc>=4);size_t n;UINT8*d=read_file(argv[1],&n);CHECK(n==sizeof(prom));memcpy(prom,d,n);free(d);
 const char*names[]={"booter_load_ad102.bin","v67-ad102-610.43.03-four-plm.bin","gsp_ga10x.bin","gsp_boot_ad102.bin","gsp_boot_desc_ad102.bin"};
 static char full[5][128];
 for(unsigned i=0;i<5;i++){char p[1024];snprintf(p,sizeof(p),"%s/%s",argv[2],names[i]);snprintf(full[i],sizeof(full[i]),"gpu-unlock/610.43.03/%s",names[i]);originals[i].name=full[i];originals[i].firmware.data=read_file(p,&originals[i].firmware.size);}
 memcpy(saved_embedded,GuEmbeddedAssets,sizeof(saved_embedded));
 setup(SUCCESS);config[0xb4]=9;config[0xb5]=0xc8;config[0xb6]=0x14;memcpy(captured_config,config,256);dispose_host_fixture();
 unsigned first=(unsigned)strtoul(argv[3],NULL,10),last=argc>4?(unsigned)strtoul(argv[4],NULL,10):first;
 for(vcase=first;vcase<=last;vcase++){
 extra_case=0;vsetup();volatile EFI_STATUS result=EFI_ABORTED;int jumped=setjmp(escape_halt);
 if(!jumped)result=EfiMain(H(1),&st);
 printf("CASE=%u status=%llx jump=%d detach=%llu connect=%d StartImage=%d maps=%llu live=%llu FLR=%llu retained=%u\n",vcase,(unsigned long long)result,jumped,(unsigned long long)disconnect_calls,connect_calls,windows_started,(unsigned long long)fake.maps,(unsigned long long)fake.live_maps,(unsigned long long)fake.flrs,Gu.retained);fflush(stdout);
 if((vcase>=64 && vcase<=68)||vcase==82||vcase==83||vcase==84||vcase==85||vcase==88||vcase==89){
  const EFI_STATUS expected[]={EFI_TIMEOUT,EFI_UNSUPPORTED,EFI_TIMEOUT,EFI_UNSUPPORTED,EFI_UNSUPPORTED,EFI_TIMEOUT,EFI_UNSUPPORTED,EFI_UNSUPPORTED,EFI_TIMEOUT,EFI_DEVICE_ERROR,EFI_DEVICE_ERROR,EFI_TIMEOUT,EFI_NOT_FOUND,EFI_NOT_FOUND,EFI_TIMEOUT,EFI_COMPROMISED_DATA,EFI_DEVICE_ERROR,EFI_TIMEOUT,EFI_DEVICE_ERROR,EFI_SECURITY_VIOLATION,EFI_UNSUPPORTED,EFI_UNSUPPORTED,EFI_DEVICE_ERROR,EFI_NOT_FOUND,EFI_TIMEOUT,EFI_TIMEOUT};
  printf("IO REPRO OUTPUT case=%u\n%s\n",vcase,fake.log);fflush(stdout);
  CHECK(!jumped&&!disconnect_calls&&!fake.maps&&!fake.mmio_writes&&!fake.flrs&&!windows_started&&!writes);
  CHECK(result==(vcase==89?EFI_DEVICE_ERROR:expected[vcase-64]));
  CHECK(strstr(fake.log,vcase==83?"stage=secure-boot":"stage=usb-assets"));
  const char *first[]={"LoadedImage","LoadedImage","DevicePath","DevicePath","DevicePath","SimpleFS","SimpleFS","SimpleFS","OpenVolume","OpenVolume","OpenVolume","Root.Open","Root.Open","Root.Open","GetInfo","GetInfo","GetInfo","GetInfo","none","none","DevicePath","DevicePath","OpenVolume","Root.Open","DevicePath","GetInfo"};
  char match[128];snprintf(match,sizeof(match),"IO first=%s",vcase==89?"none":first[vcase-64]);CHECK(strstr(fake.log,match));
  CHECK(strstr(fake.log,vcase==83?"reason=application stage=secure-boot":"reason=application stage=usb-assets"));
  if(vcase==66||vcase==67||vcase==68||vcase==84||vcase==85||vcase==88){CHECK(strstr(fake.log,"IO SimpleFS SKIPPED"));CHECK(strstr(fake.log,"IO OpenVolume SKIPPED"));}
  if(vcase==80)CHECK(strstr(fake.log,"IO GetInfo raw=0x1 result=0x1"));
  if(vcase==82||vcase==89)CHECK(strstr(fake.log,"SECONDARY logging raw=0x1"));
  if(vcase==88)CHECK(strstr(fake.log,"IO SimpleFS SKIPPED"));
  if(vcase==81)CHECK(strstr(fake.log,"SECONDARY cleanup"));
 }
 if(vcase>=121&&vcase<=180&&!auto_independent(vcase)){
  CHECK(!windows_started&&!fake.maps&&!fake.mmio_writes&&!fake.flrs);
  if(vcase==177)CHECK(disconnect_calls==1&&jumped==1&&Gu.retained);else CHECK(!disconnect_calls&&!jumped);
  if(vcase>=122&&vcase<=127){check_page("PATH REFUSAL");CHECK(!mem_calls);}
  else if(vcase>=142&&vcase<=144||vcase==179){CHECK(result==EFI_ABORTED);CHECK(!strstr(fake.log,"COLD REFUSAL"));CHECK(cold_passes==1);}
  else if(vcase==173||vcase==174||vcase==175){CHECK(result==EFI_NOT_READY);CHECK(!GuLogReady());CHECK(report_reads==mem_calls);}
  else {check_page("COLD REFUSAL");CHECK(strstr(fake.log,"not value gates"));}
  if(vcase==121||vcase==128||vcase==129||vcase==178)CHECK(strstr(fake.log,"FAIL FEAT_PLM raw=ffffff0f expected=ffffff8f"));
  if(vcase==130){CHECK(result==EFI_UNSUPPORTED&&mem_calls==1);CHECK(strstr(fake.log,"FAIL CHIP raw=193000a0"));CHECK(strstr(fake.log,"UNKNOWN VRAM"));}
  const char* gate[]={"FAIL WPR2_HI","FAIL SS0","FAIL SS1","FAIL VRAM","FAIL GFW_PLM","FAIL GFW_BOOT","FAIL GFW_CPUCTL","FAIL GFW_PLM","FAIL GFW_BOOT","FAIL GFW_CPUCTL"};
  if(vcase>=131&&vcase<=140)CHECK(strstr(fake.log,gate[vcase-131]));
  if(vcase==141){for(unsigned i=0;i<7;i++)CHECK(strstr(fake.log,gate[i]));CHECK(strstr(fake.log,"FAIL FEAT_PLM"));}
  if(vcase>=145&&vcase<=172){unsigned k=(vcase-145)%14;char want[128];snprintf(want,sizeof(want),"first-read-fault=1 offset=%08x raw-status=%016llx",cold_offsets[k],(unsigned long long)(vcase<159?EFI_TIMEOUT:EFI_WARN_UNKNOWN_GLYPH));CHECK(strstr(fake.log,want));CHECK(mem_calls==k+1);CHECK(strstr(fake.log,"FAIL fault=1"));if(k<13)CHECK(strstr(fake.log,"UNKNOWN VRAM"));}
  if(vcase==176||vcase==177){CHECK(strstr(fake.log,vcase==176?"stage=armed-cold-state":"stage=cold-after-release"));CHECK(strstr(fake.log,"FAIL SS1 raw=00000007"));}
  if(vcase==123)CHECK(strstr(fake.log,"3f:01/01/0004"));
  if(vcase==124)CHECK(strstr(fake.log,"00:03/05/0000"));
  if(vcase==125)CHECK(strstr(fake.log,"00:01/01/1001"));
  if(vcase==126)CHECK(strstr(fake.log,"exact-size=1 AND EndEntire/len4=0"));
  if(vcase==127)CHECK(strstr(fake.log,"01:7f/ff/0008"));
 }
 if(vcase==180){CHECK(result==EFI_DEVICE_ERROR);CHECK(strstr(fake.log,"COLD REFUSAL"));CHECK(strstr(fake.log,"console-at-check=0000000000000001"));}
 if(vcase==122){printf("PATH REPORT REPRO\n%s\n",fake.log);fflush(stdout);CHECK(strstr(fake.log,"PATH REFUSAL"));CHECK(strstr(fake.log,"00:04/04/002a"));CHECK(strstr(fake.log,"01:7f/ff/0004"));}
 if(vcase==121){printf("COLD REPORT REPRO\n%s\n",fake.log);fflush(stdout);CHECK(strstr(fake.log,"COLD REFUSAL"));CHECK(strstr(fake.log,"FAIL FEAT_PLM"));}
 if(vcase>=181&&vcase<=374){
  printf("FINAL CONFIG REPRO case=%u\n%s\n",vcase,fake.log);fflush(stdout);
  CHECK(jumped==1&&!windows_started&&connect_calls==1&&fake.flrs==1);
  CHECK(!fake.live_maps&&!fake.live_buffers&&!Gu.allocations&&asset_frees==5);
  CHECK(strstr(fake.log,"STOP stage=final-config-before-StartImage"));
  CHECK(!strstr(fake.log,"STOP: cleanup cannot be proven"));
  if(vcase!=374){
   CHECK(strstr(fake.log,"FINAL PCI:"));
   char want[160];unsigned valid=vcase>=245&&vcase<=372?(vcase-245)%64:64;
   EFI_STATUS raw=vcase>=245&&vcase<=308?EFI_TIMEOUT:vcase>=309&&vcase<=372?EFI_WARN_UNKNOWN_GLYPH:EFI_SUCCESS;
   unsigned diffs=vcase==373?64:vcase<=244?1:0;
   snprintf(want,sizeof(want),"read=%016llx valid=%03x/100 diffs=%u",(unsigned long long)raw,valid*4,diffs);CHECK(strstr(fake.log,want));
   CHECK(final_reads==valid+(raw!=EFI_SUCCESS));
   if(vcase<=244||vcase==373){unsigned lo=vcase==373?0:vcase-181,hi=vcase==373?63:lo;for(unsigned i=lo;i<=hi;i++){UINT32 before;memcpy(&before,captured_config+i,4);if(i==0x80/4)before&=~0x90000U;snprintf(want,sizeof(want),"%02x:%08x>%08x",i*4,before,before^1U);CHECK(strstr(fake.log,want));}}
   const char*t=strstr(fake.log,"FINAL PCI:");unsigned lines=0,col=0;for(const char*s=t;*s;s++){if(*s=='\n'){lines++;col=0;}else if(*s!='\r'){col++;CHECK(col<80);}}CHECK(lines<=24);
   int visible=0;for(unsigned i=0;i<25;i++)if(strstr(screen[i],"FINAL PCI:"))visible=1;CHECK(visible);
  }else CHECK(!GuLogReady());
 }
 if(vcase==47||vcase==48||vcase==49)CHECK(Gu.retained&&fake.live_buffers);
 if(vcase==45)CHECK(Gu.retained&&!fake.unmaps&&!fake.frees&&fake.live_buffers&&!fake.gsp_starts);
 /* Former asset file fault cases now assert successful independence below. */
 if(vcase==35)CHECK(writes==0);
 if(vcase==4)CHECK(strstr(fake.log,"reason=")&&strstr(fake.log,"OpenCount")&&strstr(fake.log,"source-line="));
 if(vcase==36)CHECK(strstr(fake.log,"POST-GOP SS0="));
 if(vcase==32||vcase==33)CHECK(strstr(fake.log,"StartImage returned")&&strstr(fake.log,"POWER OFF"));
 if(vcase>=484){
  if(vcase==484||vcase==530||vcase==543||vcase==544||vcase==535){
   CHECK(jumped==2&&windows_started==1&&disconnect_calls==1&&connect_calls==1&&fake.flrs==1);
   CHECK(!fake.live_maps&&!fake.live_buffers&&!Gu.allocations);
   if(vcase==535)CHECK(!key_calls&&!exceptional_phase);else CHECK(exceptional_keys==1&&exceptional_phase==2);
  }else if(vcase==529){CHECK(jumped==1&&disconnect_calls==1&&!windows_started&&!fake.maps&&!fake.mmio_writes&&Gu.retained);}
  else {
   CHECK(!disconnect_calls&&!windows_started&&!fake.maps&&!fake.mmio_writes&&!fake.flrs&&!Gu.retained);
   CHECK(result!=EFI_SUCCESS);
   if(vcase==508||vcase==509||vcase==510||vcase==511||vcase==512||vcase==513||vcase>=521&&vcase<=524||vcase==536||vcase>=537&&vcase<=540)CHECK(!exceptional_phase&&!exceptional_keys);
   if(vcase==485||vcase==489||vcase==546||vcase>=531&&vcase<=534)CHECK(exceptional_phase==2&&!exceptional_keys&&exceptional_reads>=600);
   if(vcase>=547&&vcase<=574){CHECK(!exceptional_phase&&!exceptional_keys&&Gu.fault);CHECK(mem_calls==(vcase-547)%14+1);}
   if(vcase==541||vcase==542)CHECK(exceptional_keys==1&&ss0_reads==3&&Gu.fault);
   if(vcase==486||vcase==487||vcase==488)CHECK(!refusal_count&&!exceptional_keys);
  }
 }
 else if(auto_independent(vcase)||vcase>=479){CHECK(jumped==2&&windows_started==1&&disconnect_calls==1&&connect_calls==1&&fake.flrs==1);CHECK(!key_calls&&!photo_phase&&!s_accepted);}
 else if(vcase>=441){
  CHECK(!fake.live_maps&&!fake.live_buffers&&!Gu.allocations);
  if(vcase==473||vcase==474||vcase==475){CHECK(!disconnect_calls&&!windows_started&&!fake.maps&&!fake.mmio_writes);}
  else {
   CHECK(connect_calls==1&&fake.flrs==1&&asset_frees==5);
   if(vcase<=445){CHECK(windows_started==1&&jumped==2);}
   else if(vcase==458||vcase==459||vcase==460){CHECK(jumped==4&&!windows_started&&!Gu.retained);}
   else CHECK(jumped==1&&!windows_started&&Gu.retained);
   if(vcase<=444 || vcase>=457&&vcase<=461 || vcase==464||vcase==465||vcase==466){
    const char *a=strstr(fake.log,"PCI EXCEPTION:"),*b=strstr(fake.log,"AUTO: final fresh");
    CHECK(a&&b&&a<b);CHECK(strstr(fake.log,"PCIe=78 v2 off=80"));
   }
   if(vcase==445)CHECK(!strstr(fake.log,"PCI EXCEPTION:"));
  }
 }
 else if(vcase>=406){
  CHECK(!disconnect_calls&&!connect_calls&&!windows_started&&!fake.maps&&!fake.mmio_writes&&!fake.flrs&&!Gu.retained);
  int waiting=vcase==406||vcase==414||vcase==415||vcase==416||vcase==417||vcase==418||vcase==419||vcase==420||vcase==421||vcase==423||vcase==428||vcase==429;
  CHECK(waiting?(jumped==5&&exit_stalls>1200):(!jumped&&result!=EFI_SUCCESS));
  if(vcase>=424&&vcase<=430){CHECK(cancel_keys==1&&!refusal_count&&!exit_keys);if(!waiting)CHECK(exit_reads==8);}
  else {CHECK(refusal_count==1);if(!waiting)CHECK(exit_keys==1);}
  if(vcase==437)CHECK(exit_reads==24);
  if(vcase==440)CHECK(exit_reads==1014);
  if(vcase==434){check_page("COLD REFUSAL v5-auto-v2");CHECK(strstr(fake.log,"expected=53422157"));}
  if(vcase==435)check_page("PATH REFUSAL v5-auto-v2");
 }
 else if(vcase>=376){
  CHECK(!photo_phase&&connect_calls==1&&fake.flrs==1&&!fake.live_maps&&!fake.live_buffers&&asset_frees==5);
  CHECK(jumped==1&&!windows_started&&Gu.retained);
  if(vcase==392||vcase==393||vcase==394||vcase==395){
   CHECK(strstr(fake.log,"STOP stage=post-AUTO-final-config-before-StartImage"));
   CHECK(strstr(fake.log,vcase==392?"valid=000/100 diffs=0":vcase==393?"valid=080/100 diffs=0":vcase==394?"valid=100/100 diffs=1":"valid=100/100 diffs=64"));
  }
  if(vcase==385)CHECK(strstr(fake.log,"STOP stage=post-AUTO-final-config-before-StartImage"));
  if(vcase==386||vcase==404)CHECK(strstr(fake.log,"STOP stage=post-AUTO-final-graph"));
 }
 else if(vcase==375||vcase==0||vcase==35||vcase==36||vcase==90||(vcase>=40&&vcase<=44)||vcase==52||(vcase>=69&&vcase<=81)||vcase==86||vcase==87)CHECK(windows_started==1&&jumped==2&&fake.flrs==1&&connect_calls==1);
 else if(vcase==32||vcase==33)CHECK(windows_started==1&&jumped==1&&Gu.retained);
 else {CHECK(!windows_started);if(disconnect_calls)CHECK(jumped==1&&Gu.retained);else CHECK(result!=EFI_SUCCESS&&!fake.maps&&!fake.mmio_writes&&!fake.flrs);}
 if(vcase==34)CHECK(replacement_writes==0);
 CHECK(!asset_fs_calls && !writes);
 if(vcase>=91&&vcase<=114 || vcase==119||vcase==120){
  CHECK(!disconnect_calls&&!windows_loaded&&!windows_fs_calls&&!fake.maps&&!fake.mmio_writes&&!Gu.retained);
  CHECK(result==(vcase<=95?EFI_CRC_ERROR:vcase<=104||vcase>=119?EFI_COMPROMISED_DATA:EFI_OUT_OF_RESOURCES));
  for(unsigned i=0;i<5;i++)CHECK(!asset_pools[i]&&!GuAssets[i].firmware.data);
  CHECK(strstr(fake.log,"stage=assets"));
 }
 if(vcase==90){CHECK(windows_fs_calls==1&&asset_allocs==5&&asset_frees==5);for(unsigned i=0;i<5;i++)CHECK(!asset_pools[i]&&!GuAssets[i].firmware.data);}
 if(vcase==115)CHECK(result==EFI_NOT_FOUND&&!disconnect_calls&&strstr(fake.log,"stage=windows-preload"));
 if(vcase==116)CHECK(!disconnect_calls&&asset_allocs==1&&asset_frees==1);
 if(vcase==117||vcase==118)CHECK(disconnect_calls==1&&asset_frees==5);
 if(corrupted>=0)patch_memory((VOID*)saved_embedded[corrupted].begin,&saved_byte,1);
 patch_memory((VOID*)GuEmbeddedAssets,saved_embedded,sizeof(saved_embedded));
 CHECK(disconnect_calls<=1&&connect_calls<=1&&fake.flrs<=1&&fake.gsp_starts<=1&&fake.sec2_starts<=1);
 const char*trace=getenv("GU_TRACE_DIR");if(trace){char name[2048];snprintf(name,sizeof(name),"%s/case-%u.txt",trace,vcase);FILE*out=fopen(name,"wb");CHECK(out);fprintf(out,"case=%u raw_status=%llx returned=%u stop=%u StartImage=%d disconnect=%llu connect=%d maps=%llu live_maps=%llu buffers=%llu FLR=%llu\n%s",vcase,(unsigned long long)result,jumped==0,jumped==1,windows_started,(unsigned long long)disconnect_calls,connect_calls,(unsigned long long)fake.maps,(unsigned long long)fake.live_maps,(unsigned long long)fake.live_buffers,(unsigned long long)fake.flrs,fake.log);CHECK(!fclose(out));}
 printf("PASS whole-application case %u\n",vcase);dispose_host_fixture();
 }
 for(unsigned i=0;i<5;i++)free((VOID*)originals[i].firmware.data);return 0;
}
