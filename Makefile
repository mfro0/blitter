CROSS=Y

CROSSBINDIR_IS_Y=m68k-atari-mintelf-
CROSSBINDIR_IS_N=

CROSSBINDIR=$(CROSSBINDIR_IS_$(CROSS))

UNAME := $(shell uname)
ifeq ($(CROSS), Y)
ifeq ($(UNAME),Linux)
PREFIX=m68k-atari-mint
HATARI=hatari
else
PREFIX=m68k-atari-mint
HATARI=/usr/bin/hatari
endif
else
PREFIX=/usr
endif

DEPEND=depend

INCLUDE=-I../libcmini/include -I/usr/m68k-atari-mintelf/include -nostdlib
LIBS=-lgem -lm -lcmini -nostdlib -lgcc
CC=$(PREFIX)/bin/gcc

CC=$(CROSSBINDIR)gcc
STRIP=$(CROSSBINDIR)strip
STACK=$(CROSSBINDIR)stack
NATIVECC=gcc

APP=blitter.app
TEST_APP=$(APP)

CHARSET_FLAGS= -finput-charset=ISO-8859-1 \
               -fexec-charset=ATARIST

CFLAGS= \
	-fomit-frame-pointer \
	-O2 \
	-g \
	-Wall \
	-Wa,--register-prefix-optional \
	$(CHARSET_FLAGS)


SRCDIR=sources
INCDIR=include
INCLUDE+=-I$(INCDIR)

CSRCS=\
	$(SRCDIR)/blitter.c \
	$(SRCDIR)/pattern.c \
	\
	$(SRCDIR)/natfeats.c

ASRCS= \
	$(SRCDIR)/nf_asm.S

ifeq (.,$(TRGTDIR))
ASRCS += blitter_asm.S
endif



COBJS=$(patsubst $(SRCDIR)/%.o,%.o,$(patsubst %.c,%.o,$(CSRCS)))
AOBJS=$(patsubst $(SRCDIR)/%.o,%.o,$(patsubst %.S,%.o,$(ASRCS)))
OBJS=$(COBJS) $(AOBJS)

TRGTDIRS=. ./m68020-60 ./m5475 ./mshort ./m68020-60/mshort ./m5475/mshort
OBJDIRS=$(patsubst %,%/objs,$(TRGTDIRS))

#
# multilib flags. These must match m68k-atari-mint-gcc -print-multi-lib output
#
./$(APP): CFLAGS += -msoft-float
m68020-60/$(APP):CFLAGS += -m68020-60
m5475/$(APP):CFLAGS += -mcpu=5475
mshort/$(APP):CFLAGS += -mshort
m68020-60/mshort/$(APP): CFLAGS += -mcpu=68030 -mshort
m5475/mshort/$(APP): CFLAGS += -mcpu=5475 -mshort

all: $(patsubst %,%/$(APP),$(TRGTDIRS)) $(DEPEND)

$(DEPEND): $(ASRCS) $(CSRCS)
	-rm -f $(DEPEND)
	for d in $(TRGTDIRS);\
		do $(CC) $(CFLAGS) $(INCLUDE) -M $(ASRCS) $(CSRCS) | sed -e "s#^\(.*\).o:#$$d/objs/\1.o:#" >> $(DEPEND); \
	done

#
# generate pattern rules for multilib object files.
#
define CC_TEMPLATE
$(1)/objs/%.o:$(SRCDIR)/%.c
	$(CC) $$(CFLAGS) $(INCLUDE) -c $$< -o $$@

$(1)/objs/%.o:$(SRCDIR)/%.S
	$(CC) $$(CFLAGS) $(INCLUDE) -c $$< -o $$@

$(1)_OBJS=$(patsubst %,$(1)/objs/%,$(OBJS))
$(1)/$(APP): $$($(1)_OBJS)
	$(CC) $$(CFLAGS) -o $$@  -Wl,-Map,$(1)/mapfile \
../libcmini/build/$(1)/objs/crt0.o $$($(1)_OBJS) -L../libcmini/build/$(1) $(LIBS)
	#$(STRIP) $$@
endef
$(foreach DIR,$(TRGTDIRS),$(eval $(call CC_TEMPLATE,$(DIR))))

clean:
	@rm -f $(patsubst %,%/objs/*.o,$(TRGTDIRS)) $(patsubst %,%/$(APP),$(TRGTDIRS))
	@rm -f $(DEPEND) mapfile

.PHONY: printvars
printvars:
	@$(foreach V,$(.VARIABLES), $(if $(filter-out environment% default automatic, $(origin $V)),$(warning $V=$($V))))

.phony: $(DEPEND)

ifneq (clean,$(MAKECMDGOALS))
-include $(DEPEND)
endif

test: $(TEST_APP)
	$(HATARI) --grab -w --tos /usr/share/hatari/etos512k.img \
	--machine falcon -s 14 --cpuclock 32 --cpulevel 3 \
	-d . $(APP)

ftest: $(TEST_APP)
	$(HATARI) --grab -w --tos /usr/share/hatari/tos404.img \
	--machine falcon --cpuclock 32 --cpulevel 3 \
	-d . $(APP)

sttest: $(TEST_APP)
	$(HATARI) --grab -w --tos "/usr/share/hatari/tos106de.img" \
	--machine st --cpuclock 32 --cpulevel 3  --vdi true --vdi-planes 4 \
	--vdi-width 640 --vdi-height 480 \
	-d . $(APP)

