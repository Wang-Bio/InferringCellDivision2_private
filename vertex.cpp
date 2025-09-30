#include "vertex.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QRectF>

Vertex::Vertex(int id, const QPointF &position, QGraphicsScene *scene, qreal radius)
    : m_id(id)
    , m_position(position)
    , m_scene(scene)
    , m_item(nullptr)
    , m_radius(radius)
{
    if (m_scene) {
        m_item = m_scene->addEllipse(0, 0, 0, 0, QPen(Qt::NoPen), QBrush(Qt::yellow));
        if (m_item) {
            m_item->setZValue(1.0);
            updateGraphicsItem();
        }
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

void Vertex::updateGraphicsItem()
{
    if (!m_item)
        return;

    const QRectF rect(m_position.x() - m_radius,
                      m_position.y() - m_radius,
                      m_radius * 2,
                      m_radius * 2);
    m_item->setRect(rect);
}
