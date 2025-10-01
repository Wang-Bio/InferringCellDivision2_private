#include "zoomablegraphicsview.h"

#include <QContextMenuEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPolygonItem>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QtGlobal>

namespace {
constexpr double kZoomFactorBase = 1.15;
}

ZoomableGraphicsView::ZoomableGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void ZoomableGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (!scene()) {
        QGraphicsView::wheelEvent(event);
        return;
    }

#if QT_CONFIG(wheelevent)
    const QPoint angleDelta = event->angleDelta();
    if (angleDelta.y() == 0) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const double factor = angleDelta.y() > 0 ? kZoomFactorBase : 1.0 / kZoomFactorBase;
    applyZoomFactor(factor);
    event->accept();
#else
    QGraphicsView::wheelEvent(event);
#endif
}

void ZoomableGraphicsView::applyZoomFactor(double factor)
{
    const double currentScale = transform().m11();
    double newScale = currentScale * factor;
    if (newScale < m_minimumScale) {
        factor = m_minimumScale / currentScale;
        newScale = m_minimumScale;
    } else if (newScale > m_maximumScale) {
        factor = m_maximumScale / currentScale;
        newScale = m_maximumScale;
    }

    if (!qFuzzyCompare(currentScale, newScale)) {
        scale(factor, factor);
    }
}

void ZoomableGraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!scene()) {
        QGraphicsView::contextMenuEvent(event);
        return;
    }

    const QList<QGraphicsItem *> selectedItems = scene()->selectedItems();
    const bool containsItemUnderCursor = [this, &event]() {
        if (QGraphicsItem *item = itemAt(event->pos()))
            return item->isSelected();
        return false;
    }();

    const auto hasMultipleItemTypes = [](const QList<QGraphicsItem *> &items) {
        bool hasVertex = false;
        bool hasLine = false;
        bool hasPolygon = false;

        for (QGraphicsItem *item : items) {
            if (!item)
                continue;

            const int type = item->type();
            if (type == QGraphicsEllipseItem::Type && item->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
                hasVertex = true;
            } else if (type == QGraphicsLineItem::Type) {
                hasLine = true;
            } else if (type == QGraphicsPolygonItem::Type) {
                hasPolygon = true;
            }
        }

        const int typeCount = static_cast<int>(hasVertex) + static_cast<int>(hasLine) + static_cast<int>(hasPolygon);
        return typeCount > 1;
    };

    const bool multipleTypesSelected = hasMultipleItemTypes(selectedItems);

    if (QGraphicsItem *itemUnderCursor = itemAt(event->pos())) {
        const int itemType = itemUnderCursor->type();
        const bool isSelectableVertex = itemType == QGraphicsEllipseItem::Type
                                        && itemUnderCursor->flags().testFlag(QGraphicsItem::ItemIsSelectable);

        if (itemType == QGraphicsLineItem::Type) {
            QMenu menu(this);
            QAction *deleteSelectedLinesAction = nullptr;
            QAction *createPolygonFromLinesAction = nullptr;
            QAction *deleteAllSelectedItemsAction = nullptr;

            QList<QGraphicsItem *> selectedLines;
            for (QGraphicsItem *selectedItem : selectedItems) {
                if (!selectedItem)
                    continue;

                if (selectedItem->type() == QGraphicsLineItem::Type)
                    selectedLines.append(selectedItem);
            }

            if (selectedLines.size() > 1 && selectedLines.contains(itemUnderCursor))
                deleteSelectedLinesAction = menu.addAction(tr("Delete selected lines"));

            if (selectedLines.size() >= 3 && selectedLines.contains(itemUnderCursor))
                createPolygonFromLinesAction = menu.addAction(tr("Create polygon from lines"));

            if (multipleTypesSelected && containsItemUnderCursor)
                deleteAllSelectedItemsAction = menu.addAction(tr("Delete all selected items"));

            QAction *deleteLineAction = menu.addAction(tr("Delete line"));
            QAction *selectedAction = menu.exec(event->globalPos());

            if (selectedAction == deleteLineAction) {
                emit deleteLineRequested(itemUnderCursor);
            } else if (selectedAction == deleteSelectedLinesAction && !selectedLines.isEmpty()) {
                emit deleteLinesRequested(selectedLines);
            } else if (selectedAction == createPolygonFromLinesAction && !selectedLines.isEmpty()) {
                emit createPolygonFromLinesRequested(selectedLines);
            } else if (selectedAction == deleteAllSelectedItemsAction && !selectedItems.isEmpty()) {
                emit deleteItemsRequested(selectedItems);
            }
            return;
        }

        if (itemType == QGraphicsPolygonItem::Type) {
            QMenu menu(this);
            QAction *deleteSelectedPolygonsAction = nullptr;
            QAction *deleteAllSelectedItemsAction = nullptr;

            QList<QGraphicsItem *> selectedPolygons;
            for (QGraphicsItem *selectedItem : selectedItems) {
                if (!selectedItem)
                    continue;

                if (selectedItem->type() == QGraphicsPolygonItem::Type)
                    selectedPolygons.append(selectedItem);
            }

            if (selectedPolygons.size() > 1 && selectedPolygons.contains(itemUnderCursor))
                deleteSelectedPolygonsAction = menu.addAction(tr("Delete selected polygons"));

            if (multipleTypesSelected && containsItemUnderCursor)
                deleteAllSelectedItemsAction = menu.addAction(tr("Delete all selected items"));

            QAction *deletePolygonAction = menu.addAction(tr("Delete polygon"));
            QAction *selectedAction = menu.exec(event->globalPos());

            if (selectedAction == deletePolygonAction) {
                emit deletePolygonRequested(itemUnderCursor);
            } else if (selectedAction == deleteSelectedPolygonsAction && !selectedPolygons.isEmpty()) {
                emit deletePolygonsRequested(selectedPolygons);
            } else if (selectedAction == deleteAllSelectedItemsAction && !selectedItems.isEmpty()) {
                emit deleteItemsRequested(selectedItems);
            }
            return;
        }

        if (isSelectableVertex) {
            QMenu menu(this);
            QAction *createLineAction = nullptr;
            QAction *createPolygonAction = nullptr;
            QAction *deleteSelectedVerticesAction = nullptr;
            QAction *deleteAllSelectedItemsAction = nullptr;
            QList<QGraphicsItem *> selectedVertices;

            for (QGraphicsItem *selectedItem : selectedItems) {
                if (!selectedItem)
                    continue;

                const bool isSelectedVertex = selectedItem->type() == QGraphicsEllipseItem::Type
                                               && selectedItem->flags().testFlag(QGraphicsItem::ItemIsSelectable);

                if (isSelectedVertex)
                    selectedVertices.append(selectedItem);
            }

            if (selectedVertices.size() == 2 && selectedVertices.contains(itemUnderCursor))
                createLineAction = menu.addAction(tr("Create line between vertices"));

            if (selectedVertices.size() >= 3 && selectedVertices.contains(itemUnderCursor))
                createPolygonAction = menu.addAction(tr("Create polygon from vertices"));

            if (selectedVertices.size() > 1 && selectedVertices.contains(itemUnderCursor))
                deleteSelectedVerticesAction = menu.addAction(tr("Delete selected vertices"));

            if (multipleTypesSelected && containsItemUnderCursor)
                deleteAllSelectedItemsAction = menu.addAction(tr("Delete all selected items"));

            QAction *deleteVertexAction = menu.addAction(tr("Delete vertex"));
            QAction *selectedAction = menu.exec(event->globalPos());

            if (selectedAction == deleteVertexAction) {
                emit deleteVertexRequested(itemUnderCursor);
            } else if (selectedAction == createLineAction && selectedVertices.size() == 2) {
                emit createLineRequested(selectedVertices.at(0), selectedVertices.at(1));
            } else if (selectedAction == createPolygonAction && selectedVertices.size() >= 3) {
                emit createPolygonRequested(selectedVertices);
            } else if (selectedAction == deleteSelectedVerticesAction && !selectedVertices.isEmpty()) {
                emit deleteVerticesRequested(selectedVertices);
            } else if (selectedAction == deleteAllSelectedItemsAction && !selectedItems.isEmpty()) {
                emit deleteItemsRequested(selectedItems);
            }
            return;
        }

        if (itemType != QGraphicsPixmapItem::Type) {
            QGraphicsView::contextMenuEvent(event);
            return;
        }
    }

    const QPointF scenePosition = mapToScene(event->pos());

    QMenu menu(this);
    QAction *addVertexAction = menu.addAction(tr("Add a new vertex"));
    QAction *selectedAction = menu.exec(event->globalPos());

    if (selectedAction == addVertexAction)
        emit addVertexRequested(scenePosition);
}

void ZoomableGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && scene() && scene()->selectedItems().isEmpty()) {
        QGraphicsItem *itemUnderCursor = itemAt(event->pos());
        const bool isBackgroundItem = !itemUnderCursor
                                      || (itemUnderCursor->type() == QGraphicsPixmapItem::Type
                                          && !itemUnderCursor->flags().testFlag(QGraphicsItem::ItemIsSelectable));

        if (isBackgroundItem) {
            m_isPanning = true;
            m_lastMousePosition = event->pos();
            viewport()->setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void ZoomableGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        if (!(event->buttons() & Qt::LeftButton)) {
            m_isPanning = false;
            viewport()->unsetCursor();
            QGraphicsView::mouseMoveEvent(event);
            return;
        }

        const QPoint delta = event->pos() - m_lastMousePosition;
        m_lastMousePosition = event->pos();

        if (QScrollBar *horizontalBar = horizontalScrollBar())
            horizontalBar->setValue(horizontalBar->value() - delta.x());
        if (QScrollBar *verticalBar = verticalScrollBar())
            verticalBar->setValue(verticalBar->value() - delta.y());

        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void ZoomableGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isPanning) {
        m_isPanning = false;
        viewport()->unsetCursor();
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}
