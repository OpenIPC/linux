/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)￡?All Rights Reserved.
*
* File Name: Test_FT5822.c
*
* Author: Software Development Team, AE
*
* Created: 2015-07-14
*
* Abstract: test item for FT5822\FT5626\FT5726\FT5826B
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
#include "../include/focaltech_test_main.h"
#include "../focaltech_test_config.h"

#if (FTS_CHIP_TEST_TYPE ==FT5822_TEST)

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/

/////////////////////////////////////////////////Reg
#define DEVIDE_MODE_ADDR    0x00
#define REG_LINE_NUM    0x01
#define REG_TX_NUM  0x02
#define REG_RX_NUM  0x03
#define REG_PATTERN_5422        0x53
#define REG_MAPPING_SWITCH      0x54
#define REG_TX_NOMAPPING_NUM        0x55
#define REG_RX_NOMAPPING_NUM      0x56
#define REG_NORMALIZE_TYPE      0x16
#define REG_ScCbBuf0    0x4E
#define REG_ScWorkMode  0x44
#define REG_ScCbAddrR   0x45
#define REG_RawBuf0 0x36
#define REG_WATER_CHANNEL_SELECT 0x09

#define REG_FREQUENCY           0x0A
#define REG_FIR                 0XFB


/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
enum WaterproofType
{
    WT_NeedProofOnTest,
    WT_NeedProofOffTest,
    WT_NeedTxOnVal,
    WT_NeedRxOnVal,
    WT_NeedTxOffVal,
    WT_NeedRxOffVal,
};
/*****************************************************************************
* Static variables
*****************************************************************************/

static int m_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_iTempRawData[TX_NUM_MAX * RX_NUM_MAX] = {0};
static unsigned char m_ucTempData[TX_NUM_MAX * RX_NUM_MAX*2] = {0};
static bool m_bV3TP = false;
static int m_DifferData[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
static int m_absDifferData[TX_NUM_MAX][RX_NUM_MAX] = {{0}};
//static int RxLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
//static int TxLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};




/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern struct stCfg_FT5822_BasicThreshold g_stCfg_FT5822_BasicThreshold;

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
//////////////////////////////////////////////Communication function
static int StartScan(void);
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer);
static unsigned char GetPanelRows(unsigned char *pPanelRows);
static unsigned char GetPanelCols(unsigned char *pPanelCols);
static unsigned char GetTxSC_CB(unsigned char index, unsigned char *pcbValue);
static unsigned char GetRawData(void);
static unsigned char GetChannelNum(void);
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount);
static void ShowRawData(void);
static boolean GetTestCondition(int iTestType, unsigned char ucChannelValue);

static unsigned char GetChannelNumNoMapping(void);
static unsigned char SwitchToNoMapping(void);



boolean FT5822_StartTest(void);
int FT5822_get_test_data(char *pTestData);//pTestData, External application for memory, buff size >= 1024*80

unsigned char FT5822_TestItem_EnterFactoryMode(void);
unsigned char FT5822_TestItem_RawDataTest(bool * bTestResult);
unsigned char FT5822_TestItem_SCapRawDataTest(bool* bTestResult);
unsigned char FT5822_TestItem_SCapCbTest(bool* bTestResult);
unsigned char FT5822_TestItem_PanelDifferTest(bool * bTestResult);


boolean GetWaterproofMode(int iTestType, unsigned char ucChannelValue);

/************************************************************************
* Name: FT5822_StartTest
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
boolean FT5822_StartTest()
{
    bool bTestResult = true;
    bool bTempResult = 1;
    unsigned char ReCode=0;
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

    for (iItemCount = 0; iItemCount < g_TestItemNum; iItemCount++)
    {
        m_ucTestItemCode = g_stTestItem[ucDevice][iItemCount].ItemCode;

        ///////////////////////////////////////////////////////FT5822_ENTER_FACTORY_MODE
        if (Code_FT5822_ENTER_FACTORY_MODE == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT5822_TestItem_EnterFactoryMode();
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
                break;//if this item FAIL, no longer test.
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT5822_RAWDATA_TEST
        if (Code_FT5822_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT5822_TestItem_RawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }


        ///////////////////////////////////////////////////////FT5822_SCAP_CB_TEST
        if (Code_FT5822_SCAP_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT5822_TestItem_SCapCbTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }

        ///////////////////////////////////////////////////////FT5822_SCAP_RAWDATA_TEST
        if (Code_FT5822_SCAP_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT5822_TestItem_SCapRawDataTest(&bTempResult);
            if (ERROR_CODE_OK != ReCode || (!bTempResult))
            {
                bTestResult = false;
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
            }
            else
                g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
        }
        ///////////////////////////////////////////////////////FT5822_PANELDIFFER_TEST
        if (Code_FT5822_PANELDIFFER_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
           )
        {
            ReCode = FT5822_TestItem_PanelDifferTest(&bTempResult);
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
* Name: FT5822_get_test_data
* Brief:  get data of test result
* Input: none
* Output: pTestData, the returned buff
* Return: the length of test data. if length > 0, got data;else ERR.
***********************************************************************/
int FT5822_get_test_data(char *pTestData)
{
    if (NULL == pTestData)
    {
        FTS_TEST_ERROR(" pTestData == NULL ");
        return -1;
    }
    memcpy(pTestData, g_pStoreAllData, (g_lenStoreMsgArea+g_lenStoreDataArea));
    return (g_lenStoreMsgArea+g_lenStoreDataArea);

//return sprintf(pTestData,"Hello man!");
}

/************************************************************************
* Name: FT5822_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT5822_TestItem_EnterFactoryMode(void)
{
    unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
    int iRedo = 5;//If failed, repeat 5 times.
    int i ;
    unsigned char chPattern=0;

    SysDelay(150);
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
            break;
        }

    }
    SysDelay(300);


    if (ReCode != ERROR_CODE_OK)
    {
        return ReCode;
    }

    //After the success of the factory model, read the number of channels
    ReCode = GetChannelNum();

    ////////////set FIR￡?0￡oclose￡?1￡oopen
    //theDevice.m_cHidDev[m_NumDevice]->WriteReg(0xFB, 0);

    // to determine whether the V3 screen body
    ReCode = ReadReg( REG_PATTERN_5422, &chPattern );
    if (chPattern == 1)
    {
        m_bV3TP = true;
    }
    else
    {
        m_bV3TP = false;
    }

    return ReCode;
}
/************************************************************************
* Name: FT5822_TestItem_RawDataTest
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT5822_TestItem_RawDataTest(bool * bTestResult)
{
    unsigned char ReCode = 0;
    bool btmpresult = true;
    int RawDataMin;
    int RawDataMax;
    unsigned char ucFre;
    unsigned char strSwitch = 0;
    unsigned char OriginValue = 0xff;
    int index = 0;
    int iRow, iCol;
    int iValue = 0;


    FTS_TEST_INFO("\n\n==============================Test Item: -------- Raw Data  Test \n");
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //Determine whether for v3 TP first, and then read the value of the 0x54
    //and check the mapping type is right or not,if not, write the register
    //rawdata test mapping before mapping 0x54=1;after mapping 0x54=0;
    if (m_bV3TP)
    {
        ReCode = ReadReg( REG_MAPPING_SWITCH, &strSwitch );
        if (strSwitch != 0)
        {
            ReCode = WriteReg( REG_MAPPING_SWITCH, 0 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
        }
    }

    //Line by line one after the rawdata value, the default 0X16=0
    ReCode = ReadReg( REG_NORMALIZE_TYPE, &OriginValue );// read the original value
    if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;


    if (g_ScreenSetParam.isNormalize == Auto_Normalize)
    {
        if (OriginValue != 1) //if original value is not the value needed,write the register to change
        {
            ReCode = WriteReg( REG_NORMALIZE_TYPE, 0x01 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
        }
        //Set Frequecy High

        FTS_TEST_DBG( "\n=========Set Frequecy High\n" );
        ReCode = WriteReg( 0x0A, 0x81 );
        if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;

        FTS_TEST_DBG( "\n=========FIR State: ON");
        ReCode = WriteReg(0xFB, 1);//FIR OFF  0:close, 1:open
        if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
        //change register value before,need to lose 3 frame data
        for (index = 0; index < 3; ++index )
        {
            ReCode = GetRawData();
        }

        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
            goto TEST_ERR;
        }

        ShowRawData();

        ////////////////////////////////To Determine RawData if in Range or not
        for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
        {
            for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
            {
                if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Min[iRow][iCol];
                RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Max[iRow][iCol];
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
        Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 2);
    }
    else
    {
        if (OriginValue != 0) //if original value is not the value needed,write the register to change
        {
            ReCode = WriteReg( REG_NORMALIZE_TYPE, 0x00 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
        }

        ReCode =  ReadReg( 0x0A, &ucFre );
        if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;


        //Set Frequecy Low
        if (g_stCfg_FT5822_BasicThreshold.RawDataTest_SetLowFreq)
        {
            FTS_TEST_DBG("\n=========Set Frequecy Low");
            ReCode = WriteReg( 0x0A, 0x80 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;

            //FIR OFF  0:close, 1:open

            FTS_TEST_DBG("\n=========FIR State: OFF\n" );
            ReCode = WriteReg(0xFB, 0);
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
            SysDelay(100);
            //change register value before,need to lose 3 frame data
            for (index = 0; index < 3; ++index )
            {
                ReCode = GetRawData();
            }

            if ( ReCode != ERROR_CODE_OK )
            {
                FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
                goto TEST_ERR;
            }
            ShowRawData();

            ////////////////////////////////To Determine RawData if in Range or not
            for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
            {

                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                    RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_Low_Min[iRow][iCol];
                    RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_Low_Max[iRow][iCol];
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
            Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 1);
        }


        //Set Frequecy High
        if ( g_stCfg_FT5822_BasicThreshold.RawDataTest_SetHighFreq )
        {

            FTS_TEST_DBG( "\n=========Set Frequecy High");
            ReCode = WriteReg( 0x0A, 0x81 );
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;

            //FIR OFF  0:close, 1:open

            FTS_TEST_DBG("\n=========FIR State: OFF\n" );
            ReCode = WriteReg(0xFB, 0);
            if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
            SysDelay(100);
            //change register value before,need to lose 3 frame data
            for (index = 0; index < 3; ++index )
            {
                ReCode = GetRawData();
            }

            if ( ReCode != ERROR_CODE_OK )
            {
                FTS_TEST_ERROR("\nGet Rawdata failed, Error Code: 0x%x",  ReCode);
                if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
            }
            ShowRawData();

            ////////////////////////////////To Determine RawData if in Range or not
            for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
            {

                for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
                {
                    if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node
                    RawDataMin = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Min[iRow][iCol];
                    RawDataMax = g_stCfg_MCap_DetailThreshold.RawDataTest_High_Max[iRow][iCol];
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
            Save_Test_Data(m_RawData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 2);
        }

    }



    ReCode = WriteReg( REG_NORMALIZE_TYPE, OriginValue );//set the origin value
    if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;

    //set V3 TP the origin mapping value
    if (m_bV3TP)
    {
        ReCode = WriteReg( REG_MAPPING_SWITCH, strSwitch );
        if ( ReCode != ERROR_CODE_OK )goto TEST_ERR;
    }

    TestResultLen += sprintf(TestResult+TestResultLen,"RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));
    //-------------------------Result
    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//RawData Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen,"RawData Test is NG. \n\n");
    return ReCode;

}
/************************************************************************
* Name: FT5822_TestItem_SCapRawDataTest
* Brief:  TestItem: SCapRawDataTest. Check if SCAP RawData is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT5822_TestItem_SCapRawDataTest(bool * bTestResult)
{
    int i =0;
    int RawDataMin = 0;
    int RawDataMax = 0;
    int Value = 0;
    boolean bFlag = true;
    unsigned char ReCode = 0;
    boolean btmpresult = true;
    int iMax=0;
    int iMin=0;
    int iAvg=0;
    int ByteNum=0;
    unsigned char wc_value = 0;//waterproof channel value
    unsigned char ucValue = 0;
    int iCount = 0;
//   int ibiggerValue = 0;

    FTS_TEST_INFO("\n\n==============================Test Item: -------- Scap RawData Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = ReadReg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //If it is V3 pattern, Get Tx/Rx Num again
    ReCode= SwitchToNoMapping();
    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to Scan SCap RawData! ");
        goto TEST_ERR;
    }
    for (i = 0; i < 3; i++)
    {
        memset(m_iTempRawData, 0, sizeof(m_iTempRawData));

        //water rawdata
        ByteNum = (g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum)*2;
        ReCode = ReadRawData(0, 0xAC, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK)goto TEST_ERR;
        memcpy( m_RawData[0+g_ScreenSetParam.iTxNum], m_iTempRawData, sizeof(int)*g_ScreenSetParam.iRxNum );
        memcpy( m_RawData[1+g_ScreenSetParam.iTxNum], m_iTempRawData + g_ScreenSetParam.iRxNum, sizeof(int)*g_ScreenSetParam.iTxNum );

        //No water rawdata
        ByteNum = (g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum)*2;
        ReCode = ReadRawData(0, 0xAB, ByteNum, m_iTempRawData);
        if (ReCode != ERROR_CODE_OK)goto TEST_ERR;
        memcpy( m_RawData[2+g_ScreenSetParam.iTxNum], m_iTempRawData, sizeof(int)*g_ScreenSetParam.iRxNum );
        memcpy( m_RawData[3+g_ScreenSetParam.iTxNum], m_iTempRawData + g_ScreenSetParam.iRxNum, sizeof(int)*g_ScreenSetParam.iTxNum );
    }


    //-----3. Judge

    //Waterproof ON
    bFlag=GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_SetWaterproof_ON && bFlag )
    {
        iCount = 0;
        RawDataMin = g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_ON_Min;
        RawDataMax = g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_ON_Max;
        iMax = -m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;


        bFlag=GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-ON:");
        for ( i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++ )
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Min[0][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Max[0][i];
            Value = m_RawData[0+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-ON:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Min[1][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_ON_Max[1][i];
            Value = m_RawData[1+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value; //find the Max value
            if (iMin > Value) iMin = Value; //fine the min value
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap RawData in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+0, 2, g_ScreenSetParam.iRxNum, 1);
    }

    //Waterproof OFF
    bFlag=GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_SetWaterproof_OFF && bFlag)
    {
        iCount = 0;
        RawDataMin = g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_OFF_Min;
        RawDataMax = g_stCfg_FT5822_BasicThreshold.SCapRawDataTest_OFF_Max;
        iMax = -m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;

        bFlag=GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Rx in Waterproof-OFF:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Min[0][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Max[0][i];
            Value = m_RawData[2+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            //FTS_TEST_DBG("zaxzax3 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            //strTemp += str;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }

        bFlag=GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("Judge Tx in Waterproof-OFF:");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;

            Value = m_RawData[3+g_ScreenSetParam.iTxNum][i];
            RawDataMin = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Min[1][i];
            RawDataMax = g_stCfg_MCap_DetailThreshold.SCapRawDataTest_OFF_Max[1][i];
            //FTS_TEST_DBG("zaxzax4 Value %d RawDataMin %d  RawDataMax %d  ",  Value, RawDataMin, RawDataMax);
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > RawDataMax || Value < RawDataMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, RawDataMin, RawDataMax);
            }
            iCount++;
        }
        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap RawData in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+2, 2, g_ScreenSetParam.iRxNum, 2);
    }
    //-----4. post-stage work
    if (m_bV3TP)
    {
        ReCode = ReadReg( REG_MAPPING_SWITCH, &ucValue );
        if (0 !=ucValue )
        {
            ReCode = WriteReg( REG_MAPPING_SWITCH, 0 );
            SysDelay(10);
            if ( ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_DBG("Failed to switch mapping type!\n ");
                btmpresult = false;
            }
        }

        //Only self content will be used before the Mapping, so the end of the test items, need to go after Mapping
        GetChannelNum();
    }

    TestResultLen += sprintf(TestResult+TestResultLen," SCap RawData Test is %s. \n\n", (btmpresult ? "OK" : "NG"));
    //-----5. Test Result
    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//SCap RawData Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//SCap RawData Test is NG!");
    }
    return ReCode;

TEST_ERR:
    * bTestResult = false;
    FTS_TEST_INFO("\n\n//SCap RawData Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen," SCap RawData Test is NG. \n\n");
    return ReCode;
}





/************************************************************************
* Name: FT5822_TestItem_SCapCbTest
* Brief:  TestItem: SCapCbTest. Check if SCAP Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT5822_TestItem_SCapCbTest(bool* bTestResult)
{
    int i,/* j, iOutNum,*/index,Value,CBMin,CBMax;
    boolean bFlag = true;
    unsigned char ReCode;
    boolean btmpresult = true;
    int iMax, iMin, iAvg;
    unsigned char wc_value = 0;
    unsigned char ucValue = 0;
    int iCount = 0;
//   int ibiggerValue = 0;

    FTS_TEST_DBG("\n\n==============================Test Item: -----  Scap CB Test \n");
    //-------1.Preparatory work
    //in Factory Mode
    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    //get waterproof channel setting, to check if Tx/Rx channel need to test
    ReCode = ReadReg( REG_WATER_CHANNEL_SELECT, &wc_value );
    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //If it is V3 pattern, Get Tx/Rx Num again
    bFlag= SwitchToNoMapping();
    if ( bFlag )
    {
        FTS_TEST_ERROR("Failed to SwitchToNoMapping! ");
        goto TEST_ERR;
    }

    //-------2.Get SCap Raw Data, Step:1.Start Scanning; 2. Read Raw Data
    ReCode = StartScan();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("Failed to Scan SCap RawData! ");
        goto TEST_ERR;
    }


    for (i = 0; i < 3; i++)
    {
        memset(m_RawData, 0, sizeof(m_RawData));
        memset(m_ucTempData, 0, sizeof(m_ucTempData));

        //waterproof CB
        ReCode = WriteReg( REG_ScWorkMode, 1 );//ScWorkMode:  1:waterproof 0:Non-waterproof
        ReCode = StartScan();
        ReCode = WriteReg( REG_ScCbAddrR, 0 );
        ReCode = GetTxSC_CB( g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum + 128, m_ucTempData );
        for ( index = 0; index < g_ScreenSetParam.iRxNum; ++index )
        {
            m_RawData[0 + g_ScreenSetParam.iTxNum][index]= m_ucTempData[index];
        }
        for ( index = 0; index < g_ScreenSetParam.iTxNum; ++index )
        {
            m_RawData[1 + g_ScreenSetParam.iTxNum][index] = m_ucTempData[index + g_ScreenSetParam.iRxNum];
        }

        //Non-waterproof rawdata
        ReCode = WriteReg( REG_ScWorkMode, 0 );//ScWorkMode:  1:waterproof 0:Non-waterproof
        ReCode = StartScan();
        ReCode = WriteReg( REG_ScCbAddrR, 0 );
        ReCode = GetTxSC_CB( g_ScreenSetParam.iRxNum + g_ScreenSetParam.iTxNum + 128, m_ucTempData );
        for ( index = 0; index < g_ScreenSetParam.iRxNum; ++index )
        {
            m_RawData[2 + g_ScreenSetParam.iTxNum][index]= m_ucTempData[index];
        }
        for ( index = 0; index < g_ScreenSetParam.iTxNum; ++index )
        {
            m_RawData[3 + g_ScreenSetParam.iTxNum][index] = m_ucTempData[index + g_ScreenSetParam.iRxNum];
        }

        if ( ReCode != ERROR_CODE_OK )
        {
            FTS_TEST_ERROR("Failed to Get SCap CB!");
        }
    }

    if (ReCode != ERROR_CODE_OK) goto TEST_ERR;

    //-----3. Judge

    //Waterproof ON
    bFlag=GetTestCondition(WT_NeedProofOnTest, wc_value);
    if (g_stCfg_FT5822_BasicThreshold.SCapCbTest_SetWaterproof_ON && bFlag)
    {
        FTS_TEST_DBG("SCapCbTest in WaterProof On Mode:  ");

        iMax = -m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[0+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag=GetTestCondition(WT_NeedRxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Rx:  ");
        for ( i = 0; bFlag && i < g_ScreenSetParam.iRxNum; i++ )
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Min[0][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Max[0][i];
            Value = m_RawData[0+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value; //find the Max Value
            if (iMin > Value) iMin = Value; //find the Min Value
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOnVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Tx:  ");
        for (i = 0; bFlag &&  i < g_ScreenSetParam.iTxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Min[1][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_ON_Max[1][i];
            Value = m_RawData[1+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap CB in Waterproof-ON, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+0, 2, g_ScreenSetParam.iRxNum, 1);
    }

    bFlag=GetTestCondition(WT_NeedProofOffTest, wc_value);
    if (g_stCfg_FT5822_BasicThreshold.SCapCbTest_SetWaterproof_OFF && bFlag)
    {
        FTS_TEST_DBG("SCapCbTest in WaterProof OFF Mode:  ");
        iMax = -m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iMin = 2 * m_RawData[2+g_ScreenSetParam.iTxNum][0];
        iAvg = 0;
        Value = 0;
        iCount = 0;


        bFlag=GetTestCondition(WT_NeedRxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Rx:  ");
        for (i = 0; bFlag &&  i < g_ScreenSetParam.iRxNum; i++)
        {
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[0][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Min[0][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Max[0][i];
            Value = m_RawData[2+g_ScreenSetParam.iTxNum][i];
            iAvg += Value;

            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }


        bFlag=GetTestCondition(WT_NeedTxOffVal, wc_value);
        if (bFlag)
            FTS_TEST_DBG("SCap CB_Tx:  ");
        for (i = 0; bFlag && i < g_ScreenSetParam.iTxNum; i++)
        {
            //if( m_ScapInvalide[1][i] == 0 )      continue;
            if ( g_stCfg_MCap_DetailThreshold.InvalidNode_SC[1][i] == 0 )      continue;
            CBMin = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Min[1][i];
            CBMax = g_stCfg_MCap_DetailThreshold.SCapCbTest_OFF_Max[1][i];
            Value = m_RawData[3+g_ScreenSetParam.iTxNum][i];

            iAvg += Value;
            if (iMax < Value) iMax = Value;
            if (iMin > Value) iMin = Value;
            if (Value > CBMax || Value < CBMin)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Failed. Num = %d, Value = %d, range = (%d, %d):",  i+1, Value, CBMin, CBMax);
            }
            iCount++;
        }

        if (0 == iCount)
        {
            iAvg = 0;
            iMax = 0;
            iMin = 0;
        }
        else
            iAvg = iAvg/iCount;

        FTS_TEST_DBG("SCap CB in Waterproof-OFF, Max : %d, Min: %d, Deviation: %d, Average: %d",  iMax, iMin, iMax - iMin, iAvg);
        //////////////////////////////Save Test Data
        //ibiggerValue = g_ScreenSetParam.iTxNum>g_ScreenSetParam.iRxNum?g_ScreenSetParam.iTxNum:g_ScreenSetParam.iRxNum;
        Save_Test_Data(m_RawData, g_ScreenSetParam.iTxNum+2, 2, g_ScreenSetParam.iRxNum, 2);
    }
    //-----4. post-stage work
    if (m_bV3TP)
    {
        ReCode = ReadReg( REG_MAPPING_SWITCH, &ucValue );
        if (0 != ucValue )
        {
            ReCode = WriteReg( REG_MAPPING_SWITCH, 0 );
            SysDelay(10);
            if ( ReCode != ERROR_CODE_OK)
            {
                FTS_TEST_DBG("Failed to switch mapping type!\n ");
                btmpresult = false;
            }
        }

        //Only self content will be used before the Mapping, so the end of the test items, need to go after Mapping
        GetChannelNum();
    }

    TestResultLen += sprintf(TestResult+TestResultLen," SCap CB Test is %s. \n\n", (btmpresult ? "OK" : "NG"));
    //-----5. Test Result

    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("\n\n//SCap CB Test Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("\n\n//SCap CB Test Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("\n\n//SCap CB Test Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen," SCap CB Test is NG. \n\n");
    return ReCode;
}
unsigned char FT5822_TestItem_PanelDifferTest(bool * bTestResult)
{
    int index = 0;
    int iRow = 0, iCol = 0;
    int iValue = 0;
    unsigned char ReCode = 0;
    bool btmpresult = true;
    int iMax, iMin; //, iAvg;
    int maxValue=0;
    int minValue=32767;
    int AvgValue = 0;
    int InvalidNum=0;
    int i = 0,  j = 0;


    unsigned char OriginRawDataType = 0xff;
    unsigned char OriginFrequecy = 0xff;
    unsigned char OriginFirState = 0xff;


    FTS_TEST_INFO("\r\n\r\n\r\n==============================Test Item: -------- Panel Differ Test  \r\n\r\n");

    ReCode = EnterFactory();
    if (ReCode != ERROR_CODE_OK)
    {
        FTS_TEST_ERROR("\n\n// Failed to Enter factory Mode. Error Code: %d", ReCode);
        goto TEST_ERR;
    }

    FTS_TEST_DBG("\r\n=========Set Auto Equalization:\r\n");
    ReCode = ReadReg( REG_NORMALIZE_TYPE, &OriginRawDataType);//Read the original value
    if ( ReCode != ERROR_CODE_OK )
    {
        btmpresult = false;
        goto TEST_ERR;
    }

    ReCode = WriteReg( REG_NORMALIZE_TYPE, 0x00 );
    SysDelay(10);
    if ( ReCode != ERROR_CODE_OK )
    {
        btmpresult = false;
        FTS_TEST_ERROR( "\r\nWrite reg failed\r\n" );
        goto TEST_ERR;
    }

    //设置高频点
    FTS_TEST_DBG("\r\n=========Set Frequecy High\r\n");
    ReCode = ReadReg( REG_FREQUENCY, &OriginFrequecy);//Read the original value
    if ( ReCode != ERROR_CODE_OK )
    {
        btmpresult = false;
        goto TEST_ERR;
    }

    ReCode = WriteReg( 0x0A, 0x81);
    SysDelay(10);

    FTS_TEST_DBG("\r\n=========FIR State: OFF\r\n");
    ReCode = ReadReg( 0xFB, &OriginFirState);//Read the original value
    if ( ReCode != ERROR_CODE_OK )
    {
        btmpresult = false;
        goto TEST_ERR;
    }
    ReCode = WriteReg( 0xFB, 0);
    SysDelay(10);
    if ( ReCode != ERROR_CODE_OK )
    {
        FTS_TEST_ERROR("\r\nFailed to Write Fir Reg!\r\n ");
        btmpresult = false;
        goto TEST_ERR;
    }

    //Previously changed the register required to lose three frame data, the fourth frame is the use of the data
    for ( index = 0; index < 4; ++index )
    {
        ReCode = GetRawData();
        if ( ReCode != ERROR_CODE_OK )
        {
            btmpresult = false;
            goto TEST_ERR;
        }
    }

    ////Differ is RawData的1/10
    for ( i = 0; i < g_ScreenSetParam.iTxNum; i++)
    {
        for ( j= 0; j < g_ScreenSetParam.iRxNum; j++)
        {
            m_DifferData[i][j] = m_RawData[i][j]/10;
        }
    }

    ////////////////////////////////To show value
#if 1
    FTS_TEST_DBG("PannelDiffer :\n");
    for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++)
    {
        FTS_TEST_DBG("\nRow%2d:    ", iRow+1);
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            //  if(g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue;//Invalid Node

            iValue = m_DifferData[iRow][iCol];
            FTS_TEST_DBG("%4d,  ", iValue);
        }
        FTS_TEST_DBG("\n" );
    }
    FTS_TEST_DBG("\n" );
#endif


    ///whether threshold is in range
    ////////////////////////////////To Determine  if in Range or not

    for (iRow = 0; iRow<g_ScreenSetParam.iTxNum; iRow++) //  iRow = 1 ???
    {
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            if (g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue; //Invalid Node

            iValue = m_DifferData[iRow][iCol];
            iMin =  g_stCfg_MCap_DetailThreshold.PanelDifferTest_Min[iRow][iCol];
            iMax = g_stCfg_MCap_DetailThreshold.PanelDifferTest_Max[iRow][iCol];

            if (iValue < iMin || iValue > iMax)
            {
                btmpresult = false;
                FTS_TEST_ERROR("Out Of Range.  Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
                               iRow+1, iCol+1, iValue, iMin, iMax);
            }
        }
    }

    ///////////////////////////  end determine

    ////////////////////
    ////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>get test data ,and save to .csv file
    FTS_TEST_DBG("PannelDiffer ABS:\n");
    for ( i = 0; i <  g_ScreenSetParam.iTxNum; i++)
    {
        FTS_TEST_DBG("\n");
        for ( j = 0; j <  g_ScreenSetParam.iRxNum; j++)
        {

            FTS_TEST_DBG("%ld,", abs(m_DifferData[i][j]));
            m_absDifferData[i][j] = abs(m_DifferData[i][j]);

            if ( NODE_AST_TYPE == g_stCfg_MCap_DetailThreshold.InvalidNode[i][j] || NODE_INVALID_TYPE == g_stCfg_MCap_DetailThreshold.InvalidNode[i][j])
            {
                InvalidNum++;
                continue;
            }
            maxValue = max(maxValue,m_DifferData[i][j]);
            minValue = min(minValue,m_DifferData[i][j]);
            AvgValue += m_DifferData[i][j];
        }
    }
    FTS_TEST_DBG("\n");
    Save_Test_Data(m_absDifferData, 0, g_ScreenSetParam.iTxNum, g_ScreenSetParam.iRxNum, 1);
    ////<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<get test data ,and save to .csv file

    AvgValue = AvgValue/( g_ScreenSetParam.iTxNum*g_ScreenSetParam.iRxNum - InvalidNum);
    FTS_TEST_DBG("PanelDiffer:Max: %d, Min: %d, Avg: %d ", maxValue, minValue,AvgValue);

    ReCode = WriteReg( REG_NORMALIZE_TYPE, OriginRawDataType );//set to original value
    ReCode = WriteReg( 0x0A, OriginFrequecy );//set to original value
    ReCode = WriteReg( 0xFB, OriginFirState );//set to original value


    TestResultLen += sprintf(TestResult+TestResultLen," Panel Differ Test is %s. \n\n", (btmpresult ? "OK" : "NG"));

    if ( btmpresult )
    {
        *bTestResult = true;
        FTS_TEST_INFO("		//Panel Differ Test is OK!");
    }
    else
    {
        * bTestResult = false;
        FTS_TEST_INFO("		//Panel Differ Test is NG!");
    }
    return ReCode;

TEST_ERR:

    * bTestResult = false;
    FTS_TEST_INFO("		//Panel Differ Test is NG!");
    TestResultLen += sprintf(TestResult+TestResultLen," Panel Differ Test is NG. \n\n");
    return ReCode;
}

/************************************************************************
* Name: GetPanelRows(Same function name as FT_MultipleTest)
* Brief:  Get row of TP
* Input: none
* Output: pPanelRows
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelRows(unsigned char *pPanelRows)
{
    return ReadReg(REG_TX_NUM, pPanelRows);
}

/************************************************************************
* Name: GetPanelCols(Same function name as FT_MultipleTest)
* Brief:  get column of TP
* Input: none
* Output: pPanelCols
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetPanelCols(unsigned char *pPanelCols)
{
    return ReadReg(REG_RX_NUM, pPanelCols);
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
    const unsigned char MaxTimes = 250;  //The longest wait 160ms
    unsigned char ReCode = ERROR_CODE_COMM_ERROR;

    ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);
    if (ReCode == ERROR_CODE_OK)
    {
        RegVal |= 0x80;     //Top bit position 1, start scan
        ReCode = WriteReg(DEVIDE_MODE_ADDR, RegVal);
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
unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer)
{
    unsigned char ReCode=ERROR_CODE_COMM_ERROR;
    unsigned char I2C_wBuffer[3];
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


    //***********************************************************Read raw data
    I2C_wBuffer[0] = REG_RawBuf0;   //set begin address
    if (ReCode == ERROR_CODE_OK)
    {
        focal_msleep(10);
        ReCode = Comm_Base_IIC_IO(I2C_wBuffer, 1, m_ucTempData, BytesNumInTestMode1);
    }

    for (i=1; i<iReadNum; i++)
    {
        if (ReCode != ERROR_CODE_OK) break;

        if (i==iReadNum-1) //last packet
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, m_ucTempData+BYTES_PER_TIME*i, ByteNum-BYTES_PER_TIME*i);
        }
        else
        {
            focal_msleep(10);
            ReCode = Comm_Base_IIC_IO(NULL, 0, m_ucTempData+BYTES_PER_TIME*i, BYTES_PER_TIME);
        }

    }

    if (ReCode == ERROR_CODE_OK)
    {
        for (i=0; i<(ByteNum>>1); i++)
        {
            pRevBuffer[i] = (m_ucTempData[i<<1]<<8)+m_ucTempData[(i<<1)+1];
            //if(pRevBuffer[i] & 0x8000)//have sign bit
            //{
            //  pRevBuffer[i] -= 0xffff + 1;
            //}
        }
    }

    return ReCode;

}
/************************************************************************
* Name: GetTxSC_CB(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx SCap
* Input: index
* Output: pcbValue
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char GetTxSC_CB(unsigned char index, unsigned char *pcbValue)
{
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char wBuffer[4];

    if (index<128) //single read
    {
        *pcbValue = 0;
        WriteReg(REG_ScCbAddrR, index);
        ReCode = ReadReg(REG_ScCbBuf0, pcbValue);
    }
    else//Sequential Read length index-128
    {
        WriteReg(REG_ScCbAddrR, 0);
        wBuffer[0] = REG_ScCbBuf0;
        ReCode = Comm_Base_IIC_IO(wBuffer, 1, pcbValue, index-128);

    }

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

/************************************************************************
* Name: GetChannelNum
* Brief:  Get Channel Num(Tx and Rx)
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNum(void)
{
    unsigned char ReCode;
    unsigned char rBuffer[1]; //= new unsigned char;

    //m_strCurrentTestMsg = "Get Tx Num...";
    ReCode = GetPanelRows(rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iTxNum = rBuffer[0];
        if (g_ScreenSetParam.iTxNum+4 > g_ScreenSetParam.iUsedMaxTxNum)
        {
            FTS_TEST_ERROR("Failed to get Tx number, Get num = %d, UsedMaxNum = %d",
                           g_ScreenSetParam.iTxNum, g_ScreenSetParam.iUsedMaxTxNum);
            g_ScreenSetParam.iTxNum = 0;
            return ERROR_CODE_INVALID_PARAM;
        }
//g_ScreenSetParam.iTxNum = 26;
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Tx number");
    }

    ///////////////m_strCurrentTestMsg = "Get Rx Num...";

    ReCode = GetPanelCols(rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iRxNum = rBuffer[0];
        if (g_ScreenSetParam.iRxNum > g_ScreenSetParam.iUsedMaxRxNum)
        {
            FTS_TEST_ERROR("Failed to get Rx number, Get num = %d, UsedMaxNum = %d",
                           g_ScreenSetParam.iRxNum, g_ScreenSetParam.iUsedMaxRxNum);
            g_ScreenSetParam.iRxNum = 0;
            return ERROR_CODE_INVALID_PARAM;
        }
        //g_ScreenSetParam.iRxNum = 28;
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Rx number");
    }

    return ReCode;

}
/************************************************************************
* Name: GetRawData
* Brief:  Get Raw Data of MCAP
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetRawData(void)
{
    unsigned char ReCode = ERROR_CODE_OK;
    int iRow = 0;
    int iCol = 0;

    //--------------------------------------------Enter Factory Mode
    ReCode = EnterFactory();
    if ( ERROR_CODE_OK != ReCode )
    {
        FTS_TEST_ERROR("Failed to Enter Factory Mode...");
        return ReCode;
    }


    //--------------------------------------------Check Num of Channel
    if (0 == (g_ScreenSetParam.iTxNum + g_ScreenSetParam.iRxNum))
    {
        ReCode = GetChannelNum();
        if ( ERROR_CODE_OK != ReCode )
        {
            FTS_TEST_ERROR("Error Channel Num...");
            return ERROR_CODE_INVALID_PARAM;
        }
    }

    //--------------------------------------------Start Scanning
    FTS_TEST_DBG("Start Scan ...");
    ReCode = StartScan();
    if (ERROR_CODE_OK != ReCode)
    {
        FTS_TEST_ERROR("Failed to Scan ...");
        return ReCode;
    }

    //--------------------------------------------Read RawData, Only MCAP
    memset(m_RawData, 0, sizeof(m_RawData));
    ReCode = ReadRawData( 1, 0xAA, ( g_ScreenSetParam.iTxNum * g_ScreenSetParam.iRxNum )*2, m_iTempRawData );
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            m_RawData[iRow][iCol] = m_iTempRawData[iRow*g_ScreenSetParam.iRxNum + iCol];
        }
    }
    return ReCode;
}
/************************************************************************
* Name: ShowRawData
* Brief:  Show RawData
* Input: none
* Output: none
* Return: none.
***********************************************************************/
static void ShowRawData(void)
{
    int iRow, iCol;
    //----------------------------------------------------------Show RawData
    for (iRow = 0; iRow < g_ScreenSetParam.iTxNum; iRow++)
    {
        FTS_TEST_DBG("Tx%2d:  ", iRow+1);
        for (iCol = 0; iCol < g_ScreenSetParam.iRxNum; iCol++)
        {
            FTS_TEST_DBG("%5d    ", m_RawData[iRow][iCol]);
        }
        FTS_TEST_DBG("\n ");
    }
}

/************************************************************************
* Name: GetChannelNumNoMapping
* Brief:  get Tx&Rx num from other Register
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNumNoMapping(void)
{
    unsigned char ReCode;
    unsigned char rBuffer[1]; //= new unsigned char;


    FTS_TEST_DBG("Get Tx Num...");
    ReCode =ReadReg( REG_TX_NOMAPPING_NUM,  rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iTxNum= rBuffer[0];
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Tx number");
    }


    FTS_TEST_DBG("Get Rx Num...");
    ReCode = ReadReg( REG_RX_NOMAPPING_NUM,  rBuffer);
    if (ReCode == ERROR_CODE_OK)
    {
        g_ScreenSetParam.iRxNum = rBuffer[0];
    }
    else
    {
        FTS_TEST_ERROR("Failed to get Rx number");
    }

    return ReCode;
}
/************************************************************************
* Name: SwitchToNoMapping
* Brief:  If it is V3 pattern, Get Tx/Rx Num again
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char SwitchToNoMapping(void)
{
    unsigned char chPattern = -1;
    unsigned char ReCode = ERROR_CODE_OK;
    unsigned char RegData = -1;
    ReCode = ReadReg( REG_PATTERN_5422, &chPattern );//

    if (1 == chPattern) // 1: V3 Pattern
    {
        RegData = -1;
        ReCode =ReadReg( REG_MAPPING_SWITCH, &RegData );
        if ( 1 != RegData )
        {
            ReCode = WriteReg( REG_MAPPING_SWITCH, 1 );  //0-mapping 1-no mampping
            focal_msleep(20);
            GetChannelNumNoMapping();
        }
    }

    if ( ReCode != ERROR_CODE_OK )
    {
        FTS_TEST_ERROR("Switch To NoMapping Failed!");
    }
    return ReCode;
}
/************************************************************************
* Name: GetTestCondition
* Brief:  Check whether Rx or TX need to test, in Waterproof ON/OFF Mode.
* Input: none
* Output: none
* Return: true: need to test; false: Not tested.
***********************************************************************/
static boolean GetTestCondition(int iTestType, unsigned char ucChannelValue)
{
    boolean bIsNeeded = false;
    switch (iTestType)
    {
        case WT_NeedProofOnTest://Bit5:  0:test waterProof mode ;  1 not test waterProof mode
            bIsNeeded = !( ucChannelValue & 0x20 );
            break;
        case WT_NeedProofOffTest://Bit7: 0: test normal mode  1:not test normal mode
            bIsNeeded = !( ucChannelValue & 0x80 );
            break;
        case WT_NeedTxOnVal:
            //Bit6:  0 : test waterProof Rx+Tx  1:test waterProof single channel
            //Bit2:  0: test waterProof Tx only;  1:  test waterProof Rx only
            bIsNeeded = !( ucChannelValue & 0x40 ) || !( ucChannelValue & 0x04 );
            break;
        case WT_NeedRxOnVal:
            //Bit6:  0 : test waterProof Rx+Tx  1 test waterProof single channel
            //Bit2:  0: test waterProof Tx only;  1:  test waterProof Rx only
            bIsNeeded = !( ucChannelValue & 0x40 ) || ( ucChannelValue & 0x04 );
            break;
        case WT_NeedTxOffVal://Bit1,Bit0:  00:test normal Tx; 10: test normal Rx+Tx
            bIsNeeded = (0x00 == (ucChannelValue & 0x03)) || (0x02 == ( ucChannelValue & 0x03 ));
            break;
        case WT_NeedRxOffVal://Bit1,Bit0:  01: test normal Rx;    10: test normal Rx+Tx
            bIsNeeded = (0x01 == (ucChannelValue & 0x03)) || (0x02 == ( ucChannelValue & 0x03 ));
            break;
        default:
            break;
    }
    return bIsNeeded;
}

#endif


