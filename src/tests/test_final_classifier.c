/* CPU-only tests of the actual checks.c, never firmware callbacks. */
#include "../checks.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define T(x) do { ++tests; if(!(x)){fprintf(stderr,"classifier line %d: %s\n",__LINE__,#x);exit(1);} }while(0)
static unsigned tests;
static void healthy(UINT32 *c,unsigned cap,unsigned version){
 memset(c,0,256);c[0]=GU_BOARD_ID;c[0x2c/4]=GU_BOARD_SUBSYSTEM;
 c[1]=0x00100003;c[2]=0x030000a1;c[3]=0x00800010;
 c[0x34/4]=0x40;c[0x40/4]=0x00030001U|(cap<<8);
 c[cap/4]=(version<<16)|0x10;c[(cap+4)/4]=1U<<28;
 c[(cap+8)/4]=0x2930;
}
int main(void){
 UINT32 a[64],b[64],save[64];GU_CAPS caps;BOOLEAN clear;
 for(unsigned version=1;version<=2;version++)for(unsigned cap=0x48;cap+(version==1?0x14:0x3c)<=256;cap+=4){
  healthy(a,cap,version);memcpy(b,a,256);T(GuCheckFinalConfig(a,b,&caps,&clear));T(!clear&&caps.pcie==cap&&caps.pcie_version==version);
  unsigned idx=(cap+8)/4;
  for(unsigned mask=1;mask<=9;mask++)if(!(mask&~9)){
   a[idx]|=mask<<16;T(GuCheckFinalConfig(a,b,&caps,&clear));T(clear);
   memcpy(save,a,256);
   for(unsigned i=0;i<64;i++)for(unsigned bit=0;bit<32;bit++){
    b[i]^=1U<<bit;T(!GuCheckFinalConfig(a,b,&caps,&clear));b[i]^=1U<<bit;
   }
   T(!memcmp(a,save,256));a[idx]&=~0x90000U;
  }
  for(unsigned bit=0;bit<16;bit++){
   a[idx]|=1U<<(bit+16);b[idx]=a[idx];
   T(GuCheckFinalConfig(a,b,&caps,&clear)==(bit==4));
   b[idx]&=~(1U<<(bit+16));
   T(GuCheckFinalConfig(a,b,&caps,&clear)==(bit==0||bit==3));
   a[idx]&=~(1U<<(bit+16));b[idx]=a[idx];
  }
 }
 /* Both readings of photo value, both v1/v2, AuxPower unchanged. */
 for(unsigned value=0;value<2;value++){
  healthy(a,0x78,2);a[0x80/4]=value?0x92830:0x92930;memcpy(b,a,256);b[0x80/4]&=~0x90000U;
  T(GuCheckFinalConfig(a,b,&caps,&clear)&&clear);
  a[0x80/4]|=0x100000;b[0x80/4]|=0x100000;T(GuCheckFinalConfig(a,b,&caps,&clear));
 }
 /* Independently corrupt each chain, including identical invalid snapshots. */
 for(unsigned side=0;side<3;side++)for(unsigned defect=0;defect<14;defect++){
  healthy(a,0x78,2);memcpy(b,a,256);a[0x80/4]|=0x90000;
  UINT32 *c=side==0?a:b;
  switch(defect){
   case 0:c[0x34/4]=0;break;
   case 1:c[0x34/4]=0x41;break;
   case 2:c[0x34/4]=0x3c;break;
   case 3:c[0x78/4]|=0x7800;break;
   case 4:c[0x78/4]|=0xb400;c[0xb4/4]=0x00020010;c[0xb8/4]=1U<<28;break;
   case 5:c[0x78/4]|=0x8000;c[0x80/4]=0x00030001;break;
   case 6:c[0x78/4]=0x003c0009;break;
   case 7:c[0x78/4]=0x00030010;break;
   case 8:c[0x78/4]=0x00220010;break;
   case 9:c[0x78/4]|=0xfc00;c[0xfc/4]=0x00080009;break;
   case 10:c[0x78/4]|=0xb400;c[0xb4/4]=0x000000ff;break;
   case 11:c[0x40/4]=0x00027810;c[0x44/4]=1U<<28;break;
   case 12:c[0x40/4]=0x00037c01;memmove(&c[0x7c/4],&c[0x78/4],0x3c);c[0x78/4]=0;break;
   case 13:c[0x78/4]=(c[0x78/4]&~0xf0000)|0x10000;break;
  }
  if(side==2)memcpy(a,b,256);
  /* A same-chain version1 is legitimate only when BOTH snapshots match. */
  if(side==2&&(defect==12||defect==13))T(GuCheckFinalConfig(a,b,&caps,&clear));
  else T(!GuCheckFinalConfig(a,b,&caps,&clear));
 }
 T(!GuCheckFinalConfig(NULL,b,&caps,&clear));T(!GuCheckFinalConfig(a,NULL,&caps,&clear));
 T(!GuCheckFinalConfig(a,b,NULL,&clear));T(!GuCheckFinalConfig(a,b,&caps,NULL));
 printf("PASS actual checks.c classifier assertions=%u\n",tests);return 0;
}
