################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../net/net.c \
../net/net_protocol.c \
../net/tcp.c \
../net/udp.c 

OBJS += \
./net/net.o \
./net/net_protocol.o \
./net/tcp.o \
./net/udp.o 

C_DEPS += \
./net/net.d \
./net/net_protocol.d \
./net/tcp.d \
./net/udp.d 


# Each subdirectory must supply rules for building sources it contributes
net/%.o: ../net/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-linux-gnueabihf-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


