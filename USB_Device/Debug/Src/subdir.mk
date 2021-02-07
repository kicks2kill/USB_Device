################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/main.c \
../Src/systeminit.c 

OBJS += \
./Src/main.o \
./Src/systeminit.o 

C_DEPS += \
./Src/main.d \
./Src/systeminit.d 


# Each subdirectory must supply rules for building sources it contributes
Src/main.o: ../Src/main.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DSTM32F429I_DISC1 -DSTM32 -DSTM32F429ZITx -DSTM32F4 -DDEBUG -DSTM32F479xx -c -I../Inc -I"/home/x_orin1/STM32CubeIDE/workspace_1.5.1/USB_Device/Inc/CMSIS/Device/ST/STM32F4xx/Include" -I"/home/x_orin1/STM32CubeIDE/workspace_1.5.1/USB_Device/Inc/Helpers" -I"/home/x_orin1/STM32CubeIDE/workspace_1.5.1/USB_Device/Inc/CMSIS/Include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/main.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Src/systeminit.o: ../Src/systeminit.c
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DSTM32F429I_DISC1 -DSTM32 -DSTM32F429ZITx -DSTM32F4 -DDEBUG -DSTM32F479xx -c -I../Inc -I"/home/x_orin1/STM32CubeIDE/workspace_1.5.1/USB_Device/Inc/CMSIS/Device/ST/STM32F4xx/Include" -I"/home/x_orin1/STM32CubeIDE/workspace_1.5.1/USB_Device/Inc/Helpers" -I"/home/x_orin1/STM32CubeIDE/workspace_1.5.1/USB_Device/Inc/CMSIS/Include" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Src/systeminit.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

