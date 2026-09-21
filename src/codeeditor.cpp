#include "codeeditor.h"
#include "theme.h"
#include "syntaxhighlighter.h"
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QPainter>
#include <QSaveFile>
#include <QStringDecoder>
#include <QTextLayout>

namespace {
class Gutter final : public QWidget {
public:
    explicit Gutter(CodeEditor *editor) : QWidget(editor), m_editor(editor) {}
protected:
    void paintEvent(QPaintEvent *event) override { m_editor->paintGutter(event); }
private:
    CodeEditor *m_editor;
};
}

CodeEditor::CodeEditor(Theme *theme, QWidget *parent)
    : QPlainTextEdit(parent), m_theme(theme), m_gutter(new Gutter(this)) {
    setObjectName("editor");
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(15);
    setFont(font);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    setLineWrapMode(NoWrap);
    setWordWrapMode(QTextOption::WrapAnywhere);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateGutterWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, [this](const QRect &rect, int dy) {
        if (dy) m_gutter->scroll(0, dy);
        else m_gutter->update(0, rect.y(), m_gutter->width(), rect.height());
    });
    connect(document(), &QTextDocument::contentsChange, this, [this] {
        for (auto &entry : m_highlighted) entry.revision = -1;
    });
    connect(theme, &Theme::changed, this, &CodeEditor::applyTheme);
    updateGutterWidth();
    applyTheme();
}

QString CodeEditor::localPath(const QUrl &url) {
    if (!url.isLocalFile()) return {};
    const QFileInfo info(url.toLocalFile());
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}
QString CodeEditor::fileName() const { return m_path.isEmpty() ? tr("Untitled") : QFileInfo(m_path).fileName(); }
QString CodeEditor::language() const {
    const QString ext = QFileInfo(m_path).suffix().toLower();
    if (QStringList{"sh", "bash", "zsh", "fish"}.contains(ext)) return "Shell";
    if (ext == "py" || ext == "pyw") return "Python";
    if (ext == "lua") return "Lua";
    if (QStringList{"js", "mjs", "cjs"}.contains(ext)) return "JavaScript";
    if (ext == "ts") return "TypeScript";
    if (ext == "json") return "JSON";
    if (ext == "toml") return "TOML";
    if (ext == "yaml" || ext == "yml") return "YAML";
    if (ext == "html" || ext == "htm") return "HTML";
    if (ext == "css") return "CSS";
    if (ext == "md") return "Markdown";
    return "Plain text";
}

bool CodeEditor::load(const QUrl &url) {
    const QString path = localPath(url);
    if (path.isEmpty()) { emit error(tr("Only local files are supported.")); return false; }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { emit error(file.errorString()); return false; }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) { emit error(file.errorString()); return false; }
    QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
    QString text = decoder(bytes);
    if (decoder.hasError() || text.contains(QChar::Null)) {
        emit error(tr("This file is not valid UTF-8 text.")); return false;
    }
    m_crlf = text.contains("\r\n");
    m_bom = bytes.startsWith("\xef\xbb\xbf");
    text.replace("\r\n", "\n");
    m_highlighted.clear();
    setExtraSelections({});
    setPlainText(text);
    m_path = path;
    document()->setModified(false);
    emit fileChanged();
    return true;
}
bool CodeEditor::save() { return !m_path.isEmpty() && saveAs(QUrl::fromLocalFile(m_path)); }
bool CodeEditor::saveAs(const QUrl &url) {
    const QString path = localPath(url);
    if (path.isEmpty()) { emit error(tr("Only local files are supported.")); return false; }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { emit error(file.errorString()); return false; }
    // No second whole-document string or byte array when saving.
    QByteArray buffer = m_bom ? QByteArray("\xef\xbb\xbf") : QByteArray();
    const QByteArray newline = m_crlf ? QByteArray("\r\n") : QByteArray("\n");
    for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
        buffer += block.text().toUtf8();
        if (block.next().isValid()) buffer += newline;
        if (buffer.size() >= 65536 || !block.next().isValid()) {
            if (file.write(buffer) != buffer.size()) {
                emit error(file.errorString()); file.cancelWriting(); return false;
            }
            buffer.clear();
        }
    }
    if (!file.commit()) { emit error(file.errorString()); return false; }
    m_path = path;
    document()->setModified(false);
    emit fileChanged();
    return true;
}

void CodeEditor::applyTheme() {
    QPalette colors = palette();
    colors.setColor(QPalette::Base, m_theme->panel());
    colors.setColor(QPalette::Text, m_theme->foreground());
    colors.setColor(QPalette::Highlight, m_theme->selection());
    colors.setColor(QPalette::HighlightedText, m_theme->foreground());
    setPalette(colors);
    for (auto &entry : m_highlighted) entry.revision = -1;
    viewport()->update();
    m_gutter->update();
}
void CodeEditor::setSyntaxEnabled(bool enabled) {
    if (m_syntax == enabled) return;
    m_syntax = enabled;
    viewport()->update();
}
void CodeEditor::setWrapEnabled(bool enabled) {
    setLineWrapMode(enabled ? WidgetWidth : NoWrap);
    ensureCursorVisible();
}
void CodeEditor::setWrapGuides(bool enabled) { m_guides = enabled; viewport()->update(); }
int CodeEditor::gutterWidth() const {
    return 18 + fontMetrics().horizontalAdvance('9') * QString::number(blockCount()).size();
}
void CodeEditor::updateGutterWidth() {
    setViewportMargins(gutterWidth(), 0, 0, 0);
    m_gutter->setGeometry(contentsRect().left(), contentsRect().top(), gutterWidth(), viewport()->height());
}
void CodeEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    m_gutter->setGeometry(contentsRect().left(), contentsRect().top(), gutterWidth(), viewport()->height());
}
void CodeEditor::paintGutter(QPaintEvent *event) {
    QPainter painter(m_gutter);
    painter.fillRect(event->rect(), m_theme->background());
    painter.setPen(m_theme->mutedForeground());
    for (QTextBlock block = firstVisibleBlock(); block.isValid(); block = block.next()) {
        const QRectF bounds = blockBoundingGeometry(block).translated(contentOffset());
        if (bounds.top() > event->rect().bottom()) break;
        if (block.isVisible() && bounds.bottom() >= event->rect().top())
            painter.drawText(0, qRound(bounds.top()), m_gutter->width() - 8, fontMetrics().height(),
                             Qt::AlignRight, QString::number(block.blockNumber() + 1));
    }
}

void CodeEditor::highlightViewport() {
    QList<Highlighted> visible;
    bool changed = false;
    if (m_syntax) {
        for (QTextBlock block = firstVisibleBlock(); block.isValid(); block = block.next()) {
            if (blockBoundingGeometry(block).translated(contentOffset()).top() > viewport()->height()) break;
            bool cached = false;
            QList<QTextLayout::FormatRange> formats;
            for (const auto &entry : m_highlighted) {
                if (entry.block == block && entry.revision == block.revision()) {
                    cached = true;
                    formats = entry.formats;
                    break;
                }
            }
            if (!cached) {
                // A minified/giant single line must not turn one visible block
                // into an unbounded regex/formatting job.
                formats = block.length() <= 32769
                    ? SyntaxHighlighter::formats(block.text(), *m_theme)
                    : QList<QTextLayout::FormatRange>();
                changed = true;
            }
            visible.append({block, block.revision(), formats});
        }
    }
    if (visible.size() != m_highlighted.size()) changed = true;
    if (!changed) {
        for (int i = 0; i < visible.size(); ++i) {
            if (visible[i].block != m_highlighted[i].block) { changed = true; break; }
        }
    }
    if (changed) {
        // Public display-only selections avoid modifying the document layout
        // or keeping per-character formatting in offscreen text blocks.
        QList<QTextEdit::ExtraSelection> selections;
        for (const auto &entry : visible) {
            for (const auto &range : entry.formats) {
                QTextCursor cursor(entry.block);
                cursor.setPosition(entry.block.position() + range.start);
                cursor.setPosition(cursor.position() + range.length, QTextCursor::KeepAnchor);
                selections.append({cursor, range.format});
            }
        }
        setExtraSelections(selections);
    }
    m_highlighted = std::move(visible);
}
void CodeEditor::paintEvent(QPaintEvent *event) {
    highlightViewport();
    QPlainTextEdit::paintEvent(event);
    if (!m_guides || lineWrapMode() == NoWrap) return;
    QPainter painter(viewport());
    painter.setPen(m_theme->surface());
    for (QTextBlock block = firstVisibleBlock(); block.isValid(); block = block.next()) {
        const qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
        if (top > viewport()->height()) break;
        const auto *layout = block.layout();
        if (!layout) continue;
        for (int i = 1; i < layout->lineCount(); ++i) {
            const int y = qRound(top + layout->lineAt(i).y());
            if (y > viewport()->height()) break;
            if (y >= 0) painter.drawLine(0, y, viewport()->width(), y);
        }
    }
}

bool CodeEditor::findNext(const QString &needle) {
    if (needle.isEmpty()) return false;
    QTextCursor found = document()->find(needle, textCursor().selectionEnd(), QTextDocument::FindCaseSensitively);
    if (found.isNull()) found = document()->find(needle, 0, QTextDocument::FindCaseSensitively);
    if (found.isNull()) return false;
    setTextCursor(found);
    ensureCursorVisible();
    return true;
}
bool CodeEditor::replaceCurrent(const QString &needle, const QString &replacement) {
    if (needle.isEmpty() || textCursor().selectedText() != needle) return false;
    QTextCursor cursor = textCursor();
    cursor.insertText(replacement);
    setTextCursor(cursor);
    findNext(needle);
    return true;
}
int CodeEditor::replaceAll(const QString &needle, const QString &replacement) {
    if (needle.isEmpty() || needle == replacement) return 0;
    QTextCursor edit(document());
    edit.beginEditBlock();
    QTextCursor found = document()->find(needle, 0, QTextDocument::FindCaseSensitively);
    int count = 0;
    while (!found.isNull()) {
        found.insertText(replacement);
        ++count;
        found = document()->find(needle, found, QTextDocument::FindCaseSensitively);
    }
    edit.endEditBlock();
    return count;
}
