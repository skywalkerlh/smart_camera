################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include tool/subdir.mk
-include reject/subdir.mk
-include net/subdir.mk
-include lvds/subdir.mk
-include interface/subdir.mk
-include image/subdir.mk
-include fpga/subdir.mk
-include app/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
smart_camera.hex \

SECONDARY_SIZE += \
smart_camera.siz \


# All Target
all: smart_camera.elf secondary-outputs

# Tool invocations
smart_camera.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	arm-linux-gnueabihf-gcc -mcpu=cortex-m3 -mthumb -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -Xlinker --gc-sections -Wl,-Map,"smart_camera.map" -o "smart_camera.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

smart_camera.hex: smart_camera.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-linux-gnueabihf-objcopy -O ihex "smart_camera.elf"  "smart_camera.hex"
	@echo 'Finished building: $@'
	@echo ' '

smart_camera.siz: smart_camera.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-linux-gnueabihf-size --format=berkeley "smart_camera.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(SECONDARY_FLASH)$(S_UPPER_DEPS) smart_camera.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
