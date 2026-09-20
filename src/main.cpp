#include "document.h"
#include "linenumbermodel.h"
#include "settings.h"
#include "syntaxhighlighter.h"
#include "theme.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[]) {
    // QApplication supplies the platform file-dialog backend used on Linux.
    QApplication app(argc, argv);
    app.setOrganizationName("Qomaedit");
    app.setApplicationName("Qomaedit");

    Theme theme;
    Document document;
    LineNumberModel lineNumberModel;
    Settings settings;
    SyntaxHighlighter syntaxHighlighter(&theme);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("omarchyTheme", &theme);
    engine.rootContext()->setContextProperty("document", &document);
    engine.rootContext()->setContextProperty("lineNumberModel", &lineNumberModel);
    engine.rootContext()->setContextProperty("settings", &settings);
    engine.rootContext()->setContextProperty("syntaxHighlighter", &syntaxHighlighter);
    engine.loadFromModule("Qomaedit", "Main");
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
