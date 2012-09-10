
#ifndef _max17042_h_
#define _max17042_h_

#define MAX17042_I2C_ADDR	(0x6C >> 1)

typedef struct max17042_fuel_gauge_linear_data {
    unsigned int min_vcell;
    unsigned int slope;
    unsigned int y_interception;
}fuel_gauge_linear_data;

u32 fuel_gauge_read_soc(void);
u32 fuel_gauge_read_socvf(void);
#if defined(CONFIG_SLP_U1)
void init_fuel_gauge(int charger_status);
#elif defined(CONFIG_P1P2) || defined(CONFIG_F1)
void init_fuel_gauge(u32 i2c_bus, uchar chip, int dc_in, int jig_on, int sense);
#endif
void init_fuel_gauge_with_reset(u32 i2c_bus, uchar chip, int battery_present, fuel_gauge_linear_data fuel_gauge_quick_table[10]);

#endif	/* _max17042_h_ */
