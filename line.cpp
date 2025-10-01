#include "line.h"

#include "vertex.h"

#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPen>
#include <QVariant>

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
