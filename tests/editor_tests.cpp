#include "codeeditor.h"
#include "mainwindow.h"
#include "syntaxhighlighter.h"
#include "theme.h"
#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>

class TestWindow : public MainWindow {
public:
    QList<CloseChoice> choices;
    QUrl savePath;
    QString lastError;
    int prompts = 0;
protected:
    CloseChoice confirmClose(CodeEditor *) override {
        ++prompts;
        return choices.isEmpty() ? CloseChoice::Cancel : choices.takeFirst();
    }
    QUrl chooseSavePath(CodeEditor *) override { return savePath; }
    void showError(const QString &message) override { lastError = message; }
};

class EditorTests : public QObject {
    Q_OBJECT
    QTemporaryDir dir;
    QUrl file(const QString &name, const QByteArray &bytes) {
        QFile output(dir.filePath(name));
        if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size()) qFatal("Cannot write test file");
        return QUrl::fromLocalFile(output.fileName());
    }
private slots:
    void branding() {
        TestWindow window;
        QVERIFY(window.windowTitle().endsWith(" — Scroll"));
        bool foundAbout = false;
        for (const auto *action : window.actions())
            foundAbout |= action->text() == "About Scroll";
        QVERIFY(foundAbout);
    }

    void aboutLicenseBundled() {
        QFile license(":/LICENSE");
        QVERIFY(license.open(QIODevice::ReadOnly));
        QVERIFY(license.readAll().contains("MIT License"));
    }

    void fileSafety() {
        Theme theme;
        CodeEditor editor(&theme);
        editor.insertPlainText("keep me");
        QSignalSpy errors(&editor, &CodeEditor::error);
        QVERIFY(!editor.load(file("invalid", QByteArray("\xff", 1))));
        QVERIFY(!editor.load(file("binary", QByteArray("a\0b", 3))));
        QVERIFY(!editor.load(file("truncated-utf8", QByteArray("\xe2\x82", 2))));
        QVERIFY(!editor.load(QUrl("https://example.com/file")));
        QVERIFY(!editor.saveAs(QUrl::fromLocalFile(dir.filePath("missing/file"))));
        QCOMPARE(editor.toPlainText(), "keep me");
        QVERIFY(editor.document()->isModified());
        QVERIFY(editor.filePath().isEmpty());
        QCOMPARE(errors.count(), 5);
    }

    void roundTrip_data() {
        QTest::addColumn<QByteArray>("bytes");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("LF") << QByteArray("hello\nworld\n");
        QTest::newRow("no-final-newline") << QByteArray("hello\nworld");
        QTest::newRow("CRLF-BOM") << QByteArray("\xef\xbb\xbfhello\r\nworld\r\n");
        QTest::newRow("UTF8") << QString::fromUtf8("héllo 🌍\n").toUtf8();
        QTest::newRow("large") << QByteArray("const x = 123;\n").repeated(20000);
    }
    void roundTrip() {
        QFETCH(QByteArray, bytes);
        Theme theme;
        CodeEditor editor(&theme);
        const QUrl url = file("Untitled", bytes);
        QVERIFY(editor.load(url));
        QVERIFY(!editor.filePath().isEmpty());
        QVERIFY(editor.save());
        QFile saved(url.toLocalFile());
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), bytes);
        QVERIFY(!editor.document()->isModified());
    }

    void duplicatePathsAndTabUndo() {
        TestWindow window;
        const auto url = file("duplicate.js", "initial");
        QVERIFY(window.open(url));
        auto *first = window.editor();
        first->moveCursor(QTextCursor::End);
        first->insertPlainText(" edited");
        QVERIFY(window.open(url));
        QCOMPARE(window.tabCount(), 1);
        const QString alias = dir.filePath("alias.js");
        QVERIFY(QFile::link(url.toLocalFile(), alias));
        QVERIFY(window.open(QUrl::fromLocalFile(alias)));
        QCOMPARE(window.tabCount(), 1);
        window.newFile();
        window.editor()->insertPlainText("second");
        QVERIFY(!window.saveEditorAs(window.editor(), url));
        QVERIFY(!window.lastError.isEmpty());
        window.setCurrentIndex(0);
        QCOMPARE(window.editor(), first);
        QCOMPARE(first->textCursor().position(), 14);
        first->undo();
        QCOMPARE(first->toPlainText(), "initial");
        QVERIFY(!first->document()->isModified());
        window.setCurrentIndex(1);
        QCOMPARE(window.editor()->toPlainText(), "second");
    }

    void findReplaceAndUndo() {
        Theme theme;
        CodeEditor editor(&theme);
        editor.setPlainText("<b>hello</b> hello");
        QVERIFY(editor.findNext("hello"));
        QCOMPARE(editor.textCursor().selectedText(), "hello");
        QVERIFY(editor.replaceCurrent("hello", "world"));
        QCOMPARE(editor.toPlainText(), "<b>world</b> hello");
        editor.undo();
        QCOMPARE(editor.toPlainText(), "<b>hello</b> hello");
        QCOMPARE(editor.replaceAll("hello", "hello hello"), 2);
        QCOMPARE(editor.toPlainText(), "<b>hello hello</b> hello hello");
        editor.undo();
        QCOMPARE(editor.toPlainText(), "<b>hello</b> hello");
        QCOMPARE(editor.replaceAll("hello", ""), 2);
        QCOMPARE(editor.toPlainText(), "<b></b> ");
        editor.undo();
        QCOMPARE(editor.toPlainText(), "<b>hello</b> hello");
        QCOMPARE(editor.replaceAll("", "anything"), 0);
    }

    void windowCloseProtectsEveryTab() {
        TestWindow window;
        window.show();
        window.editor()->insertPlainText("first");
        window.newFile();
        window.editor()->insertPlainText("second");
        window.choices = {MainWindow::CloseChoice::Discard, MainWindow::CloseChoice::Cancel};
        QVERIFY(!window.close());
        QCOMPARE(window.prompts, 2);
        QCOMPARE(window.editorAt(0)->toPlainText(), "first");
        QCOMPARE(window.editorAt(1)->toPlainText(), "second");
        window.choices = {MainWindow::CloseChoice::Discard, MainWindow::CloseChoice::Discard};
        QVERIFY(window.close());
    }

    void failedSaveAndCancelDoNotClose() {
        TestWindow window;
        window.editor()->insertPlainText("must survive");
        window.choices = {MainWindow::CloseChoice::Save};
        window.savePath = QUrl::fromLocalFile(dir.filePath("missing/file"));
        QVERIFY(!window.closeTab(0));
        QCOMPARE(window.editor()->toPlainText(), "must survive");
        QVERIFY(window.editor()->document()->isModified());
        QVERIFY(!window.lastError.isEmpty());
        window.choices = {MainWindow::CloseChoice::Save};
        window.savePath = QUrl();
        QVERIFY(!window.closeTab(0));
        window.choices = {MainWindow::CloseChoice::Save};
        window.savePath = QUrl::fromLocalFile(dir.filePath("saved.txt"));
        QVERIFY(window.closeTab(0));
        QVERIFY(window.editor()->document()->isEmpty());
        QFile saved(window.savePath.toLocalFile());
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), "must survive");
    }

    void typingKeepsCursorVisible() {
        TestWindow window;
        window.show();
        auto *editor = window.editor();
        editor->setPlainText(QString("line\n").repeated(100));
        editor->moveCursor(QTextCursor::End);
        editor->setFocus();
        const auto visible = [editor] { return editor->viewport()->rect().contains(editor->cursorRect()); };
        for (int i = 0; i < 20; ++i) QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(visible());
        editor->verticalScrollBar()->setValue(0);
        QTest::qWait(20);
        QCOMPARE(editor->verticalScrollBar()->value(), 0);
        QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(visible());
        editor->setPlainText(QString(1000, QLatin1Char('x')));
        editor->moveCursor(QTextCursor::End);
        QTRY_VERIFY(editor->horizontalScrollBar()->value() > 0);
        editor->setWrapEnabled(true);
        QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(visible());
        QCOMPARE(editor->horizontalScrollBar()->value(), 0);
    }

    void formattingStaysBoundedAndDoesNotModifyText() {
        TestWindow window;
        window.show();
        auto *editor = window.editor();
        editor->setPlainText(QString("const value = 123; // comment\n").repeated(20000));
        QTest::qWait(30);
        QVERIFY(!editor->document()->isModified());
        QVERIFY(editor->highlightedBlockCount() > 0);
        QVERIFY(editor->highlightedBlockCount() < 100);
        QVERIFY(!editor->extraSelections().isEmpty());
        QVERIFY(editor->extraSelections().size() < 500);
        editor->moveCursor(QTextCursor::End);
        QTest::qWait(30);
        QVERIFY(editor->highlightedBlockCount() < 100);
        QVERIFY(editor->extraSelections().first().cursor.position() > 1000);
        editor->setSyntaxEnabled(false);
        QTest::qWait(30);
        QCOMPARE(editor->highlightedBlockCount(), 0);
        QVERIFY(editor->extraSelections().isEmpty());
        QVERIFY(!editor->document()->isModified());
        editor->setWrapEnabled(true);
        editor->setWrapGuides(true);
        editor->setPlainText(QString(10000, QLatin1Char('x')));
        QTest::qWait(30);
        QCOMPARE(editor->blockCount(), 1);
        QVERIFY(editor->document()->firstBlock().layout()->lineCount() > 1);
        QCOMPARE(editor->toPlainText().size(), 10000);
        editor->setWrapEnabled(false);
        editor->setSyntaxEnabled(true);
        editor->setPlainText(QString("const value = 1; ").repeated(3000));
        QTest::qWait(30);
        QVERIFY(editor->extraSelections().isEmpty());
    }

    void syntaxPrecedence() {
        Theme theme;
        const auto ranges = SyntaxHighlighter::formats("\"// 123 if\" // 123 if", theme);
        QCOMPARE(ranges.size(), 2);
        QCOMPARE(ranges[0].format.foreground().color(), theme.accent());
        QCOMPARE(ranges[1].format.foreground().color(), theme.mutedForeground());
    }

    void largeDocumentTyping() {
        TestWindow window;
        window.show();
        auto *editor = window.editor();
        const QString source = QString("const value = 123; // A representative line of source text for profiling.\n").repeated(20000);
        editor->setPlainText(source);
        editor->moveCursor(QTextCursor::End);
        editor->setFocus();
        QTest::qWait(50);
        qint64 total = 0, worst = 0;
        for (int i = 0; i < 30; ++i) {
            QElapsedTimer timer;
            timer.start();
            QTest::keyClick(editor, Qt::Key_A);
            QCoreApplication::processEvents();
            const qint64 elapsed = timer.nsecsElapsed();
            total += elapsed;
            worst = qMax(worst, elapsed);
        }
        QCOMPARE(editor->toPlainText(), source + QString(30, QLatin1Char('a')));
        qInfo("20k-line input/event processing: mean %.2f ms, max %.2f ms (30 keys; excludes display latency)",
              total / 30.0 / 1000000.0, worst / 1000000.0);
    }
};

QTEST_MAIN(EditorTests)
#include "editor_tests.moc"
