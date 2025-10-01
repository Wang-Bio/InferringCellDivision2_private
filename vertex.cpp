#include "vertex.h"

#include "line.h"
#include "polygon.h"

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>
#include <QRectF>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QVariant>
#include <algorithm>

class VertexGraphicsItem : public QGraphicsEllipseItem
{
public:
    VertexGraphicsItem(Vertex *vertex, qreal radius)
        : QGraphicsEllipseItem(-radius, -radius, radius * 2, radius * 2)
        , m_vertex(vertex)
    {
        setBrush(QBrush(Qt::red));
        setPen(QPen(Qt::NoPen));
        setZValue(1.0);
        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
        setFlag(QGraphicsItem::ItemIgnoresTransformations);
        setCursor(Qt::OpenHandCursor);
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == QGraphicsItem::ItemScenePositionHasChanged && m_vertex) {
            m_vertex->updatePositionFromGraphicsItem(value.toPointF());
        }
        return QGraphicsEllipseItem::itemChange(change, value);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        setCursor(Qt::ClosedHandCursor);
        QGraphicsEllipseItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        setCursor(Qt::OpenHandCursor);
        QGraphicsEllipseItem::mouseReleaseEvent(event);
    }

private:
    Vertex *m_vertex = nullptr;
};

Vertex::Vertex(int id, const QPointF &position, QGraphicsScene *scene, qreal radius)
    : m_id(id)
    , m_position(position)
    , m_scene(scene)
    , m_item(nullptr)
    , m_radius(radius)
{
    if (m_scene) {
        m_item = new VertexGraphicsItem(this, m_radius);
        m_scene->addItem(m_item);
        updateGraphicsItem();
    }
}

Vertex::~Vertex()
{
    if (m_scene && m_item) {
        m_scene->removeItem(m_item);
    }
    delete m_item;
    m_item = nullptr;
}

int Vertex::id() const
{
    return m_id;
}

QPointF Vertex::position() const
{
    return m_position;
}

void Vertex::setPosition(const QPointF &position)
{
    m_position = position;
    updateGraphicsItem();
    notifyConnectedLines();
}

QGraphicsItem *Vertex::graphicsItem() const
{
    return m_item;
}

void Vertex::addConnectedLine(Line *line)
{
    if (!line)
        return;

    if (std::find(m_lines.begin(), m_lines.end(), line) == m_lines.end())
        m_lines.push_back(line);
}

void Vertex::removeConnectedLine(Line *line)
{
    const auto it = std::remove(m_lines.begin(), m_lines.end(), line);
    if (it != m_lines.end())
        m_lines.erase(it, m_lines.end());
}

void Vertex::addConnectedPolygon(Polygon *polygon)
{
    if (!polygon)
        return;

    if (std::find(m_polygons.begin(), m_polygons.end(), polygon) == m_polygons.end())
        m_polygons.push_back(polygon);
}

void Vertex::removeConnectedPolygon(Polygon *polygon)
{
    const auto it = std::remove(m_polygons.begin(), m_polygons.end(), polygon);
    if (it != m_polygons.end())
        m_polygons.erase(it, m_polygons.end());
}

void Vertex::updateGraphicsItem()
{
    if (!m_item)
        return;

    m_item->setRect(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
    m_item->setPos(m_position);
}

void Vertex::updatePositionFromGraphicsItem(const QPointF &position)
{
    m_position = position;
    notifyConnectedLines();
}

void Vertex::notifyConnectedLines()
{
    for (Line *line : m_lines) {
        if (line)
            line->updatePosition();
    }

    for (Polygon *polygon : m_polygons) {
        if (polygon)
            polygon->updateShape();
    }
}
