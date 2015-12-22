/*
 * AOA read/write library
 */

#include <unistd.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "libaoarw.h"

#define AOA_VID     0x18D1
#define AOA_PID     0x2D00
#define AOA_ADB_PID 0x2D01

#define AOA_SEQ_MANUFACTURE 0
#define AOA_SEQ_MODEL       1
#define AOA_SEQ_DESCRIPTION 2
#define AOA_SEQ_VERSION     3
#define AOA_SEQ_URI         4
#define AOA_SEQ_SERIAL      5

#define ACCESSORY_GET_PROTOCOL 51
#define ACCESSORY_SEND_STRING  52
#define ACCESSORY_START        53

#define ACCESSORY_CHAR_LENGTH 1000

static char g_manufacturer[ACCESSORY_CHAR_LENGTH];
static char g_model[ACCESSORY_CHAR_LENGTH];
static char g_description[ACCESSORY_CHAR_LENGTH];
static char g_version[ACCESSORY_CHAR_LENGTH];
static char g_uri[ACCESSORY_CHAR_LENGTH];
static char g_serial[ACCESSORY_CHAR_LENGTH];
static struct libusb_transfer      *g_transfer;
static struct libusb_context       *g_context;
static struct libusb_device_handle *g_handle;
static int                         g_inEP;
static int                         g_outEP;

static int __connect();
static void __disconnect();
static int __getProtocol(struct libusb_device_handle *handle);
static int __sendString(struct libusb_device_handle *handle, int index,
	const char *str);
static struct libusb_device_handle* __openAccessory();
static struct libusb_device_handle* __openAvailableDevice();
static int __findEndPoint(struct libusb_device_handle *handle);

LIBAOARW_RESULT libaoarw_initialize(
	const char *manufacturer,
	const char *model,
	const char *description,
	const char *version,
	const char *uri,
	const char *serial)
{
	if (manufacturer != 0) {
		strncpy(g_manufacturer, manufacturer, ACCESSORY_CHAR_LENGTH-1);
	}
	if (model != 0) {
		strncpy(g_model, model, ACCESSORY_CHAR_LENGTH-1);
	}
	if (description != 0) {
		strncpy(g_description, description, ACCESSORY_CHAR_LENGTH-1);
	}
	if (version != 0) {
		strncpy(g_version, version, ACCESSORY_CHAR_LENGTH-1);
	}
	if (uri != 0) {
		strncpy(g_uri, uri, ACCESSORY_CHAR_LENGTH-1);
	}
	if (serial != 0) {
		strncpy(g_serial, serial, ACCESSORY_CHAR_LENGTH-1);
	}
	
	g_transfer = libusb_alloc_transfer(0);

	if(libusb_init(&g_context) != 0){
		return LIBAOARW_RES_SYSERR;
	}

	if (__connect() != 0) {
		return LIBAOARW_RES_SYSERR;
	}

	return LIBAOARW_RES_OK;
}

void libaoarw_deinitialize()
{
	__disconnect();
}

LIBAOARW_RESULT libaoarw_read(
	unsigned char *buffer,
	int bufsize,
	int *readsize,
	unsigned int timeout)
{
	int result=0;
	LIBAOARW_RESULT ret=LIBAOARW_RES_OK;
	
	if (g_handle == NULL) {
		return LIBAOARW_RES_NODEV;
	}

	result = libusb_bulk_transfer(g_handle, g_inEP, buffer, bufsize,
		readsize, timeout);
	switch (result) {
	case 0:
		break;
	case LIBUSB_ERROR_TIMEOUT:
		ret = LIBAOARW_RES_TIMEOUT;
		break;
	default:
		ret = LIBAOARW_RES_SYSERR;
		break;
	}

	return ret;
}

LIBAOARW_RESULT libaoarw_write(
	unsigned char *buffer,
	int bufsize,
	int *writesize,
	unsigned int timeout)
{
	int result=0;
	LIBAOARW_RESULT ret=LIBAOARW_RES_OK;
	
	if (g_handle == NULL) {
		return LIBAOARW_RES_NODEV;
	}
	
	result = libusb_bulk_transfer(g_handle, g_outEP, buffer, bufsize,
		writesize, timeout);
	switch (result) {
	case 0:
		break;
	case LIBUSB_ERROR_TIMEOUT:
		ret = LIBAOARW_RES_TIMEOUT;
		break;
	default:
		ret = LIBAOARW_RES_SYSERR;
		break;
	}

	return ret;
}

static int __connect()
{
	struct libusb_device_handle *handle = 0;

	handle = __openAccessory();
	if (handle != 0) {
		__findEndPoint(handle);
		libusb_claim_interface(handle, 0);
		g_handle = handle;
		return 0;
	}

	handle = __openAvailableDevice();
	if (handle == NULL) {
		return -1;
	}

	libusb_claim_interface(handle, 0);
	usleep(1000);

	__sendString(handle, AOA_SEQ_MANUFACTURE, (char*)g_manufacturer);
	__sendString(handle, AOA_SEQ_MODEL, (char*)g_model);
	__sendString(handle, AOA_SEQ_DESCRIPTION, (char*)g_description);
	__sendString(handle, AOA_SEQ_VERSION, (char*)g_version);
	__sendString(handle, AOA_SEQ_URI, (char*)g_uri);
	__sendString(handle, AOA_SEQ_SERIAL, (char*)g_serial);

	int res = libusb_control_transfer(handle, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
		ACCESSORY_START, 0, 0, NULL, 0, 0);
	if (res < 0) {
		libusb_close(handle);
		return -1;
	}

	libusb_close(handle);

	sleep(3);

	handle = __openAccessory();
	if (handle != NULL) {
		__findEndPoint(handle);
		libusb_claim_interface(handle, 0);
		g_handle = handle;
		return 0;
	}
	
	return -1;
}

static void __disconnect()
{
	if (g_handle != 0) {
		libusb_release_interface(g_handle, 0);
		libusb_close(g_handle);
		g_handle = 0;
	}
	
	if (g_context != 0) {
		libusb_exit(g_context);
		g_context = 0;
	}
}

static int __getProtocol(struct libusb_device_handle *handle)
{
	unsigned short protocol;
	unsigned char buf[2];
	int res;

	res = libusb_control_transfer(handle,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
		ACCESSORY_GET_PROTOCOL, 0, 0, buf, 2, 0);
	if(res < 0){
		return -1;
	}
	protocol = buf[1] << 8 | buf[0];
	return((int)protocol);
}

static int __sendString(
	struct libusb_device_handle *handle,
	int index,
	const char *str)
{
	return libusb_control_transfer(handle,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
		ACCESSORY_SEND_STRING, 0, index, (unsigned char*)str,
		strlen(str) + 1, 0);
}

static struct libusb_device_handle* __openAccessory()
{
	int i;
	struct libusb_device_handle *handle = 0;
	struct libusb_device **devs;
	struct libusb_device_descriptor desc;
	ssize_t devcount = libusb_get_device_list(g_context, &devs);
	
	if (devcount <= 0) {
		return 0;
	}

	for (i=0; i<devcount; i++) {
		libusb_get_device_descriptor(devs[i], &desc);
		if (desc.idVendor == AOA_VID
			&& (desc.idProduct == AOA_PID || desc.idProduct == AOA_ADB_PID)) {
			int ret = libusb_open(devs[i], &handle);
			break;
		}
	}

	libusb_free_device_list(devs, 0);

	return handle;
}

static struct libusb_device_handle* __openAvailableDevice()
{
	int i;
	struct libusb_device_handle *handle = 0;
	struct libusb_device **devs;
	struct libusb_device_descriptor desc;
	ssize_t devcount = libusb_get_device_list(g_context, &devs);
	
	if (devcount <= 0) {
		return 0;
	}

	for (i=0; i<devcount; i++) {
		libusb_get_device_descriptor(devs[i], &desc);
		libusb_open(devs[i], &handle);
		if (handle != 0) {
			libusb_claim_interface(handle, 0);

			int res = __getProtocol(handle);
			if (res >= 1) {
				break;
			}
			
			libusb_release_interface(handle, 0);
			libusb_close(handle);
			
			handle = 0;
		}
	}

	libusb_free_device_list(devs, 0);

	return handle;
}

static int __findEndPoint(struct libusb_device_handle *handle)
{
	int i;
	struct libusb_device *dev;
	int inEP = 0;
	int outEP = 0;
	struct libusb_config_descriptor *config;
	
	dev = libusb_get_device(handle);
	libusb_get_config_descriptor(dev, 0, &config);
	
	if (config == NULL) {
		return -1;
	}
	
	if (config->bNumInterfaces == 0) {
		return -1;
	}
	
	const struct libusb_interface *cif = &config->interface[0];
	if (cif == NULL) {
		return -1;
	}
	
	struct libusb_interface_descriptor ifd = cif->altsetting[0];
	for (i=0; i<ifd.bNumEndpoints; i++) {
		struct libusb_endpoint_descriptor epd = ifd.endpoint[i];
		if (epd.bmAttributes == 2) {
			if (epd.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) {
				if (inEP == 0) {
					inEP = epd.bEndpointAddress;
				}
			}else{
				if (outEP == 0) {
					outEP = epd.bEndpointAddress;
				}
			}
		}
	}
	
	if (outEP == 0 || inEP == 0) {
		return -1;
	}else{
		g_inEP = inEP;
		g_outEP = outEP;
		return 0;
	}
}

/* EOF */
