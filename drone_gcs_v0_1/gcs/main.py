import bisect
import csv
import math
import random
import sys
import time
from collections import deque

import pyqtgraph as pg
import serial
from serial.tools import list_ports
from PySide6.QtCore import QTimer, Qt
from PySide6.QtWidgets import (QAbstractItemView, QApplication, QCheckBox, QComboBox, QFileDialog,
    QGroupBox, QHeaderView, QHBoxLayout, QLabel, QMainWindow, QMessageBox,
    QPushButton, QSlider, QSpinBox, QTableWidget, QTableWidgetItem, QVBoxLayout, QWidget)

from .attitude import AttitudeIndicator
from .altitude import AltitudeIndicator
from .drone_model import DroneModel
from . import __version__
from .protocol import decode, encode, LineFramer, TELEMETRY_FIELDS
from .replay import load_recording

LINK_TIMEOUT = 1.0
HELLO_RETRY = 0.5
UNITS = dict(zip(TELEMETRY_FIELDS,
    ('°', '°', '°', 'm/s²', 'm/s²', 'm/s²', '°/s', '°/s', '°/s', 'Pa', '°C', 'cm')))


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle(f'Drone GCS v{__version__} — STM32F411')
        self.resize(1280, 960)
        self.port = None
        self.simulation = False
        self.armed = False
        self.motor_authorized = False
        self.estop_latched = False
        self.pending_arm = None
        self.last_rx = None
        self.last_telemetry = None
        self.last_heartbeat = 0.0
        self.link_timed_out = False
        self.seq = 0
        self.pending_arm_seq = None
        self.hello_seq = None
        self.last_hello = None
        self.handshake_started = None
        self.session_ready = False
        self.rx_sequence = None
        self.baro_reference_pa = None
        self.peer_bench = self.peer_configured = None
        self.last_control_reason = ""
        self.pending_motors = {}
        self.framer = LineFramer()
        self.packet_count = self.packet_errors = self.byte_count = 0
        self.telemetry_arrivals = deque(maxlen=1000)
        self.start_time = time.monotonic()
        self.history = {k: deque(maxlen=600) for k in ('roll', 'pitch', 'yaw')}
        self.times = deque(maxlen=600)
        self.csv_file = self.csv_writer = None
        self.recording = []
        self.recording_times = []
        self.replay_index = 0
        self.replay_playing = False
        self.replay_position = 0.0
        self.replay_anchor = 0.0
        self.build_ui()
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.tick)
        self.timer.start(50)
        self.refresh_ports()
        self.update_controls()

    def build_ui(self):
        root = QWidget()
        self.setCentralWidget(root)
        outer = QVBoxLayout(root)
        row = QHBoxLayout()
        self.version_label = QLabel(f'<b>Drone GCS v{__version__}</b>')
        row.addWidget(self.version_label)
        self.ports = QComboBox()
        refresh = QPushButton('Refresh ports')
        refresh.clicked.connect(self.refresh_ports)
        self.connect_button = QPushButton('Connect UART')
        self.connect_button.clicked.connect(self.toggle_connect)
        self.sim_button = QPushButton('Start simulation')
        self.sim_button.clicked.connect(self.toggle_sim)
        self.status = QLabel('DISCONNECTED')
        for widget in (self.ports, refresh, self.connect_button, self.sim_button, self.status):
            row.addWidget(widget)
        outer.addLayout(row)
        self.link_stats = QLabel('UART: no valid packets')
        outer.addWidget(self.link_stats)
        instruments = QHBoxLayout()
        self.attitude = AttitudeIndicator()
        self.altitude = AltitudeIndicator()
        for title, instrument in (('Attitude / Roll · Pitch', self.attitude),
                                  ('Altitude / Centimetres', self.altitude)):
            box = QGroupBox(title)
            layout = QVBoxLayout(box)
            layout.setContentsMargins(4, 6, 4, 4)
            layout.addWidget(instrument)
            instruments.addWidget(box, 1)
        sensors = QVBoxLayout()
        self.imu_panel = QGroupBox('MPU6050 / IMU')
        self.imu_panel.setObjectName('imuPanel')
        imu_layout = QVBoxLayout(self.imu_panel)
        self.labels = {}
        self.imu_table = self.sensor_table(('Parameter', 'X', 'Y', 'Z', 'Unit'), 2)
        for row, (title, fields) in enumerate((('Accel', ('ax', 'ay', 'az')),
                                               ('Gyro', ('gx', 'gy', 'gz')))):
            self.imu_table.setItem(row, 0, QTableWidgetItem(title))
            for column, name in enumerate(fields, 1):
                item = QTableWidgetItem('--')
                item.setData(Qt.ItemDataRole.AccessibleTextRole, f'IMU {name.upper()}')
                self.labels[name] = item
                self.imu_table.setItem(row, column, item)
            self.imu_table.setItem(row, 4, QTableWidgetItem(UNITS[fields[0]]))
        imu_layout.addWidget(self.imu_table)
        sensors.addWidget(self.imu_panel)

        self.baro_panel = QGroupBox('BMP280 / Barometer')
        self.baro_panel.setObjectName('baroPanel')
        baro_layout = QVBoxLayout(self.baro_panel)
        self.baro_table = self.sensor_table(('Parameter', 'Value', 'Unit'), 3)
        for row, (name, title) in enumerate((('pressure', 'Pressure'),
                                             ('temperature', 'Temperature'),
                                             ('altitude', 'Altitude'))):
            self.baro_table.setItem(row, 0, QTableWidgetItem(title))
            item = QTableWidgetItem('--')
            item.setData(Qt.ItemDataRole.AccessibleTextRole, f'Barometer {title}')
            self.labels[name] = item
            self.baro_table.setItem(row, 1, item)
            self.baro_table.setItem(row, 2, QTableWidgetItem(UNITS[name]))
        baro_layout.addWidget(self.baro_table)
        for table in (self.imu_table, self.baro_table):
            for row in range(table.rowCount()):
                for column in range(table.columnCount()):
                    table.item(row, column).setTextAlignment(Qt.AlignmentFlag.AlignCenter)
        sensors.addWidget(self.baro_panel)
        instruments.addLayout(sensors, 2)
        outer.addLayout(instruments)
        self.plot = pg.PlotWidget(title='Attitude')
        self.plot.setLabel('left', 'Angle', units='°')
        self.plot.setLabel('bottom', 'Time', units='s')
        self.plot.getAxis('bottom').enableAutoSIPrefix(False)
        self.plot.addLegend()
        self.plot.showGrid(x=True, y=True, alpha=0.2)
        self.curves = {name: self.plot.plot(pen=pg.mkPen(color, width=2), name=name)
                       for name, color in [('roll', '#3b82f6'), ('pitch', '#10b981'), ('yaw', '#f59e0b')]}
        visual_row = QHBoxLayout()
        model_box = QGroupBox('3D Drone / Received attitude')
        model_layout = QVBoxLayout(model_box)
        model_layout.setContentsMargins(4, 6, 4, 4)
        self.drone_model = DroneModel()
        model_layout.addWidget(self.drone_model)
        visual_row.addWidget(model_box, 1)
        visual_row.addWidget(self.plot, 2)
        outer.addLayout(visual_row, stretch=1)
        replay_box = QGroupBox('CSV playback')
        replay_layout = QHBoxLayout(replay_box)
        load_button = QPushButton('Open CSV')
        load_button.clicked.connect(self.choose_recording)
        self.play_button = QPushButton('Play')
        self.play_button.clicked.connect(self.toggle_playback)
        self.replay_slider = QSlider(Qt.Orientation.Horizontal)
        self.replay_slider.valueChanged.connect(self.seek_replay)
        self.replay_label = QLabel('No recording loaded')
        self.exit_replay_button = QPushButton('Close replay')
        self.exit_replay_button.clicked.connect(self.exit_replay)
        for widget in (load_button, self.play_button, self.replay_slider,
                       self.replay_label, self.exit_replay_button):
            replay_layout.addWidget(widget)
        outer.addWidget(replay_box)
        motor_box = QGroupBox('ESC PWM requests — BENCH TEST ONLY / REMOVE PROPELLERS')
        motor_layout = QVBoxLayout(motor_box)
        self.sliders, self.motor_values = [], []
        self.motor_inputs, self.motor_send_buttons = [], []
        motor_help = QLabel('Set a 1000–2000 µs preset at any time. Before ARM, values stay local and are NOT sent. After ARM ACK, changes send immediately; Send PWM sends the preset.')
        motor_help.setWordWrap(True)
        motor_layout.addWidget(motor_help)
        self.motor_gate = QLabel()
        self.motor_gate.setWordWrap(True)
        motor_layout.addWidget(self.motor_gate)
        for i in range(4):
            line = QHBoxLayout()
            line.addWidget(QLabel(f'Motor {i + 1}'))
            slider = QSlider(Qt.Orientation.Horizontal)
            slider.setRange(1000, 2000)
            slider.setSingleStep(10)
            slider.setValue(1000)
            slider.valueChanged.connect(lambda value, index=i: self.motor_changed(index, value))
            pulse = QSpinBox()
            pulse.setRange(1000, 2000)
            pulse.setSuffix(' µs')
            pulse.setSingleStep(1)
            pulse.setKeyboardTracking(False)
            pulse.setValue(1000)
            pulse.setAccessibleName(f'Motor {i + 1} PWM in microseconds')
            pulse.valueChanged.connect(slider.setValue)
            send_button = QPushButton('Send PWM')
            send_button.clicked.connect(lambda checked=False, index=i: self.send_motor(index))
            value = QLabel('No PWM request')
            value.setMinimumWidth(155)
            self.sliders.append(slider)
            self.motor_values.append(value)
            self.motor_inputs.append(pulse)
            self.motor_send_buttons.append(send_button)
            line.addWidget(slider)
            line.addWidget(pulse)
            line.addWidget(send_button)
            line.addWidget(value)
            motor_layout.addLayout(line)
        outer.addWidget(motor_box)
        actions = QHBoxLayout()
        self.interlock = QCheckBox('Propellers REMOVED (bench only)')
        self.interlock.toggled.connect(self.interlock_changed)
        self.arm_button = QPushButton('Request ARM')
        self.arm_button.clicked.connect(self.request_arm)
        self.disarm_button = QPushButton('DISARM')
        self.disarm_button.clicked.connect(self.disarm)
        self.stop_button = QPushButton('EMERGENCY STOP')
        self.stop_button.clicked.connect(self.emergency_stop)
        self.log_button = QPushButton('Start CSV logging')
        self.log_button.clicked.connect(self.toggle_log)
        for widget in (self.interlock, self.arm_button, self.disarm_button,
                       self.stop_button, self.log_button):
            actions.addWidget(widget)
        outer.addLayout(actions)
        self.notice = QLabel('PWM requires a live link, PB0 to GND, the bench checkbox and an acknowledged ARM.')
        self.notice.setWordWrap(True)
        outer.addWidget(self.notice)

    @staticmethod
    def sensor_table(headers, rows):
        table = QTableWidget(rows, len(headers))
        table.setHorizontalHeaderLabels(headers)
        table.verticalHeader().hide()
        table.verticalHeader().setDefaultSectionSize(30)
        table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        table.horizontalHeader().setMinimumSectionSize(42)
        table.horizontalHeader().setSectionsClickable(False)
        table.horizontalHeader().setDefaultAlignment(Qt.AlignmentFlag.AlignCenter)
        table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
        table.setAlternatingRowColors(True)
        table.setShowGrid(True)
        table.setSortingEnabled(False)
        # Fit the rows without leaving a large empty table viewport.
        height = table.horizontalHeader().sizeHint().height() + rows * 30 + 4
        table.setMinimumHeight(height)
        table.setMaximumHeight(height)
        return table

    def refresh_ports(self):
        previous = self.ports.currentText()
        self.ports.clear()
        self.ports.addItems([p.device for p in list_ports.comports()])
        index = self.ports.findText(previous)
        if index >= 0:
            self.ports.setCurrentIndex(index)

    def link_fresh(self):
        return self.port is not None and self.last_rx is not None and time.monotonic() - self.last_rx <= LINK_TIMEOUT

    def update_controls(self):
        live = self.simulation or (self.link_fresh() and self.session_ready)
        can_motor = self.can_send_motor()
        can_edit = not self.recording and not self.estop_latched
        for slider, pulse, send_button in zip(self.sliders, self.motor_inputs, self.motor_send_buttons):
            slider.setEnabled(bool(can_edit))
            pulse.setEnabled(bool(can_edit))
            send_button.setEnabled(bool(can_motor))
        self.arm_button.setEnabled(bool(live and self.interlock.isChecked() and
            not self.armed and self.pending_arm is None and not self.estop_latched and not self.recording
            and (self.simulation or (self.peer_bench is not False and self.peer_configured is not False))))
        self.disarm_button.setEnabled(bool(self.port or self.simulation))
        self.stop_button.setEnabled(bool(self.port or self.simulation))
        self.ports.setEnabled(self.port is None)
        self.log_button.setEnabled(bool((self.port or self.simulation) and not self.recording))
        for widget in (self.play_button, self.replay_slider, self.exit_replay_button):
            widget.setEnabled(bool(self.recording))
        self.play_button.setText('Pause' if self.replay_playing else 'Play')
        if self.recording:
            state = 'REPLAY / ' + ('PLAYING' if self.replay_playing else 'PAUSED')
        elif self.simulation:
            state = 'SIMULATION / ' + ('ESTOP' if self.estop_latched else 'ARMED' if self.armed else 'DISARMED')
        elif self.port:
            state = ('UART / TIMEOUT' if self.link_timed_out
                     else 'UART / ESTOP' if self.estop_latched
                     else 'UART / WAITING FOR DATA' if not self.link_fresh()
                     else 'UART / WAITING FOR ACK' if not self.session_ready
                     else 'UART / ARM PENDING' if self.pending_arm is not None
                     else 'UART / ARMED' if self.armed else 'UART / DISARMED')
        else:
            state = 'DISCONNECTED'
        if self.recording:
            gate = 'CSV replay: motor controls disabled'
        elif self.estop_latched:
            gate = 'E-stop latched: reset MCU and reconnect'
        elif self.port is None and not self.simulation:
            gate = 'Connect UART first'
        elif not self.simulation and not self.link_fresh():
            gate = 'Waiting for valid MCU packets'
        elif not self.simulation and not self.session_ready:
            gate = 'Waiting for HELLO ACK; motor commands are locked'
        elif not self.simulation and self.peer_bench is False:
            gate = 'PB0 is OPEN: connect PB0 to GND'
        elif not self.simulation and self.peer_configured is False:
            gate = 'Firmware ESC limits are not configured'
        elif not self.interlock.isChecked():
            gate = 'Tick Propellers REMOVED after removing the propellers'
        elif self.pending_arm is not None:
            gate = 'Waiting for ARM acknowledgement'
        elif self.armed and not self.motor_authorized:
            gate = 'Press DISARM, then request ARM again'
        elif not self.armed:
            gate = 'Press Request ARM to enable PWM'
        else:
            gate = 'PWM enabled; changes send immediately (firmware ramp applies)'
        suffix = (' | Last firmware: ' + self.last_control_reason) if self.last_control_reason else ''
        self.motor_gate.setText('PWM: ' + gate + suffix)
        self.status.setText(state)
        self.drone_model.set_state(state)

    def clear_history(self):
        self.times.clear()
        for name, values in self.history.items():
            values.clear()
            self.curves[name].setData([], [])

    def clear_instruments(self):
        self.attitude.clear()
        self.altitude.clear()
        self.drone_model.clear()

    def reset_source(self):
        self.clear_history()
        self.clear_instruments()
        for name, label in self.labels.items():
            label.setText('--')
        self.start_time = time.monotonic()
        self.last_rx = None
        self.last_telemetry = None
        self.last_heartbeat = 0.0
        self.link_timed_out = False
        self.seq = 0
        self.pending_arm_seq = None
        self.hello_seq = None
        self.last_hello = None
        self.handshake_started = None
        self.session_ready = False
        self.rx_sequence = None
        self.baro_reference_pa = None
        self.peer_bench = self.peer_configured = None
        self.last_control_reason = ""
        self.pending_motors = {}
        self.framer = LineFramer()
        self.packet_count = self.packet_errors = self.byte_count = 0
        self.telemetry_arrivals.clear()
        self.armed = self.motor_authorized = self.estop_latched = False
        self.pending_arm = None
        self.interlock.setChecked(False)
        self.reset_sliders()
        self.link_stats.setText('UART: no valid packets')

    def toggle_connect(self):
        if self.port is not None:
            self.disconnect_port()
            return
        name = self.ports.currentText()
        if not name:
            QMessageBox.warning(self, 'Serial', 'No serial port found')
            return
        try:
            port = serial.Serial(name, 115200, timeout=0, write_timeout=0.1)
        except (serial.SerialException, OSError) as exc:
            QMessageBox.critical(self, 'Serial error', str(exc))
            return
        self.stop_source()
        self.reset_source()
        self.port = port
        self.connected_at = time.monotonic()
        self.connect_button.setText('Disconnect UART')
        self.request_hello()
        self.update_controls()

    def request_hello(self, now=None):
        if self.port is None:
            return
        now = time.monotonic() if now is None else now
        was_active = self.armed or self.motor_authorized or self.pending_arm is not None
        self.session_ready = False
        self.armed = self.motor_authorized = False
        self.pending_arm = self.pending_arm_seq = None
        if was_active:
            self.reset_sliders()
        if self.handshake_started is None:
            self.handshake_started = now
        self.last_hello = now
        self.hello_seq = (self.seq + 1) & 0xffffffff
        self.send({'type': 'hello'})

    def disconnect_port(self, notify=True):
        # Detach before attempting I/O: a failed DISARM must not recurse into send().
        port, self.port = self.port, None
        self.armed = self.motor_authorized = False
        self.pending_arm = self.pending_arm_seq = None
        self.reset_sliders()
        if port is not None:
            try:
                if notify:
                    self.seq = (self.seq + 1) & 0xffffffff
                    port.write(encode({'type': 'disarm'}, self.seq))
            except (serial.SerialException, OSError):
                pass
            finally:
                try:
                    port.close()
                except (serial.SerialException, OSError):
                    pass
            self.clear_instruments()
            self.close_log()
            self.link_stats.setText('UART disconnected • displayed sensor values are historical')
        self.connect_button.setText('Connect UART')
        self.update_controls()

    def stop_source(self):
        self.disconnect_port()
        self.simulation = False
        self.sim_button.setText('Start simulation')
        self.recording = []
        self.recording_times = []
        self.replay_playing = False
        self.replay_label.setText('No recording loaded')
        self.close_log()

    def toggle_sim(self):
        was_simulation = self.simulation
        self.stop_source()
        self.reset_source()
        if not was_simulation:
            self.simulation = True
            self.sim_button.setText('Stop simulation')
        self.update_controls()

    def send(self, message):
        if self.port is None:
            return False
        try:
            self.seq = (self.seq + 1) & 0xffffffff
            packet = encode(message, self.seq)
            if self.port.write(packet) != len(packet):
                raise serial.SerialException('Incomplete serial write')
            return True
        except (serial.SerialException, OSError, ValueError) as exc:
            self.notice.setText(f'TX failed: {exc}')
            self.disconnect_port(notify=False)
            return False

    def request_arm(self):
        if (not self.interlock.isChecked() or self.estop_latched or self.recording
                or self.armed or self.pending_arm is not None
                or (not self.simulation and (self.peer_bench is False or self.peer_configured is False))):
            return
        if self.simulation:
            self.armed = self.motor_authorized = True
        elif self.link_fresh() and self.session_ready:
            self.pending_arm = time.monotonic()
            self.pending_arm_seq = (self.seq + 1) & 0xffffffff
            if self.send({'type': 'arm', 'bench': True}):
                self.notice.setText('ARM requested; waiting for firmware status ACK')
        self.update_controls()

    def disarm(self):
        self.armed = self.motor_authorized = False
        self.pending_arm = self.pending_arm_seq = None
        self.reset_sliders()
        if self.port is not None:
            self.send({'type': 'disarm'})
        self.update_controls()

    def emergency_stop(self):
        self.estop_latched = True
        self.armed = self.motor_authorized = False
        self.pending_arm = self.pending_arm_seq = None
        self.reset_sliders()
        if self.port is not None:
            self.send({'type': 'estop'})
        self.notice.setText('Emergency stop requested. Verify firmware state. Firmware reset required to clear latch.')
        self.update_controls()

    def interlock_changed(self, checked):
        if not checked and (self.armed or self.pending_arm is not None):
            self.disarm()
        self.update_controls()

    def reset_sliders(self):
        self.pending_motors.clear()
        for slider, pulse, label in zip(self.sliders, self.motor_inputs, self.motor_values):
            for widget in (slider, pulse):
                widget.blockSignals(True)
                widget.setValue(1000)
                widget.blockSignals(False)
            label.setText('No PWM request')

    def can_send_motor(self):
        return (self.armed and self.motor_authorized and self.interlock.isChecked()
                and not self.estop_latched and not self.recording and
                (self.simulation or (self.link_fresh() and self.session_ready
                 and self.peer_bench is not False and self.peer_configured is not False)))

    def motor_changed(self, index, value):
        pulse = self.motor_inputs[index]
        pulse.blockSignals(True)
        pulse.setValue(value)
        pulse.blockSignals(False)
        if self.can_send_motor():
            self.send_motor(index)
        else:
            self.motor_values[index].setText(f'Preset {value} µs — NOT SENT')
            self.update_controls()

    def send_motor(self, index):
        if not self.can_send_motor():
            value = self.motor_inputs[index].value()
            self.motor_values[index].setText(f'Preset {value} µs — NOT SENT')
            self.update_controls()
            return
        value = self.motor_inputs[index].value()
        if self.simulation:
            self.motor_values[index].setText(f'SIM {value} µs')
        else:
            self.pending_motors[index] = (self.seq + 1) & 0xffffffff
            if not self.send({'type': 'motor', 'id': index + 1, 'us': value}):
                self.pending_motors.pop(index, None)
                return
            self.motor_values[index].setText(f'UART sent {value} µs')

    def receive(self, message):
        now = time.monotonic()
        # UART authorization and liveness require checked v2 frames.
        if self.port is not None:
            if message.get('_version') != 2:
                return
            sequence = message['_seq']
            if self.rx_sequence is not None:
                delta = (sequence - self.rx_sequence) & 0xffffffff
                if delta == 0 or delta >= 0x80000000:
                    return
            self.rx_sequence = sequence
        self.last_rx = now
        self.link_timed_out = False
        self.packet_count += 1
        kind = message['type']
        was_active = self.armed or self.motor_authorized
        if kind == 'ack':
            if message.get('estop'):
                self.estop_latched = True
            command = message['command']
            if command == 'hello' and message['command_seq'] == self.hello_seq:
                self.session_ready = message['ok']
                self.hello_seq = None
                if self.session_ready:
                    self.handshake_started = None
                    self.send({'type': 'heartbeat'})
                    self.last_heartbeat = now
            elif (command == 'arm' and self.pending_arm is not None and
                  message['command_seq'] == self.pending_arm_seq):
                authorized = (now - self.pending_arm <= LINK_TIMEOUT and message['ok'] and
                              message['armed'] and not self.estop_latched and
                              self.session_ready and self.interlock.isChecked())
                self.pending_arm = self.pending_arm_seq = None
                self.armed = self.motor_authorized = authorized
                if not authorized:
                    self.disarm()
            elif not message['armed']:
                self.armed = self.motor_authorized = False
                if was_active or self.estop_latched:
                    self.reset_sliders()
            if message.get('reason') == 'session_or_sequence' and self.session_ready:
                self.handshake_started = None
                self.request_hello(now)
            if command == 'motor':
                index = message['id'] - 1
                if self.pending_motors.get(index) == message['command_seq']:
                    self.pending_motors.pop(index, None)
                    text = (f"ACK driver {message['applied']} µs" if message['ok'] else
                            'Rejected: ' + message.get('reason', 'unknown'))
                    self.motor_values[index].setText(text)
            if command != 'heartbeat' or not message['ok']:
                self.last_control_reason = message.get('reason', command)
                self.notice.setText('Firmware ACK: ' + self.last_control_reason)
        elif kind == 'status':
            reason = message.get('reason', 'OK')
            self.peer_bench = message.get('bench', self.peer_bench)
            self.peer_configured = message.get('configured', self.peer_configured)
            self.estop_latched |= message.get('estop', False)
            self.armed = message['armed']
            if not self.armed or self.estop_latched:
                self.motor_authorized = False
                if was_active or self.estop_latched:
                    self.reset_sliders()
            if self.armed and self.motor_authorized and self.can_send_motor():
                for index, motor in enumerate(message.get('motors', [])):
                    self.motor_values[index].setText(
                        f"Req {motor['requested']} / driver {motor['applied']} µs")
            if reason != 'periodic':
                self.last_control_reason = reason
                self.notice.setText('Firmware status: ' + reason)
        elif kind == 'telemetry':
            self.last_telemetry = now
            self.telemetry_arrivals.append(now)
            self.update_telemetry(message)
        self.update_controls()

    def update_telemetry(self, data, sample_time=None):
        # Legacy firmware sends pressure but no altitude. Derive only for live
        # UART, preserving recorded/simulated/firmware-provided altitude values.
        self.altitude.reference_label = 'ALTITUDE · SOURCE REFERENCE'
        pressure = data.get('pressure')
        if (self.port is not None and not self.recording and not self.simulation
                and data.get('altitude') is None
                and data.get('health', {}).get('barometer') != 'fault'
                and type(pressure) in (int, float) and 30000 <= pressure <= 110000):
            if self.baro_reference_pa is None:
                self.baro_reference_pa = pressure
            data = dict(data, altitude=44330 * (1 - (pressure / self.baro_reference_pa) ** .1903))
            self.altitude.reference_label = 'RELATIVE BARO · SINCE CONNECT'
            self.altitude.setToolTip('Estimated from pressure; first valid reading on this UART connection is 0 m. '
                                     'This is not a ground-distance measurement.')
        else:
            self.altitude.setToolTip('Altitude uses the reference supplied by firmware or recording.')
        for name, label in self.labels.items():
            value = data[name]
            if value is not None and name == 'altitude':
                value *= 100
            label.setText('--' if value is None else f'{value:.2f}')
        if data['roll'] is not None and data['pitch'] is not None:
            self.attitude.set_attitude(data['roll'], data['pitch'], data['yaw'] or 0)
        else:
            self.attitude.clear()
        if data['altitude'] is not None:
            self.altitude.set_altitude(data['altitude'])
        else:
            self.altitude.clear()
        if all(data[name] is not None for name in ('roll', 'pitch')):
            self.drone_model.set_attitude(data['roll'], data['pitch'], data['yaw'])
        else:
            self.drone_model.clear()
        self.times.append(time.monotonic() - self.start_time if sample_time is None else sample_time)
        for name in self.history:
            self.history[name].append(float('nan') if data[name] is None else data[name])
            self.curves[name].setData(list(self.times), list(self.history[name]))
        if self.csv_writer:
            try:
                timestamp = self.log_epoch + time.monotonic() - self.log_anchor
                self.csv_writer.writerow([timestamp] + [data[k] for k in TELEMETRY_FIELDS])
                self.csv_file.flush()
            except (OSError, csv.Error) as exc:
                self.close_log()
                self.notice.setText(f'CSV logging failed: {exc}')

    def tick(self):
        now = time.monotonic()
        if self.recording:
            if self.replay_playing:
                position = min(self.recording_times[-1], self.replay_position + now - self.replay_anchor)
                index = max(0, bisect.bisect_right(self.recording_times, position) - 1)
                if index != self.replay_index:
                    self.show_replay(index)
                if position >= self.recording_times[-1]:
                    self.replay_position = position
                    self.replay_playing = False
            self.update_controls()
            return
        if self.simulation:
            t = now - self.start_time
            self.update_telemetry({'roll': 12 * math.sin(t), 'pitch': 8 * math.sin(t * .7), 'yaw': (t * 5) % 360,
                'ax': .2 * math.sin(t), 'ay': .1 * math.cos(t), 'az': 9.81,
                'gx': 12 * math.cos(t), 'gy': 5 * math.cos(t * .7), 'gz': 5,
                'pressure': 101325 + random.uniform(-5, 5), 'temperature': 27, 'altitude': 1.2})
            self.link_stats.setText('Simulation • 20 Hz target • software sensor data')
            return
        if self.port is None:
            return
        # Revoke motor authorization before processing delayed packets after a GUI stall.
        reference = self.last_rx if self.last_rx is not None else self.connected_at
        if now - reference > LINK_TIMEOUT and not self.link_timed_out:
            self.link_timed_out = True
            self.disarm()
            self.session_ready = False
            self.hello_seq = self.last_hello = self.handshake_started = None
            self.rx_sequence = None
            self.clear_instruments()
            self.notice.setText('No valid UART packets for 1 s; DISARM requested')
            if self.port is None:
                return
        if self.pending_arm is not None and now - self.pending_arm > LINK_TIMEOUT:
            self.disarm()
            self.notice.setText('ARM acknowledgement timeout; DISARM requested')
            if self.port is None:
                return
        try:
            data = self.port.read(min(self.port.in_waiting, 2048))
            self.byte_count += len(data)
            previous_errors = self.framer.errors
            lines = self.framer.feed(data)
            self.packet_errors += self.framer.errors - previous_errors
            for line in lines:
                try:
                    message = decode(line)
                except (ValueError, UnicodeError, OverflowError, RecursionError):
                    self.packet_errors += 1
                    continue
                self.receive(message)
        except (serial.SerialException, OSError) as exc:
            self.notice.setText(f'UART error: {exc}')
            self.disconnect_port(notify=False)
            return
        if not self.session_ready:
            if self.last_hello is None or now - self.last_hello >= HELLO_RETRY:
                self.request_hello(now)
                if self.port is None:
                    return
            if (self.handshake_started is not None and now - self.handshake_started >= LINK_TIMEOUT
                    and self.link_fresh() and not self.estop_latched):
                self.notice.setText('Telemetry received; waiting for HELLO ACK. '
                                    'Check adapter TX → PA3, RX → PA2 and common GND.')
        elif now - self.last_heartbeat >= .2:
            if not self.send({'type': 'heartbeat'}):
                return
            self.last_heartbeat = now
        while self.telemetry_arrivals and now - self.telemetry_arrivals[0] > 5:
            self.telemetry_arrivals.popleft()
        arrivals = self.telemetry_arrivals
        hz = (len(arrivals) - 1) / (arrivals[-1] - arrivals[0]) if len(arrivals) > 1 and arrivals[-1] > arrivals[0] else 0
        age = '--' if self.last_rx is None else f'{max(0, now - self.last_rx):.2f} s'
        sensor_age = '--' if self.last_telemetry is None else f'{max(0, now - self.last_telemetry):.2f} s'
        if self.last_telemetry is None or now - self.last_telemetry > LINK_TIMEOUT:
            self.clear_instruments()
        self.link_stats.setText(f'UART • telemetry {hz:.1f} Hz • valid {self.packet_count} • errors {self.packet_errors} • RX {self.byte_count} B • last packet {age} • sensor age {sensor_age}')
        self.update_controls()

    def choose_recording(self):
        path, _ = QFileDialog.getOpenFileName(self, 'Open telemetry recording', '', 'CSV (*.csv)')
        if path:
            try:
                self.open_recording(path)
            except (OSError, ValueError, csv.Error) as exc:
                QMessageBox.warning(self, 'CSV playback', str(exc))

    def open_recording(self, path):
        samples = load_recording(path)
        self.stop_source()
        self.reset_source()
        self.recording = samples
        self.recording_times = [sample[0] for sample in samples]
        self.replay_slider.blockSignals(True)
        self.replay_slider.setRange(0, len(samples) - 1)
        self.replay_slider.blockSignals(False)
        self.replay_position = 0.0
        self.rebuild_replay(0)
        self.notice.setText(f'Replay: {path}')
        self.link_stats.setText(f'CSV replay • {len(samples)} samples • duration {self.recording_times[-1]:.2f} s')
        self.update_controls()

    def show_replay(self, index):
        # Keep every visible sample, but repaint only once even after a large seek.
        for i in range(max(self.replay_index + 1, index - 599), index):
            timestamp, values = self.recording[i]
            self.times.append(timestamp)
            for name in self.history:
                self.history[name].append(values[name])
        timestamp, values = self.recording[index]
        self.update_telemetry(values, timestamp)
        self.replay_index = index
        self.replay_slider.blockSignals(True)
        self.replay_slider.setValue(index)
        self.replay_slider.blockSignals(False)
        self.replay_label.setText(f'{self.recording_times[index]:.2f} / {self.recording_times[-1]:.2f} s')

    def rebuild_replay(self, index):
        self.clear_history()
        self.replay_index = max(-1, index - 600)
        self.show_replay(index)

    def seek_replay(self, index):
        if not self.recording:
            return
        self.replay_position = self.recording_times[index]
        self.replay_anchor = time.monotonic()
        self.rebuild_replay(index)

    def toggle_playback(self):
        if not self.recording:
            return
        now = time.monotonic()
        if self.replay_playing:
            self.replay_position = min(self.recording_times[-1], self.replay_position + now - self.replay_anchor)
            self.replay_playing = False
        else:
            if self.replay_position >= self.recording_times[-1]:
                self.seek_replay(0)
            self.replay_anchor = now
            self.replay_playing = True
        self.update_controls()

    def exit_replay(self):
        self.stop_source()
        self.reset_source()
        self.update_controls()

    def close_log(self):
        stream, self.csv_file = self.csv_file, None
        self.csv_writer = None
        if stream:
            try:
                stream.close()
            except OSError as exc:
                self.notice.setText(f'CSV close failed: {exc}')
        self.log_button.setText('Start CSV logging')

    def toggle_log(self):
        if self.csv_file:
            self.close_log()
            self.notice.setText('CSV logging stopped')
            return
        if self.recording or not (self.port or self.simulation):
            return
        path, _ = QFileDialog.getSaveFileName(self, 'Save telemetry', 'telemetry.csv', 'CSV (*.csv)')
        if path:
            try:
                self.csv_file = open(path, 'w', newline='', encoding='utf-8')
                self.csv_writer = csv.writer(self.csv_file)
                self.csv_writer.writerow(['timestamp'] + list(TELEMETRY_FIELDS))
                self.log_epoch = time.time()
                self.log_anchor = time.monotonic()
            except (OSError, csv.Error) as exc:
                self.close_log()
                QMessageBox.warning(self, 'CSV logging', str(exc))
                return
            self.log_button.setText('Stop CSV logging')
            self.notice.setText(f'Logging: {path}')

    def closeEvent(self, event):
        self.timer.stop()
        self.stop_source()
        super().closeEvent(event)


def main():
    app = QApplication(sys.argv)
    app.setApplicationName('Drone GCS')
    app.setApplicationVersion(__version__)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
