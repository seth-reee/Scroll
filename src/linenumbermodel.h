#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QVector>

class QQuickTextDocument;
class QTextDocument;

class LineNumberModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { NumberRole = Qt::UserRole + 1, TopRole, HeightRole };
    explicit LineNumberModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void setEditorDocument(QQuickTextDocument *document);
    Q_INVOKABLE void scheduleRefresh();

private:
    struct Row { int number; qreal top; qreal height; };
    void refresh();
    QPointer<QTextDocument> m_document;
    QVector<Row> m_rows;
    bool m_refreshQueued = false;
};
