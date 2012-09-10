/*
 * mDNIe CMC623 controller driver
 */ 

#include <common.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <asm/errno.h>
#include <i2c.h>

static struct s5pc110_gpio *gpio;

void chip_reset(void)
{
	// CMC_RST	RESETB(K6)
	// TODO : Need specific number for reset timing delay...
	gpio_set_value(&gpio->c1, 2, 1);
	udelay(1000*0.1);
	gpio_set_value(&gpio->c1, 2, 0);
	udelay(1000*4);
	gpio_set_value(&gpio->c1, 2, 1);
	udelay(1000*0.3);
}

void cmc623_i2c_write(uchar slaveaddr, uint addr, uint data)
{
	uchar val[2] = {0,};

	val[0] = (data >> 8) & 0xff;
	val[1] = data & 0xff;

	i2c_write(slaveaddr, addr, 1, val, 2);
}

void change_path(int mode)
{
	// BYPASSB(A2) <= LOW, mdnie <=hight
	gpio_set_value(&gpio->c1, 4, mode);
	chip_reset();
}

enum {
	I2C_2,
	I2C_GPIO3,
	I2C_PMIC,
	I2C_GPIO5,
	I2C_GPIO6,
	I2C_GPIO7,
	I2C_GPIO9,
	I2C_GPI10,
};

#define CMC623_I2C_ADDRESS	0x38

void send_init_command(void)
{
	i2c_set_bus_num(I2C_GPI10);
	if (i2c_probe(CMC623_I2C_ADDRESS))
	{
		printf("Can't find cmc623\n");
		return;
	}

	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x00, 0x0000);    //BANK 0
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x01, 0x0020);    //algorithm selection
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0xb4, 0xC000);    //PWM ratio
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0xb3, 0xffff);    //up/down step
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x10, 0x001A);    // PCLK Polarity Sel
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x24, 0x0001);    // Polarity Sel	  
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x0b, 0x0184);    // Clock Gating
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x0f, 0x0010); 	// PWM clock ratio
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x0d, 0x1a08); 	// A-Stage clk
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x0e, 0x0809); 	// B-stage clk	  
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x22, 0x0400); 	// H_Size
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x23, 0x0258); 	// V_Size
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x2c, 0x0fff); 	   //DNR bypass
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x2d, 0x1900); 	   //DNR bypass
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x2e, 0x0000); 	   //DNR bypass
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x2f, 0x00ff); 	   //DNR bypass
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x3a, 0x0000);    //HDTR on DE, 
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x00, 0x0001);    //BANK 1
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x09, 0x0400);    // H_Size
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x0a, 0x0258);    // V_Size
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x0b, 0x0400);    // H_Size
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x0c, 0x0258);    // V_Size
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x01, 0x0500);    // BF_Line
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x06, 0x0074);    // Refresh time
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x07, 0x2225);    // eDRAM
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x68, 0x0080);    // TCON Polarity
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x6c, 0x0a32);    // VLW,HLW
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x6d, 0x0b0a);    // VBP,VFP
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x6e, 0xd28e);    // HBP,HFP
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x00, 0x0000);
	
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x28, 0x0000);
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x09, 0x0000);
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x09, 0xffff);
	
	//delay 5ms
	udelay(5*1000);
	
	cmc623_i2c_write(CMC623_I2C_ADDRESS, 0x26, 0x0001);

	return;
}



void cmc623_onoff(int on)
{
	gpio_set_value(&gpio->c1, 1, on);	//CMC_EN	1.2/1.8/3.3V
	gpio_set_value(&gpio->c1, 3, on);	//CMC_SHDN	FAILSAFEB(E6)
	gpio_set_value(&gpio->c1, 0, on);	//CMC_SLEEP	SLEEPB(L6)
	gpio_set_value(&gpio->c1, 4, on);	//CMC_BYPASS	BYPASSB(A2)

	if (on)
		chip_reset();
}

void cmc623_gpio_init(void)
{
	int i = 0;

	gpio = (struct s5pc110_gpio *)samsung_get_base_gpio();

	//CMC_SLEEP, CMC_EN, CMC_RST, CMC_SHDN, CMC_BYPASS
	for (i = 0; i < 5; i++) {
		gpio_set_value(&gpio->c1, i, 0);
		gpio_cfg_pin(&gpio->c1, i, GPIO_OUTPUT);
		gpio_set_pull(&gpio->c1, i, GPIO_PULL_NONE);
	}
}

void cmc623_init(void)
{
	/* change_path(0); */
	send_init_command();
}

void cmc623_set_brightness(int level)
{
	u16 pData = (0x18 << 11) | level;

	i2c_set_bus_num(I2C_GPI10);

	if (i2c_probe(CMC623_I2C_ADDRESS))
	{
		printf("Can't find cmc623\n");
		return;
	}

	cmc623_i2c_write(CMC623_I2C_ADDRESS,0x00, 0x0000);
	cmc623_i2c_write(CMC623_I2C_ADDRESS,0xb4, pData);    //PWM ratio
	cmc623_i2c_write(CMC623_I2C_ADDRESS,0x28, 0x0000);

	return;
}

