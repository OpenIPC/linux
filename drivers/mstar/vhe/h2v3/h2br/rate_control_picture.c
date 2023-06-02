/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------*/
#include <h2v3_def.h>
/* INF3: remove include files
#include <stdlib.h>
#include <stdio.h>
*/
//#include "ctu_tools.h"
#include "rate_control_picture.h"
#include "tools.h"
#include "sw_slice.h"
/* INF3: NOT use <math.h> */
#if 0
#include <math.h> /* for ln() */
#endif
#include "enccfg.h"
/* INF3: 64-bits division */
#if defined(LINUX_KERNEL)
#include <linux/math64.h>
#define DIV_S64(x,d)    div_s64((x),(d))
#define DIV_U64(x,d)    div_u64((x),(d))
#define DIV64_S64(x,d)  div64_s64((x),(d))
#define DIV64_U64(x,d)  div64_u64((x),(d))
#else
#define DIV_S64(x,d)    ((x)/(d))
#define DIV_U64(x,d)    ((x)/(d))
#define DIV64_S64(x,d)  ((x)/(d))
#define DIV64_U64(x,d)  ((x)/(d))
#endif

/*
 * Using only one leaky bucket (Multible buckets is supported by std).
 * Constant bit rate (CBR) operation, ie. leaky bucket drain rate equals
 * average rate of the stream, is enabled if RC_CBR_HRD = 1. Frame skiping and
 * filler data are minimum requirements for the CBR conformance.
 *
 * Constant HRD parameters:
 *   low_delay_hrd_flag = 0, assumes constant delay mode.
 *       cpb_cnt_minus1 = 0, only one leaky bucket used.
 *         (cbr_flag[0] = RC_CBR_HRD, CBR mode.)
 */
#ifdef ROI_SUPPORT
/* INF3: NOT used */
#include "HevcRoiModel.h"
#endif

/*------------------------------------------------------------------------------
  Module defines
------------------------------------------------------------------------------*/

/* Define this if strict bitrate control is needed, each window has a strict
 * bit budget. Otherwise a moving window is used for smoother quality.
*/
#define RC_WINDOW_STRICT

#ifdef TRACE_RC
#include <stdio.h>
FILE *fpRcTrc = NULL;
/* Select debug output: fpRcTrc or stdout */
#define DBGOUTPUT fpRcTrc
/* Select debug level: 0 = minimum, 2 = maximum */
#define DBG_LEVEL 2
#define DBG(l, str) if (l <= DBG_LEVEL) fprintf str
#else
#define DBG(l, str)
#endif

#define INITIAL_BUFFER_FULLNESS   60    /* Decoder Buffer in procents */
#define MIN_PIC_SIZE              50    /* Procents from picPerPic */

#define DIV(a, b)       ((b) ? ((a) + (SIGN(a) * (b)) / 2) / (b) : (a))
#define DSCY                      32 /* n * 32 */
#define DSCY_64                   32 /* n * 64 */

/* INF3: DIV with math64.h */
#define DIV_DIV_S64(a_i64,b_i32)    ((b_i32)?DIV_S64((a_i64)+(SIGN(a_i64)*(b_i32)/2),(b_i32)):(b_i32))
#define DIV_DIV_U64(a_i64,b_i32)    ((b_i32)?DIV_U64((a_i64)+(SIGN(a_i64)*(b_i32)/2),(b_i32)):(b_i32))
#define DIV_DIV64_S64(a_i64,b_i64)  ((b_i64)?DIV64_S64((a_i64)+(SIGN(a_i64)*(b_i64)/2),(b_i64)):(b_i64))

#define DSCBITPERMB               128 //128 /* bitPerMb scaler  */
#define I32_MAX           2147483647 /* 2 ^ 31 - 1 */
#define I64_MAX           9223372036854775807 /* 2 ^ 63 - 1 */
//#define QP_DELTA          2.0     /* Limit QP change between consecutive frames.*/
#define QP_DELTA          0x200     /* Limit QP change between consecutive frames.*/

//#define QP_DELTA_LIMIT    6.0     /* Threshold when above limit is abandoned. */
#define QP_DELTA_LIMIT    0x600     /* Threshold when above limit is abandoned. */



#define QP_FIXPOINT_0_POINT_1  0x1a


#define INTRA_QP_DELTA    (0)
#define WORD_CNT_MAX      65535


/*------------------------------------------------------------------------------
  Local structures
------------------------------------------------------------------------------*/
#if 0
/* q_step values scaled up by 4 and evenly rounded */
static const i32 q_step[53] = { 3, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11,
                                13, 14, 16, 18, 20, 23, 25, 28, 32, 36, 40, 45, 51, 57, 64, 72, 80, 90,
                                101, 114, 128, 144, 160, 180, 203, 228, 256, 288, 320, 360, 405, 456,
                                513, 577, 640, 720, 810, 896
                              };
#else
#if 0
static const i32 q_step[53] = { 2, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 9, 10, 11,
                                13, 14, 16, 18, 20, 22, 25, 28, 31, 35, 39, 44, 50, 56, 63, 71, 79, 88,
                                100, 112, 126, 141, 157, 177, 200, 224, 252, 283, 314, 354, 401, 448,
                                503, 566, 629, 707, 802, 896
                              };
#else
#if 0
static const float q_step[511] ={
  2.475138122,2.503897841,2.532991733,2.562423679,2.592197608,2.622317493,2.652787355,2.683611259,2.71479332 ,2.746337699,
2.778248606,2.8105303  ,2.843187089,2.876223331,2.909643437,2.943451865,2.977653128,3.012251791,3.047252471,3.082659839,
3.118478621,3.154713597,3.191369603,3.228451532,3.265964332,3.303913009,3.342302629,3.381138315,3.42042525 ,3.460168677,
3.5003739  ,3.541046286,3.582191262,3.623814319,3.665921013,3.708516964,3.751607855,3.795199439,3.839297532,3.883908021,
3.929036858,3.974690067,4.020873741,4.067594043,4.11485721 ,4.162669547,4.211037438,4.259967336,4.309465772,4.359539352,
4.410194759,4.461438754,4.513278175,4.565719942,4.618771052,4.672438587,4.726729708,4.781651661,4.837211777,4.893417471,
4.950276243,5.007795682,5.065983465,5.124847358,5.184395216,5.244634986,5.30557471 ,5.367222518,5.42958664 ,5.492675398,
5.556497212,5.621060599,5.686374177,5.752446663,5.819286873,5.88690373 ,5.955306256,6.024503581,6.094504941,6.165319678,
6.236957241,6.309427194,6.382739206,6.456903064,6.531928663,6.607826018,6.684605258,6.76227663 ,6.8408505  ,6.920337354,
7.0007478  ,7.082092572,7.164382523,7.247628638,7.331842027,7.417033928,7.50321571 ,7.590398877,7.678595064,7.767816041,
7.858073716,7.949380134,8.041747482,8.135188087,8.229714419,8.325339095,8.422074875,8.519934672,8.618931544,8.719078704,
8.820389519,8.922877508,9.026556351,9.131439883,9.237542104,9.344877173,9.453459416,9.563303323,9.674423555,9.786834942,
9.900552486,10.01559136,10.13196693,10.24969472,10.36879043,10.48926997,10.61114942,10.73444504,10.85917328,10.9853508 ,
11.11299442,11.2421212 ,11.37274835,11.50489333,11.63857375,11.77380746,11.91061251,12.04900716,12.18900988,12.33063936,
12.47391448,12.61885439,12.76547841,12.91380613,13.06385733,13.21565204,13.36921052,13.52455326,13.681701  ,13.84067471,
14.0014956 ,14.16418514,14.32876505,14.49525728,14.66368405,14.83406786,15.00643142,15.18079775,15.35719013,15.53563208,
15.71614743,15.89876027,16.08349496,16.27037617,16.45942884,16.65067819,16.84414975,17.03986934,17.23786309,17.43815741,
17.64077904,17.84575502,18.0531127 ,18.26287977,18.47508421,18.68975435,18.90691883,19.12660665,19.34884711,19.57366988,
19.80110497,20.03118273,20.26393386,20.49938943,20.73758086,20.97853995,21.22229884,21.46889007,21.71834656,21.97070159,
22.22598885,22.4842424 ,22.74549671,23.00978665,23.27714749,23.54761492,23.82122502,24.09801433,24.37801976,24.66127871,
24.94782897,25.23770878,25.53095683,25.82761225,26.12771465,26.43130407,26.73842103,27.04910652,27.363402  ,27.68134941,
28.0029912 ,28.32837029,28.65753009,28.99051455,29.32736811,29.66813571,30.01286284,30.36159551,30.71438026,31.07126416,
31.43229486,31.79752054,32.16698993,32.54075235,32.91885768,33.30135638,33.6882995 ,34.07973869,34.47572618,34.87631482,
35.28155808,35.69151003,36.1062254 ,36.52575953,36.95016842,37.37950869,37.81383766,38.25321329,38.69769422,39.14733977,
39.60220994,40.06236546,40.52786772,40.99877886,41.47516173,41.95707989,42.44459768,42.93778015,43.43669312,43.94140318,
44.45197769,44.96848479,45.49099342,46.0195733 ,46.55429498,47.09522984,47.64245005,48.19602865,48.75603953,49.32255742,
49.89565793,50.47541755,51.06191365,51.65522451,52.25542931,52.86260815,53.47684207,54.09821304,54.726804  ,55.36269883,
56.0059824 ,56.65674057,57.31506019,57.98102911,58.65473621,59.33627142,60.02572568,60.72319102,61.42876051,62.14252833,
62.86458973,63.59504107,64.33397986,65.0815047 ,65.83771535,66.60271276,67.376599  ,68.15947737,68.95145235,69.75262963,
70.56311615,71.38302007,72.21245081,73.05151907,73.90033683,74.75901738,75.62767532,76.50642658,77.39538844,78.29467953,
79.20441989,80.12473092,81.05573545,81.99755772,82.95032345,83.91415978,84.88919535,85.87556029,86.87338624,87.88280636,
88.90395538,89.93696959,90.98198684,92.0391466 ,93.10858997,94.19045967,95.2849001 ,96.3920573 ,97.51207906,98.64511484,
99.79131586,100.9508351,102.1238273,103.310449 ,104.5108586,105.7252163,106.9536841,108.1964261,109.453608 ,110.7253977,
112.0119648,113.3134811,114.6301204,115.9620582,117.3094724,118.6725428,120.0514514,121.446382 ,122.857521 ,124.2850567,
125.7291795,127.1900821,128.6679597,130.1630094,131.6754307,133.2054255,134.753198 ,136.3189547,137.9029047,139.5052593,
141.1262323,142.7660401,144.4249016,146.1030381,147.8006737,149.5180348,151.2553506,153.0128532,154.7907769,156.5893591,
158.4088398,160.2494618,162.1114709,163.9951154,165.9006469,167.8283196,169.7783907,171.7511206,173.7467725,175.7656127,
177.8079108,179.8739392,181.9639737,184.0782932,186.2171799,188.3809193,190.5698002,192.7841146,195.0241581,197.2902297,
199.5826317,201.9016702,204.2476546,206.620898 ,209.0217172,211.4504326,213.9073683,216.3928522,218.907216 ,221.4507953,
224.0239296,226.6269623,229.2602408,231.9241164,234.6189449,237.3450857,240.1029027,242.8927641,245.7150421,248.5701133,
251.4583589,254.3801643,257.3359194,260.3260188,263.3508614,266.410851 ,269.506396 ,272.6379095,275.8058094,279.0105185,
282.2524646,285.5320803,288.8498032,292.2060763,295.6013473,299.0360695,302.5107013,306.0257063,309.5815538,313.1787181,
316.8176796,320.4989237,324.2229418,327.9902309,331.8012938,335.6566391,339.5567814,343.5022412,347.4935449,351.5312254,
355.6158215,359.7478783,363.9279473,368.1565864,372.4343599,376.7618387,381.1396004,385.5682292,390.0483162,394.5804594,
399.1652635,403.8033404,408.4953092,413.2417961,418.0434344,422.9008652,427.8147365,432.7857043,437.814432 ,442.9015906,
448.0478592,453.2539246,458.5204815,463.8482329,469.2378897,474.6901714,480.2058055,485.7855282,491.4300841,497.1402266,
502.9167178,508.7603286,514.6718389,520.6520376,526.7017228,532.8217021,539.012792 ,545.275819 ,551.6116188,558.0210371,
564.5049292,571.0641605,577.6996065,584.4121525,591.2026947,598.0721391,605.0214026,612.0514127,619.1631075,626.3574363,
633.6353591,640.9978473,648.4458836,655.9804618,663.6025876,671.3132783,679.1135628,687.0044823,694.9870899,703.0624509,
711.2316431,719.4957567,727.8558947,736.3131728,744.8687198,753.5236774,762.2792008,771.1364584,780.0966325,789.1609187,
798.3305269,807.6066808,816.9906184,826.4835921,836.0868689,845.8017304,855.6294731,865.5714087,875.628864 ,885.8031813,
896.0957185

};
#else
static const i32 q_step[511] ={

0x279  ,0x280  ,0x288  ,0x28f  ,0x297  ,0x29f  ,0x2a7  ,0x2af  ,0x2b6  ,0x2bf  ,
0x2c7  ,0x2cf  ,0x2d7  ,0x2e0  ,0x2e8  ,0x2f1  ,0x2fa  ,0x303  ,0x30c  ,0x315  ,
0x31e  ,0x327  ,0x330  ,0x33a  ,0x344  ,0x34d  ,0x357  ,0x361  ,0x36b  ,0x375  ,
0x380  ,0x38a  ,0x395  ,0x39f  ,0x3aa  ,0x3b5  ,0x3c0  ,0x3cb  ,0x3d6  ,0x3e2  ,
0x3ed  ,0x3f9  ,0x405  ,0x411  ,0x41d  ,0x429  ,0x436  ,0x442  ,0x44f  ,0x45c  ,
0x469  ,0x476  ,0x483  ,0x490  ,0x49e  ,0x4ac  ,0x4ba  ,0x4c8  ,0x4d6  ,0x4e4  ,
0x4f3  ,0x501  ,0x510  ,0x51f  ,0x52f  ,0x53e  ,0x54e  ,0x55e  ,0x56d  ,0x57e  ,
0x58e  ,0x59e  ,0x5af  ,0x5c0  ,0x5d1  ,0x5e3  ,0x5f4  ,0x606  ,0x618  ,0x62a  ,
0x63c  ,0x64f  ,0x661  ,0x674  ,0x688  ,0x69b  ,0x6af  ,0x6c3  ,0x6d7  ,0x6eb  ,
0x700  ,0x715  ,0x72a  ,0x73f  ,0x754  ,0x76a  ,0x780  ,0x797  ,0x7ad  ,0x7c4  ,
0x7db  ,0x7f3  ,0x80a  ,0x822  ,0x83a  ,0x853  ,0x86c  ,0x885  ,0x89e  ,0x8b8  ,
0x8d2  ,0x8ec  ,0x906  ,0x921  ,0x93c  ,0x958  ,0x974  ,0x990  ,0x9ac  ,0x9c9  ,
0x9e6  ,0xa03  ,0xa21  ,0xa3f  ,0xa5e  ,0xa7d  ,0xa9c  ,0xabc  ,0xadb  ,0xafc  ,
0xb1c  ,0xb3d  ,0xb5f  ,0xb81  ,0xba3  ,0xbc6  ,0xbe9  ,0xc0c  ,0xc30  ,0xc54  ,
0xc79  ,0xc9e  ,0xcc3  ,0xce9  ,0xd10  ,0xd37  ,0xd5e  ,0xd86  ,0xdae  ,0xdd7  ,
0xe00  ,0xe2a  ,0xe54  ,0xe7e  ,0xea9  ,0xed5  ,0xf01  ,0xf2e  ,0xf5b  ,0xf89  ,
0xfb7  ,0xfe6  ,0x1015 ,0x1045 ,0x1075 ,0x10a6 ,0x10d8 ,0x110a ,0x113c ,0x1170 ,
0x11a4 ,0x11d8 ,0x120d ,0x1243 ,0x1279 ,0x12b0 ,0x12e8 ,0x1320 ,0x1359 ,0x1392 ,
0x13cd ,0x1407 ,0x1443 ,0x147f ,0x14bc ,0x14fa ,0x1538 ,0x1578 ,0x15b7 ,0x15f8 ,
0x1639 ,0x167b ,0x16be ,0x1702 ,0x1746 ,0x178c ,0x17d2 ,0x1819 ,0x1860 ,0x18a9 ,
0x18f2 ,0x193c ,0x1987 ,0x19d3 ,0x1a20 ,0x1a6e ,0x1abd ,0x1b0c ,0x1b5d ,0x1bae ,
0x1c00 ,0x1c54 ,0x1ca8 ,0x1cfd ,0x1d53 ,0x1dab ,0x1e03 ,0x1e5c ,0x1eb6 ,0x1f12 ,
0x1f6e ,0x1fcc ,0x202a ,0x208a ,0x20eb ,0x214d ,0x21b0 ,0x2214 ,0x2279 ,0x22e0 ,
0x2348 ,0x23b1 ,0x241b ,0x2486 ,0x24f3 ,0x2561 ,0x25d0 ,0x2640 ,0x26b2 ,0x2725 ,
0x279a ,0x280f ,0x2887 ,0x28ff ,0x2979 ,0x29f5 ,0x2a71 ,0x2af0 ,0x2b6f ,0x2bf0 ,
0x2c73 ,0x2cf7 ,0x2d7d ,0x2e05 ,0x2e8d ,0x2f18 ,0x2fa4 ,0x3032 ,0x30c1 ,0x3152 ,
0x31e5 ,0x3279 ,0x330f ,0x33a7 ,0x3441 ,0x34dc ,0x357a ,0x3619 ,0x36ba ,0x375c ,
0x3801 ,0x38a8 ,0x3950 ,0x39fb ,0x3aa7 ,0x3b56 ,0x3c06 ,0x3cb9 ,0x3d6d ,0x3e24 ,
0x3edd ,0x3f98 ,0x4055 ,0x4114 ,0x41d6 ,0x429a ,0x4360 ,0x4428 ,0x44f3 ,0x45c0 ,
0x4690 ,0x4762 ,0x4836 ,0x490d ,0x49e6 ,0x4ac2 ,0x4ba0 ,0x4c81 ,0x4d65 ,0x4e4b ,
0x4f34 ,0x501f ,0x510e ,0x51ff ,0x52f3 ,0x53ea ,0x54e3 ,0x55e0 ,0x56df ,0x57e1 ,
0x58e7 ,0x59ef ,0x5afb ,0x5c0a ,0x5d1b ,0x5e30 ,0x5f48 ,0x6064 ,0x6183 ,0x62a5 ,
0x63ca ,0x64f3 ,0x661f ,0x674f ,0x6882 ,0x69b9 ,0x6af4 ,0x6c32 ,0x6d74 ,0x6eb9 ,
0x7003 ,0x7150 ,0x72a1 ,0x73f6 ,0x754f ,0x76ac ,0x780d ,0x7972 ,0x7adb ,0x7c48 ,
0x7dba ,0x7f30 ,0x80aa ,0x8229 ,0x83ac ,0x8534 ,0x86c0 ,0x8851 ,0x89e7 ,0x8b81 ,
0x8d20 ,0x8ec4 ,0x906c ,0x921a ,0x93cc ,0x9584 ,0x9741 ,0x9903 ,0x9aca ,0x9c96 ,
0x9e68 ,0xa03f ,0xa21c ,0xa3fe ,0xa5e6 ,0xa7d4 ,0xa9c7 ,0xabc0 ,0xadbf ,0xafc3 ,
0xb1ce ,0xb3df ,0xb5f6 ,0xb814 ,0xba37 ,0xbc61 ,0xbe91 ,0xc0c8 ,0xc306 ,0xc54a ,
0xc795 ,0xc9e6 ,0xcc3f ,0xce9e ,0xd105 ,0xd373 ,0xd5e8 ,0xd864 ,0xdae8 ,0xdd73 ,
0xe006 ,0xe2a0 ,0xe542 ,0xe7ec ,0xea9e ,0xed58 ,0xf01a ,0xf2e4 ,0xf5b7 ,0xf891 ,
0xfb75 ,0xfe61 ,0x10155,0x10453,0x10759,0x10a69,0x10d81,0x110a3,0x113ce,0x11702,
0x11a40,0x11d88,0x120d9,0x12434,0x12799,0x12b09,0x12e82,0x13206,0x13594,0x1392d,
0x13cd1,0x1407f,0x14439,0x147fd,0x14bcd,0x14fa8,0x1538e,0x15780,0x15b7e,0x15f87,
0x1639d,0x167bf,0x16bed,0x17028,0x1746f,0x178c3,0x17d23,0x18191,0x1860c,0x18a94,
0x18f2a,0x193cd,0x1987e,0x19d3d,0x1a20b,0x1a6e6,0x1abd0,0x1b0c9,0x1b5d0,0x1bae6,
0x1c00c,0x1c541,0x1ca85,0x1cfd9,0x1d53c,0x1dab0,0x1e034,0x1e5c9,0x1eb6e,0x1f123,
0x1f6ea,0x1fcc2,0x202ab,0x208a6,0x20eb3,0x214d2,0x21b03,0x22146,0x2279c,0x22e05,
0x23481,0x23b10,0x241b3,0x24869,0x24f33,0x25612,0x25d05,0x2640d,0x26b29,0x2725b,
0x279a2,0x280ff,0x28872,0x28ffb,0x2979a,0x29f50,0x2a71d,0x2af01,0x2b6fc,0x2bf0f,
0x2c73b,0x2cf7e,0x2d7db,0x2e050,0x2e8de,0x2f186,0x2fa47,0x30322,0x30c18,0x31529,
0x31e54,0x3279b,0x330fd,0x33a7b,0x34416,0x34dcd,0x357a1,0x36192,0x36ba0,0x375cd,
0x38018
};
#endif
#endif
#endif
/*------------------------------------------------------------------------------
  Local function prototypes
------------------------------------------------------------------------------*/

static i32 InitialQp(i32 bits, i32 pels);
static void SourceParameter(hevcRateControl_s *rc, i32 nonZeroCnt);
static void PicSkip(hevcRateControl_s *rc);
static void PicQuantLimit(hevcRateControl_s *rc);
static i32 VirtualBuffer(hevcVirtualBuffer_s *vb, i32 timeInc, true_e hrd);
static void PicQuant(hevcRateControl_s *rc);
static i32 avg_rc_error(linReg_s *p);
static void update_rc_error(linReg_s *p, i32 bits, i32 windowLength);
static i32 gop_avg_qp(hevcRateControl_s *rc);
//static i32 new_pic_quant(linReg_s *p, i32 bits, true_e useQpDeltaLimit);
static i32 new_pic_quant(linReg_s *p, i32 bits, true_e useQpDeltaLimit,i32 lastActualBits, i32 lastTargetBits, i32 currentTargetBits);
static i32 get_avg_bits(linReg_s *p, i32 n);
static void update_tables(linReg_s *p, i32 qp, i32 bits);
static void update_model(linReg_s *p);
static i64 lin_sy(i32 *qp, i32 *r, i32 n);
static i64 lin_sx(i32 *qp, i32 n);
static i64 lin_sxy(i32 *qp, i32 *r, i32 n);
static i64 lin_nsxx(i32 *qp, i32 n);
static void init_hierarchial_rc (hevcRateControl_s *rc);
static void MaAddFrame(rc_ma_s *ma, i32 frameSizeBits);
static i32 moving_totalBit_window_minus1(rc_ma_s *ma);
/* INF3: kill warnings */
#if 0
static void MbQuant(hevcRateControl_s *rc);
static void LinearModel(hevcRateControl_s *rc);
static void AdaptiveModel(hevcRateControl_s *rc);
static i32 Ma(rc_ma_s *ma);
#endif
/*------------------------------------------------------------------------------

  HevcInitRc() Initialize rate control.

------------------------------------------------------------------------------*/
bool_e HevcInitRc(hevcRateControl_s *rc, u32 newStream)
{
  hevcVirtualBuffer_s *vb = &rc->virtualBuffer;
  i32 i;

  if ((rc->qpMax > (51 << QP_FRACTIONAL_BITS)))
  {
    return ENCHW_NOK;
  }
  //bitPerPic
  rc->bpp=HevcCalculate(vb->bitRate, rc->outRateDenom, rc->outRateNum);
  rc->bpp=rc->bpp*100/rc->picArea;
  //printf("bpp=%d\n",rc->bpp);
  init_hierarchial_rc (rc);
  if(rc->hierarchial_bit_allocation_GOP_size>8)
    rc->hierarchial_bit_allocation_GOP_size=8;
  else if(rc->hierarchial_bit_allocation_GOP_size<=0)
    rc->hierarchial_bit_allocation_GOP_size=1;

  rc->hierarchial_bit_allocation_GOP_size_previous=rc->hierarchial_bit_allocation_GOP_size;
  rc->hierarchial_bit_allocation_total_weight=0;
  for(i=0;i<rc->hierarchial_bit_allocation_GOP_size;i++)
  {
    rc->hierarchial_bit_allocation_total_weight+=rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][i];
  }

  //CTB_RC
  rc->seqStart = 1;


  /* QP -1: Initial QP estimation done by RC */
  if (rc->qpHdr == (-1 << QP_FRACTIONAL_BITS))
  {
    i32 tmp = HevcCalculate(vb->bitRate, rc->outRateDenom, rc->outRateNum);
    rc->qpHdr = InitialQp(tmp, rc->picArea/*rc->ctbPerPic*rc->ctbSize*rc->ctbSize*/);
    PicQuantLimit(rc);
  }

  if ((rc->qpHdr > rc->qpMax) || (rc->qpHdr < rc->qpMin))
  {
    return ENCHW_NOK;
  }

  /* HRD needs frame RC and macroblock RC*/
  if (rc->hrd == ENCHW_YES)
  {
    rc->picRc = ENCHW_YES;
  }

  //rc->ctbRc = ENCHW_NO; //CTB_RC
  rc->coeffCntMax = rc->ctbPerPic * rc->ctbSize * rc->ctbSize * 3 / 2;
  rc->frameCoded = ENCHW_YES;
  rc->sliceTypeCur = I_SLICE;
  rc->sliceTypePrev = P_SLICE;
  rc->sliceEncodedTypePrev = P_SLICE;
  rc->frameBitCntLast = 0;
  rc->targetPicSizeLast = 0;
  rc->upBase = 0;

  rc->qpHdrPrev  = rc->qpHdr;
  rc->fixedQp    = rc->qpHdr;

  vb->bitPerPic = HevcCalculate(vb->bitRate, rc->outRateDenom, rc->outRateNum);
  rc->qpCtrl.checkPointDistance = 0;
  rc->qpCtrl.checkPoints = 0;

#if defined(TRACE_RC) && (DBGOUTPUT == fpRcTrc)
  if (!fpRcTrc) fpRcTrc = fopen("rc.trc", "wt");
#endif

  /* new rate control algorithm */
  /* API parameter is named gopLen but the actual usage is rate controlling
   * window in frames. RC tries to match the target bitrate inside the
   * window. Each window can contain multiple GOPs and the RC adapts to the
   * intra rate by calculating intraInterval. */

  DBG(0, (DBGOUTPUT, "\nInitRc: picRc\t\t%i  hrd\t%i  picSkip\t%i\n",
          rc->picRc, rc->hrd, rc->picSkip));
  DBG(0, (DBGOUTPUT, "  mbRc\t\t\t%i  qpHdr\t%i  Min,Max\t%i,%i\n",
          rc->mbRc, rc->qpHdr, rc->qpMin, rc->qpMax));
  DBG(0, (DBGOUTPUT, "  checkPointDistance\t%i\n",
          rc->qpCtrl.checkPointDistance));

  DBG(0, (DBGOUTPUT, "  CPBsize\t%i\n BitRate\t%i\n BitPerPic\t%i\n",
          vb->bufferSize, vb->bitRate, vb->bitPerPic));

  /* If changing QP between frames don't reset GOP RC.
   * Changing bitrate resets RC the same way as new stream. */
  if (!newStream) return ENCHW_OK;

  /* new rate control algorithm */
  //init error for P B B B
  for(i=0;i<4;i++)
  {
    update_rc_error(&rc->rError[i], 0x7fffffff, 0);
  }
  //init error for I
  update_rc_error(&rc->intraError, 0x7fffffff, 0);
  //init model for P B B B
  for(i=0;i<4;i++)
  {
    memset(&rc->linReg[i], 0, sizeof(linReg_s));
    rc->linReg[i].qs[0]   = q_step[510];
    rc->linReg[i].qp_prev = rc->qpHdr;
    rc->linReg[i].weight = 10;
  }
  //init model for I
  memset(&rc->intra, 0, sizeof(linReg_s));
  rc->intra.qs[0]   = q_step[510];
  rc->intra.qp_prev = rc->qpHdr;
    /* API parameter is named gopLen but the actual usage is rate controlling
   * window in frames. RC tries to match the target bitrate inside the
   * window. Each window can contain multiple GOPs and the RC adapts to the
   * intra rate by calculating intraInterval. */
  rc->windowLen = rc->gopLen;
  vb->windowRem = rc->gopLen;
  rc->intraIntervalCtr = rc->intraInterval = rc->gopLen;
  rc->targetPicSize = 0;
  rc->frameBitCnt = 0;
  rc->gopQpSum = 0;
  rc->gopQpDiv = 0;

  vb->picTimeInc      = 0;
  vb->realBitCnt      = 0;
  vb->virtualBitCnt   = 0;

  rc->ma.pos = rc->ma.count = 0;
  rc->ma.frameRateNumer = rc->outRateNum;
  rc->ma.frameRateDenom = rc->outRateDenom;
  if (rc->outRateDenom)
    rc->ma.length = MAX(1, MIN(rc->monitorFrames,RC_MOVING_AVERAGE_FRAMES));
  else
    rc->ma.length = RC_MOVING_AVERAGE_FRAMES;
/* INF3: float 2 integer */
  rc->virtualBuffer.maxBitRate=(i32)(rc->virtualBuffer.bitRate+(int)DIV_S64((i64)rc->tolMovingBitRate*rc->virtualBuffer.bitRate,100));
  rc->virtualBuffer.minBitRate=(i32)(rc->virtualBuffer.bitRate-(int)DIV_S64((i64)rc->tolMovingBitRate*rc->virtualBuffer.bitRate,100));
//rc->virtualBuffer.maxBitRate=(i32)(rc->virtualBuffer.bitRate+rc->f_tolMovingBitRate*rc->virtualBuffer.bitRate/100);
//rc->virtualBuffer.minBitRate=(i32)(rc->virtualBuffer.bitRate-rc->f_tolMovingBitRate*rc->virtualBuffer.bitRate/100);
  if (rc->hrd)
  {
    vb->bucketFullness =
      HevcCalculate(vb->bufferSize, INITIAL_BUFFER_FULLNESS, 100);
    rc->gDelaySum = HevcCalculate(90000, vb->bufferSize, vb->bitRate);
    rc->gInitialDelay = HevcCalculate(90000, vb->bucketFullness, vb->bitRate);
    rc->gInitialDoffs = rc->gDelaySum - rc->gInitialDelay;
    vb->bucketFullness = vb->bucketLevel =
                           vb->bufferSize - vb->bucketFullness;
#ifdef TRACE_RC
    rc->gBufferMin = vb->bufferSize;
    rc->gBufferMax = 0;
#endif
    rc->sei.icrd = (u32)rc->gInitialDelay;
    rc->sei.icrdo = (u32)rc->gInitialDoffs;

    DBG(1, (DBGOUTPUT, "\n InitialDelay\t%i\n Offset\t\t%i\n",
            rc->gInitialDelay, rc->gInitialDoffs));
  }

  rc->rcPicComplexity = 12* rc->ctbPerPic * 8*8;
  return ENCHW_OK;
}

/*------------------------------------------------------------------------------

  InitialQp()  Returns sequence initial quantization parameter.

------------------------------------------------------------------------------*/
#define OFFSET_QP 0
static i32 InitialQp(i32 bits, i32 pels)
{
  const i32 qp_tbl[2][36] =
  {
    /*{27, 32, 36, 40, 44, 51, 58, 65, 72, 84, 96, 108, 119, 138, 156, 174, 192, 223, 253, 285, 314, 349, 384, 419, 453, 503, 553, 603, 653, 719, 784, 864, 0x7FFFFFFF},*/
    {16,19,23, 27, 32, 36, 40, 44, 51, 58, 65, 72, 84, 96, 108, 119, 138, 156, 174, 192, 223, 253, 285, 314, 349, 384, 419, 453, 503, 553, 603, 653, 719, 784, 864, 0x7FFFFFFF},
    /*{26, 38, 59, 96, 173, 305, 545, 0x7FFFFFFF},*/
    {51,50,49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16}
  };
  const i32 upscale = 20000;
  i32 i = -1;

  /* prevents overflow, QP would anyway be 17 with this high bitrate
     for all resolutions under and including 1920x1088 */
  //    if (bits > 1000000)
  //      return 17;

  /* Make room for multiplication */
  pels >>= 8;
  bits >>= 5;

  /* Use maximum QP if bitrate way too low. */
  if (!bits)
    return 51<<QP_FRACTIONAL_BITS;

  /* Adjust the bits value for the current resolution */
  bits *= pels + 250;
  ASSERT(pels > 0);
  ASSERT(bits > 0);
  bits /= 350 + (3 * pels) / 4;
  bits = HevcCalculate(bits, upscale, pels << 6);

  while ((qp_tbl[0][++i]-OFFSET_QP) < bits);

  DBG(0, (DBGOUTPUT, "BPP\t\t%d\n", bits));

  return qp_tbl[1][i]<<QP_FRACTIONAL_BITS;
}

/*------------------------------------------------------------------------------

      HevcFillerRc

      Stream watermarking. Insert filler NAL unit of certain size after each
      Nth frame.

------------------------------------------------------------------------------*/
u32 HevcFillerRc(hevcRateControl_s *rc, u32 frameCnt)
{
  const u8 filler[] = { 0, 9, 0, 9, 9, 9, 0, 2, 2, 0 };
  u32 idx;

  if (rc->fillerIdx == (u32)(-1))
  {
    rc->fillerIdx = sizeof(filler) / sizeof(*filler) - 1;
  }

  idx = rc->fillerIdx;
  if (frameCnt != 0 && ((frameCnt % 128) == 0))
  {
    idx++;
  }
  idx %= sizeof(filler) / sizeof(*filler);

  if (idx != rc->fillerIdx)
  {
    rc->fillerIdx = idx;
    return filler[idx] + 1;
  }
  return 0;
}
/*------------------------------------------------------------------------------
  VirtualBuffer()  Return difference of target and real buffer fullness.
  Virtual buffer and real bit count grow until one second.  After one second
  output bit rate per second is removed from virtualBitCnt and realBitCnt. Bit
  drifting has been taken care.

  If the leaky bucket in VBR mode becomes empty (e.g. underflow), those R * T_e
  bits are lost and must be decremented from virtualBitCnt. (NOTE: Drift
  calculation will mess virtualBitCnt up, so the loss is added to realBitCnt)
------------------------------------------------------------------------------*/
static i32 VirtualBuffer(hevcVirtualBuffer_s *vb, i32 timeInc, true_e hrd)
{
  i32 target;

  vb->picTimeInc    += timeInc;

  /* picTimeInc must be in range of [0, timeScale) */
  while (vb->picTimeInc >= vb->timeScale)
  {
    vb->picTimeInc    -= vb->timeScale;
    vb->realBitCnt    -= vb->bitRate;
    vb->bucketLevel   -= vb->bitRate;
    vb->seconds++;
    vb->averageBitRate = vb->bitRate + vb->realBitCnt / vb->seconds;
  }
  vb->virtualBitCnt = HevcCalculate(vb->bitRate, vb->picTimeInc, vb->timeScale);
  //printf("vb->virtualBitCnt=%d,vb->picTimeInc=%d,vb->timeScale=%d,vb->bitRate=%d\n",vb->virtualBitCnt,vb->picTimeInc,vb->timeScale,vb->bitRate);

  if (hrd)
  {
#if RC_CBR_HRD
    /* In CBR mode, bucket _must not_ underflow. Insert filler when
     * needed. */
    vb->bucketFullness = vb->bucketLevel - vb->virtualBitCnt;
#else
    if (vb->bucketLevel >= vb->virtualBitCnt)
    {
      vb->bucketFullness = vb->bucketLevel - vb->virtualBitCnt;
    }
    else
    {
      vb->bucketFullness = 0;
      vb->realBitCnt += vb->virtualBitCnt - vb->bucketLevel;
      vb->bucketLevel = vb->virtualBitCnt;
    }
#endif
  }

  /* Saturate realBitCnt, this is to prevent overflows caused by much greater
     bitrate setting than is really possible to reach */
  if (vb->realBitCnt > 0x1FFFFFFF)
    vb->realBitCnt = 0x1FFFFFFF;
  if (vb->realBitCnt < -0x1FFFFFFF)
    vb->realBitCnt = -0x1FFFFFFF;

  target = vb->virtualBitCnt - vb->realBitCnt;
  //printf("target=%d,vb->realBitCnt=%d\n",target,vb->realBitCnt);
  /* Saturate target, prevents rc going totally out of control.
     This situation should never happen. */
  if (target > 0x1FFFFFFF)
    target = 0x1FFFFFFF;
  if (target < -0x1FFFFFFF)
    target = -0x1FFFFFFF;


  DBG(1, (DBGOUTPUT, "virtualBitCnt:\t%8i  realBitCnt:\t%8i",
          vb->virtualBitCnt, vb->realBitCnt));
  DBG(1, (DBGOUTPUT, "  diff bits:\t%8i  avg bitrate:\t%8i\n", target,
          vb->averageBitRate));
  return target;
}

/*------------------------------------------------------------------------------
  HevcAfterPicRc()  Update source model, bit rate target error and linear
  regression model for frame QP calculation. If HRD enabled, check leaky bucket
  status and return RC_OVERFLOW if coded frame must be skipped. Otherwise
  returns number of required filler payload bytes.
------------------------------------------------------------------------------*/
i32 HevcAfterPicRc(hevcRateControl_s *rc, u32 nonZeroCnt, u32 byteCnt,
                   u32 qpSum,u32 qpNum)
{
  hevcVirtualBuffer_s *vb = &rc->virtualBuffer;
  i32 bitPerPic = rc->virtualBuffer.bitPerPic;
  i32 stat, bitCnt = (i32)byteCnt * 8;
//#if RC_CBR_HRD
  i32 tmp;
//#endif
  /* INF3: kill warnings
  i32 movingBitRate,bestBits,movingBitBase,realBitCnt,targetBitCnt;
  */

  (void) bitPerPic;
  rc->qpSum = (i32)qpSum;
  rc->qpNum = (i32)qpNum;
  /* INF3: not used
  rc->averageQp = (float)qpSum/qpNum;
  */
  //printf("average QP= %f\n",rc->averageQp);
  rc->frameBitCnt = bitCnt;
  rc->nonZeroCnt = nonZeroCnt;
  rc->gopBitCnt += bitCnt;
  rc->frameCnt++;

  //printf("I frame qp=%d,bitPerPixel=%f\n",rc->qpHdr,(float)bitCnt/rc->picArea);
  MaAddFrame(&rc->ma, bitCnt);


  //printf("\n rc->frameBitCnt=%d,rc->gopBitCnt=%d,bitPerPic=%d\n",rc->frameBitCnt,rc->gopBitCnt,bitPerPic);
//#if RC_CBR_HRD
  if (rc->targetPicSize)
  {
    tmp = ((bitCnt - rc->targetPicSize) * 100) /
          rc->targetPicSize;
  }
  else
  {
    tmp = 0;
  }
//#endif

  DBG(1, (DBGOUTPUT, "\nAFTER PIC RC:\n"));
  DBG(0, (DBGOUTPUT, "BitCnt\t\t%8d\n", bitCnt));
  DBG(1, (DBGOUTPUT, "BitErr/avg\t%7d%%  ",
          ((bitCnt - bitPerPic) * 100) / (bitPerPic + 1)));
//#if RC_CBR_HRD
  DBG(1, (DBGOUTPUT, "BitErr/target\t%7i%%  qpHdr %2i  avgQp %2i\n",
          tmp, rc->qpHdr, rc->qpSum / rc->mbPerPic));
//#endif

  /* Calculate the source parameter only for INTER frames */
  if (rc->sliceTypeCur != I_SLICE)
    SourceParameter(rc, rc->nonZeroCnt);
  /* Store the error between target and actual frame size in percentage.
   * Saturate the error to avoid inter frames with mostly intra MBs
   * to affect too much. */
  if (rc->sliceTypeCur == P_SLICE||(rc->sliceTypeCur==B_SLICE&&rc->hierarchial_decoding_order[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]==0))
  {
    //if((rc->averageQp > (rc->qpHdrPrev-7))&&(bitCnt*20>rc->targetPicSize))
    {// not still picture

      {
        update_rc_error(&rc->rError[0],
                        bitCnt - rc->targetPicSize, rc->windowLen);
      }

    }
  }
  else if(rc->sliceTypeCur == I_SLICE)
  {
    update_rc_error(&rc->intraError,
                    bitCnt - rc->targetPicSize, rc->windowLen);
  }
#if 1
  else
  {
    //if((rc->averageQp > (rc->qpHdrPrev-7))&&(bitCnt*20>rc->targetPicSize))
    { // not still picture
      update_rc_error(&rc->rError[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]],
                      bitCnt - rc->targetPicSize, rc->windowLen);

    }
  }
#endif
  if(rc->sliceTypeCur == P_SLICE)
  {
    {
      if(rc->upBase == 0)
        rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]].weight = 10;
      else if((rc->upBase == 1)&&(rc->frameBitCnt < rc->targetPicSize))
      {
        rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]].weight = 10;
      }
      else if(((rc->upBase == 2)&&(rc->frameBitCnt > rc->targetPicSize)))
      {
        rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]].weight = 10;
      }
      else if((rc->upBase == 1)&&(rc->frameBitCnt > rc->targetPicSize))
      {
        rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]].weight += 10;
      }
      else if(((rc->upBase == 2)&&(rc->frameBitCnt < rc->targetPicSize)))
      {
        rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]].weight += 10;
      }
    }
  }

  //rc->linReg.weight = 100;
  //printf("weight=%d\n",rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]].weight);

  /* Update number of bits used for residual, inter or intra */
  if (rc->sliceTypeCur == P_SLICE||(rc->sliceTypeCur==B_SLICE&&rc->hierarchial_decoding_order[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]==0))
  {
    /* INF3: kill warninigs */
    //i32 realtimebitrate;
    //printf("p update model rc->qpHdrPrev=%d,bitCnt=%d",rc->qpHdrPrev,bitCnt);
    //if(rc->hierarchial_bit_allocation_GOP_size>1)

    {
      {
        update_tables(&rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]], rc->qpHdrPrev,
                      HevcCalculate(bitCnt, DSCBITPERMB, rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16));
        update_model(&rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]]);
      }
    }
  }
  else if(rc->sliceTypeCur == I_SLICE)
  {
    update_tables(&rc->intra, rc->qpHdrPrev,
                  HevcCalculate(bitCnt, DSCBITPERMB, rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16));
    update_model(&rc->intra);
  }
  else
  {
    {
      //printf("B update model rc->qpHdrPrev=%d,bitCnt=%d",rc->qpHdrPrev,bitCnt);
      update_tables(&rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]], rc->qpHdrPrev,
                    HevcCalculate(bitCnt, DSCBITPERMB, rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16));
      update_model(&rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]]);
    }
  }

  /* Post-frame skip if HRD buffer overflow */
  if ((rc->hrd == ENCHW_YES) && (bitCnt > (vb->bufferSize - vb->bucketFullness)))
  {
    DBG(1, (DBGOUTPUT, "Be: %7i  ", vb->bucketFullness));
    DBG(1, (DBGOUTPUT, "fillerBits %5i  ", 0));
    DBG(1, (DBGOUTPUT, "bitCnt %d  spaceLeft %d  ",
            bitCnt, (vb->bufferSize - vb->bucketFullness)));
    DBG(1, (DBGOUTPUT, "bufSize %d  bucketFullness %d  bitPerPic %d\n",
            vb->bufferSize, vb->bucketFullness, bitPerPic));
    DBG(0, (DBGOUTPUT, "HRD overflow, frame discard\n"));
    rc->frameCoded = ENCHW_NO;
    return HEVCRC_OVERFLOW;
  }
  else
  {
    vb->bucketLevel += bitCnt;
    vb->bucketFullness += bitCnt;
    vb->realBitCnt += bitCnt;
  }
  rc->sliceTypePrev = rc->sliceTypeCur;
  //rc->frameBitCntLast = rc->frameBitCnt;

  if(rc->sliceTypeCur == P_SLICE)
  {
    if(rc->frameBitCnt == rc->targetPicSize)
    {
      rc->upBase = 0;
    }
    else if(rc->frameBitCnt > rc->targetPicSize)
    {
      rc->upBase = 1;
    }
    else
    {
      rc->upBase = 2;
    }
    rc->frameBitCntLast = rc->frameBitCnt;
    rc->targetPicSizeLast = rc->targetPicSize;
  }

  DBG(1, (DBGOUTPUT, "plot\t%4i\t%4i\t%8i\t%8i\t%8i\t%8i\t%8i\t%8i\t%8i\n",
          rc->frameCnt, rc->qpHdr, rc->targetPicSize, bitCnt,
          bitPerPic, rc->gopAvgBitCnt, vb->realBitCnt - vb->virtualBitCnt,
          vb->bitRate, vb->bucketFullness));

  if (rc->hrd == ENCHW_NO)
  {
    return 0;
  }

  tmp = 0;

#if RC_CBR_HRD
  /* Bits needed to prevent bucket underflow */
  tmp = bitPerPic - vb->bucketFullness;

  if (tmp > 0)
  {
    tmp = (tmp + 7) / 8;
    vb->bucketFullness += tmp * 8;
    vb->realBitCnt += tmp * 8;
  }
  else
  {
    tmp = 0;
  }
#endif

  /* Update Buffering Info */
  stat = vb->bufferSize - vb->bucketFullness;

  rc->gInitialDelay = HevcCalculate(90000, stat, vb->bitRate);
  rc->gInitialDoffs = rc->gDelaySum - rc->gInitialDelay;

  rc->sei.icrd  = (u32)rc->gInitialDelay;
  rc->sei.icrdo = (u32)rc->gInitialDoffs;

  DBG(1, (DBGOUTPUT, "initialDelay: %5i  ", rc->gInitialDelay));
  DBG(1, (DBGOUTPUT, "initialDoffs: %5i\n", rc->gInitialDoffs));
  DBG(1, (DBGOUTPUT, "Be: %7i  ", vb->bucketFullness));
  DBG(1, (DBGOUTPUT, "fillerBits %5i\n", tmp * 8));

#ifdef TRACE_RC
  if (vb->bucketFullness < rc->gBufferMin)
  {
    rc->gBufferMin = vb->bucketFullness;
  }
  if (vb->bucketFullness > rc->gBufferMax)
  {
    rc->gBufferMax = vb->bucketFullness;
  }
  DBG(1, (DBGOUTPUT, "\nLeaky Bucket Min: %i (%d%%)  Max: %i (%d%%)\n",
          rc->gBufferMin, rc->gBufferMin * 100 / vb->bufferSize,
          rc->gBufferMax, rc->gBufferMax * 100 / vb->bufferSize));
#endif
//#if RC_CBR_HRD
  return tmp;
//#else
//  return 0;
//#endif
}

static void init_hierarchial_rc (hevcRateControl_s *rc)
{
  //if(rc->hierarchial_bit_allocation_GOP_size==1)
  {
    if(rc->bpp>20)
    {
      rc->hierarchial_bit_allocation_map[0][0]=6;
    }
    else if(rc->bpp>10)
    {
      rc->hierarchial_bit_allocation_map[0][0]=10;
    }
    else if(rc->bpp>5)
    {
      rc->hierarchial_bit_allocation_map[0][0]=12;
    }
    else
    {
      rc->hierarchial_bit_allocation_map[0][0]=14;
    }
    rc->hierarchial_alg_map[0][0]=0;
    rc->hierarchial_decoding_order[0][0]=0;  // poc order : decoding order
  }
  //else if(rc->hierarchial_bit_allocation_GOP_size==2)
  {
    if(rc->bpp>20)
    {
      rc->hierarchial_bit_allocation_map[1][1]=6;
      rc->hierarchial_bit_allocation_map[1][0]=2;
    }
    else if(rc->bpp>10)
    {
      rc->hierarchial_bit_allocation_map[1][1]=10;
      rc->hierarchial_bit_allocation_map[1][0]=2;
    }
    else if(rc->bpp>5)
    {
      rc->hierarchial_bit_allocation_map[1][1]=12;
      rc->hierarchial_bit_allocation_map[1][0]=2;
    }
    else
    {
      rc->hierarchial_bit_allocation_map[1][1]=14;
      rc->hierarchial_bit_allocation_map[1][0]=2;
    }
    rc->hierarchial_alg_map[1][1]=0;
    rc->hierarchial_alg_map[1][0]=2;
    rc->hierarchial_decoding_order[1][1]=0;
    rc->hierarchial_decoding_order[1][0]=1;
  }
  //else if(rc->hierarchial_bit_allocation_GOP_size==3)
  {
    if(rc->bpp>20)
    {
      rc->hierarchial_bit_allocation_map[2][2]=6;
      rc->hierarchial_bit_allocation_map[2][0]=2;
      rc->hierarchial_bit_allocation_map[2][1]=3;
    }
    else if(rc->bpp>10)
    {
      rc->hierarchial_bit_allocation_map[2][2]=10;
      rc->hierarchial_bit_allocation_map[2][0]=2;
      rc->hierarchial_bit_allocation_map[2][1]=3;
    }
    else if(rc->bpp>5)
    {
      rc->hierarchial_bit_allocation_map[2][2]=12;
      rc->hierarchial_bit_allocation_map[2][0]=2;
      rc->hierarchial_bit_allocation_map[2][1]=3;
    }
    else
    {
      rc->hierarchial_bit_allocation_map[2][2]=14;
      rc->hierarchial_bit_allocation_map[2][0]=2;
      rc->hierarchial_bit_allocation_map[2][1]=3;
    }
    rc->hierarchial_alg_map[2][2]=0;
    rc->hierarchial_alg_map[2][1]=2;
    rc->hierarchial_alg_map[2][0]=2;
    rc->hierarchial_decoding_order[2][2]=0;
    rc->hierarchial_decoding_order[2][0]=1;
    rc->hierarchial_decoding_order[2][1]=2;
  }
  //else if(rc->hierarchial_bit_allocation_GOP_size==4)
  {
    if(rc->bpp>20)
    {
      rc->hierarchial_bit_allocation_map[3][3] = 12;
      rc->hierarchial_bit_allocation_map[3][1] = 5;
      rc->hierarchial_bit_allocation_map[3][0] = 3;
      rc->hierarchial_bit_allocation_map[3][2] = 3;
    }
    else if(rc->bpp>10)
    {
      rc->hierarchial_bit_allocation_map[3][3] = 15;
      rc->hierarchial_bit_allocation_map[3][1] = 6;
      rc->hierarchial_bit_allocation_map[3][0] = 3;
      rc->hierarchial_bit_allocation_map[3][2] = 3;
    }
    else if(rc->bpp>5)
    {
      rc->hierarchial_bit_allocation_map[3][3] = 19;
      rc->hierarchial_bit_allocation_map[3][1] = 7;
      rc->hierarchial_bit_allocation_map[3][0] = 3;
      rc->hierarchial_bit_allocation_map[3][2] = 3;
    }
    else
    {
      rc->hierarchial_bit_allocation_map[3][3] = 23;
      rc->hierarchial_bit_allocation_map[3][1] = 8;
      rc->hierarchial_bit_allocation_map[3][0] = 3;
      rc->hierarchial_bit_allocation_map[3][2] = 3;
    }
    rc->hierarchial_alg_map[3][3]=0;
    rc->hierarchial_alg_map[3][1]=1;
    rc->hierarchial_alg_map[3][0]=3;
    rc->hierarchial_alg_map[3][2]=3;

    rc->hierarchial_decoding_order[3][3]=0; // poc order: decoding order
    rc->hierarchial_decoding_order[3][1]=1;
    rc->hierarchial_decoding_order[3][0]=2;
    rc->hierarchial_decoding_order[3][2]=3;
  }
  //else if(rc->hierarchial_bit_allocation_GOP_size==5)
  {
    if(rc->bpp>20)
    {
      rc->hierarchial_bit_allocation_map[4][4] = 12;
      rc->hierarchial_bit_allocation_map[4][1] = 6;
      rc->hierarchial_bit_allocation_map[4][0] = 4;
      rc->hierarchial_bit_allocation_map[4][2] = 3;
      rc->hierarchial_bit_allocation_map[4][3] = 3;
    }
    else if(rc->bpp>10)
    {
      rc->hierarchial_bit_allocation_map[4][4] = 15;
      rc->hierarchial_bit_allocation_map[4][1] = 7;
      rc->hierarchial_bit_allocation_map[4][0] = 4;
      rc->hierarchial_bit_allocation_map[4][2] = 3;
      rc->hierarchial_bit_allocation_map[4][3] = 3;
    }
    else if(rc->bpp>5)
    {
      rc->hierarchial_bit_allocation_map[4][4] = 19;
      rc->hierarchial_bit_allocation_map[4][1] = 8;
      rc->hierarchial_bit_allocation_map[4][0] = 4;
      rc->hierarchial_bit_allocation_map[4][2] = 3;
      rc->hierarchial_bit_allocation_map[4][3] = 3;
    }
    else
    {
      rc->hierarchial_bit_allocation_map[4][4] = 23;
      rc->hierarchial_bit_allocation_map[4][1] = 9;
      rc->hierarchial_bit_allocation_map[4][0] = 4;
      rc->hierarchial_bit_allocation_map[4][2] = 3;
      rc->hierarchial_bit_allocation_map[4][3] = 3;
    }
    rc->hierarchial_alg_map[4][4]=0;
    rc->hierarchial_alg_map[4][1]=1;
    rc->hierarchial_alg_map[4][0]=2;
    rc->hierarchial_alg_map[4][2]=3;
    rc->hierarchial_alg_map[4][3]=3;

    rc->hierarchial_decoding_order[4][4]=0; // poc order: decoding order
    rc->hierarchial_decoding_order[4][1]=1;
    rc->hierarchial_decoding_order[4][0]=2;
    rc->hierarchial_decoding_order[4][2]=3;
    rc->hierarchial_decoding_order[4][3]=4;
  }
  //else if(rc->hierarchial_bit_allocation_GOP_size==6)
  {
    if(rc->bpp>20)
    {
      rc->hierarchial_bit_allocation_map[5][5] = 12;
      rc->hierarchial_bit_allocation_map[5][2] = 6;
      rc->hierarchial_bit_allocation_map[5][0] = 3;
      rc->hierarchial_bit_allocation_map[5][1] = 4;
      rc->hierarchial_bit_allocation_map[5][3] = 3;
      rc->hierarchial_bit_allocation_map[5][4] = 3;
    }
    else if(rc->bpp>10)
    {
      rc->hierarchial_bit_allocation_map[5][5] = 15;
      rc->hierarchial_bit_allocation_map[5][2] = 7;
      rc->hierarchial_bit_allocation_map[5][0] = 3;
      rc->hierarchial_bit_allocation_map[5][1] = 4;
      rc->hierarchial_bit_allocation_map[5][3] = 3;
      rc->hierarchial_bit_allocation_map[5][4] = 3;
    }
    else if(rc->bpp>5)
    {
      rc->hierarchial_bit_allocation_map[5][5] = 19;
      rc->hierarchial_bit_allocation_map[5][2] = 8;
      rc->hierarchial_bit_allocation_map[5][0] = 3;
      rc->hierarchial_bit_allocation_map[5][1] = 4;
      rc->hierarchial_bit_allocation_map[5][3] = 3;
      rc->hierarchial_bit_allocation_map[5][4] = 3;
    }
    else
    {
      rc->hierarchial_bit_allocation_map[5][5] = 23;
      rc->hierarchial_bit_allocation_map[5][2] = 9;
      rc->hierarchial_bit_allocation_map[5][0] = 3;
      rc->hierarchial_bit_allocation_map[5][1] = 4;
      rc->hierarchial_bit_allocation_map[5][3] = 3;
      rc->hierarchial_bit_allocation_map[5][4] = 3;
    }
    rc->hierarchial_alg_map[5][5]=0;
    rc->hierarchial_alg_map[5][2]=1;
    rc->hierarchial_alg_map[5][0]=2;
    rc->hierarchial_alg_map[5][1]=3;
    rc->hierarchial_alg_map[5][3]=2;
    rc->hierarchial_alg_map[5][4]=3;

    rc->hierarchial_decoding_order[5][5]=0; // poc order: decoding order
    rc->hierarchial_decoding_order[5][2]=1;
    rc->hierarchial_decoding_order[5][0]=2;
    rc->hierarchial_decoding_order[5][1]=3;
    rc->hierarchial_decoding_order[5][3]=4;
    rc->hierarchial_decoding_order[5][4]=5;
  }
  //else if(rc->hierarchial_bit_allocation_GOP_size==7)
  {
    if(rc->bpp>20)
    {
      rc->hierarchial_bit_allocation_map[6][6] = 12;
      rc->hierarchial_bit_allocation_map[6][2] = 6;
      rc->hierarchial_bit_allocation_map[6][0] = 3;
      rc->hierarchial_bit_allocation_map[6][1] = 3;
      rc->hierarchial_bit_allocation_map[6][4] = 5;
      rc->hierarchial_bit_allocation_map[6][3] = 3;
      rc->hierarchial_bit_allocation_map[6][5] = 3;
    }
    else if(rc->bpp>10)
    {
      rc->hierarchial_bit_allocation_map[6][6] = 15;
      rc->hierarchial_bit_allocation_map[6][2] = 7;
      rc->hierarchial_bit_allocation_map[6][0] = 3;
      rc->hierarchial_bit_allocation_map[6][1] = 3;
      rc->hierarchial_bit_allocation_map[6][4] = 5;
      rc->hierarchial_bit_allocation_map[6][3] = 3;
      rc->hierarchial_bit_allocation_map[6][5] = 3;
    }
    else if(rc->bpp>5)
    {
      rc->hierarchial_bit_allocation_map[6][6] = 19;
      rc->hierarchial_bit_allocation_map[6][2] = 8;
      rc->hierarchial_bit_allocation_map[6][0] = 3;
      rc->hierarchial_bit_allocation_map[6][1] = 3;
      rc->hierarchial_bit_allocation_map[6][4] = 5;
      rc->hierarchial_bit_allocation_map[6][3] = 3;
      rc->hierarchial_bit_allocation_map[6][5] = 3;
    }
    else
    {
      rc->hierarchial_bit_allocation_map[6][6] = 23;
      rc->hierarchial_bit_allocation_map[6][2] = 9;
      rc->hierarchial_bit_allocation_map[6][0] = 3;
      rc->hierarchial_bit_allocation_map[6][1] = 3;
      rc->hierarchial_bit_allocation_map[6][4] = 5;
      rc->hierarchial_bit_allocation_map[6][3] = 3;
      rc->hierarchial_bit_allocation_map[6][5] = 3;
    }
    rc->hierarchial_alg_map[6][6]=0;
    rc->hierarchial_alg_map[6][2]=1;
    rc->hierarchial_alg_map[6][0]=2;
    rc->hierarchial_alg_map[6][1]=3;
    rc->hierarchial_alg_map[6][4]=2;
    rc->hierarchial_alg_map[6][3]=3;
    rc->hierarchial_alg_map[6][5]=3;

    rc->hierarchial_decoding_order[6][6]=0; // poc order: decoding order
    rc->hierarchial_decoding_order[6][2]=1;
    rc->hierarchial_decoding_order[6][0]=2;
    rc->hierarchial_decoding_order[6][1]=3;
    rc->hierarchial_decoding_order[6][4]=4;
    rc->hierarchial_decoding_order[6][3]=5;
    rc->hierarchial_decoding_order[6][5]=6;

  }
  //else if(rc->hierarchial_bit_allocation_GOP_size==8)
  {
    if(rc->bpp>20)
    {
      rc->hierarchial_bit_allocation_map[7][7] = 12;
      rc->hierarchial_bit_allocation_map[7][3] = 6;
      rc->hierarchial_bit_allocation_map[7][1] = 5;
      rc->hierarchial_bit_allocation_map[7][0] = 3;
      rc->hierarchial_bit_allocation_map[7][2] = 3;
      rc->hierarchial_bit_allocation_map[7][5] = 5;
      rc->hierarchial_bit_allocation_map[7][4] = 3;
      rc->hierarchial_bit_allocation_map[7][6] = 3;
    }
    else if(rc->bpp>10)
    {
      rc->hierarchial_bit_allocation_map[7][7] = 15;
      rc->hierarchial_bit_allocation_map[7][3] = 7;
      rc->hierarchial_bit_allocation_map[7][1] = 5;
      rc->hierarchial_bit_allocation_map[7][0] = 3;
      rc->hierarchial_bit_allocation_map[7][2] = 3;
      rc->hierarchial_bit_allocation_map[7][5] = 5;
      rc->hierarchial_bit_allocation_map[7][4] = 3;
      rc->hierarchial_bit_allocation_map[7][6] = 3;
    }
    else if(rc->bpp>5)
    {
      rc->hierarchial_bit_allocation_map[7][7] = 19;
      rc->hierarchial_bit_allocation_map[7][3] = 8;
      rc->hierarchial_bit_allocation_map[7][1] = 5;
      rc->hierarchial_bit_allocation_map[7][0] = 3;
      rc->hierarchial_bit_allocation_map[7][2] = 3;
      rc->hierarchial_bit_allocation_map[7][5] = 5;
      rc->hierarchial_bit_allocation_map[7][4] = 3;
      rc->hierarchial_bit_allocation_map[7][6] = 3;
    }
    else
    {
      rc->hierarchial_bit_allocation_map[7][7] = 23;
      rc->hierarchial_bit_allocation_map[7][3] = 9;
      rc->hierarchial_bit_allocation_map[7][1] = 5;
      rc->hierarchial_bit_allocation_map[7][0] = 3;
      rc->hierarchial_bit_allocation_map[7][2] = 3;
      rc->hierarchial_bit_allocation_map[7][5] = 5;
      rc->hierarchial_bit_allocation_map[7][4] = 3;
      rc->hierarchial_bit_allocation_map[7][6] = 3;
    }
    rc->hierarchial_alg_map[7][7]=0;
    rc->hierarchial_alg_map[7][3]=1;
    rc->hierarchial_alg_map[7][1]=2;
    rc->hierarchial_alg_map[7][0]=3;
    rc->hierarchial_alg_map[7][2]=3;
    rc->hierarchial_alg_map[7][5]=2;
    rc->hierarchial_alg_map[7][4]=3;
    rc->hierarchial_alg_map[7][6]=3;

    rc->hierarchial_decoding_order[7][7]=0; // poc order: decoding order
    rc->hierarchial_decoding_order[7][3]=1;
    rc->hierarchial_decoding_order[7][1]=2;
    rc->hierarchial_decoding_order[7][0]=3;
    rc->hierarchial_decoding_order[7][2]=4;
    rc->hierarchial_decoding_order[7][5]=5;
    rc->hierarchial_decoding_order[7][4]=6;
    rc->hierarchial_decoding_order[7][6]=7;

  }




}

/*------------------------------------------------------------------------------
  HevcBeforePicRc()  Update virtual buffer, and calculate picInitQp for current
  picture , and coded status.
------------------------------------------------------------------------------*/
void HevcBeforePicRc(hevcRateControl_s *rc, u32 timeInc, u32 sliceType)
{
  hevcVirtualBuffer_s *vb = &rc->virtualBuffer;
  i32 i, rcWindow,intraBits = 0, tmp = 0, qp_move = 0;

  rc->frameCoded = ENCHW_YES;
  rc->sliceTypeCur = sliceType;

  if(rc->hierarchial_bit_allocation_GOP_size_previous!=rc->hierarchial_bit_allocation_GOP_size)
  {
    init_hierarchial_rc (rc);
    rc->hierarchial_bit_allocation_GOP_size_previous=rc->hierarchial_bit_allocation_GOP_size;
    rc->hierarchial_bit_allocation_total_weight=0;
    for(i=0;i<rc->hierarchial_bit_allocation_GOP_size;i++)
    {
      rc->hierarchial_bit_allocation_total_weight+=rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][i];
      //printf("rc->hierarchial_bit_allocation_map[%d]=%d\n",i,rc->hierarchial_bit_allocation_map[i]);
    }
  }
  //printf("rc->hierarchial_bit_allocation_total_weight=%d\n",rc->hierarchial_bit_allocation_total_weight);

  //printf("rc->encoded_frame_number=%d\n",rc->encoded_frame_number);

  DBG(1, (DBGOUTPUT, "\nBEFORE PIC RC: pic=%d\n", rc->frameCnt));
  DBG(1, (DBGOUTPUT, "Frame type:\t%8i  timeInc:\t%8i\n",
          sliceType, timeInc));

  tmp = VirtualBuffer(&rc->virtualBuffer, (i32) timeInc, rc->hrd);
  //printf("\n VirtualBuffer delta=%d\n",tmp);
  for (i = 0; i < CHECK_POINTS_MAX; i++)
  {
    rc->qpCtrl.wordCntTarget[i] = 0;
  }

  if (vb->windowRem == 0)
  {
    vb->windowRem = rc->windowLen - 1;
    /* New bitrate window, reset error counters. */
    for(i=0;i<4;i++)
    {
      //rc->windowLen need to be adjusted  for different model.
      update_rc_error(&rc->rError[i], 0x7fffffff, rc->windowLen);
    }
     /* New bitrate window, reset error counters. */
    //update_rc_error(&rc->rError_BFrame, 0x7fffffff, 18/*rc->windowLen*/);
    /* Don't reset intra error in case of intra-only, it would cause step. */
    if (rc->sliceTypeCur != rc->sliceTypePrev)
      update_rc_error(&rc->intraError, 0x7fffffff, rc->windowLen);

  }
  else
  {
    vb->windowRem--;
  }
  //printf("rc->encoded_frame_number=%d\n",rc->encoded_frame_number);
  /* Calculate target size for this picture. Adjust the target bitPerPic
   * with the cumulated error between target and actual bitrates (tmp).
   * Also take into account the bits used by intra frame starting the GOP. */
  if (rc->sliceTypeCur != I_SLICE &&
      rc->intraInterval > 1)
  {
    /* GOP bits that are used by intra frame. Amount of bits
     * "stolen" by intra from each inter frame in the GOP. */
    intraBits = vb->bitPerPic * rc->intraInterval * get_avg_bits(&rc->gop, 10) / 100;
    intraBits -= vb->bitPerPic;
    intraBits /= (rc->intraInterval - 1);
    intraBits = MAX(0, intraBits);

  }

  /* Compensate the virtual bit buffer for intra frames "stealing" bits
   * from inter frames because after intra frame the bit buffer seems
   * to overflow but the following inters will level the buffer. */
  tmp += intraBits * (rc->intraInterval - rc->intraIntervalCtr);

#ifdef RC_WINDOW_STRICT
  /* In the end of window don't be too strict with matching the error
   * otherwise the end of window tends to twist QP. */
  rcWindow = MAX(MAX(3, rc->windowLen / 6), vb->windowRem);
  //printf("\n rcWindow=%d\n",rcWindow);
#else
  /* Actually we can be fairly easy with this one, let's make it
   * a moving window to smoothen the changes. */
  rcWindow = MAX(1, rc->windowLen);
#endif
  if(rc->sliceTypeCur!=I_SLICE)
  {
    /* INF3: DIV */
#if 0
    if(rc->encoded_frame_number>=1)
    {
      float sum=0;
      for(i=0;i<rc->encoded_frame_number;i++)
      {

        sum+=vb->bitPerPic*rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][i];
      }
      sum=sum/rc->hierarchial_bit_allocation_total_weight;
      tmp+=sum-vb->bitPerPic*rc->encoded_frame_number;
    }

    rc->targetPicSize = ((vb->bitPerPic - intraBits + DIV(tmp, rcWindow) + qp_move)*rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight);
#else
    if(rc->encoded_frame_number>=1)
    {
      i64 sum=0;
      for(i=0;i<rc->encoded_frame_number;i++)
      {
        sum+=vb->bitPerPic*rc->hierarchial_bit_allocation_GOP_size*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][i];
      }
      sum=DIV_S64(sum,rc->hierarchial_bit_allocation_total_weight);
      tmp+=(i32)sum-vb->bitPerPic*rc->encoded_frame_number;
    }
    rc->targetPicSize=(int)DIV_S64(((vb->bitPerPic-intraBits+DIV(tmp,rcWindow)+qp_move)*rc->hierarchial_bit_allocation_GOP_size*(i64)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]),rc->hierarchial_bit_allocation_total_weight);
#endif
    //printf("GOP frame targetPicSize=%d\n",rc->targetPicSize);
  }
  else
  {
    //printf("I frame bitPerPic=%d\n",vb->bitPerPic);
    rc->targetPicSize = vb->bitPerPic - intraBits + DIV(tmp, rcWindow) + qp_move;
  }
  /* Limit the target to a realistic minimum that can be reached.
   * Setting target lower than this will confuse RC because it can never
   * be reached. Frame with only skipped mbs == 96 bits. */// carl 96 should be changed to what number
  rc->targetPicSize = MAX(96 + rc->ctbRows * rc->ctbSize, rc->targetPicSize);
  //printf("\n rc->targetPicSize=%d\n",rc->targetPicSize);
  DBG(1, (DBGOUTPUT, "intraRatio: %3i%%\tintraBits: %7i\tbufferComp: %7i\n",
          get_avg_bits(&rc->gop, 10), intraBits, DIV(tmp, rcWindow)));
  DBG(1, (DBGOUTPUT, "WndRem: %4i  ", vb->windowRem));
  if (rc->sliceTypeCur == I_SLICE)
  {
    DBG(1, (DBGOUTPUT, "Rd: %6d  ", avg_rc_error(&rc->intraError)));
  }
  else
  {
    DBG(1, (DBGOUTPUT, "Rd: %6d  ", avg_rc_error(&rc->rError)));
  }
  DBG(1, (DBGOUTPUT, "Tr: %7d  ", rc->targetPicSize));
  if (rc->sliceTypeCur == I_SLICE)
  {
    DBG(1, (DBGOUTPUT, "Td: %7d\n",
            CLIP3(rc->targetPicSize - avg_rc_error(&rc->intraError),
                  0, 2 * rc->targetPicSize)));
  }
  else
  {
    DBG(1, (DBGOUTPUT, "Td: %7d\n",
            CLIP3(rc->targetPicSize - avg_rc_error(&rc->rError),
                  0, 2 * rc->targetPicSize)));
  }

  if (rc->picSkip)
    PicSkip(rc);

  /* determine initial quantization parameter for current picture */
  PicQuant(rc);

  /* quantization parameter user defined limitations */
  PicQuantLimit(rc);

  rc->qpHdrLast = rc->qpHdrPrev;

  /* Store the start QP, before ROI adjustment */
  rc->qpHdrPrev = rc->qpHdr;

  if (rc->sliceTypeCur == I_SLICE)
  {
    if (rc->fixedIntraQp)
      rc->qpHdr = rc->fixedIntraQp;
    else if (rc->sliceTypePrev != I_SLICE)
    {
      //if(rc->picRc)
      if(rc->virtualBuffer.bitPerPic<(400*rc->picArea/(3*192*108)))
      {
        if(rc->intraQpDelta<(-3 << QP_FRACTIONAL_BITS))
        {
          rc->intraQpDelta=(-3 << QP_FRACTIONAL_BITS);
          //printf("rc->intraQpDelta=%d\n",rc->intraQpDelta);
        }

      }
      rc->qpHdr += rc->intraQpDelta /*+ rc->intraQpDeltaAdaptive*/ ;
    }

    /* quantization parameter user defined limitations still apply */
    PicQuantLimit(rc);
    if (rc->intraIntervalCtr > 1)
      rc->intraInterval = rc->intraIntervalCtr;
    rc->intraIntervalCtr = 1;
  }
  else
  {
    /* trace the QP over GOP, excluding Intra QP */
    rc->gopQpSum += rc->qpHdr;
    rc->gopQpDiv++;
    rc->intraIntervalCtr++;

    /* Check that interval is repeating */
    if (rc->intraIntervalCtr > rc->intraInterval)
      rc->intraInterval = rc->intraIntervalCtr;
#if 1
    if(rc->sliceTypeCur != I_SLICE)
    {
         //printf("model b qp=%d\n ",rc->qpHdr);
         //B frm not referenced
         if (rc->picRc != ENCHW_YES)
            rc->qpHdr += rc->frameQpDelta;
         if (rc->qpHdr > (51<<QP_FRACTIONAL_BITS))
           rc->qpHdr = (51<<QP_FRACTIONAL_BITS);
         //printf("final  b qp=%d\n ",rc->qpHdr);
    }
#endif
  }

  /* mb rate control (check point rate control) */
#ifdef ROI_SUPPORT
  if (rc->roiRc)
  {
    HevcEncRoiModel(rc);
    PicQuantLimit(rc);
  }
  else
#endif
   //CTB_RC
#if 1
    if (rc->ctbRc)
    {
      //MbQuant(rc);
      if(rc->seqStart)
      {
        /* INF3: DIV64 */
        rc->bitsRatio = DIV_U64(((u64)rc->targetPicSize<<16),rc->ctbPerPic);
        //rc->bitsRatio = ((u64)rc->targetPicSize<<16)/rc->ctbPerPic;
        //printf("rc->targetPicSize=%d,rc->ctbPerPic=%d,rc->bitsRatio=0x%x\n",rc->targetPicSize,rc->ctbPerPic,rc->bitsRatio);
        rc->ctbRcThrdMin = rc->ctbRcThrdMax = 0;
        rc->seqStart = 0;
      }
      else
      {
        u32 avg = rc->ctbRctotalLcuBit/rc->ctbPerPic;
        if(rc->ctbRctotalLcuBit==0)
        {
          u16* ptr = (u16*)rc->ctbMemCurVirtualAddr;
          i32 i;
          rc->ctbRctotalLcuBit = rc->ctbPerPic;
          for(i=0;i<rc->ctbPerPic;i++)
          {
            ptr[i] = 1;
          }
        }
        /* INF3: DIV64 */
        rc->bitsRatio = DIV_U64(((u64)rc->targetPicSize<<16),rc->ctbRctotalLcuBit);
        //rc->bitsRatio = ((u64)rc->targetPicSize<<16)/rc->ctbRctotalLcuBit;
        rc->ctbRcThrdMin = MIN(0xffff,   (((u64)(rc->ctbRcBitsMin + (avg-rc->ctbRcBitsMin)/5)*rc->bitsRatio)>>16));
        rc->ctbRcThrdMax = MIN(0xffff,   (((u64)(rc->ctbRcBitsMax - (rc->ctbRcBitsMax-avg)/5)*rc->bitsRatio)>>16));

      }
    }
#endif
  /* reset counters */
  rc->qpSum = 0;
  rc->qpLastCoded = rc->qpHdr;
  rc->qpTarget = rc->qpHdr;
  rc->nonZeroCnt = 0;
    rc->sliceEncodedTypePrev = rc->sliceTypePrev;
  //rc->sliceTypePrev = rc->sliceTypeCur;

  DBG(0, (DBGOUTPUT, "Frame coded\t%8d  ", rc->frameCoded));
  DBG(0, (DBGOUTPUT, "Frame qpHdr\t%8d  ", rc->qpHdr));
  DBG(0, (DBGOUTPUT, "GopRem:\t%8d  ", vb->windowRem));
  DBG(0, (DBGOUTPUT, "Target bits:\t%8d  ", rc->targetPicSize));
  DBG(1, (DBGOUTPUT, "\nintraBits:\t%8d  ", intraBits));
  DBG(1, (DBGOUTPUT, "bufferComp:\t%8d  ", DIV(tmp, rcWindow)));
  DBG(1, (DBGOUTPUT, "Rd:\t\t%8d\n", avg_rc_error(&rc->rError)));

  for (i = 0; i < CHECK_POINTS_MAX; i++)
  {
    DBG(1, (DBGOUTPUT, "CP %i  mbNum %4i  wTarg %5i\n", i,
            (rc->qpCtrl.checkPointDistance * (i + 1)),
            rc->qpCtrl.wordCntTarget[i] * 32));
  }

  rc->sei.crd += timeInc;

  rc->sei.dod = 0;
}

/*------------------------------------------------------------------------------

  MbQuant()

------------------------------------------------------------------------------*/
/* INF3: kill warnings */
#if 0
static void MbQuant(hevcRateControl_s *rc)
{
  i32 nonZeroTarget;

  /* Disable Mb Rc for Intra Slices, because coeffTarget will be wrong */
  if (rc->sliceTypeCur == I_SLICE ||
      rc->srcPrm == 0)
  {
    return;
  }

  /* Required zero cnt */
  nonZeroTarget = HevcCalculate(rc->targetPicSize, 256, rc->srcPrm);
  nonZeroTarget = MIN(rc->coeffCntMax, MAX(0, nonZeroTarget));

  nonZeroTarget = MIN(0x7FFFFFFFU / 1024U, (u32)nonZeroTarget);

  rc->virtualBuffer.nonZeroTarget = nonZeroTarget;

  /* Use linear model when previous frame can't be used for prediction */
  if ((rc->sliceTypeCur != rc->sliceTypePrev) || (rc->nonZeroCnt == 0))
  {
    LinearModel(rc);
  }
  else
  {
    AdaptiveModel(rc);
  }
}

/*------------------------------------------------------------------------------

  LinearModel()

------------------------------------------------------------------------------*/
void LinearModel(hevcRateControl_s *rc)
{
  const i32 sscale = 256;
  hevcQpCtrl_s *qc = &rc->qpCtrl;
  i32 scaler;
  i32 i;
  i32 tmp, nonZeroTarget = rc->virtualBuffer.nonZeroTarget;

  ASSERT(nonZeroTarget < (0x7FFFFFFF / sscale));

  if (nonZeroTarget > 0)
  {
    scaler = HevcCalculate(nonZeroTarget, sscale, (i32) rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16);
  }
  else
  {
    return;
  }

  DBG(1, (DBGOUTPUT, " Linear Target: %8d prevCnt:\t %6d Scaler:\t %6d\n",
          nonZeroTarget, rc->nonZeroCnt, scaler / sscale));

  for (i = 0; i < rc->qpCtrl.checkPoints; i++)
  {
    tmp = (scaler * (qc->checkPointDistance * (i + 1) + 1)) / sscale;
    tmp = MIN(WORD_CNT_MAX, tmp / 32 + 1);
    if (tmp < 0) tmp = WORD_CNT_MAX;    /* Detect overflow */
    qc->wordCntTarget[i] = tmp; /* div32 for regs */
  }

  /* calculate nz count for avg. bits per frame */
  tmp = HevcCalculate(rc->virtualBuffer.bitPerPic, rc->ctbSize * rc->ctbSize, rc->srcPrm);

  DBG(1, (DBGOUTPUT, "Error Limit:\t %8d SrcPrm:\t %6d\n",
          tmp, rc->srcPrm / 256));

  qc->wordError[0] = -tmp * 3;
  qc->qpChange[0] = -3;
  qc->wordError[1] = -tmp * 2;
  qc->qpChange[1] = -2;
  qc->wordError[2] = -tmp * 1;
  qc->qpChange[2] = -1;
  qc->wordError[3] = tmp * 1;
  qc->qpChange[3] = 0;
  qc->wordError[4] = tmp * 2;
  qc->qpChange[4] = 1;
  qc->wordError[5] = tmp * 3;
  qc->qpChange[5] = 2;
  qc->wordError[6] = tmp * 4;
  qc->qpChange[6] = 3;

  for (i = 0; i < CTRL_LEVELS; i++)
  {
    tmp = qc->wordError[i];
    tmp = CLIP3(-32768, 32767, tmp / 4);
    qc->wordError[i] = tmp;
  }
}


/*------------------------------------------------------------------------------

  AdaptiveModel()

------------------------------------------------------------------------------*/
void AdaptiveModel(hevcRateControl_s *rc)
{
  const i32 sscale = rc->ctbSize * rc->ctbSize;
  hevcQpCtrl_s *qc = &rc->qpCtrl;
  i32 i;
  i32 tmp, nonZeroTarget = rc->virtualBuffer.nonZeroTarget;
  i32 scaler;

  ASSERT(nonZeroTarget < (0x7FFFFFFF / sscale));

  if ((nonZeroTarget > 0) && (rc->nonZeroCnt > 0))
  {
    scaler = HevcCalculate(nonZeroTarget, sscale, rc->nonZeroCnt);
  }
  else
  {
    return;
  }
  DBG(1, (DBGOUTPUT, "Adaptive Target: %8d prevCnt:\t %6d Scaler:\t %6d\n",
          nonZeroTarget, rc->nonZeroCnt, scaler / sscale));

  for (i = 0; i < rc->qpCtrl.checkPoints; i++)
  {
    tmp = (i32)(qc->wordCntPrev[i] * scaler) / sscale;
    tmp = MIN(WORD_CNT_MAX, tmp / 32 + 1);
    if (tmp < 0) tmp = WORD_CNT_MAX;    /* Detect overflow */
    qc->wordCntTarget[i] = tmp; /* div32 for regs */
    DBG(2, (DBGOUTPUT, " CP %i  wordCntPrev %6i  wordCntTarget_div32 %6i\n",
            i, qc->wordCntPrev[i], qc->wordCntTarget[i]));
  }

  /* Qp change table */

  /* calculate nz count for avg. bits per frame */
  tmp = HevcCalculate(rc->virtualBuffer.bitPerPic, rc->ctbSize * rc->ctbSize, (rc->srcPrm * 3));

  DBG(1, (DBGOUTPUT, "Error Limit:\t %8d SrcPrm:\t %6d\n",
          tmp, rc->srcPrm / (rc->ctbSize * rc->ctbSize)));

  qc->wordError[0] = -tmp * 3;
  qc->qpChange[0] = -3;
  qc->wordError[1] = -tmp * 2;
  qc->qpChange[1] = -2;
  qc->wordError[2] = -tmp * 1;
  qc->qpChange[2] = -1;
  qc->wordError[3] = tmp * 1;
  qc->qpChange[3] = 0;
  qc->wordError[4] = tmp * 2;
  qc->qpChange[4] = 1;
  qc->wordError[5] = tmp * 3;
  qc->qpChange[5] = 2;
  qc->wordError[6] = tmp * 4;
  qc->qpChange[6] = 3;

  for (i = 0; i < CTRL_LEVELS; i++)
  {
    tmp = qc->wordError[i];
    tmp = CLIP3(-32768, 32767, tmp / 4);
    qc->wordError[i] = tmp;
  }
}
#endif
/*------------------------------------------------------------------------------

  SourceParameter()  Source parameter of last coded frame. Parameters
  has been scaled up by factor 256.

------------------------------------------------------------------------------*/
void SourceParameter(hevcRateControl_s *rc, i32 nonZeroCnt)
{
  //ASSERT(rc->qpSum <= 51 * rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16);
  ASSERT(nonZeroCnt <= rc->coeffCntMax);
  ASSERT(nonZeroCnt >= 0 && rc->coeffCntMax >= 0);

  /* AVOID division by zero */
  if (nonZeroCnt == 0)
  {
    nonZeroCnt = 1;
  }

  rc->srcPrm = HevcCalculate(rc->frameBitCnt, rc->ctbSize * rc->ctbSize, nonZeroCnt);

  DBG(1, (DBGOUTPUT, "nonZeroCnt %6i, srcPrm %i\n",
          nonZeroCnt, rc->srcPrm / (rc->ctbSize * rc->ctbSize)));

}

/*------------------------------------------------------------------------------
  PicSkip()  Decrease framerate if not enough bits available.
------------------------------------------------------------------------------*/
void PicSkip(hevcRateControl_s *rc)
{
  hevcVirtualBuffer_s *vb = &rc->virtualBuffer;
  i32 bitAvailable = vb->virtualBitCnt - vb->realBitCnt;
  i32 skipIncLimit = -vb->bitPerPic / 3;
  i32 skipDecLimit = vb->bitPerPic / 3;

  /* When frameRc is enabled, skipFrameTarget is not allowed to be > 1
   * This makes sure that not too many frames is skipped and lets
   * the frameRc adjust QP instead of skipping many frames */
  if (((rc->picRc == ENCHW_NO) || (vb->skipFrameTarget == 0)) &&
      (bitAvailable < skipIncLimit))
  {
    vb->skipFrameTarget++;
  }

  if ((bitAvailable > skipDecLimit) && vb->skipFrameTarget > 0)
  {
    vb->skipFrameTarget--;
  }

  if (vb->skippedFrames < vb->skipFrameTarget)
  {
    vb->skippedFrames++;
    rc->frameCoded = ENCHW_NO;
  }
  else
  {
    vb->skippedFrames = 0;
  }
}

/*------------------------------------------------------------------------------
  PicQuant()  Calculate quantization parameter for next frame. In the beginning
                of window use previous GOP average QP and otherwise find new QP
                using the target size and previous frames QPs and bit counts.
------------------------------------------------------------------------------*/
void PicQuant(hevcRateControl_s *rc)
{
  i32 normBits, targetBits;
  true_e useQpDeltaLimit = ENCHW_YES;
  i32 movingBitMax,movingBitBase;
  i64 frameBit;
  /* INF3: kill warnings
  i32 i, predictOrder,predictBits,predictQP_Err,bestIQP,bestBits,bestI,bitError,total_count;
  i32 bpp_accurate;
  bestI = 0;
  */
  if (rc->picRc != ENCHW_YES)
  {
    rc->qpHdr = rc->fixedQp;
    DBG(1, (DBGOUTPUT, "R/cx:  xxxx  QP: xx xx D:  xxxx newQP: xx\n"));
    return;
  }

  /* If HRD is enabled we must make sure this frame fits in buffer */
  if (rc->hrd == ENCHW_YES)
  {
    i32 bitsAvailable =
      (rc->virtualBuffer.bufferSize - rc->virtualBuffer.bucketFullness);

    /* If the previous frame didn't fit the buffer we don't limit QP change */
    if (rc->frameBitCnt > bitsAvailable)
    {
      useQpDeltaLimit = ENCHW_NO;
    }
  }

  /* determine initial quantization parameter for current picture */
  if (rc->sliceTypeCur == I_SLICE){
        /* Default intra QP == average of prev frame and prev GOP average */
        //rc->qpHdr = (rc->qpHdrPrev + gop_avg_qp(rc))/2;
        //printf("rc->qpHdrPrev=%f,gop_avg_qp=%f\n",rc->qpHdrPrev,gop_avg_qp(rc));
        rc->qpHdr = (rc->qpHdrPrev + gop_avg_qp(rc))/2;


        //printf("I frame rc->qpHdr=%f\n",rc->qpHdr);

    /* If all frames are intra we calculate new QP
     * for intra the same way as for inter */
    if (rc->sliceTypePrev == I_SLICE)
    {
      targetBits = rc->targetPicSize - avg_rc_error(&rc->intraError);
      targetBits = CLIP3(0, 2 * rc->targetPicSize, targetBits);
      normBits = HevcCalculate(targetBits, DSCBITPERMB, rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16);
      //rc->qpHdr = new_pic_quant(&rc->intra, normBits, useQpDeltaLimit);
      rc->qpHdr = new_pic_quant(&rc->intra, normBits, useQpDeltaLimit,0,0,0);
    }
    else
    {
      DBG(1, (DBGOUTPUT, "R/cx:  xxxx  QP: xx xx D:  xxxx newQP: xx\n"));
    }
    //printf("I  rc->qpHdr=%d\n",rc->qpHdr);
  }
  else if(rc->sliceTypeCur == P_SLICE||(rc->sliceTypeCur==B_SLICE&&rc->hierarchial_decoding_order[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]==0))
  {
    //printf("original P rc->targetPicSize=%d\n",rc->targetPicSize);
    if(rc->sliceTypeCur == P_SLICE)
      rc->targetPicSize=CLIP3(rc->minPicSizeP, rc->maxPicSizeP, rc->targetPicSize);
    else
      rc->targetPicSize=CLIP3(rc->minPicSizeB, rc->maxPicSizeB, rc->targetPicSize);

    if(rc->ma.count==rc->ma.length)
    {
      //printf("rc->preservedBitsForIFrame=%d\n",rc->preservedBitsForIFrame);

      /* INF3: DIV */
        frameBit = DIV_S64((i64)(rc->virtualBuffer.maxBitRate)*rc->ma.frameRateDenom, rc->ma.frameRateNumer);
        movingBitMax=(i32)(frameBit*rc->ma.count-moving_totalBit_window_minus1(&rc->ma));
//      movingBitMax=(i32)((DIV_S64(((i64)rc->virtualBuffer.maxBitRate*rc->ma.frameRateDenom),rc->ma.frameRateNumer))*rc->ma.count-moving_totalBit_window_minus1(&rc->ma));
      //movingBitMax=((float)rc->virtualBuffer.maxBitRate*rc->ma.frameRateDenom/rc->ma.frameRateNumer)*rc->ma.count-moving_totalBit_window_minus1(&rc->ma);
      /* INF3: DIV */
      movingBitMax*=(i32)DIV_S64((i64)(rc->hierarchial_bit_allocation_GOP_size)*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc], rc->hierarchial_bit_allocation_total_weight);
      //movingBitMax*=rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight;

      if(movingBitMax<100)
        movingBitMax=100;

      /* INF3: DIV */
      frameBit = DIV_S64((i64)(rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom, rc->ma.frameRateNumer);
      movingBitBase = (i32)DIV_S64(frameBit*rc->hierarchial_bit_allocation_GOP_size*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc], rc->hierarchial_bit_allocation_total_weight);
//      movingBitBase=(int)DIV_S64(((((rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom/rc->ma.frameRateNumer))*rc->hierarchial_bit_allocation_GOP_size*(i64)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]),rc->hierarchial_bit_allocation_total_weight);
      //movingBitBase=(((rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom/rc->ma.frameRateNumer))*rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight;

      movingBitMax=(movingBitMax+movingBitBase+1)/2;
      //printf("movingBitBase=%d\n",movingBitBase);
      rc->targetPicSize=CLIP3(movingBitBase/10, movingBitMax, rc->targetPicSize);
      //printf("clipped rc->targetPicSize =%d\n",rc->targetPicSize);
    }
    else
    {
      //printf("rc->preservedBitsForIFrame=%d\n",rc->preservedBitsForIFrame);
      /* INF3: DIV */
      movingBitMax=(i32)DIV_S64((i64)(rc->virtualBuffer.maxBitRate)*rc->ma.frameRateDenom, rc->ma.frameRateNumer);
      //movingBitMax=(((float)rc->virtualBuffer.maxBitRate*rc->ma.frameRateDenom/rc->ma.frameRateNumer));

      //movingBitMin=((rc->virtualBuffer.bitRate-rc->f_tolMovingBitRate*rc->virtualBuffer.bitRate/100)*rc->ma.frameRateDenom/rc->ma.frameRateNumer)-rc->preservedBitsForIFrame/rc->ma.length;

      /* INF3: DIV */
      movingBitMax*=(i32)DIV_S64((i64)(rc->hierarchial_bit_allocation_GOP_size)*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc], rc->hierarchial_bit_allocation_total_weight);
      //movingBitMax*=rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight;

      if(movingBitMax<100)
        movingBitMax=100;
      /* INF3: DIV */
      frameBit = DIV_S64((i64)(rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom, rc->ma.frameRateNumer);
      movingBitBase = (i32)DIV_S64(frameBit*rc->hierarchial_bit_allocation_GOP_size*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc], rc->hierarchial_bit_allocation_total_weight);
//      movingBitBase=(int)DIV_S64(((((rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom/rc->ma.frameRateNumer))*rc->hierarchial_bit_allocation_GOP_size*(i64)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]),rc->hierarchial_bit_allocation_total_weight);
      //movingBitBase=(((rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom/rc->ma.frameRateNumer))*rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight;

      //printf("movingBitBase=%d\n",movingBitBase);
      rc->targetPicSize=CLIP3(movingBitBase/10, movingBitMax, rc->targetPicSize);
      //printf("clipped rc->targetPicSize =%d\n",rc->targetPicSize);
    }
    //printf("rc->targetPicSize=%d\n",rc->targetPicSize);
    /* Calculate new QP by matching to previous inter frames R-Q curve */
    targetBits = rc->targetPicSize - avg_rc_error(&rc->rError[0]);
    //printf("targetBits=%d\n",targetBits);
    //printf("rc->targetPicSize=%d,avg_rc_error(&rc->rError[0])=%d\n",rc->targetPicSize,avg_rc_error(&rc->rError[0]));
    //targetBits = CLIP3(0, 2 * rc->targetPicSize, targetBits);
    //printf("targetBits=%d\n",targetBits);
    normBits = HevcCalculate(targetBits, DSCBITPERMB, rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16);
    //printf("normBits=%d\n",normBits);
    //printf("\n p normBits=%d,qpprev=%d\n",normBits,rc->linReg[0].qp_prev);
    //rc->qpHdr = new_pic_quant(&rc->linReg[0], normBits, useQpDeltaLimit);
    rc->qpHdr = new_pic_quant(&rc->linReg[0], normBits, useQpDeltaLimit,rc->frameBitCntLast,rc->targetPicSizeLast,rc->targetPicSize);
    //printf("p rc->qpHdr=%f \n",rc->qpHdr);
  }
  else
  {

    if(rc->sliceTypeCur == P_SLICE)
      rc->targetPicSize=CLIP3(rc->minPicSizeP, rc->maxPicSizeP, rc->targetPicSize);
    else
      rc->targetPicSize=CLIP3(rc->minPicSizeB, rc->maxPicSizeB, rc->targetPicSize);

    if(rc->ma.count==rc->ma.length)
    {
      //printf("rc->preservedBitsForIFrame=%d\n",rc->preservedBitsForIFrame);
      /* INF3: DIV */
        frameBit = DIV_S64((i64)(rc->virtualBuffer.maxBitRate)*rc->ma.frameRateDenom, rc->ma.frameRateNumer);
      movingBitMax = (i32)(frameBit*rc->ma.count-moving_totalBit_window_minus1(&rc->ma));
//      movingBitMax=((int)DIV_S64(((i64)rc->virtualBuffer.maxBitRate*rc->ma.frameRateDenom),rc->ma.frameRateNumer))*rc->ma.count-moving_totalBit_window_minus1(&rc->ma);
      //movingBitMax=((float)rc->virtualBuffer.maxBitRate*rc->ma.frameRateDenom/rc->ma.frameRateNumer)*rc->ma.count-moving_totalBit_window_minus1(&rc->ma);

      /* INF3: DIV */
      movingBitMax*=(i32)DIV_S64((i64)(rc->hierarchial_bit_allocation_GOP_size)*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc], rc->hierarchial_bit_allocation_total_weight);
      //movingBitMax*=rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight;

      if(movingBitMax<100)
        movingBitMax=100;
      /* INF3: DIV */
      frameBit = DIV_S64((i64)(rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom, rc->ma.frameRateNumer);
      movingBitBase = (i32)DIV_S64(frameBit*rc->hierarchial_bit_allocation_GOP_size*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc], rc->hierarchial_bit_allocation_total_weight);
//      movingBitBase=(int)DIV_S64(((((rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom/rc->ma.frameRateNumer))*rc->hierarchial_bit_allocation_GOP_size*(i64)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]),rc->hierarchial_bit_allocation_total_weight);
      //movingBitBase=(((rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom/rc->ma.frameRateNumer))*rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight;
      movingBitMax=(movingBitMax+movingBitBase+1)/2;
      rc->targetPicSize=CLIP3(movingBitBase/10, movingBitMax, rc->targetPicSize);
    }
    else
    {
      //printf("rc->preservedBitsForIFrame=%d\n",rc->preservedBitsForIFrame);
      /* INF3: DIV */
      movingBitMax=(i32)DIV_S64(((i64)rc->virtualBuffer.maxBitRate)*rc->ma.frameRateDenom, rc->ma.frameRateNumer);
      //movingBitMax=(((float)rc->virtualBuffer.maxBitRate*rc->ma.frameRateDenom/rc->ma.frameRateNumer));

      //movingBitMin=((rc->virtualBuffer.bitRate-rc->f_tolMovingBitRate*rc->virtualBuffer.bitRate/100)*rc->ma.frameRateDenom/rc->ma.frameRateNumer)-rc->preservedBitsForIFrame/rc->ma.length;

      /* INF3: DIV */
      movingBitMax*=(i32)DIV_S64(((i64)(rc->hierarchial_bit_allocation_GOP_size)*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]), rc->hierarchial_bit_allocation_total_weight);
      //movingBitMax*=rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight;

      if(movingBitMax<100)
        movingBitMax=100;
      /* INF3: DIV */
      frameBit = DIV_S64((i64)(rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom, rc->ma.frameRateNumer);
      movingBitBase = (i32)DIV_S64(frameBit*rc->hierarchial_bit_allocation_GOP_size*rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc], rc->hierarchial_bit_allocation_total_weight);
//      movingBitBase=(int)DIV_S64(((((rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom/rc->ma.frameRateNumer))*rc->hierarchial_bit_allocation_GOP_size*(i64)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]),rc->hierarchial_bit_allocation_total_weight);
      //movingBitBase=(((rc->virtualBuffer.bitRate)*rc->ma.frameRateDenom/rc->ma.frameRateNumer))*rc->hierarchial_bit_allocation_GOP_size*(float)rc->hierarchial_bit_allocation_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]/rc->hierarchial_bit_allocation_total_weight;

      rc->targetPicSize=CLIP3(movingBitBase/10, movingBitMax, rc->targetPicSize);
    }
    /* Calculate new QP by matching to previous inter frames R-Q curve */
    //printf("B rc->targetPicSize=%d,avg_rc_error(&rc->rError_BFrame)=%d\n",rc->targetPicSize,avg_rc_error(&rc->rError[rc->hierarchial_alg_map[rc->gopPoc]]));
    targetBits = rc->targetPicSize - avg_rc_error(&rc->rError[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]]);
    //printf("B targetBits minus error=%d\n",targetBits);
    targetBits = CLIP3(0, 2 * rc->targetPicSize, targetBits);

    normBits = HevcCalculate(targetBits, DSCBITPERMB, rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16);
    //printf("normBits=%d\n",normBits);
    //printf("\n B normBits=%d,qpprev=%d\n",normBits,rc->linReg[rc->hierarchial_alg_map[rc->gopPoc]].qp_prev);
    //rc->qpHdr = new_pic_quant(&rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]], normBits, useQpDeltaLimit);
    rc->qpHdr = new_pic_quant(&rc->linReg[rc->hierarchial_alg_map[rc->hierarchial_bit_allocation_GOP_size-1][rc->gopPoc]], normBits, useQpDeltaLimit,0,0,0);
  }
}

/*------------------------------------------------------------------------------

  PicQuantLimit()

------------------------------------------------------------------------------*/
void PicQuantLimit(hevcRateControl_s *rc)
{
  rc->qpHdr = MIN(rc->qpMax, MAX(rc->qpMin, rc->qpHdr));
}

/*------------------------------------------------------------------------------

  Calculate()  I try to avoid overflow and calculate good enough result of a*b/c

------------------------------------------------------------------------------*/
i32 HevcCalculate(i32 a, i32 b, i32 c)
{
  u32 left = 32;
  u32 right = 0;
  u32 shift;
  i32 sign = 1;
  i32 tmp;

  if (a == 0 || b == 0)
  {
    return 0;
  }
  else if ((a * b / b) == a && c != 0)
  {
    return (a * b / c);
  }
  if (a < 0)
  {
    sign = -1;
    a = -a;
  }
  if (b < 0)
  {
    sign *= -1;
    b = -b;
  }
  if (c < 0)
  {
    sign *= -1;
    c = -c;
  }

  if (c == 0)
  {
    return 0x7FFFFFFF * sign;
  }

  if (b > a)
  {
    tmp = b;
    b = a;
    a = tmp;
  }

  for (--left; (((u32)a << left) >> left) != (u32)a; --left);
  left--; /* unsigned values have one more bit on left,
               we want signed accuracy. shifting signed values gives
               lint warnings */

  while (((u32)b >> right) > (u32)c)
  {
    right++;
  }

  if (right > left)
  {
    return 0x7FFFFFFF * sign;
  }
  else
  {
    shift = left - right;
    return (i32)((((u32)a << shift) / (u32)c * (u32)b) >> shift) * sign;
  }
}
#if 0

/*------------------------------------------------------------------------------
  avg_rc_error()  PI(D)-control for rate prediction error.
------------------------------------------------------------------------------*/
static i32 avg_rc_error(linReg_s *p)
{
  if (ABS(p->bits[2]) < 0xFFFFFFF && ABS(p->bits[1]) < 0xFFFFFFF)
    return DIV(p->bits[2] * 8 + p->bits[1] * 4 + p->bits[0] * 0, 100);

  /* Avoid overflow */
  return HevcCalculate(p->bits[2], 8, 100) +
         HevcCalculate(p->bits[1], 4, 100);
}

/*------------------------------------------------------------------------------
  update_overhead()  Update PI(D)-control values
------------------------------------------------------------------------------*/
static void update_rc_error(linReg_s *p, i32 bits, i32 windowLen)
{
  p->len = 3;

  if (bits == (i32)0x7fffffff)
  {
    /* RESET */
    p->bits[0] = 0;
    if (windowLen)  /* Store the average error of previous GOP. */
      p->bits[1] = p->bits[1] / windowLen;
    else
      p->bits[1] = 0;
    p->bits[2] = 0;
    return;
  }
  p->bits[0] = bits - p->bits[2]; /* Derivative */
  if ((bits > 0) && (bits + p->bits[1] > p->bits[1]))
    p->bits[1] = bits + p->bits[1]; /* Integral */
  if ((bits < 0) && (bits + p->bits[1] < p->bits[1]))
    p->bits[1] = bits + p->bits[1]; /* Integral */
  p->bits[2] = bits;              /* Proportional */
  DBG(1, (DBGOUTPUT, "P %6d I %7d D %7d\n", p->bits[2],  p->bits[1], p->bits[0]));
}
#else
/*------------------------------------------------------------------------------
  avg_rc_error()  PI(D)-control for rate prediction error.
------------------------------------------------------------------------------*/
static i32 avg_rc_error(linReg_s *p)
{
  /* Avoid overflow */
  i32 i;
  i32 biterror=0;
  for(i=0;i<p->len;i++)
  {
    biterror += p->bits[i];
  }
  return biterror/p->len;
}

/*------------------------------------------------------------------------------
  update_overhead()  Update PI(D)-control values
------------------------------------------------------------------------------*/
static void update_rc_error(linReg_s *p, i32 bits, i32 windowLen)
{
  p->len = 4;

  if (bits == (i32)0x7fffffff)
  {
    /* RESET */
    p->bits[0] = 0;
    p->bits[1] = 0;
    p->bits[2] = 0;
    p->bits[3] = 0;
    p->bits[4] = 0;
    p->pos=0;
    return;
  }
  p->bits[p->pos++] = bits;
  if(p->pos==p->len)
    p->pos=0;
}

#endif
/*------------------------------------------------------------------------------
  gop_avg_qp()  Average quantization parameter of P frames since previous I.
------------------------------------------------------------------------------*/
i32 gop_avg_qp(hevcRateControl_s *rc)
{
  i32 tmp = rc->qpHdrPrev;
  i32 maxIntraBitRatio = 95;  /* Percentage of total GOP bits. */

  /* Average QP of previous GOP inter frames */
  if (rc->gopQpSum && rc->gopQpDiv)
  {
    tmp = DIV(rc->gopQpSum, rc->gopQpDiv);
  }
  /* Average bit count per frame for previous GOP (intra + inter) */
  rc->gopAvgBitCnt = DIV(rc->gopBitCnt, (rc->gopQpDiv + 1));
  /* Ratio of intra_frame_bits/all_gop_bits % for previous GOP */
  if (rc->gopBitCnt)
  {
    i32 gopIntraBitRatio =
      HevcCalculate(get_avg_bits(&rc->intra, 1), rc->ctbPerPic * rc->ctbSize * rc->ctbSize / 16 / 16, DSCBITPERMB) * 100;
    gopIntraBitRatio = DIV(gopIntraBitRatio, rc->gopBitCnt);
    /* Limit GOP intra bit ratio to leave room for inters. */
    gopIntraBitRatio = MIN(maxIntraBitRatio, gopIntraBitRatio);
    update_tables(&rc->gop, tmp, gopIntraBitRatio);
  }
  rc->gopQpSum = 0;
  rc->gopQpDiv = 0;
  rc->gopBitCnt = 0;

  return tmp;
}


/*------------------------------------------------------------------------------
  new_pic_quant()  Calculate new quantization parameter from the 2nd degree R-Q
  equation. Further adjust Qp for "smoother" visual quality.
------------------------------------------------------------------------------*/
//static i32 new_pic_quant(linReg_s *p, i32 bits, true_e useQpDeltaLimit)
static i32 new_pic_quant(linReg_s *p, i32 bits, true_e useQpDeltaLimit,i32 lastActualBits, i32 lastTargetBits, i32 currentTargetBits)
{
  i32 tmp = 0, qp_best = p->qp_prev, qp = p->qp_prev, diff;
  i32 diff_prev = 0, qp_prev = 0, diff_best = 0x7FFFFFFF;
  i32 minQP,maxQP;

  DBG(1, (DBGOUTPUT, "R/cx:%6d ", bits));

  if (p->a1 == 0 && p->a2 == 0)
  {
    //printf("p->a1 == 0 && p->a2 == 0\n");
    DBG(1, (DBGOUTPUT, " QP: xx xx D:  ==== newQP: %2d\n", qp));
    p->qp_prev = qp;
    return qp;
  }
    /* Find the qp that has the best match on fitted curve */
  do {
      /* INF3: DIV with math64.h. */
      tmp  = DIV_DIV_S64(p->a1, q_step[((qp*10)>>QP_FRACTIONAL_BITS)]);
      tmp += DIV_DIV64_S64(p->a2, (i64)q_step[((qp*10)>>QP_FRACTIONAL_BITS)] * q_step[((qp*10)>>QP_FRACTIONAL_BITS)]);
      //tmp  = DIV(p->a1, q_step[((qp*10)>>QP_FRACTIONAL_BITS)]);
      //tmp += DIV(p->a2, (i64)q_step[((qp*10)>>QP_FRACTIONAL_BITS)] * q_step[((qp*10)>>QP_FRACTIONAL_BITS)]);

      diff = ABS(tmp - (bits << QP_FRACTIONAL_BITS));

      if (diff < diff_best) {
          if (diff_best == 0x7FFFFFFF) {
              diff_prev = diff;
              qp_prev   = qp;
          } else {
              diff_prev = diff_best;
              qp_prev   = qp_best;
          }
          diff_best = diff;
          qp_best   = qp;
          if ((tmp - (bits << QP_FRACTIONAL_BITS)) <= 0) {
              if (qp < QP_FIXPOINT_0_POINT_1) {
                  break;
              }
            qp -= QP_FIXPOINT_0_POINT_1;
          } else {
              if (qp >= 0x3300) {
                  break;
              }
            qp += QP_FIXPOINT_0_POINT_1;
          }
      } else {
          break;
      }
  }
  while ((qp >= 0) && (qp <= 0x3300));

  if(qp > 0x3300)
    qp = 0x3300;

  DBG(1,(DBGOUTPUT, " QP: %2d %2d D: %5d", qp, qp_prev, diff_prev - diff_best));
  DBG(1, (DBGOUTPUT, " newQP: %2d\n", qp));

  /* Limit Qp change for smoother visual quality */
  if (useQpDeltaLimit)
  {
    tmp = qp - p->qp_prev;
    //printf("p->qp_prev=0x%x,qp - p->qp_prev=0x%x\n",p->qp_prev,tmp);
    if (tmp > QP_DELTA)
    {
#if 1
      //qp = p->qp_prev + QP_DELTA;
      /* When QP is totally wrong, allow faster QP increase */
      if (tmp > QP_DELTA_LIMIT)
        qp = p->qp_prev + QP_DELTA_LIMIT/2;
      else
        qp = p->qp_prev +QP_DELTA;
#endif
        //if(tmp <= QP_DELTA_LIMIT)
          //qp = p->qp_prev +(tmp+1)/2;
      }
      else if (tmp < -QP_DELTA)
      {
            if(tmp < -QP_DELTA)
                qp = p->qp_prev - QP_DELTA;
      }
    }
    if(lastActualBits!=0&&lastTargetBits!=0)
    {
      if(currentTargetBits>=lastTargetBits)
      {
        if(lastActualBits <= lastTargetBits)
        {
          if(qp<=p->qp_prev)
          {
            ;//printf("correct qp result\n");
          }
          else
          {
            ;//printf("error qp result\n");
            qp = p->qp_prev - QP_FIXPOINT_0_POINT_1;
          }
        }
      }
      if(currentTargetBits <= lastTargetBits)
      {
        if(lastActualBits >= lastTargetBits)
        {
          if(qp >= p->qp_prev)
          {
            ;//printf("correct qp result\n");
          }
          else
          {
            //printf("error qp result\n");
            qp = p->qp_prev + QP_FIXPOINT_0_POINT_1;
          }
        }
      }
      if((ABS(100*(lastActualBits-lastTargetBits))/lastTargetBits)<5)
      {
        minQP = p->qp_prev - QP_FIXPOINT_0_POINT_1*p->weight/10;
        maxQP = p->qp_prev + QP_FIXPOINT_0_POINT_1*p->weight/10;
        qp=p->qp_prev+(qp-p->qp_prev)*p->weight/10;
        qp = CLIP3(minQP,maxQP,qp);
      }
      else if(((ABS(100*(lastActualBits-lastTargetBits))/lastTargetBits)<10))
      {
        minQP = p->qp_prev - 2*QP_FIXPOINT_0_POINT_1*p->weight/10;
        maxQP = p->qp_prev + 2*QP_FIXPOINT_0_POINT_1*p->weight/10;
        qp=p->qp_prev+(qp-p->qp_prev)*p->weight/10;
        qp = CLIP3(minQP,maxQP,qp);
      }
      else if(((ABS(100*(lastActualBits-lastTargetBits))/lastTargetBits)<15))
      {
        minQP = p->qp_prev - 3*QP_FIXPOINT_0_POINT_1*p->weight/10;
        maxQP = p->qp_prev + 3*QP_FIXPOINT_0_POINT_1*p->weight/10;
        qp=p->qp_prev+(qp-p->qp_prev)*p->weight/10;
        qp = CLIP3(minQP,maxQP,qp);
      }
      else if(((ABS(100*(lastActualBits-lastTargetBits))/lastTargetBits)<20))
      {
        minQP = p->qp_prev - 4*QP_FIXPOINT_0_POINT_1*p->weight/10;
        maxQP = p->qp_prev + 4*QP_FIXPOINT_0_POINT_1*p->weight/10;
        qp=p->qp_prev+(qp-p->qp_prev)*p->weight/10;
        qp = CLIP3(minQP,maxQP,qp);
      }
      else
      {
        minQP = p->qp_prev - 5*QP_FIXPOINT_0_POINT_1*p->weight/10;
        maxQP = p->qp_prev + 5*QP_FIXPOINT_0_POINT_1*p->weight/10;
        qp=p->qp_prev+(qp-p->qp_prev)*p->weight/10;
        qp = CLIP3(minQP,maxQP,qp);
      }
    }
    return qp;
}


/*------------------------------------------------------------------------------
  get_avg_bits()
------------------------------------------------------------------------------*/
static i32 get_avg_bits(linReg_s *p, i32 n)
{
  i32 i;
  i64 sum = 0;
  i32 pos = p->pos;

  if (!p->len) return 0;

  if (n == -1 || n > p->len)
    n = p->len;

  i = n;
  while (i--)
  {
    if (pos) pos--;
    else pos = p->len - 1;
    sum += p->bits[pos];
    if (sum < 0)
    {
/* INF3: DIV with math64.h */
      return DIV_S64(I64_MAX , (n - i));
    //return I64_MAX / (n - i);
    }
  }
/* INF3: DIV with math64.h */
  return DIV_DIV_S64(sum, n);
//return DIV(sum, n);
}

/*------------------------------------------------------------------------------
  update_tables()  only statistics of PSLICE, please.
------------------------------------------------------------------------------*/
static void update_tables(linReg_s *p, i32 qp, i32 bits)
{
  const i32 clen = RC_TABLE_LENGTH;
  i32 tmp = p->pos;

  p->qp_prev   = qp;
  p->qs[tmp]   = q_step[((qp*10)>>QP_FRACTIONAL_BITS)];
  p->bits[tmp] = bits;

  if (++p->pos >= clen)
  {
    p->pos = 0;
  }
  if (p->len < clen)
  {
    p->len++;
  }
}

/*------------------------------------------------------------------------------
            update_model()  Update model parameter by Linear Regression.
------------------------------------------------------------------------------*/
static void update_model(linReg_s *p)
{
  i32 i, n = p->len,*r = p->bits;
  i32 *qs = p->qs ;
  i64  a1, a2, sx = lin_sx(qs, n), sy = lin_sy(qs, r, n);

  for (i = 0; i < n; i++)
  {
    DBG(2, (DBGOUTPUT, "model: qs %i  r %i\n", qs[i], r[i]));
  }

  a1 = lin_sxy(qs, r, n);
  //printf("lin_sxy a1=%d,n=%d\n",a1,n);
/* INF3: DIV with math64.h */
  a1 = a1 < DIV_S64(I64_MAX , n) ? a1 * n : I64_MAX;
//a1 = a1 < I64_MAX / n ? a1 * n : I64_MAX;
  //printf("\n r[0]=%d,sy=%d,sx=%d,a1=%d\n",r[0],sy,sx,a1);
  if (sy == 0)
  {
    a1 = 0;
    //printf("if a1=0");
  }
  else
  {
/* INF3: DIV with math64.h */
    a1 -= sx < DIV64_S64(I64_MAX , sy) ? sx * sy : I64_MAX;
  //a1 -= sx < I64_MAX / sy ? sx * sy : I64_MAX;
    //printf("else a1=%d",a1);
  }
  //printf("before a2 calc, a1=%d\n",a1);
  a2 = (lin_nsxx(qs, n) - (sx * sx));
  //printf("a2=%d\n",a2);
  if (a2 == 0)
  {
    if (p->a1 == 0)
    {
      /* If encountered in the beginning */
      a1 = 0;
    }
    else
    {
/* INF3: DIV with math64.h */
      a1 = DIV_S64(p->a1 * 2 , 3);
    //a1 = (p->a1 * 2) / 3;
    }
  }
  else
  {
    //printf("a1=%d,a2=%d\n",a1,a2);
    a1 = HevcCalculate(a1, DSCY, a2);
    //printf("a1*DSCY_64/a2=%d\n",a1);
  }
  //printf("before ceiling a1=%d\n",a1);
  /* Value of a1 shouldn't be excessive (small) */
  a1 = MAX(a1, -262144);      //????
  a1 = MIN(a1,  262143);      //????
  //printf("middle ceiling a1=%d\n",a1);
  a1 = MAX(a1, -I64_MAX / q_step[510] / RC_TABLE_LENGTH);
  a1 = MIN(a1,  I64_MAX / q_step[510] / RC_TABLE_LENGTH);
  //printf("end ceiling a1=%d\n",a1);


  ASSERT(ABS(a1) * sx >= 0);
  ASSERT(sx * DSCY >= 0);
  /* INF3: DIV with math64.h */
  a2 = DIV_DIV_S64(sy * DSCY, n) - DIV_DIV_S64(a1 * sx, n);
//a2 = DIV(sy * DSCY, n) - DIV(a1 * sx, n);

  DBG(2, (DBGOUTPUT, "model: a2:%9d  a1:%8d\n", a2, a1));

  if (p->len > 0)
  {
    p->a1 = a1;
    p->a2 = a2;
  }
  //printf("update_model p->a1=%d,p->a2=%d\n",p->a1,p->a2);
}

/*------------------------------------------------------------------------------
  lin_sy()  calculate value of Sy for n points.
------------------------------------------------------------------------------*/
static i64 lin_sy(i32 *qp, i32 *r, i32 n)
{
  i64 sum = 0;

  while (n--)
  {
    sum += (i64)qp[n] * qp[n] * (r[n] << QP_FRACTIONAL_BITS);
    if (sum < 0)
    {
      return I64_MAX / DSCY;
    }
  }
  /* INF3: DIV with math64.h */
  return (DIV_DIV_S64(sum, DSCY));
//return (DIV(sum, DSCY));
}

/*------------------------------------------------------------------------------
  lin_sx()  calculate value of Sx for n points.
------------------------------------------------------------------------------*/
static i64 lin_sx(i32 *qp, i32 n)
{
  i64 tmp = 0;

  while (n--)
  {
    ASSERT(qp[n]);
    tmp += qp[n];
  }
  return tmp;
}

/*------------------------------------------------------------------------------
  lin_sxy()  calculate value of Sxy for n points.
------------------------------------------------------------------------------*/
static i64 lin_sxy(i32 *qp, i32 *r, i32 n)
{
  i64 tmp, sum = 0;

  while (n--)
  {
    tmp = (i64)qp[n] * (i64)qp[n] * (i64)qp[n];
    //tmp =>> (QP_FRACTIONAL_BITS*3);
    if (tmp > ((i64)r[n] << (3*QP_FRACTIONAL_BITS)))
    {
      /* INF3: DIV with math64.h */
      sum += DIV_S64(tmp, DSCY) * ((i64)r[n] << (QP_FRACTIONAL_BITS));
    //sum += DIV(tmp, DSCY) * (r[n] << (QP_FRACTIONAL_BITS));
    }
    else
    {
      /* INF3: DIV with math64.h */
      sum += tmp * DIV_S64((((i64)r[n]) << (QP_FRACTIONAL_BITS)), DSCY);
    //sum += tmp * DIV((r[n] << (QP_FRACTIONAL_BITS)), DSCY);
    }
    if (sum < 0)
    {
      return I64_MAX;
    }
  }
  return sum;
}

/*------------------------------------------------------------------------------
  lin_nsxx()  calculate value of n * Sxy for n points.
------------------------------------------------------------------------------*/
static i64 lin_nsxx(i32 *qp, i32 n)
{
  i64 d = n;
  i64 tmp = 0, sum = 0;

  while (n--)
  {
    tmp = qp[n];
    tmp *= tmp;
    //tmp =>> (QP_FRACTIONAL_BITS*2);
    sum += d * tmp;
  }
  return sum;
}

static void MaAddFrame(rc_ma_s *ma, i32 frameSizeBits)
{
    ma->frame[ma->pos++] = frameSizeBits;

    if (ma->pos == ma->length)
        ma->pos = 0;

    if (ma->count < ma->length)
        ma->count++;
}
/* INF3: kill warnings */
#if 0
/*------------------------------------------------------------------------------
    Calculate average bitrate of moving window
------------------------------------------------------------------------------*/
static i32 Ma(rc_ma_s *ma)
{
    i32 i;
    unsigned long long sum = 0;     /* Using 64-bits to avoid overflow */

    for (i = 0; i < ma->count; i++)
    {
        sum += ma->frame[i];
        //printf("i=%d,ma->frame[i]=%d\n",i,ma->frame[i]);
    }

    if (!ma->frameRateDenom)
        return 0;


    sum = sum / ma->count;


    return sum * (ma->frameRateNumer+ma->frameRateDenom-1) / ma->frameRateDenom;
}
#endif
/*------------------------------------------------------------------------------
    Calculate average bitrate of moving window -1
------------------------------------------------------------------------------*/
static i32 moving_totalBit_window_minus1(rc_ma_s *ma)
{
    i32 i=0;
    i32 sum = 0;     /* Using 64-bits to avoid overflow */
    i32 readptr;
    readptr=ma->pos-1;
    if(readptr<0)
      readptr=ma->length-1;
    while(1)
    {
      if(i==ma->count-1)
        break;
      sum += ma->frame[readptr];
      readptr--;
      if(readptr<0)
      readptr=ma->length-1;
      i++;
    }
    return sum;
}
