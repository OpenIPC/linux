#-------------------------------------------------------------------------------
#	Description of some variables owned by the library
#-------------------------------------------------------------------------------
# Library module (lib) or Binary module (bin)
PROCESS = lib

PATH_C += \
          $(PATH_camclk_hal)/src

PATH_H += $(PATH_camclk_hal)/common\
          $(PATH_camclk_hal)/inc\
          $(PATH_camclk_hal)/pub\
          $(PATH_camclk)/inc\
          $(PATH_camclk)/pub\
          $(PATH_cam_os_wrapper)/pub\
          $(PATH_cam_os_wrapper)/inc\

SRC_C_LIST += hal_camclk_complex.c\
							hal_camclk_tbl.c

