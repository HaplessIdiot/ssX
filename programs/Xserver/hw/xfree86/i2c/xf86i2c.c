/* (c) Itai Nahshon
 *
 * This code is derived from and inspired by the I2C driver
 * from the Linux kernel.
 *      (c) 1998 Gerd Knorr <kraxel@cs.tu-berlin.de>
 */

/* $XFree86$ */

#if 1
#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "validate.h"
#include "resource.h"
#include "gcstruct.h"
#include "dixstruct.h"
#else
typedef int Bool;
typedef void *Pointer;
#define NULL ((void *)0)
#define X_DEFAULT 0
#define TRUE  1
#define FALSE 0
#endif

#include "xf86i2c.h"

#if 0	/* Do not show trace of  I2C commands */
#define I2C_DEBUG(x)        (x)
#else
#define I2C_DEBUG(x)	    (0)
#endif

/*
 * I2C bus works at 100 kbit/second (or less).
 *
 * These are temporary delay functions (until a better, portable
 * way is found).
 *
 * xdelay (n) - delay n microseconds.
 * Adjust counter to match CPU speed.
 */
static int ddelay = 1660;
static void xdelay(unsigned int n) {
	while(n > 0) {
		volatile x = ddelay;
		while(x > 0)
			--x;
		--n;
	}
}

static void udelay(int x) { xdelay(x); }

#define D (udelay(1))

/*
 * Most drivers will register just with Get/Put bits functions.
 * Following functions implement generic I2CPutByte and I2CGetbyte
 * by using the promitive functions given by the driver.
 *
 * It is assumed that there is just one master on the I2C bus, therefore
 * There is no explicit test for conflits.
 */

/*
 * Send a start signal on the I2C bus. The start signal notifies
 * devices that a new transaction is initiated by the bus master.
 *
 * The start signal is always followed by a device address.
 * Devide addresses are 8 bits. The first 7 bits identifies the
 * device and the last bit signals if this is a read (1) or
 * write (0) operation.
 *
 * There may be more than one start signal on one transaction.
 * This happens for example on some devices that allow reading
 * of registers. First send a start bit followed by the device
 * address (with the last bit 0) and the register number. Then send
 * a new start bit with the device address (with the last bit 1)
 * and then read the value from the device.
 */
static void
I2CStart(I2CBusPtr b)
{
	D; b->I2CPutBits(b, 1, 1);
	D; b->I2CPutBits(b, 1, 0);
	D; b->I2CPutBits(b, 0, 0);
#ifdef I2C_DEBUG
	ErrorF("i2c: <");
#endif
}

/*
 * Signal devices on the I2C bus that a transaction on the
 * bus has finished. There may be more than one start signal
 * on a transaction but only one stop signal.
 */
static void
I2CStop(I2CBusPtr b)
{
	D; b->I2CPutBits(b, 0, 0);
	D; b->I2CPutBits(b, 1, 0);
	D; b->I2CPutBits(b, 1, 1);
#ifdef I2C_DEBUG
	ErrorF(">\n");
#endif
}

/*
 * Write a single "One" bit to a device.
 */
static void
I2CWriteOne(I2CBusPtr b)
{
	D; b->I2CPutBits(b, 0, 1);
	D; b->I2CPutBits(b, 1, 1);
	D; b->I2CPutBits(b, 0, 1);
}

/*
 * Write a single "Zero" bit to a device.
 */
static void
I2CWriteZero(I2CBusPtr b)
{
	D; b->I2CPutBits(b, 0, 0);
	D; b->I2CPutBits(b, 1, 0);
	D; b->I2CPutBits(b, 0, 0);
}

/*
 * Get the "Ack" bit from a device.
 */
static int
I2CGetAck(I2CBusPtr b)
{
	int scl, ack;
    
        D; b->I2CPutBits(b, 0, 1);
        D; b->I2CPutBits(b, 1, 1);
	D; b->I2CGetBits(b, &scl, &ack);

        D; b->I2CPutBits(b, 0, 1);

	return ack;
}

/*
 * This is the default I2CPutByte if not supplied by s driver.
 *
 * A single byte is sent to the device, MSB first.
 * If wait_for_ack is non zero, there is some delay. Then
 * The Ack bit is read from the device.
 */
static Bool
xf86I2CPutByte(I2CBusPtr b, unsigned char data, int wait_for_ack, int *ack)
{
	int i;
    
        b->I2CPutBits(b, 0, 0);

	for (i = 7; i >= 0; i--) {
                if (data&(1<<i))
			I2CWriteOne(b);
		else
			I2CWriteZero(b);
	}

	if (wait_for_ack)
		udelay(wait_for_ack);

	*ack = I2CGetAck(b);

#ifdef I2C_DEBUG
	ErrorF("%02x%c ", (int)data, *ack?'-':'+');
#endif

	return *ack;
}

/*
 * This is the default I2CGetbyte if not supplied by s driver.
 *
 * A single data byte is read from a device.
 * Bits are read MSB first. 
 * If this is NOT the last byte read on a transaction,
 * we have to acknowledge this to the device that sent
 * this byte. Tha last read byte is usually not acknowledged.
 */
static Bool
xf86I2CGetByte(I2CBusPtr b, unsigned char *data, Bool last)
{
	int i;
        int c, d;
	unsigned char rdata = 0;

        b->I2CPutBits(b, 0, 1);

	for (i=7; i>=0; i--) 
	{
		b->I2CPutBits(b, 1, 1);
                b->I2CGetBits(b, &c, &d);
		if (d) rdata |= (1<<i);
		b->I2CPutBits(b, 0, 1);
	}

	if (last)
		 I2CWriteOne(b);
	else
		 I2CWriteZero(b);

        *data = rdata;

#ifdef I2C_DEBUG
	ErrorF("=%02x%c ", (int)rdata, last?'-':'+');
#endif

	return TRUE;
}

static I2CBusPtr I2CBusList;

I2CBusPtr
xf86CreateI2CBusRec(void) {
	I2CBusPtr b = (I2CBusPtr)xcalloc(1, sizeof(I2CBusRec));
	if(b == NULL)
		return NULL;
	return b;
}

void
xf86DestroyI2CBusRec(I2CBusPtr b) {
	I2CBusPtr *p;

	if(b == NULL)
		return;

	/* remove this from the list of active I2C busses. */
	for(p = &I2CBusList; *p != NULL; p = &(*p)->NextBus) {
		if(*p == b) {
			*p = (*p)->NextBus;
			break;
		}
	}

	if(b->scrnIndex >= 0)
		xf86DrvMsg(b->scrnIndex, X_DEFAULT, "I2C bus \"%s\" removed.\n",
			b->BusName);
	else
		xf86Msg(X_DEFAULT, "I2C bus \"%s\" removed.\n",
			b->BusName);

	if(b->FirstDev != NULL) {
		ErrorF("Attempt to remove I2C bus \"%s\", but device list is not empty.\n",
			b->BusName);

		return;		/* What can we do */
	}

	xfree(b);
}

Bool
xf86I2CBusInit(I2CBusPtr b) {

	/* I2C busses must be identified with a name */
	if(b->BusName == NULL)
		return FALSE;
	/*
	 * If the high level functions are not
	 * supplied, use the generic functions.
	 * In this case we need the low-level
	 * function.
	 */
	if(b->I2CPutByte == NULL ||
	   b->I2CGetByte == NULL) {

		if(b->I2CPutBits == NULL ||
		   b->I2CGetBits == NULL)
			return FALSE;

		if(b->I2CPutByte == NULL)
			b->I2CPutByte = xf86I2CPutByte;
	
		if(b->I2CPutByte == NULL)
			b->I2CGetByte = xf86I2CGetByte;
	}

	/*
	 * Put new bus on list.
	 */
	b->NextBus = I2CBusList;
	I2CBusList = b;

	if(b->scrnIndex >= 0)
		xf86DrvMsg(b->scrnIndex, X_DEFAULT, "I2C bus \"%s\" initialized.\n",
			b->BusName);
	else
		xf86Msg(X_DEFAULT, "I2C bus \"%s\" initialized.\n",
			b->BusName);

	return TRUE;
}

I2CBusPtr
xf86I2CFindBus(char *name) {
I2CBusPtr p;

	for(p = I2CBusList; p != NULL; p = p->NextBus) {
		if(!strcmp(p->BusName, name))
			return p;
	}

	return NULL;
}

/* Function for probing. Just send the address and return
   true if the device returns and ack. */
Bool
xf86I2CTryAddress(I2CBusPtr b, I2CAddr addr) {
        int ack;

	I2CStart(b);
	b->I2CPutByte(b, addr, 0, &ack);
	I2CStop(b);

	return !ack;
}

I2CDevPtr
xf86CreateI2CDevRec(void) {
	return((I2CDevPtr)xcalloc(1, sizeof(I2CDevRec)));
}

void
xf86DestroyI2CDevRec(I2CDevPtr d) {
	I2CDevPtr *p;

	if(d == NULL)
		return;

	/* remove this from the list of active I2C busses. */
	for(p = &d->pI2CBus->FirstDev; *p != NULL; p = &(*p)->NextDev) {
		if(*p == d) {
			*p = (*p)->NextDev;
			break;
		}
	}

	if(d->scrnIndex >= 0)
		xf86DrvMsg(d->scrnIndex, X_DEFAULT, "I2C device \"%s:%s\" removed.\n",
			d->pI2CBus->BusName, d->DevName);
	else
		xf86Msg(X_DEFAULT, "I2C device \"%s:%s\" removed.\n",
			d->pI2CBus->BusName, d->DevName);

	xfree(d);
}

Bool
xf86I2CDevInit(I2CDevPtr d) {
	if(d->pI2CBus == NULL)
		return FALSE;

	if(d->i2caddr & 1)
		return FALSE;

	d->NextDev = d->pI2CBus->FirstDev;
	d->pI2CBus->FirstDev = d;

	if(d->scrnIndex >= 0)
		xf86DrvMsg(d->scrnIndex, X_DEFAULT, "I2C device \"%s:%s\" registered.\n",
			d->pI2CBus->BusName, d->DevName);
	else
		xf86Msg(X_DEFAULT, "I2C device \"%s:%s\" registered.\n",
			d->pI2CBus->BusName, d->DevName);

	return TRUE;
}


/* Input/Output done by slaves */
/* 8 bit address is taken fron the I2CDev */

/* Write N bytes and then read N bytes. Only one Stop signal. */
Bool
xf86I2CWriteNReadN(I2CDevPtr d,
                   I2CByte *WriteBuffer, int nWrite,
                   I2CByte *ReadBuffer,  int nRead) {

	I2CBusPtr b = d->pI2CBus;
	int i;
	int ack;

	if(nWrite > 0) {
		I2CStart(b);
		b->I2CPutByte(b, d->i2caddr, 0, &ack);

		for(i = 0; i < nWrite; i++)
			b->I2CPutByte(b, WriteBuffer[i], 0, &ack);
	}

	if(nRead > 0) {
		I2CStart(b);
		b->I2CPutByte(b, d->i2caddr|1, 0, &ack);

		for(i = 0; i < nRead; i++)
			b->I2CGetByte(b, &ReadBuffer[i], i==nRead-1);
	}

	I2CStop(b);

	return TRUE;
}

/* Write 1 byte */
Bool
xf86I2CWrite(I2CDevPtr d, I2CByte Out) {
	I2CBusPtr b = d->pI2CBus;
        int ack;

	I2CStart(b);
	b->I2CPutByte(b, d->i2caddr, 0, &ack);
	b->I2CPutByte(b, Out, 0, &ack);
	I2CStop(b);

	return TRUE;
}

/* Write 16 bits (short) */
Bool
xf86I2CWriteShort(I2CDevPtr d, unsigned short Out) {
	I2CBusPtr b = d->pI2CBus;
        int ack;

	I2CStart(b);
	b->I2CPutByte(b, d->i2caddr, 0, &ack);

	/* Allways MSB first ! */
	b->I2CPutByte(b, ((Out >> 8) & 0xff), 0, &ack);
	b->I2CPutByte(b, ( Out       & 0xff), 0, &ack);

	I2CStop(b);

	return TRUE;
}

/* Read  1 byte  */
Bool
xf86I2CRead(I2CDevPtr d, I2CByte *In) {
	I2CBusPtr b = d->pI2CBus;
        int ack;

	I2CStart(b);
	b->I2CPutByte(b, d->i2caddr|1, 0, &ack);
	b->I2CGetByte(b, In, 1);
	I2CStop(b);

	return TRUE;
}

/* Read  16 bits (short) */
Bool
xf86I2CReadShort(I2CDevPtr d, unsigned short *In) {
	I2CBusPtr b = d->pI2CBus;
        int ack;
	I2CByte x;

	I2CStart(b);
	b->I2CPutByte(b, d->i2caddr|1, 0, &ack);
	b->I2CGetByte(b, &x, 1);
	*In = (x << 8);
	b->I2CGetByte(b, &x, 1);
	*In |= x;
	I2CStop(b);

	return TRUE;
}

/* Write 1 byte and then read 1 byte */
Bool
xf86I2CWriteRead(I2CDevPtr d, unsigned char Out, unsigned char *In) {
	I2CBusPtr b = d->pI2CBus;
        int ack;

	I2CStart(b);
	b->I2CPutByte(b, d->i2caddr, 0, &ack);
	b->I2CPutByte(b, Out, 0, &ack);

	return xf86I2CRead(d, In);
}
