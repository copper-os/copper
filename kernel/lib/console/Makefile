LIB_NAME := console

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)


lib$(LIB_NAME).a: $(OBJS)
	$(AR) --thin -vcrs $@ $(OBJS)

.PHONY: clean
clean: clean-deps-graph
	@rm -f lib$(LIB_NAME).a $(OBJS)

include $(MAKE_DIR)/deps-graph.mk
