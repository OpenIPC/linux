/*
    Module Name:
    util.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Steven Liu  2012-10-19      Initial version
*/

#ifndef _MCAST_TBL_WANTED
#define _MCAST_TBL_WANTED

//#define MCAST_DEBUG
#ifdef MCAST_DEBUG
#define MCAST_PRINT(fmt, args...) printk(KERN_INFO fmt, ## args)
#else
#define MCAST_PRINT(fmt, args...) { }
#endif



/*
 * EXPORT FUNCTION
 */
int MDrv_HWNAT_Mcast_Insert_Entry(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_px_en, uint8_t mc_px_qos_en, uint8_t mc_qos_qid);
int MDrv_HWNAT_Mcast_Update_Qid(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_qos_qid);
int MDrv_HWNAT_Mcast_Delete_Entry(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_px_en, uint8_t mc_px_qos_en, uint8_t mc_qos_qid);
void MDrv_HWNAT_Mcast_Dump(void);
void MDrv_HWNAT_Mcast_Delete_All(void);



#endif
