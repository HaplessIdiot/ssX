# $XFree86: xc/programs/Xserver/hw/xfree86/XF86Setup/carddata.tcl,v 3.1 1996/06/30 10:44:01 dawes Exp $
#
#

set ServerList {	Mono  VGA16  SVGA   8514  AGX I128
			Mach8 Mach32 Mach64 P9000 S3  W32 }
set AccelServerList {	8514 AGX I128 Mach8 Mach32 Mach64 P9000 S3 W32 }

###

# For each server, what chipsets can be chosen?
set CardChipSets(Mono)	   { ??? }
set CardChipSets(VGA16)	   { ??? }
set CardChipSets(SVGA)	   { ??? }
set CardChipSets(8514)	   { ibm8514 }
set CardChipSets(AGX)	   { XGA-1 XGA-2 AGX-010 AGX-014 AGX-015 AGX-016 }
set CardChipSets(I128)	   {}
set CardChipSets(Mach8)	   { mach8 }
set CardChipSets(Mach32)   { ??? }
set CardChipSets(Mach64)   { ??? }
set CardChipSets(P9000)	   { vipervlb viperpci orchid_p9000 }
set CardChipSets(S3)	   { s3_generic mmio_928 }
set CardChipSets(W32)	   { et4000w32 et4000w32i et4000w32p_rev_a \
			     et4000w32i_rev_b et4000w32p_rev_b \
			     et4000w32p_rev_d et4000w32p_rev_c \
			     et4000w32i_rev_c }

###

# For each server, what ramdacs can be chosen?
set CardRamDacs(Mono)	   { ??? }
set CardRamDacs(VGA16)	   { ??? }
set CardRamDacs(SVGA)	   { ??? }
set CardRamDacs(8514)	   {}
set CardRamDacs(AGX)	   { normal bt481 bt482 sc15025 \
			     herc_dual_dac herc_small_dac xga att20c490 }
set CardRamDacs(I128)	   { ti3025 ibm524 ibm528 }
set CardRamDacs(Mach8)	   {}
set CardRamDacs(Mach32)	   { ??? }
set CardRamDacs(Mach64)	   { internal ati68875 tlc34075 tvp3026 bt476 \
			     bt478 bt481 inmos176 inmos178 sc15026 \
			     ati68860 ati68860b ati68860c ati68880 \
			     stg1700 att20c498 sc15021 att21c498 \
			     stg1702 stg1703 ch8398 att20c408 \
			     att20c491 ibm_rgb514 }
set CardRamDacs(P9000)	   {}
set CardRamDacs(S3)	   { normal bt485 bt9485 att20c505 \
			     ti3020 att20c498 att21c498 att22c498 \
			     ti3025 ti3026 ti3030 ibm_rgb514 ibm_rgb524 \
			     ibm_rgb525 ibm_rgb526 ibm_rgb528 att20c490 \
			     att20c491 ch8391 sc11482 sc11483 \
			     sc11484 sc11485 sc11487 sc11489 \
			     sc15025 stg1700 stg1703 s3_sdac \
			     ics5342 s3gendac ics5300 s3_trio32 \
			     s3_trio64 s3_trio att20c409 }

set CardRamDacs(W32)	   { normal att20c47xa sc1502x att20c497 \
			     att20c490 att20c493 att20c491 att20c492 \
			     ics5341 gendac stg1700 stg1703 }

###

# For each server, what clockchips can be chosen?
set CardClockChips(Mono)   { ??? }
set CardClockChips(VGA16)  { ??? }
set CardClockChips(SVGA)   { ??? }
set CardClockChips(8514)   { ??? }
set CardClockChips(AGX)	   { ??? }
set CardClockChips(I128)   { ??? }
set CardClockChips(Mach8)  { ??? }
set CardClockChips(Mach32) { ??? }
set CardClockChips(Mach64) { ati18818 ics2595 stg1703 ch8398 \
			     att20c408 ibm_rgb514 }
set CardClockChips(P9000)  { ??? }
set CardClockChips(S3)	   { icd2061a ics9161a dcs2824 sc11412 \
			     s3gendac s3_sdac ics5300 ics5342 \
			     s3_trio s3_trio32 s3_trio64 ti3025 \
			     ti3026 ti3030 ibm_rgb5xx ics2595 ch8391 \
			     stg1703 att20c409 att20c499 }

set CardClockChips(W32)	   { icd2061a ics5341 stg1703 }

# For each server, what options can be chosen?
set CardOptions(Mono) [list \
	legend swap_hibit 8clocks 16clocks probe_clocks hibit_high hibit_low \
	intern_disp extern_disp clgd6225_lcd \
	fast_dram med_dram slow_dram nomemaccess nolinear intel_gx no_2mb_banksel fifo_conservative fifo_moderate fifo_aggressive mmio linear slow_vram s3_slow_vram slow_dram_refresh s3_slow_dram_refresh fast_vram s3_fast_vram pci_burst_on pci_burst_off w32_interleave_on w32_interleave_off \
	noaccel hw_cursor sw_cursor no_bitblt favour_bitblt favor_bitblt no_imageblt no_font_cache no_pixmap_cache trio32_fc_bug s3_968_dash_bug \
	bt485_curs ti3020_curs no_ti3020_curs ti3026_curs ibmrgb_curs dac_8_bit sync_on_green bt482_curs s3_964_bt485_vclk dac_6_bit \
	spea_mercury number_nine stb_pegasus elsa_w1000pro elsa_w1000isa elsa_w2000pro diamond genoa stb hercules miro_magic_s4 \
	composite secondary pci_hack power_saver override_bios no_block_write block_write no_bios_clocks s3_invert_vclk no_program_clocks no_pci_probe \
	showcache fb_debug \
	8_bit_bus wait_state no_wait_state crtc_delay vram_128 vram_256 refresh_20 refresh_25 vlb_a vlb_b sprite_refresh screen_refresh vram_delay_latch vram_delay_ras vram_extend_ras engine_delay clock_50 clock_66 no_wait first_wwait write_wait one_wait read_wait all_wait enable_bitblt \
]
set CardOptions(VGA16)	   $CardOptions(Mono)
set CardOptions(SVGA)	   $CardOptions(Mono)
set CardOptions(8514)	   $CardOptions(Mono)
set CardOptions(AGX)	   $CardOptions(Mono)
set CardOptions(I128)	   $CardOptions(Mono)
set CardOptions(Mach8)	   $CardOptions(Mono)
set CardOptions(Mach32)	   $CardOptions(Mono)
set CardOptions(Mach64)	   $CardOptions(Mono)
set CardOptions(P9000)	   $CardOptions(Mono)
set CardOptions(S3)	   $CardOptions(Mono)
set CardOptions(W32)	   $CardOptions(Mono)

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

