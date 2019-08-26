////////////////////////////////////////////////////////////////////////////////
//! \addtogroup hw_power
//! @{
//
// Copyright (c) 2004 - 2007 SigmaTel, Inc.
//
//! \file hw_power.c
//! \brief Contains hardware API for the 37xx power peripheral.
//! \version 0.1
//! \date 03/2007
//!
//! This file contains the hardware accessing functions for the on-chip
//! power peripheral.
//!
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes and external references
////////////////////////////////////////////////////////////////////////////////
#include "hw_power_common.h"

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////
static hw_power_5vDetection_t DetectionMethod;

////////////////////////////////////////////////////////////////////////////////
// Code							  
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
RtStatus_t hw_power_Enable5vDetection(hw_power_5vDetection_t eDetection)
{
    //--------------------------------------------------------------------------
    // Set the detection method for all the 5V detection calls
    //--------------------------------------------------------------------------
    DetectionMethod = eDetection;

    //--------------------------------------------------------------------------
    // Disable hardware 5V removal powerdown
    //--------------------------------------------------------------------------
    BF_CLR(POWER_5VCTRL, PWDN_5VBRNOUT);

    //--------------------------------------------------------------------------
    // Prepare the hardware for the detection method.  We used to set and clear
    // the VBUSVALID_5VDETECT bit, but that is also used for the DCDC 5V
    // detection.  It is sufficient to just check the status bits to see if 
    // 5V is present.  
    //--------------------------------------------------------------------------

    //----------------------------------------------------------------------
    // Use VBUSVALID for DCDC 5V detection.  The DCDC's detection is 
    // different than the USB/5V detection used to switch profiles.  This
    // is used to determine when a handoff should occur.  
    //----------------------------------------------------------------------
    BF_SET(POWER_5VCTRL, VBUSVALID_5VDETECT);

    //------------------------------------------------------------------
    // Set 5V detection threshold to normal level for VBUSVALID.  
    //------------------------------------------------------------------
    hw_power_SetVbusValidThresh(VBUS_VALID_THRESH_NORMAL);


    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
bool hw_power_Get5vPresentFlag(void)
{
    switch(DetectionMethod)
    {
        //----------------------------------------------------------------------
        // Check VBUSVALID for 5V present
        //----------------------------------------------------------------------
        case HW_POWER_5V_VBUSVALID:

            return HW_POWER_STS.B.VBUSVALID;            
        
        //----------------------------------------------------------------------
        // Check VDD5V_GT_VDDIO for 5V present
        //----------------------------------------------------------------------
        case HW_POWER_5V_VDD5V_GT_VDDIO:
        
            return HW_POWER_STS.B.VDD5V_GT_VDDIO;
            
        //----------------------------------------------------------------------
        // Invalid case.
        //----------------------------------------------------------------------
        default:
        
            break;    
    }

    return 0;

}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_Enable5vPlugInDetect(bool bEnable)
{
    switch(DetectionMethod)
    {
        case HW_POWER_5V_VBUSVALID:

            //------------------------------------------------------------------
            // Set the VBUSVALID interrupt polarity.
            //------------------------------------------------------------------

            if(bEnable)
            {
                hw_power_SetVbusValidInterruptPolarity(HW_POWER_CHECK_5V_CONNECTED);
            }
            else
            {
                hw_power_SetVbusValidInterruptPolarity(HW_POWER_CHECK_5V_DISCONNECTED);        
            }
            break;


        case HW_POWER_5V_VDD5V_GT_VDDIO:

            //------------------------------------------------------------------
            // Set the VDD5V_GT_VDDIO interrupt polarity.
            //------------------------------------------------------------------

            if(bEnable)
            {
                hw_power_SetVdd5vGtVddioInterruptPolarity(HW_POWER_CHECK_5V_CONNECTED);
            }
            else
            {
                hw_power_SetVdd5vGtVddioInterruptPolarity(HW_POWER_CHECK_5V_DISCONNECTED);        
            }

            break;


        default:

            //------------------------------------------------------------------
            // Not a valid state.
            //------------------------------------------------------------------
            break;
    }

}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_Enable5vUnplugDetect(bool bEnable)
{
    switch(DetectionMethod)
    {
        case HW_POWER_5V_VBUSVALID:

            //------------------------------------------------------------------
            // Set the VBUSVALID interrupt polarity.
            //------------------------------------------------------------------
 
            if(bEnable)
            {
                hw_power_SetVbusValidInterruptPolarity(HW_POWER_CHECK_5V_DISCONNECTED);
            }
            else
            {
                hw_power_SetVbusValidInterruptPolarity(HW_POWER_CHECK_5V_CONNECTED);        
            }
            break;



        case HW_POWER_5V_VDD5V_GT_VDDIO:

            //------------------------------------------------------------------
            // Set the VDD5V_GT_VDDIO interrupt polarity.
            //------------------------------------------------------------------
 
            if(bEnable)
            {
                hw_power_SetVdd5vGtVddioInterruptPolarity(HW_POWER_CHECK_5V_DISCONNECTED);
            }
            else
            {
                hw_power_SetVdd5vGtVddioInterruptPolarity(HW_POWER_CHECK_5V_CONNECTED);        
            }
            break;


        default:

            //------------------------------------------------------------------
            // Not a valid state.
            //------------------------------------------------------------------
            break;
    }       
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
bool hw_power_Get5vInterruptPolarity(void)
{
    switch(DetectionMethod)
    {
        case HW_POWER_5V_VBUSVALID:
        {            
            return HW_POWER_CTRL.B.POLARITY_VBUSVALID;
        }

        case HW_POWER_5V_VDD5V_GT_VDDIO:
        {
            return HW_POWER_CTRL.B.POLARITY_VDD5V_GT_VDDIO;
        }

        default:
        {
            return false;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_Enable5vInterrupt(bool bEnable)
{
    switch(DetectionMethod)
    {
        case HW_POWER_5V_VBUSVALID:
            
            if(bEnable)
            {
                //------------------------------------------------------------------
                // Enable VBUSVALID for 5V detection.
                //------------------------------------------------------------------
                hw_power_EnableVbusValidInterrupt(TRUE);
                hw_power_EnableVdd5vGtVddioInterrupt(FALSE);

            }
            else
            {
                //------------------------------------------------------------------
                // Disable VBUSVALID 5V detection.
                //------------------------------------------------------------------
                hw_power_EnableVbusValidInterrupt(FALSE);        
            }

            break;

        case HW_POWER_5V_VDD5V_GT_VDDIO:
            
            if(bEnable)
            {
                //--------------------------------------------------------------
                // Enable VDD5V_GT_VDDIO for 5V detection.
                //--------------------------------------------------------------
                hw_power_EnableVbusValidInterrupt(FALSE);
                hw_power_EnableVdd5vGtVddioInterrupt(TRUE);

            }
            else
            {
                //--------------------------------------------------------------
                // Disable VDD5V_GT_VDDIO for 5V detection.
                //--------------------------------------------------------------
                hw_power_EnableVdd5vGtVddioInterrupt(FALSE);        
            }
            break;

        default:

            //------------------------------------------------------------------
            // Not a valid state.
            //------------------------------------------------------------------
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//! See hw_power.h for details.
////////////////////////////////////////////////////////////////////////////////
void hw_power_Clear5vInterrupt(void)
{
    switch(DetectionMethod)
    {
        case HW_POWER_5V_VBUSVALID:
            
            //------------------------------------------------------------------
            // Clear the VBUSVALID interrupt.
            //------------------------------------------------------------------
            hw_power_ClearVbusValidInterrupt();        

            break;

        case HW_POWER_5V_VDD5V_GT_VDDIO:
            
            //------------------------------------------------------------------
            // Clear the VDD5V_GT_VDDIO interrupt.
            //------------------------------------------------------------------
            hw_power_ClearVdd5vGtVddioInterrupt();        

            break;

        default:

            //------------------------------------------------------------------
            // Not a valid state.
            //------------------------------------------------------------------
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

