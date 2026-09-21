#include "document.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringDecoder>

namespace {
QString localPath(const QUrl &url) {
    const QFileInfo info(url.toLocalFile());
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}
}

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
    if (!tab.modified) { tab.modified = true; emit modifiedChanged(); emit tabsChanged(); }
    emit textChanged();
}

bool Document::open(const QUrl &url) {
    if (!url.isLocalFile()) { emit error("Only local files are supported."); return false; }
    const QString path = localPath(url);
    for (int index = 0; index < m_tabs.size(); ++index) {
        if (m_tabs[index].path == path) { setCurrentIndex(index); return true; }
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { emit error(file.errorString()); return false; }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) { emit error(file.errorString()); return false; }
    QStringDecoder decoder(QStringDecoder::Utf8);
    QString text = decoder(bytes);
    if (decoder.hasError() || text.contains(QChar::Null)) {
        emit error("This file is not valid UTF-8 text."); return false;
    }
    const bool crlf = text.contains("\r\n");
    text.replace("\r\n", "\n");
    State loaded{text, path, false, crlf, bytes.startsWith("\xEF\xBB\xBF")};
    const auto &tab = current();
    if (tab.path.isEmpty() && tab.text.isEmpty() && !tab.modified) m_tabs[m_currentIndex] = loaded;
    else { m_tabs.append(loaded); m_currentIndex = m_tabs.size() - 1; }
    notifyCurrent();
    return true;
}

bool Document::writeTo(const QString &path) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { emit error(file.errorString()); return false; }
    QString text = current().text;
    if (current().crlf) text.replace("\n", "\r\n");
    const QByteArray bytes = (current().utf8Bom ? QByteArray("\xEF\xBB\xBF") : QByteArray()) + text.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        emit error(file.errorString()); file.cancelWriting(); return false;
    }
    if (!file.commit()) { emit error(file.errorString()); return false; }
    current().path = path; current().modified = false;
    emit fileChanged(); emit modifiedChanged(); emit languageChanged(); emit tabsChanged();
    return true;
}

bool Document::save() { return current().path.isEmpty() ? false : writeTo(current().path); }
bool Document::saveAs(const QUrl &url) {
    if (!url.isLocalFile()) { emit error("Only local files are supported."); return false; }
    const QString path = localPath(url);
    for (int index = 0; index < m_tabs.size(); ++index) {
        if (index != m_currentIndex && m_tabs[index].path == path) {
            emit error("This file is already open in another tab."); return false;
        }
    }
    return writeTo(path);
}
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
