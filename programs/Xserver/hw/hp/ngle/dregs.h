/* $XConsortium: dregs.h,v 1.1 93/08/08 12:56:22 rws Exp $ */
/*************************************************************************
 * 
 * (c)Copyright 1992 Hewlett-Packard Co.,  All Rights Reserved.
 *
Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Hewlett Packard not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

 HEWLETT-PACKARD MAKES NO WARRANTY OF ANY KIND WITH REGARD
TO THIS SOFWARE, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  Hewlett-Packard shall not be liable for errors
contained herein or direct, indirect, special, incidental or
consequential damages in connection with the furnishing,
performance, or use of this material.
 
 *************************************************************************/

typedef union {
    long int                lheight;
    long int                ly;
    struct {
        short int           x;
        short int           y;
    } xy;
    struct {
        short int           width;
        short int           height;
    } wh;
    unsigned long int       all;
    struct {
        unsigned short int  high;
        unsigned short int  low;
    } w;
    struct {
        char   b0;
        char   b1;
        char   b2;
        char   b3;
    } b;
} dreg_cplx_t;



typedef volatile struct {
    long int pad1[70];
    unsigned long int reg1;
    long int pad2[217];
    unsigned long int reg2;
    long int pad3[7];
    unsigned long int reg3;
    long int pad4[63];
    unsigned long int reg22;
    long int pad5[7];
    unsigned long int reg23;
    long int pad6[15];
    unsigned long int reg4;
    long int pad7[39];
    unsigned long int reg5;
    long int pad8[87];
    dreg_cplx_t reg6;
    dreg_cplx_t reg7;
    dreg_cplx_t reg24;
    long int pad9[5];
    unsigned long int reg8;
    long int pad10[120];
    dreg_cplx_t reg9;
    long int pad10a[62];
    dreg_cplx_t reg25;
    long int pad11[23871];
    unsigned long int reg10;
    unsigned long int reg11;
    long int pad12[1];
    unsigned long int reg12;
    long int pad13[2];
    unsigned long int reg13;
    dreg_cplx_t reg14;
    long int pad15[499704];    
    dreg_cplx_t reg15;
    dreg_cplx_t reg16;
    long int pad17[62];
    dreg_cplx_t reg17;
    dreg_cplx_t reg18;
    long int pad19[62];
    dreg_cplx_t reg19;
    long int pad20[1];
    dreg_cplx_t reg20;
    long int pad21[3];
    dreg_cplx_t reg21;
} ngle_dregs_t;
