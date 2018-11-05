################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tool/buf_factory.c \
../tool/crc16.c \
../tool/epcs.c \
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
	arm-linux-gnueabihf-gcc -mcpu=cortex-a15 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I/home/work/rootfs/usr/include -I/home/work/rootfs/usr/include/glib-2.0 -I/home/work/rootfs/usr/lib/glib-2.0/include -I/home/work/rootfs/usr/include/dce -I/home/work/rootfs/usr/include/libdrm -I/home/work/rootfs/usr/include/omap -I"/home/work/.workspace/smart_camera/app" -I"/home/work/.workspace/smart_camera/lvds" -I"/home/work/.workspace/smart_camera/image" -I"/home/work/.workspace/smart_camera/fpga" -I"/home/work/.workspace/smart_camera/net" -I"/home/work/.workspace/smart_camera/tool" -I"/home/work/.workspace/smart_camera/reject" -I"/home/work/.workspace/smart_camera/interface" -std=gnu11 -DBUILD_DATE="\"`date '+%Y%m%d%H%M_1.1.1_alpha'`"\" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


