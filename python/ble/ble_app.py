#!/usr/bin/env python3.10

import argparse
import readline
import sys
import json
import queue as Queue

from src.bluetooth import Bluetooth
# from src.config import Config

# Make keyboard input() (readline) behave like a "real" command prompt
readline.parse_and_bind('set editing-mode vi')

def scan_and_exit(bluetooth : Bluetooth):
    print("Scan and exit")
    queue = Queue.Queue()
    bluetooth.start(queue)
    bluetooth.command(Bluetooth.CMD_SCAN)
    try:
        response = queue.get(timeout=10)
        print(json.dumps(response, indent=2))
    except Queue.Empty:
        print("Timeout waiting for scan response")
    bluetooth.stop()
    sys.exit(0)

def list_and_exit(bluetooth : Bluetooth, device_name):
    print("List and exit")
    queue = Queue.Queue()
    bluetooth.start(queue, notify=False)
    command = Bluetooth.CMD_CONNECT + " " + device_name
    bluetooth.command(command)
    bluetooth.command(Bluetooth.CMD_LIST)
    try:
        response = queue.get(timeout=10)
        print(json.dumps(response, indent=2))
    except Queue.Empty:
        print("Timeout waiting for list response")
    bluetooth.stop()
    sys.exit(0)

def read_and_exit(bluetooth : Bluetooth, device_name, uuid):
    if device_name == None:
        print("parameter device_name is missing")
        sys.exit(1)
    if uuid == None:
        print("parameter uuid is missing")
        sys.exit(1)
    print("Read '%s' and exit" % uuid)
    queue = Queue.Queue()
    bluetooth.start(queue, notify=False)
    command = Bluetooth.CMD_CONNECT + " " + device_name
    bluetooth.command(command)
    command = Bluetooth.CMD_READ + " " + uuid
    bluetooth.command(command)
    try:
        response = queue.get(timeout=10)
        print(response)
    except Queue.Empty:
        print("Timeout waiting for read response")
    bluetooth.stop()
    sys.exit(0)

def write_and_exit(bluetooth : Bluetooth, device_name, uuid, data):
    if device_name == None:
        print("parameter device_name is missing")
        sys.exit(1)
    if uuid == None:
        print("parameter uuid is missing")
        sys.exit(1)
    if data == None:
        print("parameter data is missing")
        sys.exit(1)
    print("Write '%s' = '%s' and exit" % (uuid, data))
    queue = Queue.Queue()
    bluetooth.start(queue, notify=False)
    command = Bluetooth.CMD_CONNECT + " " + device_name
    bluetooth.command(command)
    command = Bluetooth.CMD_WRITE + " " + uuid + " " + data
    bluetooth.command(command)
    try:
        response = queue.get(timeout=10)
        print(response)
    except Queue.Empty:
        print("Timeout waiting for write response")
    bluetooth.stop()
    sys.exit(0)

def connect_and_wait(bluetooth : Bluetooth, device_name):
    print("Connect and wait")
    print("Ctrl-C to exit")
    queue = Queue.Queue()
    bluetooth.start(queue)
    command = Bluetooth.CMD_CONNECT + " " + device_name
    bluetooth.command(command)
    try:
        while True:
            response = queue.get()
            if response["command"] == Bluetooth.EVT_NOTIFY:
                event = bluetooth.parse_uuid(response["uuid"], response["data"])
                print(event)
            else:
                print(response)
    except KeyboardInterrupt:
        print("Ctrl-C: Exiting program")
    bluetooth.stop()
    sys.exit(0)

def provision_and_wait(bluetooth : Bluetooth, device_name):
    print("Provision and wait")
    print("Ctrl-C to exit")
    queue = Queue.Queue()
    bluetooth.start(queue)
    command = Bluetooth.CMD_CONNECT + " " + device_name
    bluetooth.command(command)
    try:
        command = Bluetooth.CMD_PROVISION_START + " hej"
        bluetooth.command(command)
        while True:
            response = queue.get()
            if response["command"] == Bluetooth.EVT_NOTIFY:
                event = bluetooth.parse_uuid(response["uuid"], response["data"])
                print(event)
            else:
                print(response)
    except KeyboardInterrupt:
        print("Ctrl-C: Exiting program")
    bluetooth.stop()
    sys.exit(0)

def _main(args):
    bluetooth = Bluetooth()

    # config = Config()
    # file_cu = None
    # if args.file_cu:
    #     file_cu = args.file_cu
    # cu_config = config.read_keys_and_ca_table_from_file(file_cu)
    # bluetooth.set_provision_params(cu_config['provisioningPrivateKeyAsHex'], cu_config['provisioningPublicKeyAsHex'], cu_config["cuSerialNr"])

    if args.scan:
        scan_and_exit(bluetooth)
    elif args.list:
        list_and_exit(bluetooth, args.list)
    elif args.read:
        read_and_exit(bluetooth, args.connect, args.read)
    elif args.write:
        write_and_exit(bluetooth, args.connect, args.write, args.data)
    elif args.connect:
        connect_and_wait(bluetooth, args.connect)
    elif args.provision:
        provision_and_wait(bluetooth, args.provision)
    else:
        bluetooth.start()
        bluetooth.help()

    try:
        while True:
            command = input("Command>> ")
            command.lower()
            if command == Bluetooth.CMD_QUIT:
                bluetooth.stop()
                break
            elif len(command):
                bluetooth.command(command)
    except KeyboardInterrupt:
        print("Ctrl-C: Exiting program")
        bluetooth.stop()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--scan', help="Scan and exit", action="store_true")
    parser.add_argument('--list', help="List and exit", metavar="<device-name>")
    parser.add_argument('--connect', help="Connect and wait", metavar="<device-name>")
    parser.add_argument('--provision', help="Provision and wait", metavar="<device-name>")
    parser.add_argument('--read', help="Read UUID", metavar="<uuid>")
    parser.add_argument('--write', help="Write UUID", metavar="<uuid>")
    parser.add_argument('--data', help="Data to write", metavar="<data>")
    parser.add_argument('--file_cu', help="CU configuration parameters (yaml)")
    args = parser.parse_args()

    _main(args)

if __name__ == "__main__":
    main()
