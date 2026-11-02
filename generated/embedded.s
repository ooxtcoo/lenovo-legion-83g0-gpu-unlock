.section .rdata,"dr"
.balign 16
.globl GuEmbeddedStart
GuEmbeddedStart:
.incbin "assets/booter_load_ad102.bin"
.incbin "assets/v67-ad102-610.43.03-four-plm.bin"
.incbin "assets/gsp_ga10x.bin"
.incbin "assets/gsp_boot_ad102.bin"
.incbin "assets/gsp_boot_desc_ad102.bin"
.globl GuEmbeddedEnd
GuEmbeddedEnd:
