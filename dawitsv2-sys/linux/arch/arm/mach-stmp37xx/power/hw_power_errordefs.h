#ifndef _HW_POWER_ERRORDEFS_H
#define _HW_POWER_ERRORDEFS_H

#include "../include/hw_errordefs.h"


#define ERROR_HW_DCDC_GENERAL                                    (ERROR_HW_DCDC_GROUP)
#define ERROR_HW_DCDC_INVALID_INPUT_PARAM                        (ERROR_HW_DCDC_GROUP + 1)


#define ERROR_HW_POWER_GROUP                                      (ERROR_HW_DCDC_GROUP)
#define ERROR_HW_POWER_GENERAL                                    (ERROR_HW_POWER_GROUP)
#define ERROR_HW_POWER_INVALID_POWER_FET_SETTING                  (ERROR_HW_POWER_GROUP + 0x1)
#define ERROR_HW_POWER_INVALID_LINREG_STEP                        (ERROR_HW_POWER_GROUP + 0x2)
#define ERROR_HW_POWER_UNSAFE_VDDD_VOLTAGE                        (ERROR_HW_POWER_GROUP + 0x3)
#define ERROR_HW_POWER_INVALID_VDDD_VOLTAGE                       (ERROR_HW_POWER_GROUP + 0x4)
#define ERROR_HW_POWER_INVALID_VDDD_BO_LEVEL                      (ERROR_HW_POWER_GROUP + 0x5)
#define ERROR_HW_POWER_UNSAFE_VDDA_VOLTAGE                        (ERROR_HW_POWER_GROUP + 0x6)
#define ERROR_HW_POWER_INVALID_VDDA_VOLTAGE                       (ERROR_HW_POWER_GROUP + 0x7)
#define ERROR_HW_POWER_INVALID_VDDA_BO_LEVEL                      (ERROR_HW_POWER_GROUP + 0x8)
#define ERROR_HW_POWER_UNSAFE_VDDIO_VOLTAGE                       (ERROR_HW_POWER_GROUP + 0x9)
#define ERROR_HW_POWER_INVALID_VDDIO_VOLTAGE                      (ERROR_HW_POWER_GROUP + 0xA)
#define ERROR_HW_POWER_INVALID_VDDIO_BO_LEVEL                     (ERROR_HW_POWER_GROUP + 0xB)
#define ERROR_HW_POWER_AUTO_XFER_TO_DCDC_ENABLED                  (ERROR_HW_POWER_GROUP + 0xC) 
#define ERROR_HW_POWER_INVALID_BATT_MODE                          (ERROR_HW_POWER_GROUP + 0xD)
#define ERROR_HW_POWER_INVALID_INPUT_PARAM                        (ERROR_HW_POWER_GROUP + 0xE)
#define ERROR_HW_POWER_BAD_ARGUMENT                               (ERROR_HW_POWER_GROUP + 0xF)

#endif//_HW_POWER_ERRORDEFS_H 
