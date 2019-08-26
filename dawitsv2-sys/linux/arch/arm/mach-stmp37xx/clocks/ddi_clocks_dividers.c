/////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2007-2008 SigmaTel, Inc.
//
//! \file ddi_clocks_dividers.c
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


//--------------------------------------------------------------------------
// This table is the pre-calcualted values for the PLL frequency using
// the corresponding PFD value.  
//--------------------------------------------------------------------------    
const uint32_t PfdPllFreqs[] =
{
    //|-----------+-----------|
    //|  PLL Freq | PFD Value |    
    //|-----------+-----------|
        480000,    // 18 (MIN_PFD_VALUE)
        454737,    // 19
        432000,    // 20
        411429,    // 21  
        392727,    // 22
        375652,    // 23
        360000,    // 24
        345600,    // 25
        332308,    // 26
        320000,    // 27
        308571,    // 28
        297931,    // 29
        288000,    // 30
        278710,    // 31
        270000,    // 32
        261818,    // 33
        254118,    // 34
        246857     // 35 (MAX_PFD_VALUE)
};

/////////////////////////////////////////////////////////////////////////////////
// Code
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//! \brief Calculates the PFD value and clock integer divider
//!
//! \fntype Function
//!
//! This function takes as inputs the requested clock frequency and the current
//! ref_* pll clock frequency.  It will calculate the integer divider and PFD 
//! value to get as close a possible to the requested frequency without 
//! exceeding it.  
//!
//! \param[in] pu32ClkFreq_kHz New clock frequency requested
//! \param[in] pu32RefClkFreq_kHz Current ref_* clock frequency
//! \param[in] bChangePllRefClk TRUE to allow ref_* cllk to change, FALSE to 
//! use current ref_* clock
//!
//! \param[out] pu32ClkFreq_kHz Actual frequency clock was set to
//! \param[out] pu32RefClkFreq_kHz Actual frequency ref_* clock was set to
//! \param[out] pu32IntDiv Calculated integer divider
//! \param[out] pu32PhaseFracDiv Calculated PFD value
//!
//! \retval ERROR_DDI_CLOCKS_INVALID_PLL_FREQ - Cannot calculate dividers 
//! with given inputs
//! \retval SUCCESS 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_CalculatePfdAndIntDiv(uint32_t* pu32ClkFreq_kHz,
                                      uint32_t* pu32RefClkFreq_kHz, 
                                      uint32_t* pu32IntDiv,
                                      uint32_t* pu32PhaseFracDiv,
                                      bool bChangePllRefClk)
{
    RtStatus_t rtn;
    uint32_t u32ClkFreq = *pu32ClkFreq_kHz;
    uint32_t u32RefClkFreq = *pu32RefClkFreq_kHz;
    uint32_t u32NewFreq; 
    uint32_t u32IntDiv;
    uint32_t u32PhaseFracDiv;
    
    //--------------------------------------------------------------------------
    // Change the PLL ref_* clk to get as close as possible
    //--------------------------------------------------------------------------
    if(bChangePllRefClk)
    {
        //----------------------------------------------------------------------
        // We want to get as close as possible to the requested frequency.
        // We'll call our our exact function.  
        //----------------------------------------------------------------------
        if((rtn = ddi_clocks_ExactPfdAndIntDiv(&u32ClkFreq,&u32RefClkFreq,
                                        &u32IntDiv,&u32PhaseFracDiv)) != SUCCESS)
        {
            return rtn;
        }

        //----------------------------------------------------------------------
        // Check PFD value.
        //----------------------------------------------------------------------
        if(u32PhaseFracDiv > MAX_PFD_VALUE || u32PhaseFracDiv < u8MinPfdValue)
            return ERROR_DDI_CLOCKS_INVALID_PLL_FREQ;

        //----------------------------------------------------------------------
        // Check int divider value.
        //----------------------------------------------------------------------
        if(u32IntDiv == 0)
            return ERROR_DDI_CLOCKS_DIV_BY_ZERO;

        //----------------------------------------------------------------------
        // Calculate new frequencies.
        //----------------------------------------------------------------------
        u32RefClkFreq = (u8MinPfdValue * u32MaxPllFreq)/u32PhaseFracDiv;
        u32NewFreq = u32RefClkFreq/u32IntDiv;
    }

    //--------------------------------------------------------------------------
    // Use the current ref_* clk and get close to requested speed (for ref_io).
    //--------------------------------------------------------------------------
    else
    {
        //----------------------------------------------------------------------
        // Calculate the divider and new freq.
        //----------------------------------------------------------------------
        u32IntDiv = u32RefClkFreq/u32ClkFreq;        
        u32NewFreq = u32RefClkFreq/u32IntDiv;
        u32PhaseFracDiv = u8MinPfdValue;

        //----------------------------------------------------------------------
        // but don't exceed the requested frequency.
        //----------------------------------------------------------------------
        if(u32NewFreq > u32ClkFreq)
        {
            //------------------------------------------------------------------
            // Round up to next divider.
            //------------------------------------------------------------------
            u32IntDiv++;
            u32NewFreq = u32RefClkFreq/u32IntDiv;
        }

        //----------------------------------------------------------------------
        // Check int divider value.
        //----------------------------------------------------------------------
        if(u32IntDiv == 0)
            return ERROR_DDI_CLOCKS_DIV_BY_ZERO;
    }

    //--------------------------------------------------------------------------
    // Return the calculated values.
    //--------------------------------------------------------------------------
    *pu32ClkFreq_kHz = u32NewFreq;
    *pu32RefClkFreq_kHz = u32RefClkFreq;
    *pu32IntDiv = u32IntDiv;
    *pu32PhaseFracDiv = u32PhaseFracDiv;
    return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//! \brief Calculates the clock integer divider for ref_xtal signal
//!
//! \fntype Function
//!
//! This function uses the 24MHz ref_xtal clock as the source for the calculation.
//! It will determine the interger value to get as close as possible to the
//! requested frequency.  
//!
//! \param[in] pu32ClkFreq_kHz New clock frequency requested
//!
//! \param[out] pu32ClkFreq_kHz Actual frequency clock was set to
//! \param[out] pu32IntDiv Calculated integer divider
//!
//! \retval ERROR_DDI_CLOCKS_INVALID_XTAL_FREQ - Max clock possible is 24Mhz
//! \retval SUCCESS 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_CalculateRefXtalDiv(uint32_t* pu32ClkFreq_kHz,uint32_t* pu32IntDiv)
{
    uint32_t u32ClkFreq = *pu32ClkFreq_kHz;
    uint32_t u32NewFreq;
    uint32_t u32IntDiv = *pu32IntDiv;
    
    //--------------------------------------------------------------------------    
    // Use crystal for speeds equal to and lower than 24MHz
    //--------------------------------------------------------------------------
    if(u32ClkFreq <= XTAL_24MHZ_IN_KHZ)
    {
        //----------------------------------------------------------------------        
        // Calculate the divider and the actual freq...
        //----------------------------------------------------------------------
        u32IntDiv = XTAL_24MHZ_IN_KHZ/u32ClkFreq;
        u32NewFreq = XTAL_24MHZ_IN_KHZ/u32IntDiv;

        //----------------------------------------------------------------------
        // but don't exceed the requested frequency
        //----------------------------------------------------------------------
        if(u32NewFreq > u32ClkFreq)
        {
            //------------------------------------------------------------------
            // Round up to next divider
            //------------------------------------------------------------------
            u32IntDiv++;
            u32NewFreq = XTAL_24MHZ_IN_KHZ/u32IntDiv;
        }

        //----------------------------------------------------------------------
        // Check int divider value
        //----------------------------------------------------------------------
        if(u32IntDiv == 0)
            return ERROR_DDI_CLOCKS_DIV_BY_ZERO;
    }


    //--------------------------------------------------------------------------
    // Use PLL for freqs greater than 24MHz
    //--------------------------------------------------------------------------
    else
    {
        return ERROR_DDI_CLOCKS_INVALID_XTAL_FREQ;
    }

    //--------------------------------------------------------------------------
    // Return the actual frequency determined and the integer divider.
    //--------------------------------------------------------------------------
    *pu32ClkFreq_kHz = u32NewFreq;
    *pu32IntDiv = u32IntDiv;
    return SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////
//! \brief Calculates the most accurate Pfd and integer divider.  
//!
//! \fntype Function
//!
//! \param[in] pu32ClkFreq_kHz New clock frequency requested
//! \param[in] pu32RefClkFreq_kHz Current ref_* clock frequency
//! \param[in] bChangePllRefClk TRUE to allow ref_* cllk to change, FALSE to 
//! use current ref_* clock
//!
//! \param[out] pu32ClkFreq_kHz Actual frequency clock was set to
//! \param[out] pu32RefClkFreq_kHz Actual frequency ref_* clock was set to
//! \param[out] pu32IntDiv Calculated integer divider
//! \param[out] pu32PhaseFracDiv Calculated PFD value
//!
//! \retval ERROR_DDI_CLOCKS_INVALID_PLL_FREQ - Cannot calculate dividers 
//! with given inputs
//! \retval SUCCESS 
/////////////////////////////////////////////////////////////////////////////////
RtStatus_t ddi_clocks_ExactPfdAndIntDiv(uint32_t* pu32ClkFreq_kHz,
                                      uint32_t* pu32RefClkFreq_kHz, 
                                      uint32_t* pu32IntDiv,
                                      uint32_t* pu32PhaseFracDiv)
{
    uint16_t NumPllFreqs = sizeof(PfdPllFreqs)/sizeof(uint32_t);

    uint32_t u32ClkFreq = *pu32ClkFreq_kHz;
    uint16_t i;
    uint16_t IntDivAbove,IntDivBelow;
    uint32_t DeltaAbove,DeltaBelow;    
    uint32_t BestIntDiv, BestPfdDiv, BestDelta;

    //--------------------------------------------------------------------------
    // Initialize the compare values to the maximum they could possibly be.
    //--------------------------------------------------------------------------
    BestDelta = u32MaxPllFreq;
    BestIntDiv = MIN_INT_DIV;
    BestPfdDiv = u8MinPfdValue;


    //--------------------------------------------------------------------------    
    // Check each value in the PLL frequency table.  Our index starts at zero,
    // but the MIN_PFD_VALUE is 18.  We'll have to account for this when we
    // save off the PFD divider value.  If the software has set the minimum 
    // PFD, then we need to start at that value when we traverse the table.  
    //--------------------------------------------------------------------------
    for(i=u8MinPfdValue-MIN_PFD_VALUE;i<NumPllFreqs;i++)
    {

        //----------------------------------------------------------------------
        // Calculate the integer dividers that get us just above and just below
        // the frequency we want.  
        //----------------------------------------------------------------------
        IntDivAbove = PfdPllFreqs[i]/u32ClkFreq;
        IntDivBelow = IntDivAbove + 1;
        //----------------------------------------------------------------------
        // Calculate the delta between the frequency we want and the frequency
        // we can get with the current divider values.  
        //----------------------------------------------------------------------
		// 
		// 2008/07/08
		// mooji. fix divizion by zero
		// 
		if (IntDivAbove)
			DeltaAbove = (PfdPllFreqs[i]/IntDivAbove) - u32ClkFreq;
        DeltaBelow = u32ClkFreq - (PfdPllFreqs[i]/IntDivBelow);


        //----------------------------------------------------------------------
        // If the delta below is better than the delta we have, save the divider
        // values as the best case.  The PFD value is equal to the index plus
        // MIN_PFD_VALUE (18).  
        //----------------------------------------------------------------------        
        if(DeltaBelow < BestDelta)
        {
            BestIntDiv = IntDivBelow;
            BestPfdDiv = i + MIN_PFD_VALUE;
            BestDelta = DeltaBelow;        
        }         

        //----------------------------------------------------------------------
        // If the delta above is better than the delta we have, save the divider
        // values as the best case.  The PFD value is equal to the index plus
        // MIN_PFD_VALUE (18).  The DeltaAbove == 0 conditions allows us to use
        // the combination of dividers only if we get the exact frequency.  We
        // don't want to go faster than what was requested.  
        //----------------------------------------------------------------------        
        if(DeltaAbove < BestDelta && DeltaAbove == 0)
        {
            BestIntDiv = IntDivAbove;
            BestPfdDiv = i + MIN_PFD_VALUE;
            BestDelta = DeltaAbove;        
        }
      

        //----------------------------------------------------------------------
        // A previous calculation may have given us the exact frequency we 
        // were looking for.  If so, we don't need to check any more dividers
        // because we can't get any closer.  We'll end the for loop.  
        //----------------------------------------------------------------------        
        if(BestDelta == 0)
        {
            break;
        }         


    }

    //--------------------------------------------------------------------------
    // Return the values we've calculated.
    //--------------------------------------------------------------------------        
    *pu32RefClkFreq_kHz = PfdPllFreqs[BestPfdDiv - MIN_PFD_VALUE];
    *pu32ClkFreq_kHz = PfdPllFreqs[BestPfdDiv - MIN_PFD_VALUE]/BestIntDiv;
    *pu32IntDiv = BestIntDiv;
    *pu32PhaseFracDiv = BestPfdDiv;

    return SUCCESS;

}

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////  EOF  //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

