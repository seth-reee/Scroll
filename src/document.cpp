#include "document.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

QString Document::fileName() const {
    return m_path.isEmpty() ? QStringLiteral("Untitled") : QFileInfo(m_path).fileName();
}

void Document::setText(const QString &text) {
    if (m_text == text) return;
    m_text = text;
    if (!m_modified) { m_modified = true; emit modifiedChanged(); }
    emit textChanged();
}

bool Document::open(const QUrl &url) {
    const QString path = url.toLocalFile();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { emit error(file.errorString()); return false; }
    m_text = QString::fromUtf8(file.readAll());
    m_path = path;
    m_modified = false;
    emit textChanged(); emit fileChanged(); emit modifiedChanged();
    return true;
}

bool Document::writeTo(const QString &path) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { emit error(file.errorString()); return false; }
    file.write(m_text.toUtf8());
    if (!file.commit()) { emit error(file.errorString()); return false; }
    m_path = path; m_modified = false;
    emit fileChanged(); emit modifiedChanged();
    return true;
}

bool Document::save() { return m_path.isEmpty() ? false : writeTo(m_path); }
bool Document::saveAs(const QUrl &url) { return writeTo(url.toLocalFile()); }
void Document::newFile() { m_text.clear(); m_path.clear(); m_modified = false; emit textChanged(); emit fileChanged(); emit modifiedChanged(); }
