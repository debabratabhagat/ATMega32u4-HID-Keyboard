/*
usb.c
USB Controller initialization, device setup, 
*/

#define F_CPU 16000000

#include "usb_init.h"
#include "avr/io.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

/*  Device Descriptor - The top level descriptor when enumerating a USB device`
	Specification: USB 2.0 (April 27, 2000) Chapter 9 Table 9-5

*/
static const uint8_t PROGMEM device_descriptor[] = { // Stored in PROGMEM (Program Memory) Flash, freeing up some SRAM where variables are usually stored
	18, // bLength - The total size of the descriptor 
	1, // bDescriptorType - The type of descriptor - 1 is device
	0x00, 0x02, // bcdUSB - The USB protcol supported - Refer to USB 2.0 Chapter 9.6.1
	0, // bDeviceClass - The Device Class, 0 indicating that the HID interface will specify it
	0, // bDeviceSubClass - 0, HID will specify
	0, // bDeviceProtocol - No class specific protocols on a device level, HID interface will specify
	32, // bMaxPacketSize0 - 32 byte packet size; control endpoint was configured in UECFG1X to be 32 bytes
	(idVendor & 255), ((idVendor >> 8) & 255), // idVendor - Vendor ID specified by USB-IF (To fit the 2 bytes, the ID is split into least significant and most significant byte)
	(idProduct & 255), ((idProduct >> 8) & 255), // idProduct - The Product ID specified by USB-IF - Split in the same way as idVendor
	0, // iManufacturer - The String Descriptor that has the manufacturer name - Specified by USB 2.0 Table 9-8
	0, // iProduct - The String Descriptor that has the product name - Specified by USB 2.0 Table 9-8
	0, // iSerialNumber - The String Descriptor that has the serial number of the product - Specified by USB 2.0 Table 9-8
	1 // bNumConfigurations - The number of configurations of the device, most devices only have one
};

/*  HID Descriptor - The descriptor that gives information about the HID device
	Specification: Device Class Definition for Human Interface Devices (HID) 6/27/2001 Appendix B - Keyboard Protocol Specification

*/
static const uint8_t PROGMEM keyboard_HID_descriptor[] = {
	
};

/*  Configuration Descriptor - The descriptor that gives information about the device conifguration(s) and how to select them
	Specification: USB 2.0 (April 27, 2000) Chapter 9 Table 9-10

*/
static const uint8_t PROGMEM configuration_descriptor[] = {
	9, // bLength
	2, // bDescriptorType - 2 is device
	TOTAL_DEVICE_LENGTH, TOTAL_DEVICE_LENGTH, // wTotalLength - The total length of the descriptor tree
	1, // bNumInterfaces - 1 Interface
	1, // bConfigurationValue 
	0, // iConfiguration - We have no string descriptors
	0x80, // bmAttributes - Set the device power source 
	50, // bMaxPower - 50 x 2mA units = 100mA max power consumption
	// Refer to Table 9-10 for the descriptor structure - Configuration Descriptors have interface descriptors, interface descriptors have endpoint descriptors along with a special HID descriptor
	9, // bLength
	4, // bDescriptorType - 4 is interface
	0, // bInterfaceNumber - This is the 0th and only interface
	0, // bAlternateSetting - There are no alternate settings
	1, // bNumEndpoints - This interface only uses one endpoint
	0x03, // bInterfaceClass - 0x03 (specified by USB-IF) is the interface class code for HID
	0x01, // bInterfaceSubClass - 1 (specified by USB-IF) is the constant for the boot subclass - this keyboard can communicate with the BIOS, but is limited to 6KRO, as are most keyboards
	0x01, // bInterfaceProtocol - 0x01 (specified by USB-IF) is the protcol code for keyboards
	0, // iInterface - There are no string descriptors for this
	
};

int usb_init() {
	
	cli(); // Global Interrupt Disable
	
	UHWCON |= (1 << UVREGE); // Enable USB Pads Regulator
	
	PLLCSR |= (1 << PINDIV); // Configure to use 16mHz oscillator

	PLLCSR |= (1 << PLLE); // Enable PLL

	while(!(PLLCSR & (1 << PLOCK))); // Wait for PLL Lock to be achieved

	USBCON |= (1 << USBE) | (1 << OTGPADE); // Enable USB Controller and USB power pads
	USBCON &= ~(1 << FRZCLK); // Unfreeze the clock

	UDCON = (1 << LSM); // High Speed Mode (Full 12mbps)

	USBCON &= ~(1 << DETACH); // Connect
	UDIEN |= (1 << EORSTE) | (1 << SOFE); // Re-enable the EORSTE (End Of Reset) Interrupt so we know when we can configure the control endpoint
	
	sei(); // Global Interrupt Enable

	return 0;
}

bool get_usb_config_status() {
	return usb_config_status;
}

ISR(USB_GEN_vect) {
	if(UDINT & (1 << EORSTI)) {// If end of reset interrupt	
		UDINT = 0;

		// Configure Control Endpoint
		UENUM = 0; // Select Endpoint 0, the default control endpoint
		UECONX = (1 << EPEN); // Enable the Endpoint
		UECFG0X = 0; // Control Endpoint, OUT direction for control endpoint
		UECFG1X |= 0x32; // 64 byte endpoint, 1 bank, allocate the memory
		
		if(!(UESTA0X & (1 << CFGOK))) { // Check if endpoint configuration was successful
			return;	
		}
			
		UERST = 1; // Reset Endpoint
		UERST = 0;
		
		UEIENX = (1 << RXSTPE); // Re-enable the RXSPTE (Receive Setup Packet) Interrupt
	
	}
}

ISR(USB_COM_vect) {
	UENUM = 0;
	if(UEINTX & (1 << RXSTPI)) {
		DDRC = 0xFF;
		PORTC = 0xFF; // LED indicator for setup

		uint8_t bmRequestType = UEDATX; // UEDATX is FIFO; see table in README
		uint8_t bRequest = UEDATX;
		uint16_t wValue = UEDATX | (UEDATX << 8); 
		uint16_t wIndex = UEDATX | (UEDATX << 8);
		uint16_t wLength = UEDATX | (UEDATX << 8);

		UEINTX &= ~((1 << RXSTPI) | (1 << RXOUTI) | (TXINI)); // Handshake the Interrupts, do this after recording the packet because it also clears the endpoint banks
		
		if(bReqest == GET_DESCRIPTOR) {
		}
	}
}
