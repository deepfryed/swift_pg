# ============= compiler settings ==============================================
MIX       = mix
CC       ?= gcc
OS       ?= $(shell uname -s)

ERL_ARGS  = -s init stop -noshell
ERL_EVAL  = 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version)])])'
ERTS_DIR := $(shell erl -eval $(ERL_EVAL) $(ERL_ARGS))
ERTS_INCL = $(ERTS_DIR)/include
ERTS_LIBS = -L$(ERTS_DIR)/lib -lerts

LPQ_INCL  = $(shell pg_config --includedir)

CFLAGS    = -std=c99 -g3 -Wall -Wextra -Wpedantic -Wno-unused-parameter
CFLAGS   += -Werror
CFLAGS   += -I$(ERTS_INCL) -I$(LPQ_INCL)

LPQ_LIBS  = -L$(shell pg_config --libdir) -lpq

ifeq ($(OS),Darwin)
  LDFLAGS = -fPIC -dynamiclib -undefined dynamic_lookup -flat_namespace
else
  LDFLAGS = -fPIC -shared
endif

LDFLAGS  += $(LPQ_LIBS) $(ERTS_LIBS)
# ==============================================================================

# ==============================================================================
# DEBUG malloc
#
# LDFLAGS += -Wl,--wrap,malloc -Wl,--wrap,realloc -Wl,--wrap,calloc -Wl,--wrap,free
# ==============================================================================

# ============= common library sources =========================================
LIB_SRCS  = $(wildcard src/pg/*.c)
LIB_CSRCS = $(LIB_SRCS:src/%=%)
LIB_OBJS  = $(addprefix obj/,$(LIB_CSRCS:.c=.o))
LIB_DLLS  = $(addprefix swift/,$(LIB_CSRCS:.c=.so))
# ==============================================================================

# ============= dependencies ===================================================
DEPENDS   = $(LIB_OBJS:.o=.d)
# ==============================================================================

all: $(LIB_DLLS)

-include $(DEPENDS)

swift/%.so: obj/%.o Makefile
	@mkdir -p swift/pg
	$(CC) $< $(LDFLAGS) -o $@

obj/%.o: src/%.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -MF $(@:%.o=%.d) -c -o $@ $<

clean:
	rm -rf obj/* swift/pg/*.so

# allows make print-<x> to print the value of variable x
print-%:
	@echo
	@echo $*:
	@echo $($*)
	@echo

.PHONY: all clean
.SECONDARY: $(LIB_OBJS) $(EXE_OBJS)
