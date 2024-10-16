#include "sitronix_ts.h"
#include "sitronix_ts_upgrade_fw_bin.h"
#define TFW_CHECK_OFFSET 0x83
#define TFW_CHECK_LENGTH 0x20

unsigned char fw_check[ST_FLASH_PAGE_SIZE];
unsigned char dump_buf[ST_FLASH_SIZE] = SITRONIX_DUMP;

int st_check_cfg(const char *data, int *cfgSize)
{
	if (data[0x00] != 'S' || data[0x01] != 'T' || data[0x02] != 'F' ||
	    data[0x03] != 'W') {
		STX_ERROR("check STFW fail");
		return -1;
	}

#ifdef ST_IC_A8008
	*cfgSize = data[0x08] + 2;
#endif

#ifdef ST_IC_A8010
	*cfgSize = data[0x08] * 0x100 + data[0x09] + 2;
#endif

#ifdef ST_IC_A8015
	*cfgSize = data[0x09] * 0x100 + data[0x0A] + 2;
#endif

	STX_INFO("cfgSize = 0x%X", *cfgSize);

	return 0;
}

int st_check_fw(const char *data, int *fwOff, int *fwSize, int *fwInfoOff,
		int *cfgOff)
{
	int off = 0;
	int devParamTabSize;
	int progSizeH;
	int progSizeL;

	off += 131 + 2 + 4;
	devParamTabSize = data[off];
	off += 1 + devParamTabSize + 4; //4:TFW1

	progSizeH = data[off];
	progSizeL = data[off + 1];

	off += 2 + ((progSizeH)*0x100) + progSizeL;

	*fwOff = 0;
	*cfgOff = CT_CFG_OFF;

	if (data[off] == 0x54 && data[off + 1] == 0x46 &&
	    data[off + 2] == 0x49 && data[off + 3] == 0x32) {
		*fwInfoOff = off + 4;
		*fwSize = 4 + data[off + 4] + off + 1 +
			  4; //tag(4) + TableSize + off + TableSizeOff + CksLen
		STX_INFO("fwOff = 0x%X", *fwOff);
		STX_INFO("fwSize = 0x%X", *fwSize);
		STX_INFO("fwInfoOff = 0x%X", *fwInfoOff);
		STX_INFO("cfgOff = 0x%X", *cfgOff);
		return 0;
	}

	STX_INFO("can't find TOUCH_FW_INFO offset[%d]:%x %x %x %x", off,
		 data[off], data[off + 1], data[off + 2], data[off + 3]);
	return -1;
}

void st_set_cfg(const char *data, int cfgOff, int cfgSize)
{
	int size;
	size = cfgOff + cfgSize;

	if (size <= ST_FLASH_SIZE)
		memcpy(dump_buf + cfgOff, data, cfgSize);
	else
		STX_ERROR("%s size=0x%x", __func__, size);
}

void st_set_fw(const char *data, int fwOff, int fwSize)
{
	int size;
	size = fwOff + fwSize;

	if (size <= ST_FLASH_SIZE)
		memcpy(dump_buf + fwOff, data, fwSize);
	else
		STX_ERROR("%s size=0x%x", __func__, size);
}

///I2C
int st_i2c_upg_read_direct(st_u8 *rxbuf, int len)
{
	int ret = 0;
	ret = stx_i2c_read_direct(rxbuf, len);
	if (ret < 0) {
		STX_ERROR("read direct error (%d)", ret);
		return ret;
	}
	return len;
}

int st_i2c_upg_read_bytes(st_u8 addr, st_u8 *rxbuf, int len)
{
	int ret = 0;
	ret = stx_i2c_read_bytes(addr, rxbuf, len);
	if (ret < 0) {
		STX_ERROR("read 0x%x error (%d)", addr, ret);
		return ret;
	}
	return len;
}

int st_i2c_upg_write_bytes(st_u8 *txbuf, int len)
{
	int ret = 0;
	if (txbuf == NULL)
		return -1;

	ret = stx_i2c_write_bytes(txbuf, len);
	if (ret < 0) {
		STX_ERROR("write 0x%x error (%d)", *txbuf, ret);
		return ret;
	}
	return len;
}
//////////
int st_get_device_status(void)
{
	int ret = 0;
	unsigned char buffer[8];

	ret = st_i2c_upg_read_bytes(1, buffer, 8);
	if (ret < 0) {
		STX_ERROR("read status reg error (%d)", ret);
		return ret;
	} else {
		STX_INFO("status reg = %d", buffer[0]);
	}

	return buffer[0] & 0xF;
}

int st_check_device_status(int ck1, int ck2, int delay)
{
	int maxTimes = 10;
	int isInStauts = 0;
	int status = -1;
	while (maxTimes-- > 0 && isInStauts == 0) {
		status = st_get_device_status();
		STX_INFO("status : %d", status);
		if (status == ck1 || status == ck2)
			isInStauts = 1;
		st_msleep(delay);
	}
	if (isInStauts == 0)
		return -1;
	else
		return 0;
}

int st_isp_off(void)
{
	unsigned char data[8];
	int rt = 0;

	memset(data, 0, sizeof(data));
	data[0] = ISP_CMD_RESET;

	rt += st_i2c_upg_write_bytes(data, sizeof(data));

	if (rt < 0) {
		STX_ERROR("ISP off error");
		return -1;
	}
	st_msleep(300);

	return st_check_device_status(0, 4, 10);
}
int st_isp_on(void)
{
	unsigned char IspKey[] = { 0, 'S', 0, 'T', 0, 'X', 0, '_',
				   0, 'F', 0, 'W', 0, 'U', 0, 'P' };
	unsigned char i;
	int icStatus = st_get_device_status();

	STX_INFO("ISP on");

	if (icStatus < 0)
		return -1;
	if (icStatus == 0x6)
		return 0;
	else if (icStatus == 0x5)
		st_power_up(&stx_gpts);

	for (i = 0; i < sizeof(IspKey); i += 2) {
		if (st_i2c_upg_write_bytes(&IspKey[i], 2) < 0) {
			STX_ERROR("Entering ISP fail.");
			return -1;
		}
	}
	st_msleep(300); //This delay is very important for ISP mode changing.
		//Do not remove this delay arbitrarily.
	return st_check_device_status(6, 99, 10);
}

int st_check_chipid(void)
{
	int ret = 0;
	st_u8 buffer[8];

	ret = st_i2c_upg_read_bytes(0x1, buffer, 8);
	if (ret < 0) {
		STX_ERROR("read status reg error (%d)", ret);
		return ret;
	} else {
		STX_INFO("ChipID = %d", buffer[1]);
	}

#ifdef ST_IC_A8008
	if (buffer[1] != 0x6) {
		STX_ERROR("This IC is not A8008 , cancel upgrade");
		return -1;
	}
#endif
#ifdef ST_IC_A8010
	if (buffer[1] != 0xA) {
		STX_ERROR("This IC is not A8010 , cancel upgrade");
		return -1;
	}
#endif
#ifdef ST_IC_A8015
	if (buffer[1] != 0xF) {
		STX_ERROR("This IC is not A8015 , cancel upgrade");
		return -1;
	}
#endif
	return 0;
}

int st_compare_array(unsigned char *b1, unsigned char *b2, int len)
{
	int i = 0;
	for (i = 0; i < len; i++) {
		if (b1[i] != b2[i])
			return -1;
	}
	return 0;
}

#ifdef ST_IC_A8010
#define CRC8_POLY 0xD5
#define CRC16_POLY 0x1021
u32 RemainderCal(u32 poly, u32 dat, u8 polysize, u8 datasize)
{
	const u8 SHIFT_MAX = datasize - (polysize + 1);
	u8 ind;
	u32 MSBOFDATA = (u32)1 << (datasize - 1);
	u32 tmp;

	poly |= (1 << polysize);
	for (ind = 0; ind <= SHIFT_MAX; ind++) {
		if ((dat << ind) & MSBOFDATA) { //if MSB == 1
			tmp = poly << (SHIFT_MAX - ind);
			dat ^= tmp; //poly dosen't include the MSB of the divider.
		}
	}
	return dat; // remainder is the lowest N bits
}

u16 Crc16Cal(u16 poly, u8 *dat, u32 dat_len)
{
	const u8 POLYSIZE = 16, DATASIZE = 8;
	u32 crc, ind;

	crc = dat[0];
	for (ind = 1; ind < dat_len; ind++) {
		crc = (crc << DATASIZE) | dat[ind];
		crc = RemainderCal(poly, crc, POLYSIZE,
				   (u8)(POLYSIZE + DATASIZE)); // 16 bit CRC
	}
	crc = crc
	      << DATASIZE; // CRC16, the data should be shifted left 16bits. Shift 8 bit first
	crc = RemainderCal(poly, crc, POLYSIZE, (u8)(POLYSIZE + DATASIZE));
	crc = crc << (POLYSIZE - DATASIZE); // Shift the rest 8bits
	crc = RemainderCal(poly, crc, POLYSIZE, (u8)(POLYSIZE + DATASIZE));
	return crc;
}
#endif //ST_IC_A8010

void ChecksumCalculation(unsigned short *pChecksum, unsigned char *pInData,
			 unsigned long Len)
{
#ifdef ST_IC_A8010
	*pChecksum = Crc16Cal(CRC16_POLY, pInData, Len);
#else
	unsigned long i;
	unsigned char LowByteChecksum;
	for (i = 0; i < Len; i++) {
		*pChecksum += (unsigned short)pInData[i];
		LowByteChecksum = (unsigned char)(*pChecksum & 0xFF);
		LowByteChecksum = (LowByteChecksum) >> 7 | (LowByteChecksum)
								   << 1;
		*pChecksum = (*pChecksum & 0xFF00) | LowByteChecksum;
	}
#endif //ST_IC_A8010
}

int st_flash_unlock(void)
{
	unsigned char PacketData[ISP_PACKET_SIZE];

	int retryCount = 0;
	int isSuccess = 0;
	while (isSuccess == 0 && retryCount++ < ST_ISP_RETRY_MAX) {
		memset(PacketData, 0, ISP_PACKET_SIZE);
		PacketData[0] = ISP_CMD_UNLOCK;
		if (st_i2c_upg_write_bytes(PacketData, ISP_PACKET_SIZE) ==
		    ISP_PACKET_SIZE) {
			if (retryCount > 1)
				st_msleep(150);

			if (st_i2c_upg_read_direct(PacketData,
						   ISP_PACKET_SIZE) ==
			    ISP_PACKET_SIZE) {
				if (PacketData[0] == ISP_CMD_READY)
					isSuccess = 1;
			} else
				st_msleep(50);
		}

		if (isSuccess == 0) {
			STX_ERROR(
				"Read ISP_Unlock_Ready packet fail retry : %d",
				retryCount);
			//MSLEEP(30);
		}
	}

	if (isSuccess == 0) {
		STX_ERROR("Read ISP_Unlock_Ready packet fail.");
		return -1;
	}

	return 0;
}

int st_flash_erase_page(unsigned short PageNumber)
{
	unsigned char PacketData[ISP_PACKET_SIZE];

	int retryCount = 0;
	int isSuccess = 0;
	while (isSuccess == 0 && retryCount++ < ST_ISP_RETRY_MAX) {
		memset(PacketData, 0, ISP_PACKET_SIZE);
		PacketData[0] = ISP_CMD_ERASE;
		PacketData[2] = (unsigned char)PageNumber;
		if (st_i2c_upg_write_bytes(PacketData, ISP_PACKET_SIZE) ==
		    ISP_PACKET_SIZE) {
			//if(retryCount > 1)
			st_msleep(150);

			if (st_i2c_upg_read_direct(PacketData,
						   ISP_PACKET_SIZE) ==
			    ISP_PACKET_SIZE) {
				if (PacketData[0] == ISP_CMD_READY)
					isSuccess = 1;
			} else {
				//time out
				st_msleep(50);
			}
		}

		if (isSuccess == 0) {
			STX_ERROR(
				"Read ISP_Erase_Ready packet fail with page %d retry : %d",
				PageNumber, retryCount);
			//MSLEEP(30);
		}
	}

	if (isSuccess == 0) {
		STX_ERROR("Read ISP_Erase_Ready packet fail.");
		return -1;
	}

	return 0;
}

int st_flash_read_page(unsigned char *Buf, unsigned short PageNumber)
{
	unsigned char PacketData[ISP_PACKET_SIZE];
	short ReadNumByte;
	short ReadLength;

#ifdef ST_IC_A8015
	if (st_flash_unlock() < 0)
		return -1;
#endif

	ReadNumByte = 0;
	memset(PacketData, 0, ISP_PACKET_SIZE);
	PacketData[0] = ISP_CMD_READ_FLASH;
	PacketData[2] = (unsigned char)PageNumber;
	if (st_i2c_upg_write_bytes(PacketData, ISP_PACKET_SIZE) !=
	    ISP_PACKET_SIZE) {
		STX_ERROR("Send ISP_Read_Flash packet fail.");
		return -1;
	}
	msleep(5);
	while (ReadNumByte < ST_FLASH_PAGE_SIZE) {
		if ((ReadLength = st_i2c_upg_read_direct(Buf + ReadNumByte,
							 ISP_PACKET_SIZE)) !=
		    ISP_PACKET_SIZE) {
			STX_ERROR("ISP read page data fail.");
			return -1;
		}
		if (ReadLength == 0)
			break;
		ReadNumByte += ReadLength;
	}
	return ReadNumByte;
}

int st_flash_write_page(unsigned char *Buf, unsigned short PageNumber)
{
	unsigned char PacketData[ISP_PACKET_SIZE];
	short WriteNumByte;
	short WriteLength;
	unsigned short Checksum = 0;
	unsigned char RetryCount;

	RetryCount = 0;
	while (RetryCount++ < 1) {
		WriteNumByte = 0;
		memset(PacketData, 0, ISP_PACKET_SIZE);
		ChecksumCalculation(&Checksum, Buf, ST_FLASH_PAGE_SIZE);

		//Checksum = st_flash_get_checksum(Buf,ST_FLASH_PAGE_SIZE);

		PacketData[0] = ISP_CMD_WRITE_FLASH;
		PacketData[2] = (unsigned char)PageNumber;
		//PacketData[4] = (unsigned char)(Checksum & 0xFF);
		//PacketData[5] = (unsigned char)(Checksum >> 8);
		if (st_i2c_upg_write_bytes(PacketData, ISP_PACKET_SIZE) !=
		    ISP_PACKET_SIZE) {
			STX_ERROR("Send ISP_Write_Flash packet fail.");
			return -1;
		}
		PacketData[0] = ISP_CMD_SEND_DATA;
		while (WriteNumByte < ST_FLASH_PAGE_SIZE) {
			WriteLength = ST_FLASH_PAGE_SIZE - WriteNumByte;
			if (WriteLength > 7)
				WriteLength = 7;
			memcpy(&PacketData[1], &Buf[WriteNumByte], WriteLength);
			if (st_i2c_upg_write_bytes(PacketData,
						   ISP_PACKET_SIZE) !=
			    ISP_PACKET_SIZE) {
				STX_ERROR(
					"Send ISP_Write_Flash_Data packet error.");
				return -1;
			}
			WriteNumByte += WriteLength;
		}

		st_msleep(150);
		if (st_i2c_upg_read_direct(PacketData, ISP_PACKET_SIZE) !=
		    ISP_PACKET_SIZE) {
			STX_ERROR("ISP get \"Write Data Ready Packet\" fail.");
			return -1;
		}
		if (PacketData[0] != ISP_CMD_READY) {
			STX_ERROR(
				"Command ID of \"Write Data Ready Packet\" error.");
			return -1;
		}

		if ((PacketData[4] != (Checksum >> 8)) ||
		    (PacketData[5] != (Checksum & 0xFF))) {
			STX_ERROR(
				"Checksum error , ISP output = 0x%02X%02X , driver = 0x%04X",
				PacketData[4], PacketData[5], Checksum);
			return -1;
		}

		break;
	}
	return (int)WriteNumByte;
}

int st_flash_write(unsigned char *Buf, int Offset, int NumByte)
{
	unsigned short StartPage;
	unsigned short PageOffset;
	int WriteNumByte;
	short WriteLength;
	//unsigned char TempBuf[ST_FLASH_PAGE_SIZE];
	unsigned char *TempBuf;
	int retry = 0;
	int isSuccess = 0;

	TempBuf = kzalloc(ST_FLASH_PAGE_SIZE, GFP_KERNEL);
	STX_INFO("Write flash offset:0x%X , length:0x%X", Offset, NumByte);

	WriteNumByte = 0;
	if (NumByte == 0)
		return WriteNumByte;

	if ((Offset + NumByte) > ST_FLASH_SIZE)
		NumByte = ST_FLASH_SIZE - Offset;

	StartPage = Offset / ST_FLASH_PAGE_SIZE;
	PageOffset = Offset % ST_FLASH_PAGE_SIZE;
	while (NumByte > 0) {
		if ((PageOffset != 0) || (NumByte < ST_FLASH_PAGE_SIZE)) {
			if (st_flash_read_page(TempBuf, StartPage) < 0)
				return -1;
		}

		WriteLength = ST_FLASH_PAGE_SIZE - PageOffset;
		if (NumByte < WriteLength)
			WriteLength = NumByte;
		memcpy(&TempBuf[PageOffset], Buf, WriteLength);

		retry = 0;
		isSuccess = 0;
		while (retry++ < ST_ISP_RETRY_MAX && isSuccess == 0) {
			if (st_flash_unlock() >= 0 &&
			    st_flash_erase_page(StartPage) >= 0) {
				STX_INFO("write page:%d", StartPage);
				if (st_flash_unlock() >= 0 &&
				    st_flash_write_page(TempBuf, StartPage) >=
					    0)
					isSuccess = 1;
			}
			//isSuccess =1;

			if (isSuccess == 0)
				STX_INFO(
					"FIOCTL_IspPageWrite write page %d retry: %d",
					StartPage, retry);
		}
		if (isSuccess == 0) {
			STX_ERROR("FIOCTL_IspPageWrite write page %d error",
				  StartPage);
			return -1;
		} else
			StartPage++;

		NumByte -= WriteLength;
		Buf += WriteLength;
		WriteNumByte += WriteLength;
		PageOffset = 0;
	}
	kfree(TempBuf);
	return WriteNumByte;
}

int stx_clr_crash_msg(void)
{
	return 0;
}

#ifdef ST_UPGRADE_BY_ISPID
unsigned char g_isp_id[] = SITRONIX_ISP_ID;

//unsigned char dump_buf[] = SITRONIX_DUMP;
const unsigned char dump_buf1[] = SITRONIX_DUMP1;
const unsigned char dump_buf2[] = SITRONIX_DUMP2;

static void st_replace_fw_by_id(int id)
{
	int size;
	if (id == 0) {
		STX_INFO("Found id by SITRONIX_DUMP 0\n");
		//		memcpy(dump_buf,dump_buf,sizeof(dump_buf));
	} else if (id == 1) {
		STX_INFO("Found id by SITRONIX_DUMP 1\n");
		size = sizeof(dump_buf1);
		if (size > ST_FLASH_SIZE)
			size = ST_FLASH_SIZE;

		memcpy(dump_buf, dump_buf1, size);
	} else if (id == 2) {
		STX_INFO("Found id by SITRONIX_DUMP 2\n");
		size = sizeof(dump_buf2);
		if (size > ST_FLASH_SIZE)
			size = ST_FLASH_SIZE;
		memcpy(dump_buf, dump_buf2, size);
	}
}

static int st_select_fw_by_id(int isp_idL, int isp_idH)
{
	int i = 0;
	int idlen = sizeof(g_isp_id) / 2;
	int isFindID = 0;
	int status = st_get_device_status();
	if (status < 0) {
		return -1;
	} else if (status == 0x6) {
		for (i = 0; i < idlen; i++) {
			if ((isp_idL == g_isp_id[i * 2]) &&
			    (isp_idH == g_isp_id[i * 2 + 1])) {
				isFindID = 1;
				st_replace_fw_by_id(i);
				break;
			}
		}
		if (0 == isFindID) {
			STX_ERROR("can not find ISPID");
			return -1;
		}
	}
	return 0;
}

#endif

static void sitronix_copystring_without_line(const char *buf, char *path)
{
	int len = strlen(buf);

	strcpy(path, buf);
	if (buf[len - 1] == 10)
		path[len - 1] = 0;
}

static int sitronix_request_fw_table(const char *buf)
{
	int ret = 0;
	const struct firmware *fw = NULL;
	char path[200];
	int fwOff;
	int fwSize;
	int fwInfoOff;
	int cfgOff;
	int cfgSize;

	sitronix_copystring_without_line(buf, path);

	STX_FUNC_ENTER();
	STX_INFO("input: %s", buf);
	ret = request_firmware(&fw, path, NULL);
	STX_INFO("request_firmware ret = %d", ret);
	if (ret)
		return -ENOENT;

	STX_INFO("fw size 0x%X", (unsigned int)fw->size);

	ret = st_check_fw(fw->data, &fwOff, &fwSize, &fwInfoOff, &cfgOff);
	STX_INFO("sitronix_set_fw_buf ret = %d", ret);
	if (ret == 0) {
		st_set_fw(fw->data + fwOff, fwOff, fwSize);
		if (fw->size > cfgOff) {
			ret = st_check_cfg(fw->data + cfgOff, &cfgSize);
			STX_INFO("sitronix_set_cfg_buf ret = %d", ret);
			if (ret == 0) {
				st_set_cfg(fw->data + cfgOff, cfgOff, cfgSize);
			}
		}
	} else {
		ret = st_check_cfg(fw->data, &cfgSize);
		STX_INFO("sitronix_set_cfg_buf ret = %d", ret);
		if (ret == 0) {
			st_set_cfg(fw->data, cfgOff, cfgSize);
		}
	}

	release_firmware(fw);
	if (ret != 0)
		return ret;
	return 0;
}

static int sitronix_do_upgrade(void)
{
	int rt = 0;
	int fwOff = 0;
	int cfgOff = 0;
	int fwSize = 0;
	int cfgSize = 0;
	int fwInfoOff = 0;
	int fwInfoLen = 0;
	int powerfulWrite = 0;
	int checkOff = 0;

	rt = st_check_fw(dump_buf, &fwOff, &fwSize, &fwInfoOff, &cfgOff);
	if (rt < 0)
		return -1;

	rt = st_check_cfg(dump_buf + cfgOff, &cfgSize);
	if (rt < 0)
		return -1;

	fwInfoLen = dump_buf[fwInfoOff] + 1 + 4;

	STX_INFO("fwInfoLen 0x%x", fwInfoLen);

	if (st_get_device_status() == 0x6)
		powerfulWrite = 1;

	st_irq_off();

	rt = st_isp_on();

	if (rt != 0) {
		STX_ERROR("ISP on fail");
		goto ST_IRQ_ON;
	}

	if (st_check_chipid() < 0) {
		STX_ERROR("Check ChipId fail");
		rt = -1;
		goto ST_ISP_OFF;
	}

	if (powerfulWrite == 0) {
		checkOff =
			(fwInfoOff / ST_FLASH_PAGE_SIZE) * ST_FLASH_PAGE_SIZE;

		if (st_flash_read_page(fw_check,
				       fwInfoOff / ST_FLASH_PAGE_SIZE) < 0) {
			STX_ERROR("read flash fail , cancel upgrade");
			rt = -1;
			goto ST_ISP_OFF;
		}

		if (0 == st_compare_array(fw_check + (fwInfoOff - checkOff),
					  dump_buf + fwInfoOff, fwInfoLen)) {
			STX_INFO("fwInfo compare :same\n");

			st_flash_read_page(fw_check, 0x0);
			if (0 == st_compare_array(fw_check + TFW_CHECK_OFFSET,
						  dump_buf + TFW_CHECK_OFFSET,
						  TFW_CHECK_LENGTH)) {
				STX_INFO("FW :same\n");
				fwSize = 0;
			} else
				STX_INFO("FW compare : different\n");
		} else {
			STX_INFO("fw compare :different");
		}

		////////////
		checkOff = (cfgOff / ST_FLASH_PAGE_SIZE) * ST_FLASH_PAGE_SIZE;
		st_flash_read_page(fw_check, cfgOff / ST_FLASH_PAGE_SIZE);

		if (0 == st_compare_array(fw_check + (cfgOff - checkOff),
					  dump_buf + cfgOff, cfgSize)) {
			STX_INFO("cfg compare :same");
			cfgSize = 0;
		} else {
			STX_INFO("cfg compare : different");
		}
	}

	if (cfgSize != 0)
		st_flash_write(dump_buf + cfgOff, cfgOff, cfgSize);

	if (fwSize != 0)
		st_flash_write(dump_buf + fwOff, fwOff, fwSize);

ST_ISP_OFF:
	rt = st_isp_off();
ST_IRQ_ON:
	st_irq_on();

	if (cfgSize != 0 || fwSize != 0)
		return 1;
	else
		return rt;

	return rt;
}

int st_upgrade_fw(void)
{
	int rt = 0;
	int fwOff = 0;
	int cfgOff = 0;
	int fwSize = 0;
	int cfgSize = 0;
	int fwInfoOff = 0;
	int fwInfoLen = 0;
	int powerfulWrite = 0;
	int checkOff = 0;

	STX_INFO("st upgrade fw_____________________\n");
	if (st_get_device_status() == 0x6)
		powerfulWrite = 1;

	st_irq_off();

	rt = st_isp_on();

	if (rt != 0) {
		STX_ERROR("ISP on fail");
		goto ST_IRQ_ON;
	}

	if (st_check_chipid() < 0) {
		STX_ERROR("Check ChipId fail");
		rt = -1;
		goto ST_ISP_OFF;
	}

#ifdef ST_UPGRADE_BY_ISPID
	if (st_flash_read_page(fw_check, ISP_ID_OFFSET / ST_FLASH_PAGE_SIZE) <
	    0) {
		STX_ERROR("read flash fail , cancel upgrade\n");
		rt = -1;
		goto ST_ISP_OFF;
	}
	checkOff = ISP_ID_OFFSET -
		   (ISP_ID_OFFSET / ST_FLASH_PAGE_SIZE) * ST_FLASH_PAGE_SIZE;
	STX_INFO("ISP ID checkoff 0x%x , 0x%x \n", fw_check[checkOff],
		 fw_check[checkOff + 1]);
	st_select_fw_by_id(fw_check[checkOff], fw_check[checkOff + 1]);
	checkOff = 0;
#endif

	rt = st_check_fw(dump_buf, &fwOff, &fwSize, &fwInfoOff, &cfgOff);
	if (rt < 0) {
		STX_ERROR("Check fw fail");
		goto ST_ISP_OFF;
		//return -1;
	}

	rt = st_check_cfg(dump_buf + cfgOff, &cfgSize);
	if (rt < 0) {
		STX_ERROR("Check cfg fail");
		goto ST_ISP_OFF;
		//return -1;
	}

	fwInfoLen = dump_buf[fwInfoOff] + 1 + 4;

	STX_INFO("fwInfoLen 0x%x", fwInfoLen);

	if (powerfulWrite == 0) {
		checkOff =
			(fwInfoOff / ST_FLASH_PAGE_SIZE) * ST_FLASH_PAGE_SIZE;

		if (st_flash_read_page(fw_check,
				       fwInfoOff / ST_FLASH_PAGE_SIZE) < 0) {
			STX_ERROR("read flash fail , cancel upgrade");
			rt = -1;
			goto ST_ISP_OFF;
		}

		if (0 == st_compare_array(fw_check + (fwInfoOff - checkOff),
					  dump_buf + fwInfoOff, fwInfoLen)) {
			STX_INFO("fwInfo compare :same\n");

			st_flash_read_page(fw_check, 0x0);
			if (0 == st_compare_array(fw_check + TFW_CHECK_OFFSET,
						  dump_buf + TFW_CHECK_OFFSET,
						  TFW_CHECK_LENGTH)) {
				STX_INFO("FW :same\n");
				fwSize = 0;
			} else
				STX_INFO("FW compare : different\n");
		} else {
			STX_INFO("fw compare :different");
		}

		////////////
		checkOff = (cfgOff / ST_FLASH_PAGE_SIZE) * ST_FLASH_PAGE_SIZE;
		st_flash_read_page(fw_check, cfgOff / ST_FLASH_PAGE_SIZE);

		if (0 == st_compare_array(fw_check + (cfgOff - checkOff),
					  dump_buf + cfgOff, cfgSize)) {
			STX_INFO("cfg compare :same");
			cfgSize = 0;
		} else {
			STX_INFO("cfg compare : different");
		}
	}

	if (cfgSize != 0)
		st_flash_write(dump_buf + cfgOff, cfgOff, cfgSize);

	if (fwSize != 0)
		st_flash_write(dump_buf + fwOff, fwOff, fwSize);

ST_ISP_OFF:
	rt = st_isp_off();
ST_IRQ_ON:
	st_irq_on();

	if (cfgSize != 0 || fwSize != 0)
		return 1;
	else
		return rt;

	return rt;
}

int st_upgrade_fw_handler(void *unused)
{
	int ret = 0;

	ret = st_upgrade_fw();
	st_get_touch_info(&stx_gpts);
	return ret;
}

int stx_force_upgrade_fw(char *path)
{
	int ret = 0;
	sitronix_request_fw_table(path);
	ret = sitronix_do_upgrade();
	return ret;
}
