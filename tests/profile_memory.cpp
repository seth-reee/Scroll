#include <QCoreApplication>
#include <QFile>
#include <QElapsedTimer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QSettings>
#include <QTextStream>
#include <QThread>
#include <unistd.h>

// Linux-only, opt-in measurement of the actual executable, in fresh processes.
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 2) qFatal("Usage: profile_memory /path/to/qomaedit");
    QTextStream out(stdout);
    QTemporaryDir config;
    if (!config.isValid()) qFatal("Cannot isolate settings");
    QSettings settings(config.filePath("qOmaedit/qOmaedit.conf"), QSettings::IniFormat);
    settings.setValue("editor/syntaxEnabled", true);
    settings.setValue("editor/tabsEnabled", true);
    settings.sync();
    out << "Headless/software rendering, syntax enabled; memory in KiB\n";
    for (const int lines : {0, 1000, 20000, 100000}) {
        QTemporaryFile sample;
        if (!sample.open()) qFatal("Cannot create sample");
        const QByteArray line("const value = 123; // A representative line of source text for profiling.\n");
        for (int i = 0; i < lines; ++i)
            if (sample.write(line) != line.size()) qFatal("Cannot write sample");
        sample.flush();
        QProcess process;
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("QT_QPA_PLATFORM", "offscreen");
        environment.insert("QT_QUICK_BACKEND", "software");
        environment.insert("XDG_CONFIG_HOME", config.path());
        process.setProcessEnvironment(environment);
        process.start(app.arguments().at(1), {sample.fileName()});
        if (!process.waitForStarted()) qFatal("Cannot start editor");
        QThread::msleep(3000);
        const QString proc = "/proc/" + QString::number(process.processId());
        const auto cpuTicks = [&proc] {
            QFile stat(proc + "/stat");
            if (!stat.open(QIODevice::ReadOnly)) qFatal("Cannot read process CPU");
            const auto data = stat.readAll();
            const auto fields = data.mid(data.lastIndexOf(')') + 2).split(' ');
            return fields.at(11).toLongLong() + fields.at(12).toLongLong();
        };
        QElapsedTimer timer;
        timer.start();
        auto previousTicks = cpuTicks();
        int quietSamples = 0;
        while (quietSamples < 5 && timer.elapsed() < 10000) {
            QThread::msleep(200);
            const auto ticks = cpuTicks();
            quietSamples = ticks - previousTicks <= 1 ? quietSamples + 1 : 0;
            previousTicks = ticks;
        }
        QFile memory(proc + "/smaps_rollup");
        if (!memory.open(QIODevice::ReadOnly)) qFatal("Cannot read process memory");
        out << lines << " lines, " << sample.size() << " bytes, "
            << (quietSamples == 5 ? "CPU settled" : "idle threshold not reached") << ":\n";
        for (const auto &entry : memory.readAll().split('\n')) {
            if (entry.startsWith("Rss:") || entry.startsWith("Pss:") || entry.startsWith("Private_Dirty:"))
                out << "  " << entry << '\n';
        }
        const auto idleStart = cpuTicks();
        QElapsedTimer idleTimer;
        idleTimer.start();
        QThread::msleep(1000);
        out << "  idle CPU (% of one core): "
            << 100000.0 * (cpuTicks() - idleStart) / (sysconf(_SC_CLK_TCK) * idleTimer.elapsed()) << '\n';
        out.flush();
        process.terminate();
        if (!process.waitForFinished(3000)) { process.kill(); process.waitForFinished(); }
        const auto errors = process.readAllStandardError();
        if (!errors.isEmpty()) out << "  stderr: " << errors << '\n';
    }
}
