
#-------------------------------------------------------------------------------
#	Description of some variables owned by the library
#-------------------------------------------------------------------------------
# Library module (lib) or Binary module (bin)
PROCESS = lib
PATH_C +=\
  $(PATH_cam_fs_wrapper)/src

PATH_H +=\
  $(PATH_cam_fs_wrapper)/pub\
  $(PATH_spinand)/inc\
  $(PATH_spinand_hal)/inc\
  $(PATH_spinor)/inc\
  $(PATH_spinor_hal)/inc\
  $(PATH_nandflash)/inc\
  $(PATH_norflash)/inc

#-------------------------------------------------------------------------------
#	List of source files of the library or executable to generate
#-------------------------------------------------------------------------------
SRC_C_LIST =\
  cam_fs_wrapper.c \
  cam_fs_cli.c
