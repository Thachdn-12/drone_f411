"""Read-only sensor/link diagnosis; sends DISARM, HELLO and heartbeat, never ARM/PWM."""
import argparse
import json
from pathlib import Path
import time

import serial

from .protocol import LineFramer, decode, encode


def diagnose(port, seconds=6, clock=time.monotonic, emit=lambda event: None):
    report = dict(rx_bytes=0, valid=0, malformed=0, hello_ack=False,
                  status=None, telemetry=None, reasons=[])
    framer = LineFramer()
    sequence = 0
    pending_hello = None
    start = clock()
    last_hello = last_heartbeat = start - 1

    def send(message):
        nonlocal sequence
        sequence = (sequence + 1) & 0xffffffff
        packet = encode(message, sequence)
        if port.write(packet) != len(packet):
            raise serial.SerialException('Incomplete UART write')
        emit(dict(direction='tx', sequence=sequence, payload=message))
        return sequence

    try:
        send({'type': 'disarm'})
        while clock() - start < seconds:
            now = clock()
            if not report['hello_ack'] and now - last_hello >= .5:
                pending_hello = send({'type': 'hello'})
                last_hello = now
            elif report['hello_ack'] and now - last_heartbeat >= .2:
                send({'type': 'heartbeat'})
                last_heartbeat = now
            data = port.read(min(max(port.in_waiting, 1), 4096))
            report['rx_bytes'] += len(data)
            previous_errors = framer.errors
            lines = framer.feed(data)
            report['malformed'] += framer.errors - previous_errors
            for line in lines:
                try:
                    packet = decode(line)
                except (ValueError, UnicodeError, OverflowError, RecursionError):
                    report['malformed'] += 1
                    emit(dict(direction='rx_invalid', raw=line[:160].decode('ascii', errors='replace')))
                    continue
                report['valid'] += 1
                emit(dict(direction='rx', payload=packet))
                kind = packet['type']
                reason = packet.get('reason')
                if reason and reason not in report['reasons']:
                    report['reasons'].append(reason)
                if kind == 'status':
                    report['status'] = packet
                elif kind == 'telemetry':
                    report['telemetry'] = packet
                elif (kind == 'ack' and packet.get('_version') == 2
                      and packet['command'] == 'hello'
                      and packet['command_seq'] == pending_hello):
                    report['hello_ack'] = packet['ok']
        report['result'] = ('handshake_ok' if report['hello_ack'] else
                            'receiving_without_hello_ack' if report['valid'] else
                            'invalid_data_only' if report['rx_bytes'] else 'no_rx_data')
        emit(dict(direction='summary', **report))
        return report
    finally:
        try:
            send({'type': 'disarm'})
        except (serial.SerialException, OSError):
            pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--seconds', type=float, default=6)
    parser.add_argument('--log', type=Path, default=Path('.test-output/gcs-uart-diagnose.jsonl'))
    args = parser.parse_args()
    if not 1 <= args.seconds <= 60:
        parser.error('--seconds must be between 1 and 60')
    print('Close the GCS and serial monitors first. Diagnosis sends DISARM and never arms motors.')
    args.log.parent.mkdir(parents=True, exist_ok=True)
    try:
        with args.log.open('w', encoding='utf-8') as stream:
            def emit(event):
                stream.write(json.dumps(event, ensure_ascii=False, allow_nan=False) + '\n')
                stream.flush()
            with serial.Serial(args.port, 115200, timeout=.02, write_timeout=.1) as port:
                report = diagnose(port, args.seconds, emit=emit)
    except (serial.SerialException, OSError) as exc:
        parser.exit(1, f'UART diagnosis failed: {exc}\n')
    print(json.dumps(report, indent=2, ensure_ascii=False))
    print(f'Log: {args.log.resolve()}')
    if not report['hello_ack']:
        parser.exit(2, 'No successful HELLO ACK. ARM/PWM remain locked; inspect the log before changing firmware.\n')


if __name__ == '__main__':
    main()
