SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)


libconsole.a: $(OBJS)
	$(AR) --thin -vcrs libconsole.a $(OBJS) $(GNU_EFI_CRT)

.PHONY: clean
clean: clean-deps-graph
	@rm -f libconsole.a $(OBJS)

include $(MAKE_DIR)/deps-graph.mk
