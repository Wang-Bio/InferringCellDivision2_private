#include "polygon.h"

#include "line.h"
#include "vertex.h"

#include <QGraphicsPolygonItem>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QPolygonF>
#include <QRandomGenerator>

#include <algorithm>
#include <utility>

namespace {
class PolygonGraphicsItem : public QGraphicsPolygonItem
{
public:
    explicit PolygonGraphicsItem(const QColor &color)
    {
        QPen pen(color.darker(150));
        pen.setWidthF(1.5);
        pen.setCosmetic(true);
        setPen(pen);

        QBrush brush(color);
        setBrush(brush);

        setZValue(0.25);
        setFlag(QGraphicsItem::ItemIsSelectable);
    }
};
} // namespace

Polygon::Polygon(int id,
                 std::vector<Vertex *> vertices,
                 std::vector<Line *> lines,
                 QGraphicsScene *scene)
    : m_id(id)
    , m_vertices(std::move(vertices))
    , m_lines(std::move(lines))
    , m_scene(scene)
{
    const int red = QRandomGenerator::global()->bounded(256);
    const int green = QRandomGenerator::global()->bounded(256);
    const int blue = QRandomGenerator::global()->bounded(256);
    m_color = QColor(red, green, blue);
    m_color.setAlpha(90);

    initializeGraphicsItem();
    attachToVertices();
    attachToLines();
    updateShape();
}

Polygon::~Polygon()
{
    detachFromLines();
    detachFromVertices();

    if (m_scene && m_item)
        m_scene->removeItem(m_item);

    delete m_item;
    m_item = nullptr;
}

int Polygon::id() const
{
    return m_id;
}

const std::vector<Vertex *> &Polygon::vertices() const
{
    return m_vertices;
}

const std::vector<Line *> &Polygon::lines() const
{
    return m_lines;
}

QGraphicsItem *Polygon::graphicsItem() const
{
    return m_item;
}

void Polygon::updateShape()
{
    if (!m_item)
        return;

    QPolygonF polygon;
    polygon.reserve(static_cast<int>(m_vertices.size()));

    for (Vertex *vertex : m_vertices) {
        if (!vertex)
            continue;

        const QGraphicsItem *vertexItem = vertex->graphicsItem();
        const QPointF position = vertexItem ? vertexItem->scenePos() : vertex->position();
        polygon << position;
    }

    static_cast<QGraphicsPolygonItem *>(m_item)->setPolygon(polygon);
}

bool Polygon::involvesVertex(const Vertex *vertex) const
{
    return std::find(m_vertices.begin(), m_vertices.end(), vertex) != m_vertices.end();
}

bool Polygon::involvesLine(const Line *line) const
{
    return std::find(m_lines.begin(), m_lines.end(), line) != m_lines.end();
}

void Polygon::removeLine(Line *line)
{
    if (!line)
        return;

    const auto it = std::remove(m_lines.begin(), m_lines.end(), line);
    if (it != m_lines.end()) {
        m_lines.erase(it, m_lines.end());
        line->removeConnectedPolygon(this);
    }
}

void Polygon::attachToVertices()
{
    for (Vertex *vertex : m_vertices) {
        if (vertex)
            vertex->addConnectedPolygon(this);
    }
}

void Polygon::detachFromVertices()
{
    for (Vertex *vertex : m_vertices) {
        if (vertex)
            vertex->removeConnectedPolygon(this);
    }
}

void Polygon::attachToLines()
{
    for (Line *line : m_lines) {
        if (line)
            line->addConnectedPolygon(this);
    }
}

void Polygon::detachFromLines()
{
    for (Line *line : m_lines) {
        if (line)
            line->removeConnectedPolygon(this);
    }
}

void Polygon::initializeGraphicsItem()
{
    if (!m_scene)
        return;

    m_item = new PolygonGraphicsItem(m_color);
    m_scene->addItem(m_item);
}
