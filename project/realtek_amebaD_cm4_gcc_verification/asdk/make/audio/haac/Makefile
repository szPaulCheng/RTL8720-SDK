
include $(MAKE_INCLUDE_GEN)

.PHONY: all clean

#*****************************************************************************#
#                               VARIABLES	                              #
#*****************************************************************************#
DIR = $(BASEDIR)/component/common/audio/haac
MODULE_IFLAGS = -I$(DIR)

vpath %.c $(DIR) $(shell find $(DIR) -type d)

#*****************************************************************************#
#                               Source FILE LIST                              #
#*****************************************************************************#
CSRC = $(DIR)/trigtabs_fltgen.c
CSRC += $(DIR)/trigtabs.c
CSRC += $(DIR)/tns.c
CSRC += $(DIR)/stproc.c
CSRC += $(DIR)/sbrtabs.c
CSRC += $(DIR)/sbrside.c
CSRC += $(DIR)/sbrqmf.c
CSRC += $(DIR)/sbrmath.c
CSRC += $(DIR)/sbrimdct.c
CSRC += $(DIR)/sbrhuff.c
CSRC += $(DIR)/sbrhfgen.c
CSRC += $(DIR)/sbrhfadj.c
CSRC += $(DIR)/sbrfreq.c
CSRC += $(DIR)/sbrfft.c
CSRC += $(DIR)/sbr.c
CSRC += $(DIR)/pns.c
CSRC += $(DIR)/noiseless.c
CSRC += $(DIR)/imdct.c
CSRC += $(DIR)/hufftabs.c
CSRC += $(DIR)/huffman.c
CSRC += $(DIR)/filefmt.c
CSRC += $(DIR)/fft.c
CSRC += $(DIR)/dequant.c
CSRC += $(DIR)/decelmnt.c
CSRC += $(DIR)/dct4.c
CSRC += $(DIR)/buffers.c
CSRC += $(DIR)/bitstream.c
CSRC += $(DIR)/aactabs.c
CSRC += $(DIR)/aacdec.c


 



#*****************************************************************************#
#                               Object FILE LIST                              #
#*****************************************************************************#
OBJS = $(notdir $(CSRC:.c=.o))

#*****************************************************************************#
#                        RULES TO GENERATE TARGETS                            #
#*****************************************************************************#

# Define the Rules to build the core targets
all: CLEAN_OBJS CORE_TARGETS COPY_RAM_OBJS

#*****************************************************************************#
#              GENERATE OBJECT FILE 
#*****************************************************************************#
CORE_TARGETS:	$(OBJS)


#*****************************************************************************#
#                        RULES TO CLEAN TARGETS                               #
#*****************************************************************************#
clean: CLEAN_OBJS
	$(REMOVE) *.o
	$(REMOVE) *.i
	$(REMOVE) *.s
	$(REMOVE) *.d

-include $(DEPS)