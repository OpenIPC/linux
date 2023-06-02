#-------------------------------------------------------------------------------
#   Description of some variables owned by the library
#-------------------------------------------------------------------------------
# Library module (lib) or Binary module (bin)
PROCESS = lib
PATH_C +=\
    $(PATH_jpe_hal)/src

PATH_H +=\
    $(PATH_jpe_hal)/inc\
    $(PATH_jpe_hal)/pub\
    $(PATH_cam_os_wrapper)/pub\
    $(PATH_jpe)/pub

#-------------------------------------------------------------------------------
#   List of source files of the library or executable to generate
#-------------------------------------------------------------------------------
SRC_C_LIST =\
    hal_jpe_ios.c\
    hal_jpe_ops.c