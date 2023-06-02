extern void hisi_usb_phy_on(struct phy *phy);
extern void hisi_usb_phy_off(struct phy *phy);
extern void hisi_usb3_phy_on(struct phy *phy);
extern void hisi_usb3_phy_off(struct phy *phy);

struct hisi_priv {
	void __iomem	*base;
	void __iomem	*peri_ctrl;
	void __iomem	*misc_ctrl;
	void __iomem	*switch_base;
};
