
struct ambarella_fb_color_table {
	enum ambarella_fb_color_format	color_format;
	int				bits_per_pixel;
	struct fb_bitfield		red;
	struct fb_bitfield		green;
	struct fb_bitfield		blue;
	struct fb_bitfield		transp;
};

static const struct ambarella_fb_color_table ambarella_fb_color_format_table[] =
{
	{
		.color_format = AMBFB_COLOR_CLUT_8BPP,
		.bits_per_pixel = 8,
		.red =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_RGB565,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 6,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_BGR565,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 6,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_AGBR4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_RGBA4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_BGRA4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_ABGR4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_ARGB4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_AGBR1555,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 10,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 5,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 15,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_GBR1555,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 10,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 5,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_RGBA5551,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 6,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 1,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_BGRA5551,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 1,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 6,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_ABGR1555,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 10,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 15,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_ARGB1555,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 10,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 15,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_AGBR8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_RGBA8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_BGRA8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_ABGR8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_ARGB8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_AYUV8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = AMBFB_COLOR_VYU565,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 6,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},
};

