/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)，All Rights Reserved.
*
* File Name: focaltech_test_ft8716.c
*
* Author: Software Development
*
* Created: 2016-08-01
*
* Abstract: test item for FT8716
*
************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "../include/focaltech_test_detail_threshold.h"
#include "../include/focaltech_test_supported_ic.h"
#include "../include/focaltech_test_main.h"
#include "../focaltech_test_config.h"


#if (FTS_CHIP_TEST_TYPE ==FT8736_TEST)

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/

/////////////////////////////////////////////////Reg FT8736
#define DEVIDE_MODE_ADDR    0x00
#define REG_LINE_NUM    0x01
#define REG_TX_NUM  0x02
#define REG_RX_NUM  0x03
#define FT8736_LEFT_KEY_REG    0X1E
#define FT8736_RIGHT_KEY_REG   0X1F

#define REG_CbAddrH         0x18
#define REG_CbAddrL         0x19
#define REG_OrderAddrH      0x1A
#define REG_OrderAddrL      0x1B

#define REG_RawBuf0         0x6A
#define REG_RawBuf1         0x6B
#define REG_OrderBuf0       0x6C
#define REG_CbBuf0          0x6E

#define REG_K1Delay         0x31
#define REG_K2Delay         0x32
#define REG_SCChannelCf     0x34

#define pre                                 1
#define REG_CLB                        0x04


/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

enum NOISE_TYPE
{
    NT_AvgData = 0,
    NT_MaxData = 1,
    NT_MaxDevication = 2,
    NT_DifferData = 3,
};

/*******************************************************************************
* Static variables
*******************************************************************************/

static int m_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_CBData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static BYTE m_ucTempData[TX_NUM_MAX * RX_NUM_MAX*2] = {0};//One-dimensional
static int m_iTempRawData[TX_NUM_MAX * RX_NUM_MAX] = {0};


/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
extern struct stCfg_FT8736_BasicThreshold g_stCfg_FT8736_BasicThreshold;

/*******************************************************************************
* Static function prototypes
*******************************************************************************/

/////////////////////////////////////////////////////////////
static int StartScan(void);
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer);
static unsigned char GetPanelRows(unsigned char *pPanelRows);
static unsigned char GetPanelCols(unsigned char *pPanelCols);
static unsigned char GetTxRxCB(unsigned short StartNodeNo, unsigned short ReadNum, unsigned char *pReadBuffer);
static unsigned char GetRawData(void);
static unsigned char GetChannelNum(void);
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount);
static unsigned char ChipClb(unsigned char *pClbResult);



boolean FT8736_StartTest(void);
int FT8736_get_test_data(char *pTestData);//pTestData, External application for memory, buff size >= 1024*80

unsigned char FT8736_TestItem_EnterFactoryMode(void);
unsigned char FT8736_TestItem_RawDataTest(bool * bTestResult);
unsigned char FT8736_TestItem_CbTest(bool* bTestResult);


/************************************************************************
* Name: FT8736_StartTest
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
boolean FT8736_StartTest()
{
    bool bTestResult = true, bTempResult = 1;
    unsigned char ReCode;
    unsigned char ucDevice = 0;
    int iItemCount=0;


    //--------------1. Init part
    if (InitTest() < 0)
    {
        FTS_TEST_ERROR("[focal] Failed to init test.");
        return false;
    }

    //--------------2. test item
    if (0 == g_TestItemNum)
        bTestResult = false;

    ////////Testing process, the order of the g_stTestItem structure of the test items
    for (iItemCount = 0; iItemCount < g_TestItemNum; iItemCount++)
    {
        m_ucTestItemCode = g_stTestItem[ucDevice][iItemCount].ItemCode;

        ///////////////////////////////////////////////////////FT8736_ENTER_FACTORY_MODE
        if (Code_FT8736_ENTER_FACTORY_MODE == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT8736_TestItem_EnterFactoryMode();
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8736_RAWDATA_TEST

        if (Code_FT8736_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT8736_TestItem_RawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8736_CB_TEST

        if (Code_FT8736_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT8736_TestItem_CbTest(&bTempResult); //
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

    }

    //--------------3. End Part
    FinishTest();

    //--------------4. return result
    return bTestResult;

}

/************************************************************************
* Name: FT8736_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8736_TestItem_EnterFactoryMode(void)
{

    unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
    int iRedo = 5;  //If failed, repeat 5 times.
    int i ;
    SysDelay(150);
    FTS_TEST_DBG("Enter factory mode...");
    for (i = 1; i <= iRedo; i++)
    {
        ReCode = EnterFactory();
        if (ERROR_CODE_OK != ReCode)
        {
            FTS_TEST_ERROR("Failed to Enter factory mode...");
            if (i < iRedo)
            {
                SysDelay(50);
                continue;
            }
        }
        else
        {
            FTS_TEST_DBG(" success to Enter factory mode...");
            break;
        }

    }
    SysDelay(300);

    if (ReCode == ERROR_CODE_OK) //After the success of the factory model, read the number of channels
    {
        ReCode = GetChannelNum();
    }
    return ReCode;
}

/************************************************************************
* Name: FT8736_TestItem_RawDataTest
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: bTestResult
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8736_TestItem_RawDataTest(bool * bTestResult)
{
    unsigned char ReCode;
    bool btmpresult = true;
    //int iMax, iMin, iAvg;
    int RawDataMin;
    int RawDataMax;
    int iValue = 0;
    int i=0;
    int iRow, iCol;
    bool bIncludeKey = false;

    FTS_TEST_INFO("\n\n==============================Test Item: -------- Raw Data Test\n");

    bIncludeKey = g_stCfg_FT8736_BasicThreshold.bCBTest_VKey_Check;

    //----------------------------------------------------------Read RawData
    for (i = 0 ; i < 3; i++) //Lost 3 Frames, In order to obtain stable data
        ReCode = GetRawData();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to get Raw Data!! Error Code: %d",  ReCode);
        return ReCode;
    }
    //----------------------------------------------------------Show RawData


    FTS_TEST_DBG("\nVA Channels: ");
    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {
        FTS_TEST_DBG("\nCh_%02d:  ", iRow+1);
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            FTS_TEST_DBG("%5d, ", m_RawData[iRow][iCol]);
        }
    }
    FTS_TEST_DBG("\nKeys:  ");
    for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
    {
        FTS_TEST_DBG("%5d, ",  m_RawData[g_stSCapConfEx.ChannelXNum][iCol]);
    }


    //----------------------------------------------------------To Determine RawData if in Range or not
    //   VA
    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {

        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
            RawDataMin = g_stCfg_Incell_DetailThreshold.RawDataTest_Min[iRow][iCol];
            RawDataMax = g_stCfg_Incell_DetailThreshold.RawDataTest_Max[iRow][iCol];
            iValue = m_RawData[iRow][iCol];
            //FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
            if (iValue < RawDataMin || iValue > RawDataMax)
            {
                btmpresult = false;
                FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                               iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
            }
        }
    }

    // Key
    if (bIncludeKey)
    {
        iRow = g_stSCapConfEx.ChannelXNum;
        for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
        {
            if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
            RawDataMin = g_stCfg_Incell_DetailThreshold.RawDataTest_Min[iRow][iCol];
            RawDataMax = g_stCfg_Incell_DetailThreshold.RawDataTest_Max[iRow][iCol];
            iValue = m_RawData[iRow][iCol];
            if (iValue < RawDataMin || iValue > RawDataMax)
            {
                btmpresult = false;
                FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                               iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
            }
        }
    }

    //////////////////////////////Save Test Data
    Save_Test_Data(m_RawData, 0, g_stSCapConfEx.ChannelXNum+1, g_stSCapConfEx.ChannelYNum, 1);

    TestResultLen += sprintf(TestResult+TestResultLen,"RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    //----------------------------------------------------------Return Result
    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_INFO("\n\n//RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//RawData Test is NG!");
    }
    return ReCode;
}

/************************************************************************
* Name: FT8736_TestItem_CbTest
* Brief:  TestItem: Cb Test. Check if Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8736_TestItem_CbTest(bool* bTestResult)
{
    bool btmpresult = true;
    unsigned char ReCode = ERROR_CODE_OK;
    int iRow = 0;
    int iCol = 0;
    int iMaxValue = 0;
    int iMinValue = 0;
    int iValue = 0;
    bool bIncludeKey = false;

    unsigned char bClbResult = 0;
    unsigned char ucBits = 0;
    int ReadKeyLen = g_stSCapConfEx.KeyNumTotal;

    bIncludeKey = g_stCfg_FT8736_BasicThreshold.bCBTest_VKey_Check;

    FTS_TEST_INFO("\n\n==============================Test Item: --------  CB Test\n");

    ReCode =  EnterFactory();
    SysDelay(50);
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_ERROR("\r\n//=========  Enter Factory Failed!");
    }

    //auto clb
    ReCode = ChipClb( &bClbResult );
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_ERROR("\r\n//========= auto clb Failed!");
    }

    ReCode = ReadReg(0x0B, &ucBits);
    if (ERROR_CODE_OK != ReCode)
    {
        btmpresult = false;
        FTS_TEST_ERROR("\r\n//=========  Read Reg Failed!");
    }

    ReadKeyLen = g_stSCapConfEx.KeyNumTotal;
    if (ucBits != 0)
    {

        ReadKeyLen = g_stSCapConfEx.KeyNumTotal * 2;
    }

    ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum  + ReadKeyLen), m_ucTempData );
    if ( ERROR_CODE_OK != ReCode )
    {
        btmpresult = false;
        FTS_TEST_ERROR("Failed to get CB value...");
        goto TEST_ERR;
    }

    memset(m_CBData, 0, sizeof(m_CBData));
    ///VA area
    for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            m_CBData[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
        }
    }


    for ( iCol = 0; iCol < ReadKeyLen/*g_stSCapConfEx.KeyNumTotal*/; ++iCol )
    {
        if (ucBits != 0)
        {
            m_CBData[g_stSCapConfEx.ChannelXNum][iCol/2] = (short)((m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol ] & 0x01 )<<8) + m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol + 1 ];
            iCol++;
        }
        else
        {
            m_CBData[g_stSCapConfEx.ChannelXNum][iCol] = m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol ];
        }

    }



    //------------------------------------------------Show CbData


    FTS_TEST_DBG("\nVA Channels: ");
    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {
        FTS_TEST_DBG("\nCh_%02d:  ", iRow+1);
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            FTS_TEST_DBG("%3d, ", m_CBData[iRow][iCol]);
        }
    }
    FTS_TEST_DBG("\nKeys:  ");
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
        {
            FTS_TEST_DBG("%3d, ",  m_CBData[g_stSCapConfEx.ChannelXNum][iCol]);

        }

    }



    // VA
    for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; iRow++)
    {
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            if ( (0 == g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol]) )
            {
                continue;
            }
            iMinValue = g_stCfg_Incell_DetailThreshold.CBTest_Min[iRow][iCol];
            iMaxValue = g_stCfg_Incell_DetailThreshold.CBTest_Max[iRow][iCol];
            iValue = focal_abs(m_CBData[iRow][iCol]);
            // FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
            if ( iValue < iMinValue || iValue > iMaxValue)
            {
                btmpresult = false;
                FTS_TEST_ERROR("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                               iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
            }
        }
    }

    // Key
    if (bIncludeKey)
    {

        iRow = g_stSCapConfEx.ChannelXNum;

        for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++)
        {
            if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)
            {
                continue; //Invalid Node
            }
            iMinValue = g_stCfg_Incell_DetailThreshold.CBTest_Min[iRow][iCol];
            iMaxValue = g_stCfg_Incell_DetailThreshold.CBTest_Max[iRow][iCol];
            iValue = focal_abs(m_CBData[iRow][iCol]);
            // FTS_TEST_DBG("Node1=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
            if (iValue < iMinValue || iValue > iMaxValue)
            {
                btmpresult = false;
                FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",  \
                               iRow+1, iCol+1, iValue, iMinValue, iMaxValue);
            }
        }

    }



    //////////////////////////////Save Test Data
    Save_Test_Data(m_CBData, 0, g_stSCapConfEx.ChannelXNum+1, g_stSCapConfEx.ChannelYNum, 1);

    TestResultLen += sprintf(TestResult+TestResultLen,"CB Test is %s. \n\n", (btmpresult ? "OK" : "NG"));


    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_INFO("\n\n//CB Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//CB Test is NG!");
    }

    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//CB Test is NG!");

    TestResultLen += sprintf(TestResult+TestResultLen,"CB Test is NG. \n\n");

    return ReCode;
}



/************************************************************************
* Name: FT8736_get_test_data
* Brief:  get data of test result
* Input: none
* Output: pTestData, the returned buff
* Return: the length of test data. if length > 0, got data;else ERR.
***********************************************************************/
int FT8736_get_test_data(char *pTestData)
{
    if (NULL == pTestData)
    {
        FTS_TEST_ERROR(" pTestData == NULL ");
        return -1;
    }
    memcpy(pTestData, g_pStoreAllData, (g_lenStoreMsgArea+g_lenStoreDataArea));
    return (g_lenStoreMsgArea+g_lenStoreDataArea);
}

/************************************************************************
* Name: Save_Test_Data
* Brief:  Storage format of test data
* Input: int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount
* Output: none
* Return: none
***********************************************************************/
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount)
{
    int iLen = 0;
    int i = 0, j = 0;

    //Save  Msg (ItemCode is enough, ItemName is not necessary, so set it to "NA".)
    iLen= sprintf(g_pTmpBuff,"NA, %d, %d, %d, %d, %d, ", \
                  m_ucTestItemCode, Row, Col, m_iStartLine, ItemCount);
    memcpy(g_pMsgAreaLine2+g_lenMsgAreaLine2, g_pTmpBuff, iLen);
    g_lenMsgAreaLine2 += iLen;

    m_iStartLine += Row;
    m_iTestDataCount++;

    //Save Data
    for (i = 0+iArrayIndex; i < Row+iArrayIndex; i++)
    {
        for (j = 0; j < Col; j++)
        {
            if (j == (Col -1)) //The Last Data of the Row, add "\n"
                iLen= sprintf(g_pTmpBuff,"%d, \n ",  iData[i][j]);
            else
                iLen= sprintf(g_pTmpBuff,"%d, ", iData[i][j]);

            memcpy(g_pStoreDataArea+g_lenStoreDataArea, g_pTmpBuff, iLen);
            g_lenStoreDataArea += iLen;
        }
    }

}

////////////////////////////////////////////////////////////
/************************************************************************
* Name: StartScan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int StartScan(void)
{
    unsigned char RegVal = 0x00;
    unsigned char times = 0;
    const unsigned char MaxTimes = 20;  //The longest wait 160ms
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;

    //if(hDevice == NULL)       return ERROR_CODE_NO_DEVICE;

    ReCode = ReadReg(DEVIDE_MODE_ADDR,&RegVal);
    if (ReCode == ERROR_CODE_OK)
    {
        RegVal |= 0x80;     //Top bit position 1, start scan
        ReCode = WriteReg(DEVIDE_MODE_ADDR,RegVal);
        if (ReCode == ERROR_CODE_OK)
        {
            while (times++ < MaxTimes)      //Wait for the scan to complete
            {
                SysDelay(8);    //8ms
                ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
                if (ReCode == ERROR_CODE_OK)
                {
                    if ((RegVal>>7) == 0)    break;
                }
                else
                {
                    break;
                }
            }
            if (times < MaxTimes)    ReCode = ERROR_CODE_OK;
            else ReCode = ERROR_CODE_COMM_ERROR;
        }
    }
    return ReCode;

}
/************************************************************************
* Name: ReadRawData(Same function name as FT_MultipleTest)
* Brief:  read Raw Data
* Input: Freq(No longer used, reserved), LineNum, ByteNum
* Output: pRevBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer)
{
    unsigned char ReCode=ERROR_CODE_COMM_ERROR;
    unsigned char I2C_wBuffer[3] = {0};
    unsigned char pReadData[ByteNum];
    int i, iReadNum;
    unsigned short BytesNumInTestMode1=0;

    iReadNum=ByteNum/BYTES_PER_TIME;

    if (0 != (ByteNum%BYTES_PER_TIME)) iReadNum++;

    if (ByteNum <= BYTES_PER_TIME)
    {
        BytesNumInTestMode1 = ByteNum;
    }
    else
    {
        BytesNumInTestMode1 = BYTES_PER_TIME;
    }

    ReCode = WriteReg(REG_LINE_NUM, LineNum);//Set row addr;


    //***********************************************************Read raw data in test mode1
    I2C_wBuffer[0] = REG_RawBuf0;   //set begin address
    if (ReCode == ERROR_CODE_OK)
    {
        focal_msleep(10);
        ReCode = Comm_Base_IIC_IO(I2C_wBuffer, 1, pReadData, BytesNumInTestMode1);
    }

    for (i=1; i<iReadNum; i++)
    {
        if (ReCode != ERROR_CODE_OK) break;

        if (i==iReadNum-1) //last packet
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, pReadData+BYTES_PER_TIME*i, ByteNum-BYTES_PER_TIME*i);
        }
        else
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, pReadData+BYTES_PER_TIME*i, BYTES_PER_TIME);
        }

    }

    if (ReCode == ERROR_CODE_OK)
    {
        for (i=0; i<(ByteNum>>1); i++)
        {
            pRevBuffer[i] = (pReadData[i<<1]<<8)+pReadData[(i<<1)+1];

        }
    }


    return ReCode;

}
/************************************************************************
* Name: GetTxRxCB(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx/Rx
* Input: StartNodeNo, ReadNum
* Output: pReadBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetTxRxCB(unsigned short StartNodeNo, unsigned short ReadNum, unsigned char *pReadBuffer)
{
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned short usReturnNum = 0;//Number to return in every time
    unsigned short usTotalReturnNum = 0;//Total return number
    unsigned char wBuffer[4];
    int i, iReadNum;

    iReadNum = ReadNum/BYTES_PER_TIME;

    if (0 != (ReadNum%BYTES_PER_TIME)) iReadNum++;

    wBuffer[0] = REG_CbBuf0;

    usTotalReturnNum = 0;

    for (i = 1; i <= iReadNum; i++)
    {
        if (i*BYTES_PER_TIME > ReadNum)
            usReturnNum = ReadNum - (i-1)*BYTES_PER_TIME;
        else
            usReturnNum = BYTES_PER_TIME;

        wBuffer[1] = (StartNodeNo+usTotalReturnNum) >>8;//Address offset high 8 bit
        wBuffer[2] = (StartNodeNo+usTotalReturnNum)&0xff;//Address offset low 8 bit

        ReCode = WriteReg(REG_CbAddrH, wBuffer[1]);
        ReCode = WriteReg(REG_CbAddrL, wBuffer[2]);
        ReCode = Comm_Base_IIC_IO(wBuffer, 1, pReadBuffer+usTotalReturnNum, usReturnNum);

        usTotalReturnNum += usReturnNum;

        if (ReCode != ERROR_CODE_OK)return ReCode;

        //if(ReCode < 0) return ReCode;
    }

    return ReCode;
}

//***********************************************
//获取PanelRows
//***********************************************
static unsigned char GetPanelRows(unsigned char *pPanelRows)
{
    return ReadReg(REG_TX_NUM, pPanelRows);
}

//***********************************************
//获取PanelCols
//***********************************************
static unsigned char GetPanelCols(unsigned char *pPanelCols)
{
    return ReadReg(REG_RX_NUM, pPanelCols);
}
/************************************************************************
* Name: GetChannelNum
* Brief:  Get Num of Ch_X, Ch_Y and key
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNum(void)
{
    unsigned char ReCode;
    //int TxNum, RxNum;
    int i ;
    unsigned char rBuffer[1]; //= new unsigned char;

    //FTS_TEST_DBG("Enter GetChannelNum...");
    //--------------------------------------------"Get Channel X Num...";
    for (i = 0; i < 3; i++)
    {
        ReCode = GetPanelRows(rBuffer);
        if (ReCode == ERROR_CODE_OK)
        {
            if (0 < rBuffer[0] && rBuffer[0] < 80)
            {
                g_stSCapConfEx.ChannelXNum = rBuffer[0];
                if (g_stSCapConfEx.ChannelXNum > g_ScreenSetParam.iUsedMaxTxNum)
                {
                    FTS_TEST_ERROR("Failed to get Channel X number, Get num = %d, UsedMaxNum = %d",
                                   g_stSCapConfEx.ChannelXNum, g_ScreenSetParam.iUsedMaxTxNum);
                    g_stSCapConfEx.ChannelXNum = 0;
                    return ERROR_CODE_INVALID_PARAM;
                }
                break;
            }
            else
            {
                SysDelay(150);
                continue;
            }
        }
        else
        {
            FTS_TEST_ERROR("Failed to get Channel X number");
            SysDelay(150);
        }
    }

    //--------------------------------------------"Get Channel Y Num...";
    for (i = 0; i < 3; i++)
    {
        ReCode = GetPanelCols(rBuffer);
        if (ReCode == ERROR_CODE_OK)
        {
            if (0 < rBuffer[0] && rBuffer[0] < 80)
            {
                g_stSCapConfEx.ChannelYNum = rBuffer[0];
                if (g_stSCapConfEx.ChannelYNum > g_ScreenSetParam.iUsedMaxRxNum)
                {
                    FTS_TEST_ERROR("Failed to get Channel Y number, Get num = %d, UsedMaxNum = %d",
                                   g_stSCapConfEx.ChannelYNum, g_ScreenSetParam.iUsedMaxRxNum);
                    g_stSCapConfEx.ChannelYNum = 0;
                    return ERROR_CODE_INVALID_PARAM;
                }
                break;
            }
            else
            {
                SysDelay(150);
                continue;
            }
        }
        else
        {
            FTS_TEST_ERROR("Failed to get Channel Y number");
            SysDelay(150);
        }
    }

    //--------------------------------------------"Get Key Num...";
    for (i = 0; i < 3; i++)
    {
        unsigned char regData = 0;
        g_stSCapConfEx.KeyNum = 0;
        ReCode = ReadReg( FT8736_LEFT_KEY_REG, &regData );
        if (ReCode == ERROR_CODE_OK)
        {
            if (((regData >> 0) & 0x01))
            {
                g_stSCapConfEx.bLeftKey1 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if (((regData >> 1) & 0x01))
            {
                g_stSCapConfEx.bLeftKey2 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if (((regData >> 2) & 0x01))
            {
                g_stSCapConfEx.bLeftKey3 = true;
                ++g_stSCapConfEx.KeyNum;
            }
        }
        else
        {
            FTS_TEST_ERROR("Failed to get Key number");
            SysDelay(150);
            continue;
        }
        ReCode = ReadReg( FT8736_RIGHT_KEY_REG, &regData );
        if (ReCode == ERROR_CODE_OK)
        {
            if (((regData >> 0) & 0x01))
            {
                g_stSCapConfEx.bRightKey1 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if (((regData >> 1) & 0x01))
            {
                g_stSCapConfEx.bRightKey2 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if (((regData >> 2) & 0x01))
            {
                g_stSCapConfEx.bRightKey3 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            break;
        }
        else
        {
            FTS_TEST_ERROR("Failed to get Key number");
            SysDelay(150);
            continue;
        }
    }

    //g_stSCapConfEx.KeyNumTotal = g_stSCapConfEx.KeyNum;

    FTS_TEST_DBG("CH_X = %d, CH_Y = %d, Key = %d",  g_stSCapConfEx.ChannelXNum ,g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum );
    return ReCode;
}
/************************************************************************
* Name: GetRawData
* Brief:  Get Raw Data of VA area and Key area
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetRawData(void)
{
    int ReCode = ERROR_CODE_OK;
    int iRow, iCol;

    //--------------------------------------------Enter Factory Mode
    ReCode = EnterFactory();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to Enter Factory Mode...");
        return ReCode;
    }


    //--------------------------------------------Check Num of Channel
    if (0 == (g_stSCapConfEx.ChannelXNum + g_stSCapConfEx.ChannelYNum))
    {
        ReCode = GetChannelNum();
        if ( ERROR_CODE_OK != ReCode )
        {
            FTS_TEST_ERROR("Error Channel Num...");
            return ERROR_CODE_INVALID_PARAM;
        }
    }

    //--------------------------------------------Start Scanning
    //FTS_TEST_DBG("Start Scan ...");
    ReCode = StartScan();
    if (ERROR_CODE_OK != ReCode)
    {
        FTS_TEST_ERROR("Failed to Scan ...");
        return ReCode;
    }


    //--------------------------------------------Read RawData for Channel Area
    //FTS_TEST_DBG("Read RawData...");
    memset(m_RawData, 0, sizeof(m_RawData));
    memset(m_iTempRawData, 0, sizeof(m_iTempRawData));
    ReCode = ReadRawData(0, 0xAD, g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum * 2, m_iTempRawData);
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to Get RawData");
        return ReCode;
    }

    for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow)
    {
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol)
        {
            m_RawData[iRow][iCol] = m_iTempRawData[iRow * g_stSCapConfEx.ChannelYNum + iCol];
        }
    }

    //--------------------------------------------Read RawData for Key Area
    memset(m_iTempRawData, 0, sizeof(m_iTempRawData));
    ReCode = ReadRawData( 0, 0xAE, g_stSCapConfEx.KeyNumTotal * 2, m_iTempRawData );
    if (ERROR_CODE_OK != ReCode)
    {
        FTS_TEST_ERROR("Failed to Get RawData");
        return ReCode;
    }

    for (iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; ++iCol)
    {
        m_RawData[g_stSCapConfEx.ChannelXNum][iCol] = m_iTempRawData[iCol];
    }

    return ReCode;

}

static unsigned char ChipClb(unsigned char *pClbResult)
{
    unsigned char RegData=0;
    unsigned char TimeOutTimes = 50;        //5s
    unsigned char ReCode = ERROR_CODE_OK;

    ReCode = WriteReg(REG_CLB, 4);  //start auto clb

    if (ReCode == ERROR_CODE_OK)
    {
        while (TimeOutTimes--)
        {
            SysDelay(100);  //delay 500ms
            ReCode = WriteReg(DEVIDE_MODE_ADDR, 0x04<<4);
            ReCode = ReadReg(0x04, &RegData);
            if (ReCode == ERROR_CODE_OK)
            {
                if (RegData == 0x02)
                {
                    *pClbResult = 1;
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if (TimeOutTimes == 0)
        {
            *pClbResult = 0;
        }
    }
    return ReCode;
}
#endif

