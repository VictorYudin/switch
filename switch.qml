import QtQuick 2.0
import QtGraphicalEffects 1.0
import MySwitch 1.0

Item
{
    id: root
    width: 320
    height: 480
    state: "Menu"

    SwitchRender
    {
        id: game
        anchors.fill: parent
    }

    FastBlur
    {
        id: blur
        anchors.fill: parent
        source: game
        radius: 128

        Rectangle
        {
            color: Qt.rgba(0.1, 0.1, 0.1, 0.85)
            anchors.fill: parent
        }
    }

    Column
    {
        id: menu
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        spacing: root.height / 20

        Column
        {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 0

            Text
            {
                anchors.horizontalCenter: parent.horizontalCenter

                font.pointSize: 48
                text: "Switch"
                color: "white"
            }

            Text
            {
                anchors.horizontalCenter: parent.horizontalCenter

                font.pointSize: 12
                text: "From Victor Yudin"
                color: "white"
            }
        }

        Row
        {
            spacing: root.height / 40
            anchors.horizontalCenter: parent.horizontalCenter

            SwitchButton
            {
                width: root.width/6
                height: root.width/7
                onClicked: {
                    root.state = "Game"
                    game.newGame()
                }
                text: "New game"
            }

            SwitchButton
            {
                width: root.width/6
                height: root.width/7
                text: "About"
            }
        }
    }

    Connections
    {
        target: game
        onWinGame:
        {
            root.state = "Menu"
        }
    }

    states:
    [
        State
        {
            name: "Game"
            PropertyChanges { target: game; enabled: true }
            PropertyChanges { target: blur; opacity: 0 }
            PropertyChanges { target: menu; opacity: 0; enabled: false }
        },
        State
        {
            name: "Menu"
            PropertyChanges { target: game; enabled: false }
            PropertyChanges { target: blur; opacity: 1 }
            PropertyChanges { target: menu; opacity: 1; enabled: true }
        }
    ]

    transitions: Transition
    {
        PropertyAnimation { property: "opacity"; duration: 500 }
        ColorAnimation { duration: 500 }
    }
}
