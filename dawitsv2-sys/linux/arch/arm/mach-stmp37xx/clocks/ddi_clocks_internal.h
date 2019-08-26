///////////////////////////////////////////////////////////////////////////////
//! \addtogroup ddi_clocks
//! @{
//
// Copyright (c) 2004-2005 SigmaTel, Inc.
//
//! \file ddi_clocks.h
//! \brief Contains header data for the Clocks Device Driver subsystem.
///////////////////////////////////////////////////////////////////////////////
#ifndef _DDI_CLOCKS_INTERNAL_H
#define _DDI_CLOCKS_INTERNAL_H

/////////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////////
extern uint8_t u8MinPfdValue;
extern uint32_t u32MaxPllFreq;
extern bool bKeepPllPowered;
extern uint32_t u32PllClients;

/////////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions Pclk from the 24MHz to new 24Mhz frequency 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for Pclk's ref_cpu divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPclkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

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
RtStatus_t ddi_clocks_TransPclkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions Pclk from the 24MHz to new 24Mhz frequency 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPclkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions Pclk from PLL to 24Mhz crystal
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for Pclk's ref_cpu divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPclkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions EmiClk from the 24MHz to new 24Mhz frequency 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for EmiClk's ref_emi divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransEmiClkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

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
RtStatus_t ddi_clocks_TransEmiClkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions EmiClk from the PLL to the new PLL frequency
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for EmiClk's ref_emi divider
//! \param[in] u32PhaseFracDiv - value for ref_emi PFD
//!
//! \notes The numbered sequence follows the steps outlined in the datasheet
//! \notes for transitioning from XTAL to PLL.  
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransEmiClkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions EmiClk from the PLL to 24Mhz crystal 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransEmiClkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions SspClk from the 24MHz to new 24Mhz frequency 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransSspClkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions SspClk from the XTAL to the PLL
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransSspClkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions SspClk from the PLL to the new PLL frequency 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransSspClkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions SspClk from the PLL to 24Mhz crystal 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for SspClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransSspClkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions GpmiClk from the 24MHz to new 24Mhz frequency 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransGpmiClkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions GpmiClk from the XTAL to PLL
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransGpmiClkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions GpmiClk from the PLL to the new PLL frequency
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransGpmiClkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions GpmiClk from the PLL to 24Mhz crystal
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for GpmiClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransGpmiClkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions IrClk from the 24MHz to new 24Mhz frequency 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for IrClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransIrClkXtalToXtal(uint32_t u32IrDiv, uint32_t u32IrovDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions IrClk from the XTAL to PLL
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for IrClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransIrClkXtalToPll(uint32_t u32IrDiv, uint32_t u32IrovDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions IrClk from the PLL to the new PLL frequency
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for IrClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransIrClkPllToPll(uint32_t u32IrDiv, uint32_t u32IrovDiv, uint32_t u32PhaseFracDiv, bool bChangeRefIo);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions IrClk from the PLL to 24Mhz crystal
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for IrClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransIrClkPllToXtal(uint32_t u32IrDiv, uint32_t u32IrovDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions PixClk from the 24MHz to new 24Mhz frequency 
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for PixClk's ref_io divider
//! \param[in] u32PhaseFracDiv - Unused
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPixClkXtalToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions PixClk from the XTAL to PLL
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for PixClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io's PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPixClkXtalToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Transitions PixClk from the PLL to the new PLL frequency
//!
//! \fntype Function
//!
//! \param[in] u32IntDiv - divider for PixClk's ref_io divider
//! \param[in] u32PhaseFracDiv - divider value for ref_io PFD
//!
//! \retval SUCCESS
//! \retval error code from hw layer
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_TransPixClkPllToPll(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

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
RtStatus_t ddi_clocks_TransPixClkPllToXtal(uint32_t u32IntDiv, uint32_t u32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Calculates the PFD value and clock integer divider
//!
//! \fntype Function
//!
//! \param[in] pu32ClkFreq_kHz - New clock frequency requested
//! \param[in] pu32RefClkFreq_kHz - Current ref_* clock frequency
//! \param[in] bChangePllRefClk - TRUE to allow ref_* cllk to change
//!                               FALSE to use current ref_* clock
//!
//! \param[out] pu32ClkFreq_kHz - Actual frequency clock was set to
//! \param[out] pu32RefClkFreq_kHz - Actual frequency ref_* clock was set to
//! \param[out] pu32IntDiv - Calculated integer divider
//! \param[out] pu32PhaseFracDiv - Calculated PFD value
//!
//! \retval ERROR_DDI_CLOCKS_INVALID_PLL_FREQ - Cannot calculate dividers with given inputs
//! \retval SUCCESS 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_CalculatePfdAndIntDiv(uint32_t* pu32ClkFreq_kHz,
                                      uint32_t* pu32RefClkFreq_kHz, 
                                      uint32_t* pu32IntDiv,
                                      uint32_t* pu32PhaseFracDiv,
                                      bool bChangePllRefClk);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Calculates the best match for PFD value and clock integer divider
//!
//! \fntype Function
//!
//! \param[in] pu32ClkFreq_kHz - New clock frequency requested
//! \param[in] pu32RefClkFreq_kHz - Current ref_* clock frequency
//! \param[in] bChangePllRefClk - TRUE to allow ref_* cllk to change
//!                               FALSE to use current ref_* clock
//!
//! \param[out] pu32ClkFreq_kHz - Actual frequency clock was set to
//! \param[out] pu32RefClkFreq_kHz - Actual frequency ref_* clock was set to
//! \param[out] pu32IntDiv - Calculated integer divider
//! \param[out] pu32PhaseFracDiv - Calculated PFD value
//!
//! \retval ERROR_DDI_CLOCKS_INVALID_PLL_FREQ - Cannot calculate dividers with given inputs
//! \retval SUCCESS 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_ExactPfdAndIntDiv(uint32_t* pu32ClkFreq_kHz,
                                      uint32_t* pu32RefClkFreq_kHz, 
                                      uint32_t* pu32IntDiv,
                                      uint32_t* pu32PhaseFracDiv);

/////////////////////////////////////////////////////////////////////////////////
//! \brief Calculates the clock integer divider for ref_xtal signal
//!
//! \fntype Function
//!
//! \param[in] pu32ClkFreq_kHz - New clock frequency requested
//!
//! \param[out] pu32ClkFreq_kHz - Actual frequency clock was set to
//! \param[out] pu32IntDiv - Calculated integer divider
//!
//! \retval ERROR_DDI_CLOCKS_INVALID_XTAL_FREQ - Max clock possible is 24Mhz
//! \retval SUCCESS 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_CalculateRefXtalDiv(uint32_t* pu32ClkFreq_kHz,uint32_t* pu32IntDiv);


#endif
///////////////////////////////////////////////////////////////////////////////
// End of file
///////////////////////////////////////////////////////////////////////////////
//! @}
