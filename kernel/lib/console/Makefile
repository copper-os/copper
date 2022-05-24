SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)


libconsole.a: $(OBJS)
	$(AR) --thin -vcrs libconsole.a $(OBJS) $(GNU_EFI_CRT)

%.d: %.c
	$(CC) $(CFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

.PHONY: clean
clean:
	@rm -f libconsole.a $(OBJS) $(DEPS)

include $(DEPS)
