# Makefile
CC	= gcc
CFLAGS  = -Wall -std=gnu99 -s -O2
LDFLAGS = -lm
SRCS	= core/mem.c core/color.c core/trx.c core/pred.c core/mbcore.c core/mbdc.c \
	  core/mblp.c core/mbhp.c core/mb.c core/header.c core/enc.c core/api.c
OBJS    = $(SRCS:%.c=%.o)


all: default

default: libopenjxr.a

libopenjxr.a: $(OBJS)
	$(AR) rc libopenjxr.a $(OBJS)
#	$(RANLIB) libopenjxr.a

.o.c:
	$(CC) $(CFLAGS) $<
#.depend:
#	@$(foreach SRC, $(SRCS), $(CC) $(CFLAGS) $(SRC) $(SRC:%.c=%.o))

app: libopenjxr.a
	$(CC) app/exthdr/main.c -llibopenjxr -o exthdr
	$(CC) app/jxrdec/main.c -llibopenjxr -o jxrdec
#	$(CC) app/jxrenc/main.c -llibopenjxr -o jxrenc

clean:
	-rm -f $(TARGET) $(OBJS) libopenjxr.a

.PHONY: all default clean
