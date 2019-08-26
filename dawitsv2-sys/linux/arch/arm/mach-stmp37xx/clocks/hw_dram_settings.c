// Optomized Frequency Register values
#include <../include/types.h>

#ifdef __KERNEL__
 #include <asm/hardware.h>
#else
 #include "linux_regs.h"
#endif

void hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_24MHz(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 0] = 0x01010001;  /* 0000000_1 ahb0_w_priority 0000000_1 ahb0_r_priority 0000000_0 ahb0_fifo_type_reg 0000000_1 addr_cmp_en */
        DRAM_REG[ 1] = 0x00010100;  /* 0000000_0 ahb2_fifo_type_reg 0000000_1 ahb1_w_priority 0000000_1 ahb1_r_priority 0000000_0 ahb1_fifo_type_reg */
        DRAM_REG[ 2] = 0x01000101;  /* 0000000_1 ahb3_r_priority 0000000_0 ahb3_fifo_type_reg 0000000_1 ahb2_w_priority 0000000_1 ahb2_r_priority */
        DRAM_REG[ 3] = 0x00000001;  /* 0000000_0 auto_refresh_mode 0000000_0 arefresh 0000000_0 ap 0000000_1 ahb3_w_priority */
        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 5] = 0x00000001;  /* 0000000_0 intrptreada 0000000_0 intrptapburst 0000000_0 fast_write 0000000_1 en_lowpower_mode */
        DRAM_REG[ 6] = 0x00010000;  /* 0000000_0 power_down 0000000_1 placement_en 0000000_0 no_cmd_init 0000000_0 intrptwritea */
        DRAM_REG[ 7] = 0x01000101;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_1 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[ 9] = 0x00000001;  /* 000000_00 out_of_range_type 000000_00 out_of_range_source_id 0000000_0 write_modereg 0000000_1 writeinterp */
        DRAM_REG[10] = 0x07000200;  /* 00000_111 age_count 00000_000 addr_pins 000000_10 temrs 000000_00 q_fullness */
        DRAM_REG[11] = 0x00070303;  /* 00000_000 max_cs_reg 00000_111 command_age_count 00000_011 column_size 00000_011 caslat */
        DRAM_REG[12] = 0x02010002;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_010 tcke */
        DRAM_REG[13] = 0x06060a02;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_010 twtr */
        DRAM_REG[14] = 0x00000201;  /* -->Changed from 0x0000020f to 0x00000201<-- 0000_0000 max_col_reg 0000_0000 lowpower_refresh_enable 0000_0010 initaref 0000_1111 cs_map */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[16] = 0x02000000 | (DRAM_REG[16] & 0x00FF0000);  /* 000_00010 tmrd 000_00000 lowpower_control 000_00000 lowpower_auto_enable 0000_0000 int_ack */
        DRAM_REG[17] = 0x2d000102;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00010 trc */
        DRAM_REG[18] = 0x1f1f0000;  /* 0_0100000 dll_dqs_delay_1 0_0100000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f1010;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 01101010 dll_dqs_delay_bypass_1 01101010 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x01021010;  /* 00000001 trcd_int 00000010 tras_min 01101010 wr_dqs_shift_bypass 0_0100000 wr_dqs_shift */
        DRAM_REG[21] = 0x00000002;  /* 00000000000000_0000000000 out_of_range_length 00000010 trfc */
        DRAM_REG[22] = 0x00080008;  /* 00000_00000001000 ahb0_wrcnt 00000_00000001000 ahb0_rdcnt */
        DRAM_REG[23] = 0x00200020;  /* 00000_00000100000 ahb1_wrcnt 00000_00000100000 ahb1_rdcnt */
        DRAM_REG[24] = 0x00200020;  /* 00000_00000100000 ahb2_wrcnt 00000_00000100000 ahb2_rdcnt */
        DRAM_REG[25] = 0x00200020;  /* 00000_00000100000 ahb3_wrcnt 00000_00000100000 ahb3_rdcnt */
        DRAM_REG[26] = 0x000000b3;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[27] = 0x00000000;  /* 00000000000000000000000000000000 */
        DRAM_REG[28] = 0x00000000;  /* 00000000000000000000000000000000 */
        DRAM_REG[29] = 0x00000000;  /* 0000000000000000 lowpower_internal_cnt 0000000000000000 lowpower_external_cnt */
        DRAM_REG[30] = 0x00000000;  /* 0000000000000000 lowpower_refresh_hold 0000000000000000 lowpower_power_down_cnt */
        DRAM_REG[31] = 0x00000000;  /* 0000000000000000 tdll 0000000000000000 lowpower_self_refresh_cnt */
        DRAM_REG[32] = 0x00030687;  /* 0000000000000011 txsnr 0000011010000111 tras_max */
        DRAM_REG[33] = 0x00000003;  /* 0000000000000000 version 0000000000000011 txsr */
        DRAM_REG[34] = 0x000012c1;  /* 00000000000000000001001011000001 tinit */
        DRAM_REG[35] = 0x00000000;  /* 0_0000000000000000000000000000000 out_of_range_addr */
        DRAM_REG[36] = 0x00000101;  /* 0000000_0 pwrup_srefresh_exit 0000000_0 enable_quick_srefresh 0000000_1 bus_share_enable 0000000_1 active_aging */
        DRAM_REG[37] = 0x00040001;  /* 00000000000001_0000000000 bus_share_timeout 0000000_1 tref_enable */
        DRAM_REG[38] = 0x00000000;  /* 000_0000000000000 emrs2_data_0 000_0000000000000 emrs1_data */
        DRAM_REG[39] = 0x00000000;  /* 000_0000000000000 emrs2_data_2 000_0000000000000 emrs2_data_1 */
        DRAM_REG[40] = 0x00010000;  /* 0000000000000001 tpdex 000_0000000000000 emrs2_data_3 */
        DRAM_REG[ 8] = 0x01000000;  /* 0000000_1 tras_lockout 0000000_ start 0000000_0 srefresh 0000000_0 sdr_mode */

}

void hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_6MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 7] = 0x01000101;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_1 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[12] = 0x01010002;  /* 00000_001 twr_int 00000_001 trrd 0000000000000_010 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x2d000101;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00001 trc */
        DRAM_REG[18] = 0x20200000;  /* 0_0100000 dll_dqs_delay_1 0_0100000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f1414;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 01101010 dll_dqs_delay_bypass_1 01101010 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x01011608;  /* 00000001 trcd_int 00000001 tras_min 01101010 wr_dqs_shift_bypass 0_0100000 wr_dqs_shift */
        DRAM_REG[21] = 0x00000001;  /* 00000000000000_0000000000 out_of_range_length 00000001 trfc */
        DRAM_REG[26] = 0x0000002F;  /* 00000000000000000000_000000101111 tref */
        DRAM_REG[32] = 0x000101a5;  /* 0000000000000001 txsnr 0000000110100101 tras_max */
        DRAM_REG[33] = 0x00000001;  /* 0000000000000000 version 0000000000000001 txsr */
        DRAM_REG[34] = 0x000004b1;  /* 00000000000000000000010010110001 tinit */
        DRAM_REG[40] = 0x00010000;  /* 0000000000000001 tpdex 000_0000000000000 emrs2_data_3 */
}




void hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_24MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 7] = 0x01000101;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_1 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[12] = 0x02010002;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_010 tcke */
        DRAM_REG[13] = 0x06060a02;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_010 twtr */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x2d000102;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00010 trc */
        DRAM_REG[18] = 0x20200000;  /* 0_0100000 dll_dqs_delay_1 0_0100000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f1414;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 01101010 dll_dqs_delay_bypass_1 01101010 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x01021608;  /* 00000001 trcd_int 00000010 tras_min 01101010 wr_dqs_shift_bypass 0_0100000 wr_dqs_shift */
        DRAM_REG[21] = 0x00000002;  /* 00000000000000_0000000000 out_of_range_length 00000010 trfc */
        DRAM_REG[26] = 0x000000b3;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[32] = 0x00030687;  /* 0000000000000011 txsnr 0000011010000111 tras_max */
        DRAM_REG[33] = 0x00000003;  /* 0000000000000000 version 0000000000000011 txsr */
        DRAM_REG[34] = 0x000012c1;  /* 00000000000000000001001011000001 tinit */
        DRAM_REG[40] = 0x00010000;  /* 0000000000000001 tpdex 000_0000000000000 emrs2_data_3 */
}


void hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_48MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 7] = 0x01000101;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_1 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[13] = 0x06060a02;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_010 twtr */
        DRAM_REG[12] = 0x02010002;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_010 tcke */
        DRAM_REG[15] = 0x02040000;  /* 0000_0010 trp 0000_0100 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x2d000104;  /* 10011111 dll_start_point 00000000 dll_lock 00101010 dll_increment 000_00100 trc */
        DRAM_REG[18] = 0x1f1f0000;  /* 0_0011111 dll_dqs_delay_1 0_0011111 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f0a0a;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00101001 dll_dqs_delay_bypass_1 00101001 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x02030a10;  /* 00000010 trcd_int 00000011 tras_min 00101100 wr_dqs_shift_bypass 0_0100001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000004;  /* 00000000000000_0000000000 out_of_range_length 00000100 trfc */
        DRAM_REG[26] = 0x0000016f;  /* 00000000000000000000_000101101111 tref */
        DRAM_REG[32] = 0x00060d17;  /* 0000000000000110 txsnr 0000110100010111 tras_max */
        DRAM_REG[33] = 0x00000006;  /* 0000000000000000 version 0000000000000110 txsr */
        DRAM_REG[34] = 0x00002582;  /* 00000000000000000010010110000010 tinit */
        DRAM_REG[40] = 0x00020000;  /* 0000000000000010 tpdex 000_0000000000000 emrs2_data_3 */
}


void hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_60MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 7] = 0x01000101;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_1 rd2rd_turn 0000000_1 priority_en */
        //DRAM_REG[12] = 0x02010002;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_010 tcke */
        DRAM_REG[12] = 0x02020002;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_010 tcke */
        DRAM_REG[13] = 0x06060a02;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_010 twtr */
        DRAM_REG[15] = 0x02040000;  /* 0000_0010 trp 0000_0100 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x2d000005;  /* 01111110 dll_start_point 00000000 dll_lock 00100010 dll_increment 000_00101 trc */
        DRAM_REG[18] = 0x1f1f0000;  /* 0_0011111 dll_dqs_delay_1 0_0011111 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f0a0a;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00101001 dll_dqs_delay_bypass_1 00101001 dll_dqs_delay_bypass_0 */
        //DRAM_REG[20] = 0x02030a10;  /* 00000010 trcd_int 00000011 tras_min 00101100 wr_dqs_shift_bypass 0_0100001 wr_dqs_shift */
        DRAM_REG[20] = 0x02040a10;  /* 00000010 trcd_int 00000100 tras_min 00101100 wr_dqs_shift_bypass 0_0100001 wr_dqs_shift */
        //DRAM_REG[21] = 0x00000005;  /* 00000000000000_0000000000 out_of_range_length 00000101 trfc */
        DRAM_REG[21] = 0x00000006;  /* 00000000000000_0000000000 out_of_range_length 00000110 trfc */
        DRAM_REG[26] = 0x000001cc;  /* 00000000000000000000_000111001100 tref */
        DRAM_REG[32] = 0x00081060;  /* 0000000000001000 txsnr 0001000001100000 tras_max */
        DRAM_REG[33] = 0x00000008;  /* 0000000000000000 version 0000000000001000 txsr */
        DRAM_REG[34] = 0x00002ee5;  /* 00000000000000000010111011100101 tinit */
        DRAM_REG[40] = 0x00020000;  /* 0000000000000010 tpdex 000_0000000000000 emrs2_data_3 */
}


void hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_96MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 7] = 0x01000001;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[12] = 0x02020002;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_010 tcke */
        DRAM_REG[13] = 0x06070a02;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_010 twtr */
        DRAM_REG[15] = 0x03050000;  /* 0000_0011 trp 0000_0101 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x2d000808;  /* 01001110 dll_start_point 00000000 dll_lock 00010101 dll_increment 000_01000 trc */
        DRAM_REG[18] = 0x1f1f0000;  /* 0_0011111 dll_dqs_delay_1 0_0011111 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x020c1010;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00011010 dll_dqs_delay_bypass_1 00011010 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x0305101c;  /* 00000011 trcd_int 00000101 tras_min 00011100 wr_dqs_shift_bypass 0_0100010 wr_dqs_shift */
        //DRAM_REG[21] = 0x00000007;  /* 00000000000000_0000000000 out_of_range_length 00000111 trfc */
        DRAM_REG[21] = 0x00000007;  /* 00000000000000_0000000000 out_of_range_length 00000111 trfc */
        DRAM_REG[26] = 0x000002e6;  /* 00000000000000000000_001011100110 tref */
        DRAM_REG[32] = 0x000c1a3b;  /* 0000000000001100 txsnr 0001101000111011 tras_max */
        DRAM_REG[33] = 0x0000000c;  /* 0000000000000000 version 0000000000001100 txsr */
        DRAM_REG[34] = 0x00004b0d;  /* 00000000000000000100101100001101 tinit */
        DRAM_REG[40] = 0x00030000;  /* 0000000000000011 tpdex 000_0000000000000 emrs2_data_3 */
}


void hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_120MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 7] = 0x01000001;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[12] = 0x02020002;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_010 tcke */
        DRAM_REG[13] = 0x06070a02;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_010 twtr */
        DRAM_REG[15] = 0x03050000;  /* 0000_0011 trp 0000_0101 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x2300080a;  /* 00111111 dll_start_point 00000000 dll_lock 00010001 dll_increment 000_01010 trc */
        DRAM_REG[18] = 0x1f1f0000;  /* 0_0011111 dll_dqs_delay_1 0_0011111 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x020c1010;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00010101 dll_dqs_delay_bypass_1 00010101 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x0306101c;  /* 00000011 trcd_int 00000110 tras_min 00010111 wr_dqs_shift_bypass 0_0100011 wr_dqs_shift */
        DRAM_REG[21] = 0x00000009;  /* 00000000000000_0000000000 out_of_range_length 00001001 trfc */
        DRAM_REG[26] = 0x000003a1;  /* 00000000000000000000_001110100001 tref */
        DRAM_REG[32] = 0x000f20ca;  /* 0000000000001111 txsnr 0010000011001010 tras_max */
        DRAM_REG[33] = 0x0000000f;  /* 0000000000000000 version 0000000000001111 txsr */
        DRAM_REG[34] = 0x00005dca;  /* 00000000000000000101110111001010 tinit */
        DRAM_REG[40] = 0x00040000;  /* 0000000000000100 tpdex 000_0000000000000 emrs2_data_3 */
}


void hw_dram_Init_mobile_ddr_mt46h16m16lf_7_5_133MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 7] = 0x01000001;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[12] = 0x02020002;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_010 tcke */
        DRAM_REG[13] = 0x06070a02;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_010 twtr */
        DRAM_REG[15] = 0x03050000;  /* 0000_0011 trp 0000_0101 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x2000080a;  /* 00111001 dll_start_point 00000000 dll_lock 00001111 dll_increment 000_01010 trc */
        DRAM_REG[18] = 0x1f1f0000;  /* 0_0011111 dll_dqs_delay_1 0_0011111 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x020c1010;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00010011 dll_dqs_delay_bypass_1 00010011 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x0306101c;  /* 00000011 trcd_int 00000110 tras_min 00010101 wr_dqs_shift_bypass 0_0100011 wr_dqs_shift */
        DRAM_REG[21] = 0x0000000a;  /* 00000000000000_0000000000 out_of_range_length 00001010 trfc */
        DRAM_REG[26] = 0x00000408;  /* 00000000000000000000_010000001000 tref */
        DRAM_REG[32] = 0x0010245f;  /* 0000000000010000 txsnr 0010010001011111 tras_max */
        DRAM_REG[33] = 0x00000010;  /* 0000000000000000 version 0000000000010000 txsr */
        DRAM_REG[34] = 0x00006808;  /* 00000000000000000110100000001000 tinit */
        DRAM_REG[40] = 0x00040000;  /* 0000000000000100 tpdex 000_0000000000000 emrs2_data_3 */
}


#if 0
void hw_dram_Init_mobile_sdram_k4m56163pg_7_5_24MHz(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 0] = 0x01010001;  /* 0000000_1 ahb0_w_priority 0000000_1 ahb0_r_priority 0000000_0 ahb0_fifo_type_reg 0000000_1 addr_cmp_en */
        DRAM_REG[ 1] = 0x00010100;  /* 0000000_0 ahb2_fifo_type_reg 0000000_1 ahb1_w_priority 0000000_1 ahb1_r_priority 0000000_0 ahb1_fifo_type_reg */
        DRAM_REG[ 2] = 0x01000101;  /* 0000000_1 ahb3_r_priority 0000000_0 ahb3_fifo_type_reg 0000000_1 ahb2_w_priority 0000000_1 ahb2_r_priority */
        DRAM_REG[ 3] = 0x00000001;  /* 0000000_0 auto_refresh_mode 0000000_0 arefresh 0000000_0 ap 0000000_1 ahb3_w_priority */
        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 5] = 0x00000001;  /* 0000000_0 intrptreada 0000000_0 intrptapburst 0000000_0 fast_write 0000000_1 en_lowpower_mode */
        DRAM_REG[ 6] = 0x00010000;  /* 0000000_0 power_down 0000000_1 placement_en 0000000_0 no_cmd_init 0000000_0 intrptwritea */
        DRAM_REG[ 7] = 0x01000001;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[ 9] = 0x00000001;  /* 000000_00 out_of_range_type 000000_00 out_of_range_source_id 0000000_0 write_modereg 0000000_1 writeinterp */
        DRAM_REG[10] = 0x07000200;  /* 00000_111 age_count 00000_000 addr_pins 000000_10 temrs 000000_00 q_fullness */
        DRAM_REG[11] = 0x00070303;  /* 00000_000 max_cs_reg 00000_111 command_age_count 00000_011 column_size 00000_011 caslat */
        //DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[12] = 0x02020000;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[14] = 0x00000201;  /* -->Changed from 0x0000020f to 0x00000201<-- 0000_0000 max_col_reg 0000_0000 lowpower_refresh_enable 0000_0010 initaref 0000_1111 cs_map */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[16] = 0x02000000 | (DRAM_REG[16] & 0x00FF0000);  /* 000_00010 tmrd 000_00000 lowpower_control 000_00000 lowpower_auto_enable 0000_0000 int_ack */
        DRAM_REG[17] = 0x3e005502;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00010 trc */
        DRAM_REG[18] = 0x00000000;  /* 0_0000000 dll_dqs_delay_1 0_0000000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f0202;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00000010 dll_dqs_delay_bypass_1 00000010 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x01020201;  /* 00000001 trcd_int 00000010 tras_min 00000010 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        //DRAM_REG[21] = 0x00000002;  /* 00000000000000_0000000000 out_of_range_length 00000010 trfc */
        DRAM_REG[21] = 0x00000003;  /* 00000000000000_0000000000 out_of_range_length 00000011 trfc */
        DRAM_REG[22] = 0x00080008;  /* 00000_00000001000 ahb0_wrcnt 00000_00000001000 ahb0_rdcnt */
        DRAM_REG[23] = 0x00200020;  /* 00000_00000100000 ahb1_wrcnt 00000_00000100000 ahb1_rdcnt */
        DRAM_REG[24] = 0x00200020;  /* 00000_00000100000 ahb2_wrcnt 00000_00000100000 ahb2_rdcnt */
        DRAM_REG[25] = 0x00200020;  /* 00000_00000100000 ahb3_wrcnt 00000_00000100000 ahb3_rdcnt */
        DRAM_REG[26] = 0x000000b3;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[27] = 0x00000000;  /* 00000000000000000000000000000000 */
        DRAM_REG[28] = 0x00000000;  /* 00000000000000000000000000000000 */
        DRAM_REG[29] = 0x00000000;  /* 0000000000000000 lowpower_internal_cnt 0000000000000000 lowpower_external_cnt */
        DRAM_REG[30] = 0x00000000;  /* 0000000000000000 lowpower_refresh_hold 0000000000000000 lowpower_power_down_cnt */
        DRAM_REG[31] = 0x00000000;  /* 0000000000000000 tdll 0000000000000000 lowpower_self_refresh_cnt */
        DRAM_REG[32] = 0x00030957;  /* 0000000000000011 txsnr 0000100101010111 tras_max */
        DRAM_REG[33] = 0x00000003;  /* 0000000000000000 version 0000000000000011 txsr */
        DRAM_REG[34] = 0x000012c1;  /* 00000000000000000001001011000001 tinit */
        DRAM_REG[35] = 0x00000000;  /* 0_0000000000000000000000000000000 out_of_range_addr */
        DRAM_REG[36] = 0x00000101;  /* 0000000_0 pwrup_srefresh_exit 0000000_0 enable_quick_srefresh 0000000_1 bus_share_enable 0000000_1 active_aging */
        DRAM_REG[37] = 0x00040001;  /* 00000000000001_0000000000 bus_share_timeout 0000000_1 tref_enable */
        DRAM_REG[38] = 0x00000000;  /* 000_0000000000000 emrs2_data_0 000_0000000000000 emrs1_data */
        DRAM_REG[39] = 0x00000000;  /* 000_0000000000000 emrs2_data_2 000_0000000000000 emrs2_data_1 */
        DRAM_REG[40] = 0x00010000;  /* 0000000000000001 tpdex 000_0000000000000 emrs2_data_3 */
        // Will start the controller elsewhere
        DRAM_REG[ 8] = 0x00000001;  /* 0000000_0 tras_lockout 0000000_0 start 0000000_0 srefresh 0000000_1 sdr_mode */
}


void hw_dram_Init_mobile_sdram_k4m56163pg_7_5_6MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x3e005502;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00010 trc */
        DRAM_REG[18] = 0x00000000;  /* 0_0000000 dll_dqs_delay_1 0_0000000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x01020201;  /* 00000001 trcd_int 00000010 tras_min 00000010 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000002;  /* 00000000000000_0000000000 out_of_range_length 00000010 trfc */
        DRAM_REG[26] = 0x000000b3;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[32] = 0x00030957;  /* 0000000000000011 txsnr 0000100101010111 tras_max */
        DRAM_REG[33] = 0x00000003;  /* 0000000000000000 version 0000000000000011 txsr */
        DRAM_REG[34] = 0x000012c1;  /* 00000000000000000001001011000001 tinit */

}

void hw_dram_Init_mobile_sdram_k4m56163pg_7_5_24MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x3e005502;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00010 trc */
        DRAM_REG[18] = 0x00000000;  /* 0_0000000 dll_dqs_delay_1 0_0000000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x01020201;  /* 00000001 trcd_int 00000010 tras_min 00000010 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000002;  /* 00000000000000_0000000000 out_of_range_length 00000010 trfc */
        DRAM_REG[26] = 0x000000b3;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[32] = 0x00030957;  /* 0000000000000011 txsnr 0000100101010111 tras_max */
        DRAM_REG[33] = 0x00000003;  /* 0000000000000000 version 0000000000000011 txsr */
        DRAM_REG[34] = 0x000012c1;  /* 00000000000000000001001011000001 tinit */

}





void hw_dram_Init_mobile_sdram_k4m56163pg_7_5_48MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        //DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[12] = 0x02020000;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x02040000;  /* 0000_0010 trp 0000_0100 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x9f002a04;  /* 10011111 dll_start_point 00000000 dll_lock 00101010 dll_increment 000_00100 trc */
        DRAM_REG[18] = 0x01010000;  /* 0_0000001 dll_dqs_delay_1 0_0000001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x02030101;  /* 00000010 trcd_int 00000011 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000004;  /* 00000000000000_0000000000 out_of_range_length 00000100 trfc */
        DRAM_REG[26] = 0x0000016f;  /* 00000000000000000000_000101101111 tref */
        DRAM_REG[32] = 0x000612b7;  /* 0000000000000110 txsnr 0001001010110111 tras_max */
        DRAM_REG[33] = 0x00000006;  /* 0000000000000000 version 0000000000000110 txsr */
        DRAM_REG[34] = 0x00002582;  /* 00000000000000000010010110000010 tinit */

}

void hw_dram_Init_mobile_sdram_k4m56163pg_7_5_60MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x02040000;  /* 0000_0010 trp 0000_0100 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x7e002205;  /* 01111110 dll_start_point 00000000 dll_lock 00100010 dll_increment 000_00101 trc */
        DRAM_REG[18] = 0x01010000;  /* 0_0000001 dll_dqs_delay_1 0_0000001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f0202;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00000010 dll_dqs_delay_bypass_1 00000010 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x02040101;  /* 00000010 trcd_int 00000100 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000005;  /* 00000000000000_0000000000 out_of_range_length 00000101 trfc */
        DRAM_REG[26] = 0x000001cc;  /* 00000000000000000000_000111001100 tref */
        DRAM_REG[32] = 0x00081769;  /* 0000000000001000 txsnr 0001011101101001 tras_max */
        DRAM_REG[33] = 0x00000008;  /* 0000000000000000 version 0000000000001000 txsr */
        DRAM_REG[34] = 0x00002ee5;  /* 00000000000000000010111011100101 tinit */

}


void hw_dram_Init_mobile_sdram_k4m56163pg_7_5_96MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02020000;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x03050000;  /* 0000_0011 trp 0000_0101 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x4e001507;  /* 01001110 dll_start_point 00000000 dll_lock 00010101 dll_increment 000_00111 trc */
        DRAM_REG[18] = 0x01010000;  /* 0_0000001 dll_dqs_delay_1 0_0000001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x03050101;  /* 00000011 trcd_int 00000101 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000008;  /* 00000000000000_0000000000 out_of_range_length 00001000 trfc */
        DRAM_REG[26] = 0x000002e6;  /* 00000000000000000000_001011100110 tref */
        DRAM_REG[32] = 0x000c257d;  /* 0000000000001100 txsnr 0010010101111101 tras_max */
        DRAM_REG[33] = 0x0000000c;  /* 0000000000000000 version 0000000000001100 txsr */
        DRAM_REG[34] = 0x00004b0d;  /* 00000000000000000100101100001101 tinit */

}

void hw_dram_Init_mobile_sdram_k4m56163pg_7_5_120MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02020000;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06070a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x03050000;  /* 0000_0011 trp 0000_0101 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x3f001109;  /* 00111111 dll_start_point 00000000 dll_lock 00010001 dll_increment 000_01001 trc */
        DRAM_REG[18] = 0x11110000;  /* 0_0010001 dll_dqs_delay_1 0_0010001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f0202;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00000010 dll_dqs_delay_bypass_1 00000010 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x03070101;  /* 00000011 trcd_int 00000111 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x0000000a;  /* 00000000000000_0000000000 out_of_range_length 00001010 trfc */
        DRAM_REG[26] = 0x000003a1;  /* 00000000000000000000_001110100001 tref */
        DRAM_REG[32] = 0x000f2edb;  /* 0000000000001111 txsnr 0010111011011011 tras_max */
        DRAM_REG[33] = 0x0000000f;  /* 0000000000000000 version 0000000000001111 txsr */
        DRAM_REG[34] = 0x00005dca;  /* 00000000000000000101110111001010 tinit */

        //HW_DIGCTL_EMICLK_DELAY.B.NUM_TAPS = 2;
}


void hw_dram_Init_mobile_sdram_k4m56163pg_7_5_133MHz_optimized(void)
{
    volatile uint32_t * DRAM_REG = (volatile uint32_t *) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02020000;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06070a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x03050000;  /* 0000_0011 trp 0000_0101 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x39000f0a;  /* 00111001 dll_start_point 00000000 dll_lock 00001111 dll_increment 000_01010 trc */
        DRAM_REG[18] = 0x11110000;  /* 0_0011001 dll_dqs_delay_1 0_0011001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x03070101;  /* 00000011 trcd_int 00000111 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x0000000b;  /* 00000000000000_0000000000 out_of_range_length 00001011 trfc */
        DRAM_REG[26] = 0x00000408;  /* 00000000000000000000_010000001000 tref */
        DRAM_REG[32] = 0x001033fa;  /* 0000000000010000 txsnr 0011001111111010 tras_max */
        DRAM_REG[33] = 0x00000010;  /* 0000000000000000 version 0000000000010000 txsr */
        DRAM_REG[34] = 0x00006808;  /* 00000000000000000110100000001000 tinit */
        //HW_DIGCTL_EMICLK_DELAY.B.NUM_TAPS = 9;
}



void hw_dram_Init_sdram_mt48lc32m16a2_24MHz(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 0] = 0x01010001;  /* 0000000_1 ahb0_w_priority 0000000_1 ahb0_r_priority 0000000_0 ahb0_fifo_type_reg 0000000_1 addr_cmp_en */
        DRAM_REG[ 1] = 0x00010100;  /* 0000000_0 ahb2_fifo_type_reg 0000000_1 ahb1_w_priority 0000000_1 ahb1_r_priority 0000000_0 ahb1_fifo_type_reg */
        DRAM_REG[ 2] = 0x01000101;  /* 0000000_1 ahb3_r_priority 0000000_0 ahb3_fifo_type_reg 0000000_1 ahb2_w_priority 0000000_1 ahb2_r_priority */
        DRAM_REG[ 3] = 0x00000001;  /* 0000000_0 auto_refresh_mode 0000000_0 arefresh 0000000_0 ap 0000000_1 ahb3_w_priority */
        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[ 5] = 0x00000000;  /* 0000000_0 intrptreada 0000000_0 intrptapburst 0000000_0 fast_write 0000000_0 en_lowpower_mode */
        DRAM_REG[ 6] = 0x00010000;  /* 0000000_0 power_down 0000000_1 placement_en 0000000_0 no_cmd_init 0000000_0 intrptwritea */
        DRAM_REG[ 7] = 0x01000001;  /* 0000000_1 rw_same_en 0000000_0 reg_dimm_enable 0000000_0 rd2rd_turn 0000000_1 priority_en */
        DRAM_REG[ 9] = 0x00000001;  /* 000000_00 out_of_range_type 000000_00 out_of_range_source_id 0000000_0 write_modereg 0000000_1 writeinterp */
        DRAM_REG[10] = 0x07000200;  /* 00000_111 age_count 00000_000 addr_pins 000000_10 temrs 000000_00 q_fullness */
        DRAM_REG[11] = 0x00070203;  /* 00000_000 max_cs_reg 00000_111 command_age_count 00000_010 column_size 00000_011 caslat */
        DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[14] = 0x00000201;  /*-->Changed from 0x0000020f to 0x00000201<-- 0000_0000 max_col_reg 0000_0000 lowpower_refresh_enable 0000_0010 initaref 0000_1111 cs_map */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[16] = 0x02000000 | (DRAM_REG[16] & 0x00FF0000);  /* 000_00010 tmrd 000_00000 lowpower_control 000_00000 lowpower_auto_enable 0000_0000 int_ack */
        DRAM_REG[17] = 0x3e005502;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00010 trc */
        DRAM_REG[18] = 0x00000000;  /* 0_0000000 dll_dqs_delay_1 0_0000000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[19] = 0x027f0202;  /* 00000010 dqs_out_shift_bypass 0_1111111 dqs_out_shift 00000010 dll_dqs_delay_bypass_1 00000010 dll_dqs_delay_bypass_0 */
        DRAM_REG[20] = 0x01020201;  /* 00000001 trcd_int 00000010 tras_min 00000010 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000002;  /* 00000000000000_0000000000 out_of_range_length 00000010 trfc */
        DRAM_REG[22] = 0x00080008;  /* 00000_00000001000 ahb0_wrcnt 00000_00000001000 ahb0_rdcnt */
        DRAM_REG[23] = 0x00200020;  /* 00000_00000100000 ahb1_wrcnt 00000_00000100000 ahb1_rdcnt */
        DRAM_REG[24] = 0x00200020;  /* 00000_00000100000 ahb2_wrcnt 00000_00000100000 ahb2_rdcnt */
        DRAM_REG[25] = 0x00200020;  /* 00000_00000100000 ahb3_wrcnt 00000_00000100000 ahb3_rdcnt */
        //DRAM_REG[26] = 0x000000b3;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[26] = 0x00000179;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[27] = 0x00000000;  /* 00000000000000000000000000000000 */
        DRAM_REG[28] = 0x00000000;  /* 00000000000000000000000000000000 */
        DRAM_REG[29] = 0x00000000;  /* 0000000000000000 lowpower_internal_cnt 0000000000000000 lowpower_external_cnt */
        DRAM_REG[30] = 0x00000000;  /* 0000000000000000 lowpower_refresh_hold 0000000000000000 lowpower_power_down_cnt */
        DRAM_REG[31] = 0x00000000;  /* 0000000000000000 tdll 0000000000000000 lowpower_self_refresh_cnt */
        //DRAM_REG[32] = 0x00020b37;  /* 0000000000000010 txsnr 0000101100110111 tras_max */
        DRAM_REG[32] = 0x00020957;  /* 0000000000000010 txsnr 0000100101010111 tras_max */
        DRAM_REG[33] = 0x00000002;  /* 0000000000000000 version 0000000000000010 txsr */
        DRAM_REG[34] = 0x00000961;  /* 00000000000000000000100101100001 tinit */
        DRAM_REG[35] = 0x00000000;  /* 0_0000000000000000000000000000000 out_of_range_addr */
        DRAM_REG[36] = 0x00000101;  /* 0000000_0 pwrup_srefresh_exit 0000000_0 enable_quick_srefresh 0000000_1 bus_share_enable 0000000_1 active_aging */
        DRAM_REG[37] = 0x00040001;  /* 00000000000001_0000000000 bus_share_timeout 0000000_1 tref_enable */
        DRAM_REG[38] = 0x00000000;  /* 000_0000000000000 emrs2_data_0 000_0000000000000 emrs1_data */
        DRAM_REG[39] = 0x00000000;  /* 000_0000000000000 emrs2_data_2 000_0000000000000 emrs2_data_1 */
        DRAM_REG[40] = 0x00010000;  /* 0000000000000001 tpdex 000_0000000000000 emrs2_data_3 */
        // Will startup the control elsewhere
        DRAM_REG[ 8] = 0x00000001;  /* 0000000_0 tras_lockout 0000000_0 start 0000000_0 srefresh 0000000_1 sdr_mode */
}

void hw_dram_Init_sdram_mt48lc32m16a2_6MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x3e005501;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00001 trc */
        DRAM_REG[18] = 0x00000000;  /* 0_0000000 dll_dqs_delay_1 0_0000000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x01010201;  /* 00000001 trcd_int 00000001 tras_min 00000010 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000001;  /* 00000000000000_0000000000 out_of_range_length 00000001 trfc */
        DRAM_REG[26] = 0x0000005F;  /* 00000000000000000000_000001011111 tref */
        //DRAM_REG[32] = 0x00010b37;  /* 0000000000000001 txsnr 0000101100110111 tras_max */
        DRAM_REG[32] = 0x00020957;  /* 0000000000000010 txsnr 0000100101010111 tras_max */
        DRAM_REG[33] = 0x00000001;  /* 0000000000000000 version 0000000000000001 txsr */
        DRAM_REG[34] = 0x00000259;  /* 00000000000000000000001001011001 tinit */
}



void hw_dram_Init_sdram_mt48lc32m16a2_24MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x3e005502;  /* 00111110 dll_start_point 00000000 dll_lock 01010101 dll_increment 000_00010 trc */
        DRAM_REG[18] = 0x00000000;  /* 0_0000000 dll_dqs_delay_1 0_0000000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x01020201;  /* 00000001 trcd_int 00000010 tras_min 00000010 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000002;  /* 00000000000000_0000000000 out_of_range_length 00000010 trfc */
        //DRAM_REG[26] = 0x000000b3;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[26] = 0x00000179;  /* 00000000000000000000_000010110011 tref */
        DRAM_REG[32] = 0x00020b37;  /* 0000000000000010 txsnr 0000101100110111 tras_max */
        DRAM_REG[33] = 0x00000002;  /* 0000000000000000 version 0000000000000010 txsr */
        DRAM_REG[34] = 0x00000961;  /* 00000000000000000000100101100001 tinit */
}

void hw_dram_Init_sdram_mt48lc32m16a2_48MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x01030000;  /* 0000_0001 trp 0000_0011 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x9f002a04;  /* 10011111 dll_start_point 00000000 dll_lock 00101010 dll_increment 000_00100 trc */
        DRAM_REG[18] = 0x01010000;  /* 0_0000001 dll_dqs_delay_1 0_0000001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x01030101;  /* 00000001 trcd_int 00000011 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000004;  /* 00000000000000_0000000000 out_of_range_length 00000100 trfc */
        DRAM_REG[26] = 0x0000016f;  /* 00000000000000000000_000101101111 tref */
        //DRAM_REG[32] = 0x00041677;  /* 0000000000000100 txsnr 0001011001110111 tras_max */
        DRAM_REG[32] = 0x000412bf;  /* 0000000000000100 txsnr 0001001010111111 tras_max */
        DRAM_REG[33] = 0x00000004;  /* 0000000000000000 version 0000000000000100 txsr */
        //DRAM_REG[34] = 0x000012c1;  /* 00000000000000000001001011000001 tinit */
        DRAM_REG[34] = 0x00002581;  /* 00000000000000000010010110000001 tinit */
}


void hw_dram_Init_sdram_mt48lc32m16a2_60MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x01000101;  /* 0000000_1 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02010000;  /* 00000_010 twr_int 00000_001 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06060a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x02040000;  /* 0000_0010 trp 0000_0100 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x7e002204;  /* 01111110 dll_start_point 00000000 dll_lock 00100010 dll_increment 000_00100 trc */
        DRAM_REG[18] = 0x01010000;  /* 0_0000001 dll_dqs_delay_1 0_0000001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x02030101;  /* 00000010 trcd_int 00000011 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        //DRAM_REG[21] = 0x00000004;  /* 00000000000000_0000000000 out_of_range_length 00000100 trfc */
        DRAM_REG[21] = 0x00000005;  /* 00000000000000_0000000000 out_of_range_length 00000101 trfc */
        DRAM_REG[26] = 0x000001cc;  /* 00000000000000000000_000111001100 tref */
        //DRAM_REG[32] = 0x00051c19;  /* 0000000000000101 txsnr 0001110000011001 tras_max */
        DRAM_REG[32] = 0x00051769;  /* 0000000000000101 txsnr 0001011101101001 tras_max */
        DRAM_REG[33] = 0x00000005;  /* 0000000000000000 version 0000000000000101 txsr */
        DRAM_REG[34] = 0x00002ee5;  /* 00000000000000000010111011100101 tinit */
}

void hw_dram_Init_sdram_mt48lc32m16a2_96MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02020000;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06070a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x02040000;  /* 0000_0010 trp 0000_0100 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x4e001507;  /* 01001110 dll_start_point 00000000 dll_lock 00010101 dll_increment 000_00111 trc */
        DRAM_REG[18] = 0x01010000;  /* 0_0000001 dll_dqs_delay_1 0_0000001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x02050101;  /* 00000010 trcd_int 00000101 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        DRAM_REG[21] = 0x00000007;  /* 00000000000000_0000000000 out_of_range_length 00000111 trfc */
        DRAM_REG[26] = 0x000002e6;  /* 00000000000000000000_001011100110 tref */
        DRAM_REG[32] = 0x00082574;  /* 0000000000001000 txsnr 001011101101001 tras_max */
        DRAM_REG[33] = 0x00000008;  /* 0000000000000000 version 0000000000001000 txsr */
        DRAM_REG[34] = 0x00004b01;  /* 00000000000000000100101100000001 tinit */
}

void hw_dram_Init_sdram_mt48lc32m16a2_120MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02020000;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06070a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x03050000;  /* 0000_0011 trp 0000_0101 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x3f001108;  /* 00111111 dll_start_point 00000000 dll_lock 00010001 dll_increment 000_01000 trc */
        DRAM_REG[18] = 0x10100000;  /* 0_0010000 dll_dqs_delay_1 0_0010000 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x03060101;  /* 00000011 trcd_int 00000110 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        //DRAM_REG[21] = 0x00000008;  /* 00000000000000_0000000000 out_of_range_length 00001000 trfc */
        DRAM_REG[21] = 0x00000009;  /* 00000000000000_0000000000 out_of_range_length 00001001 trfc */
        DRAM_REG[26] = 0x000003a1;  /* 00000000000000000000_001110100001 tref */
        //DRAM_REG[32] = 0x000a383c;  /* 0000000000001010 txsnr 0011100000111100 tras_max */
        DRAM_REG[32] = 0x000a2def;  /* 0000000000001010 txsnr 0010110111101111 tras_max */
        DRAM_REG[33] = 0x0000000a;  /* 0000000000000000 version 0000000000001010 txsr */
        //DRAM_REG[34] = 0x00002ee5;  /* 00000000000000000010111011100101 tinit */
        DRAM_REG[34] = 0x00005dc1;  /* 00000000000000000101110111000001 tinit */
}

void hw_dram_Init_sdram_mt48lc32m16a2_133MHz_optimized(void)
{
    volatile uint32_t* DRAM_REG = (volatile uint32_t*) HW_DRAM_CTL00_ADDR;

        DRAM_REG[ 4] = 0x00000101;  /* 0000000_0 dll_bypass_mode 0000000_0 dlllockreg 0000000_1 concurrentap 0000000_1 bank_split_en */
        DRAM_REG[12] = 0x02020000;  /* 00000_010 twr_int 00000_010 trrd 0000000000000_000 tcke */
        DRAM_REG[13] = 0x06070a01;  /* 0000_0110 caslat_lin_gate 0000_0110 caslat_lin 0000_1010 aprebit 00000_001 twtr */
        DRAM_REG[15] = 0x03050000;  /* 0000_0011 trp 0000_0101 tdal 0000_0000 port_busy 0000_0000 max_row_reg */
        DRAM_REG[17] = 0x39000f09;  /* 00111001 dll_start_point 00000000 dll_lock 00001111 dll_increment 000_01001 trc */
        DRAM_REG[18] = 0x19190000;  /* 0_0011001 dll_dqs_delay_1 0_0011001 dll_dqs_delay_0 000_00000 int_status 000_00000 int_mask */
        DRAM_REG[20] = 0x03060101;  /* 00000011 trcd_int 00000110 tras_min 00000001 wr_dqs_shift_bypass 0_0000001 wr_dqs_shift */
        //DRAM_REG[21] = 0x00000009;  /* 00000000000000_0000000000 out_of_range_length 00001001 trfc */
        DRAM_REG[21] = 0x0000000a;  /* 00000000000000_0000000000 out_of_range_length 00001010 trfc */
        DRAM_REG[26] = 0x00000408;  /* 00000000000000000000_010000001000 tref */
        //DRAM_REG[32] = 0x000a3e61;  /* 0000000000001010 txsnr 0011111001100001 tras_max */
        DRAM_REG[32] = 0x000a33f3;  /* 0000000000001010 txsnr 0011001111110011 tras_max */
        DRAM_REG[33] = 0x0000000a;  /* 0000000000000000 version 0000000000001010 txsr */
        //DRAM_REG[34] = 0x00003404;  /* 00000000000000000011010000000100 tinit */
        DRAM_REG[34] = 0x00006808;  /* 00000000000000000110100000001000 tinit */
}
#endif

void emi_block_endddress(void)
{

}

//#pragma ghs section text=".ocram.text"    /// MUST BE IN OCRAM !!!!

