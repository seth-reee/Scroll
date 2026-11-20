#pragma once
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextLayout>
#include <QUrl>
class Theme;
class CodeEditor final : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(Theme *theme, QWidget *parent = nullptr);
    QString filePath() const { return m_path; }
    QString fileName() const;
    QString language() const;
    bool load(const QUrl &url);
    bool save();
    bool saveAs(const QUrl &url);
    static QString localPath(const QUrl &url);
    void setSyntaxEnabled(bool enabled);
    void setWrapEnabled(bool enabled);
    void setWrapGuides(bool enabled);
    bool findNext(const QString &needle, bool caseSensitive = true, bool backwards = false);
    bool replaceCurrent(const QString &needle, const QString &replacement, bool caseSensitive = true);
    int replaceAll(const QString &needle, const QString &replacement, bool caseSensitive = true);
    int highlightedBlockCount() const { return m_highlighted.size(); }
    int gutterWidth() const;
    QString lineEnding() const { return m_crlf ? "CRLF" : "LF"; }
    bool hasBom() const { return m_bom; }
    bool diskChanged() const;
    void setIndentation(int width, bool spaces);
    void paintGutter(QPaintEvent *event);
signals:
    void fileChanged();
    void error(const QString &message);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
private:
    void updateGutterWidth();
    void highlightViewport();
    void applyTheme();
    struct Highlighted { QTextBlock block; int revision; QList<QTextLayout::FormatRange> formats; };
    QList<Highlighted> m_highlighted;
    Theme *m_theme;
    QWidget *m_gutter;
    QString m_path;
    bool m_crlf = false;
    bool m_bom = false;
    bool m_syntax = true;
    bool m_guides = false;
    int m_indentWidth = 4;
    bool m_indentSpaces = false;
    QByteArray m_diskHash;
};
