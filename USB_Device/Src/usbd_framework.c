#include "usbd_framework.h"

void usbd_initialize()
{
	initialize_gpio_pins();
	initialize_core();
	connect();
}
