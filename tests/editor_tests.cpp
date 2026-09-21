#include "document.h"
#include "linenumbermodel.h"
#include "settings.h"
#include "syntaxhighlighter.h"
#include "theme.h"

#include <QApplication>
#include <QFile>
#include <QElapsedTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickTextDocument>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>

class EditorTests : public QObject {
    Q_OBJECT
private:
    QTemporaryDir dir;
    QUrl file(const QString &name, const QByteArray &bytes) {
        const QString path = dir.filePath(name);
        QFile output(path);
        if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size())
            qFatal("Could not create test file");
        return QUrl::fromLocalFile(path);
    }
    struct Window {
        Theme theme;
        Document document;
        LineNumberModel lines;
        Settings settings;
        SyntaxHighlighter highlighter{&theme};
        QQmlApplicationEngine engine;
        Window() {
            auto *context = engine.rootContext();
            context->setContextProperty("omarchyTheme", &theme);
            context->setContextProperty("document", &document);
            context->setContextProperty("lineNumberModel", &lines);
            context->setContextProperty("settings", &settings);
            context->setContextProperty("syntaxHighlighter", &highlighter);
            context->setContextProperty("applicationVersion", "test");
            engine.load(QUrl::fromLocalFile(MAIN_QML));
        }
        QObject *root() { return engine.rootObjects().value(0); }
        QObject *item(const char *name) { return root()->findChild<QObject *>(name); }
    };

private slots:
    void largeDocumentTyping() {
        Window w;
        QVERIFY(w.root());
        auto *window = qobject_cast<QQuickWindow *>(w.root());
        QObject *editor = w.item("editor");
        const QString source = QString("const value = 123; // A representative line of source text for profiling.\n").repeated(20000);
        w.document.setText(source);
        editor->setProperty("cursorPosition", editor->property("length"));
        QMetaObject::invokeMethod(editor, "forceActiveFocus");
        QTest::qWait(50);
        qint64 total = 0;
        qint64 worst = 0;
        for (int i = 0; i < 30; ++i) {
            QElapsedTimer timer;
            timer.start();
            QTest::keyClick(window, Qt::Key_A);
            QCoreApplication::processEvents();
            const qint64 elapsed = timer.nsecsElapsed();
            total += elapsed;
            worst = qMax(worst, elapsed);
        }
        QCOMPARE(w.document.text(), source + QString(30, QLatin1Char('a')));
        qInfo("20k-line input/event processing: mean %.2f ms, max %.2f ms (30 keys; excludes display latency)",
              total / 30.0 / 1000000.0, worst / 1000000.0);
    }

    void typingKeepsCursorVisible() {
        Window w;
        QVERIFY(w.root());
        auto *window = qobject_cast<QQuickWindow *>(w.root());
        QObject *editor = w.item("editor");
        QObject *scroll = w.item("editorScroll");
        QVERIFY(scroll);
        w.document.setText(QString("line\n").repeated(100));
        editor->setProperty("cursorPosition", editor->property("length"));
        QMetaObject::invokeMethod(editor, "forceActiveFocus");
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(scroll->property("contentY").toReal() > 0);
        const auto visible = [&] {
            const QRectF cursor = editor->property("cursorRectangle").toRectF();
            const qreal top = scroll->property("contentY").toReal();
            return cursor.top() >= top && cursor.bottom() <= top + scroll->property("height").toReal() + 1;
        };
        QTRY_VERIFY(visible());
        for (int i = 0; i < 20; ++i) QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(visible());
        // Scrolling up to read must not snap back to the insertion point.
        scroll->setProperty("contentY", 0);
        QTest::qWait(30);
        QCOMPARE(scroll->property("contentY").toReal(), 0);
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(visible());
        editor->setProperty("cursorPosition", 0);
        QTRY_COMPARE(scroll->property("contentY").toReal(), 0);
        w.document.setText(QString(1000, QLatin1Char('x')));
        editor->setProperty("cursorPosition", editor->property("length"));
        QTRY_VERIFY(scroll->property("contentX").toReal() > 0);
        w.root()->setProperty("lineWrapping", true);
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(visible());
        QTRY_COMPARE(scroll->property("contentX").toReal(), 0);
    }

    void fileSafety() {
        Document doc;
        QSignalSpy errors(&doc, &Document::error);
        doc.setText("keep me");
        QVERIFY(!doc.open(file("invalid", QByteArray("\xff", 1))));
        QVERIFY(!doc.open(file("binary", QByteArray("a\0b", 3))));
        QCOMPARE(doc.text(), "keep me");
        QVERIFY(!doc.saveAs(QUrl::fromLocalFile(dir.filePath("missing/file"))));
        QVERIFY(doc.modified());
        QVERIFY(!doc.hasFile());
        QCOMPARE(errors.count(), 3);
        QVERIFY(!doc.closeTab(0));
    }

    void roundTrip() {
        const QByteArray original = QByteArray("\xef\xbb\xbf") + "hello\r\nworld\r\n";
        const QUrl url = file("Untitled", original);
        Document doc;
        QVERIFY(doc.open(url));
        QVERIFY(doc.hasFile());
        QCOMPARE(doc.text(), "hello\nworld\n");
        QVERIFY(doc.save());
        QFile input(url.toLocalFile());
        QVERIFY(input.open(QIODevice::ReadOnly));
        QCOMPARE(input.readAll(), original);
        doc.setText("changed");
        QVERIFY(doc.open(url));
        QCOMPARE(doc.tabs().size(), 1);
        QCOMPARE(doc.text(), "changed");
        const QString alias = dir.filePath("alias.txt");
        QVERIFY(QFile::link(url.toLocalFile(), alias));
        QVERIFY(doc.open(QUrl::fromLocalFile(alias)));
        QCOMPARE(doc.tabs().size(), 1);
        QCOMPARE(doc.text(), "changed");
        doc.newFile();
        doc.setText("other");
        QVERIFY(!doc.saveAs(url));
        QCOMPARE(doc.text(), "other");
    }

    void tabNotifications() {
        Document doc;
        QSignalSpy tabs(&doc, &Document::tabsChanged);
        doc.setText("a");
        doc.setText("ab");
        doc.setText("abc");
        QCOMPARE(tabs.count(), 1);
        doc.newFile();
        doc.setText("second");
        doc.newFile();
        doc.setText("third");
        QVERIFY(doc.closeTab(0, true));
        QCOMPARE(doc.currentIndex(), 1);
        QCOMPARE(doc.text(), "third");
    }

    void qmlSynchronizationAndReplacement() {
        Window w;
        QVERIFY(w.root());
        QObject *editor = w.item("editor");
        QVERIFY(editor);
        w.document.setText("<b>hello</b> hello");
        QCOMPARE(editor->property("text").toString(), w.document.text());
        QCOMPARE(editor->property("length").toInt(), w.document.text().size());
        w.item("findField")->setProperty("text", "hello");
        w.item("replaceField")->setProperty("text", "world");
        editor->setProperty("cursorPosition", 0);
        QVERIFY(QMetaObject::invokeMethod(w.root(), "findNext"));
        QCOMPARE(editor->property("selectedText").toString(), "hello");
        editor->setProperty("focus", false);
        QVERIFY(QMetaObject::invokeMethod(w.root(), "replaceCurrent"));
        QCOMPARE(w.document.text(), "<b>world</b> hello");
        w.document.newFile();
        editor->setProperty("text", "second tab");
        QCOMPARE(w.document.text(), "second tab");
        w.document.setCurrentIndex(0);
        QCOMPARE(editor->property("text").toString(), "<b>world</b> hello");
        w.document.setCurrentIndex(1);
        QCOMPARE(editor->property("text").toString(), "second tab");
        w.item("findField")->setProperty("text", "tab");
        w.item("replaceField")->setProperty("text", "document");
        editor->setProperty("focus", false);
        QVERIFY(QMetaObject::invokeMethod(w.root(), "replaceAll"));
        QCOMPARE(w.document.text(), "second document");
        w.document.setCurrentIndex(0);
        w.document.setCurrentIndex(1);
        QCOMPARE(editor->property("text").toString(), "second document");
    }

    void windowCloseProtectsAllTabs() {
        Window w;
        QVERIFY(w.root());
        w.document.setText("unsaved background tab");
        w.document.newFile();
        auto *window = qobject_cast<QQuickWindow *>(w.root());
        QVERIFY(window);
        QVERIFY(!window->close());
        QVERIFY(window->isVisible());
        QCOMPARE(w.item("closeTabDialog")->property("tabIndex").toInt(), 0);
        QVERIFY(w.item("closeTabDialog")->property("visible").toBool());
    }

    void failedSaveDoesNotCloseTab() {
        Window w;
        QVERIFY(w.root());
        w.document.setText("must survive");
        w.root()->setProperty("pendingCloseTab", 0);
        QObject *dialog = w.item("saveDialog");
        QVERIFY(dialog->setProperty("file", QUrl::fromLocalFile(dir.filePath("missing/file"))));
        QVERIFY(QMetaObject::invokeMethod(dialog, "accepted"));
        QCOMPARE(w.document.text(), "must survive");
        QVERIFY(w.document.modified());
        QCOMPARE(w.root()->property("pendingCloseTab").toInt(), -1);
    }

    void saveAndCloseContinuesThroughUnsavedTabs() {
        Window w;
        QVERIFY(w.root());
        w.document.setText("first");
        w.document.newFile();
        w.document.setText("second");
        auto *window = qobject_cast<QQuickWindow *>(w.root());
        QVERIFY(!window->close());
        w.document.setCurrentIndex(0);
        w.root()->setProperty("pendingCloseTab", 0);
        QObject *dialog = w.item("saveDialog");
        const QString path = dir.filePath("saved.txt");
        QVERIFY(dialog->setProperty("file", QUrl::fromLocalFile(path)));
        QVERIFY(QMetaObject::invokeMethod(dialog, "accepted"));
        QCOMPARE(w.document.tabs().size(), 1);
        QCOMPARE(w.document.text(), "second");
        QCoreApplication::processEvents();
        QVERIFY(window->isVisible());
        QVERIFY(w.item("closeTabDialog")->property("visible").toBool());
        QFile saved(path);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), "first");
        QVERIFY(w.document.closeTab(0, true));
        QVERIFY(QMetaObject::invokeMethod(w.root(), "continueClosing"));
        QTRY_VERIFY(!window->isVisible());
    }

    void gutterIsBounded() {
        Window w;
        QVERIFY(w.root());
        w.document.setText(QString("a line\n").repeated(20000));
        QTRY_VERIFY(w.lines.rowCount() > 0);
        QCOMPARE(w.lines.lineCount(), 20001);
        QVERIFY(w.lines.rowCount() < 100);
        w.lines.setViewport(100000, 600);
        QTest::qWait(20);
        QVERIFY(w.lines.rowCount() > 0);
        QVERIFY(w.lines.rowCount() < 100);
        QVERIFY(w.lines.data(w.lines.index(0), LineNumberModel::NumberRole).toInt() > 1000);
        w.root()->setProperty("lineWrapping", true);
        w.document.setText(QString(10000, QLatin1Char('x')));
        w.lines.setViewport(0, 600);
        QTest::qWait(20);
        QCOMPARE(w.lines.lineCount(), 1);
        QVERIFY(w.lines.rowCount() > 1);
        QVERIFY(w.lines.rowCount() < 100);
        QCOMPARE(w.lines.data(w.lines.index(1), LineNumberModel::NumberRole).toInt(), 0);
    }
};

QTEST_MAIN(EditorTests)
#include "editor_tests.moc"
