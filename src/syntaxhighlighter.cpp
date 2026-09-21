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

void SyntaxHighlighter::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    rehighlight();
    emit enabledChanged();
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (!m_enabled) return;
    const auto apply = [this, &text](const QRegularExpression &pattern, const QColor &color) {
        auto match = pattern.globalMatch(text);
        QTextCharFormat format;
        format.setForeground(color);
        while (match.hasNext()) {
            const auto hit = match.next();
            setFormat(hit.capturedStart(), hit.capturedLength(), format);
        }
    };

    static const QRegularExpression numbers(R"(\b\d+(?:\.\d+)?\b)");
    static const QRegularExpression keywords(R"(\b(if|then|else|elif|fi|for|while|do|done|case|esac|function|return|in|and|or|not|def|class|import|from|as|try|except|finally|with|lambda|local|export|var|let|const|end|nil|true|false)\b)");
    // Match strings and comments together so comment markers inside strings do
    // not mask code, and numbers/keywords cannot overwrite their formatting.
    static const QRegularExpression literals(R"(("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|((?:#|//|--).*$))");
    apply(numbers, m_theme->accent().lighter(120));
    apply(keywords, m_theme->foreground().lighter(125));
    auto matches = literals.globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        setFormat(match.capturedStart(), match.capturedLength(),
                  match.capturedStart(1) >= 0 ? m_theme->accent() : m_theme->mutedForeground());
    }
}
