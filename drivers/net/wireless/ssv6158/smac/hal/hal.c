/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef ECLIPSE
#include <ssv_mod_conf.h>
#endif // ECLIPSE
#include <linux/version.h>
#include <linux/platform_device.h>

#include <linux/string.h>
#include <ssv_chip_id.h>
#include <ssv6200.h>
#include <smac/dev.h>
#include <hal.h>
#include <smac/ssv_skb.h>

static void ssv6xxx_cmd_rc(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_set_sifs(struct ssv_hw *sh, int band)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_hwinfo(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}
static int ssv6xxx_get_tkip_mmic_err(struct sk_buff *skb)
{
    return 0;
}

static void ssv6xxx_cmd_txgen(struct ssv_hw *sh, u8 drate)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_rf(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_rfble(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_efuse(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_hwq_limit(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_init_gpio_cfg(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_flash_read_all_map(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static u8 ssv6xxx_read_efuse(struct ssv_hw *sh, u8 *mac)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
    return 1;
}

static void ssv6xxx_update_rf_table(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

void ssv6xxx_set_on3_enable(struct ssv_hw *sh, bool val)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static int ssv6xxx_init_hci_rx_aggr(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
    return -1;
}

static int ssv6xxx_reset_hw_mac(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
    return -1;
}

static void ssv6xxx_set_crystal_clk(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_wait_usb_rom_ready(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_detach_usb_hci(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_pll_chk(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static bool ssv6xxx_put_mic_space_for_hw_ccmp_encrypt(struct ssv_softc *sc, struct sk_buff *skb)
{

    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
    return false;
}

#ifdef CONFIG_PM
static void ssv6xxx_save_clear_trap_reason(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);

}

static void ssv6xxx_restore_trap_reason(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);

}

static void ssv6xxx_ps_save_reset_rx_flow(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);

}

static void ssv6xxx_ps_restore_rx_flow(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_pmu_awake(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_ps_hold_on3(struct ssv_softc *sc, int value)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}
#endif

static int ssv6xxx_get_sram_mode(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
    return 0;
}

static void ssv6xxx_enable_sram(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_rc_mac80211_tx_rate_idx(struct ssv_softc *sc, int hw_rate_idx, struct ieee80211_tx_info *tx_info)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_update_mac80211_chan_info(struct ssv_softc *sc,
            struct sk_buff *skb, struct ieee80211_rx_status *rxs)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_read_allid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_read_txid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_read_rxid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_read_wifi_halt_status(struct ssv_hw *sh, u32 *status)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_attach_common_hal (struct ssv_hal_ops  *hal_ops)
{
    hal_ops->cmd_rc = ssv6xxx_cmd_rc;
    hal_ops->set_sifs = ssv6xxx_set_sifs;
	hal_ops->cmd_hwinfo = ssv6xxx_cmd_hwinfo;
	hal_ops->get_tkip_mmic_err = ssv6xxx_get_tkip_mmic_err;
	hal_ops->cmd_txgen = ssv6xxx_cmd_txgen;
	hal_ops->cmd_rf = ssv6xxx_cmd_rf;
	hal_ops->cmd_rfble = ssv6xxx_cmd_rfble;
	hal_ops->cmd_efuse = ssv6xxx_cmd_efuse;
	hal_ops->cmd_hwq_limit = ssv6xxx_cmd_hwq_limit;
    hal_ops->init_gpio_cfg = ssv6xxx_init_gpio_cfg;
    hal_ops->flash_read_all_map = ssv6xxx_flash_read_all_map;
    hal_ops->read_efuse = ssv6xxx_read_efuse;
    hal_ops->update_rf_table = ssv6xxx_update_rf_table;
    hal_ops->set_on3_enable = ssv6xxx_set_on3_enable;
    hal_ops->init_hci_rx_aggr = ssv6xxx_init_hci_rx_aggr;
    hal_ops->reset_hw_mac = ssv6xxx_reset_hw_mac;
    hal_ops->set_crystal_clk = ssv6xxx_set_crystal_clk;
    hal_ops->wait_usb_rom_ready = ssv6xxx_wait_usb_rom_ready;
    hal_ops->detach_usb_hci = ssv6xxx_detach_usb_hci;
    hal_ops->pll_chk = ssv6xxx_pll_chk;
    hal_ops->put_mic_space_for_hw_ccmp_encrypt = ssv6xxx_put_mic_space_for_hw_ccmp_encrypt;
#ifdef CONFIG_PM
	hal_ops->save_clear_trap_reason = ssv6xxx_save_clear_trap_reason;
	hal_ops->restore_trap_reason = ssv6xxx_restore_trap_reason;
	hal_ops->ps_save_reset_rx_flow = ssv6xxx_ps_save_reset_rx_flow;
	hal_ops->ps_restore_rx_flow = ssv6xxx_ps_restore_rx_flow;
	hal_ops->pmu_awake = ssv6xxx_pmu_awake;
	hal_ops->ps_hold_on3 = ssv6xxx_ps_hold_on3;
#endif
    hal_ops->get_sram_mode = ssv6xxx_get_sram_mode;
    hal_ops->enable_sram = ssv6xxx_enable_sram;
    hal_ops->rc_mac80211_tx_rate_idx = ssv6xxx_rc_mac80211_tx_rate_idx;
    hal_ops->update_mac80211_chan_info = ssv6xxx_update_mac80211_chan_info;
    hal_ops->read_allid_map = ssv6xxx_read_allid_map;
    hal_ops->read_txid_map = ssv6xxx_read_txid_map;
    hal_ops->read_rxid_map = ssv6xxx_read_rxid_map;
    hal_ops->read_wifi_halt_status = ssv6xxx_read_wifi_halt_status;
}


int ssv6xxx_init_hal(struct ssv_softc *sc)
{
    struct ssv_hw *sh;
    int ret = 0;
    struct ssv_hal_ops *hal_ops = NULL;
    extern void ssv_attach_ssv6006(struct ssv_softc *sc, struct ssv_hal_ops *hal_ops);
    extern void ssv_attach_ssv6020(struct ssv_softc *sc, struct ssv_hal_ops *hal_ops);
    bool chip_supportted = false;
    struct ssv6xxx_platform_data *priv = sc->dev->platform_data;

	// alloc hal_ops memory
	hal_ops = kzalloc(sizeof(struct ssv_hal_ops), GFP_KERNEL);
	if (hal_ops == NULL) {
		printk("%s(): Fail to alloc hal_ops\n", __FUNCTION__);
		return -ENOMEM;
	}

    // load common HAL layer function;
    ssv6xxx_attach_common_hal(hal_ops);

#ifdef SSV_SUPPORT_SSV6006
    if (   strstr(priv->chip_id, SSV6006)
		 || strstr(priv->chip_id, SSV6006C)
		 || strstr(priv->chip_id, SSV6006D)) {

        printk(KERN_INFO"%s\n", sc->platform_dev->id_entry->name);
        printk(KERN_INFO"Attach SSV6006 family HAL function  \n");

        ssv_attach_ssv6006(sc, hal_ops);
        chip_supportted = true;
    }
#endif
#ifdef SSV_SUPPORT_SSV6020
    if (strstr(priv->chip_id, SSV6020B)
        || strstr(priv->chip_id, SSV6020C)) {
        printk(KERN_INFO"%s\n", sc->platform_dev->id_entry->name);
        printk(KERN_INFO"Attach SSV6020 family HAL function  \n");
        ssv_attach_ssv6020(sc, hal_ops);
        chip_supportted = true;
    }
#endif
    if (!chip_supportted) {

        printk(KERN_ERR "Chip \"%s\" is not supported by this driver\n", sc->platform_dev->id_entry->name);
        ret = -EINVAL;
        goto out;
    }

    sh = hal_ops->alloc_hw();
    if (sh == NULL) {
        ret = -ENOMEM;
        goto out;
    }

    memcpy(&sh->hal_ops, hal_ops, sizeof(struct ssv_hal_ops));
    sc->sh = sh;
    sh->sc = sc;
    INIT_LIST_HEAD(&sh->hw_cfg);
    mutex_init(&sh->hw_cfg_mutex);
    sh->priv = sc->dev->platform_data;
    sh->hci.dev = sc->dev;
    sh->hci.if_ops = sh->priv->ops;
    sh->hci.skb_alloc = ssv_skb_alloc;
    sh->hci.skb_free = ssv_skb_free;
    sh->hci.hci_rx_cb = ssv6200_rx;
    sh->hci.hci_is_rx_q_full = ssv6200_is_rx_q_full;

    sh->priv->skb_alloc = ssv_skb_alloc_ex;
    sh->priv->skb_free = ssv_skb_free;
    sh->priv->skb_param = sc;

    // Set jump to rom functions for HWIF
    sh->priv->enable_usb_acc = ssv6xxx_enable_usb_acc;
    sh->priv->disable_usb_acc = ssv6xxx_disable_usb_acc;
    sh->priv->jump_to_rom = ssv6xxx_jump_to_rom;
    sh->priv->usb_param = sc;

    // Rx burstread size for HWIF
    sh->priv->rx_burstread_size = ssv6xxx_rx_burstread_size;
    sh->priv->rx_burstread_param = sc;

    sh->priv->rx_mode = ssv6xxx_rx_mode;
    sh->priv->rx_mode_param = sc;

    sh->hci.sc = sc;
    sh->hci.sh = sh;

out:
	kfree(hal_ops);
    return ret;
}
