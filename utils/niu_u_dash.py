"""
niu u1 dash test

Need a USB to RS485 dongle.

Connection:
pin 1 Red White-12V - 12V
pin 8 Pink-Power lock - 48V
pin 4 Black-Negative - GND
pin 3 White-Comm A - 485A
pin 6 Purple Grey-Comm B - 485B
pin 9 Black White-485 GND - 485GND (can be omitted)
"""

import time
import serial


def build_frame(address, operation, payload):
    head = [0x68, address, (~address) & 0xff, 0x68]
    count = len(payload)
    payload_wire = bytes([(b + 0x33) & 0xff for b in payload])
    checksum = (sum(head) + operation + count + sum(payload_wire)) & 0xff

    data = bytearray()
    data.extend(head)
    data.append(operation)
    data.append(count)
    data.extend(payload_wire)
    data.append(checksum)
    data.append(0x16)
    return data


def parse_frame(data_raw):
    frame_start = -1
    for i in range(len(data_raw)):
        if data_raw[i] == 0x68:
            frame_start = i
            break
    if frame_start < 0:
        return False, -1

    data = data_raw[frame_start:]
    if len(data) < 6:
        return False, -2

    address = data[1]
    inv_address = data[2]
    if data[0] != 0x68 or data[3] != 0x68 or inv_address != ((~address) & 0xff):
        return False, -3

    operation = data[4]
    count = data[5]
    should_total = 4 + 1 + 1 + count + 1 + 1
    if len(data) < should_total:
        return False, -4

    should_checksum = sum(data[:6+count]) & 0xff
    checksum = data[6+count]
    if checksum != should_checksum:
        return False, -5

    if data[6 + count + 1] != 0x16:
        return False, -6

    payload_wire = data[6:6+count]
    payload = bytes([(b - 0x33) & 0xff for b in payload_wire])

    return True, (address, operation, payload)


def test_frame():
    payload = bytes.fromhex('06 f0 01 00 27 2e 00 00 00 00 64 00 cc cc 00')
    print('payload', payload.hex(' '))
    frame = build_frame(0x10, 0x04, payload)
    print('frame', frame.hex(' '))
    success, result = parse_frame(frame)
    if success:
        address, operation, payload = result
        print('success', 'address', hex(address), 'operation', hex(operation))
        print('payload', payload.hex(' '))
    else:
        print('failed', result)


class DashDisplay:
    def __init__(self):
        # charging mode 0x01
        # parking led 0x04
        # energy recovery icon 0x10
        # eco mode icon 0x20
        # parking led flash and show error code 0x80 (error code value is last byte)
        self.flags_0 = 0x06
        # cruise icon 0x01
        # error code show "UP" and show percent "%" 0x02
        # auto light icon 0x08
        # GPS icon constant with signal 0x10
        # GPS icon searching signal 0x20
        # GPS icon flashing without signal 0x30
        # GPRS icon constant with signal 0x40
        # GPRS icon searching signal 0x80
        # GPRS icon flashing without signal 0xC0
        self.flags_1 = 0xf0
        # MODE 1 0x01
        # MODE 2 0x02
        self.drive_mode = 0x01
        self.speed = 0x00
        # millage max 0x007fff
        self.millage = 0x002e27
        self.soc_percent = 0x64
        self.time_hour = 0xcc
        self.time_minute = 0xcc
        self.error_code = 0x00

    def build_payload(self):
        payload = bytes([
            self.flags_0, self.flags_1, self.drive_mode, self.speed,
            self.millage & 0xff, (self.millage >> 8) & 0xff, (self.millage >> 16) & 0xff,
            0x00, 0x00, 0x00,
            self.soc_percent, 0x00, self.time_hour, self.time_minute, self.error_code
        ])
        assert len(payload) == 0x0f
        return payload


def write_read(ser, data):
    ser.write(data)
    response = ser.read(128)
    return response


def send_dash_display(ser, payload):
    address = 0x10
    operation = 0x04
    frame = build_frame(address, operation, payload)
    frame_with_prefix = b'\xfe\xfe\xfe\xfe' + frame
    print('send', frame_with_prefix.hex(' '))
    response = write_read(ser, frame_with_prefix)
    print('response', response.hex(' '))
    success, result = parse_frame(response)
    if success:
        result_address, result_operation, result_payload = result
        print('success', 'address', hex(result_address), 'operation', hex(result_operation))
        print('payload', result_payload.hex(' '))
        print('address match', address == result_address)
        print('operation match', operation | 0x80 == result_operation)
    else:
        print('failed', result)


def simple_dash_test(ser):
    dash = DashDisplay()
    payload = dash.build_payload()
    send_dash_display(ser, payload)

    for i in range(10):
        time.sleep(1.0)
        dash.millage += 1
        dash.speed = i
        dash.soc_percent = 50 + i
        dash.time_hour = i
        dash.time_minute = i * 2
        dash.drive_mode = 0
        payload = dash.build_payload()
        send_dash_display(ser, payload)


def interactive_dash_test(ser):
    print('example: 06 f0 01 00 27 2e 00 00 00 00 64 00 cc cc 00')
    while True:
        request = input('raw> ').strip()
        if request:
            payload = bytes.fromhex(request)
            if len(payload) == 0x0f:
                send_dash_display(ser, payload)
            else:
                print('bad payload. length should be 0x0f')


def dash_demo(ser):
    dash = DashDisplay()
    dash.flags_0 = 0
    dash.flags_1 = 0
    dash.drive_mode = 0

    repeats = 15

    # speed
    for i in range(repeats):
        dash.speed = i * 5
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(0.1)
    dash.speed = 42

    # millage
    for i in range(repeats):
        dash.millage = i * 123
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(0.1)
    dash.millage = 24242

    # battery
    for i in range(repeats):
        dash.soc_percent = 100 - i
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(0.1)
    dash.soc_percent = 42

    # time
    for i in range(repeats):
        dash.time_hour = i
        dash.time_minute = i * 2
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(0.1)
    dash.time_hour = 42
    dash.time_minute = 42

    # error code
    dash.flags_0 = 0x80
    for i in range(repeats):
        dash.error_code = i
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(0.1)
    dash.flags_0 = 0x00
    dash.time_hour = 42
    dash.time_minute = 42

    # drive mode
    for drive_mode in [0x01, 0x02, 0x01 | 0x02]:
        dash.drive_mode = drive_mode
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(1.0)
    dash.drive_mode = 0

    # signal
    gps_list = [0x10, 0x20, 0x30, 0x00]
    gprs_list = [0x40, 0x80, 0xC0, 0x00]
    for gps, gprs in zip(gps_list, gprs_list):
        dash.flags_1 = gps | gprs        
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(2.0)

    # parking led
    for i in range(3):
        dash.flags_0 = 0x04
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(1.0)
        dash.flags_0 = 0x00
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(1.0)

    # other flags
    for i in range(3):
        dash.flags_0 = 0x10 | 0x20
        dash.flags_1 = 0x01 | 0x08
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(1.0)
        dash.flags_0 = 0x00
        dash.flags_1 = 0x00
        payload = dash.build_payload()
        send_dash_display(ser, payload)
        time.sleep(1.0)

    # charge mode
    dash.flags_0 = 0x01
    payload = dash.build_payload()
    send_dash_display(ser, payload)
    time.sleep(3.0)
    dash.flags_0 = 0x00
    payload = dash.build_payload()
    send_dash_display(ser, payload)
    time.sleep(1.0)

    # update mode
    dash.flags_1 = 0x02
    payload = dash.build_payload()
    send_dash_display(ser, payload)
    time.sleep(3.0)
    dash.flags_1 = 0x00
    payload = dash.build_payload()
    send_dash_display(ser, payload)
    time.sleep(1.0)


if __name__ == '__main__':
    # test_frame()
    # raise Exception()

    port = 'COM6'
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 9600
    ser.parity = serial.PARITY_EVEN
    ser.timeout = 0.2
    ser.open()

    dash = DashDisplay()
    payload = dash.build_payload()
    send_dash_display(ser, payload)

    # simple_dash_test(ser)

    # interactive_dash_test(ser)

    dash_demo(ser)

    print('end')
