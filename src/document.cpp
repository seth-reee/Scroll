#include "document.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

Document::Document(QObject *parent) : QObject(parent) {}
Document::State &Document::current() { return m_tabs[m_currentIndex]; }
const Document::State &Document::current() const { return m_tabs[m_currentIndex]; }
QString Document::text() const { return current().text; }
QString Document::fileName() const { return displayName(current()); }
QString Document::filePath() const { return current().path.isEmpty() ? QStringLiteral("Untitled") : current().path; }
QString Document::directoryPath() const {
    return current().path.isEmpty() ? QStringLiteral("Untitled") : QFileInfo(current().path).absolutePath();
}
bool Document::modified() const { return current().modified; }
QString Document::language() const { return languageForPath(current().path); }

QVariantList Document::tabs() const {
    QVariantList result;
    for (const auto &tab : m_tabs)
        result.append(QVariantMap{{"title", displayName(tab)}, {"modified", tab.modified}});
    return result;
}

void Document::notifyCurrent() {
    emit textChanged(); emit fileChanged(); emit modifiedChanged(); emit languageChanged();
    emit tabsChanged(); emit currentIndexChanged();
}

void Document::setCurrentIndex(int index) {
    if (index < 0 || index >= m_tabs.size() || index == m_currentIndex) return;
    m_currentIndex = index;
    notifyCurrent();
}

void Document::setText(const QString &text) {
    auto &tab = current();
    if (tab.text == text) return;
    tab.text = text;
    if (!tab.modified) { tab.modified = true; emit modifiedChanged(); }
    emit textChanged(); emit tabsChanged();
}

bool Document::open(const QUrl &url) {
    const QString path = url.toLocalFile();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { emit error(file.errorString()); return false; }
    State loaded{QString::fromUtf8(file.readAll()), path, false};
    const auto &tab = current();
    if (tab.path.isEmpty() && tab.text.isEmpty() && !tab.modified) m_tabs[m_currentIndex] = loaded;
    else { m_tabs.append(loaded); m_currentIndex = m_tabs.size() - 1; }
    notifyCurrent();
    return true;
}

bool Document::writeTo(const QString &path) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { emit error(file.errorString()); return false; }
    file.write(current().text.toUtf8());
    if (!file.commit()) { emit error(file.errorString()); return false; }
    current().path = path; current().modified = false;
    notifyCurrent();
    return true;
}

bool Document::save() { return current().path.isEmpty() ? false : writeTo(current().path); }
bool Document::saveAs(const QUrl &url) { return writeTo(url.toLocalFile()); }
void Document::newFile() {
    const auto &tab = current();
    if (tab.path.isEmpty() && tab.text.isEmpty() && !tab.modified) return;
    m_tabs.append(State{}); m_currentIndex = m_tabs.size() - 1;
    notifyCurrent();
}

bool Document::tabModified(int index) const {
    return index >= 0 && index < m_tabs.size() && m_tabs[index].modified;
}

bool Document::closeTab(int index, bool discard) {
    if (index < 0 || index >= m_tabs.size()) return false;
    if (m_tabs[index].modified && !discard) return false;
    if (m_tabs.size() == 1) { m_tabs[0] = {}; m_currentIndex = 0; }
    else {
        m_tabs.removeAt(index);
        if (m_currentIndex >= m_tabs.size()) m_currentIndex = m_tabs.size() - 1;
        else if (index < m_currentIndex) --m_currentIndex;
    }
    notifyCurrent();
    return true;
}

QString Document::displayName(const State &state) {
    return state.path.isEmpty() ? QStringLiteral("Untitled") : QFileInfo(state.path).fileName();
}

QString Document::languageForPath(const QString &path) {
    const QString extension = QFileInfo(path).suffix().toLower();
    if (extension == "sh" || extension == "bash" || extension == "zsh" || extension == "fish") return "Shell";
    if (extension == "py" || extension == "pyw") return "Python";
    if (extension == "lua") return "Lua";
    if (extension == "js" || extension == "mjs" || extension == "cjs") return "JavaScript";
    if (extension == "ts") return "TypeScript";
    if (extension == "json") return "JSON";
    if (extension == "toml") return "TOML";
    if (extension == "yaml" || extension == "yml") return "YAML";
    if (extension == "html" || extension == "htm") return "HTML";
    if (extension == "css") return "CSS";
    if (extension == "md") return "Markdown";
    return "Plain text";
}
