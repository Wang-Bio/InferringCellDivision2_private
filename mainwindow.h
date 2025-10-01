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
class Line;
class Polygon;
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
    void on_actionFind_Line_triggered();
    void on_actionFind_Polygon_triggered();
    void on_actionAdd_Line_triggered();
    void on_actionDelete_Line_triggered();
    void on_actionDelete_Polygon_triggered();
    void on_actionDelete_All_Lines_triggered();
    void on_actionAdd_Polygon_triggered();
    void on_actionCell_Contour_Image_triggered();
    void on_actionCustom_Canvas_triggered();
    void on_actionExport_Vertex_Only_triggered();
    void on_actionImport_Vertex_Only_triggered();
    void on_actionExport_Vertex_Line_triggered();
    void on_actionImport_Vertex_Line_triggered();
    void on_actionSnapShot_All_triggered();
    void on_actionSnapShot_View_triggered();
    void onSceneSelectionChanged();
    void onSceneChanged(const QList<QRectF> &region);
    void handleAddVertexFromContextMenu(const QPointF &scenePosition);
    void handleDeleteVertexFromContextMenu(QGraphicsItem *vertexItem);
    void handleDeleteLineFromContextMenu(QGraphicsItem *lineItem);
    void handleCreateLineFromContextMenu(QGraphicsItem *firstVertexItem, QGraphicsItem *secondVertexItem);

private:
    Vertex *createVertex(const QPointF &position);
    Vertex *createVertexWithId(int id, const QPointF &position);
    void deleteVertex(Vertex *vertex);
    int nextAvailableId() const;
    void sortVerticesById();
    Vertex *findVertexByGraphicsItem(const QGraphicsItem *item) const;
    Vertex *findVertexById(int id) const;
    int nextAvailableLineId() const;
    int nextAvailablePolygonId() const;
    Line *createLine(Vertex *startVertex, Vertex *endVertex);
    Line *createLineWithId(int id, Vertex *startVertex, Vertex *endVertex);
    void deleteLine(Line *line);
    Line *findLineByGraphicsItem(const QGraphicsItem *item) const;
    Line *findLineById(int id) const;
    Line *findLineByVertices(Vertex *startVertex, Vertex *endVertex) const;
    Polygon *findPolygonById(int id) const;
    void resetSelectionLabels();
    void updateSelectionLabels(Vertex *vertex);
    void updateSelectionLabels(Line *line);
    void updateSelectionLabels(Polygon *polygon);

    Ui::MainWindow *ui;
    QGraphicsScene *m_scene = nullptr;
    std::vector<std::unique_ptr<Vertex>> m_vertices;
    std::vector<std::unique_ptr<Line>> m_lines;
    std::vector<std::unique_ptr<Polygon>> m_polygons;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
    int m_nextLineId = 0;
    int m_nextPolygonId = 0;

    Polygon *createPolygon(const std::vector<Vertex *> &vertices, const std::vector<Line *> &lines);
    void deletePolygon(Polygon *polygon);
    Polygon *findPolygonByGraphicsItem(const QGraphicsItem *item) const;
    bool orderLinesIntoPolygon(const std::vector<Line *> &inputLines,
                               std::vector<Line *> &orderedLines,
                               std::vector<Vertex *> &orderedVertices) const;
};
#endif // MAINWINDOW_H
