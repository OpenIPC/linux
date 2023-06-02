#ifndef _RQCT_CFG_H_
#define _RQCT_CFG_H_

typedef union  rqct_cfg rqct_cfg;
typedef struct rqct_buf rqct_buf;

union rqct_cfg
{
    enum rqct_cfg_e
    {
        RQCT_CFG_SEQ= 0,
        RQCT_CFG_DQP,
        RQCT_CFG_QPR,
        RQCT_CFG_LOG,
        RQCT_CFG_PEN,
        RQCT_CFG_SPF,
        RQCT_CFG_LTR,       // Long term reference
        /* below RQCT_CFG type MUST one by one match rqct_conf_e in mdrv_rqct_st.h */
        RQCT_CFG_RES=32,
        RQCT_CFG_FPS,
        RQCT_CFG_ROI,
        RQCT_CFG_DQM,
        RQCT_CFG_RCM,
    } type;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_SEQ"
        enum rqct_mode
        {
            RQCT_MODE_CQP=0,
            RQCT_MODE_CBR,
            RQCT_MODE_VBR,
        } i_method;
        int i_period;
        int i_leadqp;
        int i_btrate;
    } seq;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_LTR"
        int i_period;
    } ltr;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_DQP"
        int i_dqp;
    } dqp;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_QPR"
        int i_iupperq;
        int i_ilowerq;
        int i_pupperq;
        int i_plowerq;
    } qpr;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_LOG"
        int b_logm;
    } log;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_PEN"
        short b_i16pln;
        short i_peni4x;
        short i_peni16;
        short i_penint;
        short i_penYpl;
        short i_penCpl;
    } pen;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_RCM"
        enum rqct_spfrm_mode
        {
            RQCT_SPFRM_NONE=0,      //!< super frame mode none.
            RQCT_SPFRM_DISCARD,     //!< super frame mode discard.
            RQCT_SPFRM_REENCODE,    //!< super frame mode reencode.
        } e_spfrm;
        int i_IfrmThr;
        int i_PfrmThr;
        int i_BfrmThr;
    } spf;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_RES"
        short i_picw;
        short i_pich;
    } res;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_FPS"
        short n_fps;
        short d_fps;
    } fps;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_ROI"
        short i_roiidx;
        short i_roidqp;
        short i_posx;
        short i_posy;
        short i_recw;
        short i_rech;
    } roi;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_DQM"
        unsigned int u_phys;
        void*        p_kptr;
        short i_dqmw;
        short i_dqmh;
        int   i_size;
        int   i_unit;
    } dqm;

    struct
    {
        enum rqct_cfg_e i_type; //!< MUST BE "RQCT_CFG_RCM"
        unsigned int u_phys;
        void*        p_kptr;
        int   i_size;
    } rcm;
};

struct  rqct_buf
{
    unsigned int u_config;
};

#endif//_RQCT_CFG_H_
