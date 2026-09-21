#pragma once
#include <QTextLayout>
class Theme;
// Stateless single-line coloring, requested only for visible blocks.
class SyntaxHighlighter {
public:
    static QList<QTextLayout::FormatRange> formats(const QString &text, const Theme &theme);
};
