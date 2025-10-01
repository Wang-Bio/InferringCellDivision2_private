#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointF>
#include <memory>
#include <vector>

class QGraphicsScene;
class ZoomableGraphicsView;
class Vertex;
class QGraphicsPixmapItem;

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
    void on_actionAdd_Vertex_triggered();
    void on_actionDelete_Vertex_triggered();
    void on_actionDelete_All_Vertices_triggered();
    void on_actionCustom_Canvas_triggered();

private:
    Vertex *createVertex(const QPointF &position);
    void deleteVertex(Vertex *vertex);
    int nextAvailableId() const;
    void sortVerticesById();

    Ui::MainWindow *ui;
    QGraphicsScene *m_scene = nullptr;
    std::vector<std::unique_ptr<Vertex>> m_vertices;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
};
#endif // MAINWINDOW_H
