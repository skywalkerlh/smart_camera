################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tool/buf_factory.c \
../tool/crc16.c \
../tool/epcs.c \
../tool/jpeg.c \
../tool/key_file.c \
../tool/list.c \
../tool/mailbox.c \
../tool/message.c \
../tool/msg_factory.c \
../tool/shared_mem_manager.c \
../tool/thread.c \
../tool/v4l2.c 

OBJS += \
./tool/buf_factory.o \
./tool/crc16.o \
./tool/epcs.o \
./tool/jpeg.o \
./tool/key_file.o \
./tool/list.o \
./tool/mailbox.o \
./tool/message.o \
./tool/msg_factory.o \
./tool/shared_mem_manager.o \
./tool/thread.o \
./tool/v4l2.o 

C_DEPS += \
./tool/buf_factory.d \
./tool/crc16.d \
./tool/epcs.d \
./tool/jpeg.d \
./tool/key_file.d \
./tool/list.d \
./tool/mailbox.d \
./tool/message.d \
./tool/msg_factory.d \
./tool/shared_mem_manager.d \
./tool/thread.d \
./tool/v4l2.d 


# Each subdirectory must supply rules for building sources it contributes
tool/%.o: ../tool/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-linux-gnueabihf-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


