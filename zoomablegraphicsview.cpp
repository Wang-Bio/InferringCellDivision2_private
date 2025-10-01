#include "zoomablegraphicsview.h"

#include <QContextMenuEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPolygonItem>
#include <QList>
#include <QMenu>
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

    if (QGraphicsItem *itemUnderCursor = itemAt(event->pos())) {
        const int itemType = itemUnderCursor->type();
        const bool isSelectableVertex = itemType == QGraphicsEllipseItem::Type
                                        && itemUnderCursor->flags().testFlag(QGraphicsItem::ItemIsSelectable);

        if (itemType == QGraphicsLineItem::Type) {
            QMenu menu(this);
            QAction *deleteLineAction = menu.addAction(tr("Delete line"));
            QAction *selectedAction = menu.exec(event->globalPos());

            if (selectedAction == deleteLineAction)
                emit deleteLineRequested(itemUnderCursor);
            return;
        }

        if (itemType == QGraphicsPolygonItem::Type) {
            QMenu menu(this);
            QAction *deletePolygonAction = menu.addAction(tr("Delete polygon"));
            QAction *selectedAction = menu.exec(event->globalPos());

            if (selectedAction == deletePolygonAction)
                emit deletePolygonRequested(itemUnderCursor);
            return;
        }

        if (isSelectableVertex) {
            QMenu menu(this);
            QAction *createLineAction = nullptr;
            QAction *createPolygonAction = nullptr;
            QList<QGraphicsItem *> selectedVertices;

            if (scene()) {
                const QList<QGraphicsItem *> selectedItems = scene()->selectedItems();
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
            }

            QAction *deleteVertexAction = menu.addAction(tr("Delete vertex"));
            QAction *selectedAction = menu.exec(event->globalPos());

            if (selectedAction == deleteVertexAction) {
                emit deleteVertexRequested(itemUnderCursor);
            } else if (selectedAction == createLineAction && selectedVertices.size() == 2) {
                emit createLineRequested(selectedVertices.at(0), selectedVertices.at(1));
            } else if (selectedAction == createPolygonAction && selectedVertices.size() >= 3) {
                emit createPolygonRequested(selectedVertices);
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
