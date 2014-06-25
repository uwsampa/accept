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
* @file xwdtps_selftest.c
*
* Contains diagnostic self-test functions for the XWdtPs driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ------ -------- --------------------------------------------
* 1.00a ecm/jz 01/15/10 First release
* 1.02a sg     08/01/12 Modified it use the Reset Length mask for the self
*		        test for CR 658287
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "xil_types.h"
#include "xil_assert.h"
#include "xwdtps.h"

/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/


/************************** Variable Definitions *****************************/


/****************************************************************************/
/**
*
* Run a self-test on the timebase. This test verifies that the register access
* locking functions. This is tested by trying to alter a register without
* setting the key value and verifying that the register contents did not
* change.
*
* @param	InstancePtr is a pointer to the XWdtPs instance.
*
* @return
*		- XST_SUCCESS if self-test was successful.
*		- XST_FAILURE if self-test was not successful.
*
* @note		None.
*
******************************************************************************/
int XWdtPs_SelfTest(XWdtPs *InstancePtr)
{
	u32 ZmrOrig;
	u32 ZmrValue1;
	u32 ZmrValue2;

	/*
	 * Assert to ensure the inputs are valid and the instance has been
	 * initialized.
	 */
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * Read the ZMR register at start the test.
	 */
	ZmrOrig = XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
				 XWDTPS_ZMR_OFFSET);

	/*
	 * EX-OR in the length of the interrupt pulse,
	 * do not set the key value.
	 */
	ZmrValue1 = ZmrOrig ^ XWDTPS_ZMR_RSTLN_MASK;


	/*
	 * Try to write to register w/o key value then read back.
	 */
	XWdtPs_WriteReg(InstancePtr->Config.BaseAddress, XWDTPS_ZMR_OFFSET,
			  ZmrValue1);

	ZmrValue2 =	XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
				 XWDTPS_ZMR_OFFSET);

	if (ZmrValue1 == ZmrValue2) {
		/*
		 * If the values match, the hw failed the test,
		 * return orig register value.
		 */
		XWdtPs_WriteReg(InstancePtr->Config.BaseAddress,
				  XWDTPS_ZMR_OFFSET,
				  (ZmrOrig | XWDTPS_ZMR_ZKEY_VAL));
		return XST_FAILURE;
	}


	/*
	 * Try to write to register with key value then read back.
	 */
	XWdtPs_WriteReg(InstancePtr->Config.BaseAddress, XWDTPS_ZMR_OFFSET,
			  (ZmrValue1 | XWDTPS_ZMR_ZKEY_VAL));

	ZmrValue2 =	XWdtPs_ReadReg(InstancePtr->Config.BaseAddress,
				 XWDTPS_ZMR_OFFSET);

	if (ZmrValue1 != ZmrValue2) {
		/*
		 * If the values do not match, the hw failed the test,
		 * return orig register value.
		 */
		XWdtPs_WriteReg(InstancePtr->Config.BaseAddress,
				  XWDTPS_ZMR_OFFSET,
				  ZmrOrig | XWDTPS_ZMR_ZKEY_VAL);
		return XST_FAILURE;

	}

	/*
	 * The hardware locking feature is functional, return the original value
	 * and return success.
	 */
	XWdtPs_WriteReg(InstancePtr->Config.BaseAddress, XWDTPS_ZMR_OFFSET,
			  ZmrOrig | XWDTPS_ZMR_ZKEY_VAL);

	return XST_SUCCESS;
}

