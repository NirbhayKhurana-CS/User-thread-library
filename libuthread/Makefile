target := libuthread.a
objs   := queue.o context.o uthread.o preempt.o

CFLAGS := -Wall -Werror
CC     := gcc

ifneq ($(V),1)
Q = @
endif

all: $(target)

deps := $(patsubst %.o,%.d,$(objs))# will be expanded into "main.d fact.d"
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

$(target): $(objs)
	@echo "AR	$@"
	$(Q)ar rcs $@ $^

%.o: %.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	@echo "CLEAN"
	$(Q)rm -f $(target) $(objs) $(deps)
