#include "zoomablegraphicsview.h"

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
