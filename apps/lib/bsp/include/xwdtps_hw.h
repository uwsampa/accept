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
* @file xwdtps_hw.h
*
* This file contains the hardware interface to the Watch Dog Timer (WDT).
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ------ -------- ---------------------------------------------
* 1.00a ecm/jz 01/15/10 First release
* 1.02a  sg    07/15/12 Removed defines related to  External Signal
*			Length functionality for CR 658287
* </pre>
*
******************************************************************************/
#ifndef XWDTPS_HW_H		/* prevent circular inclusions */
#define XWDTPS_HW_H		/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/

#include "xil_types.h"
#include "xil_assert.h"
#include "xil_io.h"

/************************** Constant Definitions *****************************/

/** @name Register Map
 * Offsets of registers from the start of the device
 * @{
 */

#define XWDTPS_ZMR_OFFSET	0x0 /**< Zero Mode Register */
#define XWDTPS_CCR_OFFSET	0x4 /**< Conunter Control Register */
#define XWDTPS_RESTART_OFFSET	0x8 /**< Restart Register */
#define XWDTPS_SR_OFFSET	0xC /**< Status Register */
/* @} */


/** @name Zero Mode Register
 * This register controls how the time out is indicated
 * and also contains the access code to allow
 * writes to the register (0xABC)
 * @{
 */
#define XWDTPS_ZMR_WDEN_MASK	0x00000001 /**< enable the WDT */
#define XWDTPS_ZMR_RSTEN_MASK	0x00000002 /**< enable the reset output */
#define XWDTPS_ZMR_IRQEN_MASK	0x00000004 /**< enable the IRQ output */

#define XWDTPS_ZMR_RSTLN_MASK	0x00000070 /**< set length of reset pulse */
#define XWDTPS_ZMR_RSTLN_SHIFT	4	   /**< shift for reset pulse */

#define XWDTPS_ZMR_IRQLN_MASK	0x00000180 /**< set length of interrupt pulse */
#define XWDTPS_ZMR_IRQLN_SHIFT	7	   /**< shift for interrupt pulse */

#define XWDTPS_ZMR_ZKEY_MASK	0x00FFF000 /**< mask for writing access key */
#define XWDTPS_ZMR_ZKEY_VAL	0x00ABC000 /**< access key, 0xABC << 12 */

/* @} */

/** @name  Counter Control register
 * This register controls how fast the timer runs and the reset value
 * and also contains the access code to allow
 * writes to the register (0x448)
 * @{
 */

#define XWDTPS_CCR_CLKSEL_MASK	0x00000003 /**< counter clock prescale */

#define XWDTPS_CCR_CRV_MASK	0x00003FFC /**< counter reset value */
#define XWDTPS_CCR_CRV_SHIFT	2	   /**< shift for writing value */

#define XWDTPS_CCR_CKEY_MASK	0x03FFC000 /**< mask for writing access key */
#define XWDTPS_CCR_CKEY_VAL	0x00920000 /**< ccess key, 0x448 << 6 */

/* Bit patterns for Clock prescale divider values */

#define XWDTPS_CCR_PSCALE_0008   0x00000000 /**< divide clock by 8 */
#define XWDTPS_CCR_PSCALE_0064   0x00000001 /**< divide clock by 64 */
#define XWDTPS_CCR_PSCALE_0512   0x00000002 /**< divide clock by 512 */
#define XWDTPS_CCR_PSCALE_4096   0x00000003 /**< divide clock by 4096 */

/* @} */

/** @name  Restart register
 * This register resets the timer preventing a timeout. Value is specific
 * 0x1999
 * @{
 */

#define XWDTPS_RESTART_KEY_VAL	0x00001999 /**< valid key */

/*@}*/

/** @name Status register
 * This register indicates timer reached zero.
 * @{
 */
#define XWDTPS_SR_WDZ_MASK	0x00000001 /**< time out occured */

/*@}*/

/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/

/****************************************************************************/
/**
*
* Read the given register.
*
* @param	BaseAddress is the base address of the device
* @param	RegOffset is the register offset to be read
*
* @return	The 32-bit value of the register
*
* @note		C-style signature:
*		u32 XWdtPs_ReadReg(u32 BaseAddress, u32 RegOffset)
*
*****************************************************************************/
#define XWdtPs_ReadReg(BaseAddress, RegOffset) \
	Xil_In32((BaseAddress) + (RegOffset))

/****************************************************************************/
/**
*
* Write the given register.
*
* @param	BaseAddress is the base address of the device
* @param	RegOffset is the register offset to be written
* @param	Data is the 32-bit value to write to the register
*
* @return	None.
*
* @note		C-style signature:
*		void XWdtPs_WriteReg(u32 BaseAddress, u32 RegOffset, u32 Data)
*
*****************************************************************************/
#define XWdtPs_WriteReg(BaseAddress, RegOffset, Data) \
	Xil_Out32((BaseAddress) + (RegOffset), (Data))


/************************** Function Prototypes ******************************/


/************************** Variable Definitions *****************************/

#ifdef __cplusplus
}
#endif

#endif
