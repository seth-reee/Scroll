#include "syntaxhighlighter.h"
#include "theme.h"
#include <QRegularExpression>

QList<QTextLayout::FormatRange> SyntaxHighlighter::formats(const QString &text, const Theme &theme) {
    static const QRegularExpression tokens(
        R"(("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|((?:#|//|--).*$)|(\b\d+(?:\.\d+)?\b)|(\b(?:if|then|else|elif|fi|for|while|do|done|case|esac|function|return|in|and|or|not|def|class|import|from|as|try|except|finally|with|lambda|local|export|var|let|const|end|nil|true|false)\b))");
    QList<QTextLayout::FormatRange> ranges;
    auto matches = tokens.globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        QColor color = theme.foreground().lighter(125);
        if (match.capturedStart(1) >= 0) color = theme.accent();
        else if (match.capturedStart(2) >= 0) color = theme.mutedForeground();
        else if (match.capturedStart(3) >= 0) color = theme.accent().lighter(120);
        QTextCharFormat format;
        format.setForeground(color);
        ranges.append({int(match.capturedStart()), int(match.capturedLength()), format});
    }
    return ranges;
}
