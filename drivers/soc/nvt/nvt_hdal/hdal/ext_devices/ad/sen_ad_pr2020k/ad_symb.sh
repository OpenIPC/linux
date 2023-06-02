#!bash
rm ad_drv.c
rm ad_dtsi_parser.c
rm ad_devnode_parser.c
rm ad_i2c.c
rm ad_std_drv.c
rm ad_std_drv_param.c
ln -s ../ad_common/ad_drv/ad_drv.c
ln -s ../ad_common/ad_i2c.c
ln -s ../ad_common/ad_dtsi_parser.c
ln -s ../ad_common/ad_std_drv.c
ln -s ../ad_common/ad_std_drv_param.c
ln -s ../ad_common/ad_devnode_parser.c
