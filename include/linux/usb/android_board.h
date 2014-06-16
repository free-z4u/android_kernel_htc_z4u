
#include <linux/if_ether.h>

struct android_usb_platform_data {
	/* USB device descriptor fields */
	__u16 vendor_id;
	/* Default product ID. */
	__u16 product_id;
	/* To indicate the GPIO num for USB id
	 */
	int	usb_id_pin_gpio;
	char	*serial_number;

	char	*manufacturer_name;
	char	*product_name;

	/* rndis */
	char	*rndisVendorDescr;
	u32	rndisVendorID;
	u8	rndisEthaddr[ETH_ALEN];

	/* ecm */
	char	*ecmVendorDescr;
	u32	ecmVendorID;
	u8	ecmEthaddr[ETH_ALEN];
	/* serial */
	char *fserial_init_string;
	/* ums initial parameters */
	unsigned char diag_init:1;
	unsigned char modem_init:1;
	unsigned char rmnet_init:1;
	unsigned char reserved:5;

	/* number of LUNS */
	int nluns;
	/* bitmap of lun to indicate cdrom disk.
	* NOTE: Only support one cdrom disk
	* and it must be located in last lun */
	int cdrom_lun;
	int (*match)(int product_id, int intrsharing);
	u8			usb_core_id;
	/* hold a performance lock while adb_read a maximum data to keep
	 * adb throughput level
	 */
	int adb_perf_lock_on;
	int mtp_perf_lock_on;
};
int htc_usb_enable_function(char *name, int ebl);
