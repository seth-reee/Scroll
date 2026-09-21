import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

ApplicationWindow {
    id: window
    width: 1080
    height: 720
    visible: true
    title: (document.modified ? "● " : "") + document.fileName + " — qOmaedit"
    color: omarchyTheme.background
    // Qt Quick Controls do not read Omarchy's colors.toml themselves. Supplying
    // the application palette makes their native/default styling follow the
    // same live palette as the editor surfaces below.
    palette {
        window: omarchyTheme.background
        windowText: omarchyTheme.foreground
        base: omarchyTheme.background
        alternateBase: omarchyTheme.panel
        text: omarchyTheme.foreground
        button: omarchyTheme.surface
        buttonText: omarchyTheme.foreground
        highlight: omarchyTheme.selection
        highlightedText: omarchyTheme.foreground
        toolTipBase: omarchyTheme.panel
        toolTipText: omarchyTheme.foreground
        placeholderText: omarchyTheme.mutedForeground
    }
    property bool lineWrapping: false
    property int pendingCloseTab: -1
    property bool closingWindow: false
    property bool syncingEditor: false

    onClosing: function(event) {
        for (let index = 0; index < document.tabs.length; ++index) {
            if (document.tabModified(index)) {
                event.accepted = false
                closingWindow = true
                document.currentIndex = index
                requestCloseTab(index)
                return
            }
        }
    }

    function continueClosing() {
        if (closingWindow) Qt.callLater(function() { window.close() })
    }

    function saveDocument() {
        if (!document.hasFile) saveDialog.open()
        else document.save()
    }

    function findNext() {
        if (findField.text.length === 0) return
        let start = editor.selectionEnd
        let position = editor.text.indexOf(findField.text, start)
        if (position < 0) position = editor.text.indexOf(findField.text, 0)
        if (position >= 0) {
            editor.select(position, position + findField.text.length)
            editor.forceActiveFocus()
        }
    }

    function replaceCurrent() {
        if (findField.text.length && editor.selectedText === findField.text) {
            const start = editor.selectionStart
            editor.remove(start, editor.selectionEnd)
            editor.insert(start, replaceField.text)
            editor.cursorPosition = start + replaceField.text.length
        }
        findNext()
    }

    function replaceAll() {
        if (!findField.text.length) return
        const replacement = editor.text.split(findField.text).join(replaceField.text)
        if (replacement === editor.text) return
        editor.remove(0, editor.length)
        editor.insert(0, replacement)
    }

    function toggleLineWrapping() {
        lineWrapping = !lineWrapping
        refreshEditorLayout()
    }

    function refreshEditorLayout() {
        const cursor = editor.cursorPosition
        const selectionStart = editor.selectionStart
        const selectionEnd = editor.selectionEnd
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

    function requestCloseTab(index) {
        if (document.tabModified(index)) {
            closeTabDialog.tabIndex = index
            closeTabDialog.open()
        } else {
            document.closeTab(index, true)
        }
    }

    function saveAndCloseTab(index) {
        document.currentIndex = index
        if (!document.hasFile) {
            pendingCloseTab = index
            closeTabDialog.close()
            saveDialog.open()
        } else if (document.save()) {
            document.closeTab(index, true)
            closeTabDialog.close()
            continueClosing()
        }
    }

    // Platform dialogs delegate to the system file picker rather than drawing a QML dialog.
    Platform.FileDialog {
        id: openDialog
        title: "Open script"
        fileMode: Platform.FileDialog.OpenFile
        nameFilters: ["All files (*)", "Script and text files (*.sh *.bash *.zsh *.fish *.py *.lua *.js *.ts *.json *.toml *.yaml *.yml *.txt)"]
        onAccepted: document.open(file)
    }
    Platform.FileDialog {
        id: saveDialog
        objectName: "saveDialog"
        title: "Save script"
        fileMode: Platform.FileDialog.SaveFile
        defaultSuffix: "txt"
        nameFilters: ["Text files (*.txt)", "All files (*)"]
        onAccepted: {
            if (!document.saveAs(file)) {
                window.pendingCloseTab = -1
                window.closingWindow = false
                return
            }
            if (window.pendingCloseTab >= 0) {
                document.closeTab(window.pendingCloseTab, true)
                window.pendingCloseTab = -1
                window.continueClosing()
            }
        }
        onRejected: { window.pendingCloseTab = -1; window.closingWindow = false }
    }

    header: Column {
        width: window.width
        height: toolBar.height + (tabBar.visible ? tabBar.height : 0)
        ToolBar {
            id: toolBar
            width: parent.width; height: 48
            background: Rectangle { color: omarchyTheme.panel }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 8
                Label {
                    text: settings.tabsEnabled ? document.directoryPath : document.filePath
                    color: omarchyTheme.mutedForeground
                    Layout.fillWidth: true
                }
                ToolButton {
                    id: menuButton
                    text: "Menu"
                    onClicked: applicationMenu.popup(menuButton, 0, menuButton.height)
                }
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
                    id: tabButton
                    required property var modelData
                    required property int index
                    text: ""
                    implicitWidth: Math.min(tabTitle.implicitWidth, 180) + closeButton.implicitWidth + 30
                    onClicked: document.currentIndex = index
                    Label {
                        id: tabTitle
                        text: (tabButton.modelData.modified ? "● " : "") + tabButton.modelData.title
                        color: omarchyTheme.foreground
                        width: Math.min(implicitWidth, 180)
                        elide: Text.ElideRight
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    ToolButton {
                        id: closeButton
                        text: "×"
                        z: 2
                        anchors.left: tabTitle.right
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: window.requestCloseTab(tabButton.index)
                        ToolTip.visible: hovered
                        ToolTip.text: "Close tab"
                    }
                }
            }
        }
    }

    Shortcut { sequences: [StandardKey.Open]; onActivated: openDialog.open() }
    Shortcut { sequences: [StandardKey.Save]; onActivated: window.saveDocument() }
    Shortcut { sequences: [StandardKey.New]; onActivated: document.newFile() }
    Shortcut { sequences: [StandardKey.Find]; onActivated: findDialog.open() }

    Menu {
        id: applicationMenu
        MenuItem { text: "New"; onTriggered: document.newFile() }
        MenuItem { text: "Open…"; onTriggered: openDialog.open() }
        MenuItem { text: "Save"; onTriggered: window.saveDocument() }
        MenuItem { text: "Find and replace…"; onTriggered: findDialog.open() }
        MenuSeparator {}
        MenuItem { text: "About qOmaedit"; onTriggered: aboutDialog.open() }
    }

    Rectangle {
        anchors.fill: parent; anchors.margins: 16
        color: omarchyTheme.panel
        clip: true
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
                    x: 0; y: 12 + model.lineTop - editorScroll.contentY
                    width: gutter.width - 10; height: model.lineHeight
                    // Wrapped visual rows deliberately have no number: one physical line, one number.
                    text: model.number === 0 ? "" : model.number
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignTop
                    color: omarchyTheme.mutedForeground; font: editor.font
                }
            }
        }

        // Soft wraps have no newline in the document. This guide appears only before
        // continuation rows, so it distinguishes them without adding real line spacing.
        Repeater {
            model: lineNumberModel
            Rectangle {
                visible: window.lineWrapping && settings.wrappedLineSpacing && model.number === 0
                x: gutter.width + 12
                y: 12 + model.lineTop - editorScroll.contentY
                width: parent.width - x - 12
                height: 2
                color: omarchyTheme.surface
                opacity: 0.9
            }
        }

        Flickable {
            id: editorScroll
            objectName: "editorScroll"
            anchors.fill: parent; anchors.leftMargin: gutter.width + 12; anchors.rightMargin: 12; anchors.topMargin: 12; anchors.bottomMargin: 12
            clip: true
            interactive: true
            flickableDirection: Flickable.AutoFlickIfNeeded
            onContentYChanged: lineNumberModel.setViewport(contentY, height)
            onHeightChanged: lineNumberModel.setViewport(contentY, height)
            Component.onCompleted: lineNumberModel.setViewport(contentY, height)
            ScrollBar.vertical: ScrollBar {
                id: verticalScrollBar
                policy: ScrollBar.AsNeeded
                interactive: true
                height: editorScroll.height - (horizontalScrollBar.visible ? horizontalScrollBar.height : 0)
            }
            ScrollBar.horizontal: ScrollBar {
                id: horizontalScrollBar
                policy: window.lineWrapping ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                interactive: true
                width: editorScroll.width - (verticalScrollBar.visible ? verticalScrollBar.width : 0)
            }

            // Attach the editor so Qt tracks the viewport and reveals the cursor.
            TextArea.flickable: TextArea {
                id: editor
                objectName: "editor"
                textFormat: TextEdit.PlainText
                persistentSelection: true
                onTextChanged: {
                    if (!window.syncingEditor) {
                        // Guard the return signal instead of serializing and comparing
                        // the whole editor text again for our own update.
                        window.syncingEditor = true
                        document.text = text
                        window.syncingEditor = false
                    }
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

        // Breeze positions attached bars independently, so cover their shared corner
        // with the same neutral block a conventional scroll viewport uses.
        Rectangle {
            z: 3
            width: verticalScrollBar.width
            height: horizontalScrollBar.height
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            visible: verticalScrollBar.visible && horizontalScrollBar.visible
            color: omarchyTheme.panel
        }
    }

    footer: Rectangle {
        height: 30; color: omarchyTheme.panel
        RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16
            Button { text: "Settings"; onClicked: settingsDialog.open() }
            Label { text: document.encoding; color: omarchyTheme.mutedForeground; font.pixelSize: 12 }
            Label { visible: settings.syntaxEnabled; text: document.language; color: omarchyTheme.mutedForeground; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            Label { text: lineNumberModel.lineCount + " lines · " + editor.length + " chars"; color: omarchyTheme.mutedForeground; font.pixelSize: 12 }
            Button {
                text: window.lineWrapping ? "Wrap: On" : "Wrap: Off"
                onClicked: window.toggleLineWrapping()
            }
        }
    }

    Connections {
        target: document
        function onError(message) { errorDialog.text = message; errorDialog.open() }
        function onTextChanged() {
            if (window.syncingEditor) return
            window.syncingEditor = true
            editor.text = document.text
            window.syncingEditor = false
        }
    }
    Connections {
        target: settings
        function onSyntaxEnabledChanged() { syntaxHighlighter.setEnabled(settings.syntaxEnabled) }
        function onWrappedLineSpacingChanged() { lineNumberModel.scheduleRefresh() }
    }

    Dialog {
        id: settingsDialog; title: "Settings"; modal: true; standardButtons: Dialog.Close
        width: 320
        background: Rectangle { color: omarchyTheme.panel; border.color: omarchyTheme.surface; radius: 8 }
        ColumnLayout {
            width: parent.width; spacing: 8
            Label { text: "Editor"; color: omarchyTheme.foreground; font.bold: true }
            CheckBox { text: "Show tabs"; checked: settings.tabsEnabled; onToggled: settings.tabsEnabled = checked }
            CheckBox { text: "Syntax highlighting"; checked: settings.syntaxEnabled; onToggled: settings.syntaxEnabled = checked }
            CheckBox {
                text: "Mark wrapped continuation lines"
                checked: settings.wrappedLineSpacing
                onToggled: settings.wrappedLineSpacing = checked
            }
            Label {
                text: "Disabling syntax highlighting also hides the detected-language label."
                color: omarchyTheme.mutedForeground; wrapMode: Text.Wrap; Layout.fillWidth: true
            }
        }
    }

    Dialog {
        id: aboutDialog
        title: "About qOmaedit"
        modal: true
        standardButtons: Dialog.Close
        width: 380
        background: Rectangle { color: omarchyTheme.panel; border.color: omarchyTheme.surface; radius: 8 }
        ColumnLayout {
            width: parent.width; spacing: 8
            Label { text: "qOmaedit"; color: omarchyTheme.foreground; font.bold: true; font.pixelSize: 20 }
            Label { text: "Version " + applicationVersion; color: omarchyTheme.mutedForeground }
            Text {
                text: "<a href=\"https://github.com/seth-reee/qOmaedit\">github.com/seth-reee/qOmaedit</a>"
                textFormat: Text.RichText
                color: omarchyTheme.accent
                onLinkActivated: Qt.openUrlExternally(link)
            }
            Label { text: "Licensed under the MIT License."; color: omarchyTheme.mutedForeground }
        }
    }

    Dialog {
        id: closeTabDialog
        objectName: "closeTabDialog"
        closePolicy: Popup.NoAutoClose
        property int tabIndex: -1
        title: "Unsaved changes"
        modal: true
        standardButtons: Dialog.NoButton
        width: 380
        background: Rectangle { color: omarchyTheme.panel; border.color: omarchyTheme.surface; radius: 8 }
        ColumnLayout {
            width: parent.width; spacing: 12
            Label {
                text: "This tab has unsaved changes."
                color: omarchyTheme.foreground; wrapMode: Text.Wrap; Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                Button { text: "Cancel"; onClicked: { window.closingWindow = false; closeTabDialog.close() } }
                Item { Layout.fillWidth: true }
                Button {
                    text: "Discard"
                    onClicked: { document.closeTab(closeTabDialog.tabIndex, true); closeTabDialog.close(); window.continueClosing() }
                }
                Button { text: "Save"; onClicked: window.saveAndCloseTab(closeTabDialog.tabIndex) }
            }
        }
    }

    Dialog {
        id: findDialog; title: "Find and replace"; modal: false; standardButtons: Dialog.Close
        width: 380
        background: Rectangle { color: omarchyTheme.panel; border.color: omarchyTheme.surface; radius: 8 }
        ColumnLayout {
            width: parent.width; spacing: 10
            TextField { id: findField; objectName: "findField"; Layout.fillWidth: true; placeholderText: "Find"; color: omarchyTheme.foreground; selectByMouse: true; onAccepted: window.findNext() }
            TextField { id: replaceField; objectName: "replaceField"; Layout.fillWidth: true; placeholderText: "Replace with"; color: omarchyTheme.foreground; selectByMouse: true; onAccepted: window.replaceCurrent() }
            RowLayout {
                Layout.fillWidth: true
                Button { text: "Find next"; onClicked: window.findNext() }
                Button { text: "Replace"; onClicked: window.replaceCurrent() }
                Button { text: "Replace all"; onClicked: window.replaceAll() }
            }
        }
        onOpened: findField.forceActiveFocus()
    }
    Dialog { id: errorDialog; width: 360; property alias text: errorLabel.text; title: "Couldn’t read or write file"; modal: true; standardButtons: Dialog.Ok
        Label { id: errorLabel; color: omarchyTheme.foreground; wrapMode: Text.Wrap; width: 320 }
    }
}
