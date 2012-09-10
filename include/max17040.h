
#ifndef _MAX17040_H
#define _MAX17040_H

#define MAX17040_I2C_ADDRESS				0x36

#define MAX17040_DEFAULT_RCOMP_VALUE		0x9700
#define MAX17040_I2C_PRO_VALUE				0x5400
#define MAX17040_QUICK_START_VALUE			0x4000

//register map
#define MAX17040_REG_VCELL					0x2
#define MAX17040_REG_SOC					0x4
#define MAX17040_REG_MODE					0x6
#define MAX17040_REG_VERSION				0x08
#define MAX17040_REG_RCOMP					0x0c
#define MAX17040_REG_COMMAND				0xfe

//quick start condition check
#define S1_QUICK_START						0x1
#define S1_RCOMP_VALUE						0xb000

#define SLP45_QUICK_START                   0x2
#define SLP45_RCOMP_VALUE                   0xd000

typedef struct max17040_fuel_gauge_linear_data {
	unsigned int min_vcell;
	unsigned int slope;
	unsigned int y_interception;
}fuel_gauge_linear_data;
 
unsigned int init_fuel_gauge(u32 i2c_bus, int qstart_table_list, fuel_gauge_linear_data fuel_gauge_quick_table[10]);

#endif



