################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hmcbs.c \
../src/hmcluster.c \
../src/hmconf.c \
../src/hmglobdb.c \
../src/hmlocmgmt.c \
../src/hmmain.c \
../src/hmmsg.c \
../src/hmnodemgmt.c \
../src/hmnotify.c \
../src/hmprocmgmt.c \
../src/hmtprt.c \
../src/hmutil.c \
../src/hmutil2.c 

OBJS += \
./src/hmcbs.o \
./src/hmcluster.o \
./src/hmconf.o \
./src/hmglobdb.o \
./src/hmlocmgmt.o \
./src/hmmain.o \
./src/hmmsg.o \
./src/hmnodemgmt.o \
./src/hmnotify.o \
./src/hmprocmgmt.o \
./src/hmtprt.o \
./src/hmutil.o \
./src/hmutil2.o 

C_DEPS += \
./src/hmcbs.d \
./src/hmcluster.d \
./src/hmconf.d \
./src/hmglobdb.d \
./src/hmlocmgmt.d \
./src/hmmain.d \
./src/hmmsg.d \
./src/hmnodemgmt.d \
./src/hmnotify.d \
./src/hmprocmgmt.d \
./src/hmtprt.d \
./src/hmutil.d \
./src/hmutil2.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DI_WANT_TO_DEBUG -DBIG_ENDIAN -I/usr/include/libxml2 -I/home/anshul/workspace/HardwareManager/src -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


