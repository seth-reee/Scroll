#include "settings.h"

#include <QSettings>

Settings::Settings(QObject *parent) : QObject(parent) {
    QSettings values;
    m_tabsEnabled = values.value("editor/tabsEnabled", true).toBool();
    m_syntaxEnabled = values.value("editor/syntaxEnabled", true).toBool();
}

void Settings::setTabsEnabled(bool enabled) {
    if (m_tabsEnabled == enabled) return;
    m_tabsEnabled = enabled; QSettings().setValue("editor/tabsEnabled", enabled); emit tabsEnabledChanged();
}

void Settings::setSyntaxEnabled(bool enabled) {
    if (m_syntaxEnabled == enabled) return;
    m_syntaxEnabled = enabled; QSettings().setValue("editor/syntaxEnabled", enabled); emit syntaxEnabledChanged();
}
