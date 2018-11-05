################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../reject/local_protocol.c \
../reject/local_socket.c 

OBJS += \
./reject/local_protocol.o \
./reject/local_socket.o 

C_DEPS += \
./reject/local_protocol.d \
./reject/local_socket.d 


# Each subdirectory must supply rules for building sources it contributes
reject/%.o: ../reject/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-linux-gnueabihf-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


