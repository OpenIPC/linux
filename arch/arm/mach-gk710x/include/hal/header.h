#ifndef _HEADER_H_INCLUDED_
#define _HEADER_H_INCLUDED_

typedef struct gk_hal_header_s {
    char magic[16] ;
    unsigned int major_version ;
    unsigned int minor_version ;
    unsigned char chip_name[8] ;
    unsigned char chip_stepping[8] ;
    unsigned char build_id[32] ;
    unsigned char build_date[32] ;
} gk_hal_header_t;

typedef struct gk_hal_function_info_s {
    unsigned int (*function)(unsigned int, unsigned int,
                 unsigned int, unsigned int) ;
    const char* name ;
} gk_hal_function_info_t ;

#endif // ifndef _HEADER_H_INCLUDED_
