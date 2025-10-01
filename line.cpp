#include "line.h"

#include "vertex.h"
#include "polygon.h"

#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPen>
#include <QVariant>
#include <algorithm>

namespace
{
class LineGraphicsItem : public QGraphicsLineItem
{
public:
    explicit LineGraphicsItem(Line *line)
        : QGraphicsLineItem()
        , m_line(line)
    {
        QPen pen(QColor(0, 170, 0));
        pen.setWidthF(2.0);
        setPen(pen);
        setZValue(0.5);
        setFlag(QGraphicsItem::ItemIsSelectable);
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == QGraphicsItem::ItemScenePositionHasChanged && m_line) {
            m_line->updatePosition();
        }
        return QGraphicsLineItem::itemChange(change, value);
    }

private:
    Line *m_line = nullptr;
};
} // namespace

Line::Line(int id, Vertex *startVertex, Vertex *endVertex, QGraphicsScene *scene)
    : m_id(id)
    , m_startVertex(startVertex)
    , m_endVertex(endVertex)
    , m_scene(scene)
{
    if (m_scene) {
        m_item = new LineGraphicsItem(this);
        m_scene->addItem(m_item);
    }

    if (m_startVertex)
        m_startVertex->addConnectedLine(this);
    if (m_endVertex)
        m_endVertex->addConnectedLine(this);

    updatePosition();
}

Line::~Line()
{
    auto polygons = m_polygons;
    for (Polygon *polygon : polygons) {
        if (polygon)
            polygon->removeLine(this);
    }
    m_polygons.clear();

    if (m_startVertex)
        m_startVertex->removeConnectedLine(this);
    if (m_endVertex)
        m_endVertex->removeConnectedLine(this);

    if (m_scene && m_item)
        m_scene->removeItem(m_item);

    delete m_item;
    m_item = nullptr;
}

int Line::id() const
{
    return m_id;
}

Vertex *Line::startVertex() const
{
    return m_startVertex;
}

Vertex *Line::endVertex() const
{
    return m_endVertex;
}

QGraphicsItem *Line::graphicsItem() const
{
    return m_item;
}

void Line::updatePosition()
{
    if (!m_item)
        return;

    const QPointF startPos = m_startVertex ? m_startVertex->position() : QPointF();
    const QPointF endPos = m_endVertex ? m_endVertex->position() : QPointF();

    static_cast<QGraphicsLineItem *>(m_item)->setLine(QLineF(startPos, endPos));
}

bool Line::involvesVertex(const Vertex *vertex) const
{
    return vertex && (vertex == m_startVertex || vertex == m_endVertex);
}

void Line::addConnectedPolygon(Polygon *polygon)
{
    if (!polygon)
        return;

    if (std::find(m_polygons.begin(), m_polygons.end(), polygon) == m_polygons.end())
        m_polygons.push_back(polygon);
}

void Line::removeConnectedPolygon(Polygon *polygon)
{
    const auto it = std::remove(m_polygons.begin(), m_polygons.end(), polygon);
    if (it != m_polygons.end())
        m_polygons.erase(it, m_polygons.end());
}

const std::vector<Polygon *> &Line::connectedPolygons() const
{
    return m_polygons;
}
