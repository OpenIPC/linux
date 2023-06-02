#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "audio_api.h"
#include "audio_drv.h"
#include "audio_dbg.h"
#include "audio_int.h"
#include "Audio.h"
#include "dai.h"
#include "eac.h"
#include "kwrap/type.h"
#include <mach/fmem.h>

#if (NVT_AUDIO_TEST_CMD == 1)

#if 0
PAUDTS_OBJ                     tx_obj,rx_obj;
AUDIO_SETTING           gAudioSetting;
static AUDIO_CODEC_FUNC gExtCodecFunc = {0};
extern void             audcodec_get_fp(PAUDIO_CODEC_FUNC pAudCodecFunc);
extern void             aud_printSetting(void);

static UINT32           gAudioDMADoneNumTx=0,gAudioDMADoneNumRx=0;
static UINT32           gAudioTimehitNumTx=0,gAudioTimehitNumRx;
static UINT32           gAudioTriggerValTx=0,gAudioTriggerValRx;
static UINT32           gAudioIsrTimeHitNumTx=0,gAudioIsrTimeHitNumRx;


const short sin_TABLE[48] = {
        0, 4277, 8481,12539,
    16383,19947,23170,25996,
    28377,30273,31650,32487,
    32767,32487,31650,30273,
    28377,25996,23170,19947,
    16383,12539, 8481, 4277,
        0,-4277,-8481,-12539,
   -16383,-19947,-23170,-25996,
   -28377,-30273,-31650,-32487,
   -32767,-32487,-31650,-30273,
   -28377,-25996,-23170,-19947,
   -16384,-12539,-8481,-4277

    };

static void InitAudio(void)
{
	AUDIO_SETTING       AudioSetting;
	AUDIO_DEVICE_OBJ    AudioDevice;
	int ret = 0;

	// The following settings are "Don't care" for NT96650 built-in audio codec.
	//AudioDevice.pEventHandler       = NULL;
	AudioDevice.uiGPIOColdReset     = 0;
	AudioDevice.uiI2SCtrl           = AUDIO_I2SCTRL_GPIO_I2C;
	AudioDevice.uiChannel           = 0;
	AudioDevice.uiGPIOData          = 64;
	AudioDevice.uiGPIOClk           = 66;
	AudioDevice.uiGPIOCS            = 0;
	AudioDevice.uiADCZero           = 0;


	AudioSetting.Clock.bClkExt      = FALSE;                        // Must be
	AudioSetting.I2S.bMaster        = TRUE;                         // Must be
	AudioSetting.I2S.I2SFmt         = AUDIO_I2SFMT_STANDARD;        // Must be
	AudioSetting.I2S.ClkRatio       = AUDIO_I2SCLKR_256FS_32BIT;    // Must be
	AudioSetting.Fmt                = AUDIO_FMT_I2S;                // Must be
	AudioSetting.Clock.Clk          = AUDIO_CLK_12288;              // Clock = Sampling rate * 256
	AudioSetting.SamplingRate       = AUDIO_SR_48000;               // Sampling rate = Clock / 256
	AudioSetting.Channel            = AUDIO_CH_STEREO;              // Audio channel
	AudioSetting.RecSrc             = AUDIO_RECSRC_MIC;             // Must be
	AudioSetting.Output             = AUDIO_OUTPUT_SPK;             // Output source
	AudioSetting.bEmbedded          = TRUE;                         // Must be. (Don't care in audio lib)


    audcodec_get_fp(&gExtCodecFunc);
    aud_set_extcodec(&gExtCodecFunc);


	aud_set_device_object(&AudioDevice);

	// Open audio driver
	aud_close();
    audio_msg("open audio driver\r\n");
	ret = aud_open();
	if(ret != 0) {
		return;
	}

	// Get AUDIO Transceive Obj
    audio_msg("Init Audio TS OBJ..\r\n");
    tx_obj = aud_get_transceive_object(AUDTS_CH_TX1);
    rx_obj = aud_get_transceive_object(AUDTS_CH_RX);


	// Init Audio driver
	memcpy(&gAudioSetting, &AudioSetting, sizeof(gAudioSetting));
	aud_init(&AudioSetting);

    audio_msg("aud_set_default_setting(AUDIO_DEFSET_20DB)\r\n");
	aud_set_default_setting(AUDIO_DEFSET_30DB);

}

void AudioTestEventTxISR(UINT32 AudioEventStatus)
{
    audio_msg("AudioTestEventTxISR : AudioEventStatus = [%d]\r\n",AudioEventStatus);
    if (AudioEventStatus & AUDIO_EVENT_DMADONE)
    {
        gAudioDMADoneNumTx++;
        audio_msg("Tx gDMADoneNum =%d\r\n", gAudioDMADoneNumTx);
    }
    if (AudioEventStatus & AUDIO_EVENT_TCHIT)
    {
        UINT32 uiAddr = dai_get_tx_dma_curaddr(tx_obj->AudTSCH);

        gAudioTimehitNumTx++;

        audio_msg("Tx time code hit =%d\r\n", gAudioTriggerValTx);
        audio_msg("Tx DMA curr adr = 0x%x\r\n", uiAddr);
        gAudioTriggerValTx += gAudioIsrTimeHitNumTx;
        tx_obj->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, gAudioTriggerValTx);
    }

    if (AudioEventStatus & AUDIO_EVENT_TCLATCH)
    {
        audio_msg("Tx time code latch =%d\r\n", tx_obj->getConfig(AUDTS_CFG_ID_TIMECODE_VALUE));
    }
    if (AudioEventStatus & AUDIO_EVENT_BUF_FULL)
    {
        audio_msg("Tx AUD FIFO Full\r\n");
    }
    if (AudioEventStatus & AUDIO_EVENT_FIFO_ERROR)
    {
        audio_msg("Tx AUD FIFO Error\r\n");
    }
    if (AudioEventStatus & AUDIO_EVENT_BUF_EMPTY)
    {
        tx_obj->stop();
        audio_msg("Tx AUD FIFO Empty\r\n");
    }

}

void AudioTestEventRxISR(UINT32 AudioEventStatus)
{
    audio_msg("AudioTestEventRxISR : AudioEventStatus = [%d]\r\n",AudioEventStatus);
    if (AudioEventStatus & AUDIO_EVENT_DMADONE)
    {
        gAudioDMADoneNumRx++;
        audio_msg("Rx gDMADoneNum =%d\r\n", gAudioDMADoneNumRx);
    }

    if (AudioEventStatus & AUDIO_EVENT_TCHIT)
    {
        UINT32 uiAddr = dai_get_rx_dma_curaddr(0);

        gAudioTimehitNumRx++;

        audio_msg("Rx time code hit =%d\r\n", gAudioTriggerValRx);
        audio_msg("Rx DMA curr adr = 0x%x\r\n", uiAddr);
        gAudioTriggerValRx += gAudioIsrTimeHitNumRx;
        rx_obj->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, gAudioTriggerValRx);
    }

    if (AudioEventStatus & AUDIO_EVENT_TCLATCH)
    {
        audio_msg("^RRx time code latch =%d\r\n", rx_obj->getConfig(AUDTS_CFG_ID_TIMECODE_VALUE));
    }
    if (AudioEventStatus & AUDIO_EVENT_BUF_FULL)
    {
        rx_obj->stop();
        audio_msg("Rx AUD FIFO Full\r\n");
    }
    if (AudioEventStatus & AUDIO_EVENT_FIFO_ERROR)
    {
        audio_msg("Rx AUD FIFO Error\r\n");
    }
}

#endif

int nvt_audio_api_write_playpattern(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
#if 0
    DAI_CH              DaiTx1Ch = DAI_CH_STEREO;
    AUDIO_SR            sampleRate = AUDIO_SR_48000;
    AUDIO_BUF_QUEUE     AudioBufQueueTx,AudioBufQueueRx;
    AUDIO_BUF_QUEUE     *rxbufq;
    UINT32              uiBufSizeTx1,uiBufSizeRx;
    UINT32              uiPreDMADoneNumTx,uiPreDMADoneNumRx;
    UINT32              loop,uiBufNum,uiNextBufId;
    UINT32              vBufAddr[5];
    UINT16              i,*temp_addr;
    //CHAR                txfilename[50] = "//mnt//sd//48KSTEO.PCM";
    frammap_buf_t       BufTx = {0};



    uiBufNum = 5;


    BufTx.size = 0x300000;
    BufTx.align = 64;      ///< address alignment
    BufTx.name = "nvtmpp";
    BufTx.alloc_type = ALLOC_CACHEABLE;
    frm_get_buf_ddr(DDR_ID0, &BufTx);


    //audio_msg("test BufTx.phyaddr = %x BufTx.va_addr = %x\r\n",BufTx.phy_addr,(UINT32)BufTx.va_addr);


    temp_addr = (UINT16 *)BufTx.va_addr;

	if (temp_addr == NULL) {
		return 0;
	}

    for(i = 0; i<48000;i++){
                *(temp_addr+2*i) = sin_TABLE[i%48];
                *(temp_addr+2*i+1) = sin_TABLE[i%48];
    }
    for(i = 0; i<6;i++){
            memcpy(temp_addr+48000*2*i, temp_addr, 48000*2*2);
            //audio_msg("================[%x]==================\r\n",temp_addr+48128*2*i);
    }





    audio_msg("nvt_audio_api_write_playpattern argc :%d\r\n", argc);

    audio_msg("================audio open==================\r\n");
    audio_msg("============================================\r\n");
    InitAudio();

    uiBufSizeTx1 = 1 << DAI_DRAMPCM_16;
    if (DaiTx1Ch == DAI_CH_STEREO)
    {
        uiBufSizeTx1 <<= 1;
    }
    uiBufSizeTx1 *= sampleRate;
    uiBufSizeTx1 = (uiBufSizeTx1+0xFFF) & ~0xFFF;

    uiBufSizeRx = 1 << DAI_DRAMPCM_16;
    if (DaiTx1Ch == DAI_CH_STEREO)
    {
        uiBufSizeRx <<= 1;
    }
    uiBufSizeRx *= sampleRate;
    uiBufSizeRx = (uiBufSizeRx+0xFFF) & ~0xFFF;



    temp_addr = (UINT16 *)BufTx.va_addr + uiBufSizeTx1*uiBufNum*2;


    for(i = 0; i<48000;i++){
                *(temp_addr+2*i) = sin_TABLE[i%48];
                *(temp_addr+2*i+1) = sin_TABLE[i%48];
    }
    for(i = 0; i<6;i++){
            memcpy(temp_addr+48000*2*i, temp_addr, 48000*2*2);
            //audio_msg("================[%x]==================\r\n",temp_addr+48128*2*i);
    }
/*
    for (i=0;i < 480;i++){


        audio_msg("rx before addr / data [%x] = [%8x][%8x]\r\n",(UINT16)(temp_addr+2*i),*(temp_addr+2*i),*(temp_addr+2*i+1));
    }

*/

    //
    //  Playback Config
    //

    audio_msg("================Playback Config=============\r\n");
    audio_msg("============================================\r\n");
    tx_obj->open();
    tx_obj->setConfig(AUDTS_CFG_ID_EVENT_HANDLE, (UINT32)AudioTestEventTxISR);
    tx_obj->setConfig(AUDTS_CFG_ID_TIMECODE_OFFSET, 0);

    gAudioTriggerValTx  = sampleRate;
    gAudioIsrTimeHitNumTx = gAudioTriggerValTx;

    tx_obj->setChannel(AUDIO_CH_STEREO);

    dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_PCMLEN,  DAI_DRAMPCM_16);
    dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL, DaiTx1Ch);
	dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);



    //
    //  Record Config
    //


    audio_msg("================Record Config=============\r\n");
    audio_msg("============================================\r\n");
    rx_obj->open();
    rx_obj->setConfig(AUDTS_CFG_ID_EVENT_HANDLE, (UINT32)AudioTestEventRxISR);
    rx_obj->setConfig(AUDTS_CFG_ID_TIMECODE_OFFSET, 0);

    gAudioTriggerValRx   = sampleRate;
    gAudioIsrTimeHitNumRx  = gAudioTriggerValRx;

    rx_obj->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, gAudioTriggerValRx);

    // Always set I2S Interface as STEREO and Config I2S to receive L/R Only.
    rx_obj->setChannel(AUDIO_CH_STEREO);

    dai_set_rx_config(DAI_RXCFG_ID_PCMLEN,    DAI_DRAMPCM_16);
    dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,   DaiTx1Ch);
	dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);




    //
    //  Playback Buffer
    //
    audio_msg("================Playback Buffer=============\r\n");
    audio_msg("============================================\r\n");
    tx_obj->setSamplingRate(sampleRate);
    tx_obj->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, gAudioTriggerValTx);
    tx_obj->resetBufferQueue(0);

    AudioBufQueueTx.uiAddress   = (UINT32)BufTx.va_addr;
    AudioBufQueueTx.uiSize      = uiBufSizeTx1;
    vBufAddr[0] = AudioBufQueueTx.uiAddress;
    //emu_msg(("TBUF0=0x%08X\r\n",vBufAddr[0]));
    loop = 1;
    while ((tx_obj->isBufferQueueFull(0) == FALSE) && (loop < uiBufNum))
    {
         // Add buffer to queue
         if( tx_obj->addBufferToQueue(0, &AudioBufQueueTx) == FALSE )
         {
             audio_msg("aud_addBufferToQueue Tx failed\n");
         }
         // Advance to next data buffer
         AudioBufQueueTx.uiAddress = AudioBufQueueTx.uiAddress+ uiBufSizeTx1;
         vBufAddr[loop++] = AudioBufQueueTx.uiAddress;
         audio_msg("TBUF%d=0x%08X\r\n",loop-1,vBufAddr[loop-1]);
    }
    uiNextBufId         = 0;
    gAudioDMADoneNumTx    = 0;
    gAudioTimehitNumTx    = 0;
    uiPreDMADoneNumTx   = gAudioDMADoneNumTx;


    //
    //  Record Buffer
    //
    rx_obj->setSamplingRate(sampleRate);
    rx_obj->resetBufferQueue(0);

    audio_msg("================Record Buffer=============\r\n");
    audio_msg("============================================\r\n");

    AudioBufQueueRx.uiAddress   = (UINT32)BufTx.va_addr + uiBufSizeTx1*uiBufNum;
    AudioBufQueueRx.uiSize      = uiBufSizeRx;
    audio_msg("Rx Buf addr = %x\r\n",AudioBufQueueRx.uiAddress);
    i = 1;
    while ((rx_obj->isBufferQueueFull(0) == FALSE) && (i < uiBufNum))
    {
         // Add buffer to queue
         if( rx_obj->addBufferToQueue(0, &AudioBufQueueRx) == FALSE )
         {
             audio_msg("aud_addBufferToQueue Rx failed\n");
         }
         // Advance to next data buffer
         AudioBufQueueRx.uiAddress = AudioBufQueueRx.uiAddress + uiBufSizeRx;
         audio_msg("Rx Buf addr = %x\r\n",AudioBufQueueRx.uiAddress);
         i++;
    }

    gAudioDMADoneNumRx    = 0;
    gAudioTimehitNumRx    = 0;
    uiPreDMADoneNumRx   = gAudioDMADoneNumRx;




    aud_set_output(AUDIO_OUTPUT_SPK);
    aud_set_volume(AUDIO_VOL_63);
    aud_set_gain(AUDIO_GAIN_7);

    tx_obj->setFeature(AUDTS_FEATURE_TIMECODE_HIT,    FALSE);
    tx_obj->playback();

    Delay_DelayMs(100);
    rx_obj->setFeature(AUDTS_FEATURE_TIMECODE_HIT,    FALSE);
    rx_obj->record();
    while(1){



        if (uiPreDMADoneNumTx != gAudioDMADoneNumTx){

                uiPreDMADoneNumTx = gAudioDMADoneNumTx;
                //audio_msg("================ in loop %d  =================\r\n",gAudioDMADoneNumTx);
                AudioBufQueueTx.uiAddress   = vBufAddr[uiNextBufId];
                AudioBufQueueTx.uiSize      = uiBufSizeTx1;
                audio_msg("================ vBufAddr[%d] = %x;  =================\r\n",uiNextBufId,AudioBufQueueTx.uiAddress);
                uiNextBufId = (uiNextBufId + 1) % uiBufNum;
                tx_obj->addBufferToQueue(0, &AudioBufQueueTx);
        }

        if (uiPreDMADoneNumRx != gAudioDMADoneNumRx){
                uiPreDMADoneNumRx = gAudioDMADoneNumRx;
                rxbufq = rx_obj->getDoneBufferFromQueue(0);
                rx_obj->addBufferToQueue(0,rxbufq);

                audio_msg("in Rx while loop %d [%x]\r\n",gAudioDMADoneNumRx,dai_get_rx_dma_curaddr(0));
                audio_msg("rx data = = = = = [%d]\r\n",*(UINT32 *)rxbufq->uiAddress);
        }

        if(gAudioDMADoneNumTx == 5){
                tx_obj->stop();
                rx_obj->stop();
                tx_obj->close();
                rx_obj->close();
                break;
        }

    }
/*
    temp_addr = (UINT16 *)rxbufq->uiAddress;



    for (i=0;i < 480;i++){


        audio_msg("rx after addr / data [%x] = [%8x][%8x]\r\n",(UINT16)(temp_addr+2*i),*(temp_addr+2*i),*(temp_addr+2*i+1));
    }

*/

    frm_free_buf_ddr(BufTx.va_addr);

    audio_msg("================audio close=================\r\n");





#endif
return 0;
}

int nvt_audio_api_write_reg(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
    /*
	unsigned long reg_addr, reg_value;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul (pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
 		return -EINVAL;

	}

	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_xxx_drv_write_reg(pmodule_info, reg_addr, reg_value);
	*/
	return 0;
}

int nvt_audio_api_write_pattern(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{
    /*
	mm_segment_t old_fs;
	struct file *fp;
	int len = 0;
	unsigned char* pbuffer;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	fp = filp_open(pargv[0], O_RDONLY , 0);
	if (IS_ERR_OR_NULL(fp)) {
	    nvt_dbg(ERR, "failed in file open:%s\n", pargv[0]);
		return -EFAULT;
	}

	pbuffer = kmalloc(256, GFP_KERNEL);
	if (pbuffer == NULL) {
		filp_close(fp, NULL);
		return -ENOMEM;
	}

	old_fs = get_fs();
	set_fs(get_ds());

	len = vfs_read(fp, pbuffer, 256, &fp->f_pos);

	// Do something after get data from file

	filp_close(fp, NULL);
	set_fs(old_fs);

	return len;
	*/
	return 0;

}

int nvt_audio_api_read_reg(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{/*
	unsigned long reg_addr;
	unsigned long value;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	nvt_dbg(IND, "R REG 0x%lx\n", reg_addr);
	value = nvt_xxx_drv_read_reg(pmodule_info, reg_addr);

	nvt_dbg(ERR, "REG 0x%lx = 0x%lx\n", reg_addr, value);*/
	return 0;
}

int nvt_audio_api_dumpinfo(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{

    aud_printSetting();


    return 0;
}

int nvt_audio_api_dumpinfo_obj(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{


	aud_printSetting_obj_releated();

    return 0;
}


int nvt_audio_api_enable_dai_dbg(PMODULE_INFO pmodule_info, unsigned char argc, char** pargv)
{

    dai_dbg_msg_enable();
    return 0;
}



#endif
