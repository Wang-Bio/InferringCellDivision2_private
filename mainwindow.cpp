#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "vertex.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 800, 600);
    ui->graphicsView->setScene(m_scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
}

MainWindow::~MainWindow()
{
    m_vertices.clear();
    delete ui;
}

Vertex *MainWindow::createVertex(const QPointF &position)
{
    auto vertex = std::make_unique<Vertex>(position, m_scene);
    Vertex *vertexPtr = vertex.get();
    m_vertices.push_back(std::move(vertex));
    return vertexPtr;
}

void MainWindow::deleteVertex(Vertex *vertex)
{
    if (!vertex)
        return;

    const auto it = std::find_if(m_vertices.begin(), m_vertices.end(),
                                 [vertex](const std::unique_ptr<Vertex> &candidate) {
                                     return candidate.get() == vertex;
                                 });

    if (it != m_vertices.end()) {
        m_vertices.erase(it);
    }
}

QPointF MainWindow::nextVertexPosition() const
{
    const qreal spacing = 30.0;
    const int index = static_cast<int>(m_vertices.size());
    const qreal x = 50.0 + (index % 10) * spacing;
    const qreal y = 50.0 + (index / 10) * spacing;
    return QPointF(x, y);
}

void MainWindow::on_actionAdd_Vertex_triggered()
{
    createVertex(nextVertexPosition());
}

void MainWindow::on_actionDelete_Vertex_triggered()
{
    if (m_vertices.empty())
        return;

    Vertex *vertex = m_vertices.back().get();
    deleteVertex(vertex);
}

void MainWindow::on_actionDelete_All_Vertices_triggered()
{
    m_vertices.clear();
}
