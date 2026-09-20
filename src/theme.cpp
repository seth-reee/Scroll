#include "theme.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>

Theme::Theme(QObject *parent) : QObject(parent) {
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] {
        reload();
        watchFiles(); // Atomic Omarchy theme replacement removes the old watch.
    });
    reload();
    watchFiles();
}

QString Theme::statePath(const QString &file) const {
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + "/.local/state/omarchy/current/" + file;
}

void Theme::watchFiles() {
    const QStringList wanted{statePath("theme/colors.toml"), statePath("theme.name")};
    const QStringList watched = m_watcher.files();
    for (const auto &path : wanted)
        if (QFile::exists(path) && !watched.contains(path)) m_watcher.addPath(path);
}

void Theme::reload() {
    QFile colors(statePath("theme/colors.toml"));
    if (colors.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString data = QString::fromUtf8(colors.readAll());
        const auto colorFor = [&data](const QString &key, const QColor &fallback) {
            const QRegularExpression expression("^\\s*" + QRegularExpression::escape(key)
                + "\\s*=\\s*\\\"(#[0-9a-fA-F]{6})\\\"", QRegularExpression::MultilineOption);
            const auto match = expression.match(data);
            return match.hasMatch() ? QColor(match.captured(1)) : fallback;
        };
        m_background = colorFor("background", m_background);
        m_panel = colorFor("dark_background", m_panel);
        m_surface = colorFor("lighter_background", m_surface);
        m_foreground = colorFor("foreground", m_foreground);
        m_mutedForeground = colorFor("dark_foreground", m_mutedForeground);
        m_accent = colorFor("accent", m_accent);
        m_selection = colorFor("selection", m_selection);
    }
    QFile nameFile(statePath("theme.name"));
    if (nameFile.open(QIODevice::ReadOnly | QIODevice::Text)) m_name = QString::fromUtf8(nameFile.readAll()).trimmed();
    emit changed();
}
