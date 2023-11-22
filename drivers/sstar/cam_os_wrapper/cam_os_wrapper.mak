#-------------------------------------------------------------------------------
# Description of some variables owned by the library
#-------------------------------------------------------------------------------
# Library module (lib) or Binary module (bin)
PROCESS = lib

#PP_OPT_COMMON += __VER_CAMOSWRAPPER__

PATH_C +=\
    $(PATH_cam_os_wrapper)/src \
    $(PATH_cam_os_wrapper)/test

PATH_H +=\
    $(PATH_cam_os_wrapper)/pub \
    $(PATH_cam_os_wrapper)/inc\
    $(PATH_drvutil_hal)/inc\

#-------------------------------------------------------------------------------
# List of source files of the library or executable to generate
#-------------------------------------------------------------------------------
SRC_C_LIST =\
    cam_os_wrapper.c \
    cam_os_informal_idr.c \
    cam_os_wrapper_test.c