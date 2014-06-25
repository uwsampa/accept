/******************************************************************************
*
* (c) Copyright 2010-12 Xilinx, Inc. All rights reserved.
*
* This file contains confidential and proprietary information of Xilinx, Inc.
* and is protected under U.S. and international copyright and other
* intellectual property laws.
*
* DISCLAIMER
* This disclaimer is not a license and does not grant any rights to the
* materials distributed herewith. Except as otherwise provided in a valid
* license issued to you by Xilinx, and to the maximum extent permitted by
* applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
* FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
* IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
* MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
* and (2) Xilinx shall not be liable (whether in contract or tort, including
* negligence, or under any other theory of liability) for any loss or damage
* of any kind or nature related to, arising under or in connection with these
* materials, including for any direct, or any indirect, special, incidental,
* or consequential loss or damage (including loss of data, profits, goodwill,
* or any type of loss or damage suffered as a result of any action brought by
* a third party) even if such damage or loss was reasonably foreseeable or
* Xilinx had been advised of the possibility of the same.
*
* CRITICAL APPLICATIONS
* Xilinx products are not designed or intended to be fail-safe, or for use in
* any application requiring fail-safe performance, such as life-support or
* safety devices or systems, Class III medical devices, nuclear facilities,
* applications related to the deployment of airbags, or any other applications
* that could lead to death, personal injury, or severe property or
* environmental damage (individually and collectively, "Critical
* Applications"). Customer assumes the sole risk and liability of any use of
* Xilinx products in Critical Applications, subject only to applicable laws
* and regulations governing limitations on product liability.
*
* THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
* AT ALL TIMES.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file xqspips.h
*
* This file contains the implementation of the XQspiPs driver. It works for
* both the master and slave mode. User documentation for the driver functions
* is contained in this file in the form of comment blocks at the front of each
* function.
*
* A QSPI device connects to an QSPI bus through a 4-wire serial interface.
* The QSPI bus is a full-duplex, synchronous bus that facilitates communication
* between one master and one slave. The device is always full-duplex,
* which means that for every byte sent, one is received, and vice-versa.
* The master controls the clock, so it can regulate when it wants to
* send or receive data. The slave is under control of the master, it must
* respond quickly since it has no control of the clock and must send/receive
* data as fast or as slow as the master does.
*
* <b> Linear Mode </b>
* The Linear Quad-SPI Controller extends the existing Quad-SPI Controller’s
* functionality by adding a linear addressing scheme that allows the SPI flash
* memory subsystem to behave like a typical ROM device.  The new feature hides
* the normal SPI protocol from a master reading from the SPI flash memory. The
* feature improves both the user friendliness and the overall read memory
* throughput over that of the current Quad-SPI Controller by lessening the
* amount of software overheads required and by the use of the faster AXI
* interface.
*
* <b>Initialization & Configuration</b>
*
* The XQspiPs_Config structure is used by the driver to configure itself. This
* configuration structure is typically created by the tool-chain based on HW
* build properties.
*
* To support multiple runtime loading and initialization strategies employed by
* various operating systems, the driver instance can be initialized in the
* following way:
*	- XQspiPs_LookupConfig(DeviceId) - Use the devide identifier to find
*	  static configuration structure defined in xqspips_g.c. This is setup
*	  by the tools. For some operating systems the config structure will be
*	  initialized by the software and this call is not needed.
*	- XQspiPs_CfgInitialize(InstancePtr, CfgPtr, EffectiveAddr) - Uses a
*	  configuration structure provided by the caller. If running in a system
*	  with address translation, the provided virtual memory base address
*	  replaces the physical address present in the configuration structure.
*
* <b>Multiple Masters</b>
*
* More than one master can exist, but arbitration is the responsibility of
* the higher layer software. The device driver does not perform any type of
* arbitration.
*
* <b>Slave Mode</b>
* The QSPI device supports slave mode, but only SPI protocol transactions will
* be supported in slave mode.
*
*
* <b>Interrupts</b>
*
* The user must connect the interrupt handler of the driver,
* XQspiPs_InterruptHandler, to an interrupt system such that it will be
* called when an interrupt occurs. This function does not save and restore
* the processor context such that the user must provide this processing.
*
* The driver handles the following interrupts:
* - Data Transmit Register/FIFO Underflow
* - Data Receive Register/FIFO Not Empty
* - Data Transmit Register/FIFO Overwater
* - Data Receive Register/FIFO Overrun
*
* The Data Transmit Register/FIFO Overwater interrupt -- indicates that the
* QSPI device has transmitted the data available to transmit, and now its data
* register and FIFO is ready to accept more data. The driver uses this
* interrupt to indicate progress while sending data.  The driver may have
* more data to send, in which case the data transmit register and FIFO is
* filled for subsequent transmission. When this interrupt arrives and all
* the data has been sent, the driver invokes the status callback with a
* value of XST_SPI_TRANSFER_DONE to inform the upper layer software that
* all data has been sent.
*
* The Data Transmit Register/FIFO Underflow interrupt -- indicates that,
* as slave, the QSPI device was required to transmit but there was no data
* available to transmit in the transmit register (or FIFO). This may not
* be an error if the master is not expecting data. But in the case where
* the master is expecting data, this serves as a notification of such a
* condition. The driver reports this condition to the upper layer
* software through the status handler.
*
* The Data Receive Register/FIFO Overrun interrupt -- indicates that the QSPI
* device received data and subsequently dropped the data because the data
* receive register and FIFO was full. The driver reports this condition to the
* upper layer software through the status handler. This likely indicates a
* problem with the higher layer protocol, or a problem with the slave
* performance.
*
*
* <b>Polled Operation</b>
*
* Transfer in polled mode is supported through a separate interface function
* XQspiPs_PolledTransfer(). Unlike the transfer function in the interrupt mode,
* this function blocks until all data has been sent/received.
*
* <b>Device Busy</b>
*
* Some operations are disallowed when the device is busy. The driver tracks
* whether a device is busy. The device is considered busy when a data transfer
* request is outstanding, and is considered not busy only when that transfer
* completes (or is aborted with a mode fault error). This applies to both
* master and slave devices.
*
* <b>Device Configuration</b>
*
* The device can be configured in various ways during the FPGA implementation
* process. Configuration parameters are stored in the xqspips_g.c file or
* passed in via XQspiPs_CfgInitialize(). A table is defined where each entry
* contains configuration information for an QSPI device, including the base
* address for the device.
*
* <b>RTOS Independence</b>
*
* This driver is intended to be RTOS and processor independent.  It works with
* physical addresses only.  Any needs for dynamic memory management, threads or
* thread mutual exclusion, virtual memory, or cache control must be satisfied
* by the layer above this driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who Date     Changes
* ----- --- -------- -----------------------------------------------
* 1.00a sdm 11/25/10 First release, based on the PS SPI driver.
* 1.01a sdm 11/22/11 Added TCL file for generating QSPI parameters
*		     in xparameters.h
* 2.00a kka 07/25/12 Added a few register defines for CR 670297
* 		     Removed code related to mode fault for CR 671468
*		     The XQspiPs_SetSlaveSelect has been modified to remove
*		     the argument of the slave select as the QSPI controller
*		     only supports one slave.
* 		     XQspiPs_GetSlaveSelect API has been removed
* 		     Added a flag ShiftReadData to the instance structure
*.		     and is used in the XQspiPs_GetReadData API.
*		     The ShiftReadData Flag indicates whether the data
*		     read from the Rx FIFO needs to be shifted
*		     in cases where the data is less than 4  bytes
* 		     Removed the selection for the following options:
*		     Master mode (XQSPIPS_MASTER_OPTION) and
*		     Flash interface mode (XQSPIPS_FLASH_MODE_OPTION) option
*		     as the QSPI driver supports the Master mode
*		     and Flash Interface mode and doesnot support
*		     Slave mode or the legacy mode.
*		     Modified the XQspiPs_PolledTransfer and XQspiPs_Transfer
*		     APIs so that the last argument (IsInst) specifying whether
*		     it is instruction or data has been removed. The first byte
*		     in the SendBufPtr argument of these APIs specify the
*		     instruction to be sent to the Flash Device.
*		     This version of the driver fixes CRs 670197/663787/
*		     670297/671468.
* 		     Added the option for setting the Holdb_dr bit in the
*		     configuration options, XQSPIPS_HOLD_B_DRIVE_OPTION
*		     is the option to be used for setting this bit in the
*		     configuration regsiter.
*		     The XQspiPs_PolledTransfer function has been updated
*		     to fill the data to fifo depth.
*
* </pre>
*
******************************************************************************/
#ifndef XQSPIPS_H		/* prevent circular inclusions */
#define XQSPIPS_H		/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/

#include "xstatus.h"
#include "xqspips_hw.h"
#include <string.h>

/************************** Constant Definitions *****************************/

/** @name Configuration options
 *
 * The following options are supported to enable/disable certain features of
 * an QSPI device.  Each of the options is a bit mask, so more than one may be
 * specified.
 *
 *
 * The <b>Active Low Clock option</b> configures the device's clock polarity.
 * Setting this option means the clock is active low and the SCK signal idles
 * high. By default, the clock is active high and SCK idles low.
 *
 * The <b>Clock Phase option</b> configures the QSPI device for one of two
 * transfer formats.  A clock phase of 0, the default, means data is valid on
 * the first SCK edge (rising or falling) after the slave select (SS) signal
 * has been asserted. A clock phase of 1 means data is valid on the second SCK
 * edge (rising or falling) after SS has been asserted.
 *
 *
 * The <b>QSPI Force Slave Select option</b> is used to enable manual control of
 * the slave select signal.
 * 0: The SPI_SS signal is controlled by the QSPI controller during
 * transfers. (Default)
 * 1: The SPI_SS signal is forced active (driven low) regardless of any
 * transfers in progress.
 *
 * NOTE: The driver will handle setting and clearing the Slave Select when
 * the user sets the "FORCE_SSELECT_OPTION". Using this option will allow the
 * QSPI clock to be set to a faster speed. If the QSPI clock is too fast, the
 * processor cannot empty and refill the FIFOs before the TX FIFO is empty
 * When the QSPI hardware is controlling the Slave Select signals, this
 * will cause slave to be de-selected and terminate the transfer.
 *
 * @{
 */
#define XQSPIPS_CLK_ACTIVE_LOW_OPTION	0x2  /**< Active Low Clock option */
#define XQSPIPS_CLK_PHASE_1_OPTION	0x4  /**< Clock Phase one option */
#define XQSPIPS_FORCE_SSELECT_OPTION	0x10 /**< Force Slave Select */
#define XQSPIPS_MANUAL_START_OPTION	0x20 /**< Manual Start enable */
#define XQSPIPS_LQSPI_MODE_OPTION	0x80 /**< Linear QPSI mode */
#define XQSPIPS_HOLD_B_DRIVE_OPTION	0x100 /**< Drive HOLD_B Pin */
/*@}*/


/** @name QSPI Clock Prescaler options
 * The QSPI Clock Prescaler Configuration bits are used to program master mode
 * bit rate. The bit rate can be programmed in divide-by-two decrements from
 * pclk/2 to pclk/256.
 *
 * @{
 */
#define XQSPIPS_CLK_PRESCALE_2		0x00 /**< PCLK/2 Prescaler */
#define XQSPIPS_CLK_PRESCALE_4		0x01 /**< PCLK/4 Prescaler */
#define XQSPIPS_CLK_PRESCALE_8		0x02 /**< PCLK/8 Prescaler */
#define XQSPIPS_CLK_PRESCALE_16		0x03 /**< PCLK/16 Prescaler */
#define XQSPIPS_CLK_PRESCALE_32		0x04 /**< PCLK/32 Prescaler */
#define XQSPIPS_CLK_PRESCALE_64		0x05 /**< PCLK/64 Prescaler */
#define XQSPIPS_CLK_PRESCALE_128	0x06 /**< PCLK/128 Prescaler */
#define XQSPIPS_CLK_PRESCALE_256	0x07 /**< PCLK/256 Prescaler */

/*@}*/


/** @name Callback events
 *
 * These constants specify the handler events that are passed to
 * a handler from the driver.  These constants are not bit masks such that
 * only one will be passed at a time to the handler.
 *
 * @{
 */
#define XQSPIPS_EVENT_TRANSFER_DONE	2 /**< Transfer done */
#define XQSPIPS_EVENT_TRANSMIT_UNDERRUN 3 /**< TX FIFO empty */
#define XQSPIPS_EVENT_RECEIVE_OVERRUN	4 /**< Receive data loss because
						RX FIFO full */
/*@}*/

/** @name Flash commands
 *
 * The following constants define most of the commands supported by flash
 * devices. Users can add more commands supported by the flash devices
 *
 * @{
 */
#define	XQSPIPS_FLASH_OPCODE_WRSR	0x01 /* Write status register */
#define	XQSPIPS_FLASH_OPCODE_PP		0x02 /* Page program */
#define	XQSPIPS_FLASH_OPCODE_NORM_READ	0x03 /* Normal read data bytes */
#define	XQSPIPS_FLASH_OPCODE_WRDS	0x04 /* Write disable */
#define	XQSPIPS_FLASH_OPCODE_RDSR1	0x05 /* Read status register 1 */
#define	XQSPIPS_FLASH_OPCODE_WREN	0x06 /* Write enable */
#define	XQSPIPS_FLASH_OPCODE_FAST_READ	0x0B /* Fast read data bytes */
#define	XQSPIPS_FLASH_OPCODE_BE_4K	0x20 /* Erase 4KiB block */
#define	XQSPIPS_FLASH_OPCODE_RDSR2	0x35 /* Read status register 2 */
#define	XQSPIPS_FLASH_OPCODE_DUAL_READ	0x3B /* Dual read data bytes */
#define	XQSPIPS_FLASH_OPCODE_BE_32K	0x52 /* Erase 32KiB block */
#define	XQSPIPS_FLASH_OPCODE_QUAD_READ	0x6B /* Quad read data bytes */
#define	XQSPIPS_FLASH_OPCODE_ERASE_SUS	0x75 /* Erase suspend */
#define	XQSPIPS_FLASH_OPCODE_ERASE_RES	0x7A /* Erase resume */
#define	XQSPIPS_FLASH_OPCODE_RDID	0x9F /* Read JEDEC ID */
#define	XQSPIPS_FLASH_OPCODE_BE		0xC7 /* Erase whole flash block */
#define	XQSPIPS_FLASH_OPCODE_SE		0xD8 /* Sector erase (usually 64KB)*/

/*@}*/

/** @name QSPI Instruction/Data options
 *
 * The following constants define whether a QSPI transfer includes a command or
 * is just a data transfer.
 *
 * @{
 */
#define	XQSPIPS_IS_DATA		0x00 /* Data-only transfer */
#define	XQSPIPS_IS_INST		0x01 /* The fist bytes in a transfer is
						instruction */

/*@}*/

/**************************** Type Definitions *******************************/
/**
 * The handler data type allows the user to define a callback function to
 * handle the asynchronous processing for the QSPI device.  The application
 * using this driver is expected to define a handler of this type to support
 * interrupt driven mode.  The handler executes in an interrupt context, so
 * only minimal processing should be performed.
 *
 * @param	CallBackRef is the callback reference passed in by the upper
 *		layer when setting the callback functions, and passed back to
 *		the upper layer when the callback is invoked. Its type is
 *		not important to the driver, so it is a void pointer.
 * @param 	StatusEvent holds one or more status events that have occurred.
 *		See the XQspiPs_SetStatusHandler() for details on the status
 *		events that can be passed in the callback.
 * @param	ByteCount indicates how many bytes of data were successfully
 *		transferred.  This may be less than the number of bytes
 *		requested if the status event indicates an error.
 */
typedef void (*XQspiPs_StatusHandler) (void *CallBackRef, u32 StatusEvent,
					unsigned ByteCount);

/**
 * This typedef contains configuration information for the device.
 */
typedef struct {
	u16 DeviceId;		/**< Unique ID  of device */
	u32 BaseAddress;	/**< Base address of the device */
	u32 InputClockHz;	/**< Input clock frequency */
} XQspiPs_Config;

/**
 * The XQspiPs driver instance data. The user is required to allocate a
 * variable of this type for every QSPI device in the system. A pointer
 * to a variable of this type is then passed to the driver API functions.
 */
typedef struct {
	XQspiPs_Config Config;	 /**< Configuration structure */
	u32 IsReady;		 /**< Device is initialized and ready */

	u8 *SendBufferPtr;	 /**< Buffer to send (state) */
	u8 *RecvBufferPtr;	 /**< Buffer to receive (state) */
	int RequestedBytes;	 /**< Number of bytes to transfer (state) */
	int RemainingBytes;	 /**< Number of bytes left to transfer(state) */
	u32 IsBusy;		 /**< A transfer is in progress (state) */
	XQspiPs_StatusHandler StatusHandler;
	void *StatusRef;  	 /**< Callback reference for status handler */
	u32 ShiftReadData;	 /**<  Flag to indicate whether the data
				   *   read from the Rx FIFO needs to be shifted
				   *   in cases where the data is less than 4
				   *   bytes
				   */

} XQspiPs;

/***************** Macros (Inline Functions) Definitions *********************/

/****************************************************************************/
/**
*
* Set the contents of the slave idle count register.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
* @param	RegisterValue is the value to be writen, valid values are
*		0-65535.
*
* @return	None
*
* @note
* C-Style signature:
*	void XQspiPs_SetSlaveIdle(XQspiPs *InstancePtr, u32 RegisterValue)
*
*****************************************************************************/
#define XQspiPs_SetSlaveIdle(InstancePtr, RegisterValue)	\
	XQspiPs_Out32(((InstancePtr)->Config.BaseAddress) + 	\
			XQSPIPS_SICR_OFFSET, (RegisterValue))

/****************************************************************************/
/**
*
* Get the contents of the slave idle count register. Use the XQSPIPS_SICR_*
* constants defined in xqspips_hw.h to interpret the bit-mask returned.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
*
* @return	A 32-bit value representing the contents of the SIC register.
*
* @note		C-Style signature:
*		u32 XQspiPs_GetSlaveIdle(XQspiPs *InstancePtr)
*
*****************************************************************************/
#define XQspiPs_GetSlaveIdle(InstancePtr)				\
	XQspiPs_In32(((InstancePtr)->Config.BaseAddress) + 		\
	XQSPIPS_SICR_OFFSET)

/****************************************************************************/
/**
*
* Set the contents of the transmit FIFO watermark register.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
* @param	RegisterValue is the value to be writen, valid values are 0-15.
*
* @return	None.
*
* @note
* C-Style signature:
*	void XQspiPs_GetTXWatermark(XQspiPs *InstancePtr, u32 RegisterValue)
*
*****************************************************************************/
#define XQspiPs_SetTXWatermark(InstancePtr, RegisterValue)		\
	XQspiPs_Out32(((InstancePtr)->Config.BaseAddress) + 		\
			XQSPIPS_TXWR_OFFSET, (RegisterValue))

/****************************************************************************/
/**
*
* Get the contents of the transmit FIFO watermark register.
* Use the XQSPIPS_TXWR_* constants defined xqspips_hw.h to interpret
* the bit-mask returned.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
*
* @return	A 32-bit value representing the contents of the TXWR register.
*
* @note		C-Style signature:
*		u32 XQspiPs_GetTXWatermark(u32 *InstancePtr)
*
*****************************************************************************/
#define XQspiPs_GetTXWatermark(InstancePtr)				\
	XQspiPs_In32((InstancePtr->Config.BaseAddress) + XQSPIPS_TXWR_OFFSET)

/****************************************************************************/
/**
*
* Enable the device and uninhibit master transactions.
*
* @param	BaseAddress is the base address of the device
*
* @return	None.
*
* @note		C-Style signature:
*		void XQspiPs_Enable(u32 BaseAddress)
*
*****************************************************************************/
#define XQspiPs_Enable(BaseAddress)					\
	XQspiPs_Out32((BaseAddress) + XQSPIPS_ER_OFFSET,		\
			XQSPIPS_ER_ENABLE_MASK)

/****************************************************************************/
/**
*
* Disable the device.
*
* @param	BaseAddress is the  base address of the device.
*
* @return	None.
*
* @note		C-Style signature:
*		void XQspiPs_Disable(u32 BaseAddress)
*
*****************************************************************************/
#define XQspiPs_Disable(BaseAddress)					\
	XQspiPs_Out32((BaseAddress) + XQSPIPS_ER_OFFSET, 0)

/****************************************************************************/
/**
*
* Set the contents of the Linear QSPI Configuration register.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
* @param	RegisterValue is the value to be writen to the Linear QSPI
*		configuration register.
*
* @return	None.
*
* @note
* C-Style signature:
*	void XQspiPs_SetLqspiConfigReg(XQspiPs *InstancePtr,
*					u32 RegisterValue)
*
*****************************************************************************/
#define XQspiPs_SetLqspiConfigReg(InstancePtr, RegisterValue)		\
	XQspiPs_Out32(((InstancePtr)->Config.BaseAddress) +		\
			XQSPIPS_LQSPI_CR_OFFSET, (RegisterValue))

/****************************************************************************/
/**
*
* Get the contents of the Linear QSPI Configuration register.
*
* @param	InstancePtr is a pointer to the XQspiPs instance.
*
* @return	A 32-bit value representing the contents of the LQSPI Config
*		register.
*
* @note		C-Style signature:
*		u32 XQspiPs_GetLqspiConfigReg(u32 *InstancePtr)
*
*****************************************************************************/
#define XQspiPs_GetLqspiConfigReg(InstancePtr)				\
	XQspiPs_In32((InstancePtr->Config.BaseAddress) +		\
			XQSPIPS_LQSPI_CR_OFFSET)

/************************** Function Prototypes ******************************/

/*
 * Initialization function, implemented in xqspips_sinit.c
 */
XQspiPs_Config *XQspiPs_LookupConfig(u16 DeviceId);

/*
 * Functions implemented in xqspips.c
 */
int XQspiPs_CfgInitialize(XQspiPs *InstancePtr, XQspiPs_Config * Config,
			   u32 EffectiveAddr);
void XQspiPs_Reset(XQspiPs *InstancePtr);
void XQspiPs_Abort(XQspiPs *InstancePtr);

int XQspiPs_Transfer(XQspiPs *InstancePtr, u8 *SendBufPtr, u8 *RecvBufPtr,
		      unsigned ByteCount);
int XQspiPs_PolledTransfer(XQspiPs *InstancePtr, u8 *SendBufPtr,
			    u8 *RecvBufPtr, unsigned ByteCount);
void XQspiPs_LqspiRead(XQspiPs *InstancePtr, u8 *RecvBufPtr,
			u32 Address, unsigned ByteCount);

int XQspiPs_SetSlaveSelect(XQspiPs *InstancePtr);

void XQspiPs_SetStatusHandler(XQspiPs *InstancePtr, void *CallBackRef,
				XQspiPs_StatusHandler FuncPtr);
void XQspiPs_InterruptHandler(void *InstancePtr);

/*
 * Functions for selftest, in xqspips_selftest.c
 */
int XQspiPs_SelfTest(XQspiPs *InstancePtr);

/*
 * Functions for options, in xqspips_options.c
 */
int XQspiPs_SetOptions(XQspiPs *InstancePtr, u32 Options);
u32 XQspiPs_GetOptions(XQspiPs *InstancePtr);

int XQspiPs_SetClkPrescaler(XQspiPs *InstancePtr, u8 Prescaler);
u8 XQspiPs_GetClkPrescaler(XQspiPs *InstancePtr);

int XQspiPs_SetDelays(XQspiPs *InstancePtr, u8 DelayBtwn, u8 DelayAfter,
			u8 DelayInit);
void XQspiPs_GetDelays(XQspiPs *InstancePtr, u8 *DelayBtwn, u8 *DelayAfter,
			u8 *DelayInit);
#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */

