#ifndef ZOOMABLEGRAPHICSVIEW_H
#define ZOOMABLEGRAPHICSVIEW_H

#include <QGraphicsView>

class ZoomableGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ZoomableGraphicsView(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    void applyZoomFactor(double factor);
    double m_minimumScale = 0.1;
    double m_maximumScale = 10.0;
};

#endif // ZOOMABLEGRAPHICSVIEW_H
