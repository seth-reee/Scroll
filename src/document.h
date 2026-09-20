#pragma once

#include <QObject>
#include <QUrl>
#include <QVector>

class Document : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY fileChanged)
    Q_PROPERTY(QString directoryPath READ directoryPath NOTIFY fileChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QString encoding READ encoding CONSTANT)
    Q_PROPERTY(QVariantList tabs READ tabs NOTIFY tabsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    explicit Document(QObject *parent = nullptr);
    QString text() const;
    QString fileName() const;
    QString filePath() const;
    QString directoryPath() const;
    bool modified() const;
    QString language() const;
    QString encoding() const { return QStringLiteral("UTF-8"); }
    QVariantList tabs() const;
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);
    void setText(const QString &text);

    Q_INVOKABLE bool open(const QUrl &url);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QUrl &url);
    Q_INVOKABLE void newFile();
    Q_INVOKABLE bool tabModified(int index) const;
    Q_INVOKABLE bool closeTab(int index, bool discard = false);

signals:
    void textChanged();
    void fileChanged();
    void modifiedChanged();
    void languageChanged();
    void tabsChanged();
    void currentIndexChanged();
    void error(const QString &message);

private:
    struct State { QString text; QString path; bool modified = false; };
    State &current();
    const State &current() const;
    void notifyCurrent();
    bool writeTo(const QString &path);
    static QString displayName(const State &state);
    static QString languageForPath(const QString &path);
    QVector<State> m_tabs{{}};
    int m_currentIndex = 0;
};
