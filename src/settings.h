#pragma once

#include <QObject>

class Settings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool tabsEnabled READ tabsEnabled WRITE setTabsEnabled NOTIFY tabsEnabledChanged)
    Q_PROPERTY(bool syntaxEnabled READ syntaxEnabled WRITE setSyntaxEnabled NOTIFY syntaxEnabledChanged)
public:
    explicit Settings(QObject *parent = nullptr);
    bool tabsEnabled() const { return m_tabsEnabled; }
    bool syntaxEnabled() const { return m_syntaxEnabled; }
    void setTabsEnabled(bool enabled);
    void setSyntaxEnabled(bool enabled);
signals:
    void tabsEnabledChanged();
    void syntaxEnabledChanged();
private:
    bool m_tabsEnabled = true;
    bool m_syntaxEnabled = true;
};
