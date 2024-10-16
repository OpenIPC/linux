obj-$(CONFIG_LIBM) += libm.o
libm-y = \
	builtins.o \
	s_scalbn.o \
	s_scalbnf.o \
	e_sqrt.o \
	e_sqrtf.o \
	e_pow.o \
	e_powf.o \
	e_exp.o \
	e_expf.o \
	s_floor.o \
	s_floorf.o \
	k_rem_pio2.o \
	e_rem_pio2.o \
	k_cos.o \
	k_sin.o \
	s_sin.o \
	s_cos.o \
	e_rem_pio2f.o \
	k_cosf.o \
	k_sinf.o \
	s_sinf.o \
	s_cosf.o \
	s_atan.o \
	s_atanf.o \
	e_atan2.o \
	e_atan2f.o \
	s_sincos.o \
	e_log.o \
	e_logf.o \
	e_log2.o \
	e_log10.o
#	e_log2f.o
#	e_log10f.o
#	libgcc.a

ifneq (x$(CONFIG_VIDEO_RT_MEDIA),xy)
libm-y += libgcc
endif
