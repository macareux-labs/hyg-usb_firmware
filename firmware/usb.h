#ifndef USB_H
#define USB_H

#define PHYS_ADDR(VIRTUAL_ADDR)  (unsigned int)(VIRTUAL_ADDR)

#define EP0_BUFLEN 8
#define EP1_BUFLEN 8

#define USB_PID_SETUP                   0x0d

#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_GET_DEVICE_DESCRIPTOR       0x01
#define USB_GET_CONFIG_DESCRIPTOR       0x02
#define USB_GET_STRING_DESCRIPTOR       0x03

#define USB_REQ_GET_STATUS              0x00

#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_SET_CONFIGURATION       0x09

void reset_usb (  ) ;
void usb_interrupt_handler (  ) ;
bool usb_out_endpoint_busy ( uint8_t ep ) ;
bool usb_in_endpoint_busy ( uint8_t ep ) ;
void usb_arm_in_transfert ( uint8_t ep, uint8_t cnt, void *ptr ) ;
void usb_arm_out_transfert (  ) ;
void *usb_get_out_buffer ( uint8_t ep ) ;
#endif
