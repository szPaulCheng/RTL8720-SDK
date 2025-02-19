/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */

#include "PinNames.h"
#include "basic_types.h"
#include "diag.h"
#include "pinmap.h"

struct i2c_int {
	uint32_t i2c_idx;
	I2C_TypeDef * I2Cx;
	u8* pbuf;
	u32 datalength;
};
typedef struct i2c_int  i2c_t;

/*I2C pin location:
* I2C0:
*	  - S0:  PA_25(SCL)/PA_26(SDA).
*	         PB_5(SCL)/PB_6(SDA).
*/

//I2C1_SEL S0
#define MBED_I2C_MTR_SDA    PA_26
#define MBED_I2C_MTR_SCL    PA_25

//I2C0_SEL S0
#define MBED_I2C_SLV_SDA    PA_26
#define MBED_I2C_SLV_SCL    PA_25

#define MBED_I2C_SLAVE_ADDR0    0x23
#define MBED_I2C_BUS_CLK        100000  //hz

#define I2C_DATA_LENGTH         127
uint8_t	i2cdatasrc[I2C_DATA_LENGTH];
uint8_t	i2cdatadst[I2C_DATA_LENGTH];
uint8_t	i2cdatardsrc[I2C_DATA_LENGTH];
uint8_t	i2cdatarddst[I2C_DATA_LENGTH];
I2C_InitTypeDef I2CInitDat[2];

#define I2C_MASTER_DEVICE

/*if defined, master send, slave read mode.
 else master read slave send mode.*/
#define MASTER_SEND

i2c_t   i2cmaster;
i2c_t   i2cslave;
u32   length=128;

static VOID I2CISRHandleTxEmpty(IN  i2c_t   *obj)
{
	u8 I2CStop = 0;

	/* To check I2C master TX data length. If all the data are transmitted,
	mask all the interrupts and invoke the user callback */
	if (!obj->datalength) {

		while(0 == I2C_CheckFlagState(obj->I2Cx, BIT_IC_STATUS_TFE));

		/* I2C Disable TX Related Interrupts */
		I2C_INTConfig(obj->I2Cx, (BIT_IC_INTR_MASK_M_TX_ABRT  | 
			BIT_IC_INTR_MASK_M_TX_EMPTY |
			BIT_IC_INTR_MASK_M_TX_OVER),
			DISABLE);

		/* Clear all I2C pending interrupts */
		I2C_ClearAllINT(obj->I2Cx);
	}

	if (obj->datalength > 0) {
		/* Check I2C TX FIFO status. If it's not full, one byte data will be written into it. */
		if (I2C_CheckFlagState(obj->I2Cx, BIT_IC_STATUS_TFNF)) {
			if (obj->datalength == 1) 	 I2CStop   = 1;
			I2C_MasterSend(obj->I2Cx, obj->pbuf,0,I2CStop, 0);
			obj->pbuf++;
			obj->datalength--;
		}
	}
}

static VOID I2CISRHandleRdReq(IN  i2c_t   *obj)
{
	if(!obj->datalength){
		while(0 == I2C_CheckFlagState(obj->I2Cx, BIT_IC_STATUS_TFE));

		/* Disable I2C TX Related Interrupts */
		I2C_INTConfig(obj->I2Cx, (BIT_IC_INTR_MASK_M_TX_ABRT |
			BIT_IC_INTR_MASK_M_TX_OVER |
			BIT_IC_INTR_MASK_M_RX_DONE |
			BIT_IC_INTR_MASK_M_RD_REQ),
			DISABLE);

		I2C_ClearAllINT(obj->I2Cx);
	}else {
		/* I2C Slave transmits data to Master. If the TX FIFO is NOT full,
		write one byte from slave TX buffer to TX FIFO. */    
		if(I2C_CheckFlagState(obj->I2Cx, BIT_IC_STATUS_TFNF)) {
			I2C_SlaveSend(obj->I2Cx, *obj->pbuf);
			obj->pbuf++;
			obj->datalength--;
		}
	}
}

static VOID I2CISRHandleRxFull(IN  i2c_t   *obj)
{
#ifdef I2C_MASTER_DEVICE

	if (I2C_CheckFlagState(obj->I2Cx, (BIT_IC_STATUS_RFNE | BIT_IC_STATUS_RFF))) {

		*(obj->pbuf) = I2C_ReceiveData(obj->I2Cx);

		obj->pbuf++;
		obj->datalength--;

	}

	if(!obj->datalength){
		I2C_INTConfig(obj->I2Cx, (BIT_IC_INTR_MASK_M_RX_FULL |
			BIT_IC_INTR_MASK_M_RX_OVER |
			BIT_IC_INTR_MASK_M_RX_UNDER|
			BIT_IC_INTR_MASK_M_TX_ABRT),
			DISABLE);
		I2C_ClearAllINT(obj->I2Cx);
	}else {
		if (I2C_CheckFlagState(obj->I2Cx, BIT_IC_STATUS_TFNF)) {
			if(obj->datalength==1){
				I2C_MasterSend(obj->I2Cx, obj->pbuf,0x1,0x1, 0);
			}else {
				I2C_MasterSend(obj->I2Cx, obj->pbuf,0x1,0x0, 0);
			}
		}
	}

#else
	/* To check I2C master RX data length. If all the data are received,
	mask all the interrupts and invoke the user callback.
	Otherwise, if there is data in the RX FIFO and move the data from RX 
	FIFO to user data buffer*/
	/* Receive data till the RX buffer data length is zero */

	if (obj->datalength > 0) {
		if (I2C_CheckFlagState(obj->I2Cx, (BIT_IC_STATUS_RFNE | BIT_IC_STATUS_RFF))) {                    
			*(obj->pbuf) = I2C_ReceiveData(obj->I2Cx);
			 obj->pbuf++;
			obj->datalength--;
		}
	}

	/* All data are received. Mask all related interrupts. */
	if (!obj->datalength) {
		/*I2C Disable RX Related Interrupts*/
		I2C_INTConfig(obj->I2Cx, (BIT_IC_INTR_MASK_M_RX_FULL |
			BIT_IC_INTR_MASK_M_RX_OVER |
			BIT_IC_INTR_MASK_M_RX_UNDER),
			DISABLE);
		}
#endif
}


VOID I2CISRHandle(IN  VOID *Data)
{
	i2c_t   *obj = (i2c_t *) Data;

	obj->I2Cx = I2C_DEV_TABLE[0].I2Cx;

	u32 intr_status = I2C_GetINT(obj->I2Cx);

	/* I2C ADDR MATCH Intr*/
	if (intr_status & BIT_IC_INTR_STAT_R_ADDR_1_MATCH) {
		/* Clear I2C interrupt */
		I2C_WakeUp(obj->I2Cx);
	}
	if (intr_status & BIT_IC_INTR_STAT_R_ADDR_2_MATCH) {
		/* Clear I2C interrupt */
		I2C_WakeUp(obj->I2Cx);
	}

	/* I2C STOP DET Intr */
	if (intr_status & BIT_IC_INTR_STAT_R_STOP_DET) {

		/* Clear I2C interrupt */
		I2C_ClearINT(obj->I2Cx, BIT_IC_INTR_STAT_R_STOP_DET);
	}

	/* I2C Activity Intr */
	if (intr_status & BIT_IC_INTR_STAT_R_ACTIVITY) {

		/* Clear I2C interrupt */
		I2C_ClearINT(obj->I2Cx, BIT_IC_INTR_STAT_R_ACTIVITY);
	}

	/* I2C RX Done Intr */
	if (intr_status & BIT_IC_INTR_STAT_R_RX_DONE) {
		//slave-transmitter and master not ACK it, This occurs on the last byte of
		//the transmission, indicating that the transmission is done. 
		I2C_ClearINT(obj->I2Cx, BIT_IC_INTR_STAT_R_RX_DONE);
	}

	/* I2C TX Abort Intr */
	if (intr_status & BIT_IC_INTR_STAT_R_TX_ABRT) {
		DBG_8195A("TX_ABRT\n");
		I2C_ClearINT(obj->I2Cx, BIT_IC_INTR_STAT_R_TX_ABRT);
	}

	/* I2C TX Empty Intr */
	if (intr_status & BIT_IC_INTR_STAT_R_TX_EMPTY) {
		I2CISRHandleTxEmpty(obj);
	}

	if (intr_status & BIT_IC_INTR_STAT_R_RD_REQ) {
		I2C_ClearINT(obj->I2Cx, BIT_IC_INTR_STAT_R_RD_REQ);
		I2CISRHandleRdReq(obj);
	}

	/* I2C TX Over Run Intr */
	if (intr_status & BIT_IC_INTR_STAT_R_TX_OVER) {
		I2C_ClearINT(obj->I2Cx, BIT_IC_INTR_STAT_R_TX_OVER);
	}

	/* I2C RX Full Intr */
	if (intr_status & BIT_IC_INTR_STAT_R_RX_FULL ||
		intr_status & BIT_IC_INTR_STAT_R_RX_OVER) {
		
		/*I2C RX Over Run Intr*/
		if (intr_status & BIT_IC_INTR_STAT_R_RX_OVER) {
			I2C_ClearINT(obj->I2Cx, BIT_IC_INTR_STAT_R_RX_OVER);
		}
		I2CISRHandleRxFull(obj);
	}

	/*I2C RX Under Run Intr*/
	if (intr_status & BIT_IC_INTR_STAT_R_RX_UNDER) {
		I2C_ClearINT(obj->I2Cx, BIT_IC_INTR_STAT_R_RX_UNDER);
	}
}


HAL_Status RtkI2CInit(IN  i2c_t  * obj1)
{
	i2c_t *obj= obj1;

	obj->I2Cx = I2C_DEV_TABLE[0].I2Cx;

	InterruptRegister((IRQ_FUN) I2CISRHandle, 6, (u32) (obj), 3); /*IrqNum=6,I2C*/
	InterruptEn(6, 3);

	/* I2C HAL Initialization */
	I2C_Init(obj->I2Cx, &I2CInitDat[0]);

	/* I2C Enable Module */
	I2C_Cmd(obj->I2Cx, ENABLE);

	return HAL_OK;
}


#ifdef MASTER_SEND
void main(void)
{
	int     i2clocalcnt;

	// prepare for transmission
	_memset(&i2cdatasrc[0], 0x00, I2C_DATA_LENGTH);
	_memset(&i2cdatadst[0], 0x00, I2C_DATA_LENGTH);

	for (i2clocalcnt=0; i2clocalcnt < length; i2clocalcnt++){
		i2cdatasrc[i2clocalcnt] = i2clocalcnt+0x2;
	}

#ifdef I2C_MASTER_DEVICE
	DBG_8195A("Slave addr=%x\n",MBED_I2C_SLAVE_ADDR0);
	_memset(&i2cmaster, 0x00, sizeof(i2c_t));
	i2c_init(&i2cmaster, MBED_I2C_MTR_SDA ,MBED_I2C_MTR_SCL);  
	i2c_frequency(&i2cmaster,MBED_I2C_BUS_CLK);
	I2CInitDat[i2cmaster.i2c_idx].I2CAckAddr =  MBED_I2C_SLAVE_ADDR0;
	RtkI2CInit(&i2cmaster);
	
	// Master write - Slave read
	DBG_8195A("\r\nMaster write int mode>>>\n");	
	i2cmaster.pbuf=i2cdatasrc;
	i2cmaster.datalength=length;
	I2C_INTConfig(i2cmaster.I2Cx, (BIT_IC_INTR_MASK_M_TX_ABRT  | 
		BIT_IC_INTR_MASK_M_TX_EMPTY |
		BIT_IC_INTR_MASK_M_TX_OVER),
		ENABLE);

#else //I2C_SLAVE_DEVICE
	DBG_8195A("Slave addr=%x\n",MBED_I2C_SLAVE_ADDR0);
	_memset(&i2cslave, 0x00, sizeof(i2c_t));
	i2c_init(&i2cslave, MBED_I2C_SLV_SDA ,MBED_I2C_SLV_SCL);
	i2c_frequency(&i2cslave,MBED_I2C_BUS_CLK);
	i2c_slave_address(&i2cslave, 0, MBED_I2C_SLAVE_ADDR0, 0xFF);
	I2CInitDat[i2cslave.i2c_idx].I2CMaster = I2C_SLAVE_MODE;
	RtkI2CInit(&i2cslave);

	// Master write - Slave read
	DBG_8195A("\r\nSlave read>>>\n");
	i2cslave.pbuf=i2cdatadst;
	i2cslave.datalength=length;
	I2C_INTConfig(i2cslave.I2Cx, (BIT_IC_INTR_MASK_M_RX_FULL |
		BIT_IC_INTR_MASK_M_RX_OVER |
		BIT_IC_INTR_MASK_M_RX_UNDER),
		ENABLE);
	
#endif // #ifdef I2C_SLAVE_DEVICE

	while(1){;}
}

#else
void main(void)
{
	int     i2clocalcnt;

	// prepare for transmission
	_memset(&i2cdatardsrc[0], 0x00, I2C_DATA_LENGTH);
	_memset(&i2cdatarddst[0], 0x00, I2C_DATA_LENGTH);

	for (i2clocalcnt=0; i2clocalcnt < length; i2clocalcnt++){
		i2cdatardsrc[i2clocalcnt] = 128-i2clocalcnt;
	}
	
#ifdef I2C_MASTER_DEVICE
	DBG_8195A("Slave addr=%x\n",MBED_I2C_SLAVE_ADDR0);
	_memset(&i2cmaster, 0x00, sizeof(i2c_t));
	i2c_init(&i2cmaster, MBED_I2C_MTR_SDA ,MBED_I2C_MTR_SCL);  
	i2c_frequency(&i2cmaster,MBED_I2C_BUS_CLK);
	I2CInitDat[i2cmaster.i2c_idx].I2CAckAddr =  MBED_I2C_SLAVE_ADDR0;
	RtkI2CInit(&i2cmaster);

	// Master write - Slave read
	DBG_8195A("\r\nMaster read int mode>>>\n");	
	i2cmaster.pbuf=i2cdatarddst;
	i2cmaster.datalength=length;
	I2C_INTConfig(i2cmaster.I2Cx, (BIT_IC_INTR_MASK_M_RX_FULL |
		BIT_IC_INTR_MASK_M_RX_OVER |
		BIT_IC_INTR_MASK_M_RX_UNDER|BIT_IC_INTR_MASK_M_TX_ABRT),
		ENABLE);

	if(i2cmaster.datalength>1){
		I2C_MasterSend(i2cmaster.I2Cx, i2cmaster.pbuf,0x1,0, 0);
		i2cmaster.datalength --;
	}else{
		I2C_MasterSend(i2cmaster.I2Cx, i2cmaster.pbuf, 0x1, 0x1, 0);
	}
	
#else //I2C_SLAVE_DEVICE
	DBG_8195A("Slave addr=%x\n",MBED_I2C_SLAVE_ADDR0);
	_memset(&i2cslave, 0x00, sizeof(i2c_t));
	i2c_init(&i2cslave, MBED_I2C_SLV_SDA ,MBED_I2C_SLV_SCL);
	i2c_frequency(&i2cslave,MBED_I2C_BUS_CLK);
	i2c_slave_address(&i2cslave, 0, MBED_I2C_SLAVE_ADDR0, 0xFF);
	I2CInitDat[i2cslave.i2c_idx].I2CMaster = I2C_SLAVE_MODE;
	RtkI2CInit(&i2cslave);

	// Master write - Slave read
	DBG_8195A("\r\nSlave send>>>\n");
	i2cslave.pbuf=i2cdatardsrc;
	i2cslave.datalength=length;
	I2C_INTConfig(i2cslave.I2Cx, (BIT_IC_INTR_MASK_M_TX_ABRT |
		BIT_IC_INTR_MASK_M_TX_OVER |
		BIT_IC_INTR_MASK_M_RX_DONE |
		BIT_IC_INTR_MASK_M_RD_REQ),
		ENABLE);
	
#endif // #ifdef I2C_SLAVE_DEVICE

	while(1){;}
}
#endif
