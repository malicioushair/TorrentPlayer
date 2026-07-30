import QtQuick
import QtQuick.Controls.Material

import "../colors.js" as Colors

Button {
    id: rootID

    property string iconName
    property string tooltipText
    property bool prominent: false

    implicitWidth: prominent ? 64 : 44
    implicitHeight: implicitWidth
    padding: 0

    flat: true
    hoverEnabled: true
    Accessible.name: tooltipText

    background: Rectangle {
        radius: width / 2
        color: rootID.prominent
            ? rootID.down
                ? Colors.PlayerControlButton.prominentPressed
                : rootID.hovered
                    ? Colors.PlayerControlButton.prominentHovered
                    : Colors.App.accent
            : rootID.down
                ? Colors.PlayerControlButton.pressed
                : rootID.hovered
                    ? Colors.PlayerControlButton.hovered
                    : Colors.App.transparent
        border.width: rootID.prominent ? 0 : 1
        border.color: rootID.hovered
            ? Colors.PlayerControlButton.hoverBorder
            : Colors.App.transparent

        Behavior on color {
            ColorAnimation {
                duration: 100
            }
        }
    }

    contentItem: Item {
        Canvas {
            id: iconCanvasID

            anchors.centerIn: parent

            width: 24
            height: 24

            onPaint: {
                const context = getContext("2d")
                context.clearRect(0, 0, width, height)
                context.strokeStyle = rootID.enabled
                    ? Colors.PlayerControlButton.icon
                    : Colors.PlayerControlButton.disabledIcon
                context.fillStyle = context.strokeStyle
                context.lineWidth = 1.8
                context.lineCap = "round"
                context.lineJoin = "round"

                switch (rootID.iconName) {
                case "play":
                    context.beginPath()
                    context.moveTo(8, 5)
                    context.lineTo(19, 12)
                    context.lineTo(8, 19)
                    context.closePath()
                    context.fill()
                    break
                case "pause":
                    context.fillRect(7, 5, 3.5, 14)
                    context.fillRect(13.5, 5, 3.5, 14)
                    break
                case "stop":
                    context.fillRect(7, 7, 10, 10)
                    break
                case "rewind":
                    context.beginPath()
                    context.moveTo(11, 6)
                    context.lineTo(4, 12)
                    context.lineTo(11, 18)
                    context.closePath()
                    context.moveTo(19, 6)
                    context.lineTo(12, 12)
                    context.lineTo(19, 18)
                    context.closePath()
                    context.fill()
                    break
                case "forward":
                    context.beginPath()
                    context.moveTo(5, 6)
                    context.lineTo(12, 12)
                    context.lineTo(5, 18)
                    context.closePath()
                    context.moveTo(13, 6)
                    context.lineTo(20, 12)
                    context.lineTo(13, 18)
                    context.closePath()
                    context.fill()
                    break
                case "volume":
                    context.beginPath()
                    context.moveTo(4, 10)
                    context.lineTo(8, 10)
                    context.lineTo(12, 6)
                    context.lineTo(12, 18)
                    context.lineTo(8, 14)
                    context.lineTo(4, 14)
                    context.closePath()
                    context.stroke()
                    context.beginPath()
                    context.arc(12, 12, 5, -0.75, 0.75)
                    context.stroke()
                    context.beginPath()
                    context.arc(12, 12, 8, -0.7, 0.7)
                    context.stroke()
                    break
                case "muted":
                    context.beginPath()
                    context.moveTo(4, 10)
                    context.lineTo(8, 10)
                    context.lineTo(12, 6)
                    context.lineTo(12, 18)
                    context.lineTo(8, 14)
                    context.lineTo(4, 14)
                    context.closePath()
                    context.stroke()
                    context.beginPath()
                    context.moveTo(16, 9)
                    context.lineTo(21, 14)
                    context.moveTo(21, 9)
                    context.lineTo(16, 14)
                    context.stroke()
                    break
                case "folder":
                    context.beginPath()
                    context.moveTo(3, 8)
                    context.lineTo(10, 8)
                    context.lineTo(12, 10)
                    context.lineTo(21, 10)
                    context.lineTo(21, 19)
                    context.lineTo(3, 19)
                    context.closePath()
                    context.moveTo(3, 8)
                    context.lineTo(3, 6)
                    context.lineTo(9, 6)
                    context.lineTo(11, 8)
                    context.stroke()
                    break
                case "settings":
                    context.beginPath()
                    context.moveTo(4, 7)
                    context.lineTo(20, 7)
                    context.moveTo(4, 12)
                    context.lineTo(20, 12)
                    context.moveTo(4, 17)
                    context.lineTo(20, 17)
                    context.stroke()
                    context.beginPath()
                    context.arc(9, 7, 2, 0, Math.PI * 2)
                    context.fill()
                    context.beginPath()
                    context.arc(15, 12, 2, 0, Math.PI * 2)
                    context.fill()
                    context.beginPath()
                    context.arc(11, 17, 2, 0, Math.PI * 2)
                    context.fill()
                    break
                case "fullscreen":
                    context.beginPath()
                    context.moveTo(9, 4)
                    context.lineTo(4, 4)
                    context.lineTo(4, 9)
                    context.moveTo(15, 4)
                    context.lineTo(20, 4)
                    context.lineTo(20, 9)
                    context.moveTo(20, 15)
                    context.lineTo(20, 20)
                    context.lineTo(15, 20)
                    context.moveTo(9, 20)
                    context.lineTo(4, 20)
                    context.lineTo(4, 15)
                    context.stroke()
                    break
                case "opened_fullscreen":
                    context.beginPath()
                    context.moveTo(4, 9)
                    context.lineTo(9, 9)
                    context.lineTo(9, 4)
                    context.moveTo(15, 4)
                    context.lineTo(15, 9)
                    context.lineTo(20, 9)
                    context.moveTo(20, 15)
                    context.lineTo(15, 15)
                    context.lineTo(15, 20)
                    context.moveTo(9, 20)
                    context.lineTo(9, 15)
                    context.lineTo(4, 15)
                    context.stroke()
                    break
                }
            }

            Connections {
                target: rootID

                function onIconNameChanged() {
                    iconCanvasID.requestPaint()
                }

                function onEnabledChanged() {
                    iconCanvasID.requestPaint()
                }
            }
        }
    }

    ToolTip.visible: hovered && tooltipText.length > 0
    ToolTip.delay: 500
    ToolTip.text: tooltipText
}
