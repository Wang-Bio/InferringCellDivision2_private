#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "vertex.h"
#include "zoomablegraphicsview.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>
#include <QRectF>
#include <QStringList>
#include <QGraphicsPixmapItem>
#include <QColor>
#include <QPixmap>
#include <QImage>
#include <QSplitter>

#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 512, 512);

    if (auto *splitter = qobject_cast<QSplitter *>(ui->graphicsView->parentWidget())) {
        const int index = splitter->indexOf(ui->graphicsView);
        if (index >= 0) {
            auto *originalView = ui->graphicsView;
            auto *zoomableView = new ZoomableGraphicsView;
            zoomableView->setObjectName(originalView->objectName());
            zoomableView->setSizePolicy(originalView->sizePolicy());
            zoomableView->setMinimumSize(originalView->minimumSize());
            zoomableView->setMaximumSize(originalView->maximumSize());
            splitter->replaceWidget(index, zoomableView);
            originalView->deleteLater();
            ui->graphicsView = zoomableView;
        }
    }


    resetSelectionLabels();

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSceneSelectionChanged);
    connect(m_scene, &QGraphicsScene::changed, this, &MainWindow::onSceneChanged);

    ui->graphicsView->setScene(m_scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
}

MainWindow::~MainWindow()
{
    m_vertices.clear();

    if (m_scene && m_backgroundItem) {
        m_scene->removeItem(m_backgroundItem);
        delete m_backgroundItem;
        m_backgroundItem = nullptr;
    }

    delete ui;
}

Vertex *MainWindow::createVertex(const QPointF &position)
{
    const int id = nextAvailableId();
    auto vertex = std::make_unique<Vertex>(id, position, m_scene);
    Vertex *vertexPtr = vertex.get();
    m_vertices.push_back(std::move(vertex));
    sortVerticesById();
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

int MainWindow::nextAvailableId() const
{
    int id = 0;
    while (std::find_if(m_vertices.begin(), m_vertices.end(),
                        [id](const std::unique_ptr<Vertex> &candidate) {
                            return candidate->id() == id;
                        }) != m_vertices.end()) {
        ++id;
    }
    return id;
}

void MainWindow::sortVerticesById()
{
    std::sort(m_vertices.begin(), m_vertices.end(),
              [](const std::unique_ptr<Vertex> &lhs, const std::unique_ptr<Vertex> &rhs) {
                  return lhs->id() < rhs->id();
              });
}

void MainWindow::on_actionAdd_Vertex_triggered()
{
    if (!m_scene)
        return;

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Vertex"));

    QFormLayout form(&dialog);

    auto *xSpinBox = new QDoubleSpinBox(&dialog);
    auto *ySpinBox = new QDoubleSpinBox(&dialog);

    const QRectF sceneRect = m_scene->sceneRect();
    xSpinBox->setRange(sceneRect.left(), sceneRect.right());
    ySpinBox->setRange(sceneRect.top(), sceneRect.bottom());
    xSpinBox->setDecimals(2);
    ySpinBox->setDecimals(2);

    form.addRow(tr("X position:"), xSpinBox);
    form.addRow(tr("Y position:"), ySpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal,
                               &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        createVertex(QPointF(xSpinBox->value(), ySpinBox->value()));
    }
}

void MainWindow::on_actionDelete_Vertex_triggered()
{
    if (m_vertices.empty()) {
        QMessageBox::information(this,
                                 tr("Delete Vertex"),
                                 tr("There are no vertices to delete."));
        return;
    }

    QStringList vertexLabels;
    vertexLabels.reserve(static_cast<int>(m_vertices.size()));
    for (std::size_t i = 0; i < m_vertices.size(); ++i) {
        const QPointF position = m_vertices[i]->position();
        vertexLabels.append(tr("Vertex %1 (%2, %3)")
                                .arg(m_vertices[i]->id())
                                .arg(position.x(), 0, 'f', 2)
                                .arg(position.y(), 0, 'f', 2));
    }

    bool ok = false;
    const QString choice = QInputDialog::getItem(this,
                                                 tr("Delete Vertex"),
                                                 tr("Select the vertex to delete:"),
                                                 vertexLabels,
                                                 0,
                                                 false,
                                                 &ok);

    if (!ok || choice.isEmpty())
        return;

    const int selectedIndex = vertexLabels.indexOf(choice);
    if (selectedIndex < 0)
        return;

    deleteVertex(m_vertices[static_cast<std::size_t>(selectedIndex)].get());
}

void MainWindow::on_actionDelete_All_Vertices_triggered()
{
    const auto reply = QMessageBox::question(this,
                                             tr("Delete All Vertices"),
                                             tr("Are you sure you want to delete all vertices?"),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_vertices.clear();
        resetSelectionLabels();
    }
}

void MainWindow::onSceneSelectionChanged()
{
    if (!m_scene)
        return;

    const auto selectedItems = m_scene->selectedItems();
    if (selectedItems.isEmpty()) {
        resetSelectionLabels();
        return;
    }

    Vertex *vertex = findVertexByGraphicsItem(selectedItems.first());
    if (vertex) {
        updateSelectionLabels(vertex);
    } else {
        resetSelectionLabels();
    }
}

void MainWindow::onSceneChanged(const QList<QRectF> & /*region*/)
{
    if (!m_scene)
        return;

    const auto selectedItems = m_scene->selectedItems();
    if (selectedItems.size() != 1)
        return;

    if (Vertex *vertex = findVertexByGraphicsItem(selectedItems.first())) {
        updateSelectionLabels(vertex);
    }
}

Vertex *MainWindow::findVertexByGraphicsItem(const QGraphicsItem *item) const
{
    if (!item)
        return nullptr;

    const auto it = std::find_if(m_vertices.begin(), m_vertices.end(),
                                 [item](const std::unique_ptr<Vertex> &vertex) {
                                     return vertex && vertex->graphicsItem() == item;
                                 });

    if (it != m_vertices.end())
        return it->get();

    return nullptr;
}

void MainWindow::resetSelectionLabels()
{
    ui->label_selected_item->setText(tr("-"));
    ui->label_selected_item_id->setText(tr("-"));
    ui->label_selected_item_pos->setText(tr("-"));
}

void MainWindow::updateSelectionLabels(Vertex *vertex)
{
    if (!vertex)
        return;

    ui->label_selected_item->setText(tr("vertex"));
    ui->label_selected_item_id->setText(QString::number(vertex->id()));

    const QGraphicsItem *graphicsItem = vertex->graphicsItem();
    const QPointF pos = graphicsItem ? graphicsItem->scenePos() : vertex->position();

    const QString xText = QString::number(pos.x(), 'f', 2);
    const QString yText = QString::number(pos.y(), 'f', 2);
    ui->label_selected_item_pos->setText(tr("(%1, %2)").arg(xText, yText));
}



void MainWindow::on_actionCustom_Canvas_triggered()
{
    if (!m_scene)
        return;

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Custom Canvas"));

    auto *layout = new QFormLayout(&dialog);

    auto *widthSpinBox = new QSpinBox(&dialog);
    widthSpinBox->setRange(1, 10000);
    widthSpinBox->setValue(static_cast<int>(m_scene->sceneRect().width()));
    layout->addRow(tr("Canvas width (px):"), widthSpinBox);

    auto *heightSpinBox = new QSpinBox(&dialog);
    heightSpinBox->setRange(1, 10000);
    heightSpinBox->setValue(static_cast<int>(m_scene->sceneRect().height()));
    layout->addRow(tr("Canvas height (px):"), heightSpinBox);

    auto *redSpinBox = new QSpinBox(&dialog);
    redSpinBox->setRange(0, 255);
    redSpinBox->setValue(0);
    layout->addRow(tr("Red (0-255):"), redSpinBox);

    auto *greenSpinBox = new QSpinBox(&dialog);
    greenSpinBox->setRange(0, 255);
    greenSpinBox->setValue(0);
    layout->addRow(tr("Green (0-255):"), greenSpinBox);

    auto *blueSpinBox = new QSpinBox(&dialog);
    blueSpinBox->setRange(0, 255);
    blueSpinBox->setValue(0);
    layout->addRow(tr("Blue (0-255):"), blueSpinBox);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                           Qt::Horizontal,
                                           &dialog);
    layout->addRow(buttonBox);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const int width = widthSpinBox->value();
    const int height = heightSpinBox->value();
    const int red = redSpinBox->value();
    const int green = greenSpinBox->value();
    const int blue = blueSpinBox->value();

    if (m_backgroundItem) {
        m_scene->removeItem(m_backgroundItem);
        delete m_backgroundItem;
        m_backgroundItem = nullptr;
    }

    QImage backgroundImage(width, height, QImage::Format_RGB32);
    backgroundImage.fill(QColor(red, green, blue));

    const QPixmap pixmap = QPixmap::fromImage(backgroundImage);
    m_backgroundItem = m_scene->addPixmap(pixmap);
    if (m_backgroundItem) {
        m_backgroundItem->setZValue(-1.0);
        m_backgroundItem->setPos(0.0, 0.0);
    }

    m_scene->setSceneRect(0.0, 0.0, static_cast<qreal>(width), static_cast<qreal>(height));
    ui->graphicsView->setSceneRect(m_scene->sceneRect());
    if (ui->label_canvas_size) {
        ui->label_canvas_size->setText(tr("%1 x %2").arg(width).arg(height));
    }

    m_vertices.clear();
}
