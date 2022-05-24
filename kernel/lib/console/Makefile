SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

CFLAGS = -I$(HEADER_DIR) \
         -Wall \
         -Wextra \
         -fpie \
         -ffreestanding \
         -fshort-wchar

libconsole.a: $(OBJS)
	$(AR) -vcrs libconsole.a $(OBJS) $(GNU_EFI_CRT)

%.d: %.c
	$(CC) $(CFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

.PHONY: clean
clean:
	@rm -f libconsole.a $(OBJS) $(DEPS)

include $(DEPS)
