#include "sitronix_ts_custom_func.h"
#include "sitronix_ts.h"

static DEFINE_MUTEX(i2c_rw_mutex);

s32 stx_i2c_write(struct i2c_client *client, u8 *buf, int len)
{
	s32 ret;

	struct i2c_msg msg = {
		.flags = 0,
#ifdef CONFIG_MTK_I2C_EXTENSION
		.addr = (client->addr & I2C_MASK_FLAG) |
			(I2C_ENEXT_FLAG), /*remain*/
		.timing = I2C_MASTER_CLOCK,
#else
		.addr = client->addr, /*remain*/
#endif
		.len = len,
		.buf = buf,
	};
	mutex_lock(&i2c_rw_mutex);
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		STX_ERROR("Sitronix I2C error in tpd_i2c_write, ret = %d", ret);
		mutex_unlock(&i2c_rw_mutex);
		return ret;
	}
	mutex_unlock(&i2c_rw_mutex);
	return ret;
}

int stx_i2c_read(struct i2c_client *client, u8 *buffer, int len, u8 addr)
{
	int ret;
	u8 txbuf = addr;

	struct i2c_msg msgs[2] = {
		{
#ifdef CONFIG_MTK_I2C_EXTENSION
			.addr = ((client->addr & I2C_MASK_FLAG) |
				 (I2C_ENEXT_FLAG)),
			.timing = I2C_MASTER_CLOCK,
#else
			.addr = client->addr,
#endif
			.flags = 0,
			.buf = &txbuf,
			.len = 1,
		},
		{
#ifdef CONFIG_MTK_I2C_EXTENSION
			.addr = ((client->addr & I2C_MASK_FLAG) |
				 (I2C_ENEXT_FLAG)),
			.timing = I2C_MASTER_CLOCK,
#else
			.addr = client->addr,
#endif
			.flags = I2C_M_RD,

			.len = len,
			.buf = buffer,
		},
	};
	mutex_lock(&i2c_rw_mutex);
	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret < 0) {
		STX_ERROR("Sitronix I2C error in tpd_i2c_read, ret = %d", ret);
		mutex_unlock(&i2c_rw_mutex);
		return ret;
	}
	mutex_unlock(&i2c_rw_mutex);
	return len;
}

int stx_i2c_read_direct(st_u8 *rxbuf, int len)
{
	int ret = 0;

	mutex_lock(&i2c_rw_mutex);
	ret = i2c_master_recv(stx_gpts.client, rxbuf, len);
	if (ret < 0) {
		STX_ERROR("read direct error (%d)", ret);
		mutex_unlock(&i2c_rw_mutex);
		return ret;
	}
	mutex_unlock(&i2c_rw_mutex);
	udelay(50);
	return len;
}

int stx_i2c_read_bytes(st_u8 addr, st_u8 *rxbuf, int len)
{
	int ret = 0;
	st_u8 txbuf = addr;

	mutex_lock(&i2c_rw_mutex);
	ret = i2c_master_send(stx_gpts.client, &txbuf, 1);
	if (ret < 0) {
		STX_ERROR("stx_i2c_read_bytes write 0x%x error (%d)", addr,
			  ret);
		goto unlock;
	}
	udelay(50);
	ret = i2c_master_recv(stx_gpts.client, rxbuf, len);
	if (ret < 0) {
		STX_ERROR("stx_i2c_read_bytes read 0x%x error (%d)", addr, ret);
		goto unlock;
	}
	mutex_unlock(&i2c_rw_mutex);
	udelay(50);
	return len;
unlock:
	mutex_unlock(&i2c_rw_mutex);
	return ret;
}

int stx_i2c_write_bytes(st_u8 *txbuf, int len)
{
	int ret = 0;
	if (txbuf == NULL)
		return -1;

	mutex_lock(&i2c_rw_mutex);
	ret = i2c_master_send(stx_gpts.client, txbuf, len);
	if (ret < 0) {
		STX_ERROR("stx_i2c_write_bytes write 0x%x error (%d)", *txbuf,
			  ret);
		mutex_unlock(&i2c_rw_mutex);
		return ret;
	}
	mutex_unlock(&i2c_rw_mutex);
	udelay(50);
	return len;
}

struct CommandIoPacket
{
	unsigned char CmdID;
	unsigned char ValidDataSize;
	unsigned char CmdData[30];
};

static void STChecksumCalculation(unsigned short *pChecksum,
				  unsigned char *pInData, unsigned long Len)
{
	unsigned long i;
	unsigned char LowByteChecksum;

	for (i = 0; i < Len; i++) {
		*pChecksum += (unsigned short)pInData[i];
		LowByteChecksum = (unsigned char)(*pChecksum & 0xFF);
		LowByteChecksum = (LowByteChecksum) >> 7 | (LowByteChecksum)
								   << 1;
		*pChecksum = (*pChecksum & 0xFF00) | LowByteChecksum;
	}
}

static bool stx_SetH2DReady(void)
{
	int ret;
	bool bRet = false;
	unsigned char buf[2];

	buf[0] = 0xF8;
	buf[1] = 0x01;
	ret = stx_i2c_write_bytes(buf, 2);
	if (ret <= 0) {
		STX_ERROR("stx_SetH2DReady: write ready error.");
		bRet = false;
	} else {
		bRet = true;
	}
	return bRet;
}

static bool stx_GetH2DReady(void)
{
	bool bRet = false;
	unsigned char tmp = 0xff;
	int ret, retry = 0;

	do {
		msleep(1);
		ret = stx_i2c_read_bytes(0xF8, &tmp, 1);
		if (ret <= 0) {
			STX_ERROR("stx_GetH2DReady: retry(%d) read ready.",
				  retry++);
		} else {
			if (tmp == 0x01) {
				STX_ERROR("retry  ............");
				retry = 0;
				continue;
			}
		}
		if (retry > 1000) {
			STX_ERROR("stx_GetH2DReady: time out");
			bRet = false;
			return bRet;
		}
	} while (ret <= 0);

	if (tmp == 0x00) { /* OK */
		bRet = true;
	} else if (tmp == 0x80) {
		STX_ERROR("stx_ReadIOCommand: Unknown Command ID.");
		bRet = false;
	} else if (tmp == 0x81) {
		STX_ERROR(
			"stx_ReadIOCommand: Host to device command checksum error.");
		bRet = false;
	} else {
		STX_ERROR("Unknown Error(0x%02X).", tmp);
		bRet = false;
	}
	return bRet;
}

static bool stx_ReadIOCommand(struct CommandIoPacket *packet)
{
	bool bRet = false;
	int ret;
	unsigned char tmp[32];
	/* process commmand */
	memset(tmp, 0, 32);
	ret = stx_i2c_read_bytes(0xD0, tmp, 32);
	if (ret <= 0) {
		STX_ERROR("stx_ReadIOCommand: read packet error.");
		bRet = false;
	} else {
		memcpy((void *)packet, (const void *)tmp, 32);
		bRet = true;
	}
	return bRet;
}

static bool stx_WriteIOCommand(struct CommandIoPacket *packet)
{
	bool bRet = false;
	int ret;
	unsigned char tmp[33];

	memset(tmp, 0x00, 33);
	memcpy((void *)tmp + 1, (const void *)packet, 32);
	tmp[0] = 0xD0;
	ret = stx_i2c_write_bytes(tmp, 33);
	if (ret <= 0) {
		STX_ERROR("stx_WriteIOCommand: write packet error.");
		bRet = false;
	} else {
		bRet = true;
	}
	return bRet;
}
int stx_cmdio_read(int type, int address, unsigned char *buf, int len)
{
	int getLen = 0, offset = 0;
	struct CommandIoPacket outPacket;
	struct CommandIoPacket inPacket;
	int remain = len;
	int pktDataSize = 0;
	unsigned short chksum, vchksum;
	int retry = 0;

	do {
		pktDataSize = (remain > 24) ? 24 : remain;
		outPacket.CmdID = 0x02; /* read RAM/ROM */
		outPacket.ValidDataSize = 5;
		outPacket.CmdData[0] = type; /* RAM */
		outPacket.CmdData[1] =
			(((address + offset) >> 8) & 0xFF); /* high byte */
		outPacket.CmdData[2] =
			(((address + offset)) & 0xFF); /* low byte */
		outPacket.CmdData[3] = pktDataSize;
		chksum = 0;
		STChecksumCalculation(&chksum, (unsigned char *)&outPacket, 6);
		outPacket.CmdData[4] = (chksum & 0xFF);

		if (!stx_WriteIOCommand(&outPacket)) {
			STX_ERROR("stx_cmdio_read: (E)stx_WriteIOCommand.");
			return getLen;
		}
		if (!stx_SetH2DReady()) {
			STX_ERROR("stx_cmdio_read: (E)stx_SetH2DReady.");
			return getLen;
		}
		if (!stx_GetH2DReady()) {
			STX_ERROR("stx_cmdio_read: (E)stx_GetH2DReady.");
			return getLen;
		}
		if (!stx_ReadIOCommand(&inPacket)) {
			STX_ERROR("stx_cmdio_read: (E)stx_ReadIOCommand.");
			return getLen;
		}
		if (inPacket.CmdID == 0x82 && inPacket.CmdData[0] == type) {
			vchksum = 0;
			STChecksumCalculation(&vchksum,
					      (unsigned char *)&inPacket,
					      inPacket.ValidDataSize + 1);
			vchksum = (vchksum & 0xFF);
			if (vchksum ==
			    inPacket.CmdData[inPacket.ValidDataSize - 1]) {
				memcpy(buf + offset, &(inPacket.CmdData[2]),
				       inPacket.CmdData[1]);
				remain -= inPacket.CmdData[1]; /* data size */
				offset += inPacket.CmdData[1];
				getLen += inPacket.CmdData[1];
			} else {
				/* drop packet */
				STX_ERROR(
					"Invalid Cheksum Expect(0x%02x) Get(0x%02X)",
					vchksum,
					inPacket.CmdData[inPacket.ValidDataSize -
							 1]);
			}
			retry = 0;
		} else {
			/* drop packet */
			STX_ERROR("Unexpect CmdID (0x%02x) or Type (0x%02X)",
				  inPacket.CmdID, inPacket.CmdData[0]);
			retry++;
			if (retry > 10)
				return -1;
		}
	} while (remain > 0);
	return getLen;
}

int stx_cmdio_write(int type, int address, unsigned char *buf, int len)
{
	int setLen = 0, offset = 0;
	struct CommandIoPacket outPacket;
	int remain = len;
	int pktDataSize = 0;
	unsigned short chksum; /* , vchksum; */

	do {
		pktDataSize = (remain > 24) ? 24 : remain;
		outPacket.CmdID = 0x01; /* write RAM/ROM */
		outPacket.ValidDataSize = pktDataSize + 5;
		outPacket.CmdData[0] = type;
		outPacket.CmdData[1] =
			(((address + offset) >> 8) & 0xFF); /* high byte */
		outPacket.CmdData[2] =
			(((address + offset)) & 0xFF); /* low byte */
		outPacket.CmdData[3] = pktDataSize;
		memcpy((void *)&(outPacket.CmdData[4]),
		       (const void *)(buf + offset), pktDataSize);
		chksum = 0;
		STChecksumCalculation(&chksum, (unsigned char *)&outPacket,
				      outPacket.ValidDataSize + 1);
		outPacket.CmdData[outPacket.ValidDataSize - 1] =
			(chksum & 0xFF);
		if (!stx_WriteIOCommand(&outPacket)) {
			STX_ERROR("TDU_SetDeviceRam: (E)stx_WriteIOCommand.");
			return setLen;
		}
		if (!stx_SetH2DReady()) {
			STX_ERROR("TDU_SetDeviceRam: (E)stx_SetH2DReady.");
			return setLen;
		}
		/* check processing result */
		if (!stx_GetH2DReady()) {
			STX_ERROR("TDU_SetDeviceRam: (E)stx_GetH2DReady.");
			return setLen;
		} else {
			/* processing write command OK */
			remain -= pktDataSize;
			offset += pktDataSize;
			setLen += pktDataSize;
		}

	} while (remain > 0);

	return setLen;
}