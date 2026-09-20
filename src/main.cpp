#include "document.h"
#include "theme.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Qomaedit");
    app.setApplicationName("Qomaedit");

    Theme theme;
    Document document;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("omarchyTheme", &theme);
    engine.rootContext()->setContextProperty("document", &document);
    engine.loadFromModule("Qomaedit", "Main");
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
