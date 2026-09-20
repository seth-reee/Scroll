#pragma once

#include <QObject>
#include <QUrl>

class Document : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)

public:
    explicit Document(QObject *parent = nullptr) : QObject(parent) {}
    QString text() const { return m_text; }
    QString fileName() const;
    bool modified() const { return m_modified; }
    void setText(const QString &text);

    Q_INVOKABLE bool open(const QUrl &url);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QUrl &url);
    Q_INVOKABLE void newFile();

signals:
    void textChanged();
    void fileChanged();
    void modifiedChanged();
    void error(const QString &message);

private:
    bool writeTo(const QString &path);
    QString m_text;
    QString m_path;
    bool m_modified = false;
};
