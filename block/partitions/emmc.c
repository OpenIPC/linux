/*
 *  fs/partitions/emmc.c
 *
 *  Code extracted from drivers/block/genhd.c
 *  Copyright (C) 1991-1998  Linus Torvalds
 *  Re-organised Feb 1998 Russell King
 */

#include <linux/ctype.h>
#include "check.h"
#include "emmc.h"

//extern int get_NVRAM_start_sector(U32 *u32_startsector);
extern int get_NVRAM_max_part_count(void);
extern int get_DEVNODE_start_sector(unsigned int *u32_startsector);
//
///*
// * read Driver Descriptor Block
// */
//static int part_emmc_read_ddb (block_dev_desc_t *dev_desc, emmc_driver_desc_t *ddb_p)
//{
//	u32 u32_err;
//
//	if(0 == g_eMMCDrv.u32_PartDevNodeStartSector){
//		u32_err = eMMC_SearchDevNodeStartSector();
//		if(eMMC_ST_SUCCESS != u32_err){
//		    printf ("** Err, %s **\n", __func__);
//		    return (-1);
//		}
//	}
//	printf("read_ddb from: 0x%Xh\n", (unsigned int)g_eMMCDrv.u32_PartDevNodeStartSector);
//
//	u32_err = eMMC_ReadData((uchar*)ddb_p, 512, g_eMMCDrv.u32_PartDevNodeStartSector);
//	if(eMMC_ST_SUCCESS != u32_err){
//	    printf ("** Can't read Driver Desriptor Block **\n");
//	    return (-1);
//	}
//
//	if (ddb_p->signature != EMMC_DRIVER_MAGIC) {
//    	printf ("** Bad Signature: expected 0x%04x, got 0x%04x\n",
//			EMMC_DRIVER_MAGIC, ddb_p->signature);
//    	return (-1);
//	}
//
//	return (0);
//}
//
///*
// * read Partition Descriptor Block
// */
//static int part_emmc_read_pdb (block_dev_desc_t *dev_desc, int part_index, emmc_partition_t *pdb_p)
//{
//	u32 u32_err;
//	   int part=part_index+=1;
//
//    u32_err = eMMC_ReadData((uchar*)pdb_p, 512,
//		g_eMMCDrv.u32_PartDevNodeStartSector + part);
//
//	if(eMMC_ST_SUCCESS != u32_err){
//	    printf ("** Can't read Driver Desriptor Block **\n");
//	    return (-1);
//	}
//
////	if (pdb_p->signature != EMMC_PARTITION_MAGIC) {
////        //printf ("** Bad Signature: "
////        //        "expected 0x%04x, got 0x%04x\n",
////        //        EMMC_PARTITION_MAGIC, pdb_p->signature);
////        return (-1);
////    }
//
//	return 0;
//}

#define PART_START_BLK_OFFSET 1

#include "linux/mmc/host.h"
#include "linux/mmc/card.h"

struct mmc_card *mmc_blkdev_to_card(struct block_device *blkdev);
extern bool mstar_mci_is_mstar_host(struct mmc_card* card);
extern int get_NVRAM_max_part_count(void);
extern int get_DEVNODE_start_sector(unsigned int *u32_startsector);


bool __attribute__((weak))  mstar_mci_is_mstar_host(struct mmc_card* card)
{
    return false;
}

int __attribute__((weak)) get_NVRAM_max_part_count(void)
{
	return 0;
}

int __attribute__((weak)) get_DEVNODE_start_sector(unsigned int *u32_startsector)
{
    return 0;
}

int emmc_partition(struct parsed_partitions *state)
{
	int slot = 1;
	Sector sect;
	int blk, blocks_in_map;
	struct emmc_partition *part;
	struct emmc_driver_desc *md;
	struct mmc_card *card;

	unsigned int start_sector;
	sect.v = NULL;

	card=mmc_blkdev_to_card(state->bdev);

	if((!card) || (!mstar_mci_is_mstar_host(card)))
	{
		return 0;
	}

//	if(     NULL==(card)
//		||	NULL==(card->host)
//		||  NULL==(mci_host=((struct mstar_mci_host *)card->host->private) )
//		||  0!=strnmp(mci_host->name,MSTAR_MCI_NAME,strlen(MSTAR_MCI_NAME))
//		)
//	{
//		return 0;
//	}

	printk(KERN_INFO"[EMMC] checking EMMC partitions...\n");

    if(0!= get_DEVNODE_start_sector(&start_sector))
    {
    	printk(KERN_ERR"failed to get DEVNODE start sector!!\n");
    	//put_dev_sector(sect);
    	return -1;
    }



	/* Get 0th block and look at the first partition map entry. */
	md = read_part_sector(state, start_sector, &sect);
	if ((!md )|| md->signature != EMMC_DRIVER_MAGIC)
	{
		printk(KERN_ERR"failed to read part sector or is not emmc!!!\n");
		if(sect.v != NULL)
			put_dev_sector(sect);
		return 0;
	}
    
//	if () {
//            //can not found the partiton map!
//            //printk("error founding EMMC_DRIVER_MAGIC!! MAGIC0x%x\n",md->signature);
//            put_dev_sector(sect);
//            return 0;
//	}
//

	blocks_in_map=get_NVRAM_max_part_count();

//	part = (struct emmc_partition *)read_part_sector(state, start_sector+PART_START_BLK_OFFSET, &sect);
//	if ((!part)||part->signature != EMMC_PARTITION_MAGIC)
//	{
//		put_dev_sector(sect);
//		return 0;
//	}


	for (blk = PART_START_BLK_OFFSET; blk <= blocks_in_map; ++blk)
	{

		part = (struct emmc_partition *)read_part_sector(state, start_sector+(blk), &sect);
		if (!part)
		{		
    		printk(KERN_ERR"failed to read part sector!!!\n");
			if(sect.v != NULL)
				put_dev_sector(sect);
			return -1;
		}

		if (part->signature != EMMC_PARTITION_MAGIC)
		{
			continue;
		}


		put_partition(state, slot, (part->start_block),	(part->block_count));
		strcpy(state->parts[slot].info.uuid, part->name); /* put parsed partition name into state */
		printk(KERN_INFO"[EMMC] partition[%d]: start=0x%08X, count=0x%08X, name='%s'\n", slot, part->start_block, part->block_count, part->name);

		++slot;
	}
	
	if(sect.v != NULL)
		put_dev_sector(sect);
	printk("\n");
	return 1;
}
