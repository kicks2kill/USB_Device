#ifndef USB_DEVICE_H_
#define USB_DEVICE_H_

typedef struct
{
	//the current USB device state
	UsbDeviceState device_state;
	//Current control transfer stage for endpoint0
	UsbControlTransferStage control_transfer_stage;
	//the selected USB config
	uint8_t configuration_value;

	//UsbDeviceOutInBufferPointers
	void const *ptr_out_buffer;
	uint32_t out_data_size;
	void const *ptr_in_buffer;
	uint32_t in_data_size;
} UsbDevice;

#endif /* USB_DEVICE_H_ */
