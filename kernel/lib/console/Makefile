SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

libconsole.a: $(OBJS)
	$(AR) -vcrs libconsole.a $(OBJS)

%.d: %.c
	$(CC) $(CFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

.PHONY: clean
clean:
	@rm -f libconsole.a $(OBJS) $(DEPS)

include $(DEPS)
