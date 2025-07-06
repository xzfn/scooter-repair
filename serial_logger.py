
import time
import datetime
import serial


class SerialLogger:
    def __init__(self, port):
        self.ser = serial.Serial()
        self.ser.port = port
        self.ser.timeout = 0.1
        self.ser.open()

        self.start_time = time.time()
        time_str = datetime.datetime.fromtimestamp(self.start_time).strftime('%Y%m%d-%H%M%S')
        filename = 'logs/serial-{}.txt'.format(time_str)
        self.file_output = open(filename, 'w')

    def run(self):
        ser = self.ser
        while True:
            waiting = ser.in_waiting
            if waiting > 0:
                data = ser.read(ser.in_waiting)
                if data:
                    now = time.time()
                    self.write_line(data, now)
            time.sleep(0.01)

    def write_line(self, data, now):
        msg = '{}: {}\n'.format(now - self.start_time, data.hex(' '))
        self.file_output.write(msg)
        print(msg, end='')

    def close(self):
        self.ser.close()
        self.file_output.close()


def main():
    logger = SerialLogger('COM5')
    try:
        logger.run()
    except KeyboardInterrupt:
        print('KeyboardInterrupt, closing...')
        logger.close()


if __name__ == '__main__':
    main()
