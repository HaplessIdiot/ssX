/* (c) Itai Nahshon */

/* $XFree86$ */

typedef unsigned char I2CByte;
typedef unsigned char I2CAddr;
typedef struct _I2CBusRec *I2CBusPtr;
typedef struct _I2CDevRec *I2CDevPtr;

/* I2C masters have to register themselve */
typedef struct _I2CBusRec {
    char	*BusName;
    int		scrnIndex;
    Bool	(*I2CPutBits)(I2CBusPtr m, int  scl, int  sda);
    Bool	(*I2CGetBits)(I2CBusPtr m, int *scl, int *sda);
    Bool	(*I2CPutByte)(I2CBusPtr m, I2CByte data, int wait_for_ack, int *ack);
    Bool	(*I2CGetByte)(I2CBusPtr m, I2CByte *data, Bool last);
    pointer	DriverPrivate;
    I2CDevPtr	FirstDev;
    I2CBusPtr	NextBus;
} I2CBusRec;

I2CBusPtr xf86CreateI2CBusRec(void);
void      xf86DestroyI2CBusRec(I2CBusPtr pI2CBus);
Bool      xf86I2CBusInit(I2CBusPtr pI2CBus);

/* I2C slave devices */
typedef struct _I2CDevRec {
    char        *DevName;
    int         scrnIndex;
    I2CAddr	i2caddr;
    I2CBusPtr	pI2CBus;
    I2CDevPtr	NextDev;
} I2CDevRec;

I2CDevPtr xf86CreateI2CDevRec(void);
void      xf86DestroyI2CDevRec(I2CDevPtr pI2CDev);
Bool      xf86I2CDevInit(I2CDevPtr pI2CDev);
I2CBusPtr xf86I2CFindBus(char *name);

/* Function for probing. Just send the address and return
   true if the device returns and ack. */
Bool	  xf86I2CTryAddress(I2CBusPtr pI2CBus, I2CAddr addr);

/* Input/Output done by slaves */
/* 8 bit address is taken fron the I2CDev */

/* Write N bytes and then read N bytes. Only one Stop signal. */
Bool      xf86I2CWriteNReadN(I2CDevPtr I2CDev,
                           I2CByte *WriteBuffer, int nWrite, 
                           I2CByte *ReadBuffer,  int nRead);

/* Write 1 byte */
Bool    xf86I2CWrite(I2CDevPtr I2CDev, I2CByte Out);
/* Write 2 bytes (short) */
Bool    xf86I2CWriteShort(I2CDevPtr I2CDev, unsigned short Out);
/* Read  1 byte  */
Bool    xf86I2CRead(I2CDevPtr I2CDev, I2CByte *In);
/* Read  2 bytes (short) */
Bool    xf86I2CReadShort(I2CDevPtr I2CDev, unsigned short *In);
/* Write 1 byte and then read 1 byte */
Bool    xf86I2CWriteRead(I2CDevPtr I2CDev, unsigned char Out, unsigned char *In);
