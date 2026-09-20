#include "linenumbermodel.h"

#include <QAbstractTextDocumentLayout>
#include <QQuickTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include <QTimer>

LineNumberModel::LineNumberModel(QObject *parent) : QAbstractListModel(parent) {}

int LineNumberModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant LineNumberModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size()) return {};
    const auto &row = m_rows.at(index.row());
    if (role == NumberRole) return row.number;
    if (role == TopRole) return row.top;
    if (role == HeightRole) return row.height;
    return {};
}

QHash<int, QByteArray> LineNumberModel::roleNames() const {
    return {{NumberRole, "number"}, {TopRole, "top"}, {HeightRole, "lineHeight"}};
}

void LineNumberModel::setEditorDocument(QQuickTextDocument *document) {
    if (m_document == (document ? document->textDocument() : nullptr)) return;
    if (m_document) disconnect(m_document, nullptr, this, nullptr);
    m_document = document ? document->textDocument() : nullptr;
    if (m_document)
        connect(m_document, &QTextDocument::contentsChanged, this, &LineNumberModel::scheduleRefresh);
    scheduleRefresh();
}

void LineNumberModel::scheduleRefresh() {
    if (m_refreshQueued) return;
    m_refreshQueued = true;
    QTimer::singleShot(0, this, [this] { m_refreshQueued = false; refresh(); });
}

void LineNumberModel::refresh() {
    beginResetModel();
    m_rows.clear();
    if (m_document && m_document->documentLayout()) {
        for (QTextBlock block = m_document->begin(); block.isValid(); block = block.next()) {
            const auto bounds = m_document->documentLayout()->blockBoundingRect(block);
            const auto *layout = block.layout();
            if (!layout || layout->lineCount() == 0) {
                m_rows.append({block.blockNumber() + 1, bounds.top(), bounds.height()});
                continue;
            }
            for (int i = 0; i < layout->lineCount(); ++i) {
                const QTextLine line = layout->lineAt(i);
                // Continuation rows have number 0, which QML renders as a gap.
                m_rows.append({i == 0 ? block.blockNumber() + 1 : 0,
                               bounds.top() + line.y(), line.height()});
            }
        }
    }
    endResetModel();
}
