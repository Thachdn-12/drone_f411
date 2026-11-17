"""Projected 3D quadrotor drawn with Qt, without an OpenGL requirement.

Body frame: X forward, Y right, Z down. World frame: north/east/down.
Euler angles in degrees use Rz(yaw) * Ry(pitch) * Rx(roll).
"""
import math

from PySide6.QtCore import QPointF, QRectF, Qt
from PySide6.QtGui import QColor, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import QWidget


def rotate_body_point(point, roll, pitch, yaw):
    """Transform a body-frame point into world coordinates."""
    r, p, y = (math.radians(angle % 360) for angle in (roll, pitch, yaw))
    cr, sr, cp, sp, cy, sy = math.cos(r), math.sin(r), math.cos(p), math.sin(p), math.cos(y), math.sin(y)
    x, right, down = point
    right, down = cr * right - sr * down, sr * right + cr * down
    x, down = cp * x + sp * down, -sp * x + cp * down
    return cy * x - sy * right, sy * x + cy * right, down


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


class DroneModel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(320, 220)
        self.roll = self.pitch = self.yaw = 0.0
        self.available = False
        self.heading_known = False
        self.state = 'DISCONNECTED'
        self.setAccessibleName('3D drone attitude')
        self.setToolTip('Roll: right side down. Pitch: nose up. Yaw: clockwise from N. '
                        'Angles in degrees; front arms are cyan. With no yaw, roll/pitch use a fixed X/Y reference. '
                        'Rotor graphics do not represent motor RPM.')

    def set_attitude(self, roll, pitch, yaw):
        self.roll, self.pitch, self.yaw = roll, pitch, yaw
        self.heading_known = yaw is not None
        self.available = True
        self.update()

    def set_state(self, state):
        if state != self.state:
            self.state = state
            self.update()

    def clear(self):
        self.available = False
        self.heading_known = False
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.fillRect(self.rect(), QColor('#132031'))
        width, height = self.width(), self.height()
        center = QPointF(width / 2, 32 + (height - 84) / 2)
        scale = min((width - 24) / 4.8, (height - 84) / 3.5)
        # Fixed camera looks from above the front-right corner of the world axes.
        camera = (.565685, .565685, -.6)
        screen_right = (.707107, -.707107, 0)
        screen_up = (-.424264, -.424264, -.8)

        def project(point):
            perspective = 8 / (8 - dot(point, camera))
            return QPointF(center.x() + dot(point, screen_right) * scale * perspective,
                           center.y() - dot(point, screen_up) * scale * perspective)

        # Unknown heading uses a fixed drawing reference, explicitly labelled below.
        angles = (self.roll, self.pitch, self.yaw if self.heading_known else 0) if self.available else (0, 0, 0)

        def transform(point):
            return rotate_body_point(point, *angles)

        # The world grid and compass stay fixed while the vehicle rotates.
        painter.setPen(QPen(QColor('#263b50'), 1))
        for i in range(-3, 4):
            v = i * .5
            painter.drawLine(project((v, -1.5, .55)), project((v, 1.5, .55)))
            painter.drawLine(project((-1.5, v, .55)), project((1.5, v, .55)))
        for endpoint, text, color in (((1.9, 0, .55), 'N' if self.heading_known else 'X ref', '#94a3b8'),
                                      ((0, 1.9, .55), 'E' if self.heading_known else 'Y ref', '#94a3b8')):
            painter.setPen(QPen(QColor(color), 1))
            point = project(endpoint)
            painter.drawLine(project((0, 0, .55)), point)
            painter.drawText(QRectF(point.x() - 32, point.y() - 9, 64, 18), Qt.AlignmentFlag.AlignCenter, text)

        faces = []

        def face(points, color, outline='#93a9bf', line_width=1):
            world = [transform(point) for point in points]
            faces.append((sum(dot(p, camera) for p in world) / len(world),
                          QPolygonF([project(p) for p in world]), color, outline, line_width))

        def box(vertices, colors):
            for indices, color in zip(((0, 1, 2, 3), (4, 5, 6, 7), (0, 1, 5, 4),
                                       (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)), colors):
                face([vertices[i] for i in indices], color)

        for x, y in ((1.05, 1.05), (1.05, -1.05), (-1.05, 1.05), (-1.05, -1.05)):
            length = math.hypot(x, y)
            dx, dy = -y / length * .075, x / length * .075
            corners = [(dx, dy), (x + dx, y + dy), (x - dx, y - dy), (-dx, -dy)]
            vertices = [(px, py, z) for z in (-.055, .055) for px, py in corners]
            color = '#22b8cf' if x > 0 else '#657c93'
            box(vertices, (color, '#243b50', color, color, '#334e68', '#334e68'))
            ring = [(x + .36 * math.cos(i * math.tau / 40),
                     y + .36 * math.sin(i * math.tau / 40), -.11) for i in range(40)]
            face(ring, '#223f52' if x > 0 else '#263446',
                 '#5eead4' if x > 0 else '#94a3b8', 2)
            hub = [(x + .07 * math.cos(i * math.tau / 16),
                    y + .07 * math.sin(i * math.tau / 16), -.13) for i in range(16)]
            face(hub, '#cbd5e1')
            # Static blades show the rotor plane, not unmeasured motor speed.
            face([(x - .27, y - .035, -.12), (x + .27, y - .035, -.12),
                  (x + .27, y + .035, -.12), (x - .27, y + .035, -.12)], '#8295a9')

        vertices = [(x, y, z) for z in (-.16, .12)
                    for x, y in ((.48, -.28), (.48, .28), (-.48, .28), (-.48, -.28))]
        box(vertices, ('#e2e8f0', '#263b50', '#22b8cf', '#526a82', '#3d5268', '#526a82'))
        # Arrow on the top plate identifies the nose independently of yaw.
        face([(.40, 0, -.17), (.05, -.16, -.17), (.05, -.055, -.17),
              (-.25, -.055, -.17), (-.25, .055, -.17), (.05, .055, -.17),
              (.05, .16, -.17)], '#fbbf24', '#fbbf24')

        painter.save()
        if not self.available:
            painter.setOpacity(.3)
        for _, polygon, fill, outline, thickness in sorted(faces, key=lambda item: item[0]):
            painter.setBrush(QColor(fill))
            painter.setPen(QPen(QColor(outline), thickness))
            painter.drawPolygon(polygon)
        painter.restore()

        painter.setPen(QColor('#5eead4') if self.available else QColor('#94a3b8'))
        painter.drawText(QRectF(8, 5, width - 16, 23), Qt.AlignmentFlag.AlignCenter, self.state)
        painter.setPen(QColor('#e2e8f0'))
        heading = f'YAW {self.yaw % 360:05.1f}°' if self.heading_known else 'YAW UNAVAILABLE · ROLL/PITCH ONLY'
        text = (f'ROLL {self.roll:+.1f}°   PITCH {self.pitch:+.1f}°\n{heading}'
                if self.available else 'NO ATTITUDE DATA')
        painter.drawText(QRectF(4, height - 48, width - 8, 44), Qt.AlignmentFlag.AlignCenter, text)
