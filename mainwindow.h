#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QPointF>
#include <QRectF>
#include <memory>
#include <vector>

class QGraphicsScene;
class ZoomableGraphicsView;
class Vertex;
class QGraphicsPixmapItem;
class QGraphicsItem;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionDelete_Image_triggered();
    void on_actionAdd_Vertex_triggered();
    void on_actionDelete_Vertex_triggered();
    void on_actionDelete_All_Vertices_triggered();
    void on_actionFind_Vertex_triggered();
    void on_actionCell_Contour_Image_triggered();
    void on_actionCustom_Canvas_triggered();
    void onSceneSelectionChanged();
    void onSceneChanged(const QList<QRectF> &region);
    void handleAddVertexFromContextMenu(const QPointF &scenePosition);

private:
    Vertex *createVertex(const QPointF &position);
    void deleteVertex(Vertex *vertex);
    int nextAvailableId() const;
    void sortVerticesById();
    Vertex *findVertexByGraphicsItem(const QGraphicsItem *item) const;
    void resetSelectionLabels();
    void updateSelectionLabels(Vertex *vertex);

    Ui::MainWindow *ui;
    QGraphicsScene *m_scene = nullptr;
    std::vector<std::unique_ptr<Vertex>> m_vertices;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
};
#endif // MAINWINDOW_H
