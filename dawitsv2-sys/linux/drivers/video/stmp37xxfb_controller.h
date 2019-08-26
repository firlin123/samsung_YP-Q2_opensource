#if !defined (SMTP37XXFB_CONTROLLER_H)
#define SMTP37XXFB_CONTROLLER_H

void write_command_16(unsigned short cmd);
void write_data_16(unsigned short data);
void write_register(unsigned reg, unsigned value);

#endif
