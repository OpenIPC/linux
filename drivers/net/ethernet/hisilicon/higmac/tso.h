#ifndef __HIETH_TSO_H
#define __HIETH_TSO_H

#define SG_FLAG		BIT(30)
#define COE_FLAG	BIT(29)
#define TSO_FLAG	BIT(28)
#define VLAN_FLAG	BIT(10)
#define IPV6_FLAG	BIT(9)
#define UDP_FLAG	BIT(8)

#define PKT_IPV6_HDR_LEN	10
#define PKT_UDP_HDR_LEN		2
#define WORD_TO_BYTE		4
enum {
	PKT_NORMAL,
	PKT_SG
};

enum {
	PKT_IPV4,
	PKT_IPV6
};

enum {
	PKT_TCP,
	PKT_UDP
};

struct frags_info {
	/* Word(2*i+2) */
	u32 addr;
	/* Word(2*i+3) */
	u32 size:16;
	u32 reserved:16;
};

struct sg_desc {
	/* Word0 */
	u32 total_len:17;
	u32 reserv:15;
	/* Word1 */
	u32 ipv6_id;
	/* Word2 */
	u32 linear_addr;
	/* Word3 */
	u32 linear_len:16;
	u32 reserv3:16;
	/* MAX_SKB_FRAGS = 17 */
	struct frags_info frags[18];
	/* struct frags_info frags[MAX_SKB_FRAGS]; */
};

#endif
