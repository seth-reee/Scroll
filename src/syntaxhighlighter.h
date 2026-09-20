#pragma once

#include <QSyntaxHighlighter>

class QQuickTextDocument;
class Theme;

class SyntaxHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit SyntaxHighlighter(Theme *theme, QObject *parent = nullptr);
    Q_INVOKABLE void setEditorDocument(QQuickTextDocument *document);

protected:
    void highlightBlock(const QString &text) override;

private:
    Theme *m_theme;
};
