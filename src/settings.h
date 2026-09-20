#pragma once

#include <QObject>

class Settings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool tabsEnabled READ tabsEnabled WRITE setTabsEnabled NOTIFY tabsEnabledChanged)
    Q_PROPERTY(bool syntaxEnabled READ syntaxEnabled WRITE setSyntaxEnabled NOTIFY syntaxEnabledChanged)
    Q_PROPERTY(bool wrappedLineSpacing READ wrappedLineSpacing WRITE setWrappedLineSpacing NOTIFY wrappedLineSpacingChanged)
public:
    explicit Settings(QObject *parent = nullptr);
    bool tabsEnabled() const { return m_tabsEnabled; }
    bool syntaxEnabled() const { return m_syntaxEnabled; }
    bool wrappedLineSpacing() const { return m_wrappedLineSpacing; }
    void setTabsEnabled(bool enabled);
    void setSyntaxEnabled(bool enabled);
    void setWrappedLineSpacing(bool enabled);
signals:
    void tabsEnabledChanged();
    void syntaxEnabledChanged();
    void wrappedLineSpacingChanged();
private:
    bool m_tabsEnabled = true;
    bool m_syntaxEnabled = true;
    bool m_wrappedLineSpacing = false;
};
