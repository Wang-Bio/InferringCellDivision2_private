#ifndef ZOOMABLEGRAPHICSVIEW_H
#define ZOOMABLEGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QPointF>

class QContextMenuEvent;
class QGraphicsItem;

class ZoomableGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ZoomableGraphicsView(QWidget *parent = nullptr);

signals:
    void addVertexRequested(const QPointF &scenePosition);
    void deleteVertexRequested(QGraphicsItem *vertexItem);
    void deleteLineRequested(QGraphicsItem *lineItem);
    void createLineRequested(QGraphicsItem *firstVertexItem, QGraphicsItem *secondVertexItem);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void applyZoomFactor(double factor);
    double m_minimumScale = 0.1;
    double m_maximumScale = 10.0;
};

#endif // ZOOMABLEGRAPHICSVIEW_H
