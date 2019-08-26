/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2005-2008 SigmaTel, Inc.
//
//! \file ddi_clocks_internal.c
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////////////////
#include "ddi_clocks_common.h"

/////////////////////////////////////////////////////////////////////////////////
// Externs
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// When the voltage is too low, the integer dividers cannot handle the high
// speed clocks from the PLL.  This variable will allow a mechanism to set
// the minimum PFD when calculating divider values.  The initial value is
// the initial hardware minimum PFD value of 18.  
//------------------------------------------------------------------------------
uint8_t u8MinPfdValue = MIN_PFD_VALUE;

//------------------------------------------------------------------------------
// The maximum PLL frequency we can run at.  This is set by an external
// application and is initialized to the default hardware maximum frequency
// of 480MHz.    
//------------------------------------------------------------------------------
uint32_t u32MaxPllFreq = MAX_PLL_KHZ;

//------------------------------------------------------------------------------
// The application can specify whether or not to force the PLL to stay powered
// even when clocks frequencies do not require it.  
//------------------------------------------------------------------------------
bool bKeepPllPowered = FALSE;

//------------------------------------------------------------------------------
// Keeps track of which clients are using the PLL.  Each client has bit assinged
// to in by the enumeration in the ddi_clocks.h header file.  If this variable
// is zero, then none of the clients are using the PLL to generate their clock.  
//------------------------------------------------------------------------------
uint32_t u32PllClients = 0;

/////////////////////////////////////////////////////////////////////////////////
// Code
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions Pclk between two frequencies that both use the 24MHz crystal. 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for Pclk's ref_cpu divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPclkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Set the XTAL divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetPclkRefXtalDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions Pclk from the 24MHz crystal to the PLL 
//!
//! \fntype Function
//!
//! This function follows the steps recommended in the datasheet to transition
//! the Pclk from crystal to PLL without violating any clock domain rules.  
//!
//! \param[in] u32IntDiv - divider for Pclk's ref_cpu divider
//! \param[in] u32PhaseFracDiv - value for ref_cpu PFD
//!
//! \notes The numbered sequence follows the steps outlined in the datasheet
//! \notes for transitioning from XTAL to PLL.  
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPclkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
//    uint32_t u32Wait;
    RtStatus_t rtn;
    hw_clkctrl_bypass_clk_t eClk = BYPASS_CPU;

    //--------------------------------------------------------------------------
    // Enable the PLL.
    //--------------------------------------------------------------------------
    hw_clkctrl_PowerPll(TRUE);

    //--------------------------------------------------------------------------
    // Wait 10us for PLL lock.
    //--------------------------------------------------------------------------
//    u32Wait = 10;
//    hw_digctl_MicrosecondWait(u32Wait);
	udelay(10);
	
    //--------------------------------------------------------------------------
    // Clear the PFD clock gate so we can change dividers.
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefCpuGate(FALSE);


    //--------------------------------------------------------------------------
    // Program the PFD.
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefCpuDiv(u32PhaseFracDiv);


    //--------------------------------------------------------------------------
    // Program the clock controller clock divider that uses the PLL/PFD 
    // as its ref_clock
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetPclkRefCpuDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Switch the bypass to off (select PLL, not crystal)
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetClkBypass(eClk,FALSE)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Set flag so other clocks know we are using PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(PCLK,TRUE);


    //--------------------------------------------------------------------------
    // Done.
    //--------------------------------------------------------------------------
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions Pclk between two frequencies that both use the PLL.
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPclkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;
    bool bPerformIntDivFirst = FALSE;

    //--------------------------------------------------------------------------    
    // We don't want a momentary high Pclk
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetPclkRefCpuDiv() <  u32IntDiv)
    {

        //----------------------------------------------------------------------
        // Set Int div after we set PFD
        //----------------------------------------------------------------------
        bPerformIntDivFirst = TRUE;

        //----------------------------------------------------------------------
        // Set PCLK ref_cpu divider
        //----------------------------------------------------------------------
        if((rtn = hw_clkctrl_SetPclkRefCpuDiv(u32IntDiv,FALSE)) != SUCCESS)
            return rtn;
    }
    else
    {
        //----------------------------------------------------------------------
        // Set Int div after we set PFD
        //----------------------------------------------------------------------
        bPerformIntDivFirst = FALSE;
    }

    //--------------------------------------------------------------------------
    // Set PFD ref_cpu divider                    
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefCpuDiv(u32PhaseFracDiv);
    
    //--------------------------------------------------------------------------
    // Now, after PFD, we can change Pclk divider
    //--------------------------------------------------------------------------
    if(!bPerformIntDivFirst)
    {
        //----------------------------------------------------------------------
        // Set PCLK ref_cpu divider
        //----------------------------------------------------------------------
        if((rtn = hw_clkctrl_SetPclkRefCpuDiv(u32IntDiv,FALSE)) != SUCCESS)
            return rtn;
    }

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions Pclk from PLL to 24MHz crystal
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for Pclk's ref_cpu divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPclkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Set the XTAL divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetPclkRefXtalDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Switch to crystal
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetClkBypass(BYPASS_CPU,TRUE)) != SUCCESS)
        return rtn;
        
    //--------------------------------------------------------------------------
    // Gate the ref_cpu signal from the PLL
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefCpuGate(TRUE);

    //--------------------------------------------------------------------------
    // Set flag so other clocks know we are using PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(PCLK,FALSE);

    //--------------------------------------------------------------------------
    // Try to power off the PLL.  
    //--------------------------------------------------------------------------
    ddi_clocks_PowerOffPll();

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions EmiClk between two frequencies that both use the 24MHz crystal. 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for EmiClk's ref_emi divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransEmiClkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate the XTAL clock divider
    //--------------------------------------------------------------------------
    hw_clkctrl_SetEmiRefXtalClkGate(FALSE);

    //--------------------------------------------------------------------------
    // Set the XTAL divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetEmiClkRefXtalDiv(u32IntDiv)) != SUCCESS)
        return rtn;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions EmiClk from the 24MHz crystal to the PLL 
//!
//! \fntype Function
//!
//! This function follows the steps recommended in the datasheet to transition
//! the EmiClk from crystal to PLL without violating any clock domain rules.  
//!
//! \param[in] u32IntDiv - divider for EmiClk's ref_emi divider
//! \param[in] u32PhaseFracDiv - value for ref_emi PFD
//!
//! \notes The numbered sequence follows the steps outlined in the datasheet
//! \notes for transitioning from XTAL to PLL.  
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransEmiClkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
//    uint32_t u32Wait;
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Enable the PLL.
    //--------------------------------------------------------------------------
    hw_clkctrl_PowerPll(TRUE);
    
    //--------------------------------------------------------------------------
    // Wait 10us for PLL lock.
    //--------------------------------------------------------------------------
//    u32Wait = 10;
//    hw_digctl_MicrosecondWait(u32Wait);
	udelay(10);
    //--------------------------------------------------------------------------
    // Program and enable the PFD with the desired configuration.
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefEmiDiv(u32PhaseFracDiv);

    //--------------------------------------------------------------------------
    // Clear the PFD clock gate to establish the desired reference 
    // clock frequency
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefEmiGate(FALSE);

    //--------------------------------------------------------------------------
    // Program the clock controller clock divider that uses the PLL/PFD 
    // as its ref_clock.
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetEmiClkRefEmiDiv(u32IntDiv)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Switch the bypass to off (select PLL, not crystal)
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetClkBypass(BYPASS_EMI,FALSE)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Set the clock gate for ref_xtal signal
    //--------------------------------------------------------------------------
    hw_clkctrl_SetEmiRefXtalClkGate(TRUE);

    //--------------------------------------------------------------------------
    // Set flag so other clocks know we are using PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(EMICLK,TRUE);

    //--------------------------------------------------------------------------
    // Done.
    //--------------------------------------------------------------------------
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions EmiClk between two frequencies that both use the PLL.
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for EmiClk's ref_emi divider
//! \param[in] u32PhaseFracDiv - value for ref_emi PFD
//!
//! \notes The numbered sequence follows the steps outlined in the datasheet
//! \notes for transitioning from XTAL to PLL.  
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransEmiClkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;
    bool bPerformIntDivFirst; 

    //--------------------------------------------------------------------------
    // We don't want a momentary high EmiClk
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetEmiClkRefEmiDiv() <  u32IntDiv)
    {
        //----------------------------------------------------------------------
        // Set Int div before we set PFD
        //----------------------------------------------------------------------
        bPerformIntDivFirst = TRUE;

        //----------------------------------------------------------------------
        // Set EmiClk ref_emi divider
        //----------------------------------------------------------------------
        if((rtn = hw_clkctrl_SetEmiClkRefEmiDiv(u32IntDiv)) != SUCCESS)
            return rtn;
    }
    else
    {
        //----------------------------------------------------------------------
        // Set Int div after we set PFD
        //----------------------------------------------------------------------
        bPerformIntDivFirst = FALSE;
    }


    //--------------------------------------------------------------------------
    // Set PFD ref_emi divider
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefEmiDiv(u32PhaseFracDiv);


    //--------------------------------------------------------------------------
    // Now, after PFD, we set ref_emi divider
    //--------------------------------------------------------------------------
    if(!bPerformIntDivFirst)
    {
        //----------------------------------------------------------------------
        // Set EmiClk ref_emi divider
        //----------------------------------------------------------------------
        if((rtn = hw_clkctrl_SetEmiClkRefEmiDiv(u32IntDiv)) != SUCCESS)
            return rtn;
    }

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions EmiClk from the PLL to 24MHz crystal 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransEmiClkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate the XTAL clock divider
    //--------------------------------------------------------------------------
    hw_clkctrl_SetEmiRefXtalClkGate(FALSE);

    //--------------------------------------------------------------------------
    // Set the XTAL divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetEmiClkRefXtalDiv(u32IntDiv)) != SUCCESS)
        return rtn;

    //--------------------------------------------------------------------------
    // Switch to crystal
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetClkBypass(BYPASS_EMI,TRUE)) != SUCCESS)
        return rtn;            

    //--------------------------------------------------------------------------
    // Gate the ref_emi clock from the PLL to save some power
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefEmiGate(TRUE);
    
    //--------------------------------------------------------------------------
    // Set flag so other clocks know we are no longer using PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(EMICLK,FALSE); 

    //--------------------------------------------------------------------------
    // Try to power off the PLL.  
    //--------------------------------------------------------------------------
    ddi_clocks_PowerOffPll();

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions SspClk between two frequencies that both use the 24MHz crystal. 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransSspClkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate the SSP clock divider
    //--------------------------------------------------------------------------
    hw_clkctrl_SetSspClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Set the SSP integer divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetSspClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions SspClk from the 24MHz crystal to the PLL.
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransSspClkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate both dividers
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefIoGate(FALSE);
    hw_clkctrl_SetSspClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Change SSP divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetSspClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    if(bChangeRefIo)
    {
        //----------------------------------------------------------------------
        // Change ref_io divider
        //----------------------------------------------------------------------
        hw_clkctrl_SetPfdRefIoDiv(u32PhaseFracDiv);
    }


    //--------------------------------------------------------------------------
    // Switch bypass
    //--------------------------------------------------------------------------
    if((rtn = ddi_clocks_BypassRefIoSsp(FALSE)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Set flag so other clocks know this clock is using the PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(SSPCLK,TRUE);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions SspClk between two frequencies that both use the PLL.
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransSspClkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate divider
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefIoGate(FALSE);

    //--------------------------------------------------------------------------
    // Change SSP divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetSspClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    if(bChangeRefIo)
    {
        //----------------------------------------------------------------------
        // Change ref_io divider
        //----------------------------------------------------------------------
        hw_clkctrl_SetPfdRefIoDiv(u32PhaseFracDiv);
    }

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions SspClk from the PLL to 24MHz crystal 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransSspClkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;
    bool bRefIoGated = FALSE;

    //--------------------------------------------------------------------------
    // Ungate both dividers
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetPfdRefIoGate())
    {
        bRefIoGated = TRUE;
        hw_clkctrl_SetPfdRefIoGate(FALSE);
    }
    hw_clkctrl_SetSspClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Change XTAL divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetSspClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;
                    

    //--------------------------------------------------------------------------
    // Switch bypass
    //--------------------------------------------------------------------------
    if((rtn = ddi_clocks_BypassRefIoSsp(TRUE)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Put ref_io gate they way we found it
    //--------------------------------------------------------------------------
    if(bRefIoGated)
    {
        hw_clkctrl_SetPfdRefIoGate(TRUE);
    }


    //--------------------------------------------------------------------------
    // Set flag so other clocks know this clock is no longer using the PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(SSPCLK,FALSE);


    //--------------------------------------------------------------------------
    // Try to power off the PLL.  
    //--------------------------------------------------------------------------
    ddi_clocks_PowerOffPll();

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions IrClk between two frequencies that both use the 24MHz crystal. 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for IrClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransIrClkXtalToXtal(uint32_t u32IrDiv, uint32_t u32IrovDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate the IR clock divider.
    //--------------------------------------------------------------------------
    hw_clkctrl_SetIrClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Set the IR integer divider.
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetIrClkDiv(u32IrDiv)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Set the IROV integer divider.
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetIrovClkDiv(u32IrovDiv)) != SUCCESS)
        return rtn;

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions IrClk from the 24MHz crystal to PLL
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for IrClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransIrClkXtalToPll(uint32_t u32IrDiv, uint32_t u32IrovDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo)
{
    RtStatus_t rtn;
    
    //--------------------------------------------------------------------------
    // Ungate both dividers
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefIoGate(FALSE);
    hw_clkctrl_SetIrClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Change IR divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetIrClkDiv(u32IrDiv)) != SUCCESS)
        return rtn;

    if((rtn = hw_clkctrl_SetIrovClkDiv(u32IrovDiv)) != SUCCESS)
        return rtn;

    if(bChangeRefIo)
    {
        //----------------------------------------------------------------------
        // Change ref_io divider
        //----------------------------------------------------------------------
        hw_clkctrl_SetPfdRefIoDiv(u32PhaseFracDiv);
    }


    //--------------------------------------------------------------------------
    // Switch bypass
    //--------------------------------------------------------------------------
    if((rtn = ddi_clocks_BypassRefIoIr(FALSE)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Set flag so other clocks know this clock is using the PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(IROVCLK,TRUE);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions IrClk between two frequencies that both use the PLL.
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for IrClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransIrClkPllToPll(uint32_t u32IrDiv, uint32_t u32IrovDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate both dividers
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefIoGate(FALSE);
    hw_clkctrl_SetIrClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Change IR divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetIrClkDiv(u32IrDiv)) != SUCCESS)
        return rtn;

    if((rtn = hw_clkctrl_SetIrovClkDiv(u32IrovDiv)) != SUCCESS)
        return rtn;

    if(bChangeRefIo)
    {
        //----------------------------------------------------------------------
        // Change ref_io divider
        //----------------------------------------------------------------------
        hw_clkctrl_SetPfdRefIoDiv(u32PhaseFracDiv);
    }

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions IrClk from the PLL to 24MHz crystal
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for IrClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransIrClkPllToXtal(uint32_t u32IrDiv, uint32_t u32IrovDiv)
{
    RtStatus_t rtn;
    bool bRefIoGated = FALSE;

    //--------------------------------------------------------------------------
    // Ungate both dividers
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetPfdRefIoGate())
    {
        bRefIoGated = TRUE;
        hw_clkctrl_SetPfdRefIoGate(FALSE);
    }
    hw_clkctrl_SetIrClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Change IR and IROV divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetIrClkDiv(u32IrDiv)) != SUCCESS)
        return rtn;

    if((rtn = hw_clkctrl_SetIrovClkDiv(u32IrovDiv)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Switch bypass
    //--------------------------------------------------------------------------
    if((rtn = ddi_clocks_BypassRefIoIr(TRUE)) != SUCCESS)
        return rtn;

    //----------------------------------------------------------------------
    // Put ref_io gate they way we found it
    //----------------------------------------------------------------------
    if(bRefIoGated)
    {
        hw_clkctrl_SetPfdRefIoGate(TRUE);
    }

    //--------------------------------------------------------------------------
    // Set flag so other clocks know this clock is using the PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(IROVCLK,FALSE);

    //--------------------------------------------------------------------------
    // Try to power off the PLL.  
    //--------------------------------------------------------------------------
    ddi_clocks_PowerOffPll();

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions PixClk between two frequencies that both use the 24MHz crystal. 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for PixClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPixClkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate the PIX clock divider
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPixClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Set the PIX integer divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetPixClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;

    return SUCCESS;


}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions PixClk from the 24MHz crystal to PLL
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for PixClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPixClkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;
    bool bPerformIntDivFirst;
    
    //--------------------------------------------------------------------------
    // Ungate both dividers
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefPixGate(FALSE);
    hw_clkctrl_SetPixClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Determine the order to set the dividers
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetPixClkDiv() <  u32IntDiv)
    {
        //----------------------------------------------------------------------
        // Set Int div first
        //----------------------------------------------------------------------
        bPerformIntDivFirst = true;

        //----------------------------------------------------------------------
        // Set PixClk integer divider
        //----------------------------------------------------------------------
        if((rtn = hw_clkctrl_SetPixClkDiv(u32IntDiv,FALSE)) != SUCCESS)
            return rtn;
    }
    else
    {
        bPerformIntDivFirst = false;
    }


    //--------------------------------------------------------------------------
    // Set PFD ref_pix divider
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefPixDiv(u32PhaseFracDiv);


    //--------------------------------------------------------------------------         
    // Now, set PixClk integer divider
    //--------------------------------------------------------------------------
    if(!bPerformIntDivFirst)
    {
        if((hw_clkctrl_SetPixClkDiv(u32IntDiv,FALSE)) != SUCCESS)
            return rtn;
    }


    //--------------------------------------------------------------------------
    // Switch bypass
    //--------------------------------------------------------------------------
    if((rtn = ddi_clocks_BypassRefPix(FALSE)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Set flag so other clocks know this clock is using the PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(PIXCLK,TRUE);

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions PixClk between two frequencies that both use the PLL.
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for PixClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPixClkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;
    bool bPerformIntDivFirst;


    //--------------------------------------------------------------------------
    // Determine the order to set the dividers
    //--------------------------------------------------------------------------
    if(hw_clkctrl_GetPixClkDiv() <  u32IntDiv)
    {
        bPerformIntDivFirst = true;

        //----------------------------------------------------------------------
        // Set PixClk integer divider
        //----------------------------------------------------------------------
        if((rtn = hw_clkctrl_SetPixClkDiv(u32IntDiv,FALSE)) != SUCCESS)
            return rtn;
    }
    else
    {
        bPerformIntDivFirst = false;
    }


    //--------------------------------------------------------------------------
    // Set PFD ref_pix divider
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefPixDiv(u32PhaseFracDiv);
         
    if(!bPerformIntDivFirst)
    {
        //----------------------------------------------------------------------
        // Set PixClk integer divider
        //----------------------------------------------------------------------
        if((hw_clkctrl_SetPixClkDiv(u32IntDiv,FALSE)) != SUCCESS)
            return rtn;
    }

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions PixClk from the PLL to 24Mhz crystal
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for PixClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPixClkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv)
{
    RtStatus_t rtn;

    //--------------------------------------------------------------------------
    // Ungate both dividers
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefPixGate(FALSE);
    hw_clkctrl_SetPixClkGate(FALSE);


    //--------------------------------------------------------------------------
    // Change PIX divider
    //--------------------------------------------------------------------------
    if((rtn = hw_clkctrl_SetPixClkDiv(u32IntDiv,FALSE)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------                    
    // Switch bypass
    //--------------------------------------------------------------------------
    if((rtn = ddi_clocks_BypassRefPix(TRUE)) != SUCCESS)
        return rtn;


    //--------------------------------------------------------------------------
    // Gate ref_pix to save power
    //--------------------------------------------------------------------------
    hw_clkctrl_SetPfdRefPixGate(TRUE);


    //--------------------------------------------------------------------------
    // Set flag so other clocks know this clock is using the PLL.
    //--------------------------------------------------------------------------
    ddi_clocks_ClockUsingPll(PIXCLK,FALSE);


    //--------------------------------------------------------------------------
    // Try to power off the PLL.  
    //--------------------------------------------------------------------------
    ddi_clocks_PowerOffPll();

    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the requested ref_* clock from the PLL.  
//!
//! \fntype Function
//!
//! \param[in] RefClk Valid values are PLL_REF_CPU, PLL_REF_EMI, PLL_REF_IO,
//! PLL_REF_PIX.
//!
//! \retval Frequency in kHz, or 0 if clock is gated.
/////////////////////////////////////////////////////////////////////////////////
uint32_t ddi_clocks_GetPllRefClkFreq(ddi_clocks_pll_ref_clks_t RefClk)
{
    uint32_t PfdValue;
    bool ClkGate;
    uint32_t ClkFreq;

    //--------------------------------------------------------------------------
    // Get the PFD value and the gate status for the specified reference clock.
    //--------------------------------------------------------------------------        
    switch(RefClk)
    {
        case PLL_REF_CPU:
            ClkGate = HW_CLKCTRL_FRAC.B.CLKGATECPU;
            PfdValue = HW_CLKCTRL_FRAC.B.CPUFRAC;
            break;

        case PLL_REF_EMI:
            ClkGate = HW_CLKCTRL_FRAC.B.CLKGATEEMI;
            PfdValue = HW_CLKCTRL_FRAC.B.EMIFRAC;
            break;

        case PLL_REF_IO:
            ClkGate = HW_CLKCTRL_FRAC.B.CLKGATEIO;
            PfdValue = HW_CLKCTRL_FRAC.B.IOFRAC;
            break;

        case PLL_REF_PIX:
            ClkGate = HW_CLKCTRL_FRAC.B.CLKGATEPIX;
            PfdValue = HW_CLKCTRL_FRAC.B.PIXFRAC;
            break;
    }

    
    //--------------------------------------------------------------------------
    // Calculate the clock frequency; everything is in kHz.
    //--------------------------------------------------------------------------
    {
        //----------------------------------------------------------------------
        // If the clock is gated, then we have no clock.
        //----------------------------------------------------------------------
        if(ClkGate)
        {
            return 0;
        }

        //----------------------------------------------------------------------
        // Start with the max PLL frequency.
        //----------------------------------------------------------------------
        ClkFreq = ddi_clocks_GetMaxPllRefFreq();

        //----------------------------------------------------------------------
        // The fractional divide is PLL*(18/X) where X is the value in the 
        // register.  To avoid trucation errors, we will first multiply the PLL
        // by 18, then divide by the X value.  
        //----------------------------------------------------------------------
        ClkFreq *= PFD_DIV_CONSTANT;
        ClkFreq /= PfdValue;

        //----------------------------------------------------------------------                            
        // Return our value.
        //----------------------------------------------------------------------
        return ClkFreq;
            
    }    


}

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

