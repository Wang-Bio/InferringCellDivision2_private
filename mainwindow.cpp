#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "vertex.h"
#include "zoomablegraphicsview.h"
#include "line.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QInputDialog>
#include <QMessageBox>
#include <QRadioButton>
#include <QPainter>
#include <QRectF>
#include <QStringList>
#include <QGraphicsPixmapItem>
#include <QColor>
#include <QPixmap>
#include <QImage>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <algorithm>
#include <set>
#include <utility>

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

    if (auto *zoomableView = qobject_cast<ZoomableGraphicsView *>(ui->graphicsView)) {
        connect(zoomableView,
                &ZoomableGraphicsView::addVertexRequested,
                this,
                &MainWindow::handleAddVertexFromContextMenu);
        connect(zoomableView,
                &ZoomableGraphicsView::deleteVertexRequested,
                this,
                &MainWindow::handleDeleteVertexFromContextMenu);
        connect(zoomableView,
                &ZoomableGraphicsView::deleteLineRequested,
                this,
                &MainWindow::handleDeleteLineFromContextMenu);
    }
}

MainWindow::~MainWindow()
{
    m_lines.clear();
    m_vertices.clear();
    m_nextLineId = 0;

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
    return createVertexWithId(id, position);
}

Vertex *MainWindow::createVertexWithId(int id, const QPointF &position)
{
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

    std::vector<Line *> linesToDelete;
    linesToDelete.reserve(m_lines.size());
    for (const auto &line : m_lines) {
        if (line && line->involvesVertex(vertex))
            linesToDelete.push_back(line.get());
    }

    for (Line *line : linesToDelete)
        deleteLine(line);

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

Vertex *MainWindow::findVertexById(int id) const
{
    const auto it = std::find_if(m_vertices.begin(), m_vertices.end(),
                                 [id](const std::unique_ptr<Vertex> &vertex) {
                                     return vertex && vertex->id() == id;
                                 });

    if (it != m_vertices.end())
        return it->get();

    return nullptr;
}

Line *MainWindow::createLine(Vertex *startVertex, Vertex *endVertex)
{
    Line *line = createLineWithId(m_nextLineId, startVertex, endVertex);
    if (line)
        ++m_nextLineId;
    return line;
}

Line *MainWindow::createLineWithId(int id, Vertex *startVertex, Vertex *endVertex)
{
    if (!startVertex || !endVertex || startVertex == endVertex || !m_scene)
        return nullptr;

    auto line = std::make_unique<Line>(id, startVertex, endVertex, m_scene);
    Line *linePtr = line.get();
    m_lines.push_back(std::move(line));
    return linePtr;
}

void MainWindow::deleteLine(Line *line)
{
    if (!line)
        return;

    const auto it = std::find_if(m_lines.begin(), m_lines.end(),
                                 [line](const std::unique_ptr<Line> &candidate) {
                                     return candidate.get() == line;
                                 });

    if (it != m_lines.end())
        m_lines.erase(it);
}

Line *MainWindow::findLineByGraphicsItem(const QGraphicsItem *item) const
{
    if (!item)
        return nullptr;

    const auto it = std::find_if(m_lines.begin(), m_lines.end(),
                                 [item](const std::unique_ptr<Line> &line) {
                                     return line && line->graphicsItem() == item;
                                 });

    if (it != m_lines.end())
        return it->get();

    return nullptr;
}

Line *MainWindow::findLineById(int id) const
{
    const auto it = std::find_if(m_lines.begin(), m_lines.end(),
                                 [id](const std::unique_ptr<Line> &line) {
                                     return line && line->id() == id;
                                 });

    if (it != m_lines.end())
        return it->get();

    return nullptr;
}

Line *MainWindow::findLineByVertices(Vertex *startVertex, Vertex *endVertex) const
{
    if (!startVertex || !endVertex)
        return nullptr;

    const auto it = std::find_if(m_lines.begin(), m_lines.end(),
                                 [startVertex, endVertex](const std::unique_ptr<Line> &line) {
                                     if (!line)
                                         return false;

                                     const bool sameDirection = line->startVertex() == startVertex && line->endVertex() == endVertex;
                                     const bool oppositeDirection = line->startVertex() == endVertex && line->endVertex() == startVertex;
                                     return sameDirection || oppositeDirection;
                                 });

    if (it != m_lines.end())
        return it->get();

    return nullptr;
}

void MainWindow::on_actionDelete_Image_triggered(){
    if (!m_scene)
        return;

    if (m_backgroundItem) {
        m_scene->removeItem(m_backgroundItem);
        delete m_backgroundItem;
        m_backgroundItem = nullptr;
    }

    m_scene->clearSelection();
    m_vertices.clear();
    m_lines.clear();
    m_nextLineId = 0;

    m_scene->setSceneRect(0.0, 0.0, 512.0, 512.0);
    ui->graphicsView->setSceneRect(m_scene->sceneRect());

    if (ui->label_2)
        ui->label_2->setText(tr("-"));
    if (ui->label_4)
        ui->label_4->setText(tr("-"));
    if (ui->label_canvas_size)
        ui->label_canvas_size->setText(tr("-"));

    resetSelectionLabels();
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

void MainWindow::on_actionAdd_Line_triggered()
{
    if (m_vertices.size() < 2) {
        QMessageBox::information(this,
                                 tr("Add Line"),
                                 tr("At least two vertices are required to add a line."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Line"));

    QFormLayout form(&dialog);

    auto *startSpinBox = new QSpinBox(&dialog);
    auto *endSpinBox = new QSpinBox(&dialog);

    int maxId = 0;
    for (const auto &vertex : m_vertices) {
        if (vertex)
            maxId = std::max(maxId, vertex->id());
    }

    startSpinBox->setRange(0, std::max(0, maxId));
    endSpinBox->setRange(0, std::max(0, maxId));

    form.addRow(tr("Start vertex ID:"), startSpinBox);
    form.addRow(tr("End vertex ID:"), endSpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal,
                               &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const int startId = startSpinBox->value();
    const int endId = endSpinBox->value();

    if (startId == endId) {
        QMessageBox::warning(this,
                              tr("Add Line"),
                              tr("The start and end vertices must be different."));
        return;
    }

    Vertex *startVertex = findVertexById(startId);
    Vertex *endVertex = findVertexById(endId);

    if (!startVertex || !endVertex) {
        QMessageBox::warning(this,
                              tr("Add Line"),
                              tr("One or both vertex IDs could not be found."));
        return;
    }

    if (findLineByVertices(startVertex, endVertex)) {
        QMessageBox::information(this,
                                 tr("Add Line"),
                                 tr("A line already exists between the selected vertices."));
        return;
    }

    Line *line = createLine(startVertex, endVertex);
    if (!line) {
        QMessageBox::warning(this,
                              tr("Add Line"),
                              tr("The line could not be created."));
        return;
    }

    if (QGraphicsItem *item = line->graphicsItem()) {
        m_scene->clearSelection();
        item->setSelected(true);
    }

    updateSelectionLabels(line);
}

void MainWindow::on_actionFind_Line_triggered()
{
    if (m_lines.empty()) {
        QMessageBox::information(this,
                                 tr("Find Line"),
                                 tr("There are no lines to select."));
        return;
    }

    int maxId = 0;
    bool hasLine = false;
    for (const auto &line : m_lines) {
        if (line) {
            maxId = std::max(maxId, line->id());
            hasLine = true;
        }
    }

    if (!hasLine) {
        QMessageBox::information(this,
                                 tr("Find Line"),
                                 tr("There are no lines to select."));
        return;
    }

    bool ok = false;
    const int idToFind = QInputDialog::getInt(this,
                                              tr("Find Line"),
                                              tr("Enter the line ID:"),
                                              0,
                                              0,
                                              std::max(0, maxId),
                                              1,
                                              &ok);

    if (!ok)
        return;

    Line *line = findLineById(idToFind);
    if (!line) {
        QMessageBox::warning(this,
                              tr("Find Line"),
                              tr("No line with ID %1 was found.").arg(idToFind));
        return;
    }

    if (m_scene)
        m_scene->clearSelection();

    if (QGraphicsItem *item = line->graphicsItem()) {
        item->setSelected(true);
        if (ui->graphicsView)
            ui->graphicsView->centerOn(item);
    }

    updateSelectionLabels(line);
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
        m_lines.clear();
        m_nextLineId = 0;
        resetSelectionLabels();
    }
}

void MainWindow::on_actionDelete_Line_triggered()
{
    if (m_lines.empty()) {
        QMessageBox::information(this,
                                 tr("Delete Line"),
                                 tr("There are no lines to delete."));
        return;
    }

    int maxId = 0;
    bool hasLine = false;
    for (const auto &line : m_lines) {
        if (line) {
            maxId = std::max(maxId, line->id());
            hasLine = true;
        }
    }

    if (!hasLine) {
        QMessageBox::information(this,
                                 tr("Delete Line"),
                                 tr("There are no lines to delete."));
        return;
    }

    bool ok = false;
    const int idToDelete = QInputDialog::getInt(this,
                                                tr("Delete Line"),
                                                tr("Enter the line ID:"),
                                                0,
                                                0,
                                                std::max(0, maxId),
                                                1,
                                                &ok);

    if (!ok)
        return;

    Line *line = findLineById(idToDelete);
    if (!line) {
        QMessageBox::warning(this,
                              tr("Delete Line"),
                              tr("No line with ID %1 was found.").arg(idToDelete));
        return;
    }

    if (m_scene)
        m_scene->clearSelection();

    deleteLine(line);
    resetSelectionLabels();
}

void MainWindow::on_actionDelete_All_Lines_triggered()
{
    if (m_lines.empty()) {
        QMessageBox::information(this,
                                 tr("Delete All Lines"),
                                 tr("There are no lines to delete."));
        return;
    }

    const auto reply = QMessageBox::question(this,
                                             tr("Delete All Lines"),
                                             tr("Are you sure you want to delete all lines?"),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (m_scene)
        m_scene->clearSelection();

    m_lines.clear();
    m_nextLineId = 0;
    resetSelectionLabels();
}

void MainWindow::on_actionFind_Vertex_triggered()
{
    if (m_vertices.empty()) {
        QMessageBox::information(this,
                                 tr("Find Vertex"),
                                 tr("There are no vertices to select."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Find Vertex"));

    auto *mainLayout = new QVBoxLayout(&dialog);

    auto *modeGroup = new QGroupBox(tr("Search Mode"), &dialog);
    auto *modeLayout = new QVBoxLayout(modeGroup);
    auto *idRadio = new QRadioButton(tr("By ID"), modeGroup);
    auto *positionRadio = new QRadioButton(tr("By Position"), modeGroup);
    idRadio->setChecked(true);
    modeLayout->addWidget(idRadio);
    modeLayout->addWidget(positionRadio);
    modeGroup->setLayout(modeLayout);

    auto *stack = new QStackedWidget(&dialog);

    auto *idWidget = new QWidget(&dialog);
    auto *idLayout = new QFormLayout(idWidget);
    auto *idSpinBox = new QSpinBox(idWidget);
    int maxId = 0;
    for (const auto &vertex : m_vertices) {
        maxId = std::max(maxId, vertex ? vertex->id() : 0);
    }
    idSpinBox->setRange(0, std::max(0, maxId));
    idLayout->addRow(tr("Vertex ID:"), idSpinBox);
    idWidget->setLayout(idLayout);
    stack->addWidget(idWidget);

    auto *positionWidget = new QWidget(&dialog);
    auto *positionLayout = new QFormLayout(positionWidget);
    auto *xSpinBox = new QDoubleSpinBox(positionWidget);
    auto *ySpinBox = new QDoubleSpinBox(positionWidget);
    auto *toleranceSpinBox = new QDoubleSpinBox(positionWidget);

    const QRectF sceneRect = m_scene ? m_scene->sceneRect() : QRectF();
    xSpinBox->setRange(sceneRect.left(), sceneRect.right());
    ySpinBox->setRange(sceneRect.top(), sceneRect.bottom());
    xSpinBox->setDecimals(2);
    ySpinBox->setDecimals(2);

    toleranceSpinBox->setRange(0.0, 1000.0);
    toleranceSpinBox->setDecimals(2);
    toleranceSpinBox->setSingleStep(0.1);
    toleranceSpinBox->setValue(0.5);

    positionLayout->addRow(tr("X position:"), xSpinBox);
    positionLayout->addRow(tr("Y position:"), ySpinBox);
    positionLayout->addRow(tr("Tolerance:"), toleranceSpinBox);
    positionWidget->setLayout(positionLayout);
    stack->addWidget(positionWidget);

    QObject::connect(idRadio, &QRadioButton::toggled, stack, [stack, idRadio](bool checked) {
        if (checked)
            stack->setCurrentIndex(0);
    });
    QObject::connect(positionRadio, &QRadioButton::toggled, stack, [stack](bool checked) {
        if (checked)
            stack->setCurrentIndex(1);
    });

    stack->setCurrentIndex(0);

    mainLayout->addWidget(modeGroup);
    mainLayout->addWidget(stack);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                           Qt::Horizontal,
                                           &dialog);
    mainLayout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    Vertex *selectedVertex = nullptr;

    const int idToFind = idSpinBox->value();
    const QPointF positionToFind(xSpinBox->value(), ySpinBox->value());
    const double tolerance = toleranceSpinBox->value();

    const auto findById = [idToFind](const std::unique_ptr<Vertex> &vertex) {
        return vertex && vertex->id() == idToFind;
    };

    auto it = std::find_if(m_vertices.begin(), m_vertices.end(), findById);
    if (it != m_vertices.end()) {
        selectedVertex = it->get();
    }

    if (!selectedVertex) {
        const auto findByPosition = [positionToFind, tolerance](const std::unique_ptr<Vertex> &vertex) {
            if (!vertex)
                return false;

            const QPointF vertexPos = vertex->graphicsItem() ? vertex->graphicsItem()->scenePos() : vertex->position();
            const qreal dx = vertexPos.x() - positionToFind.x();
            const qreal dy = vertexPos.y() - positionToFind.y();
            const qreal distanceSquared = dx * dx + dy * dy;
            return distanceSquared <= tolerance * tolerance;
        };

        it = std::find_if(m_vertices.begin(), m_vertices.end(), findByPosition);
        if (it != m_vertices.end())
            selectedVertex = it->get();
    }

    if (!selectedVertex) {
        QMessageBox::information(this,
                                 tr("Find Vertex"),
                                 tr("No vertex matched the provided criteria."));
        return;
    }

    if (m_scene) {
        m_scene->clearSelection();
        if (QGraphicsItem *item = selectedVertex->graphicsItem()) {
            item->setSelected(true);
            ui->graphicsView->centerOn(item);
        }
    }

    updateSelectionLabels(selectedVertex);
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

    QGraphicsItem *item = selectedItems.first();
    if (Vertex *vertex = findVertexByGraphicsItem(item)) {
        updateSelectionLabels(vertex);
    } else if (Line *line = findLineByGraphicsItem(item)) {
        updateSelectionLabels(line);
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

    QGraphicsItem *item = selectedItems.first();
    if (Vertex *vertex = findVertexByGraphicsItem(item)) {
        updateSelectionLabels(vertex);
    } else if (Line *line = findLineByGraphicsItem(item)) {
        updateSelectionLabels(line);
    }
}

void MainWindow::handleAddVertexFromContextMenu(const QPointF &scenePosition)
{
    if (!m_scene)
        return;

    if (!m_scene->sceneRect().contains(scenePosition))
        return;

    Vertex *vertex = createVertex(scenePosition);
    if (!vertex)
        return;

    if (QGraphicsItem *item = vertex->graphicsItem()) {
        m_scene->clearSelection();
        item->setSelected(true);
    }

    updateSelectionLabels(vertex);
}

void MainWindow::handleDeleteVertexFromContextMenu(QGraphicsItem *vertexItem)
{
    if (!m_scene || !vertexItem)
        return;

    if (Vertex *vertex = findVertexByGraphicsItem(vertexItem)) {
        m_scene->clearSelection();
        deleteVertex(vertex);
        resetSelectionLabels();
    }
}

void MainWindow::handleDeleteLineFromContextMenu(QGraphicsItem *lineItem)
{
    if (!m_scene || !lineItem)
        return;

    if (Line *line = findLineByGraphicsItem(lineItem)) {
        m_scene->clearSelection();
        deleteLine(line);
        resetSelectionLabels();
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

void MainWindow::updateSelectionLabels(Line *line)
{
    if (!line)
        return;

    ui->label_selected_item->setText(tr("line"));
    ui->label_selected_item_id->setText(QString::number(line->id()));

    auto formatVertex = [](Vertex *vertex) {
        if (!vertex)
            return QStringLiteral("-");

        const QGraphicsItem *graphicsItem = vertex->graphicsItem();
        const QPointF pos = graphicsItem ? graphicsItem->scenePos() : vertex->position();
        const QString xText = QString::number(pos.x(), 'f', 2);
        const QString yText = QString::number(pos.y(), 'f', 2);
        return QStringLiteral("v%1 (%2, %3)").arg(vertex->id()).arg(xText, yText);
    };

    const QString startText = formatVertex(line->startVertex());
    const QString endText = formatVertex(line->endVertex());
    ui->label_selected_item_pos->setText(tr("%1 → %2").arg(startText, endText));
}

void MainWindow::on_actionCell_Contour_Image_triggered()
{
    if (!m_scene)
        return;

    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          tr("Open Image"),
                                                          QString(),
                                                          tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff);;All Files (*)"));

    if (filePath.isEmpty())
        return;

    const QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this,
                             tr("Open Image"),
                             tr("Failed to load image: %1").arg(QDir::toNativeSeparators(filePath)));
        return;
    }

    if (m_backgroundItem) {
        m_scene->removeItem(m_backgroundItem);
        delete m_backgroundItem;
        m_backgroundItem = nullptr;
    }

    m_scene->clearSelection();
    m_vertices.clear();

    m_backgroundItem = m_scene->addPixmap(pixmap);
    if (m_backgroundItem) {
        m_backgroundItem->setZValue(-1.0);
        m_backgroundItem->setPos(0.0, 0.0);
    }

    m_scene->setSceneRect(pixmap.rect());
    ui->graphicsView->setSceneRect(m_scene->sceneRect());

    const QFileInfo fileInfo(filePath);
    if (ui->label_2)
        ui->label_2->setText(QDir::toNativeSeparators(fileInfo.absolutePath()));
    if (ui->label_4)
        ui->label_4->setText(fileInfo.fileName());
    if (ui->label_canvas_size)
        ui->label_canvas_size->setText(tr("%1 x %2").arg(pixmap.width()).arg(pixmap.height()));

    resetSelectionLabels();
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

void MainWindow::on_actionExport_Vertex_Only_triggered()
{
    if (!m_scene)
        return;

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          tr("Export Vertices"),
                                                          QString(),
                                                          tr("JSON Files (*.json);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QJsonArray vertexArray;
    for (const auto &vertex : m_vertices) {
        if (!vertex)
            continue;

        const QPointF position = vertex->position();
        QJsonObject vertexObject;
        vertexObject.insert(QStringLiteral("id"), vertex->id());
        vertexObject.insert(QStringLiteral("x"), position.x());
        vertexObject.insert(QStringLiteral("y"), position.y());
        vertexArray.append(vertexObject);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("vertices"), vertexArray);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this,
                              tr("Export Vertices"),
                              tr("Failed to open %1 for writing.").arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    const QJsonDocument document(rootObject);
    file.write(document.toJson(QJsonDocument::Indented));
    file.close();
}

void MainWindow::on_actionImport_Vertex_Only_triggered()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Import Vertices"),
                                                          QString(),
                                                          tr("JSON Files (*.json);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,
                              tr("Import Vertices"),
                              tr("Failed to open %1 for reading.").arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (document.isNull() || !document.isObject()) {
        const QString errorMessage = parseError.error == QJsonParseError::NoError
                                         ? tr("The file does not contain a valid JSON object.")
                                         : parseError.errorString();
        QMessageBox::warning(this,
                              tr("Import Vertices"),
                              tr("Failed to parse %1: %2").arg(QDir::toNativeSeparators(fileName), errorMessage));
        return;
    }

    const QJsonObject rootObject = document.object();
    const QJsonValue verticesValue = rootObject.value(QStringLiteral("vertices"));
    if (!verticesValue.isArray()) {
        QMessageBox::warning(this,
                              tr("Import Vertices"),
                              tr("The JSON file must contain an array named 'vertices'."));
        return;
    }

    const QJsonArray verticesArray = verticesValue.toArray();
    std::vector<std::pair<int, QPointF>> importedVertices;
    importedVertices.reserve(verticesArray.size());
    std::set<int> vertexIds;

    for (const QJsonValue &value : verticesArray) {
        if (!value.isObject()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices"),
                                  tr("Each vertex entry must be a JSON object."));
            return;
        }

        const QJsonObject vertexObject = value.toObject();
        const QJsonValue idValue = vertexObject.value(QStringLiteral("id"));
        const QJsonValue xValue = vertexObject.value(QStringLiteral("x"));
        const QJsonValue yValue = vertexObject.value(QStringLiteral("y"));

        if (!idValue.isDouble() || !xValue.isDouble() || !yValue.isDouble()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices"),
                                  tr("Vertex entries must contain numeric 'id', 'x', and 'y' fields."));
            return;
        }

        const int id = idValue.toInt();
        if (!vertexIds.insert(id).second) {
            QMessageBox::warning(this,
                                  tr("Import Vertices"),
                                  tr("Duplicate vertex id %1 detected.").arg(id));
            return;
        }

        const qreal x = xValue.toDouble();
        const qreal y = yValue.toDouble();
        importedVertices.emplace_back(id, QPointF(x, y));
    }

    m_scene->clearSelection();
    m_lines.clear();
    m_vertices.clear();
    m_nextLineId = 0;

    for (const auto &vertexData : importedVertices)
        createVertexWithId(vertexData.first, vertexData.second);

    resetSelectionLabels();
}

void MainWindow::on_actionExport_Vertex_Line_triggered()
{
    if (!m_scene)
        return;

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          tr("Export Vertices and Lines"),
                                                          QString(),
                                                          tr("JSON Files (*.json);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QJsonArray vertexArray;
    for (const auto &vertex : m_vertices) {
        if (!vertex)
            continue;

        const QPointF position = vertex->position();
        QJsonObject vertexObject;
        vertexObject.insert(QStringLiteral("id"), vertex->id());
        vertexObject.insert(QStringLiteral("x"), position.x());
        vertexObject.insert(QStringLiteral("y"), position.y());
        vertexArray.append(vertexObject);
    }

    QJsonArray lineArray;
    for (const auto &line : m_lines) {
        if (!line)
            continue;

        const Vertex *startVertex = line->startVertex();
        const Vertex *endVertex = line->endVertex();
        if (!startVertex || !endVertex)
            continue;

        QJsonObject lineObject;
        lineObject.insert(QStringLiteral("id"), line->id());
        lineObject.insert(QStringLiteral("startVertexId"), startVertex->id());
        lineObject.insert(QStringLiteral("endVertexId"), endVertex->id());
        lineArray.append(lineObject);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("vertices"), vertexArray);
    rootObject.insert(QStringLiteral("lines"), lineArray);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this,
                              tr("Export Vertices and Lines"),
                              tr("Failed to open %1 for writing.").arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    const QJsonDocument document(rootObject);
    file.write(document.toJson(QJsonDocument::Indented));
    file.close();
}

void MainWindow::on_actionImport_Vertex_Line_triggered()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Import Vertices and Lines"),
                                                          QString(),
                                                          tr("JSON Files (*.json);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,
                              tr("Import Vertices and Lines"),
                              tr("Failed to open %1 for reading.").arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (document.isNull() || !document.isObject()) {
        const QString errorMessage = parseError.error == QJsonParseError::NoError
                                         ? tr("The file does not contain a valid JSON object.")
                                         : parseError.errorString();
        QMessageBox::warning(this,
                              tr("Import Vertices and Lines"),
                              tr("Failed to parse %1: %2").arg(QDir::toNativeSeparators(fileName), errorMessage));
        return;
    }

    const QJsonObject rootObject = document.object();
    const QJsonValue verticesValue = rootObject.value(QStringLiteral("vertices"));
    const QJsonValue linesValue = rootObject.value(QStringLiteral("lines"));

    if (!verticesValue.isArray() || !linesValue.isArray()) {
        QMessageBox::warning(this,
                              tr("Import Vertices and Lines"),
                              tr("The JSON file must contain 'vertices' and 'lines' arrays."));
        return;
    }

    const QJsonArray verticesArray = verticesValue.toArray();
    const QJsonArray linesArray = linesValue.toArray();

    struct VertexImportData {
        int id;
        QPointF position;
    };
    struct LineImportData {
        int id;
        int startId;
        int endId;
    };

    std::vector<VertexImportData> importedVertices;
    importedVertices.reserve(verticesArray.size());
    std::set<int> vertexIds;

    for (const QJsonValue &value : verticesArray) {
        if (!value.isObject()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Each vertex entry must be a JSON object."));
            return;
        }

        const QJsonObject vertexObject = value.toObject();
        const QJsonValue idValue = vertexObject.value(QStringLiteral("id"));
        const QJsonValue xValue = vertexObject.value(QStringLiteral("x"));
        const QJsonValue yValue = vertexObject.value(QStringLiteral("y"));

        if (!idValue.isDouble() || !xValue.isDouble() || !yValue.isDouble()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Vertex entries must contain numeric 'id', 'x', and 'y' fields."));
            return;
        }

        const int id = idValue.toInt();
        if (!vertexIds.insert(id).second) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Duplicate vertex id %1 detected.").arg(id));
            return;
        }

        const qreal x = xValue.toDouble();
        const qreal y = yValue.toDouble();
        importedVertices.push_back({id, QPointF(x, y)});
    }

    std::vector<LineImportData> importedLines;
    importedLines.reserve(linesArray.size());
    std::set<int> lineIds;
    int maxLineId = -1;

    for (const QJsonValue &value : linesArray) {
        if (!value.isObject()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Each line entry must be a JSON object."));
            return;
        }

        const QJsonObject lineObject = value.toObject();
        const QJsonValue idValue = lineObject.value(QStringLiteral("id"));
        const QJsonValue startValue = lineObject.value(QStringLiteral("startVertexId"));
        const QJsonValue endValue = lineObject.value(QStringLiteral("endVertexId"));

        if (!idValue.isDouble() || !startValue.isDouble() || !endValue.isDouble()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Line entries must contain numeric 'id', 'startVertexId', and 'endVertexId' fields."));
            return;
        }

        const int id = idValue.toInt();
        const int startId = startValue.toInt();
        const int endId = endValue.toInt();

        if (!lineIds.insert(id).second) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Duplicate line id %1 detected.").arg(id));
            return;
        }

        if (startId == endId) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Line %1 references the same vertex for both ends.").arg(id));
            return;
        }

        if (!vertexIds.count(startId) || !vertexIds.count(endId)) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Line %1 references undefined vertices.").arg(id));
            return;
        }

        importedLines.push_back({id, startId, endId});
        maxLineId = std::max(maxLineId, id);
    }

    m_scene->clearSelection();
    m_lines.clear();
    m_vertices.clear();
    m_nextLineId = 0;

    for (const auto &vertexData : importedVertices)
        createVertexWithId(vertexData.id, vertexData.position);

    for (const auto &lineData : importedLines) {
        Vertex *startVertex = findVertexById(lineData.startId);
        Vertex *endVertex = findVertexById(lineData.endId);
        if (!createLineWithId(lineData.id, startVertex, endVertex)) {
            QMessageBox::warning(this,
                                  tr("Import Vertices and Lines"),
                                  tr("Failed to create line %1.").arg(lineData.id));
            break;
        }
    }

    m_nextLineId = maxLineId >= 0 ? maxLineId + 1 : 0;
    resetSelectionLabels();
}
