# $XFree86: xc/programs/Xserver/hw/xfree86/XF86Setup/carddata.tcl,v 3.3 1996/08/18 01:47:18 dawes Exp $
#
#

set ServerList {	Mono  VGA16  SVGA   8514  AGX I128
			Mach8 Mach32 Mach64 P9000 S3  W32 }
set AccelServerList {	8514 AGX I128 Mach8 Mach32 Mach64 P9000 S3 W32 }

###

# For each server, what chipsets can be chosen?
set CardChipSets(Mono)	   { \
			     ati \
			     cl6410 cl6412 cl6420 cl6440 \
			     clgd5420 clgd5422 clgd5424 clgd5426 \
				clgd5428 clgd5429 clgd5430 clgd5434 \
				clgd5436 clgd5446 clgd6215 clgd6225 \
				clgd6235 clgd7541 clgd7542 clgd7543 \
			     et3000 \
			     et4000 et4000w32 et4000w32i et4000w32p \
				et6000 \
			     gvga \
			     ncr77c22 ncr77c22e \
			     oti067 oti077 oti087 oti037c \
			     pvga1 wd90c00 wd90c10 wd90c30 wd90c24 \
				wd90c31 wd90c31 wd90c33 wd90c20 \
			     sis86c201 sis86c202 sis86c205 \
			     tvga8200lx tvga8800cs tvga8900b tvga8900c \
				tvga8900cl tvga8900d tvga9000 tvga9000i \
				tvga9100b tvga9200cxr \
				tgui9320lcd tgui9400cxi tgui9420 \
				tgui9420dgi tgui9430dgi tgui9440agi \
				tgui9660xgi tgui9680 \
			   }
set CardChipSets(Mono)	   [concat generic [lsort $CardChipSets(Mono)]]
set CardChipSets(VGA16)	   { \
			     ati \
			     cl6410 cl6412 cl6420 cl6440 \
			     et3000 \
			     et4000 et4000w32 et4000w32i et4000w32p \
				et6000 \
			     ncr77c22 ncr77c22e \
			     oti067 oti077 oti087 oti037c \
			     sis86c201 sis86c202 sis86c205 \
			     tvga8200lx tvga8800cs tvga8900b tvga8900c \
				tvga8900cl tvga8900d tvga9000 tvga9000i \
				tvga9100b tvga9200cxr \
				tgui9320lcd tgui9400cxi tgui9420 \
				tgui9420dgi tgui9430dgi tgui9440agi \
				tgui9660xgi tgui9680 \
			   }
set CardChipSets(VGA16)	   [concat generic [lsort $CardChipSets(VGA16)]]
set CardChipSets(SVGA)	   { \
			     al2101 \
			     ali2228 ali2301 ali2302 ali2308 ali2401 \
			     ark1000vl ark1000pv ark2000pv \
			     ati \
			     cl6410 cl6412 cl6420 cl6440 \
			     clgd5420 clgd5422 clgd5424 clgd5426 \
				clgd5428 clgd5429 clgd5430 clgd5434 \
				clgd5436 clgd5446 clgd6215 clgd6225 \
				clgd6235 clgd7541 clgd7542 clgd7543 \
			     cpq_avga \
			     ct65520 ct65530 ct65540 ct65545 \
				ct65546 ct65548 ct65550 ct65554 \
				ct451 ct452 ct453 ct455 ct456 ct457 \
			     et3000 \
			     et4000 et4000w32 et4000w32i et4000w32p \
				et6000 \
			     gvga \
			     mx \
			     ncr77c22 ncr77c22e \
			     oti067 oti077 oti087 oti037c \
			     pvga1 wd90c00 wd90c10 wd90c30 wd90c24 \
				wd90c31 wd90c31 wd90c33 wd90c20 \
			     realtek \
			     s3 \
			     sis86c201 sis86c202 sis86c205 \
			     tvga8200lx tvga8800cs tvga8900b tvga8900c \
				tvga8900cl tvga8900d tvga9000 tvga9000i \
				tvga9100b tvga9200cxr \
				tgui9320lcd tgui9400cxi tgui9420 \
				tgui9420dgi tgui9430dgi tgui9440agi \
				tgui9660xgi tgui9680 \
			     video7 \
			   }
set CardChipSets(SVGA)	   [concat generic [lsort $CardChipSets(SVGA)]]
set CardChipSets(8514)	   { ibm8514 }
set CardChipSets(AGX)	   { agx-010 agx-014 agx-015 agx-016 xga-1 xga-2 }
set CardChipSets(I128)	   { i128 }
set CardChipSets(Mach8)	   { mach8 }
set CardChipSets(Mach32)   { mach32 }
set CardChipSets(Mach64)   { mach64 }
set CardChipSets(P9000)	   { orchid_p9000 viperpci vipervlb }
set CardChipSets(S3)	   { mmio_928 s3_generic }
set CardChipSets(W32)	   { et4000w32 et4000w32i et4000w32i_rev_b \
			     et4000w32i_rev_c et4000w32p_rev_a \
			     et4000w32p_rev_b et4000w32p_rev_c \
			     et4000w32p_rev_d et6000 }

###

# For each server, what ramdacs can be chosen?
set CardRamDacs(8514)	   {}
set CardRamDacs(AGX)	   { normal att20c490 bt481 bt482 \
			     herc_dual_dac herc_small_dac \
			     sc15025 xga }
set CardRamDacs(I128)	   { ibm526 ibm528 ti3025 }
set CardRamDacs(Mach8)	   {}
set CardRamDacs(Mach32)	   { ati68830 ati68860 ati68875 \
			     att20c490 att20c491 att21c498 \
			     bt476 bt478 bt481 bt482 bt885 \
			     ims_g173 ims_g174 \
			     inmos176 inmos178 \
			     mu9c1880 mu9c4870 mu9c4910 \
			     sc11483 sc11486 sc11488 \
			     	sc15021 sc15025 sc15026 \
			     stg1700 stg1702 \
			     tlc34075 }
set CardRamDacs(Mach64)	   { internal \
			     ati68860 ati68860b ati68860c ati68875 \
			     att20c408 att20c491 att20c498 att21c498 \
			     	att498 \
			     bt476 bt478 bt481 \
			     ch8398 \
			     ibm_rgb514 \
			     ims_g174 \
			     inmos176 inmos178 \
			     mu9c1880 \
			     sc15021 sc15026 \
			     stg1700 stg1702 stg1703 \
			     tlc34075 \
			     tvp3026 \
			   }
set CardRamDacs(P9000)	   {}
set CardRamDacs(S3)	   { normal \
			     att20c409 att20c490 att20c491 att20c498 \
				att20c505 att21c498 att22c498 \
			     bt485 bt9485 \
			     ch8391 \
			     ibm_rgb514 ibm_rgb524 ibm_rgb525 \
				ibm_rgb526 ibm_rgb528 \
			     ics5300 ics5342 \
			     s3gendac s3_sdac \
				s3_trio s3_trio32 s3_trio64 \
			     sc11482 sc11483 sc11484 sc11485 \
				sc11487 sc11489 sc15025 \
			     stg1700 stg1703 \
			     ti3020 ti3025 ti3026 ti3030 \
			   }
set CardRamDacs(W32)	   { normal \
			     att20c47xa att20c490 att20c491 \
				att20c492 att20c493 att20c497 \
			     ics5341 sc1502x stg1703 et6000 }
set CardRamDacs(SVGA-ark)	   { ark1491a att20c490 att20c498 \
					ics5342 stg1700 \
					w30c491 w30c498 w30c516 \
					zoomdac }
set CardRamDacs(SVGA-ati)	   [lrmdups [concat \
					$CardRamDacs(Mach8) \
					$CardRamDacs(Mach32) \
					$CardRamDacs(Mach64)] ]
set CardRamDacs(SVGA-et4000)	   $CardRamDacs(W32)
set svgadacs ""
foreach driv [array names CardRamDacs SVGA-*] {
	eval lappend svgadacs $CardRamDacs($driv)
}
set CardRamDacs(SVGA)	   [lrmdups $svgadacs]
unset svgadacs
set CardRamDacs(VGA16)	   [lrmdups [concat $CardRamDacs(SVGA-ati) \
				$CardRamDacs(SVGA-et4000)] ]
set CardRamDacs(Mono)	   $CardRamDacs(VGA16)

###

# For each server, what clockchips can be chosen?
set CardClockChips(8514)   {}
set CardClockChips(AGX)	   {}
set CardClockChips(I128)   { ibm_rgb526 ibm_rgb528 ibm_rbg52x ibm_rgb5xx \
			     ti3025 }
set CardClockChips(Mach8)  {}
set CardClockChips(Mach32) {}
set CardClockChips(Mach64) { ati18818 att20c408 ch8398 ibm_rgb514 \
			     ics2595 stg1703 }
set CardClockChips(P9000)  { icd2061a }
set CardClockChips(S3)	   { ati18818 att20c409 att20c499 att20c408 \
			     ch8391 dcs2824 \
			     ibm_rgb514 ibm_rgb51x ibm_rgb524 ibm_rgb525 \
				ibm_rgb528 ibm_rgb52x ibm_rgb5xx \
			     icd2061a ics2595 ics5300 ics5342 ics9161a \
			     s3_sdac s3_trio s3_trio32 \
				s3_trio64 s3gendac \
			     sc11412 stg1703 ti3025 ti3026 ti3030 \
			   }

set CardClockChips(W32)	   { dcs2824 et6000 icd2061a ics5341 stg1703 }
set CardClockChips(SVGA-ark)		{ ics5341 }
set CardClockChips(SVGA-et4000)		$CardClockChips(W32)
set CardClockChips(SVGA-tvga8900)	{ tgui }
set svgaclks ""
foreach driv [array names CardClockChips SVGA-*] {
	eval lappend svgaclks $CardClockChips($driv)
}
set CardClockChips(SVGA)   [lrmdups $svgaclks]
unset svgaclks
set CardClockChips(VGA16)  [lrmdups [concat $CardClockChips(SVGA-et4000) \
				$CardClockChips(SVGA-tvga8900)] ]
set CardClockChips(Mono)   $CardClockChips(VGA16)

# For each server, what options can be chosen?
set CardOptions(Mono)	   { 16clocks 8clocks all_wait clgd6225_lcd \
			     clkdiv2 clock_50 clock_66 composite \
			     enable_bitblt epsonmemwin extern_disp \
			     fast_dram favour_bitblt favor_bitblt \
			     fb_debug fifo_aggressive fifo_conservative \
			     first_wwait ga98nb1 ga98nb2 ga98nb4 \
			     hibit_high hibit_low hw_cursor intern_disp \
			     legend linear med_dram mmio nec_cirrus \
			     noaccel nolinear no_2mb_banksel no_bitblt \
			     no_imageblt no_pci_probe no_program_clocks \
			     no_wait one_wait pc98_tgui pci_burst_off \
			     pci_burst_on power_saver probe_clocks \
			     read_wait slow_dram swap_hibit sw_cursor \
			     tgui_pci_read_off tgui_pci_read_on \
			     tgui_pci_write_off tgui_pci_write_on \
			     w32_interleave_off w32_interleave_on \
			     wap write_wait \
			   }
set CardOptions(VGA16)	   { 16clocks all_wait clgd6225_lcd clkdiv2 \
			     clock_50 clock_66 composite enable_bitblt \
			     fast_dram fb_debug fifo_aggressive \
			     fifo_conservative first_wwait hibit_high \
			     hibit_low hw_cursor legend linear med_dram \
			     mmio noaccel nolinear no_pci_probe \
			     no_program_clocks no_wait one_wait \
			     pc98_tgui pci_burst_off pci_burst_on \
			     power_saver probe_clocks read_wait \
			     slow_dram tgui_pci_read_off tgui_pci_read_on \
			     tgui_pci_write_off tgui_pci_write_on \
			     w32_interleave_off w32_interleave_on \
			     write_wait \
			   }
set CardOptions(SVGA)	   { 16clocks 8clocks all_wait clgd6225_lcd \
			     clkdiv2 clock_50 clock_66 composite \
			     enable_bitblt epsonmemwin extern_disp \
			     ext_fram_buf fast_dram favour_bitblt \
			     favor_bitblt fb_debug fifo_aggressive \
			     fifo_conservative first_wwait ga98nb1 \
			     ga98nb2 ga98nb4 hibit_high hibit_low \
			     hw_clocks hw_cursor intern_disp lcd_center \
			     lcd_centre no_stretch legend linear \
			     med_dram mmio nec_cirrus noaccel nolinear \
			     no_2mb_banksel no_bitblt no_imageblt \
			     no_pci_probe no_program_clocks no_wait \
			     one_wait pc98_tgui pci_burst_off\
			     pci_burst_on power_saver probe_clocks \
			     read_wait slow_dram stn suspend_hack \
			     swap_hibit sw_cursor tgui_pci_read_off \
			     tgui_pci_read_on tgui_pci_write_off \
			     tgui_pci_write_on use_modeline \
			     w32_interleave_off w32_interleave_on wap \
			     write_wait \
			   }
set CardOptions(8514)	   {}
set CardOptions(AGX)	   { 8_bit_bus bt482_curs bt485_curs clkdiv2 \
			     crtc_delay dac_6_bit dac_8_bit engine_delay \
			     fast_dram fast_vram s3_fast_vram \
			     fifo_aggressive fifo_conservative \
			     fifo_moderate med_dram noaccel nolinear \
			     no_wait_state refresh_20 refresh_25 \
			     slow_dram slow_vram s3_slow_vram \
			     sprite_refresh screen_refresh sw_cursor \
			     sync_on_green vlb_a vlb_b vram_128 \
			     vram_256 vram_delay_latch vram_delay_ras \
			     vram_extend_ras wait_state \
			   }
set CardOptions(I128)	   { dac_8_bit power_saver showcache sync_on_green }
set CardOptions(Mach8)	   composite
set CardOptions(Mach32)	   { clkdiv2 composite dac_8_bit intel_gx \
			     nolinear sw_cursor }
set CardOptions(Mach64)	   { block_write clkdiv2 composite dac_6_bit \
			     dac_8_bit hw_cursor no_bios_clocks \
			     no_block_write no_font_cache no_pixmap_cache \
			     no_program_clocks override_bios power_saver \
			     sw_cursor \
			   }
set CardOptions(P9000)	   { noaccel sw_cursor sync_on_green vram_128 vram_256 }
set CardOptions(S3)	   { bt485_curs clkdiv2 dac_6_bit dac_8_bit \
			     diamond elsa_w1000pro elsa_w1000isa \
			     elsa_w2000pro epsonmemwin fast_vram \
			     s3_fast_vram fb_debug genoa hercules \
			     ibmrgb_curs legend miro_magic_s4 \
			     necwab noinit nolinear no_font_cache \
			     nomemaccess no_pixmap_cache no_ti3020_curs \
			     number_nine pchkb pci_hack pcskb pcskb4 \
			     power_saver pw805i pw968 pw_localbus \
			     pw_mux s3_964_bt485_vclk s3_968_dash_bug \
			     showcache slow_dram slow_dram_refresh \
			     s3_slow_dram_refresh slow_edodram slow_vram \
			     s3_slow_vram spea_mercury stb stb_pegasus \
			     sw_cursor sync_on_green ti3020_curs \
			     ti3026_curs trio32_fc_bug trio64v+_bug1 \
			     trio64v+_bug2 trio64v+_bug3 \
			   }
set CardOptions(W32)	   { clkdiv2 fast_dram hibit_high hibit_low \
			     legend pci_burst_off pci_burst_on power_saver \
			     w32_interleave_off w32_interleave_on }

# For each server, what readme files are applicable?
set CardReadmes(Mono)	   {}
set CardReadmes(VGA16)	   {}
set CardReadmes(SVGA)	   {README.Oak README.Video7 README.WstDig \
			    README.ark README.ati README.cirrus \
			    README.trident README.tseng}
set CardReadmes(8514)	   {}
set CardReadmes(AGX)	   README.agx
set CardReadmes(I128)	   {}
set CardReadmes(Mach8)	   {}
set CardReadmes(Mach32)	   {}
set CardReadmes(Mach64)	   README.Mach64
set CardReadmes(P9000)	   README.P9000
set CardReadmes(S3)	   README.S3
set CardReadmes(W32)	   README.W32

