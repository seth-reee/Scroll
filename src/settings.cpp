#include "settings.h"

#include <QSettings>

Settings::Settings(QObject *parent) : QObject(parent) {
    QSettings values;
    const QSettings legacy("Qomaedit", "Qomaedit");
    const auto setting = [&values, &legacy](const QString &key, const QVariant &fallback) {
        return values.contains(key) ? values.value(key) : legacy.value(key, fallback);
    };
    m_tabsEnabled = setting("editor/tabsEnabled", true).toBool();
    m_syntaxEnabled = setting("editor/syntaxEnabled", true).toBool();
    m_wrappedLineSpacing = setting("editor/wrappedLineSpacing", false).toBool();
}

void Settings::setTabsEnabled(bool enabled) {
    if (m_tabsEnabled == enabled) return;
    m_tabsEnabled = enabled; QSettings().setValue("editor/tabsEnabled", enabled); emit tabsEnabledChanged();
}

void Settings::setSyntaxEnabled(bool enabled) {
    if (m_syntaxEnabled == enabled) return;
    m_syntaxEnabled = enabled; QSettings().setValue("editor/syntaxEnabled", enabled); emit syntaxEnabledChanged();
}

void Settings::setWrappedLineSpacing(bool enabled) {
    if (m_wrappedLineSpacing == enabled) return;
    m_wrappedLineSpacing = enabled; QSettings().setValue("editor/wrappedLineSpacing", enabled); emit wrappedLineSpacingChanged();
}
