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
	arm-linux-gnueabihf-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


