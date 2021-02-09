#include "usbd_driver.h"


//These functions in the driver should not be called, except from the framework.

static void initialize_gpio_pins()
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

static void initialize_core()
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

	//Unmasks transfer completed interrupt for all endpoints
	SET_BIT(USB_OTG_HS_DEVICE->DOEPMSK, USB_OTG_DOEPMSK_XFRCM);
	SET_BIT(USB_OTG_HS_DEVICE->DIEPMSK, USB_OTG_DOEPMSK_XFRCM);
}

static void connect()
{
	//Power the transceivers on
	SET_BIT(USB_OTG_HS->GCCFG, USB_OTG_GCCFG_PWRDWN);

	//Connect the device to bus
	CLEAR_BIT(USB_OTG_HS_DEVICE->DCTL, USB_OTG_DCTL_SDIS); //soft-connect
}


static void disconnect()
{
	//Disconnect device from bus
	SET_BIT(USB_OTG_HS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);

	//Power transceivers off
	CLEAR_BIT(USB_OTG_HS->GCCFG, USB_OTG_GCCFG_PWRDWN);
}


static void set_device_address(uint8_t addr)
{
	MODIFY_REG(
			USB_OTG_HS_DEVICE->DCFG,
			USB_OTG_DCFG_DAD,
			_VAL2FLD(USB_OTG_DCFG_DAD, addr)
	);
}

//Pops data from the RxFIFO and stores it in the buffer
//buffer is pointer to the buffer in which the popped data will be stored
static void read_packet(void *buffer, uint16_t size)
{
	//Note: there is only one RxFIFO
	uint32_t *fifo = FIFO(0); //popped data would be lost if not stored

	for(; size >= 4; size -=4, buffer += 4) //shift buffer by 4, otherwise data will be lost each cycle (FIFO structure uses 32-bit words)
	{
		//pops one 32-bit word of data (until there is less than one word remaining)
		uint32_t data = *fifo;
		//store data in buffer, dereference the buffer pointer
		*((uint32_t*)buffer) = data;
	}
	if(size > 0)
	{
		//pops remaining last bytes (less than a word)
		uint32_t data = *fifo;

		for(; size > 0; size--, buffer++, data>>= 8)
		{
			//store data in the buffer with correct alignment
			*((uint8_t*)buffer) = 0xFF & data;
		}
	}
}


static void write_packet(uint8_t endpoint_number, void const *buffer, uint16_t size)
{
	uint32_t *fifo = FIFO(endpoint_number);
	USB_OTG_INEndpointTypeDef *in_endpoint = IN_ENDPOINT(endpoint_number);

	//configure the transmission (1 packet that has size bytes)
	MODIFY_REG(in_endpoint->DIEPTSIZ,
			USB_OTG_DIEPTSIZ_PKTCNT | USB_OTG_DIEPTSIZ_XFRSIZ,
			_VAL2FLD(USB_OTG_DIEPTSIZ_PKTCNT, 1) | _VAL2FLD(USB_OTG_DIEPTSIZ_XFRSIZ, size)
	);

	//enable the transmission after clearing both STALL and NAK of the endpoint
	MODIFY_REG(in_endpoint->DIEPCTL,
			USB_OTG_DIEPCTL_STALL,
			USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA
	);

	//get the size in terms of 32-bit words
	size = (size + 3) / 4;

	for(; size > 0; size--, buffer += 4)
	{
		//pushes data to the TxFIFO
		*fifo = *((uint32_t *)buffer);
	}
}

//Update the start addresses of all FIFOs according to size of each FIFO
static void refresh_fifo_start_addresses()
{
	//First changeable start address begins after the region of RxFIFO
	uint16_t start_addr = _FLD2VAL(USB_OTG_GRXFSIZ_RXFD, USB_OTG_HS->GRXFSIZ) * 4; //must be aligned with a 32-bit location/word

	//Update the start of TxFIFO0
	MODIFY_REG(USB_OTG_HS->DIEPTXF0_HNPTXFSIZ,
			USB_OTG_TX0FSA,
			_VAL2FLD(USB_OTG_TX0FSA, start_addr)
	);

	//Next start address is after where the last TxFIFO ends
	start_addr += _FLD2VAL(USB_OTG_TX0FD, USB_OTG_HS->DIEPTXF0_HNPTXFSIZ) * 4; //must be aligned with a 32-bit location/word

	//update the start address of the remaining TxFIFO
	for (uint8_t txfifo_number = 0; txfifo_number < ENDPOINT_COUNT -1; txfifo_number++)
	{
		MODIFY_REG(USB_OTG_HS->DIEPTXF[txfifo_number],
				USB_OTG_NPTXFSA,
				_VAL2FLD(USB_OTG_NPTXFSA, start_addr)
		);

		start_addr += _FLD2VAL(USB_OTG_NPTXFD, USB_OTG_HS->DIEPTXF[txfifo_number]) * 4;
	}
}


//Configure RxFIFO of all OUT endpoints, size of largest OUT endpoints in bytes.
//Shared between all OUT endpoints
static void configure_rxfifo_size(uint16_t size)
{
	//Consider the space required to save status packets in RxFIFO and get size in term of 32-bit words.
	size = 10 + (2 * ((size / 4) + 1));

	MODIFY_REG(USB_OTG_HS->GRXFSIZ,
		USB_OTG_GRXFSIZ_RXFD,
		_VAL2FLD(USB_OTG_GRXFSIZ_RXFD, size)
	);

	refresh_fifo_start_addresses();
}

//Configure TxFIFO of all IN endpoints, the size of IN endpoint in bytes
//Any change on any FIFO will update the registers of all TxFIFOs to adapt the start offsets
static void configure_txfifo_size(uint8_t endpoint_number, uint16_t size)
{
	//Get FIFO size in term of 32-bit words
	size = (size + 3) / 4;

	//configure the depth of the TxFIFO
	if(endpoint_number == 0)
	{
		MODIFY_REG(USB_OTG_HS->DIEPTXF0_HNPTXFSIZ,
				USB_OTG_TX0FD,
				_VAL2FLD(USB_OTG_TX0FD,size)
		);
	}
	else
	{
		MODIFY_REG(USB_OTG_HS->DIEPTXF[endpoint_number - 1],
				USB_OTG_NPTXFD,
				_VAL2FLD(USB_OTG_NPTXFD, size)
		);

	}
	refresh_fifo_start_addresses();
}


//Flush the RxFIFO of all OUT endpoints
static void flush_rxfifo()
{
	SET_BIT(USB_OTG_HS->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);
}


//flush the TxFIFO of an IN endpoint
static void flush_txfifo(uint8_t endpoint_number)
{
	//Sets the number of TxFIFO to be flushed and then triggers the flush
	MODIFY_REG(USB_OTG_HS->GRSTCTL,
			USB_OTG_GRSTCTL_TXFNUM,
			_VAL2FLD(USB_OTG_GRSTCTL_TXFNUM, endpoint_number) | USB_OTG_GRSTCTL_TXFFLSH
	);
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

	//Note: 64 bytes is the maximum packet size for full speed USB devices
	configure_rxfifo_size(64);
	configure_txfifo_size(0, endpoint_size);
}

static void configure_in_endpoint(uint8_t endpoint_number, UsbEndpointType endpoint_type, uint16_t endpoint_size)
{
	//Unmask all interrupts of the targeted IN endpoint
	SET_BIT(USB_OTG_HS_DEVICE->DAINTMSK, 1 << endpoint_number);

	//Activate endpoint and set endpoint handshake to NAK.
	MODIFY_REG(IN_ENDPOINT(endpoint_number)->DIEPCTL,
		USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_EPTYP,
		USB_OTG_DIEPCTL_USBAEP | _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, endpoint_size) | USB_OTG_DIEPCTL_SNAK |
		_VAL2FLD(USB_OTG_DIEPCTL_EPTYP, endpoint_type) | _VAL2FLD(USB_OTG_DIEPCTL_TXFNUM, endpoint_number) | USB_OTG_DIEPCTL_SD0PID_SEVNFRM
	);

	configure_txfifo_size(endpoint_number, endpoint_size);
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

	//Flush the FIFOs
	flush_txfifo(endpoint_number);
	flush_rxfifo();
}




static void usbrst_handler()
{
	log_info("USB reset singal was detected");
	for(uint8_t i = 0; i <= ENDPOINT_COUNT; i++)
	{
		deconfigure_endpoint(i);
	}

	usb_events.on_usb_reset_received();
}

static void enumdne_handler()
{
	log_info("USB device speed enumeration done");
	configure_endpoint0(8);
}

static void rxflvl_handler()
{
	//Pop the status information word from RxFIFO
	uint32_t receive_status = USB_OTG_HS_GLOBAL->GRXSTSP;

	//endpoint that receives the data
	uint8_t endpoint_number = _FLD2VAL(USB_OTG_GRXSTSP_EPNUM, receive_status);
	//count of bytes in received packet
	uint16_t bcnt = _FLD2VAL(USB_OTG_GRXSTSP_BCNT, receive_status);
	//status of received packet
	uint16_t pktsts = _FLD2VAL(USB_OTG_GRXSTSP_PKTSTS, receive_status);

	switch(pktsts)
	{
	case 0x06: //SETUP packet (includes data)
		usb_events.on_setup_data_received(endpoint_number, bcnt);
		break;
	case 0x02: //OUT packet
		break;
	case 0x04: //SETUP stage has completed
		//re-enable the transmission on the endpoint
		SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPCTL,
				USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA
		);
		break;
	case 0x03: //OUT transfer has completed
		SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPCTL,
				USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA
		);
		break;
	}

}

static void gintsts_handler()
{
	volatile uint32_t gintsts = USB_OTG_HS_GLOBAL->GINTSTS;

	if(gintsts & USB_OTG_GINTSTS_USBRST)
	{
		usbrst_handler();
		//Clear interrupt to avoid global interrupt persistence
		SET_BIT(USB_OTG_HS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_USBRST);
	}
	else if (gintsts & USB_OTG_GINTSTS_ENUMDNE)
	{
		enumdne_handler();
		//Clear interrupt after handling
		SET_BIT(USB_OTG_HS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_ENUMDNE);
	}
	else if (gintsts & USB_OTG_GINTSTS_RXFLVL) //interrupt occurs when RxFIFO data is not empty
	{
		rxflvl_handler();
		//clear interrupt after handling
		SET_BIT(USB_OTG_HS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_RXFLVL);

	}
	else if (gintsts & USB_OTG_GINTSTS_IEPINT)
	{

	}
	else if (gintsts & USB_OTG_GINTSTS_OEPINT)
	{

	}
}

//function pointers
const UsbDriver usb_driver = {
		.initialize_core = &initialize_core,
		.initialize_gpio_pins = &initialize_gpio_pins,
		.set_device_address = &set_device_address,
		.connect = &connect,
		.disconnect = &disconnect,
		.flush_rxfifo = &flush_rxfifo,
		.flush_txfifo = &flush_txfifo,
		.configure_in_endpoint = &configure_in_endpoint,
		.read_packet = &read_packet,
		.write_packet = &write_packet,
		.poll = &gintsts_handler

};
