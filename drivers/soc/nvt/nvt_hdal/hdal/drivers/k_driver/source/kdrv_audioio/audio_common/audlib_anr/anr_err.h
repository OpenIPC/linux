#ifndef __ANR_ERR_H__
#define __ANR_ERR_H__

#define ErrCode_101  (101)   // Input buffer size should be great than 256 
#define ErrCode_102  (102)   // Input buffer size should be multiple of 128 
#define ErrCode_103  (103)   // Wrong SamplingRate in Get_FFT_SizeExp()
#define ErrCode_104  (104)   // Wrong SamplingRate in Get_CB_Size()
#define ErrCode_105  (105)   // Wrong SamplingRate in AUD_ANR_Init()
#define ErrCode_106  (106)   // kf_bfly not support in kf_work()
#define ErrCode_107  (107)   // No more key in ANR_Key_Ring
#define ErrCode_108  (108)   // Wrong Key value when calling AUD_ANR_Main()
#define ErrCode_109  (109)   // Sampling Rate is not support in CB_Filter()
#define ErrCode_110  (110)   // Wrong Sampling Rate in GetMinUpdateInterval()
#define ErrCode_111  (111)   // Wrong Sampling Rate in GetMaxUpdateInterval()
#define ErrCode_112  (112)   // Option is not support in CB_Filter()
#define ErrCode_113  (113)   // Consequency in CB_Filter() is not support
#define ErrCode_114  (114)   // 

#endif