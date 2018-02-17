import QtQuick 2.0
import QtGraphicalEffects 1.0
import MySwitch 1.0

Item
{
    id: root
    width: 320
    height: 480
    state: "Menu"
    focus: true

    property date previousTime: new Date()

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
            // (.322, .38, .424)(.15) + (r, g, b)(.85) = (.1137, .1529, .1843)
            // color: Qt.rgba(0.0769, 0.1128, 0.142, 0.85)
            // (.322, .38, .424)(.15) + (r, g, b)(.85) = (.169, .22, .255)
            color: Qt.rgba(0.142, 0.192, 0.225, 0.85)
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
                onClicked:
                {
                    root.state = "Game"
                    game.newGame()
                    root.previousTime = new Date()
                    stopwatch.start()
                    timer.text = "Go go go"
                }
                text: "New game"
            }

            SwitchButton
            {
                id: aboutButton
                width: root.width/6
                height: root.width/7
                onClicked: {
                    if (root.state != "ShowInfo")
                    {
                        root.state = "ShowInfo"
                    }
                    else
                    {
                        root.state = "Menu"
                    }
                }
                text: "About"

                Text
                {
                    id: copyright
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: font.pointSize * 10
                    color: "white"
                    font.pointSize: 10
                    text: "// Copyright (c) Victor Yudin. All rights reserved."
                    opacity: 0
                }
            }
        }
    }

    Text
    {
        id: timer
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: font.pointSize
        color: "white"
        opacity: 0
        text: ""
    }

    Timer
    {
        id: stopwatch

        interval:  1000
        repeat:  true
        running: false
        triggeredOnStart: true

        function zeroPad(n)
        {
            return (n < 10 ? "0" : "") + n
        }

        function toTime(usec)
        {
            var mod = Math.abs(usec)
            return (mod >= 3600000 ? Math.floor(mod / 3600000) + ':' : '') +
                zeroPad(Math.floor((mod % 3600000) / 60000)) + ':' +
                zeroPad(Math.floor((mod % 60000) / 1000))
        }

        onTriggered:
        {
            var currentTime = new Date()
            var delta = (currentTime.getTime() - root.previousTime.getTime())
            timer.text = toTime(delta)
        }

    }

    Connections
    {
        target: game
        onWinGame:
        {
            root.state = "Win"
            stopwatch.stop()
        }
    }

    Keys.onPressed:
    {
        if (event.key == Qt.Key_Escape)
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
            PropertyChanges { target: timer; opacity: 1; font.pointSize: 15 }
        },
        State
        {
            name: "Menu"
            PropertyChanges { target: game; enabled: false }
            PropertyChanges { target: blur; opacity: 1 }
            PropertyChanges { target: menu; opacity: 1; enabled: true }
            PropertyChanges { target: timer; opacity: 0; font.pointSize: 30 }
        },
        State
        {
            name: "Win"
            PropertyChanges { target: game; enabled: false }
            PropertyChanges { target: blur; opacity: 1 }
            PropertyChanges { target: menu; opacity: 1; enabled: true }
            PropertyChanges { target: timer; opacity: 1; font.pointSize: 30 }
        },
        State
        {
            name: "ShowInfo"
            PropertyChanges { target: aboutButton; width: 2 * root.width / 3 }
            PropertyChanges { target: aboutButton; height: 2 * root.height / 4 }
            PropertyChanges { target: copyright; opacity: 1 }
            PropertyChanges { target: timer; opacity: 0 }
        }
    ]

    transitions: Transition
    {
        PropertyAnimation { property: "opacity"; duration: 500 }
        PropertyAnimation { property: "width"; duration: 500 }
        PropertyAnimation { property: "height"; duration: 500 }
        PropertyAnimation { property: "font.pointSize"; duration: 500 }
        ColorAnimation { duration: 500 }
    }
}
