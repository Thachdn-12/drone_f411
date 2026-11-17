"""Version 2 JSON-lines frames; CRC covers the exact bytes before the crc field."""
import binascii
import json
import math

PROTOCOL_VERSION = 2
MAX_LINE = 1024
TELEMETRY_FIELDS = ('roll', 'pitch', 'yaw', 'ax', 'ay', 'az', 'gx', 'gy', 'gz',
                    'pressure', 'temperature', 'altitude')
RANGES = {'roll': (-180, 180), 'pitch': (-90, 90), 'yaw': (-360, 360),
          **dict.fromkeys(('ax', 'ay', 'az'), (-156.91, 156.91)),
          **dict.fromkeys(('gx', 'gy', 'gz'), (-2000, 2000)),
          'pressure': (30000, 110000), 'temperature': (-40, 85), 'altitude': (-20000, 20000)}


class LineFramer:
    def __init__(self):
        self.buffer = bytearray()
        self.discarding = False
        self.errors = 0

    def feed(self, data):
        lines = []
        for byte in data:
            if byte == 10:
                if not self.discarding and self.buffer:
                    lines.append(bytes(self.buffer))
                self.buffer.clear()
                self.discarding = False
            elif not self.discarding:
                if len(self.buffer) >= MAX_LINE - 1:
                    self.buffer.clear()
                    self.discarding = True
                    self.errors += 1
                else:
                    self.buffer.append(byte)
        return lines


def encode(message, seq=None):
    payload = json.dumps(message, separators=(',', ':'), allow_nan=False).encode('ascii')
    if seq is None:  # Read-only compatibility tools may still inspect v1 data.
        data = payload + b'\n'
    else:
        if type(seq) is not int or not 0 <= seq <= 0xffffffff:
            raise ValueError('Invalid sequence')
        prefix = b'{"v":2,"seq":' + str(seq).encode() + b',"payload":' + payload
        data = prefix + b',"crc":' + str(binascii.crc_hqx(prefix, 0xffff)).encode() + b'}\n'
    if len(data) > MAX_LINE:
        raise ValueError('Packet too long')
    return data


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Duplicate JSON key')
        result[key] = value
    return result


def number(value, low, high):
    try:
        return type(value) in (int, float) and math.isfinite(value) and low <= value <= high
    except OverflowError:
        return False


def validate_telemetry(message):
    for name, (low, high) in RANGES.items():
        if name not in message or (message[name] is not None and not number(message[name], low, high)):
            raise ValueError(f'Invalid/out-of-range telemetry: {name}')
    for variant in ('raw', 'filtered'):
        if variant in message:
            if not isinstance(message[variant], dict):
                raise ValueError('Invalid sensor variant')
            for name, value in message[variant].items():
                if name not in RANGES or not number(value, *RANGES[name]):
                    raise ValueError(f'Invalid {variant} field: {name}')
    if 'health' in message:
        health = message['health']
        if not isinstance(health, dict):
            raise ValueError('Invalid sensor health')
        allowed = {'imu': ('unknown', 'ok', 'fault'), 'barometer': ('unknown', 'ok', 'fault'),
                   'calibration': ('unknown', 'needed', 'running', 'calibrated')}
        if any(key not in allowed or value not in allowed[key] for key, value in health.items()):
            raise ValueError('Invalid sensor health state')


def decode(line):
    if len(line) > MAX_LINE:
        raise ValueError('Packet too long')
    wire = line.rstrip(b'\r\n')
    message = json.loads(wire.decode('utf-8'), object_pairs_hook=unique_object)
    if not isinstance(message, dict):
        raise ValueError('Expected JSON object')
    version, seq = 1, None
    if 'v' in message:
        if (set(message) != {'v', 'seq', 'payload', 'crc'} or type(message['v']) is not int
                or message['v'] != PROTOCOL_VERSION or type(message['seq']) is not int
                or not 0 <= message['seq'] <= 0xffffffff or type(message['crc']) is not int):
            raise ValueError('Invalid protocol envelope')
        prefix, separator, tail = wire.rpartition(b',"crc":')
        if not separator or binascii.crc_hqx(prefix, 0xffff) != message['crc']:
            raise ValueError('CRC mismatch')
        version, seq = message['v'], message['seq']
        message = message['payload']
        if not isinstance(message, dict):
            raise ValueError('Invalid payload')
    kind = message.get('type')
    if kind == 'telemetry':
        validate_telemetry(message)
    elif kind in ('status', 'ack'):
        if type(message.get('armed')) is not bool:
            raise ValueError('Invalid armed state')
        if not isinstance(message.get('reason', ''), str):
            raise ValueError('Invalid reason')
        for flag in ('bench', 'configured', 'outputs_known'):
            if flag in message and type(message[flag]) is not bool:
                raise ValueError('Invalid firmware gate flag')
        if 'estop' in message and type(message['estop']) is not bool:
            raise ValueError('Invalid estop state')
        if kind == 'ack':
            if (type(message.get('ok')) is not bool or type(message.get('command_seq')) is not int
                    or not 0 <= message['command_seq'] <= 0xffffffff
                    or message.get('command') not in ('hello', 'heartbeat', 'arm', 'disarm', 'estop', 'motor')):
                raise ValueError('Invalid command acknowledgement')
            if message['command'] == 'motor':
                if not number(message.get('id'), 1, 4) or type(message['id']) is not int:
                    raise ValueError('Invalid motor acknowledgement')
                if not number(message.get('applied'), 0, 2000) or type(message['applied']) is not int:
                    raise ValueError('Invalid applied PWM')
        if 'motors' in message:
            if not isinstance(message['motors'], list) or len(message['motors']) != 4:
                raise ValueError('Invalid motor states')
            for motor in message['motors']:
                if not isinstance(motor, dict):
                    raise ValueError('Invalid motor state')
                if (type(motor.get('applied')) is not int or not 0 <= motor['applied'] <= 2000
                        or type(motor.get('requested')) is not int or not 0 <= motor['requested'] <= 2000):
                    raise ValueError('Invalid motor pulse')
        if 'min_us' in message or 'max_us' in message:
            if (type(message.get('min_us')) is not int or type(message.get('max_us')) is not int
                    or not 1000 <= message['min_us'] <= message['max_us'] <= 2000):
                raise ValueError('Invalid ESC limits')
    else:
        raise ValueError('Unknown packet type')
    if version == 2:
        message['_version'], message['_seq'] = version, seq
    return message
