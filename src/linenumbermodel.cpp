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
    if (role == LineTopRole) return row.top;
    if (role == LineHeightRole) return row.height;
    return {};
}

QHash<int, QByteArray> LineNumberModel::roleNames() const {
    return {{NumberRole, "number"}, {LineTopRole, "lineTop"}, {LineHeightRole, "lineHeight"}};
}

void LineNumberModel::setEditorDocument(QQuickTextDocument *document) {
    if (m_document == (document ? document->textDocument() : nullptr)) return;
    if (m_document) disconnect(m_document, nullptr, this, nullptr);
    m_document = document ? document->textDocument() : nullptr;
    if (m_document)
        connect(m_document, &QTextDocument::contentsChanged, this, &LineNumberModel::scheduleRefresh);
    if (m_document)
        connect(m_document, &QTextDocument::blockCountChanged, this, &LineNumberModel::lineCountChanged);
    emit lineCountChanged();
    scheduleRefresh();
}

int LineNumberModel::lineCount() const { return m_document ? m_document->blockCount() : 1; }

void LineNumberModel::setViewport(qreal top, qreal height) {
    if (m_viewportTop == top && m_viewportHeight == height) return;
    m_viewportTop = top;
    m_viewportHeight = height;
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
        const qreal top = qMax(qreal(0), m_viewportTop - 40);
        const qreal bottom = m_viewportTop + m_viewportHeight + 40;
        const int position = m_document->documentLayout()->hitTest(QPointF(0, top), Qt::FuzzyHit);
        QTextBlock first = m_document->findBlock(qMax(0, position));
        if (first.previous().isValid()) first = first.previous();
        for (QTextBlock block = first; block.isValid(); block = block.next()) {
            const auto bounds = m_document->documentLayout()->blockBoundingRect(block);
            if (bounds.top() > bottom) break;
            if (bounds.bottom() < top) continue;
            const auto *layout = block.layout();
            if (!layout || layout->lineCount() == 0) {
                m_rows.append({block.blockNumber() + 1, bounds.top(), bounds.height()});
                continue;
            }
            for (int i = 0; i < layout->lineCount(); ++i) {
                const QTextLine line = layout->lineAt(i);
                if (bounds.top() + line.y() + line.height() < top) continue;
                if (bounds.top() + line.y() > bottom) break;
                // Continuation rows have number 0, which QML renders as a gap.
                m_rows.append({i == 0 ? block.blockNumber() + 1 : 0,
                               bounds.top() + line.y(), line.height()});
            }
        }
    }
    endResetModel();
}
