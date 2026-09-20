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

    function saveDocument() {
        if (document.fileName === "Untitled") saveDialog.open()
        else document.save()
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
        }
    }

    Shortcut { sequence: StandardKey.Open; onActivated: openDialog.open() }
    Shortcut { sequence: StandardKey.Save; onActivated: window.saveDocument() }
    Shortcut { sequence: StandardKey.New; onActivated: document.newFile() }

    Rectangle {
        anchors.fill: parent; anchors.margins: 16
        color: omarchyTheme.panel
        radius: 8
        border.color: omarchyTheme.surface
        border.width: 1

        TextArea {
            id: editor
            anchors.fill: parent; anchors.margins: 12
            text: document.text
            onTextChanged: if (activeFocus && text !== document.text) document.text = text
            font.family: "monospace"; font.pixelSize: 15
            color: omarchyTheme.foreground
            selectionColor: omarchyTheme.selection
            selectedTextColor: omarchyTheme.foreground
            wrapMode: TextArea.NoWrap
            background: Rectangle { color: "transparent" }
            focus: true
        }
    }

    footer: Rectangle {
        height: 30; color: omarchyTheme.panel
        RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16
            Label { text: omarchyTheme.name; color: omarchyTheme.accent; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            Label { text: editor.cursorPosition + " chars"; color: omarchyTheme.mutedForeground; font.pixelSize: 12 }
        }
    }

    Connections { target: document; function onError(message) { errorDialog.text = message; errorDialog.open() } }
    Dialog { id: errorDialog; property alias text: errorLabel.text; title: "Couldn’t read or write file"; modal: true; standardButtons: Dialog.Ok
        Label { id: errorLabel; color: omarchyTheme.foreground; wrapMode: Text.Wrap; width: 320 }
    }
}
