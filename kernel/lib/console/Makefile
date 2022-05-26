LIB_NAME := console

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)


lib$(LIB_NAME).a: $(OBJS)
	$(AR) --thin -vcrs $@ $(OBJS)

.PHONY: clean
clean: clean-deps-graph
	@rm -f lib$(LIB_NAME).a $(OBJS)

include $(MAKE_DIR)/deps-graph.mk
