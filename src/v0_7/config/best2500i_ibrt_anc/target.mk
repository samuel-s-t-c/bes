export A2DP_CP_ACCEL ?= 1
export SCO_CP_ACCEL ?= 1
export OS_DYNAMIC_MEM_SIZE ?= 17*1024#0x3000
export FAST_XRAM_SECTION_SIZE ?=0x8500
include config/best2500i_ibrt/target.mk

#### ANC DEFINE START ######
export ANC_APP ?= 1

#### ANC CONFIG ######
export ANC_FF_ENABLED	    ?= 1
export ANC_FB_ENABLED	    ?= 1
export ANC_WNR_ENABLED	    ?= 0
export ANC_ASSIST_ENABLED   ?= 0
export AUDIO_ANC_FB_MC      ?= 0
export AUDIO_ANC_FB_MC_HW   ?= 1
export AUDIO_ANC_FB_ADJ_MC  ?= 0
export AUDIO_SECTION_SUPPT  ?= 1
export AUD_SECTION_STRUCT_VERSION ?= 2
##### ANC DEFINE END ######

export PSAP_APP  ?= 0



APP_ANC_TEST ?= 1
TEST_OVER_THE_AIR ?= 1
ifeq ($(ANC_APP),1)
KBUILD_CPPFLAGS += \
    -DANC_APP \
    -D__BT_ANC_KEY__\
    -D__APP_KEY_FN_STYLE_A__ 
endif

ifeq ($(USE_CYBERON),1)

export THIRDPARTY_LIB ?= cyberon
KBUILD_CPPFLAGS += -D__CYBERON

export KWS_IN_RAM := 0
ifeq ($(KWS_IN_RAM),1)
CPPFLAGS_${LDS_FILE} += -DKWS_IN_RAM
endif #KWS_IN_RAM

endif #USE_CYBERON
