#ifndef VERTEX_H
#define VERTEX_H

#include <QPointF>

class QGraphicsScene;
class QGraphicsEllipseItem;

class Vertex
{
public:
    Vertex(int id, const QPointF &position, QGraphicsScene *scene, qreal radius = 6.0);
    ~Vertex();

    int id() const;
    QPointF position() const;
    void setPosition(const QPointF &position);

private:
    void updateGraphicsItem();

    int m_id;
    QPointF m_position;
    QGraphicsScene *m_scene;
    QGraphicsEllipseItem *m_item;
    qreal m_radius;
};

#endif // VERTEX_H
