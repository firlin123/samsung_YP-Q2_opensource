////////////////////////////////////////////////////////////////////////////////
//! \addtogroup ddi_power
//! @{
//
// Copyright(C) 2005 SigmaTel, Inc.
//
//! \file ddi_power_init.c
//! \brief Implementation file for the power driver.
//!
//!
////////////////////////////////////////////////////////////////////////////////
//   Includes and external references
////////////////////////////////////////////////////////////////////////////////
#include "ddi_power_common.h"

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////
extern ddi_power_PowerHandoff_t   g_ddi_power_Handoff;

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Globals & Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Initize the power driver
//!
//! \fntype Function
//! This function initializes the power-supply hardware.
//!
//! \param[in]  eLradcDelayTrigger    LRADC delay trigger for battery measurement
//!
//! \return     SUCCESS                         No error
//!             Others                          Error
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_Init(ddi_power_InitValues_t* pInitValues)
{
    RtStatus_t Status;
    uint32_t u32SampleInterval;
    hw_power_5vDetection_t eDetection;

    //--------------------------------------------------------------------------
    // Initialize the power block.
    //--------------------------------------------------------------------------
    {
        if((Status = hw_power_Init()) != SUCCESS)
            return Status;
    }


    //--------------------------------------------------------------------------
    // Initialize the battery monitor.
    //--------------------------------------------------------------------------
    {
        u32SampleInterval = pInitValues->u32BatterySamplingInterval;

        //----------------------------------------------------------------------
        // Set up the battery monitor and return if unsuccessful.
        //----------------------------------------------------------------------
        if((Status = hw_power_InitBatteryMonitor(u32SampleInterval)) != SUCCESS)
            return Status;
    }


    //--------------------------------------------------------------------------
    // Initialize 5V/USB detection.
    //--------------------------------------------------------------------------
    {

        //----------------------------------------------------------------------
        // Parse the init structure for the 5V detection method.
        //----------------------------------------------------------------------
        eDetection = (hw_power_5vDetection_t) pInitValues->e5vDetection;

        //----------------------------------------------------------------------
        // Initialize the 5V or USB insertion/removal detection.  Does not
        // prepare the 5V-DCDC/DCDC-5V handoff.
        //----------------------------------------------------------------------
        if((Status = hw_power_Enable5vDetection(eDetection)) != SUCCESS)
            return Status;
    }


    //----------------------------------------------------------------------
    // Configure the power supplies for their current power source.
    //----------------------------------------------------------------------
    {

        if((Status = hw_power_InitPowerSupplies()) != SUCCESS)
            return Status;
    }

    //----------------------------------------------------------------------
    // If we are here, the power driver is initialized.
    //----------------------------------------------------------------------
    return SUCCESS;

}

#if 0
int set_battery_charging_2(unsigned int voltage, unsigned int threshold)
{
        unsigned int power_battchrg_value;
        
        HW_POWER_CTRL_CLR(BM_POWER_CTRL_CLKGATE);
        
        power_battchrg_value = HW_POWER_CHARGE_RD();
        power_battchrg_value &= ~(BM_POWER_CHARGE_BATTCHRG_I|BM_POWER_CHARGE_STOP_ILIMIT);
        
        if(voltage == 0){
                power_battchrg_value |= BM_POWER_CHARGE_PWD_BATTCHRG;
        }       
        else{
                power_battchrg_value |= BF_POWER_CHARGE_BATTCHRG_I(voltage);
                // Setting Current Threshold to 50 mA 
                power_battchrg_value |= BF_POWER_CHARGE_STOP_ILIMIT(threshold);
                power_battchrg_value &= ~BM_POWER_CHARGE_PWD_BATTCHRG;
        }       
        
        //write to charging register
        HW_POWER_CHARGE_WR(power_battchrg_value);
        
        return 1; //BATT_SUCCESS;
}       

int check_batt_status_2(void)
{
        int i;
        long tot_value = 0, batt_ch7conversionValue = 0;
        int batt_stat, batt_average = 0, num_check = 0;
        int init_batt_cnt = 0;
 
        do
        {
batt_loop:
                for(i = 0; i < 10; i++) //10ms
                        udelay(1000);
                /* Reading Battery Value */
                batt_ch7conversionValue = HW_POWER_BATTMONITOR.B.BATT_VAL;
 
                //BF_CLRn(LRADC_CHn, 7, VALUE);
                //HW_LRADC_CTRL0_WR(0x80);  // schedule ch6, ch7 fix for TB1
 
                init_batt_cnt++;
 
                //printk("[Battery check] %d batterylevel is %d\n\n\n", init_batt_cnt, batt_ch7conversionValue);
 
                if(init_batt_cnt > 10) //ITER_LOW_BATT_CHECK)
                        break;
 
                if(batt_ch7conversionValue == 0)
                        goto batt_loop;
 
                num_check++;
 
                tot_value += batt_ch7conversionValue;
 
                if(num_check % 10)
                        goto batt_loop;
                else
                        batt_average = tot_value / 10;
 
        }while(batt_ch7conversionValue == 0);
 
        /* By leeth, Change Threshold detecting No-Battery State at 20061011 */
        /* By leeth, check less 3.1V at 20060913 */
#if 0
        if(batt_average < 415 )//TOO_LOW_BATTLEVEL)
        {
                batt_stat = -1;
        }
        else if(batt_average < 475)//BATT_EXIST_THRESHOLD)
        {
                batt_stat = 0;
        }
        else if(batt_average < 490) //NO_BATT_THRESHOLD) /* hot fix by Lee */
        {
                batt_stat = 1;
        }
        else
        {
                batt_stat = 2;
        }
#endif
        //printk("[Battery check] batterylevel is %d ,batt_average = %d\n\n", batt_stat, batt_average);
 
        //return batt_stat;
        return batt_average;
}

int stmp37xx_check_battery_2(void)
{
        int First_checked_level = 0;
        int Second_checked_level = 0;
        int icheckedlevel = 0;
        int i = 0;
	int is_battery = 0;
 
        set_battery_charging_2(0x00, 0x00); //SS_CHG_OFF, SS_THRESHOLD_OFF  
 
        First_checked_level = check_batt_status_2();
        printk("[Battery check] First_checked_level is %d\n\n", First_checked_level);
 
        if (First_checked_level < 200) {
                printk("There is no battery or battery is abnormal state \n");
                is_battery = 0;
                return is_battery;
        }
 
        /* H.Lim - The definition of check level on this time is different with nandboot_main. */
        //if(icheckedlevel <= 0)
        //{
        set_battery_charging_2(0x24, 0x04); //SS_CHG_450, SS_THRESHOLD_450 
 
        for(i = 0; i < 10; i++) //10 = 10ms
                udelay(1000);
 
        Second_checked_level = check_batt_status_2();
        printk("[Battery check] Second_checked_level is %d\n\n", Second_checked_level);
 
        if ( (Second_checked_level - First_checked_level) > 100) {
                printk("After charging, There is no battery\n");
                is_battery = 0;
        }
        else {
                printk("After charging, There is battery\n");
                is_battery = 1;
        }
 
 
#if 0
                if(check_batt_status_2() > 1)
                {
                        is_battery = 0;
                        printk("[Battery_Mon] after charging, decide no battery\n");
                }
                else
                {
                        is_battery = 1;
                        printk("[Battery_Mon] after charging, decide battery is exist\n");
                }
                set_battery_charging_2(0x00, 0x00); //SS_CHG_OFF, SS_THRESHOLD_OFF  
        //}
        //else
        //{
        //        is_battery = 1;
        //        printk("[Battery_Mon] normal battery state\n");
        //}
#endif
 
        set_battery_charging_2(0x00, 0x00); //SS_CHG_OFF, SS_THRESHOLD_OFF  
        //return 0;
        return is_battery;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Initializes the power handoff handler
//!
//! \fntype Function
//! This function sets up battery-5V handoff, installs the VDD5V interrupt
//! handler, creates a TX application timer to handle the 5V-battery handoff
//! debounce, and calls the callback functions:
//!     pFnxHandoffStartCallback on a handoff event detected,
//!     pFnxHandoffEndCallback on the handoff debounce end,
//!     pFxnHandoffTo5VoltCallback on the handoff-to-5V debounce end,
//!     pFxnHandoffToBatteryCallback on the handoff-to-battery debounce end.
//!
//! \param[in]  pFxnHandoffStartCallback        Pointer to the handoff-start callback function
//! \param[in]  pFxnHandoffEndCallback          Pointer to the handoff-end callback function
//! \param[in]  pFxnHandoffTo5VoltCallback      Pointer to the send-handoff-to-5V message function
//! \param[in]  pFxnHandoffToBatteryCallback    Pointer to the send-handoff-to-battery message function
//! \param[in]  u32HandoffDebounce              Unused.
//!
//! \return     SUCCESS                         No error
//!             Others                          Error
//!
//! \note This function runs in ThreadX environment
//!
////////////////////////////////////////////////////////////////////////////////
RtStatus_t  ddi_power_HandoffInit(ddi_power_MsgHandler_t    *pFxnHandoffStartCallback,
                                  ddi_power_MsgHandler_t    *pFxnHandoffEndCallback,
                                  ddi_power_MsgHandler_t    *pFxnHandoffTo5VoltCallback,
                                  ddi_power_MsgHandler_t    *pFxnHandoffToBatteryCallback,
                                  uint32_t                u32HandoffDebounce,
				  uint32_t		  battery_check)
{
	//int battery_check = 0;

    //--------------------------------------------------------------------------
    // Init the callback function pointers.
    //--------------------------------------------------------------------------
    g_ddi_power_Handoff.pFxnHandoffStartCallback      = pFxnHandoffStartCallback;
    g_ddi_power_Handoff.pFxnHandoffEndCallback        = pFxnHandoffEndCallback;
    g_ddi_power_Handoff.pFxnHandoffTo5VoltCallback    = pFxnHandoffTo5VoltCallback;
    g_ddi_power_Handoff.pFxnHandoffToBatteryCallback  = pFxnHandoffToBatteryCallback;

#if 0
	HW_POWER_5VCTRL_CLR(0x1<<2); //always linreg vddio enable, dhsong
        HW_POWER_VDDDCTRL_SET(0x1<<21); //always linreg vddd enable, dhsong 
        HW_POWER_VDDDCTRL_SET(0x1<<22); //enable LINREG_FROM_BATT, dhsong
        HW_POWER_VDDACTRL_SET(0x1<<17); //always linreg vdda enable, dhsong
#endif

    //--------------------------------------------------------------------------
    // Check for 5V present.  We'll prepare the appropriate handoff.  
    //--------------------------------------------------------------------------
#if 1
    if (hw_power_Get5vPresentFlag())
    {
 #if 1 //add for checking battery, dhsong
	//battery_check = stmp37xx_check_battery_2();

	if(battery_check == 1){ //There is battery
		hw_power_EnableDcdc(true);

		hw_power_SetVdddPowerSource(HW_POWER_DCDC_LINREG_READY);
		//hw_power_SetVddaPowerSource(HW_POWER_DCDC_LINREG_READY);
                //hw_power_SetVddioPowerSource(HW_POWER_DCDC_LINREG_ON);

                /*20081229, add to control vddio, vdda with linear regulator */
                /*to reduce load of battery when 5v is connected */
		//printk("<pm> 20081229 add to control vddio, vdda with linear regulator\n");
                hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
                hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);

		HW_POWER_CHARGE.B.PWD_BATTCHRG = 1; //power down pwd_battchrg, add 20090108, dhsong

	}
	else if(battery_check == 0) {
		hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_READY);
		hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
		hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);		

		hw_power_EnableDcdc(false);
	}

 #endif

#else
    if (battery_check == 0) {
		hw_power_SetVdddPowerSource(HW_POWER_LINREG_DCDC_READY);
		hw_power_SetVddaPowerSource(HW_POWER_LINREG_DCDC_READY);
		hw_power_SetVddioPowerSource(HW_POWER_LINREG_DCDC_READY);		

		hw_power_EnableDcdc(false);

#endif
        //----------------------------------------------------------------------
        // It's 5V mode, enable 5V-to-battery handoff.
        //----------------------------------------------------------------------
        /* HOTFIX:: by leeth, except for VDDA, enable Power rail brownout at 20090721 */
        //ddi_power_ExecuteBatteryTo5VoltsHandoff();
        ddi_power_Enable5VoltsToBatteryHandoff();
    }
    else
    {

        //----------------------------------------------------------------------
        // It's battery mode, enable battery-to-5V handoff.
        //----------------------------------------------------------------------
        /* HOTFIX:: by leeth, except for VDDA, enable Power rail brownout at 20090721 */
        //ddi_power_Execute5VoltsToBatteryHandoff();
        ddi_power_EnableBatteryTo5VoltsHandoff();

	HW_POWER_CHARGE.B.PWD_BATTCHRG = 1; //power down pwd_battchrg, add 20090108, dhsong
	//printk("%d, HW_POWER_5VCTRL = 0x%08x\n", __LINE__, HW_POWER_5VCTRL_RD() );

    }

    //--------------------------------------------------------------------------
    // Done.
    //--------------------------------------------------------------------------
    return SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// End of file
////////////////////////////////////////////////////////////////////////////////
//! @}

