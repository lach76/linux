#ifndef AML_M8_H
#define AML_M8_H

#include <sound/soc.h>
#include <linux/gpio/consumer.h>
struct wm8750_private_data {
	const char *pinctrl_name;
	struct pinctrl *pin_ctl;
	void *data;
};

#endif
