#include <stdint.h>
#include "system_stm32f4xx.h"
#include "stm32f4xx.h"
#include "Helpers/logger.h"

LogLevel system_log_level = LOG_LEVEL_DEBUG;
uint32_t SystemCoreClock = 72000000; // 72 MHz


 static void configure_clock()
{
	 MODIFY_REG(FLASH->ACR,
			 FLASH_ACR_LATENCY,
			 _VAL2FLD(FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_2WS) //bit-shift left by val; <<
			 );

	 //Enable HSE (RESET & CLOCK CONTROL REG)
	 SET_BIT(RCC->CR, RCC_CR_HSEON);

	 //wait until HSE is stable and ready
	 while (!READ_BIT(RCC->CR, RCC_CR_HSERDY));

	 //Configure PLL: source = HSE, SYSCLK = 72MHz
	 MODIFY_REG(RCC->PLLCFGR,
			 RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLQ | RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLP,
			 _VAL2FLD(RCC_PLLCFGR_PLLM, 4) | _VAL2FLD(RCC_PLLCFGR_PLLN, 72) | _VAL2FLD(RCC_PLLCFGR_PLLQ, 3) | _VAL2FLD(RCC_PLLCFGR_PLLSRC, 1)
			 );

	 //Enable PLL module
	 SET_BIT(RCC->CR, RCC_CR_PLLON);

	 //wait until PLL is stable
	 while (!READ_BIT(RCC->CR, RCC_CR_PLLRDY));

	 //switch system clock to PLL
	 MODIFY_REG(RCC->CFGR,
			 RCC_CFGR_SW,
			 _VAL2FLD(RCC_CFGR_SW, RCC_CFGR_SW_PLL)
			 );

	 //configure the PPRE1
	 MODIFY_REG(RCC->CFGR,
			 RCC_CFGR_PPRE1,
			 _VAL2FLD(RCC_CFGR_PPRE1, 4)
			 );

	//wait until PLL is used
	 while( READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

	 CLEAR_BIT(RCC->CR, RCC_CR_HSION);
}

 void configure_mco1()
 {
	 //Configures MCO1: source = PLLCLK, MCO1PRE = 2
	 MODIFY_REG(RCC->CFGR,
			 RCC_CFGR_MCO1 | RCC_CFGR_MCO1PRE,
			 _VAL2FLD(RCC_CFGR_MCO1, 3) | _VAL2FLD(RCC_CFGR_MCO1PRE,4)
	 );

	 //Enable GPIOA (MCO1 is connected to PA8);
	 SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);

	 //Medium Speed
	 MODIFY_REG(GPIOA->OSPEEDR,
			 GPIO_OSPEEDR_OSPEED8,
			 _VAL2FLD(GPIO_OSPEEDR_OSPEED8, 1)
	 );

	 //Configure PA8 to work in alternate function mode
	 MODIFY_REG(GPIOA->MODER,
			 GPIO_MODER_MODER8,
			 _VAL2FLD(GPIO_MODER_MODER8, 2)
	 );
 }

void SystemInit(void)
{
	configure_mco1();
	configure_clock();
}
