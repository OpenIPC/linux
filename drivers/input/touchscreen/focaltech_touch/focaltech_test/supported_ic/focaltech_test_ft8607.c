/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)��All Rights Reserved.
*
* File Name: Focaltech_test_ft8607.c
*
* Author: Software Development Team, AE
*
* Created: 2016-08-01
*
* Abstract: test item for FT8607
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "../include/focaltech_test_detail_threshold.h"
#include "../include/focaltech_test_supported_ic.h"
#include "../focaltech_test_config.h"

#if (FTS_CHIP_TEST_TYPE ==FT8607_TEST)

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/


#define DEVIDE_MODE_ADDR                0x00

// copy from 8606
#define REG_LINE_NUM                    0x01
#define REG_TX_NUM                      0x02
#define REG_RX_NUM                      0x03

#define REG_RawBuf0                     0x6A
#define REG_RawBuf1                     0x6B
#define REG_OrderBuf0                   0x6C
#define REG_CbBuf0                      0x6E

#define REG_CbAddrH                     0x18
#define REG_CbAddrL                     0x19
#define REG_OrderAddrH                  0x1A
#define REG_OrderAddrL                  0x1B

#define pre 1

//  }} from 8606


#define SHEILD_NODE                     -1
#define MAX_CAP_GROUP_OF_8607        4
#define FT_8607_CONFIG_START_ADDR    0x7a80

#define FT_8607_LEFT_KEY_REG             0X1E
#define FT_8607_RIGHT_KEY_REG        0X1F

#define REG_8607_LCD_NOISE_FRAME     0X12
#define REG_8607_LCD_NOISE_START         0X11
#define REG_8607_LCD_NOISE_NUMBER    0X13

#define REG_CLB                          0x04

#define OUTPUT_MAXMIN


/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
enum
{
    MODE_FREERUN,
    MODE_MONITOR,
    MODE_ACTIVE,
    MODE_COMPENSATION
};

enum NOISE_TYPE
{
    NT_AvgData = 0,
    NT_MaxData = 1,
    NT_MaxDevication = 2,
    NT_DifferData = 3,
};

/*****************************************************************************
* Static variables
*****************************************************************************/

static int m_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static int m_CBData[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static BYTE m_ucTempData[TX_NUM_MAX * RX_NUM_MAX*2] = {0};//One-dimensional
static int m_iTempRawData[TX_NUM_MAX * RX_NUM_MAX] = {0};


/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/

extern struct stCfg_FT8607_BasicThreshold g_stCfg_FT8607_BasicThreshold;

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
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



boolean FT8607_StartTest(void);
int FT8607_get_test_data(char *pTestData);//pTestData, External application for memory, buff size >= 1024*80


unsigned char FT8607_TestItem_EnterFactoryMode(void);
unsigned char FT8607_TestItem_RawDataTest(bool * bTestResult);
unsigned char FT8607_TestItem_CbTest(bool * bTestResult);

/************************************************************************
* Name: _StartTest
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/

boolean FT8607_StartTest()
{
    bool bTestResult = true, bTempResult = 1;
    unsigned char ReCode;
    unsigned char ucDevice = 0;
    int iItemCount=0;


    FTS_TEST_FUNC_ENTER();


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

        ///////////////////////////////////////////////////////FT8607_ENTER_FACTORY_MODE
        if (Code_FT8607_ENTER_FACTORY_MODE == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {


            ReCode = FT8607_TestItem_EnterFactoryMode();
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT8607_RAWDATA_TEST
        if (Code_FT8607_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {


            ReCode = FT8607_TestItem_RawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }


        ///////////////////////////////////////////////////////FT8607_CB_TEST
        if (Code_FT8607_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {


            ReCode = FT8607_TestItem_CbTest(&bTempResult); //
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
* Name: FT8607_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8607_TestItem_EnterFactoryMode(void)

{
    unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
    int iRedo = 5;          //If failed, repeat 5 times.
    int i ;
    SysDelay(150);
    FTS_TEST_DBG("Enter factory mode...\n");
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

    if (ReCode == ERROR_CODE_OK)        //After the success of the factory model, read the number of channels
    {
        ReCode = GetChannelNum();

    }
    return ReCode;
}


unsigned char FT8607_TestItem_RawDataTest(bool * bTestResult)
{
    unsigned char ReCode = ERROR_CODE_OK;
    bool btmpresult = true;
    //int iMax, iMin, iAvg;
    int RawDataMin = 0;
    int RawDataMax = 0;
    int iValue = 0;
    int i=0;
    int iRow = 0, iCol = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -------- Raw Data Test\n\n");

    for (i = 0 ; i < 3; i++) //Lost 3 Frames, In order to obtain stable data
        ReCode = GetRawData();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to get Raw Data!! Error Code: %d\n", ReCode);
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
    FTS_TEST_DBG("\n");

    //----------------------------------------------------------To Determine RawData if in Range or not

    // VA
    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {

        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
            RawDataMin = g_stCfg_Incell_DetailThreshold.RawDataTest_Min[iRow][iCol];
            RawDataMax = g_stCfg_Incell_DetailThreshold.RawDataTest_Max[iRow][iCol];
            iValue = m_RawData[iRow][iCol];

            // FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
            if (iValue < RawDataMin || iValue > RawDataMax)
            {
                btmpresult = false;

                FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
                               iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);

            }
        }
    }

    // Key
    iRow = g_stSCapConfEx.ChannelXNum;
    for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
    {
        if (g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
        RawDataMin = g_stCfg_Incell_DetailThreshold.RawDataTest_Min[iRow][iCol];
        RawDataMax = g_stCfg_Incell_DetailThreshold.RawDataTest_Max[iRow][iCol];
        iValue = m_RawData[iRow][iCol];

        // FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
        if (iValue < RawDataMin || iValue > RawDataMax)
        {
            btmpresult = false;

            FTS_TEST_ERROR("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
                           iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);

        }
    }

    //////////////////////////////Save Test Data
    Save_Test_Data(m_RawData, 0, g_stSCapConfEx.ChannelXNum+1, g_stSCapConfEx.ChannelYNum, 1);
    //----------------------------------------------------------Return Result

    TestResultLen += sprintf(TestResult+TestResultLen," RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_INFO("\n\n//RawData Test is OK!\n");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//RawData Test is NG!\n");
    }
    return ReCode;
}


/************************************************************************
* Name: FT8607_TestItem_CbTest
* Brief:  TestItem: Cb Test. Check if Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8607_TestItem_CbTest(bool * bTestResult)
{
    bool btmpresult = true;
    unsigned char ReCode = ERROR_CODE_OK;
    int iRow = 0;
    int iCol = 0;
//  int readlen = g_stSCapConf.ChannelsNum + g_stSCapConf.KeyNum;
    int iMaxValue = 0;
    int iMinValue = 0;
    unsigned char bClbResult = 0;
    int i=0;
    bool bIncludeKey = false;

    bIncludeKey = g_stCfg_FT8607_BasicThreshold.bCBTest_VKey_Check;

    FTS_TEST_INFO("\n\n==============================Test Item: --------  CB Test\n\n");


    FT8607_TestItem_EnterFactoryMode();

    for ( i=0; i<10; i++)
    {
        FTS_TEST_DBG(" start chipclb times:%d. ", i);

        ReCode = ChipClb( &bClbResult );
        SysDelay(50);
        if ( 0 != bClbResult)
        {
            break;
        }
    }
    if ( false == bClbResult)
    {
        FTS_TEST_ERROR(" ReCalib Failed.");
        btmpresult = false;
    }

//  ReCode = GetTxRxCB( 0, (UINT16)g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum + g_stSCapConfEx.KeyNumTotal, chCBData );
    ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum + g_stSCapConfEx.KeyNumTotal), m_ucTempData );

    if ( ERROR_CODE_OK != ReCode )
    {
        btmpresult = false;
        FTS_TEST_ERROR("Failed to get CB value...\n");
        goto TEST_ERR;
    }

    memset(m_CBData, 0, sizeof(m_CBData));

    for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
    {
        for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
        {
            m_CBData[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
        }
    }

    ///key
    for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; ++iCol )
    {
        m_CBData[g_stSCapConfEx.ChannelXNum][iCol] = m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol ];
    }

    //------------------------------------------------Show CbData
    FTS_TEST_DBG("\nVA Channels:  \n");
    for (iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
    {
        FTS_TEST_DBG("\nCh_%02d:  ", iRow+1);
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
        {
            FTS_TEST_DBG("%3d, ", m_CBData[iRow][iCol]);
        }
    }

    FTS_TEST_DBG("\nKeys:  \n");
    for ( iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
    {
        FTS_TEST_DBG("%3d, ",  m_CBData[g_stSCapConfEx.ChannelXNum][iCol]);
    }
    FTS_TEST_DBG("\n");



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

            //  FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, focal_abs(m_CBData[iRow][iCol]), iMinValue, iMaxValue);

            if (focal_abs(m_CBData[iRow][iCol]) < iMinValue || focal_abs(m_CBData[iRow][iCol]) > iMaxValue)
            {
                btmpresult = false;
                FTS_TEST_ERROR("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",
                               iRow+1, iCol+1, m_CBData[iRow][iCol], iMinValue, iMaxValue);
            }
        }
    }

    // Key
    if (bIncludeKey)
    {
        iRow = g_stSCapConfEx.ChannelXNum;
        for (iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; iCol++ )
        {
            if ( (0 == g_stCfg_Incell_DetailThreshold.InvalidNode[iRow][iCol]) )
            {
                continue;
            }

            iMinValue = g_stCfg_Incell_DetailThreshold.CBTest_Min[iRow][iCol];
            iMaxValue = g_stCfg_Incell_DetailThreshold.CBTest_Max[iRow][iCol];

            // FTS_TEST_DBG("Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",  iRow+1, iCol+1, focal_abs(m_CBData[iRow][iCol]), iMinValue, iMaxValue);
            if (focal_abs(m_CBData[iRow][iCol]) < iMinValue || focal_abs(m_CBData[iRow][iCol]) > iMaxValue)
            {
                btmpresult = false;
                FTS_TEST_ERROR("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n",
                               iRow+1, iCol+1, m_CBData[iRow][iCol], iMinValue, iMaxValue);
            }


        }
    }


    //////////////////////////////Save Test Data
    Save_Test_Data(m_CBData, 0, g_stSCapConfEx.ChannelXNum+1, g_stSCapConfEx.ChannelYNum, 1);

    TestResultLen += sprintf(TestResult+TestResultLen," CB Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if (btmpresult)
    {
        * bTestResult = true;
        FTS_TEST_INFO("\n\n//CB Test is OK!\n");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//CB Test is NG!\n");
    }

    return ReCode;
TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//CB Test is NG!\n");

    TestResultLen += sprintf(TestResult+TestResultLen," CB Test is NG. \n\n");
    return ReCode;

}



/************************************************************************
* Name: FT8607_get_test_data
* Brief:  get data of test result
* Input: none
* Output: pTestData, the returned buff
* Return: the length of test data. if length > 0, got data;else ERR.
***********************************************************************/
int FT8607_get_test_data(char *pTestData)
{
    FTS_TEST_FUNC_ENTER();

    if (NULL == pTestData)
    {
        FTS_TEST_ERROR("pTestData == NULL \n");
        return -1;
    }
    memcpy(pTestData, g_pStoreAllData, (g_lenStoreMsgArea+g_lenStoreDataArea));
    return (g_lenStoreMsgArea+g_lenStoreDataArea);
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

    FTS_TEST_FUNC_ENTER();

    memset(pReadData, 0, sizeof(pReadData));
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

    ReCode = WriteReg(REG_LINE_NUM, LineNum);//Set row addr;    //  set  0x01 register's value to 0xAD to clear 0 of FM

    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to write REG_LINE_NUM! ");
        goto READ_ERR;
    }

    //***********************************************************Read raw data in test mode1
    I2C_wBuffer[0] = REG_RawBuf0;   //set begin address
    if (ReCode == ERROR_CODE_OK)
    {
        focal_msleep(10);
        ReCode = Comm_Base_IIC_IO(I2C_wBuffer, 1, pReadData, BytesNumInTestMode1);
        if (ReCode != ERROR_CODE_OK)
        {
            FTS_TEST_ERROR("read rawdata Comm_Base_IIC_IO Failed!1 ");
            goto READ_ERR;
        }
    }

    for (i=1; i<iReadNum; i++)
    {
        if (ReCode != ERROR_CODE_OK) break;

        if (i==iReadNum-1) //last packet
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, pReadData+BYTES_PER_TIME*i, ByteNum-BYTES_PER_TIME*i);
            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_ERROR("read rawdata Comm_Base_IIC_IO Failed!2 ");
                goto READ_ERR;
            }
        }
        else
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, pReadData+BYTES_PER_TIME*i, BYTES_PER_TIME);
            if (ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_ERROR("read rawdata Comm_Base_IIC_IO Failed!3 ");
                goto READ_ERR;
            }
        }

    }

//  FTS_TEST_DBG("data:  ");
    if (ReCode == ERROR_CODE_OK)
    {
        for (i=0; i<(ByteNum>>1); i++)
        {
            pRevBuffer[i] = (pReadData[i<<1]<<8)+pReadData[(i<<1)+1];
            //if(pRevBuffer[i] & 0x8000) // The sign bit
            //{
            //  pRevBuffer[i] -= 0xffff + 1;
            //}
            //  FTS_TEST_PRINT("%d,  ", pRevBuffer[i]);
        }
    }
//  FTS_TEST_PRINT("\n");

    FTS_TEST_FUNC_EXIT();

READ_ERR:
    return ReCode;

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

    FTS_TEST_FUNC_ENTER();

    //Save  Msg (ItemCode is enough, ItemName is not necessary, so set it to "NA".)
    iLen= sprintf(g_pTmpBuff,"NA, %d, %d, %d, %d, %d, ", \
                  m_ucTestItemCode, Row, Col, m_iStartLine, ItemCount);
    memcpy(g_pMsgAreaLine2+g_lenMsgAreaLine2, g_pTmpBuff, iLen);
    g_lenMsgAreaLine2 += iLen;

    m_iStartLine += Row;
    m_iTestDataCount++;

    //Save Data
    for (i = 0+iArrayIndex; (i < Row+iArrayIndex) && (i < TX_NUM_MAX); i++)
    {
        for (j = 0; (j < Col) && (j < RX_NUM_MAX); j++)
        {
            if (j == (Col -1)) //The Last Data of the Row, add "\n"
                iLen= sprintf(g_pTmpBuff,"%d, \n", iData[i][j]);
            else
                iLen= sprintf(g_pTmpBuff,"%d, ", iData[i][j]);

            memcpy(g_pStoreDataArea+g_lenStoreDataArea, g_pTmpBuff, iLen);
            g_lenStoreDataArea += iLen;
        }
    }
}
/************************************************************************
* Name: StartScan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int StartScan(void)
{
    unsigned char RegVal = 0;
    unsigned char times = 0;
    const unsigned char MaxTimes = 250;
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;

    FTS_TEST_FUNC_ENTER();

    ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
    if (ReCode == ERROR_CODE_OK)
    {
        RegVal |= 0x80;     //Top bit position 1, start scan
        ReCode = WriteReg(DEVIDE_MODE_ADDR, RegVal);
        if (ReCode == ERROR_CODE_OK)
        {
            while (times++ < MaxTimes)      //Wait for the scan to complete
            {
                SysDelay(16);
                ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
                if (ReCode == ERROR_CODE_OK)
                {
                    if ((RegVal>>7) == 0)    break;
                }
                else
                {
                    FTS_TEST_ERROR("StartScan read DEVIDE_MODE_ADDR error.\n");
                    break;
                }
            }
            if (times < MaxTimes)    ReCode = ERROR_CODE_OK;
            else ReCode = ERROR_CODE_COMM_ERROR;
        }
        else
            FTS_TEST_ERROR("StartScan write DEVIDE_MODE_ADDR error.\n");
    }
    else
        FTS_TEST_ERROR("StartScan read DEVIDE_MODE_ADDR error.\n");
    return ReCode;

}

/************************************************************************
* Name: ShowRawData
* Brief:  Show RawData
* Input: none
* Output: none
* Return: none.
***********************************************************************/
/*static void ShowRawData(void)
{
    int iRow, iCol;
    //----------------------------------------------------------Show RawData
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        FTS_TEST_DBG("Tx%2d:  ", iRow+1);
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            FTS_TEST_PRINT("%5d    ", m_RawData[iRow][iCol]);
        }
        FTS_TEST_PRINT("\n ");
    }
}*/

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
    unsigned char wBuffer[4] = {0};
    int i = 0, iReadNum = 0;

    FTS_TEST_FUNC_ENTER();

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
        //ReCode = fts_i2c_read_test(wBuffer, 1, pReadBuffer+usTotalReturnNum, usReturnNum);
        ReCode = Comm_Base_IIC_IO(wBuffer, 1, pReadBuffer+usTotalReturnNum, usReturnNum);

        usTotalReturnNum += usReturnNum;

        if (ReCode != ERROR_CODE_OK)return ReCode;

        //if(ReCode < 0) return ReCode;
    }

    return ReCode;
}

//***********************************************
// Get PanelRows
//***********************************************
static unsigned char GetPanelRows(unsigned char *pPanelRows)
{
    return ReadReg(REG_TX_NUM, pPanelRows);
}

//***********************************************
//Get PanelCols
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

    //FTS_TEST_DBG("Enter GetChannelNum...\n");
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
                    FTS_TEST_ERROR("Failed to get Channel X number, Get num = %d, UsedMaxNum = %d\n",
                                   g_stSCapConfEx.ChannelXNum, g_ScreenSetParam.iUsedMaxTxNum);
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
            FTS_TEST_ERROR("Failed to get Channel X number\n");
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
                    FTS_TEST_ERROR("Failed to get Channel Y number, Get num = %d, UsedMaxNum = %d\n",
                                   g_stSCapConfEx.ChannelYNum, g_ScreenSetParam.iUsedMaxRxNum);
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
            FTS_TEST_ERROR("Failed to get Channel Y number\n");
            SysDelay(150);
        }
    }

    //--------------------------------------------"Get Key Num...";
    for (i = 0; i < 3; i++)
    {
        unsigned char regData = 0;
        g_stSCapConfEx.KeyNum = 0;
        ReCode = ReadReg( FT_8607_LEFT_KEY_REG, &regData );
        if (ReCode == ERROR_CODE_OK)
        {
            if ( ( (regData >> 0 ) & 0x01 ) )
            {
                g_stSCapConfEx.bLeftKey1 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if ( ( (regData >> 1 ) & 0x01 ) )
            {
                g_stSCapConfEx.bLeftKey2 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if ( ( (regData >> 2 ) & 0x01 ) )
            {
                g_stSCapConfEx.bLeftKey3 = true;
                ++g_stSCapConfEx.KeyNum;
            }
        }
        else
        {
            FTS_TEST_ERROR("Failed to get Key number\n");
            SysDelay(150);
            continue;
        }
        ReCode = ReadReg( FT_8607_RIGHT_KEY_REG, &regData );
        if (ReCode == ERROR_CODE_OK)
        {
            if ( ( (regData >> 0 ) & 0x01 ) )
            {
                g_stSCapConfEx.bRightKey1 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if ( ( (regData >> 1 ) & 0x01 ) )
            {
                g_stSCapConfEx.bRightKey2 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            if ( ( (regData >> 2 ) & 0x01 ) )
            {
                g_stSCapConfEx.bRightKey3 = true;
                ++g_stSCapConfEx.KeyNum;
            }
            break;
        }
        else
        {
            FTS_TEST_ERROR("Failed to get Key number\n");
            SysDelay(150);
            continue;
        }
    }


    FTS_TEST_INFO("CH_X = %d, CH_Y = %d, Key = %d\n", g_stSCapConfEx.ChannelXNum ,g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum );
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
    int iRow = 0, iCol = 0;

    //--------------------------------------------Enter Factory Mode
    ReCode = EnterFactory();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to Enter Factory Mode...\n");
        return ReCode;
    }

    //--------------------------------------------Check Num of Channel
    if (0 == (g_stSCapConfEx.ChannelXNum + g_stSCapConfEx.ChannelYNum))
    {
        ReCode = GetChannelNum();
        if ( ERROR_CODE_OK != ReCode )
        {
            FTS_TEST_ERROR("Error Channel Num...\n");
            return ERROR_CODE_INVALID_PARAM;
        }
    }
    //--------------------------------------------Start Scanning
    //FTS_TEST_DBG("Start Scan ...\n");
    ReCode = StartScan();
    if (ERROR_CODE_OK != ReCode)
    {
        FTS_TEST_ERROR("Failed to Scan ...\n");
        return ReCode;
    }

    //--------------------------------------------Read RawData for Channel Area
    //FTS_TEST_DBG("Read RawData...\n");
    memset(m_RawData, 0, sizeof(m_RawData));
    memset(m_iTempRawData, 0, sizeof(m_iTempRawData));
    ReCode = ReadRawData(0, 0xAD, g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum * 2, m_iTempRawData);
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to Get RawData of channel.\n");
        return ReCode;
    }
    for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow)
    {
        for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol)
        {
            m_RawData[iRow][iCol] = m_iTempRawData[iRow * g_stSCapConfEx.ChannelYNum + iCol];
        }
    }

    memset(m_iTempRawData, 0, sizeof(m_iTempRawData));
    ReCode = ReadRawData( 0, 0xAE, g_stSCapConfEx.KeyNumTotal* 2, m_iTempRawData );
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to Get RawData of keys.\n");
        return ReCode;
    }

    for (iCol = 0; iCol < g_stSCapConfEx.KeyNumTotal; ++iCol)
    {
        m_RawData[g_stSCapConfEx.ChannelXNum][iCol] = m_iTempRawData[iCol];
    }
    //m_strCurrentTestMsg = "Finish Get RawData...";

    return ReCode;
}

//Auto clb
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
