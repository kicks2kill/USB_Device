#include <stdint.h>
#include "stm32f4xx.h"
#include "Helpers/logger.h"
#include "usbd_framework.h"

int main(void)
{
	log_info("Program entrypoint");
	usbd_initialize();
	for(;;);
}
