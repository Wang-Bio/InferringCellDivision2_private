#ifndef ZOOMABLEGRAPHICSVIEW_H
#define ZOOMABLEGRAPHICSVIEW_H

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QList>
#include <QPoint>
#include <QPointF>

class QContextMenuEvent;
class QMouseEvent;
class ZoomableGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ZoomableGraphicsView(QWidget *parent = nullptr);

signals:
    void addVertexRequested(const QPointF &scenePosition);
    void deleteVertexRequested(QGraphicsItem *vertexItem);
    void deleteLineRequested(QGraphicsItem *lineItem);
    void deleteVerticesRequested(const QList<QGraphicsItem *> &vertexItems);
    void deleteLinesRequested(const QList<QGraphicsItem *> &lineItems);
    void deletePolygonsRequested(const QList<QGraphicsItem *> &polygonItems);
    void deleteItemsRequested(const QList<QGraphicsItem *> &items);
    void createLineRequested(QGraphicsItem *firstVertexItem, QGraphicsItem *secondVertexItem);
    void deletePolygonRequested(QGraphicsItem *polygonItem);
    void createPolygonRequested(const QList<QGraphicsItem *> &vertexItems);
    void createPolygonFromLinesRequested(const QList<QGraphicsItem *> &lineItems);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void applyZoomFactor(double factor);
    double m_minimumScale = 0.1;
    double m_maximumScale = 10.0;
    bool m_isPanning = false;
    QPoint m_lastMousePosition;
};

#endif // ZOOMABLEGRAPHICSVIEW_H
