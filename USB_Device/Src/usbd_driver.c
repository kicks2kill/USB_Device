#include "usbd_driver.h"
#include "usb_standards.h"

void initialize_gpio_pins()
{
	//Enable clock for GPIOB
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);

	//Sets alternate function 12 for: PB14 (-) and PB15 (+)
	MODIFY_REG(GPIOB->AFR[1],
			GPIO_AFRH_AFSEL14 | GPIO_AFRH_AFSEL15,
			_VAL2FLD(GPIO_AFRH_AFSEL14, 0xC) | _VAL2FLD(GPIO_AFRH_AFSEL15, 0xC) //12 in hex
	);

	//Configure USB pins (GPIOB) to work in alternate function mode
	MODIFY_REG(GPIOB-> MODER,
			GPIO_MODER_MODER14 | GPIO_MODER_MODER15,
			_VAL2FLD(GPIO_MODER_MODER14, 2) | _VAL2FLD(GPIO_MODER_MODER15, 2)
	);

}

void initialize_core()
{
	//Enable the clock for USB core
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_OTGHSEN);

	//Configure USB core to run in device mode, and use embedded full-speed PHY
	MODIFY_REG(USB_OTG_HS -> GUSBCFG,
			USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYSEL | USB_OTG_GUSBCFG_TRDT,
			USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYSEL | _VAL2FLD(USB_OTG_GUSBCFG_TRDT, 0x09) //9 is only supported option for this micro-controller
	);

	//Configure device to run in full speed mode
	MODIFY_REG(USB_OTG_HS_DEVICE->DCFG,
			USB_OTG_DCFG_DSPD,
			_VAL2FLD(USB_OTG_DCFG_DSPD, 0x03)
	);

	//Enable VBUS sensing device
	//SET_BIT(USB_OTG_HS->GCCFG,USB_OTG_GCCFG_VBUSBSEN);
	SET_BIT(USB_OTG_HS->GCCFG, USB_OTG_GCCFG_VBDEN);


	//Unmasks the main USB core interrupts
	SET_BIT(USB_OTG_HS->GINTMSK,
			USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM | USB_OTG_GINTMSK_SOFM |
			USB_OTG_GINTMSK_USBSUSPM | USB_OTG_GINTMSK_WUIM | USB_OTG_GINTMSK_IEPINT |
			USB_OTG_GINTSTS_OEPINT | USB_OTG_GINTMSK_RXFLVLM
	);

	//Clear all pending core interrupts
	WRITE_REG(USB_OTG_HS-> GINTSTS, 0xFFFFFFFF);

	//Unmasks USB global interrupt
	SET_BIT(USB_OTG_HS->GAHBCFG, USB_OTG_GAHBCFG_GINT);
}

void connect()
{
	//Power the transceivers on
	SET_BIT(USB_OTG_HS->GCCFG, USB_OTG_GCCFG_PWRDWN);

	//Connect the device to bus
	CLEAR_BIT(USB_OTG_HS_DEVICE->DCTL, USB_OTG_DCTL_SDIS); //soft-connect
}


void disconnect()
{
	//Disconnect device from bus
	SET_BIT(USB_OTG_HS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);

	//Power transceivers off
	CLEAR_BIT(USB_OTG_HS->GCCFG, USB_OTG_GCCFG_PWRDWN);
}

static void configure_endpoint0(uint16_t endpoint_size)
{
	//Unmask all interrupts of IN and OUT endpoint0
	SET_BIT(USB_OTG_HS_DEVICE->DAINTMSK, 1 << 0 | 1 << 16);

	//Configure the maximum packet size, activate endpoint and NAK the endpoint
	MODIFY_REG(IN_ENDPOINT(0)->DIEPCTL,
			USB_OTG_DIEPCTL_MPSIZ,
			USB_OTG_DIEPCTL_USBAEP | _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, endpoint_size) | USB_OTG_DIEPCTL_SNAK
	);

	//Clear NAK and enable endpoint data transmission
	SET_BIT(OUT_ENDPOINT(0)->DOEPCTL,
			USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK
			);
}

static void configure_in_endpoint(uint8_t endpoint_number, UsbEndpointType endpoint_type, uint16_t endpoint_size)
{
	//Unmask all interrupts of the targeted IN endpoint
	SET_BIT(USB_OTG_HS_DEVICE->DAINTMSK, 1 << endpoint_number);

	//Activate endpoint and set endpoint handshake to NAK.
	MODIFY_REG(IN_ENDPOINT(endpoint_number)->DIEPCTL,
		USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_EPTYP,
		USB_OTG_DIEPCTL_USBAEP | _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, endpoint_size) | USB_OTG_DIEPCTL_SNAK |
		_VAL2FLD(USB_OTG_DIEPCTL_EPTYP, endpoint_type) | USB_OTG_DIEPCTL_SD0PID_SEVNFRM
	);
}

static void deconfigure_endpoint(uint8_t endpoint_number)
{
	USB_OTG_INEndpointTypeDef *in_endpoint = IN_ENDPOINT(endpoint_number);
	USB_OTG_OUTEndpointTypeDef *out_endpoint = OUT_ENDPOINT(endpoint_number);

	//masks all interrupts of targeted in and out endpoints
	CLEAR_BIT(USB_OTG_HS_DEVICE->DAINTMSK,
			(1 << endpoint_number) | (1 << 16 << endpoint_number)
	);

	//Clears all interrupts of the endpoint
	SET_BIT(in_endpoint->DIEPINT, 0x29FF);
	SET_BIT(out_endpoint->DOEPINT, 0x71FF);

	//disable endpoints if possible
	if(in_endpoint->DIEPCTL & USB_OTG_DIEPCTL_EPENA)
	{
		SET_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_EPDIS);
	}

	//deactivate endpoint
	CLEAR_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_USBAEP);

	if(endpoint_number != 0) //0 endpoint must be active/enabled.
	{
		if(out_endpoint->DOEPCTL & USB_OTG_DOEPCTL_EPENA)
		{
			//Disable endpoint transmission
			SET_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_EPDIS);
		}

		//deactivate the endpoint
		CLEAR_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_USBAEP);
	}
}

static void usbrst_handler()
{

	for(uint8_t i = 0; i <= ENDPOINT_COUNT; i++)
	{

	}
}

void gintsts_handler()
{
	volatile uint32_t gintsts = USB_OTG_HS_GLOBAL->GINTSTS;

	if(gintsts & USB_OTG_GINTSTS_USBRST)
	{
		//Clear interrupt to avoid global interrupt persistence
		SET_BIT(USB_OTG_HS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_USBRST);
	}
	else if (gintsts & USB_OTG_GINTSTS_ENUMDNE)
	{

	}
	else if (gintsts & USB_OTG_GINTSTS_RXFLVL)
	{

	}
	else if (gintsts & USB_OTG_GINTSTS_IEPINT)
	{

	}
	else if (gintsts & USB_OTG_GINTSTS_OEPINT)
	{

	}
}
