"""Painter-based attitude indicator; no additional graphics dependencies."""
import math

from PySide6.QtCore import QPointF, QRectF, Qt
from PySide6.QtGui import QColor, QPainter, QPainterPath, QPen, QPolygonF
from PySide6.QtWidgets import QWidget


class AttitudeIndicator(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(280, 260)
        self.roll = self.pitch = self.yaw = 0.0
        self.available = False

    def set_attitude(self, roll, pitch, yaw):
        self.roll, self.pitch, self.yaw = roll, pitch, yaw
        self.available = True
        self.update()

    def clear(self):
        self.available = False
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        width, height = self.width(), self.height()
        painter.fillRect(self.rect(), QColor('#132031'))
        center = QPointF(width / 2, (height - 46) / 2)
        radius = min(width - 32, height - 70) / 2
        face = QRectF(center.x() - radius, center.y() - radius, radius * 2, radius * 2)
        clip = QPainterPath()
        clip.addEllipse(face)
        painter.save()
        painter.setClipPath(clip)
        painter.translate(center)
        painter.rotate(-self.roll if self.available else 0)
        # Keep the horizon centred: only roll rotates the circular face.
        extent = math.hypot(width, height)
        painter.fillRect(QRectF(-extent, -extent, extent * 2, extent), QColor('#2968a1'))
        painter.fillRect(QRectF(-extent, 0, extent * 2, extent), QColor('#865c3a'))
        painter.setPen(QPen(QColor('white'), 2))
        painter.drawLine(QPointF(-extent, 0), QPointF(extent, 0))
        painter.restore()

        # Fixed circular bezel and bank-angle marks remain outside the moving sky.
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.setPen(QPen(QColor('#52657c'), 5))
        painter.drawEllipse(face.adjusted(-3, -3, 3, 3))
        painter.save()
        painter.translate(center)
        painter.setPen(QPen(QColor('#e2e8f0'), 2))
        for angle in (-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60):
            painter.save()
            painter.rotate(angle)
            length = 12 if angle % 30 == 0 else 7
            painter.drawLine(QPointF(0, -radius + 4), QPointF(0, -radius + 4 + length))
            painter.restore()
        painter.save()
        painter.rotate(-self.roll if self.available else 0)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QColor('#ffd166'))
        painter.drawPolygon(QPolygonF([QPointF(0, -radius + 21),
                                       QPointF(-5, -radius + 30),
                                       QPointF(5, -radius + 30)]))
        painter.restore()
        painter.restore()

        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.setPen(QPen(QColor('#ffd166'), 3))
        for left, right in ((-.60, -.15), (.15, .60)):
            painter.drawLine(QPointF(center.x() + left * radius, center.y()),
                             QPointF(center.x() + right * radius, center.y()))
        painter.drawEllipse(center, 4, 4)
        painter.setPen(QColor('white'))
        text = (f'ROLL {self.roll:+.1f}°   PITCH {self.pitch:+.1f}°'
                if self.available else 'ATTITUDE — NO DATA')
        painter.drawText(QRectF(0, height - 43, width, 40), Qt.AlignmentFlag.AlignCenter, text)
