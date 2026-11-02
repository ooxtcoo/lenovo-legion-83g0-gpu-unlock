/* SPDX-License-Identifier: GPL-2.0-only
 * v5 graph adapter derived from live-tested release/reconnect-v2.
 * EDK2 protocol declarations define all firmware ABIs. Firmware pointers must
 * be conforming; bounded traversal cannot make corrupt pointers safe.
 */
#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/LoadedImage.h>
static EFI_GUID pg=EFI_PCI_IO_PROTOCOL_GUID,gg=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,dg=EFI_DEVICE_PATH_PROTOCOL_GUID,bg=EFI_DRIVER_BINDING_PROTOCOL_GUID,lg=EFI_LOADED_IMAGE_PROTOCOL_GUID,tg=EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID,ig=EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID;
#include <Guid/ConsoleOutDevice.h>
/* Pinned EDK2 MdeModulePkg declaration, also legible in photo page 0005. */
static EFI_GUID cg=EFI_CONSOLE_OUT_DEVICE_GUID;
#define MAX_RECORDS 128
#define MAX_BINDINGS 16
#define MAX_PATH 256
static EFI_SYSTEM_TABLE *st;
static EFI_BOOT_SERVICES *bs;
static EFI_HANDLE self;
static CHAR16 *volatile banner=L"Lenovo AD103M v5 experimental unlock\r\n";
static int failed;
static UINTN stage;
static EFI_STATUS last_status;
typedef struct {EFI_HANDLE at;EFI_GUID guid;EFI_OPEN_PROTOCOL_INFORMATION_ENTRY e;} RECORD;
typedef struct {EFI_HANDLE agent;EFI_DRIVER_BINDING_PROTOCOL *ptr;EFI_DRIVER_BINDING_PROTOCOL binding;EFI_LOADED_IMAGE_PROTOCOL *loaded;VOID *base;UINT64 size;EFI_MEMORY_TYPE type;} BINDING;
typedef struct {
 EFI_HANDLE target,physical,console,owner,alias,cin;
 EFI_PCI_IO_PROTOCOL *pci;
 EFI_GRAPHICS_OUTPUT_PROTOCOL *gop[2];
 EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out;
 EFI_SIMPLE_TEXT_INPUT_PROTOCOL *in;
 UINT32 config[16];
 UINT64 fb;UINTN fbsize;
 EFI_GRAPHICS_OUTPUT_MODE_INFORMATION geometry;
 UINT8 target_path[MAX_PATH],physical_path[MAX_PATH];UINTN tn,pn;
 RECORD rec[MAX_RECORDS];UINTN nr;
 BINDING bind[MAX_BINDINGS];UINTN nb;
} SNAP;
static SNAP original,current;
static void zero(VOID*p,UINTN n){UINT8*x=p;while(n--)*x++=0;}
static int equal(const VOID*a,const VOID*b,UINTN n){const UINT8*x=a,*y=b;while(n--)if(*x++!=*y++)return 0;return 1;}
static int guid_eq(EFI_GUID*a,EFI_GUID*b){return equal(a,b,sizeof(*a));}
typedef struct {CHAR16 *reason;UINTN line,where;UINT64 expected,observed;EFI_STATUS status;RECORD edge;} DIAG;
static DIAG diag;
static VOID (*refusal_report)(VOID);
static RECORD context;
static int deny(UINTN line,CHAR16*reason,UINT64 expected,UINT64 observed){
 if(!diag.reason){diag.reason=reason;diag.line=line;diag.where=stage;diag.expected=expected;diag.observed=observed;diag.status=last_status;diag.edge=context;}return 0;
}
#define REQUIRE(c,r,e,o) do{if(!(c))return deny(__LINE__,L##r,(UINT64)(e),(UINT64)(o));}while(0)
static int status_ok(EFI_STATUS s,UINTN line,CHAR16*name){last_status=s;return s==EFI_SUCCESS?1:deny(line,name,EFI_SUCCESS,s);}
#define ok(s) status_ok((s),__LINE__,L"firmware status (source line identifies call)")
static void ctx(EFI_HANDLE h,EFI_GUID*g,EFI_OPEN_PROTOCOL_INFORMATION_ENTRY*e){zero(&context,sizeof(context));context.at=h;if(g)context.guid=*g;if(e)context.e=*e;}
static int freebuf(VOID*p){return !p||status_ok(bs->FreePool(p),__LINE__,L"FreePool status");}
static int get(EFI_HANDLE h,EFI_GUID*g,VOID**p){
 *p=0;if(!context.e.Attributes)ctx(h,g,0);
 CHAR16*name=guid_eq(g,&bg)?L"HandleProtocol DriverBinding status":guid_eq(g,&lg)?L"HandleProtocol LoadedImage status":guid_eq(g,&pg)?L"HandleProtocol PCI_IO status":guid_eq(g,&dg)?L"HandleProtocol DevicePath status":guid_eq(g,&gg)?L"HandleProtocol GOP status":guid_eq(g,&tg)?L"HandleProtocol TextOut status":L"HandleProtocol TextIn status";
 if(!status_ok(bs->HandleProtocol(h,g,p),__LINE__,name))return 0;
 REQUIRE(*p,"HandleProtocol nonnull interface",1,0);return 1;
}
static int console(SNAP*s){
 s->console=st->ConsoleOutHandle;s->cin=st->ConsoleInHandle;
 ctx(s->console,&tg,0);REQUIRE(s->console,"ConsoleOutHandle present",1,0);
 if(!get(s->console,&tg,(VOID**)&s->out))return 0;
 REQUIRE(s->out==st->ConOut,"ConOut interface identity",(UINTN)st->ConOut,(UINTN)s->out);
 REQUIRE(s->out->OutputString,"OutputString present",1,0);REQUIRE(s->out->QueryMode,"QueryMode present",1,0);REQUIRE(s->out->Mode,"text Mode present",1,0);
 /* AUTO success never consumes input. Refusal exit independently validates it. */
 UINTN c=0,r=0;if(!status_ok(s->out->QueryMode(s->out,s->out->Mode->Mode,&c,&r),__LINE__,L"QueryMode status"))return 0;
 REQUIRE(c>=80,"console columns >= expected",80,c);REQUIRE(r>=25,"console rows >= expected",25,r);return 1;
}
static void out(CHAR16*s){if(!failed&&(!st->ConOut||!st->ConOut->OutputString||!status_ok(st->ConOut->OutputString(st->ConOut,s),__LINE__,L"OutputString status")))failed=1;}
static void hex(UINT64 n){CHAR16 b[19];b[0]='0';b[1]='x';for(unsigned i=0;i<16;i++){unsigned v=(unsigned)(n&15);b[17-i]=(CHAR16)(v<10?'0'+v:'a'+v-10);n>>=4;}b[18]=0;out(b);}
static void value(CHAR16*l,UINT64 n){out(l);hex(n);out(L"\r\n");}
static int path(EFI_HANDLE h,UINT8*copy,UINTN*len){
 ctx(h,&dg,0);EFI_DEVICE_PATH_PROTOCOL*p=0;if(!get(h,&dg,(VOID**)&p))return 0;UINTN off=0;
 for(unsigned i=0;i<64&&off<=MAX_PATH-4;i++){
  UINT8*q=(UINT8*)p+off;UINTN n=q[2]+((UINTN)q[3]<<8);
  REQUIRE(n>=4,"path node length >= expected",4,n);REQUIRE(n<=MAX_PATH-off,"path node length <= remaining",MAX_PATH-off,n);
  for(UINTN j=0;j<n;j++)copy[off+j]=q[j];off+=n;
  if(q[0]==0x7f){REQUIRE(q[1]==0xff,"path end subtype",0xff,q[1]);REQUIRE(n==4,"path end length",4,n);*len=off;return 1;}
 }return deny(__LINE__,L"path terminated within byte/node bounds",1,0);
}
static int binding(SNAP*s,EFI_HANDLE a){
 REQUIRE(a&&a!=self&&a!=s->target&&a!=s->physical&&a!=s->console,"binding agent independent of app/controllers",1,0);
 for(UINTN i=0;i<s->nb;i++)if(s->bind[i].agent==a)return 1;
 REQUIRE(s->nb<MAX_BINDINGS,"binding count < expected",MAX_BINDINGS,s->nb);BINDING*b=&s->bind[s->nb];b->agent=a;
 if(!get(a,&bg,(VOID**)&b->ptr))return 0;b->binding=*b->ptr;
 REQUIRE(b->binding.DriverBindingHandle==a,"DriverBindingHandle equals agent",(UINTN)a,(UINTN)b->binding.DriverBindingHandle);
 REQUIRE(b->binding.Supported,"binding Supported present",1,0);REQUIRE(b->binding.Start,"binding Start present",1,0);REQUIRE(b->binding.Stop,"binding Stop present",1,0);
 REQUIRE(b->binding.ImageHandle&&b->binding.ImageHandle!=self,"binding ImageHandle present and not self",1,0);
 if(!get(b->binding.ImageHandle,&lg,(VOID**)&b->loaded))return 0;
 b->base=b->loaded->ImageBase;b->size=b->loaded->ImageSize;b->type=b->loaded->ImageCodeType;
 REQUIRE(b->base,"loaded image base present",1,0);REQUIRE(b->size,"loaded image size nonzero",1,0);
 REQUIRE(b->size<=64U*1024U*1024U,"loaded image size <= expected",64U*1024U*1024U,b->size);REQUIRE(b->type==EfiBootServicesCode,"loaded image code type",EfiBootServicesCode,b->type);s->nb++;return 1;
}
static int passive(UINT32 a){return a==EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL||a==EFI_OPEN_PROTOCOL_GET_PROTOCOL||a==EFI_OPEN_PROTOCOL_TEST_PROTOCOL;}
/* Capture complete bounded protocol sets and every ACTIVE edge. Passive GET
 * records can change as a consequence of HandleProtocol, so do not pin counts. */
static int records(SNAP*s,EFI_HANDLE h){
 EFI_GUID**p=0;EFI_OPEN_PROTOCOL_INFORMATION_ENTRY*e=0;UINTN n=0;int good=1;ctx(h,0,0);
 #define RC(c,r,x,y) do{if(!(c)){deny(__LINE__,L##r,(UINT64)(x),(UINT64)(y));good=0;goto cleanup;}}while(0)
 RC(status_ok(bs->ProtocolsPerHandle(h,&p,&n),__LINE__,L"ProtocolsPerHandle status"),"ProtocolsPerHandle status",0,last_status);
 RC(p,"protocol list present",1,0);RC(n&&n<=32,"protocol count in 1..32",32,n);
 for(UINTN i=0;i<n;i++){
  RC(p[i],"protocol GUID present",1,0);ctx(h,p[i],0);
  for(UINTN j=0;j<i;j++)RC(!guid_eq(p[i],p[j]),"duplicate protocol GUID",0,1);
  RC(s->nr<MAX_RECORDS,"record count < expected",MAX_RECORDS,s->nr);RECORD*r=&s->rec[s->nr++];r->at=h;r->guid=*p[i];
  UINTN count=0;RC(status_ok(bs->OpenProtocolInformation(h,p[i],&e,&count),__LINE__,L"OpenProtocolInformation status"),"OpenProtocolInformation status",0,last_status);
  RC(count<=64,"open record count <= expected",64,count);RC(!count||e,"open record buffer present",1,0);
  for(UINTN j=0;j<count;j++){
   ctx(h,p[i],&e[j]);if(passive(e[j].Attributes))continue;
   RC(e[j].Attributes==EFI_OPEN_PROTOCOL_BY_DRIVER||e[j].Attributes==EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER,"active attrs must equal 0x10 or 0x08",0x10,e[j].Attributes);
   RC(e[j].OpenCount==1,"active OpenCount",1,e[j].OpenCount);RC(e[j].ControllerHandle,"active ControllerHandle present",1,0);
   RC(binding(s,e[j].AgentHandle),"active agent binding valid",1,0);RC(s->nr<MAX_RECORDS,"record count < expected",MAX_RECORDS,s->nr);
   r=&s->rec[s->nr++];r->at=h;r->guid=*p[i];r->e=e[j];
  }
  good=freebuf(e);e=0;if(!good)goto cleanup;
 }
 cleanup: if(!freebuf(e))good=0;if(!freebuf(p))good=0;
 #undef RC
 return good;
}
static int geometry(EFI_GRAPHICS_OUTPUT_PROTOCOL*g,EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*i,UINT64*base,UINTN*size){
 REQUIRE(g,"GOP present",1,0);REQUIRE(g->Mode,"GOP Mode present",1,0);REQUIRE(g->Mode->Info,"GOP Info present",1,0);
 REQUIRE(g->Mode->SizeOfInfo>=sizeof(*i),"GOP info bytes >= expected",sizeof(*i),g->Mode->SizeOfInfo);
 *i=*g->Mode->Info;*base=g->Mode->FrameBufferBase;*size=g->Mode->FrameBufferSize;
 REQUIRE(*base,"framebuffer base nonzero",1,0);REQUIRE(*size,"framebuffer size nonzero",1,0);REQUIRE(*base<=~(UINT64)0-*size,"framebuffer range no overflow",1,0);
 REQUIRE(i->Version==0,"GOP info version",0,i->Version);REQUIRE(i->HorizontalResolution,"GOP width nonzero",1,0);REQUIRE(i->VerticalResolution,"GOP height nonzero",1,0);
 REQUIRE(i->PixelFormat<=PixelBlueGreenRedReserved8BitPerColor,"GOP pixel format <= expected",PixelBlueGreenRedReserved8BitPerColor,i->PixelFormat);
 REQUIRE(i->PixelsPerScanLine>=i->HorizontalResolution,"GOP stride >= width",i->HorizontalResolution,i->PixelsPerScanLine);
 REQUIRE((UINT64)i->PixelsPerScanLine*i->VerticalResolution<=*size/4,"GOP pixels <= framebuffer capacity",*size/4,(UINT64)i->PixelsPerScanLine*i->VerticalResolution);return 1;
}
static int snapshot(SNAP*s){
 zero(s,sizeof(*s));stage=10;ctx(0,0,0);if(!console(s))return 0;
 EFI_HANDLE*h=0;UINTN n=0;ctx(0,&pg,0);int good=status_ok(bs->LocateHandleBuffer(ByProtocol,&pg,0,&n,&h),__LINE__,L"LocateHandleBuffer status");UINTN found=0;
 if(good&&(!h||!n||n>256))good=deny(__LINE__,L"PCI handle count 1..256 and buffer present",256,n);
 if(good)for(UINTN i=0;i<n;i++){
  EFI_PCI_IO_PROTOCOL*p=0;UINT32 c[16];ctx(h[i],&pg,0);
  if(!get(h[i],&pg,(VOID**)&p)){good=0;break;}
  if(!p->Pci.Read){good=deny(__LINE__,L"PCI Read present",1,0);break;}
  if(!status_ok(p->Pci.Read(p,EfiPciIoWidthUint32,0,16,c),__LINE__,L"PCI config Read status")){good=0;break;}
  if(c[0]==0x275710de){found++;s->target=h[i];s->pci=p;for(unsigned j=0;j<16;j++)s->config[j]=c[j];}
 }
 if(!freebuf(h))good=0;if(!good)return 0;ctx(s->target,&pg,0);REQUIRE(found==1,"matching PCI target count",1,found);
 stage=11;UINTN seg=0,b=0,d=0,f=0;
 REQUIRE(s->pci->GetLocation,"GetLocation present",1,0);if(!status_ok(s->pci->GetLocation(s->pci,&seg,&b,&d,&f),__LINE__,L"GetLocation status"))return 0;
 REQUIRE(seg==0,"PCI segment",0,seg);REQUIRE(b==1,"PCI bus",1,b);REQUIRE(d==0,"PCI device",0,d);REQUIRE(f==0,"PCI function",0,f);
 REQUIRE(s->config[11]==0x3d3417aa,"PCI subsystem",0x3d3417aa,s->config[11]);REQUIRE((s->config[1]&0xffff)==3,"PCI command",3,s->config[1]&0xffff);
 REQUIRE(((s->config[3]>>16)&0x7f)==0,"PCI header layout (bit7 ignored)",0,(s->config[3]>>16)&0x7f);
 REQUIRE((s->config[4]&~15U)==0x85000000,"PCI BAR0 masked",0x85000000,s->config[4]&~15U);
 if(!path(s->target,s->target_path,&s->tn)||!records(s,s->target))return 0;
 /* Photo pages 0003/0004: owner opens PCI_IO; child opens DevicePath.
  * Count exactly one child across both allowed protocols, never ignore extras. */
 UINTN owners=0,children=0;
 for(UINTN i=0;i<s->nr;i++){RECORD*r=&s->rec[i];
  if(guid_eq(&r->guid,&pg)&&r->e.Attributes==EFI_OPEN_PROTOCOL_BY_DRIVER){s->owner=r->e.AgentHandle;owners++;}
  if((guid_eq(&r->guid,&pg)||guid_eq(&r->guid,&dg))&&r->e.Attributes==EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER){s->physical=r->e.ControllerHandle;children++;ctx(r->at,&r->guid,&r->e);}
 }
 RECORD child_context=context;ctx(s->target,&pg,0);REQUIRE(owners==1,"PCI_IO BY_DRIVER owner count",1,owners);
 context=child_context;if(!children)ctx(s->target,&dg,0);REQUIRE(children==1,"PCI_IO/DevicePath child edge count",1,children);
 REQUIRE(s->physical!=s->target&&s->physical!=s->console&&s->physical!=self,"child distinct from target/console/app",1,0);
 for(UINTN i=0;i<s->nr;i++){RECORD*r=&s->rec[i];if(!r->e.Attributes)continue;ctx(r->at,&r->guid,&r->e);
  REQUIRE(r->e.AgentHandle==s->owner,"target edge agent equals owner",(UINTN)s->owner,(UINTN)r->e.AgentHandle);
  if(r->e.Attributes==EFI_OPEN_PROTOCOL_BY_DRIVER){REQUIRE(r->e.ControllerHandle==s->target,"driver edge controller equals target",(UINTN)s->target,(UINTN)r->e.ControllerHandle);}
  else {REQUIRE(r->e.ControllerHandle==s->physical,"child edge controller equals physical",(UINTN)s->physical,(UINTN)r->e.ControllerHandle);REQUIRE(guid_eq(&r->guid,&pg)||guid_eq(&r->guid,&dg),"child edge protocol PCI_IO or DevicePath",1,0);}
 }
 stage=12;if(!path(s->physical,s->physical_path,&s->pn))return 0;
 EFI_DEVICE_PATH_PROTOCOL*remaining=(VOID*)s->physical_path;EFI_HANDLE ancestor=0;
 if(!status_ok(bs->LocateDevicePath(&pg,&remaining,&ancestor),__LINE__,L"LocateDevicePath status"))return 0;REQUIRE(ancestor==s->target,"physical GOP PCI ancestor",(UINTN)s->target,(UINTN)ancestor);
 ctx(s->physical,&gg,0);if(!get(s->physical,&gg,(VOID**)&s->gop[0])||!geometry(s->gop[0],&s->geometry,&s->fb,&s->fbsize))return 0;
 h=0;n=0;good=status_ok(bs->LocateHandleBuffer(ByProtocol,&gg,0,&n,&h),__LINE__,L"LocateHandleBuffer status");
 if(good&&(!h||n!=2))good=deny(__LINE__,L"GOP handles count and buffer",2,n);
 if(good&&!((h[0]==s->physical&&h[1]==s->console)||(h[1]==s->physical&&h[0]==s->console)))good=deny(__LINE__,L"GOP set exactly physical + console",1,0);
 if(!freebuf(h))good=0;if(!good)return 0;
 ctx(s->console,&dg,0);EFI_DEVICE_PATH_PROTOCOL*cp=0;EFI_STATUS ps=bs->HandleProtocol(s->console,&dg,(VOID**)&cp);last_status=ps;
 REQUIRE(!cp,"console wrapper has no device path",0,(UINTN)cp);REQUIRE(ps==EFI_UNSUPPORTED||ps==EFI_NOT_FOUND,"console path status UNSUPPORTED or NOT_FOUND",EFI_UNSUPPORTED,ps);
 last_status=EFI_SUCCESS;ctx(s->console,&gg,0);EFI_GRAPHICS_OUTPUT_MODE_INFORMATION ci;UINT64 cb;UINTN cz;
 if(!get(s->console,&gg,(VOID**)&s->gop[1]))return 0;REQUIRE(s->gop[0]!=s->gop[1],"distinct GOP interfaces",1,0);
 if(!geometry(s->gop[1],&ci,&cb,&cz))return 0;
 REQUIRE(cb==s->fb,"console framebuffer base equals physical",s->fb,cb);REQUIRE(cz==s->fbsize,"console framebuffer size equals physical",s->fbsize,cz);
 REQUIRE(equal(&ci,&s->geometry,sizeof(ci)),"console geometry equals physical (predicate)",1,0);
 ctx(s->physical,&cg,0);h=0;n=0;good=status_ok(bs->LocateHandleBuffer(ByProtocol,&cg,0,&n,&h),__LINE__,L"LocateHandleBuffer status");
 if(good&&(!h||n!=1))good=deny(__LINE__,L"ConsoleOutDevice parent count and buffer",1,n);
 if(good&&h[0]!=s->physical)good=deny(__LINE__,L"ConsoleOutDevice parent equals physical",(UINTN)s->physical,(UINTN)h[0]);
 if(!freebuf(h))good=0;if(!good)return 0;
 UINTN start=s->nr;stage=13;if(!records(s,s->physical)||!records(s,s->console))return 0;
 UINTN drivers=0,edges=0;
 for(UINTN i=start;i<s->nr;i++){RECORD*r=&s->rec[i];if(!r->e.Attributes)continue;ctx(r->at,&r->guid,&r->e);
  if(r->at==s->physical&&guid_eq(&r->guid,&cg)&&r->e.Attributes==EFI_OPEN_PROTOCOL_BY_DRIVER){s->alias=r->e.AgentHandle;drivers++;}
  if(r->e.Attributes==EFI_OPEN_PROTOCOL_BY_DRIVER){REQUIRE(r->e.ControllerHandle==r->at,"downstream driver controls own handle",(UINTN)r->at,(UINTN)r->e.ControllerHandle);}
  else {REQUIRE(r->at==s->physical,"alias child edge on physical",(UINTN)s->physical,(UINTN)r->at);REQUIRE(guid_eq(&r->guid,&cg),"alias child uses ConsoleOutDevice GUID",1,0);REQUIRE(r->e.ControllerHandle==s->console,"alias child equals system console",(UINTN)s->console,(UINTN)r->e.ControllerHandle);edges++;}
 }
 ctx(s->physical,&cg,0);
 REQUIRE(drivers==1,"ConsoleOutDevice owner count",1,drivers);REQUIRE(edges==1,"console child edge count",1,edges);REQUIRE(s->alias!=s->owner,"alias driver distinct from GPU owner",1,0);
 for(UINTN i=start;i<s->nr;i++){RECORD*r=&s->rec[i];if(r->e.Attributes==EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER){ctx(r->at,&r->guid,&r->e);REQUIRE(r->e.AgentHandle==s->alias,"console child agent equals alias",(UINTN)s->alias,(UINTN)r->e.AgentHandle);}}
 return 1;
}
static EFI_HANDLE norm(EFI_HANDLE h,const SNAP*s){return h==s->physical?(EFI_HANDLE)(UINTN)1:h==s->console?(EFI_HANDLE)(UINTN)2:h;}
static int same(const SNAP*a,const SNAP*b,int restored){
 ctx(b->target,&pg,0);
 #define FIELD(f) REQUIRE(a->f==b->f,"snapshot field " #f,(UINT64)(UINTN)a->f,(UINT64)(UINTN)b->f)
 FIELD(target);FIELD(owner);FIELD(alias);FIELD(console);FIELD(cin);FIELD(nr);FIELD(nb);FIELD(tn);FIELD(pn);FIELD(fb);FIELD(fbsize);
 REQUIRE(equal(&a->geometry,&b->geometry,sizeof(a->geometry)),"snapshot geometry equal (predicate)",1,0);
 REQUIRE(equal(a->target_path,b->target_path,a->tn),"snapshot target path equal (predicate)",1,0);REQUIRE(equal(a->physical_path,b->physical_path,a->pn),"snapshot physical path equal (predicate)",1,0);
 REQUIRE(a->config[0]==b->config[0],"snapshot PCI identity",a->config[0],b->config[0]);REQUIRE(a->config[11]==b->config[11],"snapshot PCI subsystem",a->config[11],b->config[11]);
 REQUIRE((a->config[1]&0xffff)==(b->config[1]&0xffff),"snapshot PCI command",a->config[1]&0xffff,b->config[1]&0xffff);REQUIRE(a->config[3]==b->config[3],"snapshot PCI header dword",a->config[3],b->config[3]);
 for(unsigned i=4;i<=9;i++)REQUIRE(a->config[i]==b->config[i],"snapshot PCI BAR dword",a->config[i],b->config[i]);
 if(!restored){FIELD(physical);FIELD(pci);FIELD(gop[0]);FIELD(gop[1]);FIELD(out);FIELD(in);}
 #undef FIELD
 for(UINTN i=0;i<a->nb;i++){int match=0;for(UINTN j=0;j<b->nb;j++)if(equal(&a->bind[i],&b->bind[j],sizeof(BINDING)))match++;ctx(a->bind[i].agent,&bg,0);REQUIRE(match==1,"snapshot exact binding metadata matches",1,match);}
 for(UINTN i=0;i<a->nr;i++){RECORD x=a->rec[i];ctx(x.at,&x.guid,&x.e);if(restored){x.at=norm(x.at,a);x.e.ControllerHandle=norm(x.e.ControllerHandle,a);}int match=0;for(UINTN j=0;j<b->nr;j++){RECORD y=b->rec[j];if(restored){y.at=norm(y.at,b);y.e.ControllerHandle=norm(y.e.ControllerHandle,b);}if(equal(&x,&y,sizeof(x)))match++;}REQUIRE(match==1,"snapshot exact protocol/active edge matches",1,match);}
 return 1;
}
/* Pre-mutation cancellation is consumed once, then cleanup and quiet exit. */
static int cancelled;
/* A fresh empty queue must be observed; never Reset the input device. */
static int key(CHAR16 wanted){
 EFI_INPUT_KEY k;EFI_STATUS s;EFI_SIMPLE_TEXT_INPUT_PROTOCOL*in=st->ConIn;
 ctx(st->ConsoleInHandle,&ig,0);REQUIRE(!failed,"visible consent/output required",0,failed);REQUIRE(in&&in->ReadKeyStroke,"input interface/method present",1,0);
 if(wanted=='U'||wanted=='C'){
  unsigned quiet=0;
  for(unsigned i=0;i<64&&quiet<5;i++){
   s=in->ReadKeyStroke(in,&k);
   if(s==EFI_NOT_READY)quiet++;else if(s==EFI_SUCCESS)quiet=0;else return status_ok(s,__LINE__,L"quiet input read status");
   if(!ok(bs->Stall(100000)))return 0;
  }
  REQUIRE(quiet>=5,"five quiet input samples required",5,quiet);
  if(wanted=='C')out(L"Press fresh C to CONTINUE ONCE; Q/Esc/Space cancels.\r\n");
  else out(L"ARMED: press fresh U for UNLOCK + reset; later S for Windows.\r\n");
  REQUIRE(!failed,"ARMED prompt output successful",0,failed);
 }
 for(unsigned i=0;i<64;i++){s=in->ReadKeyStroke(in,&k);if(s==EFI_NOT_READY)break;if(s!=EFI_SUCCESS)return status_ok(s,__LINE__,L"input drain status");REQUIRE(i!=63,"input queue drained within 64 polls",1,0);}
 for(unsigned i=0;i<600;i++){
  s=in->ReadKeyStroke(in,&k);
  if(s==EFI_SUCCESS){
   if(k.ScanCode==23||(!k.ScanCode&&(k.UnicodeChar=='q'||k.UnicodeChar=='Q'||k.UnicodeChar==' '))){if(wanted=='Q')return 1;cancelled=1;return deny(__LINE__,L"user cancelled before detach",wanted,k.UnicodeChar);}
   if(!k.ScanCode&&(k.UnicodeChar==wanted||k.UnicodeChar==wanted+32))return 1;
  }else if(s!=EFI_NOT_READY)return status_ok(s,__LINE__,L"consent input read status");
  if(!ok(bs->Stall(100000)))return 0;
 }
 return deny(__LINE__,L"fresh consent key timeout (600 polls)",wanted,0);
}
/* Pre-mutation only: quiet queue, fresh exit key, then quiet queue again.
 * No timeout return, reset, retry, or mutation. Input/Stall faults latch a
 * non-returning wait; a broken/missing Stall cannot provide a timed delay.
 * Firmware callbacks can hang. Existing watchdog policy is unchanged. */
static void exit_wait(int cancel_only){
 EFI_SIMPLE_TEXT_INPUT_PROTOCOL*in=st?st->ConIn:NULL;
 unsigned quiet=0;int ready=0,exiting=cancel_only,broken=0;
 for(;;){
  if(!broken && in && in->ReadKeyStroke && bs && bs->Stall){
   EFI_INPUT_KEY k;EFI_STATUS s=in->ReadKeyStroke(in,&k);
   if(s!=EFI_SUCCESS && s!=EFI_NOT_READY)broken=1;
   else if(!ready){
    if(s==EFI_SUCCESS)quiet=0;
    else if(++quiet>=5){if(exiting)return;ready=1;}
   }else if(s==EFI_SUCCESS &&
     ((k.ScanCode==23 && !k.UnicodeChar) ||
      (!k.ScanCode && (k.UnicodeChar==' ' || k.UnicodeChar=='q' || k.UnicodeChar=='Q')))){
    ready=0;quiet=0;exiting=1;
   }
  }
  if(bs && bs->Stall && bs->Stall(100000)!=EFI_SUCCESS)broken=1;
 }
}

static void guid_text(EFI_GUID*g){
 CHAR16 b[37];unsigned pos=0;UINT8 bytes[16];
 bytes[0]=(UINT8)(g->Data1>>24);bytes[1]=(UINT8)(g->Data1>>16);bytes[2]=(UINT8)(g->Data1>>8);bytes[3]=(UINT8)g->Data1;
 bytes[4]=(UINT8)(g->Data2>>8);bytes[5]=(UINT8)g->Data2;bytes[6]=(UINT8)(g->Data3>>8);bytes[7]=(UINT8)g->Data3;for(unsigned i=0;i<8;i++)bytes[8+i]=g->Data4[i];
 for(unsigned i=0;i<16;i++){if(i==4||i==6||i==8||i==10)b[pos++]='-';b[pos++]=L"0123456789abcdef"[bytes[i]>>4];b[pos++]=L"0123456789abcdef"[bytes[i]&15];}b[pos]=0;out(b);
}
static EFI_STATUS refuse(EFI_STATUS result){
 if(cancelled){exit_wait(1);return result;}
 if(!diag.reason)deny(__LINE__,L"missing diagnostic: internal refusal",1,0);
 /* Reattempt only the current pre-mutation console. Failure stays terminal. */
 failed=0;
 if(st&&st->ConOut&&st->ConOut->OutputString){
  out(L"\r\nREFUSED - NO DETACH. NOT ARMED. v5-auto-v2\r\n");out(L"reason=");out(diag.reason);out(L"\r\n");
  value(L"stage=",diag.where);value(L"source-line=",diag.line);value(L"status=",diag.status);
  value(L"expected=",diag.expected);value(L"observed=",diag.observed);
  if(diag.where){value(L"handle=",(UINTN)diag.edge.at);out(L"GUID=");guid_text(&diag.edge.guid);out(L"\r\n");
  value(L"attrs=",diag.edge.e.Attributes);value(L"count=",diag.edge.e.OpenCount);
  value(L"agent=",(UINTN)diag.edge.e.AgentHandle);value(L"controller=",(UINTN)diag.edge.e.ControllerHandle);
  out(L"Zero context fields mean not applicable/not obtained.\r\n");}
  out(L"Photograph now. Release keys; Space/Q/Esc returns to launcher.\r\n");
  out(L"No retry inside this run. Firmware/watchdog can still hang/reset.\r\n");
 }
 if(refusal_report)refusal_report();
 exit_wait(0);
 return result;
}

#include "lifecycle.h"
#include "port.h"
static BOOLEAN captured;
EFI_STATUS GuGraphInit(EFI_HANDLE image,EFI_SYSTEM_TABLE *table) {
 st=table;bs=st?st->BootServices:NULL;self=image;failed=0;stage=0;last_status=EFI_SUCCESS;captured=FALSE;
 refusal_report=NULL;cancelled=0;
 zero(&diag,sizeof(diag));zero(&context,sizeof(context));zero(&original,sizeof(original));
 if(!st||!bs)return EFI_UNSUPPORTED;
 #define S(f) if(!bs->f){deny(__LINE__,L"required service " #f,1,0);return EFI_UNSUPPORTED;}
 S(HandleProtocol);S(LocateHandleBuffer);S(LocateDevicePath);S(ProtocolsPerHandle);S(OpenProtocolInformation);S(FreePool);S(Stall);S(DisconnectController);S(ConnectController);S(AllocatePool);S(RaiseTPL);S(RestoreTPL);S(OpenProtocol);S(CloseProtocol);S(LoadImage);S(UnloadImage);S(StartImage);S(SetWatchdogTimer);
 #undef S
 return EFI_SUCCESS;
}
BOOLEAN GuGraphAttached(BOOLEAN restored) {
 if(!snapshot(&current))return FALSE;
 if(current.target!=Gu.handle||current.pci!=Gu.pci)return deny(__LINE__,L"graph/transaction PCI identity",(UINTN)Gu.pci,(UINTN)current.pci);
 if(!captured){original=current;captured=TRUE;return TRUE;}
 return same(&original,&current,restored);
}
EFI_HANDLE GuGraphOwner(VOID){return captured?original.owner:NULL;}
BOOLEAN GuGraphDetached(VOID) {
 zero(&current,sizeof(current));current.target=Gu.handle;stage=25;
 if(!captured||Gu.handle!=original.target||!records(&current,Gu.handle))return FALSE;
 for(UINTN i=0;i<current.nr;i++)if(current.rec[i].e.Attributes)return deny(__LINE__,L"residual target active open after disconnect",0,current.rec[i].e.Attributes);
 return TRUE;
}
/* Called after EXCLUSIVE acquisition and during the transaction. Unlike an
 * attached snapshot this never dereferences an old GOP/console interface. */
BOOLEAN GuGraphDetachedDisplay(VOID) {
 EFI_HANDLE *h=NULL;UINTN n=0;EFI_STATUS s=bs->LocateHandleBuffer(ByProtocol,&gg,NULL,&n,&h);
 if(s==EFI_NOT_FOUND)return TRUE;
 if(!ok(s))return FALSE;
 int good=(n<=1&&(!n||h));
 if(good&&n)good=h[0]==original.console;
 if(!freebuf(h))good=0;
 if(!good)deny(__LINE__,L"detached GOP set empty or console-only",1,n);
 return good;
}
EFI_STATUS GuGraphConsent(VOID) {
 out(L"AUTO: launch authorizes ONE transaction, then Windows after all checks.\r\n");
 out(L"ONLY Lenovo 83G0 NXCN26WW; confirm BIOS yourself.\r\n");
 out(L"EXPERIMENT: firmware DMA + SEC2 writes + FLR. No retry.\r\n");
 out(L"Black screen/hang possible. On STOP/blank hold physical power OFF.\r\n");
 return failed?EFI_DEVICE_ERROR:EFI_SUCCESS;
}
EFI_STATUS GuGraphSs0Consent(UINT32 ss0) {
 EFI_SIMPLE_TEXT_INPUT_PROTOCOL *in=NULL;
 if(failed || !st->ConsoleInHandle || !get(st->ConsoleInHandle,&ig,(VOID**)&in) ||
    in!=st->ConIn || !in->ReadKeyStroke)return EFI_DEVICE_ERROR;
 out(L"SS0 EXCEPTION: unfamiliar cold value; safety UNKNOWN.\r\n");
 value(L"Observed SS0=",ss0);
 out(L"Automatic values: 0x53422157 / 0x53402157 only; SS1 must be 6.\r\n");
 out(L"Continuing may hang or damage state. No safety/performance claim.\r\n");
 out(L"Other guards still apply. Release all keys; no timeout acceptance.\r\n");
 return !failed && key('C')?EFI_SUCCESS:EFI_ABORTED;
}
VOID GuGraphSetRefusalReport(VOID (*report)(VOID)){refusal_report=report;}
EFI_STATUS GuGraphRefuseAt(EFI_STATUS s,const char *app_stage){
 static CHAR16 reason[96];UINTN n=0;const char *prefix="application stage=";
 while(*prefix && n<95)reason[n++]=(UINT8)*prefix++;
 while(app_stage && *app_stage && n<95)reason[n++]=(UINT8)*app_stage++;
 reason[n]=0;
 if(!diag.reason){stage=0;ctx(0,0,0);last_status=s;deny(__LINE__,reason,0,s);}return refuse(s);
}
EFI_STATUS GuGraphRefuse(EFI_STATUS s){if(!diag.reason){stage=0;ctx(0,0,0);last_status=s;deny(__LINE__,L"application preflight failure; see FINAL stage",0,s);}return refuse(s);}
BOOLEAN GuGraphFreshConsole(VOID) {
 EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *p=NULL;
 if(!st||!bs||!bs->HandleProtocol||!st->ConsoleOutHandle)return FALSE;
 EFI_STATUS s=bs->HandleProtocol(st->ConsoleOutHandle,&tg,(VOID**)&p);
 if(s!=EFI_SUCCESS||!p||p!=st->ConOut||!p->OutputString)return FALSE;
 failed=0;return TRUE;
}
