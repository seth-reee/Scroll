import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1080
    height: 720
    visible: true
    title: (document.modified ? "● " : "") + document.fileName + " — Qomaedit"
    color: omarchyTheme.background
    property bool lineWrapping: false

    function saveDocument() {
        if (document.fileName === "Untitled") saveDialog.open()
        else document.save()
    }

    function findNext() {
        if (findField.text.length === 0) return
        let start = editor.cursorPosition
        if (editor.selectedText === findField.text) start += findField.text.length
        let position = editor.text.indexOf(findField.text, start)
        if (position < 0) position = editor.text.indexOf(findField.text, 0)
        if (position >= 0) {
            editor.select(position, position + findField.text.length)
            editor.cursorPosition = position + findField.text.length
            editor.forceActiveFocus()
        }
    }

    function replaceCurrent() {
        if (editor.selectedText === findField.text) editor.insert(editor.selectionStart, replaceField.text)
        findNext()
    }

    function logicalLineAt(visualLine) {
        const item = editor.contentItem
        if (!item || !item.positionAt || editor.lineCount === 0) return visualLine + 1
        const visualHeight = item.contentHeight / editor.lineCount
        const position = item.positionAt(0, (visualLine + 0.5) * visualHeight)
        return editor.text.slice(0, Math.max(0, position)).split("\n").length
    }

    FileDialog { id: openDialog; title: "Open script"; onAccepted: document.open(selectedFile) }
    FileDialog { id: saveDialog; title: "Save script"; fileMode: FileDialog.SaveFile; onAccepted: document.saveAs(selectedFile) }

    header: ToolBar {
        height: 48
        background: Rectangle { color: omarchyTheme.panel }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 8
            Label { text: "Qomaedit"; color: omarchyTheme.foreground; font.bold: true; font.pixelSize: 16 }
            Label { text: " / " + document.fileName; color: omarchyTheme.mutedForeground; Layout.fillWidth: true }
            ToolButton { text: "New"; onClicked: document.newFile() }
            ToolButton { text: "Open"; onClicked: openDialog.open() }
            ToolButton { text: "Save"; onClicked: window.saveDocument() }
            ToolButton { text: "Find"; onClicked: findDialog.open() }
        }
    }

    Shortcut { sequence: StandardKey.Open; onActivated: openDialog.open() }
    Shortcut { sequence: StandardKey.Save; onActivated: window.saveDocument() }
    Shortcut { sequence: StandardKey.New; onActivated: document.newFile() }
    Shortcut { sequence: StandardKey.Find; onActivated: findDialog.open() }

    Rectangle {
        anchors.fill: parent; anchors.margins: 16
        color: omarchyTheme.panel
        radius: 8
        border.color: omarchyTheme.surface
        border.width: 1

        Rectangle {
            id: gutter
            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            anchors.margins: 1; width: 52; color: omarchyTheme.background; clip: true
            Column {
                y: 12 - editor.contentY
                width: parent.width
                Repeater {
                    model: editor.lineCount
                    Label {
                        required property int index
                        readonly property int logicalLine: window.logicalLineAt(index)
                        readonly property int previousLogicalLine: index > 0 ? window.logicalLineAt(index - 1) : 0
                        width: gutter.width - 10; height: editor.font.pixelSize * 1.45
                        // Wrapped visual rows deliberately have no number: one physical line, one number.
                        text: index === 0 || logicalLine !== previousLogicalLine ? logicalLine : ""
                        horizontalAlignment: Text.AlignRight
                        color: omarchyTheme.mutedForeground; font: editor.font
                    }
                }
            }
        }

        TextArea {
            id: editor
            anchors.fill: parent; anchors.leftMargin: gutter.width + 12; anchors.rightMargin: 12; anchors.topMargin: 12; anchors.bottomMargin: 12
            text: document.text
            onTextChanged: if (activeFocus && text !== document.text) document.text = text
            font.family: "monospace"; font.pixelSize: 15
            color: omarchyTheme.foreground
            selectionColor: omarchyTheme.selection
            selectedTextColor: omarchyTheme.foreground
            // These enums belong to TextEdit (the text item inside TextArea).
            // Using TextArea.Wrap can appear to toggle without changing layout at runtime.
            wrapMode: window.lineWrapping ? TextEdit.Wrap : TextEdit.NoWrap
            background: Rectangle { color: "transparent" }
            focus: true
            Component.onCompleted: syntaxHighlighter.setEditorDocument(textDocument)
        }
    }

    footer: Rectangle {
        height: 30; color: omarchyTheme.panel
        RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16
            Label { text: omarchyTheme.name; color: omarchyTheme.accent; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            Label { text: document.text.split("\n").length + " lines · " + editor.cursorPosition + " chars"; color: omarchyTheme.mutedForeground; font.pixelSize: 12 }
            Button {
                text: window.lineWrapping ? "Wrap: On" : "Wrap: Off"
                onClicked: window.lineWrapping = !window.lineWrapping
            }
        }
    }

    Connections { target: document; function onError(message) { errorDialog.text = message; errorDialog.open() } }

    Dialog {
        id: findDialog; title: "Find and replace"; modal: false; standardButtons: Dialog.Close
        width: 380
        background: Rectangle { color: omarchyTheme.panel; border.color: omarchyTheme.surface; radius: 8 }
        ColumnLayout {
            width: parent.width; spacing: 10
            TextField { id: findField; Layout.fillWidth: true; placeholderText: "Find"; color: omarchyTheme.foreground; selectByMouse: true; onAccepted: window.findNext() }
            TextField { id: replaceField; Layout.fillWidth: true; placeholderText: "Replace with"; color: omarchyTheme.foreground; selectByMouse: true; onAccepted: window.replaceCurrent() }
            RowLayout {
                Layout.fillWidth: true
                Button { text: "Find next"; onClicked: window.findNext() }
                Button { text: "Replace"; onClicked: window.replaceCurrent() }
                Button { text: "Replace all"; onClicked: { if (findField.text.length) editor.text = editor.text.split(findField.text).join(replaceField.text) } }
            }
        }
        onOpened: findField.forceActiveFocus()
    }
    Dialog { id: errorDialog; property alias text: errorLabel.text; title: "Couldn’t read or write file"; modal: true; standardButtons: Dialog.Ok
        Label { id: errorLabel; color: omarchyTheme.foreground; wrapMode: Text.Wrap; width: 320 }
    }
}
