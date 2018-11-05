################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ui/sys_log.c \
../ui/udisk.c \
../ui/upstate.c 

OBJS += \
./ui/sys_log.o \
./ui/udisk.o \
./ui/upstate.o 

C_DEPS += \
./ui/sys_log.d \
./ui/udisk.d \
./ui/upstate.d 


# Each subdirectory must supply rules for building sources it contributes
ui/%.o: ../ui/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-linux-gnueabihf-gcc -mcpu=cortex-a15 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I/home/work/rootfs/usr/include -I/home/work/rootfs/usr/include/glib-2.0 -I/home/work/rootfs/usr/lib/glib-2.0/include -I/home/work/rootfs/usr/include/dce -I/home/work/rootfs/usr/include/libdrm -I/home/work/rootfs/usr/include/omap -I"/home/work/.workspace/smart_camera/app" -I"/home/work/.workspace/smart_camera/lvds" -I"/home/work/.workspace/smart_camera/image" -I"/home/work/.workspace/smart_camera/fpga" -I"/home/work/.workspace/smart_camera/net" -I"/home/work/.workspace/smart_camera/tool" -I"/home/work/.workspace/smart_camera/reject" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


