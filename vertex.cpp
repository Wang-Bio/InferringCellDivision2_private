#include "vertex.h"

#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>
#include <QRectF>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QVariant>

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
}

QGraphicsItem *Vertex::graphicsItem() const
{
    return m_item;
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
}
