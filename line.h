#ifndef LINE_H
#define LINE_H

#include <QColor>
#include <QPointF>
#include <vector>

class QGraphicsItem;
class QGraphicsScene;
class Vertex;
class Polygon;

class Line
{
public:
    Line(int id, Vertex *startVertex, Vertex *endVertex, QGraphicsScene *scene);
    ~Line();

    int id() const;
    Vertex *startVertex() const;
    Vertex *endVertex() const;
    QGraphicsItem *graphicsItem() const;

    void updatePosition();
    bool involvesVertex(const Vertex *vertex) const;
    void addConnectedPolygon(Polygon *polygon);
    void removeConnectedPolygon(Polygon *polygon);
    const std::vector<Polygon *> &connectedPolygons() const;

private:
    int m_id = -1;
    Vertex *m_startVertex = nullptr;
    Vertex *m_endVertex = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsItem *m_item = nullptr;
    std::vector<Polygon *> m_polygons;
};

#endif // LINE_H
