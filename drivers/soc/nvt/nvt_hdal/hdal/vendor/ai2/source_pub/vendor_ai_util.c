/**
	@brief Source file of utility functions of vendor net postprocessing sample.

	@file vendor_ai_post_util.c

	@ingroup vendor_ai

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "hd_type.h"
#include "vendor_ai_util.h"
#include "vendor_ai_net/nn_net.h"

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

UINT8 *vendor_ais_getbuffrom(UINT8 **buftop, UINT32 nbyte)
{
	UINT8 *buf = *buftop;
	*buftop += ALIGN_CEIL_4(nbyte);
	return buf;
}

HD_RESULT vendor_ais_readbin(UINT32 addr, UINT32 size, const CHAR *filename)
{
	FILE *fd;
	HD_RESULT ret = HD_OK;

	fd = fopen(filename, "rb");

	if (!fd) {
		DBG_ERR("cannot read %s\r\n", filename);
		return HD_ERR_NG;
	}

	DBG_IND("open %s ok\r\n", filename);

	if (fread((VOID *)addr, 1, size, fd) != size) {
		DBG_ERR("read size < %ld\r\n", size);
		ret = HD_ERR_NG;
	}

	if (fd) {
		fclose(fd);
	}

	return ret;
}

HD_RESULT vendor_ais_writebin(UINT32 addr, UINT32 size, const CHAR *filename)
{
	FILE *fd;
	HD_RESULT ret = HD_OK;

	fd = fopen(filename, "wb");

	if (!fd) {
		DBG_ERR("cannot write %s\r\n", filename);
		return HD_ERR_NG;
	}

	DBG_IND("open %s ok\r\n", filename);

	if (fwrite((VOID *)addr, 1, size, fd) != size) {
		DBG_ERR("wrote size < %ld\r\n", size);
		ret = HD_ERR_NG;
	}

	if (fd) {
		fclose(fd);
	}

	return ret;
}

INT32 vendor_ais_loadbin(UINT32 addr, const CHAR *filename)
{
	FILE *fd;
	INT32 size = 0;

	fd = fopen(filename, "rb");

	if (NULL == fd) {
		DBG_ERR("cannot read %s\r\n", filename);
		return -1;
	}

	DBG_IND("open %s ok\r\n", filename);

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	
	if (size < 0) {
		DBG_ERR("getting %s size failed\r\n", filename);
	} else if ((INT32)fread((VOID *)addr, 1, size, fd) != size) {
		DBG_ERR("read size < %ld\r\n", size);
		size = -1;
	}

	if (fd) {
		fclose(fd);
	}

	return size;
}

HD_RESULT vendor_ais_readtxt(UINT32 addr, UINT32 line_len, UINT32 line_num, const CHAR *filename)
{
	FILE *fd;
	CHAR *p_line = (CHAR *)addr;
	UINT32 i;

	fd = fopen(filename, "r");

	if (!fd) {
		DBG_ERR("cannot read %s\r\n", filename);
		return HD_ERR_NG;
	}

	DBG_IND("open %s ok\r\n", filename);

	for (i = 0; i < line_num; i++) {
		fgets(p_line, line_len, fd);
		p_line[strlen(p_line) - 1] = '\0'; // remove newline character
		p_line += line_len;
	}

	if (fd) {
		fclose(fd);
	}

	return HD_OK;
}
