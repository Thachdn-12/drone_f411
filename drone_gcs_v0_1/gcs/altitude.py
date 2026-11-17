"""Altitude displayed in centimetres; incoming/internal values remain metres."""
import math

from PySide6.QtCore import QPointF, QRectF, Qt
from PySide6.QtGui import QColor, QFont, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import QWidget


class AltitudeIndicator(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(240, 260)
        self.altitude = 0.0
        self.available = False
        self.reference_label = "ALTITUDE"
        self.setAccessibleName('Altitude in centimetres')

    @property
    def altitude_cm(self):
        return self.altitude * 100

    @property
    def display_text(self):
        if not self.available:
            return '-- cm'
        value = self.altitude_cm
        return (f'{value:.2f}' if abs(value) < 1e6 else f'{value:.2e}') + ' cm'

    def set_altitude(self, altitude):
        self.altitude = altitude
        self.available = True
        self.setAccessibleDescription(self.display_text)
        self.update()

    def clear(self):
        self.available = False
        self.setAccessibleDescription(self.display_text)
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        width, height = self.width(), self.height()
        painter.fillRect(self.rect(), QColor('#132031'))
        painter.setPen(QColor('#94a3b8'))
        painter.drawText(QRectF(0, 8, width, 20), Qt.AlignmentFlag.AlignCenter, self.reference_label)
        painter.save()
        font = QFont(self.font())
        font.setPointSize(24)
        font.setBold(True)
        painter.setFont(font)
        painter.setPen(QColor('#5eead4') if self.available else QColor('#94a3b8'))
        painter.drawText(QRectF(0, 31, width, 42), Qt.AlignmentFlag.AlignCenter,
                         self.display_text)
        painter.restore()

        tape = QRectF(width * .18, 85, width * .64, height - 122)
        center_y = tape.center().y()
        axis_x = tape.right() - 30
        painter.fillRect(tape, QColor('#1c2c40'))
        painter.setPen(QPen(QColor('#52657c'), 1))
        painter.drawLine(QPointF(axis_x, tape.top()), QPointF(axis_x, tape.bottom()))
        if self.available:
            painter.save()
            painter.setClipRect(tape.adjusted(0, 2, 0, -2))
            # Fifty centimetres per tick; larger heights scroll rather than saturate.
            tick_position = self.altitude_cm / 50
            base = math.floor(tick_position)
            fractional = tick_position - base
            visible_ticks = math.ceil(tape.height() / 40) + 1
            for offset in range(-visible_ticks, visible_ticks + 1):
                y = center_y - (offset - fractional) * 20
                centimetres = (base + offset) * 50
                major = (base + offset) % 2 == 0
                painter.setPen(QPen(QColor('#e2e8f0'), 2 if major else 1))
                painter.drawLine(QPointF(axis_x - (20 if major else 11), y), QPointF(axis_x, y))
                if major:
                    label = str(centimetres) if abs(centimetres) < 1e6 else f'{centimetres:.1e}'
                    painter.drawText(QRectF(tape.left() + 4, y - 10, tape.width() - 62, 20),
                                     Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter, label)
            painter.restore()
            painter.setPen(QPen(QColor('#ffd166'), 2))
            painter.drawLine(QPointF(tape.left() + 5, center_y), QPointF(axis_x + 4, center_y))
            painter.setPen(Qt.PenStyle.NoPen)
            painter.setBrush(QColor('#ffd166'))
            painter.drawPolygon(QPolygonF([QPointF(axis_x + 3, center_y),
                                           QPointF(axis_x + 16, center_y - 7),
                                           QPointF(axis_x + 16, center_y + 7)]))
        painter.setPen(QColor('#94a3b8'))
        painter.drawText(QRectF(0, height - 29, width, 24), Qt.AlignmentFlag.AlignCenter,
                         'CENTIMETRES · 50 cm per tick' if self.available else 'ALTITUDE — NO DATA')
