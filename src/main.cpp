#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QTimer>
#include <QUrl>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("Scroll");
    app.setApplicationName("Scroll");
    app.setApplicationVersion(SCROLL_VERSION);
    app.setDesktopFileName("scroll");
    app.setWindowIcon(QIcon(":/scroll.png"));
    QCommandLineParser parser;
    parser.setApplicationDescription("A lightweight plain-text editor for Omarchy");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Files to open", "[files...]");
    parser.process(app);
    MainWindow window;
    window.show();
    QTimer::singleShot(0, &window, [&] {
        for (const auto &path : parser.positionalArguments())
            window.open(QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile));
    });
    return app.exec();
}
