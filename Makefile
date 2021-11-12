########################################################################
#
#  Use this Makefile to build the CGEN for HiTech C v3.09 under Linux
#  using John Elliott's zxcc emulator.
#
########################################################################

VERSION = 3.0

CSRCS =	optim1.c \
	part21.c \
	part31.c \
	ctype1.c \
	initvar1.c


COBJS = $(CSRCS:.c=.obj)


OBJS = $(COBJS)

all:	$(COBJS) optim.com 

.SUFFIXES:		# delete the default suffixes
.SUFFIXES: .c .obj

$(COBJS): %.obj: %.c
	zxc -c -o  $<

#$(AOBJS): %.obj: %.asm
#	zxas -n $<
#	zxas -j -n $<

optim.com: $(OBJS)
	zxcc link -"<" +lkoptim
	sort optim1.sym | uniq > optim1.sym.sorted

clean:
	rm -f $(OBJS) optim1.com *.\$$\$$\$$ optim1.map optim1.sym optim1.sym.sorted

