/*
 * AOA read/write library
 */

#ifndef __LIBAOARW_H
#define __LIBAOARW_H

typedef enum libaoarw_result {
	LIBAOARW_RES_OK=0,
	LIBAOARW_RES_NODEV,
	LIBAOARW_RES_SYSERR,
	LIBAOARW_RES_TIMEOUT
} LIBAOARW_RESULT;

LIBAOARW_RESULT libaoarw_initialize(
	const char *manufacturer,
	const char *model,
	const char *description,
	const char *version,
	const char *uri,
	const char *serial);

void libaoarw_deinitialize();

LIBAOARW_RESULT libaoarw_read(
	unsigned char *buffer,
	int bufsize,
	int *readsize,
	unsigned int timeout);

LIBAOARW_RESULT libaoarw_write(
	unsigned char *buffer,
	int bufsize,
	int *writesize,
	unsigned int timeout);

#endif

/* EOF */
