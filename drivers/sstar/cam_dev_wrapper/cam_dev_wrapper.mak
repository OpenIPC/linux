
#-------------------------------------------------------------------------------
#	Description of some variables owned by the library
#-------------------------------------------------------------------------------
# Library module (lib) or Binary module (bin)
PROCESS = lib
PATH_C +=\
  $(PATH_cam_dev_wrapper)/src \
  $(PATH_cam_dev_wrapper)/test

PATH_H +=\
  $(PATH_cam_os_wrapper)/pub \
  $(PATH_cam_dev_wrapper)/pub \
  $(PATH_cam_dev_wrapper)/inc \
  $(PATH_cam_drv_poll)/pub \
  $(PATH_cam_drv_poll)/sample_driver/pub \

#-------------------------------------------------------------------------------
#	List of source files of the library or executable to generate
#-------------------------------------------------------------------------------
SRC_C_LIST =\
  cam_dev_wrapper.c \
#  cam_dev_wrapper_test.c \
