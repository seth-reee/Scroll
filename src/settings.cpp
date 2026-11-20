#include "settings.h"

#include <QSettings>

Settings::Settings(QObject *parent) : QObject(parent) {
    QSettings values;
    m_indentWidth = qBound(1, values.value("editor/indentWidth", 4).toInt(), 16);
    m_indentSpaces = values.value("editor/indentSpaces", false).toBool();
    m_tabsEnabled = values.value("editor/tabsEnabled", true).toBool();
    m_syntaxEnabled = values.value("editor/syntaxEnabled", true).toBool();
    m_wrappedLineSpacing = values.value("editor/wrappedLineSpacing", false).toBool();
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

void Settings::setIndentWidth(int width) {
    width = qBound(1, width, 16);
    if (width == m_indentWidth) return;
    m_indentWidth = width; QSettings().setValue("editor/indentWidth", width); emit indentationChanged();
}
void Settings::setIndentSpaces(bool spaces) {
    if (spaces == m_indentSpaces) return;
    m_indentSpaces = spaces; QSettings().setValue("editor/indentSpaces", spaces); emit indentationChanged();
}
