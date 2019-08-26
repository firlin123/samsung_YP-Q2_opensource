////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_clocks
//! @{
//
// Copyright (c) 2004-2007 SigmaTel, Inc.
//
//! \file    hw_clocks.h
//! \brief   
//! \version 
//! \date    
////////////////////////////////////////////////////////////////////////////////
#ifndef _HW_CLOCKS_H
#define _HW_CLOCKS_H 1

// These are here so app_PreTxInit will compile.  
#define CPU_SPEED_MAX 250
#define CPU_SPEED_MIN 12

////////////////////////////////////////////////////////////////////////////////
// Enums
////////////////////////////////////////////////////////////////////////////////

//! \brief Bit positions for clock gates controlled in HW_CLKCTRL_XTAL
//!
//! Bit positions allow access to each clock gate.  
typedef enum _hw_clkctrl_xtal_gate_t
{
    //! \brief UART clock gate bit position.
    UART_CLK_GATE       = 0x80000000, 
    //! \brief Digital filter's 24MHz clock gate bit position.
    FILT_CLK24M_GATE    = 0x40000000,
    //! \brief PWM's 24MHz clock gate bit position.
    PWM_CLK24M_GATE     = 0x20000000,
    //! \brief DRI clock gate bit position.
    DRI_CLK24M_GATE     = 0x10000000,
    //! \brief Digital control's 1MHz clock gate bit position.
    DIGCTRL_CLK1M_GATE  = 0x08000000,
    //! \brief Timer and rotary encoder's 32kHz clock gate bit position.
    TIMROT_CLK32K_GATE  = 0x04000000
} hw_clkctrl_xtal_gate_t;

//! \brief Bit positions to control PLL bypass.
typedef enum _hw_clkctrl_bypass_clk_t
{
    //! \brief Bit position to bypass ref_cpu and use the crystal for the CPU clock.
    BYPASS_CPU  = 0x80,
    //! \brief Bit position to bypass ref_emi and use the crystal for the EMI clock.
    BYPASS_EMI  = 0x40,
    //! \brief Bit position to bypass ref_io and use the crystal for the SSP clock.
    BYPASS_SSP  = 0x20,
    //! \brief Bit position to bypass ref_io and use the crystal for the GPMI clock.
    BYPASS_GPMI = 0x10,
    //! \brief Bit position to bypass ref_io and use the crystal for the IR clock.
    BYPASS_IR   = 0x08,
    //! \brief Bit position to bypass ref_pix and use the crystal for the display clock.
    BYPASS_PIX  = 0x02,
    //! \brief Bit position to bypass ref_pll and use the crystal for the SAIF clock.
    BYPASS_SAIF = 0x01,
} hw_clkctrl_bypass_clk_t;

//! \brief Register settings for HBUS clock slow divide.
typedef enum _hw_clkctrl_hclk_slow_div_t
{
    //! \brief HBUS clock is equal to CPU clock divided by 1.
    SLOW_DIV_BY1  = 0x0,
    //! \brief HBUS clock is equal to CPU clock divided by 2.
    SLOW_DIV_BY2  = 0x1,
    //! \brief HBUS clock is equal to CPU clock divided by 4.
    SLOW_DIV_BY4  = 0x2,
    //! \brief HBUS clock is equal to CPU clock divided by 8.
    SLOW_DIV_BY8  = 0x3,
    //! \brief HBUS clock is equal to CPU clock divided by 16.
    SLOW_DIV_BY16 = 0x4,
    //! \brief HBUS clock is equal to CPU clock divided by 32.
    SLOW_DIV_BY32 = 0x5,
    //! \brief Maximum divider. HBUS clock is equal to CPU clock divided by 32.
    SLOW_DIV_MAX = SLOW_DIV_BY32
} hw_clkctrl_hclk_slow_div_t;

//! \brief Modules with capability to be auto-slowed
typedef enum _hw_clkctrl_autoslow_t
{
    //! \brief Enable auto-slow mode based on APBH DMA activity.
    APBHDMA_AUTOSLOW,
    //! \brief Enable auto-slow mode based on APBX DMA activity.
    APBXDMA_AUTOSLOW,     
    //! \brief Enable auto-slow mode when less than three masters are trying to use the AHB.
    TRAFFIC_JAM_AUTOSLOW, 
    //! \brief Enable auto-slow mode based on AHB master activity.
    TRAFFIC_AUTOSLOW,     
    //! \brief Enable auto-slow mode based on with CPU Data access to AHB.
    CPU_DATA_AUTOSLOW,    
    //! \brief Enable auto-slow mode based on with CPU Instruction access to AHB.
    CPU_INSTR_AUTOSLOW     
} hw_clkctrl_autoslow_t;


////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief 
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_Init(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief 
//!
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_InitPowerSavings(void);
RtStatus_t hw_clkctrl_UndoPowerSavings(void);



/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets divider for the crystal reference clock (ref_xtal)
//! 
//! \fntype Function                        
//!     
//! This field controls the divider connected to the crystal reference clock 
//! that drives the CLK_P domain when bypass is selected.  This divides the 
//! ref_xtal clock.  The divider can be an integer or fractional divider.
//!
//! \param[in] u32Div - Integer or fractional divide value. 
//! \param[in] bDivFracEn - TRUE to specify u32Div as a fractional divider. FALSE for integer
//!
//! \retval SUCCESS - Call was successful.
//! \return ERROR_HW_CLKCTRL_DIV_BY_ZERO - u32Div should not be zero
//!
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0.
//! \notes HW_CLKCTRL_CPU.B.DIV_XTAL is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPclkRefXtalDiv(uint32_t u32Div, bool bDivFracEn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the current divider for the crystal reference clock (ref_xtal)
//! 
//! \fntype Function                        
//!     
//! \retval Current divider value
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPclkRefXtalDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets divider for the PLL reference cpu signal (ref_cpu)
//! 
//! \fntype Function                        
//!     
//! This field controls the divider connected to the ref_cpu reference clock 
//! that drives the CLK_P domain when bypass is NOT selected. For changes to 
//! this field to take effect, the ref_cpu reference clock must be running.
//!
//! \param[in] u32Div - Integer divide value. 
//! \param[in] bDivFracEn - Not used.  Divider will always be integer.
//!
//! \retval SUCCESS - Call was successful.
//! \return ERROR_HW_CLKCTRL_DIV_BY_ZERO - u32Div should not be zero
//! \return ERROR_HW_CLKCTRL_REF_CPU_GATED - Upstream reference clock is gated
//!
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0.
//! \notes HW_CLKCTRL_CPU.B.DIV_CPU is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPclkRefCpuDiv(uint32_t u32Div, bool bDivFracEn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the current divider for the PLL reference cpu signal (ref_cpu)
//! 
//! \fntype Function                        
//!     
//! \retval Current divider value
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPclkRefCpuDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Enables the gating of PClk while waiting for an interrupt
//! 
//! \fntype Function                        
//!
//! \param[in] bGatePclk - TRUE to gate PClk, FALSE to ungate
//!/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPclkInterruptWait(bool bGatePclk);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Enables/disables auto-slow modes for specific HClk components
//! 
//! \fntype Function                        
//!
//! There are six components that use the HClk and can be auto-slowed.  This
//! function will enable or disable each of the components auto-slow mode.
//! Components: APBHDMA_AUTOSLOW, APBXDMA_AUTOSLOW, TRAFFIC_JAM_AUTOSLOW, 
//!             TRAFFIC_AUTOSLOW, CPU_DATA_AUTOSLOW, CPU_INSTR_AUTOSLOW
//!
//! \param[in] eAutoslow - HClk component to change
//! \param[in] bEnable - TRUE to enable auto-slow mode, FALSE to disable
//!     
//! \retval ERROR_HW_CLKCTRL_UNSUPPORTED_AUTOSLOW_COMPONENT - unsupported component
//! \retval SUCCESS - component's auto-slow mode set properly
//!/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_EnableHclkModuleAutoSlow(hw_clkctrl_autoslow_t eModule, bool bEnable);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Enables auto-slow mode for HClk
//! 
//! \fntype Function                        
//!
//! Enable CLK_H auto-slow mode. When this is set, then CLK_H will run at 
//! the slow rate until one of the fast mode events has occurred.
//!
//! \param[in] bEnable - TRUE to enable auto-slow mode, FALSE to disable
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_EnableHclkAutoSlow(bool bEnable);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets PClk to HClk divide ratio
//! 
//! \fntype Function                        
//!
//! HClk = PClk/u32Div
//! The HClk frequency will be the result of the PClk divided by u32Div.
//!
//! \param[in] u32Div - Ratio of PClk to HClk
//! \param[in] bDivFracEn - Enable divider to be a fractional ratio.
//!
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - HClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - u32Div should not be zero
//!     
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0
//! \notes HW_CLKCTRL_HBUS.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetHclkDiv(uint32_t u32Div, bool bDivFracEn);

RtStatus_t hw_clkctrl_GetHclkDiv(uint32_t *u32Div);


/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets fast HClk to slow HClk divide ratio
//! 
//! \fntype Function                        
//!
//! Slow mode divide ratio. Sets the ratio of CLK_H fast rate to the slow rate.
//! Valid ratios: SLOW_DIV_BY1, SLOW_DIV_BY2, SLOW_DIV_BY4, 
//!               SLOW_DIV_BY8, SLOW_DIV_BY16, SLOW_DIV_BY32
//!
//! \param[in] u32Div - Ratio of PClk to HClk
//! \param[in] bDivFracEn - Enable divider to be a fractional ratio.
//!
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - HClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_INVALID_PARAM - Divide value out of range
//! \retval SUCCESS - HClk slow divide set
//!     
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetHclkSlowDiv(hw_clkctrl_hclk_slow_div_t sSlowDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the fast HClk to slow HClk divide ratio
//! 
//! \fntype Function                        
//!
//! \retval HClk slow divide ratio
//!     
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetHclkSlowDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the XClk divider
//! 
//! \fntype Function                        
//!
//! This field controls the CLK_X divide ratio. CLK_X is sourced from the 
//! 24-MHz XTAL through this divider.
//!
//! \param[in] u32Div - Divider in range 1 to 1023
//! \param[in] bDivFracEn - Unused
//!
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - XClk is still transitioning
//! \notes HW_CLKCTRL_XBUS.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero. 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetXclkDiv(uint32_t u32Div, bool bDivFracEn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gates or ungates EmiClk  for the ref_xtal source
//! 
//! \fntype Function                        
//!
//! CLK_EMI crystal divider Gate. If set to 1, the EMI_CLK divider that is 
//! sourced by the crystal reference clock, ref_xtal, is gated off. If set to
//! 0, CLK_EMI crystal divider is not gated
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
//!
//! \notes Only gates the ref_xtal reference clock to the XTAL divider. ref_emi
//! \notes has no gate but can be muxed.  
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetEmiRefXtalClkGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the EmiClk divider for the ref_xtal source
//! 
//! \fntype Function                        
//!
//! This field controls the divider connected to the crystal reference clock, 
//! ref_xtal, that drives the CLK_EMI domain when bypass IS selected.
//!
//! \param[in] u32Div - Divider in range 1 to 15
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - EmiClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval SUCCESS - EmiClk ref_xtal divider set
//!
//! \notes HW_CLKCTRL_EMI.B.DIV_XTAL is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetEmiClkRefXtalDiv(uint32_t u32Div);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the EmiClk divider for the ref_xtal source
//! 
//! \fntype Function                        
//!
//! This field controls the divider connected to the crystal reference clock, 
//! ref_xtal, that drives the CLK_EMI domain when bypass IS selected.
//!
//! \retval Current divider value for EmiClk's ref_xtal signal
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetEmiClkRefXtalDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the EmiClk divider for the ref_emi source
//! 
//! \fntype Function                        
//!
//! This field controls the divider connected to the ref_emi reference clock 
//! that drives the CLK_EMI domain when bypass IS NOT selected. For changes 
//! to this field to take effect, the ref_emi reference clock must be running.
//!
//! \param[in] u32Div - Divider in range 1 to 63
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - EmiClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval SUCCESS - EmiClk ref_emi divider set
//!
//! \notes HW_CLKCTRL_EMI.B.DIV_EMI is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetEmiClkRefEmiDiv(uint32_t u32Div);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Returns the EmiClk divider for the ref_emi source
//! 
//! \fntype Function                        
//!
//! This field controls the divider connected to PLL reference clock, 
//! ref_emi, that drives the CLK_EMI domain when bypass is not selected.
//!
//! \retval Current divider value for EmiClk's ref_emi signal
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetEmiClkRefEmiDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Reserved in the datasheet.  May be used for SDRAM
//! 
//! \retval Status of BUSY_DCC_RESYNC
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_EnableDccResync(bool bEnable);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Reserved in the datasheet.  May be used for SDRAM
//! 
//! \param[in] bEnable - TRUE to enable DCC resync
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetBusyDccResync(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief controls the selection of clock sources for various clock dividers.
//! 
//! \fntype Function                        
//!
//! The PClk, EmiClk, SspClk, GpmiClk, IrClk, PixClk, and SaifClk can choose their 
//! clock source from the PLL output, or bypass the PLL and use the 24MHz clock 
//! signal instead.
//!
//! \param[in] - eClk - clock to bypass.  Valid inputs are: BYPASS_CPU, BYPASS_EMI,
//!              BYPASS_SSP, BYPASS_GPMI, BYPASS_IR, BYPASS_PIX, BYPASS_SAIF
//! \param[in] - bBypass - TRUE to use ref_xtal clock, FALSE to use PLL 
//!
//! \retval - ERROR_HW_CLKCTRL_REF_CPU_GATED - ref_cpu is gated at PFD
//! \retval - ERROR_HW_CLKCTRL_REF_EMI_GATED - ref_emi is gated at PFD
//! \retval - ERROR_HW_CLKCTRL_REF_IO_GATED - ref_io is gated at PFD
//! \retval - ERROR_HW_CLKCTRL_REF_PIX_GATED - ref_pix is gated at PFD
//! \retval - ERROR_HW_CLKCTRL_INVALID_GATE_VALUE - gate cannot be set
//! \retval - SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetClkBypass(hw_clkctrl_bypass_clk_t eClk, bool bBypass);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for PClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates Pclk
//! \retval FALSE - ref_cpu generates Pclk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPclkBypass(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for EmiClk
//!
//! \retval TRUE - ref_xtal generates Emiclk
//! \retval FALSE - ref_emi generates Emiclk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetEmiClkBypass(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for SspClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates Sspclk
//! \retval FALSE - ref_io generates Sspclk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetSspClkBypass(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for GpmiClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates GpmiClk
//! \retval FALSE - ref_io generates GpmiClk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetGpmiClkBypass(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for IrClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates IrClk
//! \retval FALSE - ref_io generates IrClk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetIrClkBypass(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the bypass status for PixClk
//!
//! \fntype Function
//!
//! \retval TRUE - ref_xtal generates PixClk
//! \retval FALSE - ref_io generates PixClk
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPixClkBypass(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set clock gates for various clocks with only ref_xtal as source
//! 
//! \fntype Function                        
//!
//! Controls various fixed-rate divider clocks working off the 24.0-MHz crystal clock.
//!
//! \param[in] eClk - Clock to gate/ungate.  Valid inputs are: UART_CLK_GATE,
//!                   FILT_CLK24M_GATE, PWM_CLK24M_GATE, DRI_CLK24M_GATE,
//!                   DIGCTRL_CLK1M_GATE, TIMROT_CLK32K_GATE
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
//!
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetXtalClkGate(hw_clkctrl_xtal_gate_t eClk, bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PixClk
//! 
//! \fntype Function                        
//!
//! CLK_PIX Gate. If set to 1, CLK_PIX is gated off. 0:CLK_PIX is not gated. 
//! When this bit is modified, or when it is high, the DIV field should 
//! not change its value. The DIV field can change ONLY when this clock
//! gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPixClkGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PixClk divider
//! 
//! \fntype Function                        
//!
//! The Pixel clock frequency is determined by dividing the selected reference 
//! clock (ref_xtal or ref_pix) by the value in this bit field. This field can 
//! be programmed with a new value only when CLKGATE = 0.
//!
//! \param[in] u32Div - Divider in range 1 to 255
//! \param[in] bDivFracEn - Unused
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_INVALID_DIV_VALUE - divider exceeds max divide
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - PixClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - PixClk divider set
//! 
//! \notes The divider is set to divide by 1 at power-on reset. 
//! \notes Do NOT divide by 0. Do not divide by more than 255.
//! \notes HW_CLKCTRL_PIX.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPixClkDiv(uint32_t u32Div, bool bDivFracEn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get PixClk divider
//! 
//! \fntype Function                        
//!
//! Returns the pix clock divider value
//!
//! \retval Current pix clock divider value
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPixClkDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate SspClk
//! 
//! \fntype Function                        
//!
//! CLK_SSP Gate. If set to 1, CLK_SSP is gated off. 0:CLK_SSP is not gated. 
//! When this bit is modified, or when it is high, the DIV field should not 
//! change its value. The DIV field can change ONLY when this clock
//! gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetSspClkGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set SspClk divider
//! 
//! \fntype Function                        
//!
//! The synchronous serial port clock frequency is determined by dividing 
//! the selected reference clock (ref_xtal or ref_io) by the value in this 
//! bit field. This field can be programmed with a new value only when CLKGATE = 0.
//!
//! \param[in] u32Div - Divider in range 1 to 511
//! \param[in] bDivFracEn - TRUE to use fractional divide, FALSE to use integer divide
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - SspClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - SspClk divider set
//! 
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0.
//! \notes HW_CLKCTRL_SSP.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero. 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetSspClkDiv(uint32_t u32Div, bool bDivFracEn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get SspClk divider
//! 
//! \fntype Function                        
//!
//! Returns the SSP clock divider value
//!
//! \retval Current SSP clock divider value
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetSspClkDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate GpmiClk
//! 
//! \fntype Function                        
//!
//! CLK_GPMI Gate. If set to 1, CLK_GPMI is gated off. 0:CLK_GPMI is not gated. 
//! When this bit is modified, or when it is high, the DIV field should 
//! not change its value. The DIV field can change ONLY when this clock
//! gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetGpmiClkGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set GpmiClk divider
//! 
//! \fntype Function                        
//!
//! The GPMI clock frequency is determined by dividing the selected reference 
//! clock (ref_xtal or ref_io) by the value in this bit field. This field 
//! can be programmed with a new value only when CLKGATE = 0.
//!
//! \param[in] u32Div - Divider in range 1 to 1023
//! \param[in] bDivFracEn - TRUE to use fractional divide, FALSE to use integer divide
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - GpmiClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - GpmiClk divider set
//! 
//! \notes The divider is set to divide by 1 at power-on reset. Do NOT divide by 0.
//! \notes HW_CLKCTRL_GPMI.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero. 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetGpmiClkDiv(uint32_t u32Div, bool bDivFracEn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get GpmiClk divider
//! 
//! \fntype Function                        
//!
//! Returns the GPMI clock divider value
//!
//! \retval Current GPMI clock divider value
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetGpmiClkDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate SpdifClk
//! 
//! \fntype Function                        
//!
//! CLK_PCMSPDIF Gate. If set to 1, CLK_PCMSPDIF is gated off. 0: CLK_PCMSPDIF 
//! is not gated. When this bit is modified, or when it is high, the DIV field should
//! not change its value. The DIV field can change ONLY when this 
//! clock gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetSpdifClkGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set GpmiClk divider
//!
//! \notes Datasheet only has clock gate.  Definition mentions DIV field so we put 
//! \notes it in here in case datasheet changes.  bDivFracEn and u32Div are 
//! \notes ignored and left in for future consideration.
//! \notes HW_CLKCTRL_SPDIF.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetSpdifClkDiv(uint32_t u32Div, bool bDivFracEn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Allow hardware to automatically set the divide ratios.
//! 
//! \fntype Function                        
//!
//! \param[in] bEnable - TRUE to enable, FALSE to disable
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_EnableIrAutoDivide(bool bEnable);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate IrClk and IrovClk
//! 
//! \fntype Function                        
//!
//! CLK_IR and CLK_IROV Gate. If set to 1, CLK_IR and CLK_IROV are gated off. 
//! 0: CLK_IR and CLK_IROV are not gated. When this bit is modified, or when it is
//! high, the DIV field should not change its value. The DIV field can change 
//! ONLY when this clock gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetIrClkGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set IrClk divider
//! 
//! \fntype Function                        
//!
//! This field controls the CLK_IR divide-ratio-2. Values between 5 and 768 
//! are valid. This divider is used in conjunction with IROV_DIV to set the 
//! final rate of CLK_IR. This field can be programmed with a new
//! value only when CLKGATE = 0.
//!
//! \param[in] u32IrDiv - Divider in range 5 to 768
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_INVALID_DIV_VALUE - divider value out of range
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - IrClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - IrClk divider set
//! 
//! \notes HW_CLKCTRL_IR.B.IR_DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetIrClkDiv(uint32_t u32IrDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set IrovClk divider
//! 
//! \fntype Function                        
//!
//! This field controls the CLK_IR Divide ratio-1 and the CLK_IROV divide 
//! ratio. Values between 4 and 260 are valid. This divider is used in 
//! conjunction with IR_DIV to set the final rate of CLK_IR. This divider is 
//! directly used to set the rate of CLK_IROV. This field can be programmed 
//! with a new value only when CLKGATE = 0.
//!
//! \param[in] u32IrDiv - Divider in range 4 to 260
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_INVALID_DIV_VALUE - divider value out of range
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - IrClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - upstream ref clock is gated
//! \retval SUCCESS - IrClk divider set
//! 
//! \notes HW_CLKCTRL_IR.B.IROV_DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetIrovClkDiv(uint32_t u32Div);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate SaifClk
//! 
//! \fntype Function                        
//!
//! CLK_SAIF Gate. If set to 1, CLK_SAIF is gated off. 0:CLK_SAIF is not gated. 
//! When this bit is modified, or when it is high, the DIV field should not change 
//! its value. The DIV field can change ONLY when this clock gate bit field is low.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetSaifClkGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set SaifClk divider
//! 
//! \fntype Function                        
//!
//! The SAIF clock frequency is determined by dividing the selected reference 
//! clock (ref_xtal or ref_pll) by the value in this bit field. This field can 
//! be programmed with a new value only when CLKGATE = 0.
//!
//! \param[in] u32IrDiv - Divider in range 1 to 65535
//! \param[in] bDivFracEn - Unused
//!
//! \retval ERROR_HW_CLKCTRL_DIV_BY_ZERO - should not divide by zero
//! \retval ERROR_HW_CLKCTRL_CLK_DIV_BUSY - SaifClk is still transitioning
//! \retval ERROR_HW_CLKCTRL_CLK_GATED - clock must be ungated to change divider
//! \retval SUCCESS - SaifClk divider set
//!
//! \notes HW_CLKCTRL_SAIF.B.DIV is used to avoid a clear/set 
//! \notes which momentarily divides by zero.
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetSaifClkDiv(uint32_t u32Div, bool bDivFracEn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PLL output through PFD for ref_io
//! 
//! \fntype Function                        
//!
//! IO Clock Gate. If set to 1, the IO fractional divider clock
//! (reference PLL ref_io) is off (power savings). 0: IO
//! fractional divider clock is enabled.
//!
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate 
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPfdRefIoGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the gate status for ref_io in the PFD
//! 
//! \fntype Function                        
//! 
//! \retval - TRUE if gated, FALSE if ungated
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdRefIoGate(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PFD for ref_io
//! 
//! \fntype Function                        
//!
//! This field controls the IO clocks fractional divider. The
//! resulting frequency shall be 480 * (18/IOFRAC) where
//! IOFRAC = 18-35.
//!
//! \param[in] u32Div - Divider in range 18 to 35
//! 
//! \notes Divider = 18 will disable PFD circuit to save power since 18/18 is 
//! \notes equal to 1.  The output will be 480MHz.
//!
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - can't change divider when clock is gated
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPfdRefIoDiv(uint32_t u32Div);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return divider value for ref_io
//! 
//! \fntype Function                        
//! 
//! \retval - divider value in range 18 - 35
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPfdRefIoDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PLL output through PFD for ref_pix
//! 
//! \fntype Function                        
//!
//! PIX Clock Gate. If set to 1, the PIX fractional divider
//! clock (reference PLL ref_pix) is off (power savings). 0:
//! PIX fractional divider clock is enabled.
//! 
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate 
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPfdRefPixGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the gate status for ref_pix in the PFD
//! 
//! \fntype Function                        
//! 
//! \retval - TRUE if gated, FALSE if ungated
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdRefPixGate(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PFD for ref_pix
//! 
//! \fntype Function                        
//!
//! This field controls the PIX clock fractional divider. The
//! resulting frequency shall be 480 * (18/PIXFRAC) where
//! PIXFRAC = 18-35.
//!
//! \param[in] u32Div - Divider in range 18 to 35
//! 
//! \notes Divider = 18 will disable PFD circuit to save power since 18/18 is 
//! \notes equal to 1.  The output will be 480MHz.
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - can't change divider when clock is gated
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPfdRefPixDiv(uint32_t u32Div);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return divider value for ref_pix
//! 
//! \fntype Function                        
//! 
//! \retval - divider value in range 18 - 35
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPfdRefPixDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PLL output through PFD for ref_emi
//! 
//! \fntype Function                        
//!
//! EMI Clock Gate. If set to 1, the EMI fractional divider
//! clock (reference PLL ref_emi) is off (power savings). 0:
//! EMI fractional divider clock is enabled.
//! 
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate 
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPfdRefEmiGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the gate status for ref_emi in the PFD
//! 
//! \fntype Function                        
//! 
//! \retval - TRUE if gated, FALSE if ungated
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdRefEmiGate(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PFD for ref_emi
//! 
//! \fntype Function                        
//!
//! This field controls the EMI clock fractional divider. The
//! resulting frequency shall be 480 * (18/EMIFRAC) where
//! EMIFRAC = 18-35.
//!
//! \param[in] u32Div - Divider in range 18 to 35
//! 
//! \notes Divider = 18 will disable PFD circuit to save power since 18/18 is 
//! \notes equal to 1.  The output will be 480MHz.
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - can't change divider when clock is gated
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPfdRefEmiDiv(uint32_t u32Div);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return divider value for ref_emi
//! 
//! \fntype Function                        
//! 
//! \retval - divider value in range 18 - 35
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPfdRefEmiDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Gate/ungate PLL output through PFD for ref_cpu
//! 
//! \fntype Function                        
//!
//! CPU Clock Gate. If set to 1, the CPU fractional divider
//! clock (reference PLL ref_cpu) is off (power savings). 0:
//! CPU fractional divider clock is enabled.
//! 
//! \param[in] bClkGate - TRUE to gate, FALSE to ungate 
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetPfdRefCpuGate(bool bClkGate);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get the gate status for ref_cpu in the PFD
//! 
//! \fntype Function                        
//! 
//! \retval - TRUE if gated, FALSE if ungated
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdRefCpuGate(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Set PFD for ref_cpu
//! 
//! \fntype Function                        
//!
//! This field controls the CPU clock fractional divider. The
//! resulting frequency shall be 480 * (18/CPUFRAC) where
//! CPUFRAC = 18-35.
//!
//! \param[in] u32Div - Divider in range 18 to 35
//! 
//! \notes Divider = 18 will disable PFD circuit to save power since 18/18 is 
//! \notes equal to 1.  The output will be 480MHz.
//! \retval ERROR_HW_CLKCTRL_REF_CLK_GATED - can't change divider when clock is gated
//! \retval SUCCESS
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_SetPfdRefCpuDiv(uint32_t u32Div);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return divider value for ref_cpu
//! 
//! \fntype Function                        
//! 
//! \retval - divider value in range 18 - 35
/////////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPfdRefCpuDiv(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get status of fractional divider transition for ref_io
//!
//! \retval Divider transition for ref_io
//!
//! \notes This read-only bitfield is for DIAGNOSTIC PURPOSES ONLY since the 
//! \notes fractional divide should become stable quickly enough that this 
//! \notes field will never need to be used by either device driver or 
//! \notes application code. The value inverts when the new programmed fractional 
//! \notes divide value has taken effect. Read this bit, program the new value, 
//! \notes and when this bit inverts, the phase divider clock output is stable. 
//! \notes Note that the value will not invert when the fractional divider 
//! \notes is taken out of or placed into clock-gated state. 
//! 
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdFracDivIoStable(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get status of fractional divider transition for ref_pix
//!
//! \retval Divider transition for ref_pix
//!
//! \notes This read-only bitfield is for DIAGNOSTIC PURPOSES ONLY since the 
//! \notes fractional divide should become stable quickly enough that this 
//! \notes field will never need to be used by either device driver or 
//! \notes application code. The value inverts when the new programmed fractional 
//! \notes divide value has taken effect. Read this bit, program the new value, 
//! \notes and when this bit inverts, the phase divider clock output is stable. 
//! \notes Note that the value will not invert when the fractional divider 
//! \notes is taken out of or placed into clock-gated state. 
//! 
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdFracDivPixStable(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get status of fractional divider transition for ref_emi
//!
//! \retval Divider transition for ref_emi
//!
//! \notes This read-only bitfield is for DIAGNOSTIC PURPOSES ONLY since the 
//! \notes fractional divide should become stable quickly enough that this 
//! \notes field will never need to be used by either device driver or 
//! \notes application code. The value inverts when the new programmed fractional 
//! \notes divide value has taken effect. Read this bit, program the new value, 
//! \notes and when this bit inverts, the phase divider clock output is stable. 
//! \notes Note that the value will not invert when the fractional divider 
//! \notes is taken out of or placed into clock-gated state. 
//! 
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdFracDivEmiStable(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Get status of fractional divider transition for ref_cpu
//!
//! \retval Divider transition for ref_cpu
//!
//! \notes This read-only bitfield is for DIAGNOSTIC PURPOSES ONLY since the 
//! \notes fractional divide should become stable quickly enough that this 
//! \notes field will never need to be used by either device driver or 
//! \notes application code. The value inverts when the new programmed fractional 
//! \notes divide value has taken effect. Read this bit, program the new value, 
//! \notes and when this bit inverts, the phase divider clock output is stable. 
//! \notes Note that the value will not invert when the fractional divider 
//! \notes is taken out of or placed into clock-gated state. 
//! 
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPfdFracDivCpuStable(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Enable/disable PLL outputs for USB PHY
//! 
//! \fntype Function                        
//!
//! 0: 8-phase PLL outputs for USB PHY are powered
//! down. If set to 1, 8-phase PLL outputs for USB PHY
//! are powered up. Additionally, the UTMICLK120_GATE
//! and UTMICLK30_GATE must be deasserted in the
//! UTMI phy to enable USB operation.
//!
//! \param[in] bEnableUsbClks - TRUE to enable, FALSE to disable
//! 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_clkctrl_EnableUsbClks(bool bEnableUsbClks);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Power the PLL
//! 
//! \fntype Function                        
//!
//! PLL Power On (0 = PLL off; 1 = PLL On). Allow 10 us
//! after turning the PLL on before using the PLL as a
//! clock source. This is the time the PLL takes to lock to 480 MHz.
//!
//! \param[in] bPowerOn - TRUE to power, FALSE to unpower
//!
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_PowerPll(bool bPowerOn);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return PLL lock status
//! 
//! \fntype Function                        
//!
//! \retval - PLL lock status
//! 
/////////////////////////////////////////////////////////////////////////////////
bool hw_clkctrl_GetPllLock(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Return PLL lock count
//! 
//! \fntype Function                        
//!
//! \retval - PLL lock count
//!//////////////////////////////////////////////////////////////////////////////
uint32_t hw_clkctrl_GetPllLockCount(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Force PLL lock
//! 
//! \fntype Function                        
//!
//! \param[in] bForceLock - RESERVED 
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_SetForceLock(bool bForceLock);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Resets the digital registers to their default state.
//!
//! \fntype Function
//!
//! The DCDC and power module will not be reset.  All the digital sections
//! of the chip will be reset. 
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_ResetDigital(void);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Resets the entire chip.  Same as power on reset. 
//!
//! \fntype Function
//!
//! The entire chip will reset.  
/////////////////////////////////////////////////////////////////////////////////
void hw_clkctrl_ResetChip(void);

bool hw_clkctrl_GetPllPowered(void);

// 36xx functions I have to resolve
RtStatus_t hw_clocks_PowerPll(bool bPowerOn);
RtStatus_t hw_clocks_BypassPll(bool bBypass);
RtStatus_t hw_clocks_SetPllFreq(uint16_t u16_FreqHz);
RtStatus_t hw_clocks_SetPclkDiv(uint16_t u16_PclkDivider);
RtStatus_t hw_clocks_SetHclkDiv(uint16_t u16_PclkDivider);
RtStatus_t hw_clocks_SetHclkAutoSlowDivider(int16_t uDiv);
RtStatus_t hw_clocks_EnableHclkAutoSlow(bool bEnable);
RtStatus_t hw_clocks_SetXclkDiv(uint16_t u16_XclkDivider);


#endif//__hw_clocks_H

///////////////////////////////////////////////////////////////////////////////
// End of file
///////////////////////////////////////////////////////////////////////////////
//! @}

