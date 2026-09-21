#include "document.h"
#include "linenumbermodel.h"
#include "settings.h"
#include "syntaxhighlighter.h"
#include "theme.h"

#include <QApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

int main(int argc, char *argv[]) {
    // QApplication supplies the platform file-dialog backend used on Linux.
    QApplication app(argc, argv);
    app.setOrganizationName("qOmaedit");
    app.setApplicationName("qOmaedit");
    app.setApplicationVersion(QOMAEDIT_VERSION);

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
    engine.rootContext()->setContextProperty("applicationVersion", app.applicationVersion());
    engine.rootContext()->setContextProperty("syntaxHighlighter", &syntaxHighlighter);
    engine.loadFromModule("Qomaedit", "Main");
    if (engine.rootObjects().isEmpty()) return 1;

    // Desktop launchers pass selected files as positional arguments. Open each
    // one after QML has loaded so any file error can be shown by the UI.
    const auto arguments = app.arguments();
    for (qsizetype index = 1; index < arguments.size(); ++index)
        document.open(QUrl::fromUserInput(arguments.at(index), QDir::currentPath(), QUrl::AssumeLocalFile));

    return app.exec();
}
