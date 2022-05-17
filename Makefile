VPATH=game geoip linux qcommon server
KPBUILDPATH=obj/kp
Q2BUILDPATH=obj/q2

ALLSRC:=cmd.c cmodel.c common.c cvar.c files.c md4.c net_chan.c \
	     mersennetwister.c redblack.c sv_ccmds.c sv_ents.c sv_game.c \
	     sv_init.c sv_main.c sv_send.c sv_user.c sv_world.c q_shlinux.c \
	     sys_linux.c glob.c net_udp.c q_shared.c pmove.c \
	     maxminddb.c data-pool.c
Q2SRC:=crc.c ioapi.c unzip.c
KPOBJ:=$(addprefix $(KPBUILDPATH)/, $(ALLSRC:.c=.o))
Q2OBJ:=$(addprefix $(Q2BUILDPATH)/, $(ALLSRC:.c=.o) $(Q2SRC:.c=.o))

TARGETS:=kpded2 q2ded2

# need -fno-associative-math to avoid prediction misses, -fno-finite-math-only to avoid GLIBC_2.15 requirement
CFLAGS+=-DDEDICATED_ONLY -DNDEBUG -DLINUX -m32 -O2 -flto -ffast-math -fno-associative-math -fno-finite-math-only -MMD
kpded2: CFLAGS+=-MF $(KPBUILDPATH)/$*.d -DKINGPIN
q2ded2: CFLAGS+=-MF $(Q2BUILDPATH)/$*.d
%/maxminddb.o %/data-pool.o: CFLAGS+=-std=c99

LDFLAGS=-lm -lz -lpthread -ldl -lrt -m32 -Wl,--gc-sections -Wl,--no-undefined

.PHONY: clean

kpded2: $(KPOBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(KPBUILDPATH)/%.o : %.c
	@mkdir -p $(KPBUILDPATH)
	$(CC) $(CFLAGS) -c -o $@ $<

q2ded2: $(Q2OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(Q2BUILDPATH)/%.o : %.c
	@mkdir -p $(Q2BUILDPATH)
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(TARGETS)

clean:
	-rm -f $(TARGETS) $(KPBUILDPATH)/* $(Q2BUILDPATH)/*

-include $(KPBUILDPATH)/*.d $(Q2BUILDPATH)/*.d
