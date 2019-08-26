/* $Id: revtypechk.h,v 1.7 2007/09/19 05:58:52 biglow Exp $ */

/**
 * \file revtypechk.h
 * \brief Revision Information of Z5
 * \author Lee Tae Hun <th76.lee@samsung.com>
 * \version $Revision: 1.7 $
 * \date $Date: 2007/09/19 05:58:52 $
 *
 * For sub-model of Z5, we need to check it, 
 * because the h/w is changed some parts each model.
 * $Log: revtypechk.h,v $
 * Revision 1.7  2007/09/19 05:58:52  biglow
 * - board type detecting method changed
 *
 * -- Taehun Lee
 *
 * Revision 1.6  2006/03/06 10:43:02  biglow
 * - add detecting s/w version
 *
 * Revision 1.5  2006/03/06 10:41:36  biglow
 * - add detecting s/w version
 *
 * Revision 1.4  2006/02/22 10:18:52  biglow
 * - change GPIO option for capacity type touchpad
 *
 * Revision 1.3  2006/02/22 09:37:38  biglow
 * - add GPIO option for capacity type touchpad
 *
 * Revision 1.2  2006/02/22 04:38:37  biglow
 * - change unix type
 *
 * Revision 1.1  2006/02/17 08:28:42  biglow
 * - Add Check function H/W type
 *
 */

#ifndef __REVTYPE_CHK_H
#define __REVTYPE_CHK_H


/*------------ Defines ------------*/
/* By leeth, change board type detect method at 20070919 */
#define OPTION1_TP		24
#define OPTION2_TP		27
#define OPTION3_TP		28
#define OPTION4_TP		31

#define OPTION1_STR		"UNDEF"
#define OPTION1_STR0	"Unknown"
#define OPTION1_STR1	"Unknown"
#define OPTION2_STR		"UNDEF"
#define OPTION2_STR0	"Unknown"
#define OPTION2_STR1	"Unknown"
#define OPTION3_STR		"FM"
#define OPTION3_STR0	"FMONLY"
#define OPTION3_STR1	"RDS"
#define OPTION4_STR		"TOUCH"
#define OPTION4_STR0	"MELPAS"
#define OPTION4_STR1	"Unknown"

#define S_VER "VER"
#define S_MODEL "MODEL"

/* By leeth, change board type detect method at 20070919 */
typedef enum 
{
	OPTION1 = 0,
	OPTION2,
	OPTION3,
	OPTION4,
	MAX_OPTIONS
} optiontype_t;

/* By leeth, change board type detect method at 20070919 */
typedef struct 
{
	unsigned option[MAX_OPTIONS];
	char *string;
	int len;
} rev_inf_t;

typedef struct
{
	char version[16];
	char nation[4];
	char model[32];
} version_inf_t;


/*---------- External Functions ----------*/
/* By leeth, change board type detect method at 20070919 */
extern unsigned chk_option_type(optiontype_t option);
extern version_inf_t *chk_sw_version(void);


/*---------- Macro Definitions ----------*/
/* By leeth, change board type detect method at 20070919 */
#define REVTYPE_PORT() \
{ \
	HW_PINCTRL_CTRL_CLR(BM_PINCTRL_CTRL_SFTRST | \
		BM_PINCTRL_CTRL_CLKGATE ); \
	HW_PINCTRL_MUXSEL1_CLR(0xC3C30000); \
	HW_PINCTRL_MUXSEL1_SET(0xC3C30000); \
	HW_PINCTRL_DOE0_CLR((0x1 << OPTION1_TP) | \
	(0x1 << OPTION2_TP) | (0x1 << OPTION3_TP) | (0x1 << OPTION4_TP)); \
}


#endif /* __REVTYPE_CHK_H */

