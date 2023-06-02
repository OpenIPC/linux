#ifndef _GPIO_ELEC_SC1135_V1_H_
#define _GPIO_ELEC_SC1135_V1_H_

#ifdef CONFIG_PHY_USE_AO_MCLK
    #define SYSTEM_GPIO_PHY_CLK     GPIO_TYPE_OUTPUT_AOMCLK
#endif
#ifdef CONFIG_PHY_USE_SD_CLK
    #define SYSTEM_GPIO_PHY_CLK     GPIO_TYPE_OUTPUT_SDIO_CLK
#endif
#ifdef CONFIG_PHY_USE_EXTERN_CLK
    #define SYSTEM_GPIO_PHY_CLK     GPIO_TYPE_INPUT
#endif


#if defined(CONFIG_MACH_GK7101S_ELEC_SC1135_V1_00) || defined(CONFIG_MACH_GK7102S_ELEC_SC1135_V1_00)
#define SYSTEM_GPIO_EXTPHY_TABLE                          \
    { GPIO_3 , GPIO_TYPE_INPUT_ENET_PHY_RXDV           }, \
    { GPIO_30, GPIO_TYPE_OUTPUT_ENET_PHY_TXEN          }, \
    { GPIO_31, GPIO_TYPE_OUTPUT_ENET_PHY_TXD_1         }, \
    { GPIO_32, GPIO_TYPE_OUTPUT_ENET_PHY_TXD_0         }, \
    { GPIO_33, GPIO_TYPE_INPUT_ENET_PHY_RXER           }, \
    { GPIO_35, GPIO_TYPE_OUTPUT_ENET_GMII_MDC_O        }, \
    { GPIO_36, GPIO_TYPE_INOUT_ETH_MDIO                }, \
    { GPIO_48, GPIO_TYPE_INPUT                         }, \
    { GPIO_49, GPIO_TYPE_OUTPUT_1         /*ETH reset*/}, \
    { GPIO_50, SYSTEM_GPIO_PHY_CLK                     }, \
    { GPIO_51, GPIO_TYPE_INPUT_ENET_PHY_RXD_1          }, \
    { GPIO_52, GPIO_TYPE_INPUT_ENET_PHY_RXD_2   /*CLK*/}, \
    { GPIO_53, GPIO_TYPE_INPUT_ENET_PHY_RXD_0          }, \
    { GPIO_54, GPIO_TYPE_INPUT                         },

#define SYSTEM_GPIO_INTPHY_TABLE                          \
    { GPIO_3 , GPIO_TYPE_INPUT                         }, \
    { GPIO_30, GPIO_TYPE_INPUT                         }, \
    { GPIO_31, GPIO_TYPE_INPUT                         }, \
    { GPIO_32, GPIO_TYPE_INPUT                         }, \
    { GPIO_33, GPIO_TYPE_INPUT                         }, \
    { GPIO_35, GPIO_TYPE_INPUT                         }, \
    { GPIO_36, GPIO_TYPE_INPUT                         }, \
    { GPIO_48, SYSTEM_GPIO_PHY_CLK                     }, \
    { GPIO_49, GPIO_TYPE_INPUT                         }, \
    { GPIO_50, GPIO_TYPE_INPUT                         }, \
    { GPIO_51, GPIO_TYPE_INPUT                         }, \
    { GPIO_52, GPIO_TYPE_INPUT                         }, \
    { GPIO_53, GPIO_TYPE_INPUT                         }, \
    { GPIO_54, GPIO_TYPE_OUTPUT_EPHY_LED_3             },

#define SYSTEM_GPIO_XREF_TABLE                            \
    { GPIO_0 , GPIO_TYPE_OUTPUT_SF_CS0                 }, \
    { GPIO_1 , GPIO_TYPE_OUTPUT_SF_CS1                 }, \
    { GPIO_2 , GPIO_TYPE_INPUT                         }, \
    { GPIO_4 , GPIO_TYPE_INPUT                         }, \
    { GPIO_5 , GPIO_TYPE_INPUT                         }, \
    { GPIO_6 , GPIO_TYPE_INPUT                         }, \
    { GPIO_7 , GPIO_TYPE_INPUT                         }, \
    { GPIO_8 , GPIO_TYPE_INPUT                         }, \
    { GPIO_9 , GPIO_TYPE_INPUT                         }, \
    { GPIO_10, GPIO_TYPE_INPUT                         }, \
    { GPIO_11, GPIO_TYPE_INPUT                         }, \
    { GPIO_12, GPIO_TYPE_OUTPUT_0       /*IR_LED_CTRL*/}, \
    { GPIO_13, GPIO_TYPE_INPUT                         }, \
    { GPIO_14, GPIO_TYPE_INPUT                         }, \
    { GPIO_15, GPIO_TYPE_INPUT           /*IR_DET_ADC*/}, \
    { GPIO_16, GPIO_TYPE_INPUT                         }, \
    { GPIO_17, GPIO_TYPE_INPUT                         }, \
    { GPIO_18, GPIO_TYPE_INPUT                         }, \
    { GPIO_19, GPIO_TYPE_INPUT                         }, \
    { GPIO_20, GPIO_TYPE_OUTPUT_0           /*IR_CUT+*/}, \
    { GPIO_21, GPIO_TYPE_INPUT                         }, \
    { GPIO_22, GPIO_TYPE_INPUT                         }, \
    { GPIO_23, GPIO_TYPE_OUTPUT_0           /*IR_CUT-*/}, \
    { GPIO_24, GPIO_TYPE_INPUT                         }, \
    { GPIO_25, GPIO_TYPE_INPUT_UART0_RX                }, \
    { GPIO_26, GPIO_TYPE_OUTPUT_UART0_TX               }, \
    { GPIO_27, GPIO_TYPE_OUTPUT_1      /*Sensor reset*/}, \
    { GPIO_28, GPIO_TYPE_INOUT_I2C_DATA         /*S D*/}, \
    { GPIO_29, GPIO_TYPE_INOUT_I2C_CLK          /*S C*/}, \
    { GPIO_34, GPIO_TYPE_INPUT                         }, \
    { GPIO_37, GPIO_TYPE_INPUT                         }, \
    { GPIO_38, GPIO_TYPE_INPUT                         }, \
    { GPIO_39, GPIO_TYPE_INPUT_SD_CD_N                 }, \
    { GPIO_40, GPIO_TYPE_INPUT                         }, \
    { GPIO_41, GPIO_TYPE_INOUT_SD_DATA_3               }, \
    { GPIO_42, GPIO_TYPE_INOUT_SD_DATA_2               }, \
    { GPIO_43, GPIO_TYPE_INOUT_SD_CMD                  }, \
    { GPIO_44, GPIO_TYPE_OUTPUT_SDIO_CLK               }, \
    { GPIO_45, GPIO_TYPE_INOUT_SD_DATA_1               }, \
    { GPIO_46, GPIO_TYPE_OUTPUT_1/*SDIO_CARD_POWER_EN*/}, \
    { GPIO_47, GPIO_TYPE_INOUT_SD_DATA_0               }, \
    { GPIO_55, GPIO_TYPE_INPUT                         }, \
    { GPIO_56, GPIO_TYPE_INPUT                         }, \
    { GPIO_57, GPIO_TYPE_INPUT                         }, \
    { GPIO_58, GPIO_TYPE_INPUT                         }, \
    { GPIO_59, GPIO_TYPE_INPUT                         }, \
    { GPIO_60, GPIO_TYPE_INPUT                         }, \
    { GPIO_61, GPIO_TYPE_INPUT                         }, \
    { GPIO_62, GPIO_TYPE_INPUT                         }

#define SYSTEM_GPIO_IR_LED_CTRL     GPIO_4  // not real pin
#define SYSTEM_GPIO_IR_CUT1         GPIO_20
#define SYSTEM_GPIO_IR_CUT2         GPIO_23
#define SYSTEM_GPIO_SENSOR_RESET    GPIO_27
#define SYSTEM_GPIO_PHY_RESET       GPIO_49
#define SYSTEM_GPIO_PHY_SPEED_LED   GPIO_54

#define SYSTEM_GPIO_SPI0_EN0        GPIO_5  // not real pin
#define SYSTEM_GPIO_SPI1_EN0        GPIO_6  // not real pin
#define SYSTEM_GPIO_PWM0            GPIO_7  // not real pin
#define SYSTEM_GPIO_PWM1            GPIO_8  // not real pin
#define SYSTEM_GPIO_PWM2            GPIO_9  // not real pin
#define SYSTEM_GPIO_PWM3            GPIO_10 // not real pin
#define SYSTEM_GPIO_USB_HOST        GPIO_11 // not real pin
#define SYSTEM_GPIO_SD_DETECT       GPIO_39
#define SYSTEM_GPIO_SD_POWER        GPIO_46
#endif /* CONFIG_MACH_GK7102S_ELEC_SC1135_V1_00 */

#endif /* _GPIO_RB_SC1035_H_ */

