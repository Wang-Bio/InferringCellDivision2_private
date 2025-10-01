#ifndef POLYGON_H
#define POLYGON_H

#include <QColor>
#include <vector>

class QGraphicsItem;
class QGraphicsScene;
class Vertex;
class Line;

class Polygon
{
public:
    Polygon(int id,
            std::vector<Vertex *> vertices,
            std::vector<Line *> lines,
            QGraphicsScene *scene);
    ~Polygon();

    int id() const;
    const std::vector<Vertex *> &vertices() const;
    const std::vector<Line *> &lines() const;
    QGraphicsItem *graphicsItem() const;

    void updateShape();
    bool involvesVertex(const Vertex *vertex) const;
    bool involvesLine(const Line *line) const;
    void removeLine(Line *line);

private:
    void attachToVertices();
    void detachFromVertices();
    void attachToLines();
    void detachFromLines();
    void initializeGraphicsItem();

    int m_id = -1;
    std::vector<Vertex *> m_vertices;
    std::vector<Line *> m_lines;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsItem *m_item = nullptr;
    QColor m_color;
};

#endif // POLYGON_H
