#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QObject>

class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QColor background READ background NOTIFY changed)
    Q_PROPERTY(QColor panel READ panel NOTIFY changed)
    Q_PROPERTY(QColor surface READ surface NOTIFY changed)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QColor mutedForeground READ mutedForeground NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor selection READ selection NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)

public:
    explicit Theme(QObject *parent = nullptr);
    QColor background() const { return m_background; }
    QColor panel() const { return m_panel; }
    QColor surface() const { return m_surface; }
    QColor foreground() const { return m_foreground; }
    QColor mutedForeground() const { return m_mutedForeground; }
    QColor accent() const { return m_accent; }
    QColor selection() const { return m_selection; }
    QString name() const { return m_name; }

    Q_INVOKABLE void reload();

signals:
    void changed();

private:
    void watchFiles();
    QString statePath(const QString &file) const;
    QColor m_background{"#121212"};
    QColor m_panel{"#121212"};
    QColor m_surface{"#1e1e1e"};
    QColor m_foreground{"#bebebe"};
    QColor m_mutedForeground{"#555555"};
    QColor m_accent{"#e68e0d"};
    QColor m_selection{"#333333"};
    QString m_name{"Omarchy"};
    QFileSystemWatcher m_watcher;
};
