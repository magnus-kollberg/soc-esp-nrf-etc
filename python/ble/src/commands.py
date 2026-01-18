# Copyright (c) 2018 Ondosis AB
#
# All rights are reserved.
# Proprietary and confidential.
# Unauthorized copying of this file, via any medium is strictly prohibited.
# Any use is subject to an appropriate license granted by Ondosis AB

"""Communication interface against BEP"""

# import binascii
from abc import ABCMeta
import enum

# import logging
# import struct


class CommandBase(enum.IntEnum):
    __metaclass__ = ABCMeta

# class ArgumentBase(enum.IntEnum):
#     __metaclass__ = ABCMeta

# UNITS AVAILABLE IN THE FM2 BOARD
# Motor
# Display
# Battery
# USB
# External Flash
# Cartridge Memory
# Real Time Clock
# Fingerprint Sensor
# Button Push


class Command(CommandBase):
    # Ondosis commands

    CMD_DEFAULT = 0x0
    CMD_USB_TRANSFER = 0x1
    CMD_DEVICE_RESTART = 0x2
    CMD_START_BOOTLOADER = 0x3
    CMD_KEEP_DEVICE_ON = 0x4

    """ Display """
    CMD_DISPLAY_SET_THEME_COLOR = 0x1001
    CMD_DISPLAY_SET_ERROR_COLOR = 0x1002

    """ Battery """
    CMD_BATTERY_INIT = 0x2001
    CMD_BATTERY_UNINIT = 0x2002
    
    """ TEST """
    CMD_TEST_SET = 0x3001
    CMD_TEST_GET = 0x3002
    
    """ External Flash """
    CMD_EXT_MEM_INIT = 0x4001
    CMD_EXT_MEM_UNINIT = 0x4002
    CMD_EXT_MEM_FORMAT = 0x4003
    CMD_EXT_MEM_WRITE_FILE = 0x4004
    CMD_EXT_MEM_READ_FILE = 0x4005
    CMD_EXT_MEM_WRITE_FEVENT = 0x4006
    CMD_EXT_MEM_READ_FEVENT = 0x4007
    CMD_EXT_MEM_LAST_WRITE_ADDRESS = 0x4008
    CMD_READ_NBR_LOGS = 0x4009
    CMD_READ_LOGS = 0x400A
    CMD_RUN_GC = 0x400B
    CMD_READ_DEVICE_CONF = 0x400C
    CMD_READ_DEVICE_PROP = 0x400D
    CMD_WRITE_DEVICE_CONF = 0x400E
    CMD_WRITE_DEVICE_PROP = 0x400F
    CMD_WRITE_TEST_SW_CONF = 0x4010
    CMD_READ_TEST_SW_CONF = 0x4011
    CMD_WRITE_FEVENT_ONDEVICE = 0x4012
    CMD_READ_SW_REVISION_STRING = 0x4013

    """ Crypto """
    CMD_CRYPTO_WRITE_KEY_RO  = 0x4021
    CMD_CRYPTO_WRITE_KEY_RW  = 0x4022
    CMD_CRYPTO_WRITE_KEY_PROVISION_PRIVATE = 0x4023
    CMD_CRYPTO_WRITE_KEY_PROVISION_PUBLIC  = 0x4024
    
    """ FLASH """
    CMD_RUN_FDS_GARBAGE_COLEECTION = 0x4025

    """ MOTOR & VIBRATION data log """
    CMD_READ_TEST_DATA_LOG_1   =    0x4026
    CMD_READ_TEST_DATA_LOG_2   =    0x4027
    CMD_READ_TEST_DATA_LOG_3   =    0x4028
    CMD_READ_TEST_DATA_LOG_4   =    0x4029
    CMD_READ_TEST_DATA_LOG_5   =    0x402A
    CMD_READ_TEST_DATA_LOG_6   =    0x402B
    CMD_READ_TEST_DATA_LOG_7   =    0x402C

    """ Cartridge Memory """
    CMD_CARTRIDGE_INIT = 0x5001
    CMD_CARTRIDGE_UNINIT = 0x5002
    CMD_CARTRIDGE_FORMAT = 0x5003
    CMD_CARTRIDGE_WRITE = 0x5004
    CMD_CARTRIDGE_READ = 0x5005
    CMD_ACC_READ = 0x5006
    CMD_ACC_VIB_CALIB_READ = 0x5007
    CMD_ACC_VIB_CALIB_WRITE = 0x5008
    CMD_ACC_ANGLE_READ = 0x5009
    CMD_CARTRIDGE_WRITE_RO = 0x500a
    CMD_CARTRIDGE_WRITE_RW = 0x500b
    CMD_CARTRIDGE_WRITE_SIGNATURE = 0x500c
    CMD_CARTRIDGE_READ_RO = 0x500d
    CMD_CARTRIDGE_READ_RW = 0x500e
    CMD_CARTRIDGE_READ_SIGNATURE = 0x500f
    CMD_CARTRIDGE_READ_MAC = 0x5010
    CMD_CARTRIDGE_READ_NONCE = 0x5011
    CMD_CARTRIDGE_WRITE_KEYS = 0x5012

    """ Real Time Clock """
    CMD_RTC_INIT = 0x6001
    CMD_RTC_UNINIT = 0x6002
    CMD_RTC_SET = 0x6003
    CMD_RTC_GET = 0x6004
    CMD_RTC_SOFT_RESET = 0x6005
    
    """ Fingerprint Sensor """
    CMD_FP_SENSOR_INIT = 0x7001
    CMD_FP_SENSOR_UNINIT = 0x7002
    
    """ Button Push """
    CMD_BUTTON_INIT = 0x8001
    CMD_BUTTON_UNINIT = 0x8002

class TestCommand(CommandBase):
    TEST_LOW_MEDICIN                  = 0
    TEST_NO_MEDICIN                   = 1
    TEST_RESET_MEDICIN                = 2
    TEST_BATTERY_LEVEL                = 3
    TEST_ERROR_DISPENSE_CU_ERROR      = 4
    TEST_ERROR_DISPENSE_CA_ERROR      = 5
    TEST_ERROR_DISPENSE_WRONG_ANGLE   = 6
    TEST_ERROR_DISPENSE_OK            = 7
    TEST_ERROR_TEMPERATURE_LOW        = 8
    TEST_ERROR_TEMPERATURE_HIGH       = 9
    TEST_ERROR_TEMPERATURE_NORMAL     = 10
    TEST_USB_POWER_CONNECTED          = 11
    TEST_USB_POWER_DISCONNECTED       = 12
    TEST_ERROR_CA_NEAR_EXPIRED        = 13
    TEST_ERROR_CA_EXPIRED             = 14
    TEST_ERROR_CU_LAST_DAY_TO_DOCK    = 15
    TEST_ERROR_CA_INCORRECT           = 16
    TEST_ERROR_CU_EXPIRED             = 17
    TEST_ERROR_CU_BROKEN              = 18
    TEST_CA_CU_OK                     = 19
    TEST_CONFIRM_NEW_CA               = 20
    TEST_DISPENSE_DOSE                = 21
    TEST_DISPENSE_OK                  = 22
    TEST_SET_DOSE_10                  = 23
    TEST_SET_DOSE_20                  = 24
    TEST_ERROR_CU_CA_NEAR_EXPIRED     = 25
    TEST_ANGLE_LIMIT                  = 26
    TEST_MODE_SET                     = 27
    TEST_ONE_DOSE_MEDICIN             = 28
    TEST_NEXT_DAY                     = 29
    TEST_ERROR_CA_BROKEN              = 30
    TEST_CU_STANDBY                   = 31

class Cmd_Data:
    crc: int = 0
    msg_len: int = 0
    cmd: Command = Command.CMD_DEFAULT
    ply_len: int = 0
    ply_load: list = []
    nbr_arg: int = 0
    arg: list = []

    def clear(self):
        self.crc = 0
        self.msg_len = 0
        self.cmd = Command.CMD_DEFAULT
        self.ply_len = 0
        self.ply_load = []
        self.nbr_arg = 0
        self.arg = []
