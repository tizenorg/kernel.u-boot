/*
 * Copyright (C) 2011 Samsung Electrnoics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Kyungmin Park <kyungmin.park@samsung.com>
 */

#define MAX8997_PMIC_ADDR	(0xCC >> 1)
#define MAX8997_RTC_ADDR	(0x0C >> 1)
#define MAX8997_MUIC_ADDR	(0x4A >> 1)
#define MAX8997_FG_ADDR		(0x6C >> 1)

enum {
	PWRONR,
	PWRONF,
	PWRON1S,
};

enum {
	CHARGER_NO = 0,
	CHARGER_TA,
	CHARGER_USB,
	CHARGER_TA_500,
	CHARGER_UNKNOWN
};

void pmic_bus_init(int bus_num);
int pmic_probe(void);
int muic_probe(void);
unsigned int pmic_get_irq(int irq);
unsigned int pmic_get_irq_booton(int irq);
void pmic_charger_en(int enable);
int pmic_charger_detbat(void);
