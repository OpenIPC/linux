#-------------------------------------------------------------------------------
# Description of some variables owned by the library
#-------------------------------------------------------------------------------
# Library module (lib) or Binary module (bin)
PROCESS = lib

#PP_OPT_COMMON += __VER_PROC_TEST__

PATH_C +=\
    $(PATH_cam_proc_wrapper)/src \
    $(PATH_cam_proc_wrapper)/test

PATH_H +=\
    $(PATH_cam_proc_wrapper)/pub \
    $(PATH_cam_proc_wrapper)/inc

#-------------------------------------------------------------------------------
# List of source files of the library or executable to generate
#-------------------------------------------------------------------------------

SRC_C_LIST =\
    cam_proc_wrapper.c \
    cam_proc_cli.c \

ifeq ($(filter __VER_PROC_TEST__ ,$(PP_OPT_COMMON)),__VER_PROC_TEST__)
SRC_C_LIST +=\
    cam_proc_wrapper_test.c
endif
