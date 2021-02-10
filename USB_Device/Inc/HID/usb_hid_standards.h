#ifndef HID_USB_HID_STANDARDS_H_
#define HID_USB_HID_STANDARDS_H_

#include "usb_hid.h"
#include "usb_hid_usage_button.h"
#include "usb_hid_usage_desktop.h"
#define USB_DESCRIPTOR_TYPE_HID				0x21
#define USB_DESCRIPTOR_TYPE_HID_REPORT		0x22
#define USB_HID_COUNTRY_NONE				0

typedef struct {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint16_t	bcdHID;
	uint8_t		bCountryCode;
	uint8_t		bNumDescriptors;
	uint8_t		bDescriptorType0;
	uint16_t	wDescriptorLength0;
} __attribute__((__packed__))UsbHidDescriptor; //__attribute__((__packed__)) used for data alignment


#endif /* HID_USB_HID_STANDARDS_H_ */
