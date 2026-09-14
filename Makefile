# ------------------------------------------------------------------
#  lsf_taskmgr - LSF cluster monitor (X11 GUI)
# ------------------------------------------------------------------
CC       ?= gcc
CFLAGS   ?= -O2 -Wall -Wextra -std=c99 -Werror=implicit-function-declaration
CFLAGS   += -D_GNU_SOURCE
CPPFLAGS += -Iinclude
LDLIBS   += -lX11

TARGET   := lsf_taskmgr
SRCDIR   := src
OBJDIR   := build

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run rebuild

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR):
	@mkdir -p $@

run: $(TARGET)
	./$(TARGET)

rebuild: clean all

clean:
	@rm -rf $(OBJDIR) $(TARGET)

-include $(DEPS)
