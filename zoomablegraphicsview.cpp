#include "zoomablegraphicsview.h"

#include <QContextMenuEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
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
        const bool isVertexItem = itemType == QGraphicsEllipseItem::Type
                                  && itemUnderCursor->flags().testFlag(QGraphicsItem::ItemIsSelectable);

        if (isVertexItem || itemType != QGraphicsPixmapItem::Type) {
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
