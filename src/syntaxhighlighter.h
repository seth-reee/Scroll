#pragma once

#include <QSyntaxHighlighter>

class QQuickTextDocument;
class Theme;

class SyntaxHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
public:
    explicit SyntaxHighlighter(Theme *theme, QObject *parent = nullptr);
    Q_INVOKABLE void setEditorDocument(QQuickTextDocument *document);
    bool enabled() const { return m_enabled; }
    Q_INVOKABLE void setEnabled(bool enabled);

signals:
    void enabledChanged();

protected:
    void highlightBlock(const QString &text) override;

private:
    Theme *m_theme;
    bool m_enabled = true;
};
