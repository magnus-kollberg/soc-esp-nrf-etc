import asyncio
import inspect
import os
import sys
import json
import time
import base64
from datetime import datetime
from enum import Enum
from hashlib import sha256
# from ecdsa import SigningKey, VerifyingKey, NIST256p
import os

from bleak import BleakClient, BleakScanner, uuids
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData
from .debug import Thread

# Find the module path of the Command module and add that path for protobuf to sys path
from .commands import Command
module_path = os.path.dirname(inspect.getfile(Command))
sys.path.append("%s/protobuf" % module_path)
# from services import event_pb2 as EVENT
# from services import dose_manager_pb2 as DOSE_MANAGER
# from helpers import to_string as pb_to_string

RTC_TIME_FMT = "%Y-%m-%d %H:%M:%S"

class Provision:
    class State(Enum):
        INIT = 0
        START = 1
        USER_ID_SIGNED = 2
        CLAIM_CHALLENGE_OK = 3
        CU_CHALLENGE_OK = 4
        IOT_CHALLENGE_RESPONSE = 5
        DONE_SUCCESS = 6
        DONE_FAIL = 7

    IOT_CHALLENGE_SIZE = 64

    private_key_hex = None
    public_key_hex = None
    state = State.INIT
    CU_challenge = None
    IOT_challenge = None
    CU_serial_number_bytes = None
    user_id_bytes = None

    def __init__(self):
        self.param_init()
        
    def param_init(self):
        self.state = self.State.INIT
        self.IOT_challenge = bytearray(os.urandom(self.IOT_CHALLENGE_SIZE))

    def set_keys(self, private_key, public_key):
        self.private_key_hex = private_key
        self.public_key_hex = public_key
        print("Provision keys: \nPrivate %s \nPublic %s" % (self.private_key_hex, self.public_key_hex))
    
    def set_serial_number(self, serial_number_bytes):
        self.CU_serial_number_bytes = serial_number_bytes

    def set_user_id(self, userID_bytes):
        self.user_id_bytes = userID_bytes

    def set_IOT_challenge(self, challenge):
        self.IOT_challenge = challenge

    def set_next_state(self, result):
        if self.state == self.State.INIT:
            self.state = self.State.START if result else self.State.DONE_FAIL
            print("Provision keys: \nPrivate %s \nPublic %s" % (self.private_key_hex, self.public_key_hex))
        elif self.state == self.State.START:
            self.state = self.State.USER_ID_SIGNED if result else self.State.DONE_FAIL

        elif self.state == self.State.USER_ID_SIGNED:
            self.state = self.State.CLAIM_CHALLENGE_OK if result else self.State.DONE_FAIL

        elif self.state == self.State.CLAIM_CHALLENGE_OK:
            self.state = self.State.CU_CHALLENGE_OK if result else self.State.DONE_FAIL

        elif self.state == self.State.CU_CHALLENGE_OK:
            self.state = self.State.IOT_CHALLENGE_RESPONSE if result else self.State.DONE_FAIL

        elif self.state == self.State.IOT_CHALLENGE_RESPONSE:
            self.state = self.State.DONE_SUCCESS if result else self.State.DONE_FAIL
        print("Provision state: %s" % self.state)

    
    def sign_challenge(self, challenge_data):
        private_key = bytes.fromhex(self.private_key_hex)
        signing_key = SigningKey.from_string(private_key, curve=NIST256p, hashfunc=sha256)
        try:
            signature = signing_key.sign(challenge_data)
        except Exception as e:
            print("Error, %s" % e)
        return signature
    
    def verify_signature(self, signature, challenge):
        verify_ret = False
        if signature and challenge:
            public_key = bytes.fromhex(self.public_key_hex)
            verifying_key = VerifyingKey.from_string(public_key, curve=NIST256p, hashfunc=sha256)
            verify_ret = verifying_key.verify(signature, challenge)
        return verify_ret

    def combine_user_id_and_CU_SN(self):
        if self.user_id_bytes and self.CU_serial_number_bytes:
            concat_msg = self.user_id_bytes + bytearray(b'.') + self.CU_serial_number_bytes
            return concat_msg
        else:
            return None

class DataError(Exception):
    pass

class Bluetooth:
    CMD_SCAN                        = "scan"
    CMD_CONNECT                     = "connect"
    CMD_READ                        = "read"
    CMD_WRITE                       = "write"
    CMD_LIST                        = "list"
    CMD_DISCONNECT                  = "disconnect"
    CMD_QUIT                        = "quit"
    CMD_HELP                        = "help"
    CMD_PROVISION_START             = "provision"
    CMD_PROVISION_RUN               = "provision_run"
    CMD_PROVISION_BACKEND_CHALLENGE = "challenge"
    CMD_PROVISION_BACKEND_RESPONSE  = "response"
    EVT_NOTIFY                      = "notify"

    commands = {
        CMD_SCAN                        : ["Scan devices",             CMD_SCAN],
        CMD_CONNECT                     : ["Connect to device",        CMD_CONNECT + " <device-name>"],
        CMD_READ                        : ["Read uuid",                CMD_READ + " <uuid>"],
        CMD_WRITE                       : ["Write uuid",               CMD_WRITE + " <uuid> <data>"],
        CMD_LIST                        : ["List all services",        CMD_LIST],
        CMD_QUIT                        : ["Quit",                     CMD_QUIT],
        CMD_HELP                        : ["Help",                     CMD_HELP],
        CMD_PROVISION_START             : ["Provision",                CMD_PROVISION_START + " <user_id_16_bytes>"],
        CMD_PROVISION_BACKEND_CHALLENGE : ["Provision back challenge", CMD_PROVISION_BACKEND_CHALLENGE + " <backend_challenge_64_bytes>"],
        CMD_PROVISION_BACKEND_RESPONSE  : ["Provision back response",  CMD_PROVISION_BACKEND_RESPONSE + " <backend_reponse_signature_64_bytes>"],
    }

    UUID_ONDOSIS_BASE  = "-1212-efde-1523-785fef13d123"
    UUID_STANDARD_BASE = "-0000-1000-8000-00805f9b34fb"

    UUID_TIME_SERVICE                = "0000abad" + UUID_ONDOSIS_BASE
    UUID_TIME                        = "0000c001" + UUID_ONDOSIS_BASE
    UUID_EVENT_SERVICE               = "0000b10c" + UUID_ONDOSIS_BASE
    UUID_EVENT                       = "0000b001" + UUID_ONDOSIS_BASE
    UUID_EVENT_ERROR                 = "0000b002" + UUID_ONDOSIS_BASE
    UUID_DOSE_MANAGER_SERVICE        = "0000abcd" + UUID_ONDOSIS_BASE
    UUID_DOSE_MANAGER_DEVICE_INFO    = "0000ab01" + UUID_ONDOSIS_BASE
    UUID_DOSE_MANAGER_DEVICE_STATUS  = "0000ab02" + UUID_ONDOSIS_BASE
    UUID_DOSE_MANAGER_CARTRIDGE_INFO = "0000ab03" + UUID_ONDOSIS_BASE
    UUID_DOSE_MANAGER_STATISTICS     = "0000ab04" + UUID_ONDOSIS_BASE
    UUID_PROVISION_SERVICE           = "0000e000" + UUID_ONDOSIS_BASE
    UUID_PROVISION_USER_ID           = "0000e001" + UUID_ONDOSIS_BASE # Write
    UUID_PROVISION_CLAIM_SIGNATURE   = "0000e002" + UUID_ONDOSIS_BASE # Notify
    UUID_PROVISION_BACKEND_CHALLENGE = "0000e003" + UUID_ONDOSIS_BASE # Write
    UUID_PROVISION_CU_RESPONSE       = "0000e004" + UUID_ONDOSIS_BASE # Notify
    UUID_PROVISION_CU_CHALLENGE      = "0000e005" + UUID_ONDOSIS_BASE # Read
    UUID_PROVISION_BACKEND_RESPONSE  = "0000e006" + UUID_ONDOSIS_BASE # Write
    UUID_PROVISION_CU_CONFIRM        = "0000e007" + UUID_ONDOSIS_BASE # Notify
    UUID_TEST_SW_SERVICE             = "0000baad" + UUID_ONDOSIS_BASE
    UUID_TEST_SERVICE                = "0000e100" + UUID_ONDOSIS_BASE
    UUID_TEST_CMD                    = "0000e102" + UUID_ONDOSIS_BASE

    test_events = [UUID_TEST_CMD]
    events = [UUID_EVENT, UUID_EVENT_ERROR, UUID_PROVISION_CLAIM_SIGNATURE, UUID_PROVISION_CU_RESPONSE, UUID_PROVISION_CU_CONFIRM] + test_events

    uuid_descriptions = {
        UUID_EVENT_SERVICE               : "Event service",
        UUID_EVENT                       : "Event data",
        UUID_EVENT_ERROR                 : "Event error data",
        UUID_DOSE_MANAGER_SERVICE        : "Dose Manager service",
        UUID_DOSE_MANAGER_DEVICE_INFO    : "Dose Manager device information",
        UUID_DOSE_MANAGER_DEVICE_STATUS  : "Dose Manager device status",
        UUID_DOSE_MANAGER_CARTRIDGE_INFO : "Dose Manager cartridge information",
        UUID_DOSE_MANAGER_STATISTICS     : "Dose Manager statistics",
        UUID_TIME_SERVICE                : "Time service",
        UUID_TIME                        : "Time",
        UUID_PROVISION_SERVICE           : "Provision service",
        UUID_PROVISION_USER_ID           : "Provision user id",
        UUID_PROVISION_CLAIM_SIGNATURE   : "Provision claim signature",
        UUID_PROVISION_BACKEND_CHALLENGE : "Provision backend challenge",
        UUID_PROVISION_CU_RESPONSE       : "Provision cu response",
        UUID_PROVISION_CU_CHALLENGE      : "Provision cu challange",
        UUID_PROVISION_BACKEND_RESPONSE  : "Provision backend response",
        UUID_PROVISION_CU_CONFIRM        : "Provision cu confirm",
        UUID_TEST_SW_SERVICE             : "Test SW service",
        UUID_TEST_SERVICE                : "Test service",
        UUID_TEST_CMD                    : "USB test cmd",
    }

    def __init__(self, reconnect=True):
        """
        Constructor
        """        

        self.command_queue = asyncio.Queue()
        self.response_queue = None
        self.thread = None
        self.loop = asyncio.new_event_loop()
        self.device_name = "not-set"
        self.connected = False
        self.waiting_for_test_cmd = False
        self.internal_queue_tiemout_sec = 2
        self.provisioning = Provision()
        self.reconnect = reconnect

        uuids.register_uuids(Bluetooth.uuid_descriptions)

    def log(self, string, always=False):
        """
        Log function
        """

        if self.response_queue == None or always:
            print(string)

    def to_bytearray(self, data, size, name):
        if not isinstance(data, bytes):
            data = data.ljust(size, ' ').encode('utf-8')
        if len(data) != size:
            raise DataError("wrong size %d != %d for data '%s'" % (len(data), size, name))
        else:
            print("%s: %s" % (name, data))
            return data

    async def _scan(self):
        """
        Scans all advertising bluetooth devices
        """

        self.log("\nSCAN STARTED ...")
        devices = await BleakScanner.discover(timeout=1, return_adv=True)
        self.log("SCAN RESULT:")
        result = []
        for d, a in devices.values():
            device: BLEDevice = d 
            advertisement_data: AdvertisementData = a 
            attributes = {}
            attributes['address'] = device.address
            attributes['name'] = str(advertisement_data.local_name).replace('\xa0', '')
            print(attributes)
            if self.response_queue:
                result.append(attributes)
            else:
                self.log(attributes)

        if self.response_queue:
            response = {}
            response['command'] = Bluetooth.CMD_SCAN
            response['data'] = result
            self.response_queue.put(response)

    async def _list_services(self, services):
        """
        List all services supported by bluetooh device
        """

        result = []
        for service in services:
            serv = {}
            serv['handle'] = service.handle
            serv['uuid'] = str(service.uuid)
            serv['description'] = service.description
            characteristics = []
            for char in service.characteristics:
                chars = {}
                chars['handle'] = char.handle
                chars['uuid'] = str(char.uuid)
                chars['description'] = char.description
                chars['properties'] = char.properties
                serv['characteristics'] = chars
                characteristics.append(chars)
            serv['characteristics'] = characteristics
            result.append(serv)
        if self.response_queue:
            response = {}
            response['command'] = Bluetooth.CMD_LIST
            response['data'] = result
            self.response_queue.put(response)
        else:
            self.log(json.dumps(result, indent=2))
    
    async def _run_provision_flow(self, client, user_id):
        if (self.provisioning.state == self.provisioning.State.START) and (user_id != None):
            self.provisioning.set_user_id(user_id)
            try:         
                await self._write(client, Bluetooth.UUID_PROVISION_USER_ID, user_id, decode=False)
            except (IndexError, DataError) as e:
                print("Error, %s" % e)
            self.log("Provision: Wrote user ID to Dosage Manager")

        elif self.provisioning.state == self.provisioning.State.USER_ID_SIGNED:
            await self._read(client, Bluetooth.UUID_PROVISION_CU_CHALLENGE)
            if self.provisioning.CU_challenge:
                self.log("Provision: CU challenge is read")
                #self.provisioning.CU_challenge = "hejhopp".encode()
                signature = self.provisioning.sign_challenge(self.provisioning.CU_challenge)
                self.log("Provision signature: %s" %signature.hex())
                self.log("Provision: sending CU challenge signature")
                await self._write(client, Bluetooth.UUID_PROVISION_BACKEND_RESPONSE,signature, decode=False)
            else:
                self.log("Provision: No CU challenge data")

        elif self.provisioning.state == self.provisioning.State.CLAIM_CHALLENGE_OK:
            if self.provisioning.CU_challenge:
                #self.provisioning.CU_challenge = "hejhopp".encode()
                signature = self.provisioning.sign_challenge(self.provisioning.CU_challenge)
                self.log("Provision signature: %s" %signature.hex())
                self.log("Provision: sending CU challenge signature again")
                await self._write(client, Bluetooth.UUID_PROVISION_BACKEND_RESPONSE,signature, decode=False)
            else:
                self.log("Provision: No CU challenge data")

        elif self.provisioning.state == self.provisioning.State.CU_CHALLENGE_OK:
            IOT_challenge = bytearray(os.urandom(self.provisioning.IOT_CHALLENGE_SIZE))
            self.provisioning.set_IOT_challenge(IOT_challenge)
            self.log("Provision: IOT challenge %s" %IOT_challenge.hex())
            self.provisioning.set_next_state(True)
            await self._write(client, Bluetooth.UUID_PROVISION_BACKEND_CHALLENGE, self.provisioning.IOT_challenge, decode=False)

        elif self.provisioning.state == self.provisioning.State.DONE_FAIL:
            self.log("Provision: FAIL")

        elif self.provisioning.state == self.provisioning.State.DONE_SUCCESS:
            self.log("Provision: SUCCESS")

    def _handle_provision_notify(self, uuid, data):
        """
        Provision flow
        """
        if Bluetooth.UUID_PROVISION_CLAIM_SIGNATURE in uuid or \
               Bluetooth.UUID_PROVISION_CU_RESPONSE in uuid or \
               Bluetooth.UUID_PROVISION_CU_CONFIRM in uuid:
            
            if Bluetooth.UUID_PROVISION_CLAIM_SIGNATURE in uuid:
                self.log("Provision: claim signature from CU received %s" %data.hex())
                challenge = self.provisioning.combine_user_id_and_CU_SN()
                self.log("userID.CuSN: %s" % challenge.hex())

                verify_ret = self.provisioning.verify_signature(data, challenge)            
                self.log(f"CU signature response is {verify_ret} for User ID")

                self.provisioning.set_next_state(verify_ret)
                self.command(Bluetooth.CMD_PROVISION_RUN)

            if Bluetooth.UUID_PROVISION_CU_CONFIRM in uuid:
                if data:
                    is_IOT_signature_ok = (data[0] == 1)
                    self.log(f"Provision: CU response with {is_IOT_signature_ok} for Python signature")
                    self.provisioning.set_next_state(is_IOT_signature_ok)                  
                else:
                    self.provisioning.set_next_state(False)
                self.command(Bluetooth.CMD_PROVISION_RUN)

            if Bluetooth.UUID_PROVISION_CU_RESPONSE in uuid:
                verify_ret = self.provisioning.verify_signature(data, self.provisioning.IOT_challenge)

                self.log(f"CU signature response is {verify_ret} for IOT challenge")
                self.provisioning.set_next_state(verify_ret)
                self.command(Bluetooth.CMD_PROVISION_RUN)

    async def _notification_callback(self, characteristic, data):
        """
        Notification callback from bluetooh device
        """

        if self.response_queue:
            response = {}
            response['command'] = Bluetooth.EVT_NOTIFY
            response['uuid'] = characteristic.uuid
            response['data'] = data
            self.response_queue.put(response)
        else:
            message = self.parse_uuid(characteristic.uuid, data)
            self.log("Notification: %s\n" % message)
          
        self._handle_provision_notify(characteristic.uuid, data)

    async def _read(self, client, uuid):
        """
        Read uuid from bluetooh device
        """

        if uuid == None:
            if self.response_queue:
                response['command'] = Bluetooth.CMD_READ
                response['uuid'] = uuid
                response['status'] = "missing uuid"
                self.response_queue.put(response)
            else:
                self.log("\nError missing uuid")
            return

        self.log("\nRead uuid '%s'" % uuid)

        status = "ok"
        data = 0
        try:
            data = await client.read_gatt_char(uuid)
            message = self.parse_uuid(uuid, data)
            self.log(message)
        except Exception as e:
            self.log("Failed to read uuid '%s', error '%s'" % uuid, e)
            status = "read failed"
        if self.response_queue:
            response = {}
            response['command'] = Bluetooth.CMD_READ
            response['uuid'] = uuid
            response['data'] = data
            response['status'] = status
            self.response_queue.put(response)
        if (uuid == Bluetooth.UUID_PROVISION_CU_CHALLENGE) and (data != 0):
            self.provisioning.CU_challenge = data

    async def _write(self, client, uuid, data, decode = True):
        """
        Write uuid to bluetooh device
        """

        if uuid == None or data == None:
            missing = "uuid" if uuid == None else "data"
            if self.response_queue:
                response['command'] = Bluetooth.CMD_WRITE
                response['uuid'] = uuid
                response['status'] = "missing '%s'" % missing
                self.response_queue.put(response)
            else:
                self.log("\nError missing %s" % missing)
            return

        if(decode):
            try:
                data_decoded = base64.b64decode(data)
            except:
                data_decoded = data
        else:
            data_decoded = data

        try:
            status = "ok"
            await client.write_gatt_char(uuid, data_decoded, response=True)
        except:
            self.log("Failed to write uuid '%s'" % uuid)
            status = "failed"

        if self.response_queue:
            response = {}
            response['command'] = Bluetooth.CMD_WRITE
            response['uuid'] = uuid
            response['status'] = status
            self.response_queue.put(response)

    def _disconnected_callback(self, client):
        """
        Disconnect callback from bluetooh device
        """

        self.connected = False
        self.log("Device '%s' disconnected" % self.device_name, always=True)
        self.command(Bluetooth.CMD_DISCONNECT)
        if self.reconnect:
            self.command(Bluetooth.CMD_CONNECT + " " + self.device_name)

    async def _connect(self, device_name):
        """
        Connects to a bluetooh device
        """

        self.device_name = device_name
        self.log("\nFinding device '%s'" % self.device_name, always=True)

        device = await BleakScanner.find_device_by_name(self.device_name)
        if device is None:
            self.log("Device '%s' not found" % self.device_name)
            if self.response_queue:
                response = {}
                response['command'] = Bluetooth.CMD_CONNECT
                response['status'] = "failed"
                self.response_queue.put(response)
            return None

        self.log("Connecting to device '%s'" % self.device_name)

        #raise ValueError("An example error")

        command = "none"
        try:
            async with BleakClient(device, disconnected_callback=self._disconnected_callback, timeout=30) as client:
                self.log("Device '%s' connected" % self.device_name, always=True)
                self.connected = True

                if self.response_queue:
                    response = {}
                    response['command'] = Bluetooth.CMD_CONNECT
                    response['status'] = "ok"
                    self.response_queue.put(response)

                if self.notify:
                    events = Bluetooth.events
                else:
                    events = Bluetooth.test_events

                for event in events:
                    try:
                        await client.start_notify(event, self._notification_callback)
                        self.log("Subscript for notification: %s" % event)
                    except:
                        pass

                while True:
                    message = await self.command_queue.get()
                    command = message.split()[0]
                    if command == Bluetooth.CMD_QUIT or command == Bluetooth.CMD_DISCONNECT:
                        # self.log("Disconnecting")
                        break
                    elif command == Bluetooth.CMD_READ:
                        try:
                            uuid = message.split()[1]
                        except IndexError:
                            uuid = None
                        await self._read(client, uuid)
                    elif command == Bluetooth.CMD_WRITE:
                        try:
                            uuid = message.split()[1]
                        except IndexError:
                            uuid = None
                        try:
                            data = message.split()[2]
                        except IndexError:
                            data = None
                        await self._write(client, uuid, data)
                    elif command == Bluetooth.CMD_LIST:
                        await self._list_services(client.services)
                    elif command == Bluetooth.CMD_PROVISION_START:
                        self.provisioning.param_init()
                        self.provisioning.set_next_state(True)
                        data = self.to_bytearray(message.split()[1], 16, "user_id")
                        await self._run_provision_flow(client, data)     
                    elif command == Bluetooth.CMD_PROVISION_RUN:    
                        await self._run_provision_flow(client, None)

                    elif command == Bluetooth.CMD_PROVISION_BACKEND_CHALLENGE:
                        data = self.to_bytearray(message.split()[1], 64, "backend_challenge")
                        self.provisioning.set_IOT_challenge (data)

                    elif command == Bluetooth.CMD_PROVISION_BACKEND_RESPONSE:
                        print("No action")

                    elif command == Bluetooth.CMD_HELP:
                        self.help()
                    else:
                        self.log("Command ignored, connected to '%s', please disconnect first" % self.device_name)

                if client.is_connected:
                    if self.notify:
                        events = Bluetooth.events
                    else:
                        events = Bluetooth.test_events

                    for event in events:
                        try:
                            await client.stop_notify(event)
                        except Exception as e:
                            self.log("Error, stop subscription failed to uuid '%s' error %s" % (event,e))
        except Exception as e:
            self.log("Connect error %s" %e)

        self.log("Disconnected")

        if self.response_queue:
            response = {}
            response['command'] = Bluetooth.CMD_DISCONNECT
            response['status'] = "ok"
            self.response_queue.put(response)

        return command

    async def _async_task(self):
        """
        Main bluetooth async task which listen to commands
        """

        while True:
            message = await self.command_queue.get()
            command = message.split()[0]
            if command == Bluetooth.CMD_QUIT:
                break
            elif command == Bluetooth.CMD_SCAN:
                await self._scan()
            elif command == Bluetooth.CMD_CONNECT:
                ix = message.find(" ")
                if ix >= 0:
                    device_name = message[ix + 1:]
                    command = await self._connect(device_name)
                    if command == Bluetooth.CMD_QUIT:
                        break
                else:
                    self.log("Device name missing")
            elif command == Bluetooth.CMD_DISCONNECT or command == Bluetooth.CMD_READ:
                self.log("Not connected, command '%s' ignored" % command)
            elif command == Bluetooth.CMD_HELP:
                self.help()
            else:
                self.log("Unknown command '%s'" % command)

        # self.log("Bluetooth async task exiting")

    def _thread(self):
        """
        Main bluetooth thread which runs the async bluetooth task
        """

        asyncio.set_event_loop(self.loop)
        self.loop.run_until_complete(self._async_task())
        # self.log("Bluetooth thread exiting")

    def start(self, queue=None, notify=True):
        """
        Starts bluetooth thread
        """

        self.response_queue = queue
        self.notify = notify
        if not self.thread or not self.thread.is_alive():
            self.loop = asyncio.new_event_loop()
            self.thread = Thread(target=self._thread, daemon=True, title="bluetooth")
            self.thread.start()

    def stop(self):
        """
        Stop bluetooth thread
        """

        if self.thread and self.thread.is_alive():
            self.command(Bluetooth.CMD_QUIT)
            self.thread.join()
        # self.log("Bluetooth stopped")
        # Give asyncio some time to shutdown
        time.sleep(1)

    def check_for_exceptions(self):
        self.thread.check_for_exceptions()

    def command(self, command):
        """
        Send command to bluetooth thread
        """

        asyncio.run_coroutine_threadsafe(self.command_queue.put(command), self.loop)

    def parse_uuid(self, uuid, data):
        """
        Parse some uuid's
        """

        try:
            if Bluetooth.UUID_EVENT in uuid:
                event = EVENT.EventService()
                event.ParseFromString(data)
                if event.HasField("device"):
                    timestamp = datetime.fromtimestamp(event.device.timestamp.seconds)
                elif event.HasField("functional"):
                    timestamp = datetime.fromtimestamp(event.functional.timestamp.seconds)
                elif event.HasField("dispense"):
                    timestamp = datetime.fromtimestamp(event.dispense.timestamp.seconds)
                elif event.HasField("test"):
                    timestamp = datetime.fromtimestamp(event.test.timestamp.seconds)
                else:
                    timestamp = datetime.fromtimestamp(0)
                message = "\nEVENT:\nTimestamp: %s\n%s" % (timestamp.strftime(RTC_TIME_FMT), pb_to_string(event))
            elif Bluetooth.UUID_EVENT_ERROR in uuid:
                event = EVENT.ErrorService()
                event.ParseFromString(data)
                timestamp = datetime.fromtimestamp(event.error.timestamp.seconds)
                message = "\nEVENT-ERROR:\nTimestamp: %s\n%s" % (timestamp.strftime(RTC_TIME_FMT), pb_to_string(event))
            elif Bluetooth.UUID_DOSE_MANAGER_DEVICE_INFO in uuid:
                dose_manager = DOSE_MANAGER.DeviceInformation()
                dose_manager.ParseFromString(data)
                message = "\nDEVICE_INFO\n%s" % pb_to_string(dose_manager)
            elif Bluetooth.UUID_DOSE_MANAGER_DEVICE_STATUS in uuid:
                dose_manager = DOSE_MANAGER.DeviceStatus()
                dose_manager.ParseFromString(data)
                message = "\nDEVICE_STATUS\n%s" % pb_to_string(dose_manager)
            elif Bluetooth.UUID_DOSE_MANAGER_CARTRIDGE_INFO in uuid:
                dose_manager = DOSE_MANAGER.CartridgeInformation()
                dose_manager.ParseFromString(data)
                message = "\nCARTRIDGE_INFO\n%s" % pb_to_string(dose_manager)
            elif Bluetooth.UUID_DOSE_MANAGER_STATISTICS in uuid:
                dose_manager = DOSE_MANAGER.Statistics()
                dose_manager.ParseFromString(data)
                message = "\nSTATISTICS\n%s" % pb_to_string(dose_manager)
            elif Bluetooth.UUID_PROVISION_CLAIM_SIGNATURE in uuid:
                hex = ''.join(format(byte, '02x') for byte in data)
                message = "\nUUID_PROVISION_CLAIM_SIGNATURE\nraw-data: %s\nascii: '%s'" % (hex, data.decode("utf-8", errors="ignore"))
            elif Bluetooth.UUID_PROVISION_CU_RESPONSE in uuid:
                hex = ''.join(format(byte, '02x') for byte in data)
                message = "\nUUID_PROVISION_CU_RESPONSE\nraw-data: %s\nascii: '%s'" % (hex, data.decode("utf-8", errors="ignore"))
            elif Bluetooth.UUID_PROVISION_CU_CHALLENGE in uuid:
                hex = ''.join(format(byte, '02x') for byte in data)
                message = "\nUUID_PROVISION_CU_CHALLENGE\nraw-data: %s\nascii: '%s'" % (hex, data.decode("utf-8", errors="ignore"))
            elif Bluetooth.UUID_PROVISION_CU_CONFIRM in uuid:
                hex = ''.join(format(byte, '02x') for byte in data)
                message = "\nUUID_PROVISION_CU_CONFIRM\nraw-data: %s\nascii: '%s'" % (hex, data.decode("utf-8", errors="ignore"))
            elif Bluetooth.UUID_TEST_CMD in uuid:
                hex = ''.join(format(byte, '02x') for byte in data)
                message = "\nUUID_TEST_CMD\nraw-data: %s\nascii: '%s'" % (hex, data.decode("utf-8", errors="ignore"))
            elif Bluetooth.UUID_TIME in uuid:
                timestamp = int.from_bytes(data, "little")
                time = datetime.fromtimestamp(timestamp)
                message = "\nTIME\n%s" % time
            else:
                # Not supported
                hex = ''.join(format(byte, '02x') for byte in data)
                message = "\nUnknown uuid '%s'\nraw-data: %s\nascii: '%s'" % (uuid, hex, data.decode("utf-8", errors="ignore"))
        except Exception as e:
            self.log("Failed to parse uuid '%s', error '%s'" % (uuid, e))
            hex = ''.join(format(byte, '02x') for byte in data)
            message = "\nFailed to parse uuid '%s'\nraw-data: %s\nascii: '%s'" % (uuid, hex, data.decode("utf-8", errors="ignore"))

        return message

    def help(self):
        """
        Print all supported commands
        """

        self.log("Available commands:\n", always=True)
        self.log("%-10s %-30s %s\n" % ("command","Information","Syntax"), always=True)
        for (key, value) in self.commands.items():
            self.log("%-10s %-30s %s" % (key, value[0], value[1]), always=True)
        self.log("", always=True)

    def is_connected(self):
        """
        Return if we are connected or not
        """
        return self.connected

    def set_provision_params(self, private_key, public_key, serial_number):
        self.provisioning.set_keys(private_key, public_key)
        self.provisioning.set_serial_number(self.to_bytearray(serial_number, 19, "CU_SN"))
        
        
