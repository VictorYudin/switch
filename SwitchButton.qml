import QtQuick 2.7

Item
{
    id: root

    property string text: "Button"

    signal clicked()

    Item
    {
        id: shadow
        anchors.fill: parent

        property real glowRadius: 2.0
        property real cornerRadius: rect.radius + glowRadius
        property real spread: 0.0
        property color color: Qt.rgba(0, 0, 0, 0.75)

        ShaderEffect
        {
            id: shaderItem

            x: (parent.width - width) / 2.0
            y: (parent.height - height) / 2.0
            width: parent.width + shadow.glowRadius * 2 + cornerRadius * 2
            height: parent.height + shadow.glowRadius * 2 + cornerRadius * 2

            function clampedCornerRadius()
            {
                var maxCornerRadius = Math.min(shadow.width, shadow.height) / 2 + shadow.glowRadius;
                return Math.max(0, Math.min(shadow.cornerRadius, maxCornerRadius))
            }

            property color color: shadow.color
            property real inverseSpread: 1.0 - shadow.spread
            property real relativeSizeX: ((inverseSpread * inverseSpread) * shadow.glowRadius + cornerRadius * 2.0) / width
            property real relativeSizeY: relativeSizeX * (width / height)
            property real relativeRectSizeX: (shadow.glowRadius + cornerRadius) / width
            property real relativeRectSizeY: (shadow.glowRadius + cornerRadius) / height
            property real relativeSizeR: rect.radius / width
            property real spread: shadow.spread / 2.0
            property real cornerRadius: clampedCornerRadius()

            fragmentShader: "
                uniform highp float qt_Opacity;
                uniform mediump float relativeSizeX;
                uniform mediump float relativeSizeY;
                uniform mediump float relativeRectSizeX;
                uniform mediump float relativeRectSizeY;
                uniform mediump float relativeSizeR;
                uniform mediump float spread;
                uniform lowp vec4 color;
                varying highp vec2 qt_TexCoord0;

                highp float linearStep(
                    highp float e0, highp float e1, highp float x)
                {
                    return clamp((x - e0) / (e1 - e0), 0.0, 1.0);
                }

                highp float roundBox(
                    highp vec2 p, highp vec2 b, highp float r)
                {
                    return length(max(abs(p) - b, 0.0)) - r;
                }

                void main()
                {
                    lowp float alpha =
                        smoothstep(
                            0.0,
                            relativeSizeX,
                            0.5 - abs(0.5 - qt_TexCoord0.x)) *
                        smoothstep(
                            0.0,
                            relativeSizeY,
                            0.5 - abs(0.5 - qt_TexCoord0.y));

                    highp float rect =
                        smoothstep(
                            0.0,
                            0.001,
                            roundBox(
                                vec2(
                                    qt_TexCoord0.x - 0.5, qt_TexCoord0.y - 0.5),
                                vec2(
                                    0.5 - relativeRectSizeX - relativeSizeR,
                                    0.5 - relativeRectSizeY - relativeSizeR),
                                relativeSizeR));

                    highp float spreadMultiplier =
                        linearStep(spread, 1.0 - spread, alpha);
                    gl_FragColor =
                        color *
                        qt_Opacity *
                        spreadMultiplier *
                        spreadMultiplier *
                        spreadMultiplier *
                        rect;
                }
            "
        }

        states:
        [
            State
            {
                name: "Selected"
                when: mouseArea.containsMouse
                PropertyChanges { target: shadow; glowRadius: 15 }
            }
        ]

        transitions: Transition
        {
            PropertyAnimation { property: "glowRadius"; duration: 100 }
        }
    }

    Rectangle
    {
        id: rect
        color: Qt.rgba(1, 1, 1, 0.1)
        radius: 4.0
        anchors.fill: parent

        border.color: Qt.rgba(0.4, 0.4, 0.4, 1)
        border.width: 0.5

        Text
        {
            anchors.centerIn: parent
            color: "white"
            font.pointSize: 12
            text: parent.parent.text
        }
    }

    MouseArea
    {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    states:
    [
        State
        {
            name: "Pressed"
            when: mouseArea.pressed
            PropertyChanges { target: rect; color: Qt.rgba(0.5, 0.5, 0.5, 0.1) }
        }
    ]

    transitions: Transition
    {
        ColorAnimation { duration: 50 }
    }
}
