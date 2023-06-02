#------------------------------------------------------------------------------
# Description of some variables owned by the library
#------------------------------------------------------------------------------
# Library module (lib) or Binary module (bin)
PROCESS   = lib
#------------------------------------------------------------------------------
# List of source files of the library or executable to generate
#------------------------------------------------------------------------------

PATH_C += \
          $(PATH_camclk)\
          $(PATH_camclk)/src\
          $(PATH_camclk)/src/rtk\
          $(PATH_camclk)/verify\

PATH_H += $(PATH_camclk_hal)/common\
          $(PATH_camclk_hal)/inc\
          $(PATH_camclk)/inc\
          $(PATH_camclk)/pub\
          $(PATH_cam_os_wrapper)/pub\
          $(PATH_cam_os_wrapper)/inc\
	  
PP_OPT_COMMON += CONFIG_NOTTOGATED
PP_OPT_COMMON += CONFIG_NOTTOAUTOENABLE          

PP_OPT_COMMON += CAMCLK_RTK_UNITTEST
PP_OPT_COMMON += CONFIG_CAM_CLK_SYSFS
PP_OPT_COMMON += CONFIG_CAM_CLK_PROFILING
ifneq (FALSE, $(strip $(DUALOS_SUPPORT)))
PP_OPT_COMMON += CONFIG_SS_DUALOS
endif
SRC_C_LIST += drv_camclk.c\
							drv_camclk_debug.c \
							drv_camclk_impl.c  \
							drv_camclk_module.c \
							clk_ut.c \
							drv_camclk_os.c
