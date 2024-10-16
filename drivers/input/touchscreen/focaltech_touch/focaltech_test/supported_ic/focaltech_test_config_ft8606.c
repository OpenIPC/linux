/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: Focaltech_test_config_ft8606.c
*
* Author: Software Development Team, AE
*
* Created: 2016-08-01
*
* Abstract: Set Config for FT8606
*
************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>

#include "../include/focaltech_test_ini.h"
#include "../include/focaltech_test_supported_ic.h"
#include "../focaltech_test_config.h"

#if (FTS_CHIP_TEST_TYPE ==FT8606_TEST)


struct stCfg_FT8606_TestItem g_stCfg_FT8606_TestItem;
struct stCfg_FT8606_BasicThreshold g_stCfg_FT8606_BasicThreshold;

void OnInit_FT8606_BasicThreshold(char *strIniFile)
{

    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString( "Basic_Threshold", "FW_VER_VALUE", "0",str, strIniFile);
    g_stCfg_FT8606_BasicThreshold.FW_VER_VALUE = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("Basic_Threshold","Factory_ID_Number","255",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.Factory_ID_Number = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("Basic_Threshold","Project_Code"," ",str,strIniFile);
    //g_stCfg_FT8606_BasicThreshold.Project_Code.Format("%s", str);
    sprintf(g_stCfg_FT8606_BasicThreshold.Project_Code, "%s", str);

    //////////////////////////////////////////////////////////// IC Version
    GetPrivateProfileString("Basic_Threshold","IC_Version","3",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.IC_Version = fts_atoi(str);

    //////////////////////////////////////////////////////////// IC Version
    GetPrivateProfileString("Basic_Threshold","RawDataTest_Min","5000",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.RawDataTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","RawDataTest_Max","11000",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.RawDataTest_Max = fts_atoi(str);

    //////////////////////////////////////////////////////////// ChannelNumTest
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_ChannelX","15",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.ChannelNumTest_ChannelXNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_ChannelY","24",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.ChannelNumTest_ChannelYNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_KeyNum","0",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.ChannelNumTest_KeyNum = fts_atoi(str);

    //////////////////////////////////////////////////////////// ResetPinTest
    GetPrivateProfileString("Basic_Threshold","ResetPinTest_RegAddr","136",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.ResetPinTest_RegAddr = fts_atoi(str);

    //////////////////////////////////////////////////////////// IntPinTest
    GetPrivateProfileString("Basic_Threshold","IntPinTest_RegAddr","175",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.IntPinTest_RegAddr = fts_atoi(str);

    //////////////////////////////////////////////////////////// NoiseTest
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Coefficient","50",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.NoiseTest_Coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Frames","32",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.NoiseTest_Frames = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Time","1",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.NoiseTest_Time = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_SampeMode","0",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.NoiseTest_SampeMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold","NoiseTest_NoiseMode","0",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.NoiseTest_NoiseMode = fts_atoi(str);

    GetPrivateProfileString("Basic_Threshold","NoiseTest_ShowTip","0",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.NoiseTest_ShowTip = fts_atoi(str);

    //////////////////////////////////////////////////////////// CBTest
    GetPrivateProfileString("Basic_Threshold","CBTest_Min","3",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.CbTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Max","100",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.CbTest_Max = fts_atoi(str);

    //////////////////////////////////////////////////////////// ShortCircuit
    GetPrivateProfileString("Basic_Threshold","ShortCircuit_CBMax","120",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.ShortTest_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","ShortCircuit_K2Value","150",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.ShortTest_K2Value = fts_atoi(str);

    //////////////////////////////////////////////////////////// LCD_Noise
    GetPrivateProfileString("Basic_Threshold","LCD_NoiseTest_Frame","50",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.iLCDNoiseTestFrame = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LCD_NoiseTest_Max_Screen","32",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.iLCDNoiseTestMaxScreen = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LCD_NoiseTest_Max_Frame","32",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.iLCDNoiseTestMaxFrame = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LCD_NoiseTest_Coefficient","50",str,strIniFile);
    g_stCfg_FT8606_BasicThreshold.iLCDNoiseCoefficient = fts_atoi(str);

    FTS_TEST_FUNC_EXIT();

}

void OnInit_FT8606_TestItem(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString("TestItem","FW_VERSION_TEST","0",str,strIniFile);
    g_stCfg_FT8606_TestItem.FW_VERSION_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("TestItem","FACTORY_ID_TEST","0",str,strIniFile);
    g_stCfg_FT8606_TestItem.FACTORY_ID_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("TestItem","PROJECT_CODE_TEST","0",str,strIniFile);
    g_stCfg_FT8606_TestItem.PROJECT_CODE_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// IC Version
    GetPrivateProfileString("TestItem","IC_VERSION_TEST","0",str,strIniFile);
    g_stCfg_FT8606_TestItem.IC_VERSION_TEST = fts_atoi(str);

    /////////////////////////////////// RawData Test
    GetPrivateProfileString("TestItem","RAWDATA_TEST","1",str,strIniFile);
    g_stCfg_FT8606_TestItem.RAWDATA_TEST = fts_atoi(str);

    /////////////////////////////////// CHANNEL_NUM_TEST
    GetPrivateProfileString("TestItem","CHANNEL_NUM_TEST","1",str,strIniFile);
    g_stCfg_FT8606_TestItem.CHANNEL_NUM_TEST = fts_atoi(str);

    /////////////////////////////////// INT_PIN_TEST
    GetPrivateProfileString("TestItem","INT_PIN_TEST","0",str,strIniFile);
    g_stCfg_FT8606_TestItem.INT_PIN_TEST = fts_atoi(str);

    /////////////////////////////////// RESET_PIN_TEST
    GetPrivateProfileString("TestItem","RESET_PIN_TEST","0",str,strIniFile);
    g_stCfg_FT8606_TestItem.RESET_PIN_TEST = fts_atoi(str);

    /////////////////////////////////// NOISE_TEST
    GetPrivateProfileString("TestItem","NOISE_TEST","0",str,strIniFile);
    g_stCfg_FT8606_TestItem.NOISE_TEST = fts_atoi(str);

    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem","CB_TEST","1",str,strIniFile);
    g_stCfg_FT8606_TestItem.CB_TEST = fts_atoi(str);

    /////////////////////////////////// SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem","SHORT_CIRCUIT_TEST","1",str,strIniFile);
    g_stCfg_FT8606_TestItem.SHORT_CIRCUIT_TEST = fts_atoi(str);

    /////////////////////////////////// LCD_NOISE_TEST
    GetPrivateProfileString("TestItem","LCD_NOISE_TEST","0",str,strIniFile);
    g_stCfg_FT8606_TestItem.LCD_NOISE_TEST = fts_atoi(str);


    FTS_TEST_FUNC_EXIT();

}

void SetTestItem_FT8606(void)
{

    g_TestItemNum = 0;

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////FACTORY_ID_TEST
    if ( g_stCfg_FT8606_TestItem.FACTORY_ID_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_FACTORY_ID_TEST);
    }

    //////////////////////////////////////////////////Project Code Test
    if ( g_stCfg_FT8606_TestItem.PROJECT_CODE_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_PROJECT_CODE_TEST);
    }

    //////////////////////////////////////////////////FW Version Test
    if ( g_stCfg_FT8606_TestItem.FW_VERSION_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_FW_VERSION_TEST);
    }

    //////////////////////////////////////////////////IC Version Test
    if ( g_stCfg_FT8606_TestItem.IC_VERSION_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_IC_VERSION_TEST);
    }

    //////////////////////////////////////////////////Enter Factory Mode

    fts_SetTestItemCodeName(Code_FT8606_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////CHANNEL_NUM_TEST
    if ( g_stCfg_FT8606_TestItem.CHANNEL_NUM_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_CHANNEL_NUM_TEST);
    }

    //////////////////////////////////////////////////Short Test
    if ( g_stCfg_FT8606_TestItem.SHORT_CIRCUIT_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_SHORT_CIRCUIT_TEST);
    }

    //////////////////////////////////////////////////CB_TEST
    if ( g_stCfg_FT8606_TestItem.CB_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_CB_TEST);
    }


    //////////////////////////////////////////////////NOISE_TEST
    if ( g_stCfg_FT8606_TestItem.NOISE_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_NOISE_TEST);
    }

    //////////////////////////////////////////////////LCD_NOISE_TEST
    if ( g_stCfg_FT8606_TestItem.LCD_NOISE_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_LCD_NOISE_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( g_stCfg_FT8606_TestItem.RAWDATA_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_RAWDATA_TEST);
    }

    //////////////////////////////////////////////////RESET_PIN_TEST
    if ( g_stCfg_FT8606_TestItem.RESET_PIN_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_RESET_PIN_TEST);
    }
    //////////////////////////////////////////////////INT_PIN_TEST
    if ( g_stCfg_FT8606_TestItem.INT_PIN_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8606_INT_PIN_TEST);
    }

    FTS_TEST_FUNC_EXIT();
}

#endif

