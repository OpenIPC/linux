/************************************************************************
* Copyright (C) 2012-2017, Focaltech Systems (R)£¬All Rights Reserved.
*
* File Name: Focaltech_test_config_ft8006.c
*
* Author: Software Development Team, AE
*
* Created: 2016-08-01
*
* Abstract: Set Config for FT8006
*
************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>

#include "../include/focaltech_test_ini.h"
#include "../include/focaltech_test_supported_ic.h"
#include "../focaltech_test_config.h"

#if (FTS_CHIP_TEST_TYPE ==FT8006_TEST)


struct stCfg_FT8006_TestItem g_stCfg_FT8006_TestItem;
struct stCfg_FT8006_BasicThreshold g_stCfg_FT8006_BasicThreshold;

void OnInit_FT8006_TestItem(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();


    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString("TestItem","FW_VERSION_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.FW_VERSION_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("TestItem","FACTORY_ID_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.FACTORY_ID_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("TestItem","PROJECT_CODE_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.PROJECT_CODE_TEST = fts_atoi(str);

    //////////////////////////////////////////////////////////// IC Version
    GetPrivateProfileString("TestItem","IC_VERSION_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.IC_VERSION_TEST = fts_atoi(str);

    /////////////////////////////////// RawData Test
    GetPrivateProfileString("TestItem","RAWDATA_TEST","1",str,strIniFile);
    g_stCfg_FT8006_TestItem.RAWDATA_TEST = fts_atoi(str);

    /////////////////////////////////// CHANNEL_NUM_TEST
    GetPrivateProfileString("TestItem","CHANNEL_NUM_TEST","1",str,strIniFile);
    g_stCfg_FT8006_TestItem.CHANNEL_NUM_TEST = fts_atoi(str);

    /////////////////////////////////// INT_PIN_TEST
    GetPrivateProfileString("TestItem","INT_PIN_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.INT_PIN_TEST = fts_atoi(str);

    /////////////////////////////////// RESET_PIN_TEST
    GetPrivateProfileString("TestItem","RESET_PIN_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.RESET_PIN_TEST = fts_atoi(str);

    /////////////////////////////////// NOISE_TEST
    GetPrivateProfileString("TestItem","NOISE_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.NOISE_TEST = fts_atoi(str);

    /////////////////////////////////// CB_TEST
    GetPrivateProfileString("TestItem","CB_TEST","1",str,strIniFile);
    g_stCfg_FT8006_TestItem.CB_TEST = fts_atoi(str);

    /////////////////////////////////// SHORT_CIRCUIT_TEST
    GetPrivateProfileString("TestItem","SHORT_CIRCUIT_TEST","1",str,strIniFile);
    g_stCfg_FT8006_TestItem.SHORT_CIRCUIT_TEST = fts_atoi(str);

    /////////////////////////////////// LCD_NOISE_TEST
    GetPrivateProfileString("TestItem","LCD_NOISE_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.LCD_NOISE_TEST = fts_atoi(str);

    /////////////////////////////////// OSC60MHZ_TEST
    GetPrivateProfileString("TestItem","OSC60MHZ_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.OSC60MHZ_TEST = fts_atoi(str);

    /////////////////////////////////// OSCTRM_TEST
    GetPrivateProfileString("TestItem","OSCTRM_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.OSCTRM_TEST = fts_atoi(str);

    /////////////////////////////////// SNR_TEST
    GetPrivateProfileString("TestItem","SNR_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.SNR_TEST = fts_atoi(str);

    /////////////////////////////////// Differ_TEST
    GetPrivateProfileString("TestItem","DIFFER_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.DIFFER_TEST = fts_atoi(str);

    /////////////////////////////////// DIFFER_UNIFORMITY_TEST
    GetPrivateProfileString("TestItem","DIFFER_UNIFORMITY_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.DIFFER_UNIFORMITY_TEST = fts_atoi(str);

    /////////////////////////////////// LPWG_RAWDATA_TEST
    GetPrivateProfileString("TestItem","LPWG_RAWDATA_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.LPWG_RAWDATA_TEST = fts_atoi(str);

    /////////////////////////////////// LPWG_CB_TEST
    GetPrivateProfileString("TestItem","LPWG_CB_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.LPWG_CB_TEST = fts_atoi(str);

    /////////////////////////////////// LPWG_NOISE_TEST
    GetPrivateProfileString("TestItem","LPWG_NOISE_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.LPWG_NOISE_TEST = fts_atoi(str);

    /////////////////////////////////// DIFFER2_TEST
    GetPrivateProfileString("TestItem","DIFFER2_TEST","0",str,strIniFile);
    g_stCfg_FT8006_TestItem.DIFFER2_TEST = fts_atoi(str);


    FTS_TEST_FUNC_EXIT();
}

void OnInit_FT8006_BasicThreshold(char *strIniFile)
{
    char str[512];

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////////////// FW Version
    GetPrivateProfileString( "Basic_Threshold", "FW_VER_VALUE", "0",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.FW_VER_VALUE = fts_atoi(str);

    //////////////////////////////////////////////////////////// Factory ID
    GetPrivateProfileString("Basic_Threshold","Factory_ID_Number","255",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.Factory_ID_Number = fts_atoi(str);

    //////////////////////////////////////////////////////////// Project Code Test
    GetPrivateProfileString("Basic_Threshold","Project_Code"," ",str,strIniFile);
    sprintf(g_stCfg_FT8006_BasicThreshold.Project_Code, "%s", str);

    //////////////////////////////////////////////////////////// IC Version
    GetPrivateProfileString("Basic_Threshold","IC_Version","3",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.IC_Version = fts_atoi(str);

    //////////////////////////////////////////////////////////// RawData Test
    GetPrivateProfileString("Basic_Threshold","RawDataTest_Min","5000",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.RawDataTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","RawDataTest_Max","11000",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.RawDataTest_Max = fts_atoi(str);


    //////////////////////////////////////////////////////////// Channel Num Test
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_ChannelX","15",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.ChannelNumTest_ChannelXNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_ChannelY","24",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.ChannelNumTest_ChannelYNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","ChannelNumTest_KeyNum","0",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.ChannelNumTest_KeyNum = fts_atoi(str);

    //////////////////////////////////////////////////////////// Reset Pin Test
    GetPrivateProfileString("Basic_Threshold","ResetPinTest_RegAddr","136",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.ResetPinTest_RegAddr = fts_atoi(str);

    //////////////////////////////////////////////////////////// Int Pin Test
    GetPrivateProfileString("Basic_Threshold","IntPinTest_RegAddr","175",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.IntPinTest_RegAddr = fts_atoi(str);

    //////////////////////////////////////////////////////////// Noise Test
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Coefficient","50",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.NoiseTest_Coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Frames","32",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.NoiseTest_Frames = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_Time","1",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.NoiseTest_Time = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_SampeMode","0",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.NoiseTest_SampeMode = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_NoiseMode","0",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.NoiseTest_NoiseMode = fts_atoi(str);


    GetPrivateProfileString("Basic_Threshold","NoiseTest_ShowTip","0",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.NoiseTest_ShowTip = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","NoiseTest_IsDiffer","1",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.IsDifferMode = fts_atoi(str);

    //////////////////////////////////////////////////////////// CB Test
    GetPrivateProfileString("Basic_Threshold","CBTest_VA_Check","1",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.bCBTest_VA_Check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Min","3",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.CbTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Max","100",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.CbTest_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_VKey_Check","1",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.bCBTest_VKey_Check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Min_Vkey","3",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.CbTest_Min_Vkey = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","CBTest_Max_Vkey","100",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.CbTest_Max_Vkey = fts_atoi(str);


    //////////////////////////////////////////////////////////// Short Circuit Test
    GetPrivateProfileString("Basic_Threshold","ShortCircuit_ResMin","200",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.ShortCircuit_ResMin = fts_atoi(str);

    //////////////////////////////////////////////////////////// Lcd Noise Test
    GetPrivateProfileString("Basic_Threshold","LCD_NoiseTest_Frame","50",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.iLCDNoiseTestFrame = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LCD_NoiseTest_Max_Screen","32",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.iLCDNoiseTestMaxScreen = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LCD_NoiseTest_Max_Frame","32",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.iLCDNoiseTestMaxFrame = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LCD_NoiseTest_Coefficient","50",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.iLCDNoiseCoefficient = fts_atoi(str);

    ////////////////////////////////////////////////////////////OSC60MHZ_TEST
    GetPrivateProfileString("Basic_Threshold","OSC60MHZTest_OSCMin","12",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.OSC60MHZTest_OSCMin = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","OSC60MHZTest_OSCMax","17",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.OSC60MHZTest_OSCMax = fts_atoi(str);

    ////////////////////////////////////////////////////////////OSCTRM_TEST
    GetPrivateProfileString("Basic_Threshold","OSCTRMTest_OSCMin","15",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.OSCTRMTest_OSCMin = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","OSCTRMTest_OSCMax","17",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.OSCTRMTest_OSCMax = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","OSCTRMTest_OSCDetMin","15",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.OSCTRMTest_OSCDetMin = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","OSCTRMTest_OSCDetMax","17",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.OSCTRMTest_OSCDetMax = fts_atoi(str);

    ////////////////////////////////////////////////////////////SNR_TEST
    GetPrivateProfileString("Basic_Threshold","SNRTest_FrameNum","32",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.SNRTest_FrameNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","SNRTest_Min","10",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.SNRTest_Min = fts_atoi(str);

    ////////////////////////////////////////////////////////////Differ_TEST
    GetPrivateProfileString("Basic_Threshold","DIFFERTest_Frame_Num","32",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DIFFERTest_FrameNum = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DIFFERTest_Differ_Max","100",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DIFFERTest_DifferMax = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DIFFERTest_Differ_Min","10",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DIFFERTest_DifferMin = fts_atoi(str);


    ////////////////////////////////////////////////////////////DifferUniformityTest
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_Check_CHX","0",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DifferUniformityTest_Check_CHX = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_Check_CHY","0",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DifferUniformityTest_Check_CHY = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_Check_MinMax","0",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DifferUniformityTest_Check_MinMax = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_CHX_Hole","20",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DifferUniformityTest_CHX_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_CHY_Hole","20",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DifferUniformityTest_CHY_Hole = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","DifferUniformityTest_MinMax_Hole","70",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.DifferUniformityTest_MinMax_Hole = fts_atoi(str);

    ////////////////////////////////////////////////////////////LPWG_RAWDATA_TEST
    GetPrivateProfileString("Basic_Threshold","LPWG_RawDataTest_Min","5000",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.LPWG_RawDataTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LPWG_RawDataTest_Max","11000",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.LPWG_RawDataTest_Max = fts_atoi(str);

    ////////////////////////////////////////////////////////////LPWG_CB_TEST
    GetPrivateProfileString("Basic_Threshold","LPWG_CBTest_VA_Check","1",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.bLPWG_CBTest_VA_Check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LPWG_CBTest_Min","3",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.LPWG_CbTest_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LPWG_CBTest_Max","60",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.LPWG_CbTest_Max = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LPWG_CBTest_VKey_Check","1",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.bLPWG_CBTest_VKey_Check = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LPWG_CBTest_Min_Vkey","3",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.LPWG_CbTest_Min_Vkey = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LPWG_CBTest_Max_Vkey","100",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.LPWG_CbTest_Max_Vkey = fts_atoi(str);

    ////////////////////////////////////////////////////////////LPWG_NOISE_TEST
    GetPrivateProfileString("Basic_Threshold","LPWG_NoiseTest_Coefficient","50",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.LPWG_NoiseTest_Coefficient = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","LPWG_NoiseTest_Frames","50",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.LPWG_NoiseTest_Frames = fts_atoi(str);

    ////////////////////////////////////////////////////////////DIFFER2_TEST
    GetPrivateProfileString("Basic_Threshold","Differ2Test_Min","1500",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.Differ2Test_Min = fts_atoi(str);
    GetPrivateProfileString("Basic_Threshold","Differ2Test_Max","2000",str,strIniFile);
    g_stCfg_FT8006_BasicThreshold.Differ2Test_Max = fts_atoi(str);




    FTS_TEST_FUNC_EXIT();

}
void SetTestItem_FT8006(void)
{
    g_TestItemNum = 0;

    FTS_TEST_FUNC_ENTER();

    //////////////////////////////////////////////////FACTORY_ID_TEST
    if ( g_stCfg_FT8006_TestItem.FACTORY_ID_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_FACTORY_ID_TEST);
    }

    //////////////////////////////////////////////////Project Code Test
    if ( g_stCfg_FT8006_TestItem.PROJECT_CODE_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_PROJECT_CODE_TEST);
    }

    //////////////////////////////////////////////////FW Version Test
    if ( g_stCfg_FT8006_TestItem.FW_VERSION_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_FW_VERSION_TEST);
    }

    //////////////////////////////////////////////////IC Version Test
    if ( g_stCfg_FT8006_TestItem.IC_VERSION_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_IC_VERSION_TEST);
    }

    //////////////////////////////////////////////////Enter Factory Mode
    fts_SetTestItemCodeName(Code_FT8006_ENTER_FACTORY_MODE);

    //////////////////////////////////////////////////CHANNEL_NUM_TEST
    if ( g_stCfg_FT8006_TestItem.CHANNEL_NUM_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_CHANNEL_NUM_TEST);
    }

    //////////////////////////////////////////////////SHORT_CIRCUIT_TEST
    if ( g_stCfg_FT8006_TestItem.SHORT_CIRCUIT_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_SHORT_CIRCUIT_TEST) ;
    }

    //////////////////////////////////////////////////OSC60MHZ_TEST
    if ( g_stCfg_FT8006_TestItem.OSC60MHZ_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8006_OSC60MHZ_TEST);
    }

    //////////////////////////////////////////////////OSCTRM_TEST
    if ( g_stCfg_FT8006_TestItem.OSCTRM_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8006_OSCTRM_TEST);
    }

    //////////////////////////////////////////////////DIFFER2_TEST
    if ( g_stCfg_FT8006_TestItem.DIFFER2_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8006_DIFFER2_TEST);
    }

    //////////////////////////////////////////////////CB_TEST
    if ( g_stCfg_FT8006_TestItem.CB_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_CB_TEST);
    }

    //////////////////////////////////////////////////NOISE_TEST
    if ( g_stCfg_FT8006_TestItem.NOISE_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_NOISE_TEST);
    }

    //////////////////////////////////////////////////LCD_NOISE_TEST
    if ( g_stCfg_FT8006_TestItem.LCD_NOISE_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_LCD_NOISE_TEST);
    }

    //////////////////////////////////////////////////RawData Test
    if ( g_stCfg_FT8006_TestItem.RAWDATA_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_RAWDATA_TEST);
    }

    //////////////////////////////////////////////////SNR Test
    if ( g_stCfg_FT8006_TestItem.SNR_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8006_SNR_TEST);
    }


    //////////////////////////////////////////////////Differ_TEST
    if ( g_stCfg_FT8006_TestItem.DIFFER_TEST == 1)
    {

        fts_SetTestItemCodeName(Code_FT8006_DIFFER_TEST);
    }


    //////////////////////////////////////////////////DIFFER_UNIFORMITY_TEST
    if ( g_stCfg_FT8006_TestItem.DIFFER_UNIFORMITY_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_DIFFER_UNIFORMITY_TEST);
    }

    //////////////////////////////////////////////////RESET_PIN_TEST
    if ( g_stCfg_FT8006_TestItem.RESET_PIN_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_RESET_PIN_TEST);
    }

    //////////////////////////////////////////////////INT_PIN_TEST
    if ( g_stCfg_FT8006_TestItem.INT_PIN_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_RESET_PIN_TEST);
    }

    //////////////////////////////////////////////////RESET_PIN_TEST
    if ( g_stCfg_FT8006_TestItem.RESET_PIN_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_INT_PIN_TEST);
    }

    //////////////////////////////////////////////////LPWG_CB_TEST
    if ( g_stCfg_FT8006_TestItem.LPWG_CB_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_LPWG_CB_TEST);
    }

    //////////////////////////////////////////////////LPWG_NOISE_TEST
    if ( g_stCfg_FT8006_TestItem.LPWG_NOISE_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_LPWG_NOISE_TEST);
    }

    //////////////////////////////////////////////////LPWG_RAWDATA_TEST
    if ( g_stCfg_FT8006_TestItem.LPWG_RAWDATA_TEST == 1)
    {
        fts_SetTestItemCodeName(Code_FT8006_LPWG_RAWDATA_TEST);
    }



    FTS_TEST_FUNC_EXIT();

}

#endif

