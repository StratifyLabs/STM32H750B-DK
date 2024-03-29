cmake_minimum_required (VERSION 3.12)

if(NOT SOS_SDK_PATH)
	set(SOS_SDK_PATH $ENV{SOS_SDK_PATH})
endif()

set(CMAKE_MODULE_PATH ${SOS_SDK_PATH}/cmake)
set(CMAKE_TOOLCHAIN_FILE ${SOS_SDK_PATH}/cmake/sos-toolchain.cmake)
include(sos-sdk)

# change to the ARCH of your chip like v7m for Cortex M3
set(SOS_ARCH "v7em_f5dh")
# set to an empty string
set(SOS_ARCH_LIST "")
set(MCU stm32h750xx)



include(compiler_rt)
include(newlib)
include(StratifyOS)

include(StratifyOS_${MCU}_debug_${SOS_ARCH})
include(StratifyOS_${MCU}_release_${SOS_ARCH})

# include other cmake modules as needed

project(STM32H750B-DK)


#Stratify Labs assigned hardware ID
set(HARDWARE_ID 0x00000027)
set(BOOT_START_ADDRESS 0x08000000)
set(KERNEL_START_ADDRESS 0x24000000)
set(KERNEL_LINKER_FILE ldscripts/stm32h750xbram-rom.ld)
set(BOOT_LINKER_FILE ldscripts/stm32h750xb-rom.ld)

set(DEFINITIONS
	__BOOT_START_ADDRESS=${BOOT_START_ADDRESS}
	__KERNEL_START_ADDRESS=${KERNEL_START_ADDRESS}
	__stm32h735xx
  MCU_ARCH_STM32
	)


set(KERNEL_LIBRARIES
	StratifyOS_sys
	StratifyOS_${MCU}
	StratifyOS_usbd
	StratifyOS_sys
	newlib_libc
	newlib_liba_vsprintf
	newlib_liba_vprintf
	newlib_liba_vsiprintf
	newlib_liba_viprintf
	newlib_libc
	newlib_libm
	StratifyOS_sys
	StratifyOS_${MCU}
	compiler_rt)

sos_sdk_add_subdirectory(KERNEL_SOURCELIST ${CMAKE_CURRENT_SOURCE_DIR}/src)

sos_sdk_bsp_target(RELEASE ${PROJECT_NAME} "" release ${SOS_ARCH})
sos_sdk_bsp_target(DEBUG ${PROJECT_NAME} "" debug ${SOS_ARCH})

add_executable(${RELEASE_TARGET})

target_sources(${RELEASE_TARGET}
	PUBLIC
	${KERNEL_SOURCELIST}
	${CMAKE_CURRENT_SOURCE_DIR}/sl_settings.json
	)
target_include_directories(${RELEASE_TARGET}
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
	)
target_compile_definitions(${RELEASE_TARGET}
	PUBLIC
	${DEFINITIONS}
	VECTOR_TABLE_ADDRESS=${KERNEL_START_ADDRESS}
	)
target_compile_options(${RELEASE_TARGET} PUBLIC -Os)
set_target_properties(${RELEASE_TARGET}
	PROPERTIES
	LINK_FLAGS
	"-u symbols_table -T${KERNEL_LINKER_FILE}"
	)

add_executable(${DEBUG_TARGET})
sos_sdk_copy_target(${RELEASE_TARGET} ${DEBUG_TARGET})

sos_sdk_bsp_target(BOOT_RELEASE ${PROJECT_NAME} boot release ${SOS_ARCH})
sos_sdk_bsp_target(BOOT_DEBUG ${PROJECT_NAME} boot debug ${SOS_ARCH})

add_executable(${BOOT_RELEASE_TARGET})

target_sources(${BOOT_RELEASE_TARGET}
	PUBLIC
	${KERNEL_SOURCELIST}
	)
target_include_directories(${BOOT_RELEASE_TARGET}
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
	)
target_compile_definitions(${BOOT_RELEASE_TARGET}
	PUBLIC
	${DEFINITIONS}
	VECTOR_TABLE_ADDRESS=${BOOT_START_ADDRESS}
	_IS_BOOT=1
	)
target_compile_options(${BOOT_RELEASE_TARGET} PUBLIC -Os)
set_target_properties(${BOOT_RELEASE_TARGET}
	PROPERTIES
	LINK_FLAGS
	"-u _main -T${BOOT_LINKER_FILE} "
	)


add_executable(${BOOT_DEBUG_TARGET})
sos_sdk_copy_target(${BOOT_RELEASE_TARGET} ${BOOT_DEBUG_TARGET})

sos_sdk_bsp("${BOOT_RELEASE_OPTIONS}" ${HARDWARE_ID} ${BOOT_START_ADDRESS} "${KERNEL_LIBRARIES}")
sos_sdk_bsp("${BOOT_DEBUG_OPTIONS}" ${HARDWARE_ID} ${BOOT_START_ADDRESS} "${KERNEL_LIBRARIES}")

sos_sdk_bsp("${RELEASE_OPTIONS}" ${HARDWARE_ID} ${KERNEL_START_ADDRESS} "${KERNEL_LIBRARIES}")
sos_sdk_bsp("${DEBUG_OPTIONS}" ${HARDWARE_ID} ${KERNEL_START_ADDRESS} "${KERNEL_LIBRARIES}")


