export ARCH := x86_64
export AR   := ar
export AS   := nasm
export CXX  := clang++

SOURCE_DIR  := src
LIBRARY_DIR := lib

export HEADER_DIR  := $(CURDIR)/include

LIBRARIES = console

export CXXFLAGS = -I$(HEADER_DIR) \
                -Wall \
                -Wextra \
                -fpie \
                -ffreestanding \
                -fshort-wchar \
                -fno-exceptions \
                -fno-rtti \
                -std=c++17

LDFLAGS = $(addprefix -L$(LIBRARY_DIR)/,$(LIBRARIES)) \
          -Bstatic \
          -Bsymbolic \
          -pie \
          -nostdlib \
          --gc-sections \
          --no-undefined-version

SRCS = $(wildcard $(SOURCE_DIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

ASM_SRCS = $(wildcard $(SOURCE_DIR)/*.S)
ASM_OBJS = $(ASM_SRCS:.S=.o)

LINKER_SCRIPT = kernel.lds


all: kernel.elf

kernel.elf: $(LINKER_SCRIPT) $(OBJS) $(ASM_OBJS) $(LIBRARIES)
	$(LD) -T$(LINKER_SCRIPT) $(LDFLAGS) -o $@ $(OBJS) $(ASM_OBJS) $(addprefix -l,$(LIBRARIES))

$(ASM_OBJS): $(ASM_SRCS)
	$(AS) $(ASMFLAGS) -o $@ -f elf64 $^

.PHONY: $(LIBRARIES)
$(LIBRARIES):
	$(MAKE) -C $(addprefix $(LIBRARY_DIR)/,$@)

.PHONY: clean
clean: clean-deps-graph
	@rm -f kernel.elf $(OBJS) $(ASM_OBJS)
	@for libdir in $(addprefix $(LIBRARY_DIR)/,$(LIBRARIES)); do \
		$(MAKE) -C $$libdir clean; \
	done

include $(MAKE_DIR)/deps-graph.mk
