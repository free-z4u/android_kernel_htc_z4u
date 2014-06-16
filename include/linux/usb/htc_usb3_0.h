
void htc_disable_usb_set(int val);
int htc_disable_usb_get(void);

enum {
	USB_FUNCTION_UMS = 0,
	USB_FUNCTION_ADB = 1,
	USB_FUNCTION_RNDIS,
	USB_FUNCTION_DIAG,
	USB_FUNCTION_SERIAL,
	USB_FUNCTION_PROJECTOR,
	USB_FUNCTION_FSYNC,
	USB_FUNCTION_MTP,
	USB_FUNCTION_MODEM,
	USB_FUNCTION_ECM,
	USB_FUNCTION_ACM,
	USB_FUNCTION_DIAG_MDM,
	USB_FUNCTION_RMNET,
	USB_FUNCTION_PHONET, /* HTC_KER_ADD, ken, enable STE phonet function */
	USB_FUNCTION_STE_MODEM,
	USB_FUNCTION_ACCESSORY,
	USB_FUNCTION_AUTOBOT,
	USB_FUNCTION_RNDIS_IPT = 31,
};

struct usb_pid_table {
	u32 usb_function_flag;
	u32 pid;
};

struct usb_string_node{
	u32 usb_function_flag;
	char *name;
};

struct htc_usb_pid {
	struct usb_pid_table *usb_pid_table_array;
	u32 length;
};


#define PID_ACM 0x0ff4


