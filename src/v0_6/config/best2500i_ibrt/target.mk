CHIP        ?= best1305

DEBUG       ?= 1

FPGA        ?= 0

RTOS        ?= 1

LIBC_ROM    ?= 0

#KERNEL      ?= RTX5

ifeq ($(KERNEL), NUTTX)
export ALLOW_WARNING := 1
export LIBC_OVERRIDE := 0
export NO_LIBC := 1
export NUTTX_BUILD :=1
export RTC_ENABLE := 1
export NUTTX_BLE_BRIDGE_MODE ?= 0
export RAMCP_SIZE := 0x0
export A2DP_CP_ACCEL := 1
export SCO_CP_ACCEL := 1
KBUILD_CPPFLAGS += -DRAMCP_SIZE=$(RAMCP_SIZE)
KBUILD_CPPFLAGS += -DRAMCPX_SIZE=$(RAMCP_SIZE)

KBUILD_CFLAGS += -DPWR_KEY_AS_SYS_RQ_KEY -DNUTTX_SERIAL_CONSOLE

ifeq ($(RTC_ENABLE),1)
KBUILD_CPPFLAGS += -DCHIP_HAS_DIG_RTC -DRTC_BASE=0
endif
ifeq ($(NUTTX_BLE_BRIDGE_MODE),1)
KBUILD_CFLAGS += -DNUTTX_BLE_BRIDGE_MODE
endif
endif

export USER_SECURE_BOOT	?= 0
# enable:1
# disable:0

WATCHER_DOG ?= 1

DEBUG_PORT  ?= 1
# 0: usb
# 1: uart0
# 2: uart1

FLASH_CHIP	?= ALL
# GD25Q80C
# GD25Q32C
# ALL

#If you want to use the single-wire communication function,
#please open this Macro, otherwise the compilation error will be reported
export PMU_IRQ_UNIFIED ?= 0

export FUZZ_TEST_SUPPORT ?= 0

export NEW_SWAGC_MODE ?= 0

export BLE_NEW_SWAGC_MODE ?= 0

export BT_UART_LOG_P16 ?= 0

export BT_UART_LOG ?= 0

export BT_SYSTEM_52M ?= 0

export BES_FA_MODE ?= 0

export LL_MONITOR ?= 0

export SOFTBIT_EN ?= 0

export ACL_DATA_CRC_TEST ?= 0

export FORCE_SIGNALINGMODE ?= 0

export FORCE_NOSIGNALINGMODE ?= 0

export FORCE_SCO_MAX_RETX ?= 0

export BT_FA_ECC ?= 0

ifeq ($(BT_FA_ECC),1)
export BT_FA_SCO_ECC ?= 1
export BT_FA_ACL_ECC ?= 1
endif

export TRACE_GLOBAL_TAG ?= 1

export BT_ECC_CONFIG_BLK ?= 2

export BT_FAST_LOCK_ENABLE ?= 0

export IBRT_TESTMODE ?= 0

export CONTROLLER_DUMP_ENABLE ?= 1

export CONTROLLER_MEM_LOG_ENABLE ?= 0

export INTERSYS_DEBUG ?= 1

export PROFILE_DEBUG ?= 0

export BTDUMP_ENABLE ?= 0

export BT_DEBUG_TPORTS ?= 0

export AF_STREAM_ID_0_PLAYBACK_FADEOUT ?= 1

TPORTS_KEY_COEXIST ?= 0

export PATCH_SYMBOL_DIR ?= $(srctree)/platform/drivers/bt/best1305

export LD_USE_PATCH_SYMBOL ?= -Wl,--just-symbols=$(PATCH_SYMBOL_DIR)/patch_symbol_parsed.txt -Wl,--just-symbols=$(PATCH_SYMBOL_DIR)/patch_symbol_parsed_testmode.txt

AUDIO_OUTPUT_MONO ?= 0

AUDIO_OUTPUT_DIFF ?= 0

HW_FIR_EQ_PROCESS ?= 0

SW_IIR_EQ_PROCESS ?= 0

HW_DAC_IIR_EQ_PROCESS ?= 0

HW_IIR_EQ_PROCESS ?= 0

HW_DC_FILTER_WITH_IIR ?= 0

AUDIO_DRC ?= 0

AUDIO_LIMITER ?= 0

PC_CMD_UART ?= 0

AUDIO_SECTION_ENABLE ?= 0

AUDIO_RESAMPLE ?= 1

RESAMPLE_ANY_SAMPLE_RATE ?= 1

export OSC_26M_X4_AUD2BB ?= 0
export FLASH_LOW_SPEED ?= 1

AUDIO_OUTPUT_VOLUME_DEFAULT ?= 12
# range:1~16

AUDIO_INPUT_CAPLESSMODE ?= 0

AUDIO_INPUT_LARGEGAIN ?= 0

AUDIO_CODEC_ASYNC_CLOSE ?= 0

AUDIO_SCO_BTPCM_CHANNEL ?= 1

export A2DP_CP_ACCEL ?= 1

export SCO_CP_ACCEL ?= 1

export SCO_TRACE_CP_ACCEL ?= 0

# For TWS SCO DMA snapshot and low delay
export PCM_FAST_MODE ?= 1

export PCM_PRIVATE_DATA_FLAG ?= 0

export CVSD_BYPASS ?= 1

export LOW_DELAY_SCO ?= 0

export SPEECH_TX_24BIT ?= 0

SPEECH_TX_DC_FILTER ?= 0

SPEECH_TX_AEC2FLOAT ?= 0

SPEECH_TX_NS3 ?= 0

SPEECH_TX_2MIC_NS2 ?= 0

SPEECH_TX_COMPEXP ?= 0

SPEECH_TX_EQ ?= 0

SPEECH_TX_POST_GAIN ?= 0

SPEECH_RX_NS2FLOAT ?= 0

SPEECH_RX_EQ ?= 0

SPEECH_RX_POST_GAIN ?= 0

LARGE_RAM ?= 1

HSP_ENABLE ?= 0

VOICE_TX_AEC ?= 0

APP_NOISE_ESTIMATION ?= 0

SBC_FUNC_IN_ROM ?= 0

ROM_UTILS_ON ?= 1

VOICE_PROMPT ?= 1


# Power up source select
# 0: Power key and acon power up system.
# 1: Power key and acon, acon out power up system.
ACON_CAUSE_POWER_UP ?= 0

#### Google related feature ####
# the overall google service switch
# currently, google service includes BISTO and GFPS
export GOOGLE_SERVICE_ENABLE ?= 0

# BISTO is a GVA service on Bluetooth audio device
# BISTO is an isolated service relative to GFPS
export BISTO_ENABLE ?= 0

# macro switch for reduced_guesture
export REDUCED_GUESTURE_ENABLE ?= 0

# GSOUND_HOTWORD is a hotword library running on Bluetooth audio device
# GSOUND_HOTWORD is a subset of BISTO
export GSOUND_HOTWORD_ENABLE ?= 0

# this is a subset choice for gsound hotword
export GSOUND_HOTWORD_EXTERNAL ?= 0

# GFPS is google fastpair service
# GFPS is an isolated service relative to BISTO
export GFPS_ENABLE ?= 0
#### Google related feature ####

BLE ?= 0

TOTA ?= 0

GATT_OVER_BR_EDR ?= 0

export OTA_ENABLE ?= 0

TILE_DATAPATH_ENABLED ?= 0

CUSTOM_INFORMATION_TILE_ENABLE ?= 0

INTERCONNECTION ?= 0

INTERACTION ?= 0

INTERACTION_FASTPAIR ?= 0

BT_ONE_BRING_TWO ?= 0

DSD_SUPPORT ?= 0

A2DP_EQ_24BIT ?= 1

A2DP_AAC_ON ?= 1

A2DP_SCALABLE_ON ?= 0

A2DP_LHDC_ON ?= 0

ifeq ($(A2DP_LHDC_ON),1)
A2DP_LHDC_V3 ?= 0
endif

A2DP_LDAC_ON ?= 0

export TX_RX_PCM_MASK ?= 0

A2DP_SCALABLE_ON ?= 0

FACTORY_MODE ?= 1

ENGINEER_MODE ?= 1

ULTRA_LOW_POWER	?= 1

DAC_CLASSG_ENABLE ?= 1

NO_SLEEP ?= 0

CORE_DUMP ?= 1

CORE_DUMP_TO_FLASH ?= 0

ENHANCED_STACK ?= 1

export SYNC_BT_CTLR_PROFILE ?= 0

export A2DP_AVDTP_CP ?= 0

export A2DP_DECODER_VER := 2

export IBRT ?= 1

export IBRT_SEARCH_UI ?= 0

export IBRT_UI_V1 ?= 1
ifeq ($(IBRT_UI_V1),1)
KBUILD_CPPFLAGS += -DIBRT_UI_V1
endif

export IBRT_CONN_API ?= 0
ifeq ($(IBRT_CONN_API),1)
KBUILD_CPPFLAGS += -DIBRT_CONN_API_ENABLE
endif

export IBRT_MULTI_POINT ?= 0
ifeq ($(IBRT_MULTI_POINT),1)
KBUILD_CPPFLAGS += -DIBRT_MULTI_POINT
endif

export BES_AUD ?= 1

export POWER_MODE   ?= DIG_DCDC

export BT_RF_PREFER ?= 2M

export SPEECH_CODEC ?= 1

export MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED ?= 1
export IOS_MFI ?= 0

export FLASH_SIZE ?= 0x200000
export FLASH_SUSPEND ?= 1

export HOST_GEN_ECDH_KEY ?= 1

ifeq ($(DSD_SUPPORT),1)
export BTUSB_AUDIO_MODE     ?= 1
export AUDIO_INPUT_MONO     ?= 1
export USB_ISO              ?= 1
export USB_AUDIO_DYN_CFG    ?= 1
export DELAY_STREAM_OPEN    ?= 0
export KEEP_SAME_LATENCY    ?= 1
export HW_FIR_DSD_PROCESS   ?= 1
ifeq ($(HW_FIR_DSD_PROCESS),1)
ifeq ($(CHIP),best2300)
export HW_FIR_DSD_BUF_MID_ADDR  ?= 0x200A0000
export DATA_BUF_START           ?= 0x20040000
endif
endif
export USB_AUDIO_UAC2 ?= 1
export USB_HIGH_SPEED ?= 1
KBUILD_CPPFLAGS += \
    -DHW_FIR_DSD_BUF_MID_ADDR=$(HW_FIR_DSD_BUF_MID_ADDR) \
    -DDATA_BUF_START=$(DATA_BUF_START)
endif

USE_THIRDPARTY ?= 0
export USE_KNOWLES ?= 0

export LAURENT_ALGORITHM ?= 1

export TX_IQ_CAL ?= 1

export RX_IQ_CAL ?= 1

export BT_XTAL_SYNC ?= 1

export BTADDR_FOR_DEBUG ?= 1

export POWERKEY_I2C_SWITCH ?=0

AUTO_TEST ?= 0

BES_AUTOMATE_TEST ?= 0

export DUMP_NORMAL_LOG ?= 0

SUPPORT_BATTERY_REPORT ?= 1

SUPPORT_HF_INDICATORS ?= 0

SUPPORT_SIRI ?= 1

BES_AUDIO_DEV_Main_Board_9v0 ?= 0

APP_USE_LED_INDICATE_IBRT_STATUS ?= 0

export BT_EXT_LNA_PA ?=0
export BT_EXT_LNA ?=0
export BT_EXT_PA ?=0

ifeq ($(A2DP_LHDC_ON),1)
AUDIO_BUFFER_SIZE ?= 140*1024
else
AUDIO_BUFFER_SIZE ?= 86*1024
endif

export TRACE_BUF_SIZE ?= 8*1024
export TRACE_BAUD_RATE := 10*115200

export OS_DYNAMIC_MEM_SIZE ?= 15*1024#0x3000

init-y :=
core-y := platform/ services/ apps/ utils/cqueue/ utils/list/ services/multimedia/ utils/intersyshci/ utils/sha256/

KBUILD_CPPFLAGS += \
    -Iplatform/cmsis/inc \
    -Iservices/audioflinger \
    -Iplatform/hal

KBUILD_CPPFLAGS += \
    -DAPP_AUDIO_BUFFER_SIZE=$(AUDIO_BUFFER_SIZE) \
    -DCHARGER_PLUGINOUT_RESET=0

KBUILD_CPPFLAGS += \
    -DAAC_MTU_LIMITER=20 \
    -DSBC_MTU_LIMITER=180 \
    -DA2DP_AUDIO_MEMPOOL_SIZE=60*1024
 
 
ifneq ($(A2DP_CP_ACCEL)_$(SCO_CP_ACCEL),0_0)
export RAMCP_SIZE ?= 107*1024 #114*1024
export RAMCPX_SIZE ?= 66*1024 
else
export RAMCP_SIZE ?= 0
export RAMCPX_SIZE ?= 0
export CHIP_HAS_CP ?= 0
endif
KBUILD_CPPFLAGS += -DRAMCP_SIZE=$(RAMCP_SIZE)
KBUILD_CPPFLAGS += -DRAMCPX_SIZE=$(RAMCPX_SIZE)   

ifeq ($(BES_AUDIO_DEV_Main_Board_9v0),1)
KBUILD_CPPFLAGS += -DBES_AUDIO_DEV_Main_Board_9v0
endif

ifeq ($(TPORTS_KEY_COEXIST),1)
KBUILD_CPPFLAGS += -DTPORTS_KEY_COEXIST
endif

ifneq ($(FAST_XRAM_SECTION_SIZE),)
KBUILD_CPPFLAGS += -DFAST_XRAM_SECTION_SIZE=$(FAST_XRAM_SECTION_SIZE)
endif
#-DIBRT_LINK_LOWLAYER_MONITOR

#-D_AUTO_SWITCH_POWER_MODE__
#-D__APP_KEY_FN_STYLE_A__
#-D__APP_KEY_FN_STYLE_B__
#-D__EARPHONE_STAY_BOTH_SCAN__
#-D__POWERKEY_CTRL_ONOFF_ONLY__
#-DAUDIO_LINEIN

ifeq ($(CURRENT_TEST),1)
INTSRAM_RUN ?= 1
endif
ifeq ($(INTSRAM_RUN),1)
LDS_FILE := best1000_intsram.lds
else
LDS_FILE := best1000_1305.lds
endif

export OTA_SUPPORT_SLAVE_BIN ?= 0
ifeq ($(OTA_SUPPORT_SLAVE_BIN),1)
export SLAVE_BIN_FLASH_OFFSET ?= 0x100000
export SLAVE_BIN_TARGET_NAME ?= anc_usb
endif

ifeq ($(GATT_OVER_BR_EDR),1)
export GATT_OVER_BR_EDR ?= 1
KBUILD_CPPFLAGS += -D__GATT_OVER_BR_EDR__
endif

ifeq ($(TOTA),1)
ifeq ($(BLE),1)
KBUILD_CPPFLAGS += -DBLE_TOTA_ENABLED
endif
KBUILD_CPPFLAGS += -DSHOW_RSSI
KBUILD_CPPFLAGS += -DTEST_OVER_THE_AIR_ENANBLED
export TEST_OVER_THE_AIR ?= 1
endif

ifeq ($(BES_OTA),1)
export FLASH_REMAP ?= 1
endif

ifneq ($(A2DP_DECODER_VER), )
KBUILD_CPPFLAGS += -DA2DP_DECODER_VER=$(A2DP_DECODER_VER)
endif

ifeq ($(HOST_GEN_ECDH_KEY),1)
KBUILD_CPPFLAGS += -D__HOST_GEN_ECDH_KEY__
endif

ifeq ($(ACON_CAUSE_POWER_UP),1)
KBUILD_CPPFLAGS += -DPMU_ACON_CAUSE_POWER_UP
endif
KBUILD_CPPFLAGS += \
    -DHAL_TRACE_RX_ENABLE

# KBUILD_CPPFLAGS += -DVOICE_DEV
# KBUILD_CPPFLAGS += -DVOICE_ALGO_CP_ACCEL
# KBUILD_CPPFLAGS += -DVOICE_DEV_TEST

KBUILD_CFLAGS +=

LIB_LDFLAGS += -lstdc++ -lsupc++

#CFLAGS_IMAGE += -u _printf_float -u _scanf_float

#LDFLAGS_IMAGE += --wrap main

## CSD configurations
CSD ?= 1
ifeq ($(CSD),1)
KBUILD_CFLAGS += -DCSD
KBUILD_CPPFLAGS += -DCSD
endif
## END  CSD
