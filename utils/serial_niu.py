
import serial


def write_read(ser, data):
    ser.write(data)
    response = ser.read(100)
    return response


def send_command(ser, data_str):
    request = bytes.fromhex(data_str)
    reponse = write_read(ser, request)
    print(reponse.hex(' '))


COMMON_COMMANDS = {
    'foc0': 'fe fe fe fe 68 20 df 68 01 01 36 07 16',
    'foc1': 'fe fe fe fe 68 20 df 68 05 02 43 ae c7 16',
    'bat0': 'fe fe fe fe 68 31 ce 68 02 02 60 42 75 16',
    'dash0': 'fe fe fe fe 68 10 ef 68 04 0f 39 33 34 33 51 73 33 33 33 33 7a 39 ff ff 33 29 16',
    'dash1': 'fe fe fe fe 68 10 ef 68 04 0f 39 33 35 33 51 73 33 33 33 33 7a 39 ff ff 33 2a 16',

    'focb0': 'fe fe fe fe 68 20 df 68 01 01 34 05 16',  # 01
    'focb1': 'fe fe fe fe 68 20 df 68 01 01 36 07 16',  # 03
    'focb2': 'fe fe fe fe 68 20 df 68 01 01 35 06 16',  # 02
    'focb3': 'fe fe fe fe 68 20 df 68 01 01 37 08 16',  # 04
    'focb4': 'fe fe fe fe 68 20 df 68 05 02 4f b9 de 16',  # 1c 86 (expect ce)
    'focb5': 'fe fe fe fe 68 20 df 68 02 02 38 3b 46 16',  # 05 08
}


if __name__ == '__main__':
    port = 'COM8'
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 9600
    ser.parity = serial.PARITY_EVEN
    ser.timeout = 0.5
    ser.open()

    while True:
        request = input('request> ').strip()
        if request:
            if request.startswith('!'):
                command = COMMON_COMMANDS[request[1:].strip()]
                print('command:', command)
            else:
                command = request
            send_command(ser, command)
