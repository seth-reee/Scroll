#include "syntaxhighlighter.h"
#include "theme.h"

#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QTextDocument>

SyntaxHighlighter::SyntaxHighlighter(Theme *theme, QObject *parent)
    : QSyntaxHighlighter(parent), m_theme(theme) {
    connect(theme, &Theme::changed, this, [this] { rehighlight(); });
}

void SyntaxHighlighter::setEditorDocument(QQuickTextDocument *document) {
    setDocument(document ? document->textDocument() : nullptr);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    const auto apply = [this, &text](const QRegularExpression &pattern, const QColor &color) {
        auto match = pattern.globalMatch(text);
        QTextCharFormat format;
        format.setForeground(color);
        while (match.hasNext()) {
            const auto hit = match.next();
            setFormat(hit.capturedStart(), hit.capturedLength(), format);
        }
    };

    apply(QRegularExpression(R"((#|//|--).*?$)"), m_theme->mutedForeground());
    apply(QRegularExpression(R"(("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'))"), m_theme->accent());
    apply(QRegularExpression(R"(\b\d+(?:\.\d+)?\b)"), m_theme->accent().lighter(120));
    apply(QRegularExpression(R"(\b(if|then|else|elif|fi|for|while|do|done|case|esac|function|return|in|and|or|not|def|class|import|from|as|try|except|finally|with|lambda|local|export|var|let|const|function|end|nil|true|false)\b)"), m_theme->foreground().lighter(125));
}
