/*
    SEN DAL configuration file.

    SEN DAL configuration file. Define semaphore ID, flag ID, etc.

    @file       ctl_sen_config.c
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "kflow_videocapture/ctl_sen_config.h"
#include "sen_int.h"

// ==================================================================
// Flag
// ==================================================================
ID ctl_sen_flag_id[CTL_SEN_FLAG_COUNT] = {0};

// ==================================================================
// Semaphore
// ==================================================================
#define SEM_MAX_CNT 1
CTL_SEN_SEM_TABLE ctl_sen_semtbl[CTL_SEN_SEMAPHORE_COUNT];

/*
    Install sen dal data

    Install sen dal data. This API will be called when system start.

    @return void
*/

void ctl_sen_install_id(void)
{
	UINT32 i;

	// ctl_sen flag
	vos_flag_create(&ctl_sen_flag_id[FLG_ID_CTL_SEN], NULL, "ctl_sen");
	vos_flag_set(ctl_sen_get_flag_id(FLG_ID_CTL_SEN), CTL_SEN_FLAG_ALL);

	vos_flag_create(&ctl_sen_flag_id[FLG_ID_CTL_SEN_GLB], NULL, "ctl_sen_glb");
	vos_flag_set(ctl_sen_get_flag_id(FLG_ID_CTL_SEN_GLB), CTL_SEN_FLAG_GLB_ALL);

	// ctl_sen semphore
	for (i = 0; i < CTL_SEN_SEMAPHORE_COUNT; i++) {
		vos_sem_create(&ctl_sen_semtbl[i].semphore_id, 1, "ctl_sen_sem");
	}
}

void ctl_sen_uninstall_id(void)
{
	UINT32  i;

	for (i = 0; i < CTL_SEN_FLAG_COUNT; i++) {
		vos_flag_destroy(ctl_sen_flag_id[i]);
	}
	for (i = 0; i < CTL_SEN_SEMAPHORE_COUNT; i++) {
		vos_sem_destroy(ctl_sen_semtbl[i].semphore_id);
	}
}

SEM_HANDLE *ctl_sen_get_sem_id(CTL_SEN_SEM idx)
{
	if (idx >= CTL_SEN_SEMAPHORE_COUNT) {
		CTL_SEN_DBG_ERR("\r\n");
		return 0;
	}

	return &ctl_sen_semtbl[idx].semphore_id;
}

ID ctl_sen_get_flag_id(CTL_SEN_FLAG idx)
{
	if (idx >= CTL_SEN_FLAG_COUNT) {
		CTL_SEN_DBG_ERR("\r\n");
		return 0;
	}

	return ctl_sen_flag_id[idx];
}

