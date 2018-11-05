################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../image/dsp.c \
../image/dsp_context.c \
../image/sample_count.c \
../image/sample_factory.c \
../image/shared_vid_buf.c \
../image/video.c \
../image/video_context.c 

OBJS += \
./image/dsp.o \
./image/dsp_context.o \
./image/sample_count.o \
./image/sample_factory.o \
./image/shared_vid_buf.o \
./image/video.o \
./image/video_context.o 

C_DEPS += \
./image/dsp.d \
./image/dsp_context.d \
./image/sample_count.d \
./image/sample_factory.d \
./image/shared_vid_buf.d \
./image/video.d \
./image/video_context.d 


# Each subdirectory must supply rules for building sources it contributes
image/%.o: ../image/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-linux-gnueabihf-gcc -mcpu=cortex-a15 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I/home/work/rootfs/usr/include -I/home/work/rootfs/usr/include/glib-2.0 -I/home/work/rootfs/usr/lib/glib-2.0/include -I/home/work/rootfs/usr/include/dce -I/home/work/rootfs/usr/include/libdrm -I/home/work/rootfs/usr/include/omap -I"/home/work/.workspace/smart_camera/app" -I"/home/work/.workspace/smart_camera/lvds" -I"/home/work/.workspace/smart_camera/image" -I"/home/work/.workspace/smart_camera/fpga" -I"/home/work/.workspace/smart_camera/net" -I"/home/work/.workspace/smart_camera/tool" -I"/home/work/.workspace/smart_camera/reject" -I"/home/work/.workspace/smart_camera/interface" -std=gnu11 -DBUILD_DATE="\"`date '+%Y%m%d%H%M_1.1.1_alpha'`"\" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


