#include "vendor_mau.h"
#include "hdal.h"
#include "ddr_arb_ioctl.h"
#include <string.h>

#if defined(__LINUX)
#include <sys/ioctl.h>
#endif

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT32 vendor_mau_ch_mon_start(int ch, int rw, int dram)
{
	MON_INFO mon_info = {0};
	INT32 mau_fd = -1;
	INT32 ret;

	// open ive device
	mau_fd= open("/dev/nvt_ddr_arb0", O_RDWR);
	if (mau_fd < 0) {
		printf("[VENDOR_MAU] Open fail!\n");
		return HD_ERR_NG;
	}

	mon_info.ch = ch;
	mon_info.rw = rw;
	mon_info.dram = dram;

	ret = ioctl(mau_fd, DDR_IOC_MON_START, &mon_info);
	if (ret < 0) {
		printf("[VENDOR_MAU] mon start fail!\n");
		return ret;
	}

	if (mau_fd != -1) {
		close(mau_fd);
	}

	return 0;
}

UINT64 vendor_mau_ch_mon_stop(int ch, int dram)
{
	MON_INFO mon_info = {0};
	INT32 ret;
	INT32 mau_fd = -1;

	// open ive device
	mau_fd= open("/dev/nvt_ddr_arb0", O_RDWR);
	if (mau_fd < 0) {
		printf("[VENDOR_MAU] Open fail!\n");
		return HD_ERR_NG;
	}

	mon_info.ch = ch;
	mon_info.rw = 0;
	mon_info.dram = dram;

	ret = ioctl(mau_fd, DDR_IOC_MON_STOP, &mon_info);
	if (ret < 0) {
		printf("[VENDOR_MAU] mon stop fail!\n");
		return ret;
	}

	if (mau_fd != -1) {
		close(mau_fd);
	}

	return mon_info.count;
}
