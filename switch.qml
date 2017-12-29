import QtQuick 2.0
import QtGraphicalEffects 1.0
import MySwitch 1.0

Item
{
    width: 320
    height: 480

    Item
    {
        id: mygame
        anchors.fill: parent

        SwitchRender
        {
            id: game
            anchors.fill: parent
        }


        Column
        {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter

            spacing: parent.height / 50

            Rectangle
            {
                color: Qt.rgba(1, 1, 1, 0.9)
                radius: 10.0
                width: parent.parent.width/3
                height: parent.parent.width/6

                Text
                {
                    anchors.centerIn: parent
                    font.pointSize: 24
                    text: "New game"
                }
            }

            Rectangle
            {
                color: Qt.rgba(1, 1, 1, 0.9)
                radius: 10.0
                width: parent.parent.width/3
                height: parent.parent.width/6

                Text
                {
                    anchors.centerIn: parent
                    font.pointSize: 24
                    text: "Continue"
                }
            }

            Rectangle
            {
                color: Qt.rgba(1, 1, 1, 0.9)
                radius: 10.0
                width: parent.parent.width/3
                height: parent.parent.width/6

                Text
                {
                    anchors.centerIn: parent
                    font.pointSize: 24
                    text: "About"
                }
            }
        }
    }

    Rectangle {
        id: rect
        color: Qt.rgba(1, 1, 1, 0.7)
        radius: 10
        border.width: 1
        border.color: "white"
        anchors.fill: label
        anchors.margins: -10

        MouseArea {
            anchors.fill: parent
            onPressed: {
                game.newGame()
            }
        }
    }

    Text {
        id: label
        color: "black"
        wrapMode: Text.WordWrap
        text: "New Game"
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }

    FastBlur
    {
        anchors.fill: parent
        source: mygame
        radius: 32
    }

    Connections {
        target: game
        onWinGame: {
            rect.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1);
        }
    }
}
