#include <QCoreApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>
#include <unistd.h>

namespace {
QByteArray read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
qint64 ticks(qint64 pid) {
    const auto raw = read("/proc/" + QString::number(pid) + "/stat");
    const auto fields = raw.mid(raw.lastIndexOf(')') + 2).split(' ');
    return fields.size() > 12 ? fields[11].toLongLong() + fields[12].toLongLong() : -1;
}
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("executable", "Path to the scroll executable");
    parser.addOption({"desktop", "Use the live desktop instead of Qt's offscreen backend (opens test windows)"});
    parser.addOption({"compare", "Also measure installed KWrite and gedit; requires --desktop"});
    parser.addOption({"repeats", "Repeat with rotated application order (1 to 5)", "count", "1"});
    parser.addOption({"settle-ms", "Wait before sampling memory (at least 1000 ms)", "milliseconds", "8000"});
    parser.process(app);
    const int repeats = parser.value("repeats").toInt();
    const int settle = parser.value("settle-ms").toInt();
    if (parser.positionalArguments().size() != 1 || repeats < 1 || repeats > 5 || settle < 1000
        || (parser.isSet("compare") && !parser.isSet("desktop"))) parser.showHelp(1);
    QStringList names{"Scroll"};
    QStringList programs{parser.positionalArguments().first()};
    if (parser.isSet("compare")) {
        for (const QString &name : {QStringLiteral("kwrite"), QStringLiteral("gedit")}) {
            const auto executable = QStandardPaths::findExecutable(name);
            if (executable.isEmpty()) { qCritical("Missing editor: %s", qPrintable(name)); return 1; }
            names << name;
            programs << executable;
        }
    }
    QTemporaryDir data;
    if (!data.isValid()) return 1;
    QTextStream out(stdout);
    out << (parser.isSet("desktop") ? "Desktop" : "Headless")
        << "; isolated settings; syntax enabled; " << settle << " ms warmup; RSS/PSS in KiB\n";
    for (int run = 0; run < repeats; ++run) {
        for (int lines : {0, 1000, 20000, 100000}) {
            const QString path = data.filePath(QString::number(lines) + ".js");
            QFile sample(path);
            if (!sample.open(QIODevice::WriteOnly)) return 1;
            const QByteArray line("const value = 123; // A representative line of source text for profiling.\n");
            for (int i = 0; i < lines; ++i)
                if (sample.write(line) != line.size()) return 1;
            sample.close();
            for (int offset = 0; offset < programs.size(); ++offset) {
                const int index = (offset + run) % programs.size();
                QTemporaryDir profile;
                if (!profile.isValid()) return 1;
                QSettings settings(profile.filePath("config/Scroll/Scroll.conf"), QSettings::IniFormat);
                settings.setValue("editor/syntaxEnabled", true);
                settings.setValue("editor/tabsEnabled", true);
                settings.sync();
                auto env = QProcessEnvironment::systemEnvironment();
                env.insert("XDG_CONFIG_HOME", profile.filePath("config"));
                env.insert("XDG_CACHE_HOME", profile.filePath("cache"));
                env.insert("XDG_DATA_HOME", profile.filePath("data"));
                env.insert("GSETTINGS_BACKEND", "memory");
                if (parser.isSet("desktop")) {
                    const bool wayland = !env.value("WAYLAND_DISPLAY").isEmpty();
                    env.insert("QT_QPA_PLATFORM", wayland ? "wayland" : "xcb");
                    env.insert("GDK_BACKEND", wayland ? "wayland" : "x11");
                    env.remove("QT_QUICK_BACKEND");
                } else {
                    env.insert("QT_QPA_PLATFORM", "offscreen");
                    env.insert("QT_QUICK_BACKEND", "software");
                }
                QProcess process;
                process.setProcessEnvironment(env);
                QStringList args;
                if (names[index] == "gedit") args << "--standalone";
                args << path;
                process.start(programs[index], args);
                if (!process.waitForStarted() || process.waitForFinished(settle)) {
                    out << "Failed to keep " << names[index] << " running: " << process.readAllStandardError() << '\n';
                    return 1;
                }
                const auto pid = process.processId();
                const auto memory = read("/proc/" + QString::number(pid) + "/smaps_rollup");
                if (memory.isEmpty()) { process.terminate(); process.waitForFinished(); return 1; }
                out << "run=" << run + 1 << " editor=" << names[index] << " lines=" << lines;
                for (const auto &entry : memory.split('\n'))
                    if (entry.startsWith("Rss:") || entry.startsWith("Pss:")) out << " " << entry.simplified();
                const auto startTicks = ticks(pid);
                QElapsedTimer timer;
                timer.start();
                if (process.waitForFinished(1000)) return 1;
                const double cpu = (ticks(pid) - startTicks) * 100000.0 / (sysconf(_SC_CLK_TCK) * timer.elapsed());
                out << " sampled_cpu=" << cpu << "%";
                if (cpu > 2) out << " (still active; repeat with a longer --settle-ms)";
                out << '\n';
                out.flush();
                process.terminate();
                if (!process.waitForFinished(3000)) { process.kill(); process.waitForFinished(); }
                const auto errors = process.readAllStandardError();
                if (!errors.isEmpty()) out << names[index] << " stderr: " << errors << '\n';
            }
        }
    }
}
