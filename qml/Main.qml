import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

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

    function toggleLineWrapping() {
        const cursor = editor.cursorPosition
        const selectionStart = editor.selectionStart
        const selectionEnd = editor.selectionEnd
        lineWrapping = !lineWrapping

        // TextArea can defer reflow of an existing QTextDocument until it receives
        // an edit or selection event. Toggle a transient selection on the next frame
        // to invalidate that layout without changing the file's text.
        Qt.callLater(function() {
            editor.selectAll()
            editor.deselect()
            editor.cursorPosition = cursor
            if (selectionStart !== selectionEnd)
                editor.select(selectionStart, selectionEnd)
        })
    }

    // Platform dialogs delegate to the system file picker rather than drawing a QML dialog.
    Platform.FileDialog {
        id: openDialog
        title: "Open script"
        fileMode: Platform.FileDialog.OpenFile
        nameFilters: ["Script and text files (*.sh *.bash *.zsh *.fish *.py *.lua *.js *.ts *.json *.toml *.yaml *.yml *.txt)", "All files (*)"]
        onAccepted: document.open(file)
    }
    Platform.FileDialog {
        id: saveDialog
        title: "Save script"
        fileMode: Platform.FileDialog.SaveFile
        defaultSuffix: "txt"
        nameFilters: ["Text files (*.txt)", "All files (*)"]
        onAccepted: document.saveAs(file)
    }

    header: Column {
        width: window.width
        implicitHeight: toolBar.height + (tabBar.visible ? tabBar.height : 0)
        ToolBar {
            id: toolBar
            width: parent.width; height: 48
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
        TabBar {
            id: tabBar
            width: parent.width; height: 34
            visible: settings.tabsEnabled
            currentIndex: document.currentIndex
            background: Rectangle { color: omarchyTheme.background }
            Repeater {
                model: document.tabs
                TabButton {
                    required property var modelData
                    text: (modelData.modified ? "● " : "") + modelData.title
                    onClicked: document.currentIndex = index
                }
            }
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
            Repeater {
                model: lineNumberModel
                Label {
                    x: 0; y: 12 + model.lineTop - editorScroll.contentItem.contentY
                    width: gutter.width - 10; height: model.lineHeight
                    // Wrapped visual rows deliberately have no number: one physical line, one number.
                    text: model.number === 0 ? "" : model.number
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignTop
                    color: omarchyTheme.mutedForeground; font: editor.font
                }
            }
        }

        ScrollView {
            id: editorScroll
            anchors.fill: parent; anchors.leftMargin: gutter.width + 12; anchors.rightMargin: 12; anchors.topMargin: 12; anchors.bottomMargin: 12
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                interactive: true
            }
            ScrollBar.horizontal: ScrollBar {
                policy: window.lineWrapping ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                interactive: true
            }

            TextArea {
                id: editor
                // ScrollView owns the viewport; TextArea grows to its content inside it.
                width: window.lineWrapping ? editorScroll.availableWidth : Math.max(editorScroll.availableWidth, implicitWidth)
                height: Math.max(editorScroll.availableHeight, implicitHeight)
                text: document.text
                onTextChanged: {
                    if (activeFocus && text !== document.text) document.text = text
                    lineNumberModel.scheduleRefresh()
                }
                onWidthChanged: lineNumberModel.scheduleRefresh()
                onWrapModeChanged: lineNumberModel.scheduleRefresh()
                font.family: "monospace"; font.pixelSize: 15
                color: omarchyTheme.foreground
                selectionColor: omarchyTheme.selection
                selectedTextColor: omarchyTheme.foreground
                // WrapAnywhere is visual only: no newline is inserted into the document.
                // It also wraps long unbroken script lines, unlike WordWrap.
                wrapMode: window.lineWrapping ? TextEdit.WrapAnywhere : TextEdit.NoWrap
                background: Rectangle { color: "transparent" }
                focus: true
                Component.onCompleted: {
                    syntaxHighlighter.setEditorDocument(textDocument)
                    syntaxHighlighter.setEnabled(settings.syntaxEnabled)
                    lineNumberModel.setEditorDocument(textDocument)
                    lineNumberModel.scheduleRefresh()
                }
            }
        }
    }

    footer: Rectangle {
        height: 30; color: omarchyTheme.panel
        RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16
            Button { text: "Settings"; onClicked: settingsDialog.open() }
            Label { text: document.encoding; color: omarchyTheme.mutedForeground; font.pixelSize: 12 }
            Label { visible: settings.syntaxEnabled; text: document.language; color: omarchyTheme.mutedForeground; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            Label { text: document.text.split("\n").length + " lines · " + editor.cursorPosition + " chars"; color: omarchyTheme.mutedForeground; font.pixelSize: 12 }
            Button {
                text: window.lineWrapping ? "Wrap: On" : "Wrap: Off"
                onClicked: window.toggleLineWrapping()
            }
        }
    }

    Connections { target: document; function onError(message) { errorDialog.text = message; errorDialog.open() } }
    Connections { target: settings; function onSyntaxEnabledChanged() { syntaxHighlighter.setEnabled(settings.syntaxEnabled) } }

    Dialog {
        id: settingsDialog; title: "Settings"; modal: true; standardButtons: Dialog.Close
        width: 320
        background: Rectangle { color: omarchyTheme.panel; border.color: omarchyTheme.surface; radius: 8 }
        ColumnLayout {
            width: parent.width; spacing: 8
            Label { text: "Editor"; color: omarchyTheme.foreground; font.bold: true }
            CheckBox { text: "Show tabs"; checked: settings.tabsEnabled; onToggled: settings.tabsEnabled = checked }
            CheckBox { text: "Syntax highlighting"; checked: settings.syntaxEnabled; onToggled: settings.syntaxEnabled = checked }
            Label {
                text: "Disabling syntax highlighting also hides the detected-language label."
                color: omarchyTheme.mutedForeground; wrapMode: Text.Wrap; Layout.fillWidth: true
            }
        }
    }

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
