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
/****************************************************************************/
/**
*
* @file xwdtps.c
*
* Contains the implementation of interface functions of the XWdtPs driver.
* See xwdtps.h for a description of the driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ------ -------- ---------------------------------------------
* 1.00a ecm/jz 01/15/10 First release
* 1.02a  sg    07/15/12 Removed code/APIs related to  External Signal
*			Length functionality for CR 658287
*			Removed APIs XWdtPs_SetExternalSignalLength,
*			XWdtPs_GetExternalSignalLength
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "xwdtps.h"

/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/


/************************** Variable Definitions *****************************/


/****************************************************************************/
/**
*
* Initialize a specific watchdog timer instance/driver. This function
* must be called before other functions of the driver are called.
*
* @param	InstancePtr is a pointer to the XWdtPs instance.
* @param	ConfigPtr is the config structure.
* @param	EffectiveAddress is the base address for the device. It could be
*		a virtual address if address translation is supported in the
*		system, otherwise it is the physical address.
*
* @return
*		- XST_SUCCESS if initialization was successful.
*		- XST_DEVICE_IS_STARTED if the device has already been started.
*
* @note		None.
*
******************************************************************************/
int XWdtPs_CfgInitialize(XWdtPs *InstancePtr,
			XWdtPs_Config *ConfigPtr, u32 EffectiveAddress)
{
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(ConfigPtr != NULL);

	/*
	 * If the device is started, disallow the initialize and return a
	 * status indicating it is started. This allows the user to stop the
	 * device and reinitialize, but prevents a user from inadvertently
	 * initializing.
	 */
	if (InstancePtr->IsStarted == XIL_COMPONENT_IS_STARTED) {
		return XST_DEVICE_IS_STARTED;
	}

	/*
	 * Copy configuration into instance.
	 */
	InstancePtr->Config.DeviceId = ConfigPtr->DeviceId;

	/*
	 * Save the base address pointer such that the registers of the block
	 * can be accessed and indicate it has not been started yet.
	 */
	InstancePtr->Config.BaseAddress = EffectiveAddress;
	InstancePtr->IsStarted = 0;

	/*
	 * Indicate the instance is ready to use, successfully initialized.
	 */
	InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

	return XST_SUCCESS;
}

/****************************************************************************/
/**
*
* Start the watchdog timer of the device.
*
* @param	InstancePtr is a pointer to the XWdtPs instance.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void XWdtPs_Start(XWdtPs *InstancePtr)
{
	u32 Register;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * Read the contents of the ZMR register.
	 */
	Register = XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
				 XWDTPS_ZMR_OFFSET);

	/*
	 * Enable the Timer field in the register and Set the access key so the
	 * write takes place.
	 */
	Register |= XWDTPS_ZMR_WDEN_MASK;
	Register |= XWDTPS_ZMR_ZKEY_VAL;

	/*
	 * Update the ZMR with the new value.
	 */
	XWdtPs_WriteReg(InstancePtr->Config.BaseAddress, XWDTPS_ZMR_OFFSET,
			  Register);

	/*
	 * Indicate that the device is started.
	 */
	InstancePtr->IsStarted = XIL_COMPONENT_IS_STARTED;

}

/****************************************************************************/
/**
*
* Disable the watchdog timer.
*
* It is the caller's responsibility to disconnect the interrupt handler
* of the watchdog timer from the interrupt source, typically an interrupt
* controller, and disable the interrupt in the interrupt controller.
*
* @param	InstancePtr is a pointer to the XWdtPs instance.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void XWdtPs_Stop(XWdtPs *InstancePtr)
{
	u32 Register;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * Read the contents of the ZMR register.
	 */
	Register = XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
				 XWDTPS_ZMR_OFFSET);

	/*
	 * Disable the Timer field in the register and
	 * Set the access key for the write to be done the register.
	 */
	Register &= ~XWDTPS_ZMR_WDEN_MASK;
	Register |= XWDTPS_ZMR_ZKEY_VAL;

	/*
	 * Update the ZMR with the new value.
	 */
	XWdtPs_WriteReg(InstancePtr->Config.BaseAddress, XWDTPS_ZMR_OFFSET,
			  Register);

	InstancePtr->IsStarted = 0;
}


/****************************************************************************/
/**
*
* Enables the indicated signal/output.
* Performs a read/modify/write cycle to update the value correctly.
*
* @param	InstancePtr is a pointer to the XWdtPs instance.
* @param	Signal is the desired signal/output.
*		Valid Signal Values are XWDTPS_RESET_SIGNAL and
*		XWDTPS_IRQ_SIGNAL.
*		Only one of them can be specified at a time.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void XWdtPs_EnableOutput(XWdtPs *InstancePtr, u8 Signal)
{
	u32 Register = 0;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertVoid((Signal == XWDTPS_RESET_SIGNAL) ||
			(Signal == XWDTPS_IRQ_SIGNAL));

	/*
	 * Read the contents of the ZMR register.
	 */
	Register = XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
				 XWDTPS_ZMR_OFFSET);

	if (Signal == XWDTPS_RESET_SIGNAL) {
		/*
		 * Enable the field in the register.
		 */
		Register |= XWDTPS_ZMR_RSTEN_MASK;

	} else if (Signal == XWDTPS_IRQ_SIGNAL) {
		/*
		 * Enable the field in the register.
		 */
		Register |= XWDTPS_ZMR_IRQEN_MASK;

	}

	/*
	 * Set the access key so the write takes.
	 */
	Register |= XWDTPS_ZMR_ZKEY_VAL;

	/*
	 * Update the ZMR with the new value.
	 */
	XWdtPs_WriteReg(InstancePtr->Config.BaseAddress, XWDTPS_ZMR_OFFSET,
			  Register);
}

/****************************************************************************/
/**
*
* Disables the indicated signal/output.
* Performs a read/modify/write cycle to update the value correctly.
*
* @param	InstancePtr is a pointer to the XWdtPs instance.
* @param	Signal is the desired signal/output.
*		Valid Signal Values are XWDTPS_RESET_SIGNAL and
*		XWDTPS_IRQ_SIGNAL
*		Only one of them can be specified at a time.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void XWdtPs_DisableOutput(XWdtPs *InstancePtr, u8 Signal)
{
	u32 Register = 0;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertVoid((Signal == XWDTPS_RESET_SIGNAL) ||
			(Signal == XWDTPS_IRQ_SIGNAL));

	/*
	 * Read the contents of the ZMR register.
	 */
	Register = XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
				 XWDTPS_ZMR_OFFSET);

	if (Signal == XWDTPS_RESET_SIGNAL) {
		/*
		 * Disable the field in the register.
		 */
		Register &= ~XWDTPS_ZMR_RSTEN_MASK;

	} else if (Signal == XWDTPS_IRQ_SIGNAL) {
		/*
		 * Disable the field in the register.
		 */
		Register &= ~XWDTPS_ZMR_IRQEN_MASK;

	}

	/*
	 * Set the access key so the write takes place.
	 */
	Register |= XWDTPS_ZMR_ZKEY_VAL;

	/*
	 * Update the ZMR with the new value.
	 */
	XWdtPs_WriteReg(InstancePtr->Config.BaseAddress, XWDTPS_ZMR_OFFSET,
			  Register);
}

/****************************************************************************/
/**
*
* Returns the current control setting for the indicated signal/output.
* The register referenced is the Counter Control Register (XWDTPS_CCR_OFFSET)
*
* @param	InstancePtr is a pointer to the XWdtPs instance.
* @param	Control is the desired signal/output.
*		Valid Control Values are XWDTPS_CLK_PRESCALE and
*		XWDTPS_COUNTER_RESET. Only one of them can be specified at a
*		time.
*
* @return	The contents of the requested control field in the Counter
*		Control Register (XWDTPS_CCR_OFFSET).
*		If the Control is XWDTPS_CLK_PRESCALE then use the
*		defintions XWDTEPB_CCR_PSCALE_XXXX.
*		If the Control is XWDTPS_COUNTER_RESET then the values are
*		0x0 to 0xF. This is the most significant nibble of the CCR
*		register.
*
* @note		None.
*
******************************************************************************/
u32 XWdtPs_GetControlValue(XWdtPs *InstancePtr, u8 Control)
{
	u32 Register;
	u32 ReturnValue = 0;

	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertNonvoid((Control == XWDTPS_CLK_PRESCALE) ||
			(Control == XWDTPS_COUNTER_RESET));

	/*
	 * Read the contents of the CCR register.
	 */
	Register = XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
			 XWDTPS_CCR_OFFSET);

	if (Control == XWDTPS_CLK_PRESCALE) {
		/*
		 * Mask off the field in the register.
		 */
		ReturnValue = Register & XWDTPS_CCR_CLKSEL_MASK;

	} else if (Control == XWDTPS_COUNTER_RESET) {
		/*
		 * Mask off the field in the register.
		 */
		Register &= XWDTPS_CCR_CRV_MASK;

		/*
		 * Shift over to the right most positions.
		 */
		ReturnValue = Register >> XWDTPS_CCR_CRV_SHIFT;
	}

	return ReturnValue;
}

/****************************************************************************/
/**
*
* Updates the current control setting for the indicated signal/output with
* the provided value.
*
* Performs a read/modify/write cycle to update the value correctly.
* The register referenced is the Counter Control Register (XWDTPS_CCR_OFFSET)
*
* @param	InstancePtr is a pointer to the XWdtPs instance.
* @param	Control is the desired signal/output.
*		Valid Control Values are XWDTPS_CLK_PRESCALE and
*		XWDTPS_COUNTER_RESET. Only one of them can be specified at a
*		time.
* @param	Value is the desired control value.
*		If the Control is XWDTPS_CLK_PRESCALE then use the
*		defintions XWDTEPB_CCR_PSCALE_XXXX.
*		If the Control is XWDTPS_COUNTER_RESET then the valid values
*		are 0x0 to 0xF, this sets the most significant nibble of the CCR
*		register.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void XWdtPs_SetControlValue(XWdtPs *InstancePtr, u8 Control, u32 Value)
{
	u32 Register = 0;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertVoid((Control == XWDTPS_CLK_PRESCALE) ||
			(Control == XWDTPS_COUNTER_RESET));

	/*
	 * Read the contents of the CCR register.
	 */
	Register = XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
				 XWDTPS_CCR_OFFSET);

	if (Control == XWDTPS_CLK_PRESCALE) {
		/*
		 * Zero the field in the register.
		 */
		Register &= ~XWDTPS_CCR_CLKSEL_MASK;

	} else if (Control == XWDTPS_COUNTER_RESET) {
		/*
		 * Zero the field in the register.
		 */
		Register &= ~XWDTPS_CCR_CRV_MASK;

		/*
		 * Shift Value over to the proper positions.
		 */
		Value = Value << XWDTPS_CCR_CRV_SHIFT;
	}

	Register |= Value;

	/*
	 * Set the access key so the write takes.
	 */
	Register |= XWDTPS_CCR_CKEY_VAL;

	/*
	 * Update the CCR with the new value.
	 */
	XWdtPs_WriteReg(InstancePtr->Config.BaseAddress, XWDTPS_CCR_OFFSET,
			  Register);
}

