
import os
import sys
import glob


DEVICE_MAP = {
    0x31: 'BMS',
    0x20: 'FOC',
    0x10: 'Dash',
}


class Packet:
    def __init__(self, address, operation, payload):
        self.address = address
        self.operation = operation
        self.payload = payload

    def format_raw(self):
        address = self.address
        operation = self.operation
        payload = self.payload

        head = [0x68, address, (~address) & 0xff, 0x68]
        count = len(payload)
        payload_raw = bytes([(b + 0x33) & 0xff for b in payload])
        checksum = (sum(head) + operation + count + sum(payload_raw)) & 0xff

        data = bytearray()
        data.extend(head)
        data.append(operation)
        data.append(count)
        data.extend(payload_raw)
        data.append(checksum)
        data.append(0x16)
        return data

    def format_readable(self):
        data = bytearray()
        data.append(self.address)
        data.append(self.operation)
        data.extend(self.payload)
        return data


class State:
    EMPTY = 0
    HEAD = 1
    BODY = 2


class PacketDetector:
    def __init__(self, packet_callback):
        self.packet_callback = packet_callback

        self.state = State.EMPTY

        self.head = []
        self.head_time = 0.0
        self.head_address = 0
        self.body = []

    def push(self, t, data):
        for b in data:
            if self.state == State.EMPTY:
                if b == 0x68:
                    self.state = State.HEAD
                    self.head.clear()
                    self.head.append(b)
                    self.head_time = t
            elif self.state == State.HEAD:
                self.head.append(b)
                if len(self.head) >= 4:
                    begin = self.head[0]
                    address = self.head[1]
                    inv_address = self.head[2]
                    end = self.head[3]
                    if address ^ inv_address == 0xff and begin == 0x68 and end == 0x68:
                        self.state = State.BODY
                        self.body.clear()
                    else:
                        self.state = State.EMPTY
            elif self.state == State.BODY:
                self.body.append(b)
                if b == 0x16:
                    complete = self._try_parse_packet(self.head_time, self.head, self.body)
                    if complete:
                        self.state = State.EMPTY
        self.buffer = b''

    def _try_parse_packet(self, t, head, data):
        # op count payload checksum 0x16
        data_len = len(data)
        index = 0
        if index < data_len:
            op = data[index]
            index += 1
        else:
            return False
        if index < data_len:
            count = data[index]
            index += 1
        else:
            return False
        if index + count < data_len:
            payload_raw = bytes(data[index:index+count])
            index += count
        else:
            return False
        if index < data_len:
            checksum = data[index]
            index += 1
        else:
            return False
        if index < data_len:
            last = data[index]
            index += 1
        else:
            return False
        if last != 0x16:
            print('last byte error', t, head, data)
            return True
        calc_checksum = (sum(head) + op + count + sum(payload_raw)) & 0xff
        if checksum != calc_checksum:
            print('checksum error', t, head, data)
            return True

        address = head[1]
        payload = bytes([(b - 0x33) & 0xff for b in payload_raw])
        self.on_packet(t, address, op, payload)
        return True

    def on_packet(self, t, address, operation, payload):
        packet = Packet(address, operation, payload)
        self.packet_callback(t, packet)


def main(logfile, out_folder):
    with open(logfile, 'r') as f:
        raw_lines = f.readlines()

    raw_data = []
    for raw_lines in raw_lines:
        time_str, hex_str = raw_lines.split(':')
        t = float(time_str)
        data = bytes.fromhex(hex_str)
        # (time, data)
        raw_data.append((t, data))

    packets = []
    def on_packet(t, packet):
        packets.append((t, packet))
    detector = PacketDetector(on_packet)
    for t, data in raw_data:
        detector.push(t, data)

    print('Detected {} packets'.format(len(packets)))

    logfile_name = os.path.basename(logfile)
    prefix, ext = os.path.splitext(logfile_name)
    outfile_raw = os.path.join(out_folder, prefix + '_raw' + ext)
    outfile_readable = os.path.join(out_folder, prefix + '_readable' + ext)
    line_format = '{:<20}: {}'
    line_readable_format = '{:<20}: {}{:8}: {:02x} {:02x} {:02x} | {}'
    with open(outfile_raw, 'w') as f_raw, open(outfile_readable, 'w') as f_readable:
        for t, packet in packets:
            data_raw = packet.format_raw()
            # print('{}: {}'.format(t, data_raw.hex(' ')))
            f_raw.write(line_format.format(t, data_raw.hex(' ')))
            f_raw.write('\n')
            device_name = DEVICE_MAP.get(packet.address, 'unknown')
            direction = '<-' if packet.operation & 0x80 else '->'
            line = line_readable_format.format(
                t,
                direction, device_name,
                packet.address, packet.operation, len(packet.payload),
                packet.payload.hex(' ')
            )
            f_readable.write(line)
            f_readable.write('\n')



if __name__ == '__main__':
    if len(sys.argv) >= 2:
        logfiles = [sys.argv[1]]
    else:
        logs_folder = '../logs'
        logfiles = glob.glob(os.path.join(logs_folder, 'serial-*.txt'))

    for logfile in logfiles:
        print('process', logfile)
        main(logfile, '../logs_processed')
