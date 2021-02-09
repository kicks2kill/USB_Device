#ifndef USBD_DRIVER_H_
#define USBD_DRIVER_H_
#include "stm32f4xx.h"
#include "usb_standards.h"

#define USB_OTG_HS_GLOBAL ((USB_OTG_GlobalTypeDef *)(USB_OTG_HS_PERIPH_BASE + USB_OTG_GLOBAL_BASE))
#define USB_OTG_HS_DEVICE ((USB_OTG_DeviceTypeDef *)(USB_OTG_HS_PERIPH_BASE + USB_OTG_DEVICE_BASE))
#define USB_OTG_HS_PCGCCTL ((uint32_t *)(USB_OTG_HS_PERIPH_BASE + USB_OTG_PCGCCTL_BASE))

//return structure containing the regs of a specific IN endpoint
inline static USB_OTG_INEndpointTypeDef * IN_ENDPOINT(uint8_t endpoint_number)
{
	return (USB_OTG_INEndpointTypeDef *)(USB_OTG_HS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + (endpoint_number * 0x20));
}

//return structure containing the regs of a specific OUT endpoint
inline static USB_OTG_OUTEndpointTypeDef * OUT_ENDPOINT(uint8_t endpoint_number)
{
	return (USB_OTG_OUTEndpointTypeDef *)(USB_OTG_HS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + (endpoint_number *0x20));
}

inline static volatile uint32_t *FIFO(uint8_t endpoint_number)
{
	return (volatile uint32_t *)(USB_OTG_HS_PERIPH_BASE + USB_OTG_FIFO_BASE + (endpoint_number * 0x1000));
}

typedef struct
{
	void (*initialize_core)();
	void (*initialize_gpio_pins)();
	void (*set_device_address)(uint8_t addr);
	void (*connect)();
	void (*disconnect)();
	void (*flush_rxfifo)();
	void (*flush_txfifo)(uint8_t endpoint_number);
	void (*configure_in_endpoint)(uint8_t endpoint_number, enum UsbEndpointType endpoint_type, uint16_t size );
	void (*read_packet)(void const *buffer, uint16_t size);
	void (*write_packet)(uint8_t endpoint_number, void const *buffer, uint16_t size);
	void (*poll)();
} UsbDriver;

extern const UsbDriver usb_driver;
extern UsbEvents usb_events;
//IN and OUT
#define ENDPOINT_COUNT 6
#endif
