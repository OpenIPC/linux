static struct hidg_func_descriptor my_keybord_data = {
	.subclass = 1,				// Boot device
	.protocol = 1,				// Keyboard
	.report_length = 8,
	.report_desc_length = 63,
	.report_desc = {
		0x05, 0x01,			// USAGE_PAGE (Generic Desktop)
		0x09, 0x06,			// USAGE (Keyboard)
		0xa1, 0x01,			// COLLECTION (Application)
		0x05, 0x07,			//   USAGE_PAGE (Keyboard)
		0x19, 0xe0,			//   USAGE_MINIMUM (Keyboard LeftControl)
		0x29, 0xe7,			//   USAGE_MAXIMUM (Keyboard Right GUI)
		0x15, 0x00,			//   LOGICAL_MINIMUM (0)
		0x25, 0x01,			//   LOGICAL_MAXIMUM (1)
		0x75, 0x01,			//   REPORT_SIZE (1)
		0x95, 0x08,			//   REPORT_COUNT (8)
		0x81, 0x02,			//   INPUT (Data,Var,Abs)
		0x95, 0x01,			//   REPORT_COUNT (1)
		0x75, 0x08,			//   REPORT_SIZE (8)
		0x81, 0x03,			//   INPUT (Cnst,Var,Abs)
		0x95, 0x05,			//   REPORT_COUNT (5)
		0x75, 0x01,			//   REPORT_SIZE (1)
		0x05, 0x08,			//   USAGE_PAGE (LEDs)
		0x19, 0x01,			//   USAGE_MINIMUM (Num Lock)
		0x29, 0x05,			//   USAGE_MAXIMUM (Kana)
		0x91, 0x02,			//   OUTPUT (Data,Var,Abs)
		0x95, 0x01,			//   REPORT_COUNT (1)
		0x75, 0x03,			//   REPORT_SIZE (3)
		0x91, 0x03,			//   OUTPUT (Cnst,Var,Abs)
		0x95, 0x06,			//   REPORT_COUNT (6)
		0x75, 0x08,			//   REPORT_SIZE (8)
		0x15, 0x00,			//   LOGICAL_MINIMUM (0)
		0x25, 0x65,			//   LOGICAL_MAXIMUM (101)
		0x05, 0x07,			//   USAGE_PAGE (Keyboard)
		0x19, 0x00,			//   USAGE_MINIMUM (Reserved)
		0x29, 0x65,			//   USAGE_MAXIMUM (Keyboard Application)
		0x81, 0x00,			//   INPUT (Data,Ary,Abs)
		0xc0				// END_COLLECTION
	}
};

static struct platform_device my_keybord_hid = {
	.name = "hidg",
	.id = 0,
	.num_resources = 0,
	.resource = 0,
	.dev = {
		.platform_data  = &my_keybord_data,
	}, 
};

static struct hidg_func_descriptor my_mouse_data = {
	.subclass = 1,				// Boot device
	.protocol = 2,				// Mouse
	.report_length = 3,
	.report_desc_length= 50,
	.report_desc= {
		0x05, 0x01,			// USAGE_PAGE (Generic Desktop)
		0x09, 0x02,			// USAGE (Mouse)
		0xa1, 0x01,			// COLLECTION (Application)
		0x09, 0x01,			//   USAGE (Pointer)
		0xa1, 0x00,			//   COLLECTION (Physical)
		0x05, 0x09,			//     USAGE_PAGE (Button)
		0x19, 0x01,			//     USAGE_MINIMUM (Button 1)
		0x29, 0x03,			//     USAGE_MAXIMUM (Button 3)
		0x15, 0x00,			//     LOGICAL_MINIMUM (0)
		0x25, 0x01,			//     LOGICAL_MAXIMUM (1)
		0x95, 0x03,			//     REPORT_COUNT (3)
		0x75, 0x01,			//     REPORT_SIZE (1)
		0x81, 0x02,			//     INPUT (Data,Var,Abs)
		0x95, 0x01,			//     REPORT_COUNT (1)
		0x75, 0x05,			//     REPORT_SIZE (5)
		0x81, 0x03,			//     INPUT (Cnst,Var,Abs)
		0x05, 0x01,			//     USAGE_PAGE (Generic Desktop)
		0x09, 0x30,			//     USAGE (X)
		0x09, 0x31,			//     USAGE (Y)
		0x15, 0x81,			//     LOGICAL_MINIMUM (-127)
		0x25, 0x7f,			//     LOGICAL_MAXIMUM (127)
		0x75, 0x08,			//     REPORT_SIZE (8)
		0x95, 0x02,			//     REPORT_COUNT (2)
		0x81, 0x06,			//     INPUT (Data,Var,Rel)
		0xc0,				//   END_COLLECTION
		0xc0				// END_COLLECTION
	}
};

static struct platform_device my_mouse_hid = {
	.name = "hidg",
	.id = 1,
	.num_resources = 0,
	.resource = 0,
	.dev = {
		.platform_data = &my_mouse_data,
	},
};
