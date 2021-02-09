#include "usbd_framework.h"
#include "usbd_driver.h"
#include "usb_device.h"
#include "usb_standards.h"
#include "Helpers/logger.h"
#include "usbd_descriptors.h"

static UsbDevice *usbd_handle;

void usbd_initialize(UsbDevice *usb_device)
{
	usbd_handle = usb_device;
	usb_driver.initialize_gpio_pins();
	usb_driver.initialize_core();
	usb_driver.connect();
}

static void process_standard_device_request()
{
	UsbRequest const *request = usbd_handle->ptr_out_buffer; //request will not be manipulated.
	switch(request->bRequest)
	{
	case USB_STANDARD_GET_DESCRIPTOR:;

		const uint8_t descriptor_type = request->wValue >> 8;
		const uint16_t descriptor_length = request->wLength;
		switch(descriptor_type)
		{
		case USB_DESCRIPTOR_TYPE_DEVICE:
			log_info(" - Get Device Descriptor");
			usbd_handle->ptr_in_buffer = &device_descriptor;
			usbd_handle->in_data_size = descriptor_length;

			log_info(" Switching control stage to IN data");
			usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN;
			break;
		}
		break;
	}
}

static void process_request()
{
	UsbRequest const *request = usbd_handle->ptr_out_buffer; //receiving a request, so need out buffer

	//type of request can be standard and need to gather from recipient
	switch(request->bmRequestType & (USB_BM_REQUEST_TYPE_TYPE_MASK | USB_BM_REQUEST_TYPE_RECIPIENT_MASK))
	{
		case USB_BM_REQUEST_TYPE_TYPE_STANDARD | USB_BM_REQUEST_TYPE_RECIPIENT_DEVICE:
			//process_standard_device_request();
			break;
	}
}

static void process_control_transfer_stage()
{
	switch(usbd_handle->control_transfer_stage)
	{
		case USB_CONTROL_STAGE_SETUP:
			break;
		case USB_CONTROL_STAGE_DATA_IN:
			log_info("Processing IN-DATA stage");
			uint8_t data_size = device_descriptor.bMaxPacketSize0;
			usb_driver.write_packet(0, usbd_handle->ptr_in_buffer, data_size);

			usbd_handle->in_data_size -= data_size;
			usbd_handle->ptr_in_buffer += data_size;

			usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_DATA_IN_IDLE;
			break;
		case USB_CONTROL_STAGE_DATA_IN_IDLE:
			break;
	}
}

static void usb_polled_handler()
{
	process_control_transfer_stage();
}

void usbd_poll()
{
	usb_driver.poll();
}

static void usbrst_handler()
{
	usbd_handle->in_data_size = 0;
	usbd_handle->out_data_size = 0;
	usbd_handle->configuration_value = 0;
	usbd_handle->device_state = USB_DEVICE_STATE_DEFAULT;
	usbd_handle->control_transfer_stage = USB_CONTROL_STAGE_SETUP;
	usb_driver.set_device_address(0);
}


static void setup_data_received_handler(uint8_t endpoint_number, uint16_t byte_count)
{
	usb_driver.read_packet(usbd_handle->ptr_out_buffer, byte_count);

	//print out the received data
	log_debug_array("SETUP data: ", usbd_handle->ptr_out_buffer, byte_count);

	process_request();
}

UsbEvents usb_events = {
		.on_usb_reset_received = &usbrst_handler,
		.on_setup_data_received = &setup_data_received_handler,
		.on_usb_polled = &usb_polled_handler
		//TODO
};
