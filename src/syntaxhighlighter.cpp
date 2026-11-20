#include "syntaxhighlighter.h"
#include "theme.h"
#include <QRegularExpression>
#include <QHash>

QList<QTextLayout::FormatRange> SyntaxHighlighter::formats(const QString &text, const Theme &theme, const QString &language) {
    if (language == "Plain text") return {};
    const bool hashComments = QStringList{"Shell", "Python", "TOML", "YAML", "PHP"}.contains(language);
    const bool slashComments = QStringList{"JavaScript", "TypeScript", "C", "C++", "Rust", "Go", "PHP"}.contains(language);
    QString comment = hashComments ? QStringLiteral("#") : QString();
    if (slashComments) comment += comment.isEmpty() ? QStringLiteral("//") : QStringLiteral("|//");
    if (language == "Lua") comment = "--";
    const QString words = language == "Python" ? "if|else|elif|for|while|def|class|return|import|from|as|try|except|finally|with|lambda|and|or|not|in|True|False|None|pass|yield|async|await" :
        language == "Shell" ? "if|then|else|elif|fi|for|while|do|done|case|esac|function|in|export|local" :
        language == "Lua" ? "if|then|else|elseif|for|while|do|function|return|local|end|nil|true|false|and|or|not" :
        language == "JSON" ? "true|false|null" :
        QStringList{"JavaScript", "TypeScript", "C", "C++", "Rust", "Go", "PHP"}.contains(language) ? "if|else|for|while|return|class|import|from|try|catch|finally|function|var|let|const|true|false|null|struct|enum|static|void|int|char|public|private|fn|mut|impl|pub|func|package|interface|defer|go" : QString();
    QStringList alternatives;
    alternatives << QStringLiteral(R"(((?:"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')))" );
    alternatives << (comment.isEmpty() ? QStringLiteral("(?!)()") : QStringLiteral("((?:%1).*$)").arg(comment));
    alternatives << QStringLiteral(R"((\b\d+(?:\.\d+)?\b))");
    alternatives << (words.isEmpty() ? QStringLiteral("(?!)()") : QStringLiteral(R"((\b(?:%1)\b))").arg(words));
    static QHash<QString, QRegularExpression> cache;
    if (!cache.contains(language)) cache.insert(language, QRegularExpression(alternatives.join('|')));
    const auto &tokens = cache[language];
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
