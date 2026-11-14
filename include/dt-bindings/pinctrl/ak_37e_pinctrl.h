/*
 * This header provides constants for the AK37E GPIO binding.
 */
#ifndef __AK_37E_PINCTRL_H__
#define __AK_37E_PINCTRL_H__

#define XGPIO_000                      0
#define XGPIO_000_FUNC_GPIO0           0
#define XGPIO_000_FUNC_RMII0_MDIO      1
#define XGPIO_000_FUNC_SD2_D1          2
#define XGPIO_000_FUNC_I2S0_DIN        3
#define XGPIO_000_FUNC_TWI0_SCL        4

#define XGPIO_001                      1
#define XGPIO_001_FUNC_GPIO1           0
#define XGPIO_001_FUNC_RMII0_MDC       1
#define XGPIO_001_FUNC_SD2_D0          2
#define XGPIO_001_FUNC_I2S0_LRCLK      3
#define XGPIO_001_FUNC_TWI0_SDA        4

#define XGPIO_002                      2
#define XGPIO_002_FUNC_GPIO2           0
#define XGPIO_002_FUNC_RMII0_RXER      1
#define XGPIO_002_FUNC_SD2_MCLK        2
#define XGPIO_002_FUNC_I2S0_BCLK       3
#define XGPIO_002_FUNC_PWM5            4

#define XGPIO_003                      3
#define XGPIO_003_FUNC_GPIO3           0
#define XGPIO_003_FUNC_RMII0_RXDV      1
#define XGPIO_003_FUNC_SD2_CMD         2
#define XGPIO_003_FUNC_I2S0_MCLK       3
#define XGPIO_003_FUNC_PWM4            4

#define XGPIO_004                      4
#define XGPIO_004_FUNC_GPIO4           0
#define XGPIO_004_FUNC_RMII0_RXD0      1
#define XGPIO_004_FUNC_SD2_D3          2
#define XGPIO_004_FUNC_I2S0_DOUT       3

#define XGPIO_005                      5
#define XGPIO_005_FUNC_GPIO5           0
#define XGPIO_005_FUNC_RMII0_RXD1      1
#define XGPIO_005_FUNC_SD2_D2          2
#define XGPIO_005_FUNC_PWM3            3

#define XGPIO_006                      6
#define XGPIO_006_FUNC_GPIO6           0
#define XGPIO_006_FUNC_OPCLK0          1
#define XGPIO_006_FUNC_TWI3_SCL        2
#define XGPIO_006_FUNC_PWM2            3
#define XGPIO_006_FUNC_I2S1_LRCLK      4

#define XGPIO_007                      7
#define XGPIO_007_FUNC_GPIO7           0
#define XGPIO_007_FUNC_RMII0_TXD0      1
#define XGPIO_007_FUNC_TWI3_SDA        2
#define XGPIO_007_FUNC_PWM1            3
#define XGPIO_007_FUNC_I2S1_BCLK       4

#define XGPIO_008                      8
#define XGPIO_008_FUNC_GPIO8           0
#define XGPIO_008_FUNC_RMII0_TXD1      1
#define XGPIO_008_FUNC_UART3_RXD       2
#define XGPIO_008_FUNC_PDM_CLK         3
#define XGPIO_008_FUNC_I2S1_MCLK       4

#define XGPIO_009                      9
#define XGPIO_009_FUNC_GPIO9           0
#define XGPIO_009_FUNC_RMII0_TXEN      1
#define XGPIO_009_FUNC_UART3_TXD       2
#define XGPIO_009_FUNC_PDM_DATA        3
#define XGPIO_009_FUNC_I2S1_DIN        4

#define XGPIO_010                      10
#define XGPIO_010_FUNC_GPIO10          0
#define XGPIO_010_FUNC_SPI0_CS0        1

#define XGPIO_011                      11
#define XGPIO_011_FUNC_GPIO11          0
#define XGPIO_011_FUNC_SPI0_HOLD       1

#define XGPIO_012                      12
#define XGPIO_012_FUNC_GPIO12          0
#define XGPIO_012_FUNC_SPI0_DIN        1

#define XGPIO_013                      13
#define XGPIO_013_FUNC_GPIO13          0
#define XGPIO_013_FUNC_SPI0_SCLK       1

#define XGPIO_014                      14
#define XGPIO_014_FUNC_GPIO14          0
#define XGPIO_014_FUNC_SPI0_WP         1

#define XGPIO_015                      15
#define XGPIO_015_FUNC_GPIO15          0
#define XGPIO_015_FUNC_SPI0_DOUT       1

#define XGPIO_016                      16
#define XGPIO_016_FUNC_GPIO16          0
#define XGPIO_016_FUNC_SPI1_CS0        1
#define XGPIO_016_FUNC_SD1_D1          2
#define XGPIO_016_FUNC_TWI1_SCL        3
#define XGPIO_016_FUNC_PWM1            4

#define XGPIO_017                      17
#define XGPIO_017_FUNC_GPIO17          0
#define XGPIO_017_FUNC_SPI1_DIN        1
#define XGPIO_017_FUNC_SD1_D0          2
#define XGPIO_017_FUNC_TWI1_SDA        3
#define XGPIO_017_FUNC_PWM4            4

#define XGPIO_018                      18
#define XGPIO_018_FUNC_GPIO18          0
#define XGPIO_018_FUNC_SPI1_SCLK       1
#define XGPIO_018_FUNC_SD1_MCLK        2

#define XGPIO_019                      19
#define XGPIO_019_FUNC_GPIO19          0
#define XGPIO_019_FUNC_SPI1_DOUT       1
#define XGPIO_019_FUNC_SD1_CMD         2

#define XGPIO_020                      20
#define XGPIO_020_FUNC_GPIO20          0
#define XGPIO_020_FUNC_SPI1_WP         1
#define XGPIO_020_FUNC_SD1_D3          2
#define XGPIO_020_FUNC_UART2_RXD       3
#define XGPIO_020_FUNC_PWM3            4

#define XGPIO_021                      21
#define XGPIO_021_FUNC_GPIO21          0
#define XGPIO_021_FUNC_SPI1_HOLD       1
#define XGPIO_021_FUNC_SD1_D2          2
#define XGPIO_021_FUNC_UART2_TXD       3
#define XGPIO_021_FUNC_PWM2            4

#define XGPIO_022                      22
#define XGPIO_022_FUNC_GPIO22          0
#define XGPIO_022_FUNC_UART0_RXD       1
#define XGPIO_022_FUNC_PWM4            2

#define XGPIO_023                      23
#define XGPIO_023_FUNC_GPIO23          0
#define XGPIO_023_FUNC_UART0_TXD       1
#define XGPIO_023_FUNC_PWM5            2

#define XGPIO_024                      24
#define XGPIO_024_FUNC_GPIO24          0
#define XGPIO_024_FUNC_UART1_RXD       1
#define XGPIO_024_FUNC_SPI2_CS0        2
#define XGPIO_024_FUNC_PWM3            3
#define XGPIO_024_FUNC_RGB_D20         4

#define XGPIO_025                      25
#define XGPIO_025_FUNC_GPIO25          0
#define XGPIO_025_FUNC_UART1_TXD       1
#define XGPIO_025_FUNC_SPI2_SCLK       2
#define XGPIO_025_FUNC_PWM2            3
#define XGPIO_025_FUNC_RGB_D21         4

#define XGPIO_026                      26
#define XGPIO_026_FUNC_GPIO26          0
#define XGPIO_026_FUNC_UART1_CTS       1
#define XGPIO_026_FUNC_SPI2_DIN        2
#define XGPIO_026_FUNC_PWM0            3
#define XGPIO_026_FUNC_RGB_D22         4

#define XGPIO_027                      27
#define XGPIO_027_FUNC_GPIO27          0
#define XGPIO_027_FUNC_UART1_RTS       1
#define XGPIO_027_FUNC_SPI2_DOUT       2
#define XGPIO_027_FUNC_PWM5            3
#define XGPIO_027_FUNC_RGB_D23         4

#define XGPIO_028                      28
#define XGPIO_028_FUNC_GPIO28          0
#define XGPIO_028_FUNC_UART2_RXD       1
#define XGPIO_028_FUNC_PWM1            2
#define XGPIO_028_FUNC_SPI2_CS1        3
#define XGPIO_028_FUNC_RGB_D19         4

#define XGPIO_029                      29
#define XGPIO_029_FUNC_GPIO29          0
#define XGPIO_029_FUNC_UART2_TXD       1
#define XGPIO_029_FUNC_PWM3            2
#define XGPIO_029_FUNC_SPI1_CS1        3
#define XGPIO_029_FUNC_RGB_D18         4

#define XGPIO_030                      30
#define XGPIO_030_FUNC_GPIO30          0
#define XGPIO_030_FUNC_TWI0_SCL        1

#define XGPIO_031                      31
#define XGPIO_031_FUNC_GPIO31          0
#define XGPIO_031_FUNC_TWI0_SDA        1

#define XGPIO_032                      32
#define XGPIO_032_FUNC_GPIO32          0
#define XGPIO_032_FUNC_TWI1_SCL        1

#define XGPIO_033                      33
#define XGPIO_033_FUNC_GPIO33          0
#define XGPIO_033_FUNC_TWI1_SDA        1

#define XGPIO_034                      34
#define XGPIO_034_FUNC_GPIO34          0
#define XGPIO_034_FUNC_RGB_VOGATE      1
#define XGPIO_034_FUNC_MPU_RD          2

#define XGPIO_035                      35
#define XGPIO_035_FUNC_GPIO35          0
#define XGPIO_035_FUNC_RGB_VOVSYNC     1
#define XGPIO_035_FUNC_MPU_A0          2

#define XGPIO_036                      36
#define XGPIO_036_FUNC_GPIO36          0
#define XGPIO_036_FUNC_RGB_VOHSYNC     1
#define XGPIO_036_FUNC_MPU_CS          2

#define XGPIO_037                      37
#define XGPIO_037_FUNC_GPIO37          0
#define XGPIO_037_FUNC_RGB_VOPCLK      1
#define XGPIO_037_FUNC_MPU_WR          2

#define XGPIO_038                      38
#define XGPIO_038_FUNC_GPIO38          0
#define XGPIO_038_FUNC_RGB_D0          1
#define XGPIO_038_FUNC_MPU_D0          2

#define XGPIO_039                      39
#define XGPIO_039_FUNC_GPIO39          0
#define XGPIO_039_FUNC_RGB_D1          1
#define XGPIO_039_FUNC_MPU_D1          2

#define XGPIO_040                      40
#define XGPIO_040_FUNC_GPIO40          0
#define XGPIO_040_FUNC_RGB_D2          1
#define XGPIO_040_FUNC_MPU_D2          2

#define XGPIO_041                      41
#define XGPIO_041_FUNC_GPIO41          0
#define XGPIO_041_FUNC_RGB_D3          1
#define XGPIO_041_FUNC_MPU_D3          2

#define XGPIO_042                      42
#define XGPIO_042_FUNC_GPIO42          0
#define XGPIO_042_FUNC_RGB_D4          1
#define XGPIO_042_FUNC_MPU_D4          2

#define XGPIO_043                      43
#define XGPIO_043_FUNC_GPIO43          0
#define XGPIO_043_FUNC_RGB_D5          1
#define XGPIO_043_FUNC_MPU_D5          2

#define XGPIO_044                      44
#define XGPIO_044_FUNC_GPIO44          0
#define XGPIO_044_FUNC_RGB_D6          1
#define XGPIO_044_FUNC_MPU_D6          2

#define XGPIO_045                      45
#define XGPIO_045_FUNC_GPIO45          0
#define XGPIO_045_FUNC_RGB_D7          1
#define XGPIO_045_FUNC_MPU_D7          2

#define XGPIO_046                      46
#define XGPIO_046_FUNC_GPIO46          0
#define XGPIO_046_FUNC_RGB_D8          1
#define XGPIO_046_FUNC_MPU_D8          2
#define XGPIO_046_FUNC_TWI2_SCL        3

#define XGPIO_047                      47
#define XGPIO_047_FUNC_GPIO47          0
#define XGPIO_047_FUNC_RGB_D9          1
#define XGPIO_047_FUNC_MPU_D9          2
#define XGPIO_047_FUNC_TWI2_SDA        3

#define XGPIO_048                      48
#define XGPIO_048_FUNC_GPIO48          0
#define XGPIO_048_FUNC_RGB_D10         1
#define XGPIO_048_FUNC_MPU_D10         2
#define XGPIO_048_FUNC_JTAG_RSTN       3

#define XGPIO_049                      49
#define XGPIO_049_FUNC_GPIO49          0
#define XGPIO_049_FUNC_RGB_D11         1
#define XGPIO_049_FUNC_MPU_D11         2
#define XGPIO_049_FUNC_JTAG_TDI        3

#define XGPIO_050                      50
#define XGPIO_050_FUNC_GPIO50          0
#define XGPIO_050_FUNC_RGB_D12         1
#define XGPIO_050_FUNC_MPU_D12         2
#define XGPIO_050_FUNC_JTAG_TMS        3
#define XGPIO_050_FUNC_I2S1_LRCLK      4

#define XGPIO_051                      51
#define XGPIO_051_FUNC_GPIO51          0
#define XGPIO_051_FUNC_RGB_D13         1
#define XGPIO_051_FUNC_MPU_D13         2
#define XGPIO_051_FUNC_JTAG_TCLK       3
#define XGPIO_051_FUNC_I2S1_BCLK       4

#define XGPIO_052                      52
#define XGPIO_052_FUNC_GPIO52          0
#define XGPIO_052_FUNC_RGB_D14         1
#define XGPIO_052_FUNC_MPU_D14         2
#define XGPIO_052_FUNC_JTAG_RTCK       3
#define XGPIO_052_FUNC_I2S1_MCLK       4

#define XGPIO_053                      53
#define XGPIO_053_FUNC_GPIO53          0
#define XGPIO_053_FUNC_RGB_D15         1
#define XGPIO_053_FUNC_MPU_D15         2
#define XGPIO_053_FUNC_JTAG_TDO        3
#define XGPIO_053_FUNC_I2S1_DIN        4

#define XGPIO_054                      54
#define XGPIO_054_FUNC_GPIO54          0
#define XGPIO_054_FUNC_RGB_D16         1
#define XGPIO_054_FUNC_MPU_D16         2
#define XGPIO_054_FUNC_PDM_CLK         3
#define XGPIO_054_FUNC_TWI3_SCL        4

#define XGPIO_055                      55
#define XGPIO_055_FUNC_GPIO55          0
#define XGPIO_055_FUNC_RGB_D17         1
#define XGPIO_055_FUNC_MPU_D17         2
#define XGPIO_055_FUNC_PDM_DATA        3
#define XGPIO_055_FUNC_TWI3_SDA        4

#define XGPIO_056                      56
#define XGPIO_056_FUNC_GPIO56          0
#define XGPIO_056_FUNC_CSI0_SCLK       1
#define XGPIO_056_FUNC_UART3_RXD       2

#define XGPIO_057                      57
#define XGPIO_057_FUNC_GPIO57          0
#define XGPIO_057_FUNC_CSI0_PCLK       1
#define XGPIO_057_FUNC_UART3_TXD       2

#define XGPIO_058                      58
#define XGPIO_058_FUNC_GPIO58          0
#define XGPIO_058_FUNC_CSI0_HSYNC      1
#define XGPIO_058_FUNC_RMII1_MDIO      2
#define XGPIO_058_FUNC_SD2_D1          3

#define XGPIO_059                      59
#define XGPIO_059_FUNC_GPIO59          0
#define XGPIO_059_FUNC_CSI0_VSYNC      1
#define XGPIO_059_FUNC_RMII1_MDC       2
#define XGPIO_059_FUNC_SD2_D0          3

#define XGPIO_060                      60
#define XGPIO_060_FUNC_GPIO60          0
#define XGPIO_060_FUNC_CSI0_D0         1
#define XGPIO_060_FUNC_RMII1_RXER      2
#define XGPIO_060_FUNC_SD2_MCLK        3

#define XGPIO_061                      61
#define XGPIO_061_FUNC_GPIO61          0
#define XGPIO_061_FUNC_CSI0_D1         1
#define XGPIO_061_FUNC_RMII1_RXDV      2
#define XGPIO_061_FUNC_SD2_CMD         3

#define XGPIO_062                      62
#define XGPIO_062_FUNC_GPIO62          0
#define XGPIO_062_FUNC_CSI0_D2         1
#define XGPIO_062_FUNC_RMII1_RXD0      2
#define XGPIO_062_FUNC_SD2_D3          3

#define XGPIO_063                      63
#define XGPIO_063_FUNC_GPIO63          0
#define XGPIO_063_FUNC_CSI0_D3         1
#define XGPIO_063_FUNC_RMII1_RXD1      2
#define XGPIO_063_FUNC_SD2_D2          3

#define XGPIO_064                      64
#define XGPIO_064_FUNC_GPIO64          0
#define XGPIO_064_FUNC_CSI0_D4         1
#define XGPIO_064_FUNC_OPCLK1          2
#define XGPIO_064_FUNC_TWI2_SCL        3
#define XGPIO_064_FUNC_I2S1_LRCLK      4

#define XGPIO_065                      65
#define XGPIO_065_FUNC_GPIO65          0
#define XGPIO_065_FUNC_CSI0_D5         1
#define XGPIO_065_FUNC_RMII1_TXD0      2
#define XGPIO_065_FUNC_TWI2_SDA        3
#define XGPIO_065_FUNC_I2S1_BCLK       4

#define XGPIO_066                      66
#define XGPIO_066_FUNC_GPIO66          0
#define XGPIO_066_FUNC_CSI0_D6         1
#define XGPIO_066_FUNC_RMII1_TXD1      2
#define XGPIO_066_FUNC_PDM_CLK         3
#define XGPIO_066_FUNC_I2S1_MCLK       4

#define XGPIO_067                      67
#define XGPIO_067_FUNC_GPIO67          0
#define XGPIO_067_FUNC_CSI0_D7         1
#define XGPIO_067_FUNC_RMII1_TXEN      2
#define XGPIO_067_FUNC_PDM_DATA        3
#define XGPIO_067_FUNC_I2S1_DIN        4

#define XGPIO_068                      68
#define XGPIO_068_FUNC_GPIO68          0
#define XGPIO_068_FUNC_CSI1_SCLK       1
#define XGPIO_068_FUNC_SD0_D2          2
#define XGPIO_068_FUNC_SPI2_CS0        3

#define XGPIO_069                      69
#define XGPIO_069_FUNC_GPIO69          0
#define XGPIO_069_FUNC_CSI1_PCLK       1
#define XGPIO_069_FUNC_SD0_D3          2
#define XGPIO_069_FUNC_SPI2_HOLD       3

#define XGPIO_070                      70
#define XGPIO_070_FUNC_GPIO70          0
#define XGPIO_070_FUNC_CSI1_HSYNC      1
#define XGPIO_070_FUNC_SD0_D4          2
#define XGPIO_070_FUNC_SPI2_DIN        3
#define XGPIO_070_FUNC_UART1_RXD       4

#define XGPIO_071                      71
#define XGPIO_071_FUNC_GPIO71          0
#define XGPIO_071_FUNC_CSI1_VSYNC      1
#define XGPIO_071_FUNC_SD0_CMD         2
#define XGPIO_071_FUNC_SPI2_SCLK       3
#define XGPIO_071_FUNC_UART1_TXD       4

#define XGPIO_072                      72
#define XGPIO_072_FUNC_GPIO72          0
#define XGPIO_072_FUNC_CSI1_D0         1
#define XGPIO_072_FUNC_SD0_D5          2
#define XGPIO_072_FUNC_SPI2_WP         3
#define XGPIO_072_FUNC_UART1_CTS       4

#define XGPIO_073                      73
#define XGPIO_073_FUNC_GPIO73          0
#define XGPIO_073_FUNC_CSI1_D1         1
#define XGPIO_073_FUNC_SD0_CLK         2
#define XGPIO_073_FUNC_SPI2_DOUT       3
#define XGPIO_073_FUNC_UART1_RTS       4

#define XGPIO_074                      74
#define XGPIO_074_FUNC_GPIO74          0
#define XGPIO_074_FUNC_CSI1_D2         1
#define XGPIO_074_FUNC_SD0_D6          2
#define XGPIO_074_FUNC_RGB_D23         3
#define XGPIO_074_FUNC_SPI2_CS1        4

#define XGPIO_075                      75
#define XGPIO_075_FUNC_GPIO75          0
#define XGPIO_075_FUNC_CSI1_D3         1
#define XGPIO_075_FUNC_SD0_D7          2
#define XGPIO_075_FUNC_RGB_D22         3
#define XGPIO_075_FUNC_I2S0_DIN        4

#define XGPIO_076                      76
#define XGPIO_076_FUNC_GPIO76          0
#define XGPIO_076_FUNC_CSI1_D4         1
#define XGPIO_076_FUNC_SD0_D1          2
#define XGPIO_076_FUNC_RGB_D21         3
#define XGPIO_076_FUNC_I2S0_LRCLK      4

#define XGPIO_077                      77
#define XGPIO_077_FUNC_GPIO77          0
#define XGPIO_077_FUNC_CSI1_D5         1
#define XGPIO_077_FUNC_SD0_D0          2
#define XGPIO_077_FUNC_RGB_D20         3
#define XGPIO_077_FUNC_I2S0_BCLK       4

#define XGPIO_078                      78
#define XGPIO_078_FUNC_GPIO78          0
#define XGPIO_078_FUNC_CSI1_D6         1
#define XGPIO_078_FUNC_TWI2_SCL        2
#define XGPIO_078_FUNC_RGB_D19         3
#define XGPIO_078_FUNC_I2S0_MCLK       4

#define XGPIO_079                      79
#define XGPIO_079_FUNC_GPIO79          0
#define XGPIO_079_FUNC_CSI1_D7         1
#define XGPIO_079_FUNC_TWI2_SDA        2
#define XGPIO_079_FUNC_RGB_D18         3
#define XGPIO_079_FUNC_I2S0_DOUT       4

#define XGPIO_082                      82
#define XGPIO_082_FUNC_GPIO82          0
#define XGPIO_082_FUNC_PWM0            1
#define XGPIO_082_FUNC_SPI0_CS1        2
#define XGPIO_082_FUNC_SPI1_CS1        3

#define XGPIO_083                      83
#define XGPIO_083_FUNC_GPIO83          0
#define XGPIO_083_FUNC_TWI2_SCL        1
#define XGPIO_083_FUNC_SPI2_CS0        2
#define XGPIO_083_FUNC_UART2_RXD       3
#define XGPIO_083_FUNC_I2S1_LRCLK      4

#define XGPIO_084                      84
#define XGPIO_084_FUNC_GPIO84          0
#define XGPIO_084_FUNC_TWI2_SDA        1
#define XGPIO_084_FUNC_SPI2_SCLK       2
#define XGPIO_084_FUNC_UART2_TXD       3
#define XGPIO_084_FUNC_I2S1_BCLK       4

#define XGPIO_085                      85
#define XGPIO_085_FUNC_GPIO85          0
#define XGPIO_085_FUNC_TWI3_SCL        1
#define XGPIO_085_FUNC_SPI2_DIN        2
#define XGPIO_085_FUNC_UART3_RXD       3
#define XGPIO_085_FUNC_I2S1_MCLK       4

#define XGPIO_086                      86
#define XGPIO_086_FUNC_GPIO86          0
#define XGPIO_086_FUNC_TWI3_SDA        1
#define XGPIO_086_FUNC_SPI2_DOUT       2
#define XGPIO_086_FUNC_UART3_TXD       3
#define XGPIO_086_FUNC_I2S1_DIN        4

#define XGPI_0                         95
#define XGPI_0_FUNC_MIC_P              0
#define XGPI_0_FUNC_GPI                1

#define XGPI_1                         96
#define XGPI_1_FUNC_MIC_N              0
#define XGPI_1_FUNC_GPI                1

#endif //__AK_37E_PINCTRL_H__
