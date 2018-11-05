################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../interface/led.c \
../interface/sys_log.c \
../interface/udisk.c \
../interface/ui_protocol.c \
../interface/ui_socket.c \
../interface/upstate.c 

OBJS += \
./interface/led.o \
./interface/sys_log.o \
./interface/udisk.o \
./interface/ui_protocol.o \
./interface/ui_socket.o \
./interface/upstate.o 

C_DEPS += \
./interface/led.d \
./interface/sys_log.d \
./interface/udisk.d \
./interface/ui_protocol.d \
./interface/ui_socket.d \
./interface/upstate.d 


# Each subdirectory must supply rules for building sources it contributes
interface/%.o: ../interface/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-linux-gnueabihf-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


