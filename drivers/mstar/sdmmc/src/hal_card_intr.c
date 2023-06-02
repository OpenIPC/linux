
/***************************************************************************************************************
 *
 * FileName hal_card_intr.c
 *     @author jeremy.wang (2011/07/21)
 * Desc:
 *     The Interrupt behavior of all cards will run here.
 *     The goal is that we don't need to change HAL Level code (But its h file code)
 *
 *     The limitations were listed as below:
 *     (1) This c file belongs to HAL level.
 *     (2) Its h file is included by driver API or HAL level, not driver flow process.
 *     (3) MIE Event Int and Card Change Event Int function belong to here.
 *     (4) FCIE/SDIO IP interrupt register function belong to here.
 *     (5) Because ISR belongs to OS design, so we must use OS define option to separate them.
 *     (6) This c file could not use project/cpu/icver/specific define option here, but its h file could.
 *
 *     P.S. EN_XX for ON/OFF Define, V_XX  for Value Define,
 *          RT_XX for Retry Times Define, WT_XX for Wait Time Define, M_XX for Mask Value Define
 *
 ***************************************************************************************************************/

#include "../inc/hal_card_intr.h"

//***********************************************************************************************************
// Config Setting (Internel)
//***********************************************************************************************************

// Enable Setting
//-----------------------------------------------------------------------------------------------------------
#define EN_TRFUNC         (TRUE)

// Value Setting
//-----------------------------------------------------------------------------------------------------------
#define V_MAXSLOTS          2


//***********************************************************************************************************


// Reg Static Init Setting
//-----------------------------------------------------------------------------------------------------------
//#define V_SD_MIE_INT_INIT   (R_SD_CMD_END_EN|R_SD_DATA_END_EN|R_CARD_DMA_END_EN|R_MMA_LSDONE_END_EN)

// Mask Range
//-----------------------------------------------------------------------------------------------------------
#define M_SD_DCRCSTS        (R_SD_DAT_CERR|R_SD_DAT_STSERR|R_SD_DAT_STSNEG)

#define M_CARD_INT_EN       (R_MMA_DATA_END_EN|R_SD_CMD_END_EN|R_SD_DATA_END_EN|R_MS_DATA_END_EN| \
                             R_CF_CMD_END_EN|R_CF_DATA_END_EN|R_SM_JOB_END_EN|R_CARD_DMA_END_EN|R_MMA_LSDONE_END_EN)

// Mask Reg Value
//-----------------------------------------------------------------------------------------------------------
#define M_REG_DCRCERR(IP)              (CARD_REG(A_SD_STS_REG(IP)) & M_SD_DCRCSTS)  //0x1F


//IP_FCIE or IP_SDIO Register Basic Address
//-----------------------------------------------------------------------------------------------------------
#define A_SD_REG_POS(IP)                GET_CARD_BANK(IP, 0)

#define A_MIE_EVENT_REG(IP)             GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x00)
#define A_MIE_INT_EN_REG(IP)            GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x01)

#define A_CARD_EVENT_REG(IP)            GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x05)
#define A_CARD_INT_EN_REG(IP)           GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x06)
#define A_SD_STS_REG(IP)                GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x12)
#define A_EMMC_PATH_CTL_REG(IP)         GET_CARD_REG_ADDR(A_SD_REG_POS(IP), 0x16)


// Reg Dynamic Variable
//-----------------------------------------------------------------------------------------------------------
static volatile U16_T   gu16_MIEINT_Mode[3] = {0};
static volatile U16_T   gu16_MIEEvent_ForInt[3] = {0};   //MIEEvent for Interrupt
static volatile U16_T   gu16_MIEIntEN_ForSDIO[3] = {0};
static volatile BOOL_T  gb_StopWaitMIE[3] = {0};

// Trace Funcion
//-----------------------------------------------------------------------------------------------------------
#if (EN_TRFUNC)
    #define TR_INT(p)           p
#else
    #define TR_INT(p)
#endif

// Register Operation Define ==> For Return SD Change Status Pos
//-----------------------------------------------------------------------------------------------------------
static U16_T _REG_SDCHG(IPEmType eIP)
{
    PortEmType ePort = GET_CARD_PORT(eIP);

    switch( (U8_T)ePort )
    {
        case (U8_T)EV_PORT_SDIO1:
            return R_SDIO_STS_CHG;
        case (U8_T)EV_PORT_SDIO2:
            return R_SDIO2_STS_CHG;
        case (U8_T) EV_PORT_SD:
            return R_SD_STS_CHG;
        default:
            return 0;
    }
}

// Register Operation Define ==> For Return SD Change Status Int Pos
//-----------------------------------------------------------------------------------------------------------
static U16_T _REG_SDCHG_INTEN(IPEmType eIP)
{
    PortEmType ePort = GET_CARD_PORT(eIP);

    switch( (U8_T)ePort )
    {
        case (U8_T)EV_PORT_SDIO1:
            return R_SDIO_STS_EN;
        case (U8_T)EV_PORT_SDIO2:
            return R_SDIO2_STS_EN;
        case (U8_T) EV_PORT_SD:
            return R_SD_STS_EN;
        default:
            return 0;
    }
}


BOOL_T _CARD_INT_SaveMIEEvent(IPEmType eIP)
{
    U16_T u16Reg = CARD_REG(A_MIE_EVENT_REG(eIP));

    if(CARD_REG(A_EMMC_PATH_CTL_REG(eIP)) & R_EMMC_EN) //Emmc Event
        return (FALSE);

    gu16_MIEEvent_ForInt[eIP] |= u16Reg;  //Summary All 0x00Reg Event

    u16Reg &= CARD_REG(A_MIE_INT_EN_REG(eIP));

    /****** Clean Card MIE Event *******/
    CARD_REG(A_MIE_EVENT_REG(eIP)) = (M_CARD_INT_EN & u16Reg);

#if 0 // For Debug whether event clear or not
    prtstring("Debug: [MIE_EVENT_REG]=       ");
    prtU16Hex(CARD_REG(A_MIE_EVENT_REG(eIP)));
    prtstring("\r\n");

    while(1)
    {
    }
#endif



    return (TRUE);
}


static BOOL_T _CARD_INT_DetectSDIOInt(IPEmType eIP)
{
    U16_T u16Reg = CARD_REG(A_MIE_EVENT_REG(eIP));

    if( (u16Reg & R_SDIO_INT) && (gu16_MIEIntEN_ForSDIO[eIP] == R_SDIO_INT_EN) )
    {
        //printk("_M[%04X]_", CARD_REG(A_MIE_EVENT_REG(eIP)));
        CARD_REG(A_MIE_EVENT_REG(eIP)) = R_SDIO_INT;    //Clear SDIO_EVENT to avoid trig int again

        /****** Fix IP Bug for DAT1 CRC error, sdio will trig interrupt ******/
        if(!M_REG_DCRCERR(eIP))
            return (TRUE);
        else
            return (FALSE);
    }

    return (FALSE);

}


BOOL_T _CARD_INT_DetectCDZEvent(IPEmType eIP, IntCardEmType eCardInt)
{
    U16_T u16CardEvent = 0, u16CardInt = 0;
    BOOL_T bRet = FALSE;


    u16CardEvent = CARD_REG(A_CARD_EVENT_REG(eIP));
    u16CardInt   = CARD_REG(A_CARD_INT_EN_REG(eIP));

    if((u16CardEvent==0) || (u16CardInt==0)) // No Card Int Event
        return FALSE;

    TR_INT(prtstring("\n>> @@@@@ DetectCH...CINT("));
    TR_INT(prtU16Hex(u16CardInt));
    TR_INT(prtstring("),CENT("));
    TR_INT(prtU16Hex(u16CardEvent));
    TR_INT(prtstring(")\n"));

    if(eCardInt == EV_INT_SD)
    {
        if( (u16CardEvent & _REG_SDCHG(eIP)) && (u16CardInt & _REG_SDCHG_INTEN(eIP)) )
        {
            CARD_REG_CLRBIT(A_CARD_INT_EN_REG(eIP), _REG_SDCHG_INTEN(eIP));    //Disable SDMMC Card Change Int
            bRet = TRUE;
        }
    }

    return bRet;
}


/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_MIEIntCtrl
 *     @author jeremy.wang (2015/11/4)
 * Desc: Enable Card MIE Interrupt Mode
 *
 * @param eIP : FCIE1/FCIE2/...
 * @param eCardInt : SD/MS/CF/XD...
 * @param bEnable : Enable (TRUE) or Disable (FALSE)
 ----------------------------------------------------------------------------------------------------------*/
void Hal_CARD_INT_MIEIntCtrl(IPEmType eIP, IntCardEmType eCardInt, BOOL_T bEnable)
{
    if(bEnable)
        gu16_MIEINT_Mode[eIP] |= eCardInt;
    else
        gu16_MIEINT_Mode[eIP]  &= (~eCardInt);
}


/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_MIEIntRunning
 *     @author jeremy.wang (2011/9/7)
 * Desc: Check Int Mode Running or Not
 *
 * @param eIP :  FCIE1/FCIE2/...
 * @param eCardInt : SD/MS/CF/XD...
 *
 * @return BOOL_T  : Interrupt Running or Not
 ----------------------------------------------------------------------------------------------------------*/
BOOL_T Hal_CARD_INT_MIEIntRunning(IPEmType eIP, IntCardEmType eCardInt)
{
    if(gu16_MIEINT_Mode[eIP] & eCardInt )
        return (TRUE);
    else
        return (FALSE);
}


/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_SetMIEIntEn
 *     @author jeremy.wang (2011/12/20)
 * Desc: Set MIE_Int_En to Trig Interrupt
 *
 * @param eIP : FCIE1/FCIE2/...
 * @param eCardInt : SD/MS/CF/XD...
 * @param u16RegMIEIntEN : MIE_Int_En Reg Setting
 ----------------------------------------------------------------------------------------------------------*/
void Hal_CARD_INT_SetMIEIntEn(IPEmType eIP, IntCardEmType eCardInt, U16_T u16RegMIEIntEN)
{
    /****** Set SD MIE Int Event ******/
    if(eCardInt == EV_INT_SD)
    {
        if(gu16_MIEINT_Mode[eIP] & EV_INT_SD)
            CARD_REG(A_MIE_INT_EN_REG(eIP)) = (u16RegMIEIntEN | gu16_MIEIntEN_ForSDIO[eIP]) ;  //Enable MIE_INT_EN
        else
            CARD_REG(A_MIE_INT_EN_REG(eIP)) = 0;                //Clear MIE_INT_EN to Avoid Interrupt
    }

}


/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_SetMIEIntEn_ForSDIO
 *     @author jeremy.wang (2014/5/28)
 * Desc:
 *
 * @param eIP :
 * @param eCardInt :
 * @param bEnable :
 ----------------------------------------------------------------------------------------------------------*/
void Hal_CARD_INT_SetMIEIntEn_ForSDIO(IPEmType eIP, IntCardEmType eCardInt, BOOL_T bEnable)
{
    if(eCardInt == EV_INT_SD)
    {
        if(gu16_MIEINT_Mode[eIP] & EV_INT_SD)
        {
            if(bEnable)
            {
                /****** Specail Process: Clear SDIO Int to avoid trig Int after SDIO Int Enable ******/
                CARD_REG(A_MIE_EVENT_REG(eIP)) = R_SDIO_INT;

                gu16_MIEIntEN_ForSDIO[eIP] = R_SDIO_INT_EN;
                CARD_REG_SETBIT(A_MIE_INT_EN_REG(eIP), R_SDIO_INT_EN);
            }
            else
            {
                gu16_MIEIntEN_ForSDIO[eIP] = 0;
                CARD_REG_CLRBIT(A_MIE_INT_EN_REG(eIP), R_SDIO_INT_EN);
            }
        }
    }
}


/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_ClearMIEEvent
 *     @author jeremy.wang (2011/9/7)
 * Desc: Clear MIE Event That We Have Saved in Interrupt Mode
 *
 * @param eIP : FCIE1/FCIE2/...
 ----------------------------------------------------------------------------------------------------------*/
void Hal_CARD_INT_ClearMIEEvent(IPEmType eIP)
{
    gu16_MIEEvent_ForInt[eIP] = 0;
}


/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_GetMIEEvent
 *     @author jeremy.wang (2011/9/7)
 * Desc: Get Current MIE Event That We Have Saved in Interrupt Mode
 *
 * @param eIP : FCIE1/FCIE2/...

 * @return U16_T  : Current MIE Event
 ----------------------------------------------------------------------------------------------------------*/
U16_T Hal_CARD_INT_GetMIEEvent(IPEmType eIP)
{
    return gu16_MIEEvent_ForInt[eIP];
}


//***********************************************************************************************************
// Interrupt Handler Function
//***********************************************************************************************************

//###########################################################################################################
#if(D_OS == D_OS__LINUX)
//###########################################################################################################
static DECLARE_WAIT_QUEUE_HEAD(fcie1_mieint_wait);
static DECLARE_WAIT_QUEUE_HEAD(fcie2_mieint_wait);
static DECLARE_WAIT_QUEUE_HEAD(fcie3_mieint_wait);

#endif
//###########################################################################################################


/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_WaitMIEEvent
 *     @author jeremy.wang (2011/12/20)
 * Desc: Set WaitQueue Handler to Wait MIE Event
 *
 * @param eIP : FCIE1/FCIE2/...
 * @param u16ReqEvent : MIE Event for waiting
 * @param u32WaitMs : Waiting Time (ms)
 *
 * @return BOOL_T  : TRUE (Success), FALSE (Timeout)
 ----------------------------------------------------------------------------------------------------------*/
BOOL_T Hal_CARD_INT_WaitMIEEvent(IPEmType eIP, U16_T u16ReqEvent, U32_T u32WaitMs)
{
//###########################################################################################################
#if (D_OS == D_OS__LINUX)
//###########################################################################################################
    if(eIP==EV_IP_FCIE1)
        wait_event_timeout(fcie1_mieint_wait, ((gu16_MIEEvent_ForInt[eIP]& u16ReqEvent) == u16ReqEvent) || gb_StopWaitMIE[eIP], msecs_to_jiffies(u32WaitMs+WT_INT_RISKTIME));
    else if((eIP==EV_IP_FCIE2))
        wait_event_timeout(fcie2_mieint_wait, ((gu16_MIEEvent_ForInt[eIP]& u16ReqEvent) == u16ReqEvent) || gb_StopWaitMIE[eIP], msecs_to_jiffies(u32WaitMs+WT_INT_RISKTIME));
    else if((eIP==EV_IP_FCIE3))
        wait_event_timeout(fcie3_mieint_wait, ((gu16_MIEEvent_ForInt[eIP]& u16ReqEvent) == u16ReqEvent) || gb_StopWaitMIE[eIP], msecs_to_jiffies(u32WaitMs+WT_INT_RISKTIME));


    if((gu16_MIEEvent_ForInt[eIP] & u16ReqEvent) != u16ReqEvent)
        return FALSE;

    return TRUE;
//###########################################################################################################
#endif

    /****** Not Support (Ex: D_OS_ARCH = D_OS_UBOOT) ******/
    return FALSE;
}

/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_StopWaitMIEEventCtrl
 *     @author jeremy.wang (2011/12/20)
 * Desc: Stop Wait MIE Event to Avoid Long Time Waiting
 *
 * @param eIP : FCIE1/FCIE2/...
 * @param bEnable : Enable (TRUE) or Disable (FALSE)
 ----------------------------------------------------------------------------------------------------------*/
void Hal_CARD_INT_StopWaitMIEEventCtrl(IPEmType eIP, BOOL_T bEnable)
{
    gb_StopWaitMIE[eIP] = bEnable;

//###########################################################################################################
#if (D_OS == D_OS__LINUX)
//###########################################################################################################
    if(gb_StopWaitMIE[eIP])
    {
         /****** Break Current Wait Event *******/
        if(eIP==EV_IP_FCIE1)
            wake_up(&fcie1_mieint_wait);
        else if((eIP==EV_IP_FCIE2))
            wake_up(&fcie2_mieint_wait);
        else if((eIP==EV_IP_FCIE3))
            wake_up(&fcie3_mieint_wait);
    }
#endif
//###########################################################################################################

}


//***********************************************************************************************************
// Interrupt Function
//***********************************************************************************************************

//###########################################################################################################
#if(D_OS == D_OS__LINUX)
//###########################################################################################################
#include <linux/mmc/host.h>
#include "../inc/ms_sdmmc_lnx.h"
/*----------------------------------------------------------------------------------------------------------
 *
 * Function: Hal_CARD_INT_MIE
 *     @author jeremy.wang (2012/2/13)
 * Desc:
 *
 * @param irq :
 * @param p_dev_id :
 *
 * @return irqreturn_t  :
 ----------------------------------------------------------------------------------------------------------*/
irqreturn_t Hal_CARD_INT_MIE(int irq, void *p_dev_id)
{
    irqreturn_t irq_t = IRQ_NONE;
    IntSourceStruct* pstIntSource = p_dev_id;
    IPEmType eIP  = pstIntSource->eIP;
    //IntCardEmType eCardInt = pstIntSource->eCardInt;
    struct ms_sdmmc_slot  *p_sdmmc_slot = pstIntSource->p_data;

    if(_CARD_INT_DetectSDIOInt(eIP))
    {
        mmc_signal_sdio_irq(p_sdmmc_slot->mmc);
        irq_t = IRQ_HANDLED;
    }

    if(_CARD_INT_SaveMIEEvent(eIP))
    {
        if(eIP==EV_IP_FCIE1)
            wake_up(&fcie1_mieint_wait);
        else if((eIP==EV_IP_FCIE2))
            wake_up(&fcie2_mieint_wait);
        else if((eIP==EV_IP_FCIE3))
            wake_up(&fcie3_mieint_wait);

        irq_t = IRQ_HANDLED;
    }

    return irq_t;
}


//###########################################################################################################
#endif








