SRCS := tester.c

OBJS := tester.o

CC := gcc
LDPATH =  -I/usr/include/libxml2
LDPATH += -I/home/anshul/workspace/HardwareManager/src

LDFLAGS := -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"
LIBS := -lrt -lpthread -lxml2

DEF := -DI_WANT_TO_DEBUG

RM := rm -rf

all: tester

tester: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'	
	gcc  -o "tester" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

%.o: %.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc $(DEF) $(LDPATH) $(LDFLAGS) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '	

clean:
	-$(RM) $(OBJS) tester
	-@echo ' '	