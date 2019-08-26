// ddi_emi.c

#include <linux/delay.h>
#include <../include/types.h>
#include <../include/error.h>

#include "../include/errordefs.h"
#include "hw_clocks.h"
#include "ddi_clocks.h"
#include "hw_emi.h"
#include "hw_dram_settings.h"
#include "ddi_emi_errordefs.h"
#include "ddi_emi.h"
#include "../power/hw_power.h"
#ifdef __KERNEL__
 #include <asm/cacheflush.h>
 #include <asm/hardware.h>
#else
 #include "linux_regs.h"
#endif


// Valid stmp37xx values are 4, 8, and 12mA
#define EMI_PIN_DRIVE_ADDRESS       PIN_DRIVE_12mA
#define EMI_PIN_DRIVE_DATA          PIN_DRIVE_12mA
#define EMI_PIN_DRIVE_CHIP_ENABLE   PIN_DRIVE_12mA
#define EMI_PIN_DRIVE_CLOCK         PIN_DRIVE_12mA
#define EMI_PIN_DRIVE_CONTROL       PIN_DRIVE_12mA

#define EXTENDED_MODE_REGISTER_VALUE 0x20;
//#define DDI_EMI_DEBUG


#define DDI_EMI_DDR_MAX_NO_DCCRESYNC_FREQ_MHZ 60


//#pragma ghs section bss=".ocram.bss"   /// MUST BE IN OCRAM !!!!

static ddi_emi_vars_t s_ddi_emi_vars;

//
// TODO : verify operation without TLB lockdown
//  - disabled for now
// 
RtStatus_t ddi_emi_LockDownMemory(void)
{
#if 0 // TODOs
    uint32_t first_page_address = ((uint32_t)ddi_emi_start_function_for_locating & 0xFFFFF000);
    uint32_t last_page_address = ((uint32_t)hw_emi_end_function_for_locating & 0xFFFFF000);
    uint32_t bss_start_address = (uint32_t)&s_ddi_emi_vars & 0xFFFFF000;
    int32_t bss_size = (uint32_t)hw_emi_GetObjectStartAddress() - bss_start_address;

    bool bPrevIntState;

    // The EMI code shouldn't need more than 4kB.  Size should
    // be limited to limit OCRAM usage and fixed TLB entries.
    if(
	((last_page_address - first_page_address + 0xfff) > MAXIMUM_EMI_TEXT_SIZE_DESIRED) ||
        (last_page_address < first_page_address) ||
        ((bss_size + 0xfff) > MAXIMUM_EMI_BSS_SIZE_DESIRED) ||
        (bss_size < 0) )
    {
        return ERROR_DDI_EMI_UNEXPECTED_OBJECT_SIZE;
    }

    // disable interrupts as required by the hw_core_LockxTLB functions
    bPrevIntState = hw_core_EnableIrqInterrupt(FALSE);

#if 0    
    // clean the data cache to ensure nothing gets written to sdram while
    // we are in self-refresh mode
    hw_core_clean_DCache();

    // make sure data written    
    hw_core_drain_write_buffer();
        
    //flush dcache
    hw_core_invalidate_DCache();

    //flush icache
    hw_core_invalidate_ICache();
    
    hw_core_invalidate_TLBs();

#endif
    
    
    // Dependency:  For this code to work correctly, it requires that the
    // hw_core_LockITLBMVA_corrected and hw_core_LockDTLBMVA code not be located
    // the the same pages we are locking down.  Also, the stack which this code runs in 
    // cannot be the same as the page data lockdown pages.
    
    hw_core_LockITLBMVA_corrected( DDI_EMI_TLB_TEXT_LOCK_ELEMENT1, first_page_address );
    // Does text area cross page boundary?  If so, a second text TLB entry is needed.
    if(first_page_address != last_page_address)
    {
        hw_core_LockITLBMVA_corrected( DDI_EMI_TLB_TEXT_LOCK_ELEMENT2, first_page_address+0x1000 );
    }

    if((first_page_address+0x1000) != last_page_address)
    {
        hw_core_LockITLBMVA_corrected( DDI_EMI_TLB_TEXT_LOCK_ELEMENT3, last_page_address );
    }


    hw_core_LockDTLBMVA( DDI_EMI_TLB_BSS_LOCK_ELEMENT1, bss_start_address );
    // Does bss object cross page boundary?  if so, a second bss TLB entry is needed
    if( (( (uint32_t)&s_ddi_emi_vars + bss_size) & 0xFFFFF000) != bss_start_address)
    {
        hw_core_LockDTLBMVA( DDI_EMI_TLB_BSS_LOCK_ELEMENT2, bss_start_address + 0x1000 );
    }
    
    hw_core_EnableIrqInterrupt(bPrevIntState);
#endif
    return SUCCESS;
}

//
// NOT used in linux platform, but enabled for bootloader
//  - done in bootloader
// 
#ifndef __KERNEL__
RtStatus_t ddi_emi_PreOsInit(hw_emi_MemType_t MemType,
    hw_emi_ChipSelectMask_t ChipSelectMask,
        hw_emi_TotalDramSize_t TotalDramSize)
{
    RtStatus_t Rtn;
    bool bInterruptEnableState;
    uint32_t StartTime;
    uint8_t i, NumChipSelects = 0;
    hw_emi_TotalDramSize_t TotalDieSize;  // for calculating column_size and addr_pins


    s_ddi_emi_vars.MemType = MemType;
    s_ddi_emi_vars.bAutoMemorySelfRefreshModeEnabled = FALSE;
    s_ddi_emi_vars.bAutoMemoryClockGateModeEnabled = FALSE;
    s_ddi_emi_vars.bStaticMemorySelfRefreshModeEnabled = FALSE;
    s_ddi_emi_vars.bStaticMemoryClockGateModeEnabled = FALSE;


    // this may be temporary.  Another direction is to have a separate function
    // to reset the peripheral if the application tries to initial it.  But
    // this requires that this function not be called while the SDRAM contents need
    // to be retained which is currently violated by the SDK.

    // make sure the HW_DRAM registers have a clock signal.  Otherwise, any
    // attempt to read a HW_DRAM register will result in a crash.
    if(
        (HW_CLKCTRL_CLKSEQ.B.BYPASS_EMI && !HW_CLKCTRL_EMI.B.CLKGATE) ||
        (!HW_CLKCTRL_CLKSEQ.B.BYPASS_EMI && !HW_CLKCTRL_FRAC.B.CLKGATEEMI)
      )
    {
            if(HW_DRAM_CTL08.B.START == 1)
            {
                ddi_emi_ExitAutoMemorySelfRefreshMode();
                ddi_emi_EnterAutoMemoryClockGateMode();
                return SUCCESS;  // emi peripheral already initialized.
	        }


    }

    if(!ChipSelectMask)
    {
        return ERROR_DDI_EMI_NO_CHIP_SELECT_WAS_SELECTED;
    }
    else
    {
      for(i = 0; i < 4; i++)
      {
      	 if(ChipSelectMask & (1 << i))
      	    NumChipSelects++;
      }

      if(NumChipSelects > 2)
      	return ERROR_DDI_EMI_ONLY_2_CHIP_SELECTS_SUPPORTED;
    }

    if( (TotalDramSize > EMI_64MB_DRAM) && (NumChipSelects==1))
        return ERROR_DDI_EMI_64MB_PER_CHIP_SELCT_MAXIMUM;

    if(TotalDramSize > EMI_128MB_DRAM)
        return ERROR_DDI_EMI_64MB_PER_CHIP_SELCT_MAXIMUM;

    // initialize clocks

    hw_clkctrl_SetClkBypass(BYPASS_EMI, true);
    hw_clkctrl_SetEmiRefXtalClkGate(false);
    hw_clkctrl_SetEmiClkRefXtalDiv(1);
    //HW_CLKCTRL_EMI_CLR(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);

    // Enable the EMI block by clearing the Soft Reset and Clock Gate
    hw_emi_ClearReset();

    if(!hw_emi_IsDramSupported())
    {
        return ERROR_DDI_EMI_SDRAM_NOT_SUPPORTED;
    }


#if 0
if(MemType == EMI_DEV_SDRAM)
{
        // Set up the pinmux for the EMI
    hw_emi_ConfigureEmiPins(
        PIN_VOLTAGE_3pt3V, 
        EMI_PIN_DRIVE_ADDRESS,
        EMI_PIN_DRIVE_DATA,
        EMI_PIN_DRIVE_CHIP_ENABLE,
        EMI_PIN_DRIVE_CLOCK,
        EMI_PIN_DRIVE_CONTROL);

}
else if(MemType == EMI_DEV_MOBILE_SDRAM)
{
       // Set up the pinmux for the EMI
    hw_emi_ConfigureEmiPins(
        PIN_VOLTAGE_1pt8V, 
        EMI_PIN_DRIVE_ADDRESS,
        EMI_PIN_DRIVE_DATA,
        EMI_PIN_DRIVE_CHIP_ENABLE,
        EMI_PIN_DRIVE_CLOCK,
        EMI_PIN_DRIVE_CONTROL);
    

}
else
if(MemType == EMI_DEV_MOBILE_DDR)
#endif
{
    // Set up the pinmux for the EMI
    hw_emi_ConfigureEmiPins(
        PIN_VOLTAGE_1pt8V, 
        EMI_PIN_DRIVE_ADDRESS,
        EMI_PIN_DRIVE_DATA,
        EMI_PIN_DRIVE_CHIP_ENABLE,
        EMI_PIN_DRIVE_CLOCK,
        EMI_PIN_DRIVE_CONTROL);

}


//    if (device == EMI_DEV_MOBILE_DDR) {
    hw_emi_DisableEmiPadKeepers();

    // Set the default EMICLK_DELAY value
    //HW_DIGCTL_EMICLK_DELAY_WR(emi_clk_delay);
//    hw_emi_ChgEmiClkCrossMode(EMI_CLK_24MHz);


#if 0
    //disable interrupts, so nothing can access SDRAM while we're doing this.
    bInterruptEnableState = hw_core_EnableIrqInterrupt(false);

    // flush and invalidate the data cache to ensure nothing gets written to sdram while
    // we are changing the emi clk
    hw_core_invalidate_clean_DCache();
    // make sure data written
    hw_core_drain_write_buffer();
#endif

    ddi_emi_ExitAutoMemorySelfRefreshMode();
    ddi_emi_ExitAutoMemoryClockGateMode();

    Rtn = ddi_emi_EnterStaticSelfRefreshMode();
    if(Rtn != SUCCESS)
    {
        return Rtn;
    }
#if 0
    if(MemType == EMI_DEV_SDRAM)
    {
        hw_dram_Init_sdram_mt48lc32m16a2_24MHz();
    }
	else if(MemType == EMI_DEV_MOBILE_SDRAM)
    {
        hw_dram_Init_mobile_sdram_k4m56163pg_7_5_24MHz();
        //hw_dram_Init_mobile_sdram_mt48h16m16lf_7_5_24MHz();

    }
	else
    if(MemType == EMI_DEV_MOBILE_DDR)
#endif
    {
        // Write the Databahn SDRAM setup register values
        hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_24MHz();
    }

    HW_DRAM_CTL14.B.CS_MAP = ChipSelectMask;


    // here we calculate the TotalDieSize for to get the COLUMN_SIZE and
    // ADDR_PINS per die.  We want to avoid doing a regular division which
    // calls a function that might be located in SDRAM (very bad right now!)

    if(NumChipSelects==1)
        TotalDieSize = TotalDramSize;
    else
        TotalDieSize = TotalDramSize >> (NumChipSelects - 1);


    if(TotalDieSize == EMI_2MB_DRAM)
    {
        HW_DRAM_CTL11.B.COLUMN_SIZE = 4;
        HW_DRAM_CTL10.B.ADDR_PINS = 2;
    }
    else if(TotalDieSize == EMI_4MB_DRAM)
    {
        HW_DRAM_CTL11.B.COLUMN_SIZE = 4;
        HW_DRAM_CTL10.B.ADDR_PINS = 1;
    }
    else if(TotalDieSize == EMI_8MB_DRAM)
    {
        HW_DRAM_CTL11.B.COLUMN_SIZE = 4;
        HW_DRAM_CTL10.B.ADDR_PINS = 1;
    }
    else if(TotalDieSize == EMI_16MB_DRAM)
    {
        HW_DRAM_CTL11.B.COLUMN_SIZE = 3;
        HW_DRAM_CTL10.B.ADDR_PINS = 1;
    }
    else if(TotalDieSize == EMI_32MB_DRAM)
    {
        HW_DRAM_CTL11.B.COLUMN_SIZE = 3;
        HW_DRAM_CTL10.B.ADDR_PINS = 0;
    }
    else if(TotalDieSize == EMI_64MB_DRAM)
    {
        HW_DRAM_CTL11.B.COLUMN_SIZE = 2;
        HW_DRAM_CTL10.B.ADDR_PINS = 0;
    }
    else
    {
        return ERROR_DDI_EMI_SDRAM_NOT_SUPPORTED;
    }
    
    if( (TotalDieSize == EMI_2MB_DRAM) &&
        (s_ddi_emi_vars.MemType == EMI_DEV_MOBILE_SDRAM) )
    {
        // For some reason, this must be done for ESMT mobile sdram parts.
        HW_DRAM_CTL05.B.EN_LOWPOWER_MODE = 0;
  
    }
    // Start the EMI controller (initializes the SDRAM)
    HW_DRAM_CTL08.B.START = 1;


#if 0
	StartTime = hw_digctl_GetCurrentTime();
#endif
    // todo:  Add timeout and error
    while (HW_CLKCTRL_EMI_RD() &
            (BM_CLKCTRL_EMI_BUSY_REF_EMI  |
             BM_CLKCTRL_EMI_BUSY_DCC_RESYNC))
             {}
#if 0
    {
        if(hw_digctl_CheckTimeOut(StartTime, 10000))
        {
            return ERROR_DDI_EMI_RESYNC_TIMEOUT;
        }
    }
#endif

    Rtn = ddi_emi_ExitStaticSelfRefreshMode();
#if 0
    if(s_ddi_emi_vars.MemType == EMI_DEV_MOBILE_SDRAM)
    {
        //controller bug requires this
        HW_DRAM_CTL08.B.SDR_MODE = 0;
        
        HW_DRAM_CTL38.B.EMRS1_DATA = EXTENDED_MODE_REGISTER_VALUE;
        HW_DRAM_CTL09.B.WRITE_MODEREG = 1;
        
        //controller bug requires this
        HW_DRAM_CTL08.B.SDR_MODE = 1;
    }
    else if(s_ddi_emi_vars.MemType == EMI_DEV_MOBILE_DDR)
#endif
    {
        //controller bug requires this
        HW_DRAM_CTL08.B.SDR_MODE = 1;
        
        HW_DRAM_CTL38.B.EMRS1_DATA = EXTENDED_MODE_REGISTER_VALUE;
        HW_DRAM_CTL09.B.WRITE_MODEREG = 1;
        
        //controller bug requires this
        HW_DRAM_CTL08.B.SDR_MODE = 0;
    }

    ddi_emi_EnterAutoMemoryClockGateMode();

#if 0
    hw_core_EnableIrqInterrupt(bInterruptEnableState);
#endif
    return Rtn;

}
#endif

void ddi_emi_start_function_for_locating(void)
{




}





hw_emi_ClockState_t ddi_emi_GetCurrentSpeedState(void)
{
    return s_ddi_emi_vars.EmiClkSpeedState;
}

void ddi_emi_PrepareForDramSelfRefreshMode(void)
{
// NOTE !!! done before enter ddi_emi functions in linux
#if 0
    // invalidate instruction cache
    //hw_core_invalidate_ICache();
    
    // clean the data cache to ensure nothing gets written to sdram while
    // we are in self-refresh mode
    hw_core_clean_DCache();


    // make sure data written    
    hw_core_drain_write_buffer();
    
#endif
}


RtStatus_t ddi_emi_EnterStaticSelfRefreshMode(void)
{

    ddi_emi_PrepareForDramSelfRefreshMode();

    hw_emi_EnterMemorySelfRefreshMode(TRUE);
#if 0
    uint32_t StartTime;
	StartTime = hw_digctl_GetCurrentTime();

    while(!hw_emi_IsControllerHalted())
    {
        if(hw_digctl_CheckTimeOut(StartTime, 1000))
        {
            return ERROR_DDI_EMI_ENTER_SELF_REFRESH_TIMEOUT;
        }
    }
#else
	while(!hw_emi_IsControllerHalted());
#endif
    return SUCCESS;

}


// Function Specification ******************************************************
//!
//! \brief DRAM Leave Device Self-Refresh Mode
//!
//! This function takes the DRAM controller out of device self-refresh mode.
//!
// End Function Specification **************************************************

RtStatus_t ddi_emi_ExitStaticSelfRefreshMode(void)
{
    uint32_t StartTime;

    hw_emi_EnterMemorySelfRefreshMode(FALSE);
#if 0
    StartTime = hw_digctl_GetCurrentTime();

    while(hw_emi_IsControllerHalted())
    {
        if(hw_digctl_CheckTimeOut(StartTime, 1000))
        {
            return ERROR_DDI_EMI_EXIT_SELF_REFRESH_TIMEOUT;
        }
    }
#else
	while(hw_emi_IsControllerHalted());
#endif
    return SUCCESS;
}


void ddi_emi_EnterAutoMemoryClockGateMode(void)
{

	hw_emi_SetAutoMemoryClockGateIdleCounterTimeoutCycles(8);
	hw_emi_SetMemoryClockGateAutoFlag(TRUE);
	hw_emi_EnterMemoryClockGateMode(TRUE);
	// not using global variables
	//s_ddi_emi_vars.bAutoMemoryClockGateModeEnabled = TRUE;
  
}

void ddi_emi_ExitAutoMemoryClockGateMode(void)
{
    hw_emi_EnterMemoryClockGateMode(FALSE);
	hw_emi_SetMemoryClockGateAutoFlag(FALSE);
	// not using global variables
	//s_ddi_emi_vars.bAutoMemoryClockGateModeEnabled = FALSE;
    hw_emi_SetAutoMemoryClockGateIdleCounterTimeoutCycles(0);
    
}

void ddi_emi_EnterAutoMemorySelfRefreshMode(void)
{
    hw_emi_SetAutoMemorySelfRefreshIdleCounterTimeoutCycles(64);
    hw_emi_SetMemorySelfRefeshAutoFlag(TRUE);
	hw_emi_EnterMemorySelfRefreshMode(TRUE);
	// not using global variables
	//s_ddi_emi_vars.bAutoMemorySelfRefreshModeEnabled = TRUE;
}


void ddi_emi_ExitAutoMemorySelfRefreshMode(void)
{

    hw_emi_EnterMemorySelfRefreshMode(FALSE);
    hw_emi_SetMemorySelfRefeshAutoFlag(FALSE);
	// not using global variables
	//s_ddi_emi_vars.bAutoMemorySelfRefreshModeEnabled = FALSE;
    hw_emi_SetAutoMemorySelfRefreshIdleCounterTimeoutCycles(0);
}




// The following is the Validation code which sets up our Development board DDR chips.
// Refined SDK to be added later.  - RDL March, 15, 2007.


// Function Specification ******************************************************
//!
//! \brief Wait for REF_EMI and DCC_RESYNC Not Busy
//!
//! This function waits for the CLKCTRL block to go not busy with respect to
//! the BUSY_REF_EMI and DCC_RESYNC signals.
//!
// End Function Specification **************************************************

RtStatus_t ddi_emi_WaitForRefEmiAndDccResyncNotBusy()
{
    uint32_t timeout_start = HW_DIGCTL_MICROSECONDS_RD();

    while (HW_CLKCTRL_EMI_RD() &
            (BM_CLKCTRL_EMI_BUSY_REF_EMI  |
             BM_CLKCTRL_EMI_BUSY_DCC_RESYNC)) {

        uint32_t timeout_end = HW_DIGCTL_MICROSECONDS_RD();
        if ((timeout_end - timeout_start) >= 100) {

            //printf("Timeout on EMI_BUSY_REF_EMI or DCC_RESYNC\n");
            return ERROR_DDI_EMI_RESYNC_TIMEOUT;
        }
    }
    return SUCCESS;
}

// Function Specification ******************************************************
//!
//! \brief Wait for REF_XTAL and DCC_RESYNC Not Busy
//!
//! This function waits for the CLKCTRL block to go not busy with respect to
//! the BUSY_REF_XTAL and DCC_RESYNC signals.
//!
// End Function Specification **************************************************

RtStatus_t ddi_emi_WaitForRefXtalAndDccResyncNotBusy()
{
    uint32_t timeout_start = HW_DIGCTL_MICROSECONDS_RD();


    while (HW_CLKCTRL_EMI_RD() &
            (BM_CLKCTRL_EMI_BUSY_REF_XTAL  |
             BM_CLKCTRL_EMI_BUSY_DCC_RESYNC)) {

        uint32_t timeout_end = HW_DIGCTL_MICROSECONDS_RD();
        if ((timeout_end - timeout_start) >= 100) {

            //printf("Timeout on EMI_BUSY_REF_XTAL or DCC_RESYNC\n");
            return ERROR_DDI_EMI_RESYNC_TIMEOUT;
        }
    }
    return SUCCESS;
}


// Function Specification ******************************************************
//!
//! \brief Wait for REF_EMI, REF_XTAL and DCC_RESYNC Not Busy
//!
//! This function waits for the CLKCTRL block to go not busy with respect to
//! the BUSY_REF_EMI, BUSY_REF_XTAL and DCC_RESYNC signals.
//!
// End Function Specification **************************************************

RtStatus_t ddi_emi_WaitForRefEmiRefXtalAndDccResyncNotBusy()
{
    uint32_t timeout_start = HW_DIGCTL_MICROSECONDS_RD();

    while (HW_CLKCTRL_EMI_RD() &
            (BM_CLKCTRL_EMI_BUSY_REF_EMI   |
             BM_CLKCTRL_EMI_BUSY_REF_XTAL  |
             BM_CLKCTRL_EMI_BUSY_DCC_RESYNC)) {

        uint32_t timeout_end = HW_DIGCTL_MICROSECONDS_RD();
        if ((timeout_end - timeout_start) >= 100) {

            //printf("Timeout on EMI_BUSY_REF_EMI, EMI_BUSY_REF_XTAL or DCC_RESYNC\n");
            return ERROR_DDI_EMI_RESYNC_TIMEOUT;
        }
    }
    return SUCCESS;
}

// Function Specification ******************************************************
//!
//! \brief Change EMI_CLK Frequency (Cross-Mode)
//!
//! This function programs the EMI_CLK to the specified frequency by selecting
//! the appropriate clock source (XTAL or PLL) and divider settings.
//!
//! \param[in] emi_freq - EMI_CLK Frequency
//!
// End Function Specification **************************************************

RtStatus_t ddi_emi_PerformFrequencyChange(hw_emi_ClockState_t emi_freq, bool bDccResync)
{

    RtStatus_t status;
    //uint32_t DebugEndTime, DebugStartTime = hw_digctl_GetCurrentTime();

    uint32_t dcc_resync_enable = 1;

    int use_xtal_src = 1;

    uint32_t new_xtal_int_div = 1;

    uint32_t cur_pll_int_div = (HW_CLKCTRL_EMI_RD() & BM_CLKCTRL_EMI_DIV_EMI) >> BP_CLKCTRL_EMI_DIV_EMI;
    uint32_t new_pll_int_div = 4;

    uint32_t cur_pll_frac_div = (HW_CLKCTRL_FRAC_RD() & BM_CLKCTRL_FRAC_EMIFRAC) >> BP_CLKCTRL_FRAC_EMIFRAC;
    uint32_t new_pll_frac_div = 34;

    uint32_t cur_xtal_div = (HW_CLKCTRL_EMI_RD() & BM_CLKCTRL_EMI_DIV_XTAL) >> BP_CLKCTRL_EMI_DIV_XTAL;


    HW_CLKCTRL_EMI_CLR(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);

    switch (emi_freq)
    {
#if USE_OLD_DIVIDER_VALUES

        case EMI_CLK_6MHz:
            use_xtal_src = 1;
            new_xtal_int_div = 4;
            break;

        case EMI_CLK_24MHz:
            use_xtal_src = 1;
            new_xtal_int_div = 1;
            break;

        case EMI_CLK_48MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 10;
            break;

        case EMI_CLK_60MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 8;
            break;

        case EMI_CLK_96MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 5;
            break;

        case EMI_CLK_120MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 18;
            new_pll_int_div = 4;
            break;

        case EMI_CLK_133MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 22;
            new_pll_int_div = 3;
            break;
#else


        case EMI_CLK_6MHz:
            use_xtal_src = 1;
            new_xtal_int_div = 4;
            break;

        case EMI_CLK_24MHz:
            use_xtal_src = 1;
            new_xtal_int_div = 1;
            break;

        case EMI_CLK_48MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 30;
            new_pll_int_div = 6;
            break;

        case EMI_CLK_60MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 29;
            new_pll_int_div = 5;
            break;

        case EMI_CLK_96MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 30;
            new_pll_int_div = 3;
            break;

        case EMI_CLK_120MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 35;
            new_pll_int_div = 2;
            break;

        case EMI_CLK_133MHz:
            use_xtal_src = 0;
            new_pll_frac_div = 33;
            new_pll_int_div = 2;
            break;
#endif
    }


    if (use_xtal_src)
    {
        // Write the XTAL EMI clock divider
        HW_CLKCTRL_EMI_WR(
                BF_CLKCTRL_EMI_CLKGATE(0) |
                BF_CLKCTRL_EMI_DCC_RESYNC_ENABLE(dcc_resync_enable) |
                BF_CLKCTRL_EMI_DIV_XTAL(new_xtal_int_div) |
                BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div));

        // Wait until the BUSY_REF_XTAL clears or a timeout occurs
        status = ddi_emi_WaitForRefXtalAndDccResyncNotBusy();
        if(status != SUCCESS)
            return status;

        // Set the PLL EMI-bypass bit so that the XTAL clock takes over
        HW_CLKCTRL_CLKSEQ_SET(BM_CLKCTRL_CLKSEQ_BYPASS_EMI);

        // Wait until the BUSY_REF_XTAL clears or a timeout occurs
        status = ddi_emi_WaitForRefXtalAndDccResyncNotBusy();
        if(status != SUCCESS)
            return status;

    }
    else
    {
        // Determine if both the fractional divider and the integer divider must
        // be updated. If so, then on the first change turn off DCC_RESYNC
        if ((new_pll_frac_div != cur_pll_frac_div) &&
            (new_pll_int_div != cur_pll_int_div)) {

            HW_CLKCTRL_EMI_CLR(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 0;
        }

        // The fractional divider and integer divider must be written in such
        // an order to guarantee that when going from a lower frequency to a
        // higher frequency that any intermediate frequencies do not exceed
        // the final frequency. For this reason, we must make sure to check
        // the current divider values with the new divider values and write
        // them in the correct order.
        if (new_pll_frac_div > cur_pll_frac_div) {
            // Write the PLL fractional divider
            uint32_t frac_val = (HW_CLKCTRL_FRAC_RD() & ~BM_CLKCTRL_FRAC_EMIFRAC) |
                BF_CLKCTRL_FRAC_EMIFRAC(new_pll_frac_div);

            HW_CLKCTRL_FRAC_WR(frac_val);
            cur_pll_frac_div = new_pll_frac_div;

            // Wait until the BUSY_REF_EMI clears or a timeout occurs
            status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
            if(status != SUCCESS)
                return status;

            HW_CLKCTRL_EMI_SET(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 1;
        }

        if (new_pll_int_div > cur_pll_int_div) {
            // Write the PLL EMI clock divider
            HW_CLKCTRL_EMI_WR(
                    BF_CLKCTRL_EMI_CLKGATE(0) |
                    BF_CLKCTRL_EMI_DCC_RESYNC_ENABLE(dcc_resync_enable) |
                    BF_CLKCTRL_EMI_DIV_XTAL(cur_xtal_div) |
                    BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div));
            cur_pll_int_div = new_pll_int_div;

            // Wait until the BUSY_REF_EMI clears or a timeout occurs
            status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
            if(status != SUCCESS)
                return status;

            HW_CLKCTRL_EMI_SET(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 1;
        }

        if (new_pll_frac_div != cur_pll_frac_div) {
            // Write the PLL fractional divider
            uint32_t frac_val = (HW_CLKCTRL_FRAC_RD() & ~BM_CLKCTRL_FRAC_EMIFRAC) |
                BF_CLKCTRL_FRAC_EMIFRAC(new_pll_frac_div);

            HW_CLKCTRL_FRAC_WR(frac_val);

            // Wait until the BUSY_REF_EMI clears or a timeout occurs
            status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
            if(status != SUCCESS)
                return status;

            HW_CLKCTRL_EMI_SET(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 1;
        }

        if (new_pll_int_div != cur_pll_int_div) {
            // Write the PLL EMI clock divider
            HW_CLKCTRL_EMI_WR(
                    BF_CLKCTRL_EMI_CLKGATE(0) |
                    BF_CLKCTRL_EMI_DCC_RESYNC_ENABLE(dcc_resync_enable) |
                    BF_CLKCTRL_EMI_DIV_XTAL(cur_xtal_div) |
                    BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div));

            // Wait until the BUSY_REF_EMI clears or a timeout occurs
            status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
            if(status != SUCCESS)
                return status;

            HW_CLKCTRL_EMI_SET(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 1;
        }

        // Clear the PLL EMI-bypass bit so that the PLL clock takes over
        HW_CLKCTRL_CLKSEQ_CLR(BM_CLKCTRL_CLKSEQ_BYPASS_EMI);

        // Wait until the BUSY_REF_EMI clears or a timeout occurs
        status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
        if(status != SUCCESS)
            return status;
    }


#if 0

    if (use_xtal_src)
    {
        // Write the XTAL EMI clock divider
        HW_CLKCTRL_EMI_WR(
                BF_CLKCTRL_EMI_CLKGATE(0) |
                BF_CLKCTRL_EMI_DCC_RESYNC_ENABLE(dcc_resync_enable) |
                BF_CLKCTRL_EMI_DIV_XTAL(new_xtal_int_div) |
                BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div));

        // Wait until the BUSY_REF_XTAL clears or a timeout occurs
        status = ddi_emi_WaitForRefXtalAndDccResyncNotBusy();
        if(status != SUCCESS)
            return status;

        // Set the PLL EMI-bypass bit so that the XTAL clock takes over
        HW_CLKCTRL_CLKSEQ_SET(BM_CLKCTRL_CLKSEQ_BYPASS_EMI);

        // Wait until the BUSY_REF_XTAL clears or a timeout occurs
        status = ddi_emi_WaitForRefXtalAndDccResyncNotBusy();
        if(status != SUCCESS)
            return status;

                // Set the PLL EMI-bypass bit so that the XTAL clock takes over
        HW_CLKCTRL_CLKSEQ_SET(BM_CLKCTRL_CLKSEQ_BYPASS_EMI);

        //DebugEndTime = hw_digctl_GetCurrentTime();

    //  printf("DIFFERENCE: %i", DebugEndTime - DebugStartTime);


    }
    else
    {

        // Determine if both the fractional divider and the integer divider must
        // be updated. If so, then on the first change turn off DCC_RESYNC
        if ((new_pll_frac_div != cur_pll_frac_div) &&
            (new_pll_int_div != cur_pll_int_div)) {

            HW_CLKCTRL_EMI_CLR(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 0;
        }

        // The fractional divider and integer divider must be written in such
        // an order to guarantee that when going from a lower frequency to a
        // higher frequency that any intermediate frequencies do not exceed
        // the final frequency. For this reason, we must make sure to check
        // the current divider values with the new divider values and write
        // them in the correct order.
        if (new_pll_frac_div > cur_pll_frac_div) {
            // Write the PLL fractional divider
            uint32_t frac_val = (HW_CLKCTRL_FRAC_RD() & ~BM_CLKCTRL_FRAC_EMIFRAC) |
                BF_CLKCTRL_FRAC_EMIFRAC(new_pll_frac_div);

            HW_CLKCTRL_FRAC_WR(frac_val);
            cur_pll_frac_div = new_pll_frac_div;

            // Wait until the BUSY_REF_EMI clears or a timeout occurs
            status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
            if(status != SUCCESS)
                return status;

            HW_CLKCTRL_EMI_SET(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 1;
        }

        if (new_pll_int_div > cur_pll_int_div) {
            // Write the PLL EMI clock divider
            HW_CLKCTRL_EMI_WR(
                    BF_CLKCTRL_EMI_CLKGATE(0) |
                    BF_CLKCTRL_EMI_DCC_RESYNC_ENABLE(dcc_resync_enable) |
//                    BF_CLKCTRL_EMI_DIV_XTAL(new_xtal_int_div) |
                    BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div));
            cur_pll_int_div = new_pll_int_div;

            // Wait until the BUSY_REF_EMI clears or a timeout occurs
            status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
            if(status != SUCCESS)
                return status;

            HW_CLKCTRL_EMI_SET(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 1;
        }

        //if (new_pll_frac_div != cur_pll_frac_div) {
        if (new_pll_frac_div < cur_pll_frac_div) {
            // Write the PLL fractional divider
            uint32_t frac_val = (HW_CLKCTRL_FRAC_RD() & ~BM_CLKCTRL_FRAC_EMIFRAC) |
                BF_CLKCTRL_FRAC_EMIFRAC(new_pll_frac_div);

            HW_CLKCTRL_FRAC_WR(frac_val);

            // Wait until the BUSY_REF_EMI clears or a timeout occurs
            status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
            if(status != SUCCESS)
                return status;

            HW_CLKCTRL_EMI_SET(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 1;
        }

        //if (new_pll_int_div != cur_pll_int_div) {
        if (new_pll_int_div < cur_pll_int_div) {
            // Write the PLL EMI clock divider
            HW_CLKCTRL_EMI_WR(
                    BF_CLKCTRL_EMI_CLKGATE(0) |
                    BF_CLKCTRL_EMI_DCC_RESYNC_ENABLE(dcc_resync_enable) |
//                    BF_CLKCTRL_EMI_DIV_XTAL(new_xtal_int_div) |
                    BF_CLKCTRL_EMI_DIV_EMI(new_pll_int_div));

            // Wait until the BUSY_REF_EMI clears or a timeout occurs
            status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();
            if(status != SUCCESS)
                return status;

            HW_CLKCTRL_EMI_SET(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
            dcc_resync_enable = 1;
        }

        // Clear the PLL EMI-bypass bit so that the PLL clock takes over
        HW_CLKCTRL_CLKSEQ_CLR(BM_CLKCTRL_CLKSEQ_BYPASS_EMI);

        status = ddi_emi_WaitForRefEmiAndDccResyncNotBusy();




    }
    #endif

    HW_CLKCTRL_EMI_CLR(BM_CLKCTRL_EMI_DCC_RESYNC_ENABLE);
    return status;


}



RtStatus_t ddi_emi_ChangeClockFrequency (int emiclk_new)
{
	hw_emi_ClockState_t EmiClockSetting;
//	bool         bPrevIntState;
    RtStatus_t status;

	while (HW_DRAM_CTL15.B.PORT_BUSY); //wait until dram Port is not busy, add 20090111, dhsong 

	// clock setting is checked before this call
	#if 0
	// restricted speeds no longer needed.
#if FAILSAFE_ENABLED
   if(EmiClockSetting > EMI_CLK_96MHz)
       EmiClockSetting = EMI_CLK_96MHz;
#endif

    if(s_ddi_emi_vars.MemType==EMI_DEV_MOBILE_SDRAM)
    {
        if(EmiClockSetting > EMI_CLK_96MHz)
            EmiClockSetting = EMI_CLK_96MHz;
    }

	if(EmiClockSetting == s_ddi_emi_vars.EmiClkSpeedState)
		return SUCCESS;
	#else
	#endif

	if (emiclk_new <= EMI_CLK_6MHz*1000) {
		EmiClockSetting = EMI_CLK_6MHz;
	}
	else if (emiclk_new <= EMI_CLK_24MHz*1000) {
		EmiClockSetting = EMI_CLK_24MHz;
	}
	else if (emiclk_new <= EMI_CLK_48MHz*1000) {
		EmiClockSetting = EMI_CLK_48MHz;
	}
	else if (emiclk_new <= EMI_CLK_60MHz*1000) {
		EmiClockSetting = EMI_CLK_60MHz;
	}
	else if (emiclk_new <= EMI_CLK_96MHz*1000) {
		EmiClockSetting = EMI_CLK_96MHz;
	}
	else if (emiclk_new <= EMI_CLK_120MHz*1000) {
		EmiClockSetting = EMI_CLK_120MHz;
	}
	else {
		EmiClockSetting = EMI_CLK_133MHz;
	}

	// NOTE!!! already done before this call in linux
	// disable interrupts
    //bPrevIntState = hw_core_EnableIrqInterrupt(FALSE);
    
    status = ddi_emi_OsDisabledChangeClockFrequency(EmiClockSetting);

	// already done before this call in linux
    //hw_core_EnableIrqInterrupt(bPrevIntState);
    return status;
}


// Function Specification ******************************************************
//!
//! \brief Adjust the DLL Startpoint based on VDDD TRG value
//!
//! \param[in] emiclk_freq
//!
// End Function Specification **************************************************
void ddi_emi_AdjustDllStartPoint(hw_emi_ClockState_t emiclk_freq)
{
    int Vddd1_2_Dll_Lock_133 = 34;
    int Vddd1_2_Dll_Lock_120 = 38;
    int Vddd1_2_Dll_Lock_96 = 44;

    int Nominal_Dll_Lock = 0;
    int temp1;

    if(emiclk_freq==EMI_CLK_133MHz)
    {
    	Nominal_Dll_Lock = Vddd1_2_Dll_Lock_133;
    }
    else if(emiclk_freq==EMI_CLK_120MHz)
    {
    	Nominal_Dll_Lock = Vddd1_2_Dll_Lock_120;
    }
    else if(emiclk_freq==EMI_CLK_96MHz)
    {
    	Nominal_Dll_Lock = Vddd1_2_Dll_Lock_96;
    }

    // Startpoint correction
    if(Nominal_Dll_Lock)
    {
		    temp1 = HW_POWER_VDDDCTRL.B.TRG - 16;
		    HW_DRAM_CTL17.B.DLL_START_POINT = Nominal_Dll_Lock + (temp1 << 1);  // temp * 2
	}

}


RtStatus_t ddi_emi_OsDisabledChangeClockFrequency (hw_emi_ClockState_t EmiClockSetting)
{

    RtStatus_t status;

	// NOTE!!!
	// not using global varaibles in linux
	// 	### just assumming selfrefresh is disabled
	// 	### just assumming clockgate is enabled
    bool bAutoMemorySelfRefreshState = 0; //s_ddi_emi_vars.bAutoMemorySelfRefreshModeEnabled;
    bool bAutoMemoryClockGateState = 1; //s_ddi_emi_vars.bAutoMemoryClockGateModeEnabled;

    ddi_emi_ExitAutoMemorySelfRefreshMode();
    ddi_emi_ExitAutoMemoryClockGateMode();
    
    status = ddi_emi_EnterStaticSelfRefreshMode();

    if(status != SUCCESS)
    {
        ddi_emi_ExitStaticSelfRefreshMode();
        //SystemHalt();
        return status;
    }

    hw_emi_PrepareControllerForNewFrequency(EmiClockSetting, EMI_DEV_MOBILE_DDR);
    ddi_emi_AdjustDllStartPoint(EmiClockSetting);

	// NOTE!!! bDccResync is not used right now
#if 0
    if(
        ((uint16_t)s_ddi_emi_vars.EmiClkSpeedState <=
            (uint16_t)DDI_EMI_DDR_MAX_NO_DCCRESYNC_FREQ_MHZ)
            &&
        ( (uint16_t)EmiClockSetting <=
            (uint16_t)DDI_EMI_DDR_MAX_NO_DCCRESYNC_FREQ_MHZ)
      )
    {

        status = ddi_emi_PerformFrequencyChange(EmiClockSetting, false);
        // perform in mode frequency change
    }
    else
#endif
    {
        status = ddi_emi_PerformFrequencyChange(EmiClockSetting, true);
    }
	// not using global varaibles in linux
    //s_ddi_emi_vars.EmiClkSpeedState = EmiClockSetting;

    if(status!=SUCCESS)
    {
        ddi_emi_ExitStaticSelfRefreshMode();
        //SystemHalt();
        return status;
    }

	status = ddi_emi_ExitStaticSelfRefreshMode();
    
    if(status!=SUCCESS)
    {
        ddi_emi_ExitStaticSelfRefreshMode();
        //SystemHalt();
        return status;
    }

    if(bAutoMemoryClockGateState==TRUE)
        ddi_emi_EnterAutoMemoryClockGateMode();

    if(bAutoMemorySelfRefreshState==TRUE)
        ddi_emi_EnterAutoMemorySelfRefreshMode();

    return status;

}

int delay_func()
{
	uint32_t cur_time=0;
	uint32_t end_time=0;
	
	cur_time = HW_RTC_MILLISECONDS_RD(); //HW_DIGCTL_MICROSECONDS_RD();

	while(true) {
		end_time = HW_RTC_MILLISECONDS_RD(); //HW_DIGCTL_MICROSECONDS_RD();
		if ( (end_time-cur_time) > 30) //30ms
			break;
	} 
	return 1;
}

int stmp37xx_enter_idle (int notused)
{
	uint32_t current_time = 0;
	uint32_t pswitch_status = 0; //if 0, while loop
	uint32_t pswitch = 0;
	uint32_t prev_pswitch = 0;

	ddi_emi_EnterAutoMemorySelfRefreshMode();

	current_time = HW_RTC_SECONDS_RD();

	// Sit in a loop waiting for a button press
	//while (!HW_POWER_STS.B.PSWITCH && !HW_POWER_STS.B.VDD5V_GT_VDDIO) {
	while ( !( (HW_POWER_STS.B.PSWITCH == 1) && pswitch_status) && !HW_POWER_STS.B.VDD5V_GT_VDDIO) {
#if 0
		asm("mov r0, #0");
		asm("mcr p15,0,r0,c7,c0,4");
        asm("nop");
#endif
	
#if 1
	//while ( !HW_POWER_STS.B.VDD5V_GT_VDDIO) {

	pswitch = HW_POWER_STS.B.PSWITCH;

	//else pswitch_status  = 0;
	
	if (pswitch == 1) {
		delay_func();
		pswitch = HW_POWER_STS.B.PSWITCH;
		if ( pswitch==1 )  	
			pswitch_status = 1;
		else pswitch_status = 0;
	}
#else
	if (HW_POWER_STS.B.PSWITCH == 0)
		pswitch_status = 1;
#endif

#if 1 
		/* 24hour sleep before power down */
		if ((HW_RTC_SECONDS_RD() - current_time) > 60*60*24) {
			HW_POWER_RESET_WR(POWERDOWN_KEY << 16);
			HW_POWER_RESET_WR((POWERDOWN_KEY << 16) | BM_POWER_RESET_PWD);
		}
#else //add for idle-wakeup test, dhsong
		if ((HW_RTC_SECONDS_RD() - current_time) > 1) {
			break;
		}
#endif
	}

	ddi_emi_ExitAutoMemorySelfRefreshMode();
	return 0;
}

void ddi_emi_end_function_for_locating(void)
{
}
//#pragma ghs section




