#ifndef MSB250X_MEMORY_H
#define MSB250X_MEMORY_H

#define GET_REG_ADDR(x, y)                  ((x) + ((y) << 2))
#define GET_BASE_ADDR_BY_BANK(x, y)         ((x) + ((y) << 1))

#define RIU_BASE                            0xFD200000
#define UTMI_BASE_ADDR                      GET_BASE_ADDR_BY_BANK(RIU_BASE, 0x42100)
#define USBC_BASE_ADDR                      GET_BASE_ADDR_BY_BANK(RIU_BASE, 0x42300)
#define OTG0_BASE_ADDR                      GET_BASE_ADDR_BY_BANK(RIU_BASE, 0x42500)

#define MIU0_BASE_ADDR                      0x20000000

#define MIU0_SIZE                           ((unsigned long) 0x10000000)

#define MIU0_BUS_BASE_ADDR                  ((unsigned long) 0x00000000)
#define MIU1_BUS_BASE_ADDR                  ((unsigned long) 0x80000000)

#define USB_MIU_SEL0                        ((u8) 0x70U)
#define USB_MIU_SEL1                        ((u8) 0xefU)
#define USB_MIU_SEL2                        ((u8) 0xefU)
#define USB_MIU_SEL3                        ((u8) 0xefU)


#define MSB250X_MAX_ENDPOINTS                   4
#define MSB250X_USB_DMA_CHANNEL                 3

#define ENABLE_OTG_USB_NEW_MIU_SLE              1

#define MSB250X_EPS_CAP(_dev, _ep_op) \
    .ep[0] = {                                                                      \
        .ep = {                                                                     \
            .name = ep0name,                                                        \
            .ops = _ep_op,                                                          \
            .caps = USB_EP_CAPS(USB_EP_CAPS_TYPE_CONTROL, USB_EP_CAPS_DIR_ALL),     \
            .maxpacket_limit = 64,                                                  \
        },                                                                          \
        .fifo_size = 64,                                                            \
        .dev = _dev,                                                                \
    },                                                                              \
    .ep[1] = {                                                                      \
        .ep = {                                                                     \
            .name = "ep1",                                                          \
            .ops = _ep_op,                                                          \
            .caps = USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL),         \
            .maxpacket_limit = 1024,                                                \
        },                                                                          \
        .fifo_size = 8192,                                                          \
        .dev = _dev,                                                                \
    },                                                                              \
    .ep[2] = {                                                                      \
        .ep = {                                                                     \
            .name = "ep2",                                                          \
            .ops = _ep_op,                                                          \
            .caps = USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL),         \
            .maxpacket_limit = 1024,                                                \
        },                                                                          \
        .fifo_size = 1024,                                                          \
        .dev = _dev,                                                                \
    },                                                                              \
    .ep[3] = {                                                                      \
        .ep = {                                                                     \
            .name = "ep3",                                                          \
            .ops = _ep_op,                                                          \
            .caps = USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL),         \
            .maxpacket_limit = 64,                                                  \
        },                                                                          \
        .fifo_size = 64,                                                            \
        .dev = _dev,                                                                \
    }

#define MSB250X_HIGH_BANDWIDTH_EP(_dev)     &((_dev)->ep[1].ep)

#define MSB250X_PA2BUS(a)   (MIU0_BUS_BASE_ADDR | (a - MIU0_BASE_ADDR))
#define MSB250X_BUS2PA(a)   ((a - MIU0_BUS_BASE_ADDR) + MIU0_BASE_ADDR)

#endif
