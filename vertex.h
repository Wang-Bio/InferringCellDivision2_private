#ifndef VERTEX_H
#define VERTEX_H

#include <QPointF>
#include <vector>

class QGraphicsItem;
class QGraphicsScene;
class VertexGraphicsItem;
class Line;
class Polygon;

class Vertex
{
public:
    Vertex(int id, const QPointF &position, QGraphicsScene *scene, qreal radius = 6.0);
    ~Vertex();

    int id() const;
    QPointF position() const;
    void setPosition(const QPointF &position);
    QGraphicsItem *graphicsItem() const;
    void addConnectedLine(Line *line);
    void removeConnectedLine(Line *line);
    void addConnectedPolygon(Polygon *polygon);
    void removeConnectedPolygon(Polygon *polygon);
    const std::vector<Line *> &connectedLines() const;
    const std::vector<Polygon *> &connectedPolygons() const;

private:
    void updateGraphicsItem();
    void updatePositionFromGraphicsItem(const QPointF &position);
    void notifyConnectedLines();

    int m_id;
    QPointF m_position;
    QGraphicsScene *m_scene;
    VertexGraphicsItem *m_item;
    qreal m_radius;
    std::vector<Line *> m_lines;
    std::vector<Polygon *> m_polygons;

    friend class VertexGraphicsItem;
};

#endif // VERTEX_H
