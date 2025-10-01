#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "vertex.h"
#include "zoomablegraphicsview.h"
#include "line.h"
#include "polygon.h"

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
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QWidget>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLineEdit>
#include <QRegularExpression>

#include <algorithm>
#include <set>
#include <utility>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace {
struct SnapshotOptions
{
    QString filePath;
    int quality = 90;
};

qreal signedArea(const std::vector<Vertex *> &vertices)
{
    if (vertices.size() < 3)
        return 0.0;

    qreal area = 0.0;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const Vertex *current = vertices[i];
        const Vertex *next = vertices[(i + 1) % vertices.size()];
        if (!current || !next)
            continue;

        const QPointF currentPos = current->position();
        const QPointF nextPos = next->position();
        area += (currentPos.x() * nextPos.y()) - (nextPos.x() * currentPos.y());
    }

    return area * 0.5;
}

void ensureCounterClockwise(std::vector<Vertex *> &vertices, std::vector<Line *> &lines)
{
    if (vertices.size() < 3 || lines.size() != vertices.size())
        return;

    if (signedArea(vertices) < 0) {
        std::vector<Vertex *> reversedVertices;
        reversedVertices.reserve(vertices.size());
        reversedVertices.push_back(vertices.front());
        for (std::size_t i = vertices.size(); i-- > 1;)
            reversedVertices.push_back(vertices[i]);

        std::vector<Line *> reversedLines;
        reversedLines.reserve(lines.size());
        for (auto it = lines.rbegin(); it != lines.rend(); ++it)
            reversedLines.push_back(*it);

        vertices = std::move(reversedVertices);
        lines = std::move(reversedLines);
    }
}

std::vector<Vertex *> sortVerticesCounterClockwise(const std::vector<Vertex *> &vertices)
{
    std::vector<Vertex *> sorted;
    sorted.reserve(vertices.size());
    for (Vertex *vertex : vertices) {
        if (vertex)
            sorted.push_back(vertex);
    }

    if (sorted.size() < 3)
        return sorted;

    qreal sumX = 0.0;
    qreal sumY = 0.0;
    for (Vertex *vertex : sorted) {
        const QPointF pos = vertex->position();
        sumX += pos.x();
        sumY += pos.y();
    }
    const qreal invCount = 1.0 / static_cast<qreal>(sorted.size());
    const QPointF centroid(sumX * invCount, sumY * invCount);

    std::sort(sorted.begin(), sorted.end(), [&centroid](Vertex *lhs, Vertex *rhs) {
        const QPointF lhsPos = lhs->position();
        const QPointF rhsPos = rhs->position();
        const qreal lhsAngle = std::atan2(lhsPos.y() - centroid.y(), lhsPos.x() - centroid.x());
        const qreal rhsAngle = std::atan2(rhsPos.y() - centroid.y(), rhsPos.x() - centroid.x());
        if (lhsAngle < rhsAngle)
            return true;
        if (lhsAngle > rhsAngle)
            return false;

        const qreal lhsDx = lhsPos.x() - centroid.x();
        const qreal lhsDy = lhsPos.y() - centroid.y();
        const qreal rhsDx = rhsPos.x() - centroid.x();
        const qreal rhsDy = rhsPos.y() - centroid.y();
        const qreal lhsDistSq = lhsDx * lhsDx + lhsDy * lhsDy;
        const qreal rhsDistSq = rhsDx * rhsDx + rhsDy * rhsDy;
        return lhsDistSq < rhsDistSq;
    });

    if (signedArea(sorted) < 0) {
        std::vector<Vertex *> reversed;
        reversed.reserve(sorted.size());
        reversed.push_back(sorted.front());
        for (std::size_t i = sorted.size(); i-- > 1;)
            reversed.push_back(sorted[i]);
        sorted = std::move(reversed);
    }

    return sorted;
}

std::optional<SnapshotOptions> requestSnapshotOptions(QWidget *parent,
                                                     const QString &title,
                                                     const QString &defaultFileName)
{
    QFileDialog dialog(parent, title);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setDefaultSuffix(QStringLiteral("png"));
    dialog.selectFile(defaultFileName);
    dialog.setNameFilters({QObject::tr("PNG Image (*.png)"),
                          QObject::tr("JPEG Image (*.jpg *.jpeg)"),
                          QObject::tr("Bitmap Image (*.bmp)"),
                          QObject::tr("TIFF Image (*.tif *.tiff)")});

    auto *qualityLabel = new QLabel(QObject::tr("Quality (0-100):"), &dialog);
    auto *qualitySpinBox = new QSpinBox(&dialog);
    qualitySpinBox->setRange(0, 100);
    qualitySpinBox->setValue(90);

    auto *qualityLayout = new QHBoxLayout;
    qualityLayout->setContentsMargins(0, 0, 0, 0);
    qualityLayout->addWidget(qualityLabel);
    qualityLayout->addWidget(qualitySpinBox);

    auto *qualityContainer = new QWidget(&dialog);
    qualityContainer->setLayout(qualityLayout);

    if (QLayout *layout = dialog.layout()) {
        if (auto *gridLayout = qobject_cast<QGridLayout *>(layout)) {
            const int row = gridLayout->rowCount();
            gridLayout->addWidget(qualityContainer, row, 0, 1, gridLayout->columnCount());
        } else {
            layout->addWidget(qualityContainer);
        }
    }

    if (dialog.exec() != QDialog::Accepted)
        return std::nullopt;

    const QStringList selectedFiles = dialog.selectedFiles();
    if (selectedFiles.isEmpty())
        return std::nullopt;

    SnapshotOptions options;
    options.filePath = selectedFiles.first();
    options.quality = qualitySpinBox->value();
    return options;
}
} // namespace

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
        connect(zoomableView,
                &ZoomableGraphicsView::createLineRequested,
                this,
                &MainWindow::handleCreateLineFromContextMenu);
        connect(zoomableView,
                &ZoomableGraphicsView::deletePolygonRequested,
                this,
                &MainWindow::handleDeletePolygonFromContextMenu);
        connect(zoomableView,
                &ZoomableGraphicsView::createPolygonRequested,
                this,
                &MainWindow::handleCreatePolygonFromContextMenu);
    }
}

MainWindow::~MainWindow()
{
    m_polygons.clear();
    m_lines.clear();
    m_vertices.clear();
    m_nextLineId = 0;
    m_nextPolygonId = 0;

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

    std::vector<Polygon *> polygonsToDelete;
    polygonsToDelete.reserve(m_polygons.size());
    for (const auto &polygon : m_polygons) {
        if (polygon && polygon->involvesVertex(vertex))
            polygonsToDelete.push_back(polygon.get());
    }

    for (Polygon *polygon : polygonsToDelete)
        deletePolygon(polygon);

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

int MainWindow::nextAvailableLineId() const
{
    int id = 0;
    while (std::find_if(m_lines.begin(), m_lines.end(),
                        [id](const std::unique_ptr<Line> &candidate) {
                            return candidate && candidate->id() == id;
                        }) != m_lines.end()) {
        ++id;
    }
    return id;
}

int MainWindow::nextAvailablePolygonId() const
{
    int id = 0;
    while (std::find_if(m_polygons.begin(), m_polygons.end(),
                        [id](const std::unique_ptr<Polygon> &candidate) {
                            return candidate && candidate->id() == id;
                        }) != m_polygons.end()) {
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
    const int id = nextAvailableLineId();
    Line *line = createLineWithId(id, startVertex, endVertex);
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

Polygon *MainWindow::createPolygon(const std::vector<Vertex *> &vertices, const std::vector<Line *> &lines)
{
    const int id = nextAvailablePolygonId();
    return createPolygonWithId(id, vertices, lines);
}

Polygon *MainWindow::createPolygonWithId(int id,
                                         const std::vector<Vertex *> &vertices,
                                         const std::vector<Line *> &lines)
{
    if (!m_scene || vertices.size() < 3 || lines.size() < 3)
        return nullptr;

    if (vertices.size() != lines.size())
        return nullptr;

    if (findPolygonById(id))
        return nullptr;

    std::vector<Line *> lineCopy = lines;
    std::vector<Vertex *> vertexCopy;

    std::vector<Line *> orderedLines;
    std::vector<Vertex *> orderedVertices;
    if (!orderLinesIntoPolygon(lineCopy, orderedLines, orderedVertices))
        return nullptr;

    lineCopy = std::move(orderedLines);
    vertexCopy = std::move(orderedVertices);

    auto polygon = std::make_unique<Polygon>(id, std::move(vertexCopy), std::move(lineCopy), m_scene);
    Polygon *polygonPtr = polygon.get();
    m_polygons.push_back(std::move(polygon));
    return polygonPtr;
}

void MainWindow::deleteLine(Line *line)
{
    if (!line)
        return;

    std::vector<Polygon *> polygonsToDelete;
    polygonsToDelete.reserve(m_polygons.size());
    for (const auto &polygon : m_polygons) {
        if (polygon && polygon->involvesLine(line))
            polygonsToDelete.push_back(polygon.get());
    }

    for (Polygon *polygon : polygonsToDelete)
        deletePolygon(polygon);

    const auto it = std::find_if(m_lines.begin(), m_lines.end(),
                                 [line](const std::unique_ptr<Line> &candidate) {
                                     return candidate.get() == line;
                                 });

    if (it != m_lines.end())
        m_lines.erase(it);
}

void MainWindow::deletePolygon(Polygon *polygon)
{
    if (!polygon)
        return;

    const auto it = std::find_if(m_polygons.begin(),
                                 m_polygons.end(),
                                 [polygon](const std::unique_ptr<Polygon> &candidate) {
                                     return candidate.get() == polygon;
                                 });

    if (it != m_polygons.end())
        m_polygons.erase(it);
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

Polygon *MainWindow::findPolygonByGraphicsItem(const QGraphicsItem *item) const
{
    if (!item)
        return nullptr;

    const auto it = std::find_if(m_polygons.begin(),
                                 m_polygons.end(),
                                 [item](const std::unique_ptr<Polygon> &polygon) {
                                     return polygon && polygon->graphicsItem() == item;
                                 });

    if (it != m_polygons.end())
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

Polygon *MainWindow::findPolygonById(int id) const
{
    const auto it = std::find_if(m_polygons.begin(), m_polygons.end(),
                                 [id](const std::unique_ptr<Polygon> &polygon) {
                                     return polygon && polygon->id() == id;
                                 });

    if (it != m_polygons.end())
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

bool MainWindow::orderLinesIntoPolygon(const std::vector<Line *> &inputLines,
                                       std::vector<Line *> &orderedLines,
                                       std::vector<Vertex *> &orderedVertices) const
{
    if (inputLines.size() < 3)
        return false;

    std::unordered_set<Line *> uniqueLines;
    uniqueLines.reserve(inputLines.size());

    std::unordered_map<Vertex *, std::vector<Line *>> adjacency;
    adjacency.reserve(inputLines.size() * 2);

    for (Line *line : inputLines) {
        if (!line)
            return false;

        Vertex *start = line->startVertex();
        Vertex *end = line->endVertex();
        if (!start || !end)
            return false;

        if (!uniqueLines.insert(line).second)
            return false;

        adjacency[start].push_back(line);
        adjacency[end].push_back(line);
    }

    for (auto &entry : adjacency) {
        if (entry.second.size() != 2)
            return false;

        if (entry.second.front() == entry.second.back())
            return false;
    }

    orderedLines.clear();
    orderedVertices.clear();
    orderedLines.reserve(inputLines.size());
    orderedVertices.reserve(inputLines.size());

    Line *initialLine = inputLines.front();
    Vertex *startVertex = initialLine->startVertex();
    Vertex *currentVertex = startVertex;
    Line *previousLine = nullptr;

    std::unordered_set<Line *> usedLines;
    usedLines.reserve(inputLines.size());

    do {
        const auto adjacencyIt = adjacency.find(currentVertex);
        if (adjacencyIt == adjacency.end())
            return false;

        const auto &adjacentLines = adjacencyIt->second;
        Line *nextLine = nullptr;

        if (!previousLine) {
            for (Line *candidate : adjacentLines) {
                if (!usedLines.count(candidate)) {
                    nextLine = candidate;
                    break;
                }
            }
        } else {
            nextLine = adjacentLines.front() == previousLine ? adjacentLines.back() : adjacentLines.front();
            if (usedLines.count(nextLine))
                return false;
        }

        if (!nextLine)
            return false;

        orderedLines.push_back(nextLine);
        usedLines.insert(nextLine);
        orderedVertices.push_back(currentVertex);

        currentVertex = nextLine->startVertex() == currentVertex ? nextLine->endVertex() : nextLine->startVertex();
        previousLine = nextLine;

    } while (currentVertex != startVertex && orderedLines.size() <= inputLines.size());

    if (currentVertex != startVertex)
        return false;

    if (orderedLines.size() != inputLines.size())
        return false;

    if (orderedVertices.size() != orderedLines.size())
        return false;

    ensureCounterClockwise(orderedVertices, orderedLines);

    return true;
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
    m_polygons.clear();
    m_nextLineId = 0;
    m_nextPolygonId = 0;

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

void MainWindow::on_actionAdd_Polygon_triggered()
{
    if (!m_scene)
        return;

    if (m_vertices.size() < 3 && m_lines.size() < 3) {
        QMessageBox::information(this,
                                 tr("Add Polygon"),
                                 tr("At least three vertices or three lines are required to create a polygon."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Polygon"));

    auto *mainLayout = new QVBoxLayout(&dialog);

    auto *modeGroup = new QGroupBox(tr("Definition Mode"), &dialog);
    auto *modeLayout = new QHBoxLayout(modeGroup);
    auto *verticesRadio = new QRadioButton(tr("By Vertices"), modeGroup);
    auto *linesRadio = new QRadioButton(tr("By Lines"), modeGroup);
    verticesRadio->setChecked(true);
    modeLayout->addWidget(verticesRadio);
    modeLayout->addWidget(linesRadio);
    modeGroup->setLayout(modeLayout);
    mainLayout->addWidget(modeGroup);

    auto *stack = new QStackedWidget(&dialog);

    auto *verticesWidget = new QWidget(&dialog);
    auto *verticesLayout = new QVBoxLayout(verticesWidget);
    auto *verticesLabel = new QLabel(tr("Enter vertex IDs separated by commas or spaces:"), verticesWidget);
    auto *verticesLineEdit = new QLineEdit(verticesWidget);
    verticesLayout->addWidget(verticesLabel);
    verticesLayout->addWidget(verticesLineEdit);
    verticesWidget->setLayout(verticesLayout);
    stack->addWidget(verticesWidget);

    auto *linesWidget = new QWidget(&dialog);
    auto *linesLayout = new QVBoxLayout(linesWidget);
    auto *linesLabel = new QLabel(tr("Enter line IDs separated by commas or spaces:"), linesWidget);
    auto *linesLineEdit = new QLineEdit(linesWidget);
    linesLayout->addWidget(linesLabel);
    linesLayout->addWidget(linesLineEdit);
    linesWidget->setLayout(linesLayout);
    stack->addWidget(linesWidget);

    QObject::connect(verticesRadio, &QRadioButton::toggled, stack, [stack](bool checked) {
        if (checked)
            stack->setCurrentIndex(0);
    });
    QObject::connect(linesRadio, &QRadioButton::toggled, stack, [stack](bool checked) {
        if (checked)
            stack->setCurrentIndex(1);
    });

    mainLayout->addWidget(stack);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                           Qt::Horizontal,
                                           &dialog);
    mainLayout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    auto parseIds = [](const QString &text, std::vector<int> &ids) {
        ids.clear();
        const QString trimmed = text.trimmed();
        if (trimmed.isEmpty())
            return false;

        const QRegularExpression separator(QStringLiteral("[,\\s]+"));
        const QStringList parts = trimmed.split(separator, Qt::SkipEmptyParts);
        ids.reserve(parts.size());
        for (const QString &part : parts) {
            bool ok = false;
            const int value = part.toInt(&ok);
            if (!ok)
                return false;
            ids.push_back(value);
        }
        return !ids.empty();
    };

    std::vector<Vertex *> polygonVertices;
    std::vector<Line *> polygonLines;
    std::vector<Line *> newlyCreatedLines;

    if (verticesRadio->isChecked()) {
        std::vector<int> vertexIds;
        if (!parseIds(verticesLineEdit->text(), vertexIds)) {
            QMessageBox::warning(this,
                                  tr("Add Polygon"),
                                  tr("Please provide at least three vertex IDs."));
            return;
        }

        if (vertexIds.size() < 3) {
            QMessageBox::warning(this,
                                  tr("Add Polygon"),
                                  tr("Please provide at least three distinct vertex IDs."));
            return;
        }

        if (vertexIds.size() > 1 && vertexIds.front() == vertexIds.back())
            vertexIds.pop_back();

        std::set<int> uniqueVertexIds;
        polygonVertices.reserve(vertexIds.size());
        for (int id : vertexIds) {
            if (!uniqueVertexIds.insert(id).second) {
                QMessageBox::warning(this,
                                      tr("Add Polygon"),
                                      tr("Vertex ID %1 is duplicated.").arg(id));
                return;
            }

            Vertex *vertex = findVertexById(id);
            if (!vertex) {
                QMessageBox::warning(this,
                                      tr("Add Polygon"),
                                      tr("Vertex ID %1 was not found.").arg(id));
                return;
            }

            polygonVertices.push_back(vertex);
        }

        if (polygonVertices.size() < 3) {
            QMessageBox::warning(this,
                                  tr("Add Polygon"),
                                  tr("A polygon requires at least three vertices."));
            return;
        }

        polygonVertices = sortVerticesCounterClockwise(polygonVertices);
        if (polygonVertices.size() < 3) {
            QMessageBox::warning(this,
                                  tr("Add Polygon"),
                                  tr("Failed to determine a valid polygon from the provided vertices."));
            return;
        }

        polygonLines.reserve(polygonVertices.size());
        newlyCreatedLines.reserve(polygonVertices.size());
        for (std::size_t i = 0; i < polygonVertices.size(); ++i) {
            Vertex *startVertex = polygonVertices[i];
            Vertex *endVertex = polygonVertices[(i + 1) % polygonVertices.size()];
            Line *line = findLineByVertices(startVertex, endVertex);
            if (!line) {
                line = createLine(startVertex, endVertex);
                if (!line) {
                    QMessageBox::warning(this,
                                          tr("Add Polygon"),
                                          tr("Failed to create a line between vertex %1 and vertex %2.").arg(startVertex->id()).arg(endVertex->id()));
                    for (Line *createdLine : newlyCreatedLines)
                        deleteLine(createdLine);
                    return;
                }
                newlyCreatedLines.push_back(line);
            }

            polygonLines.push_back(line);
        }

        ensureCounterClockwise(polygonVertices, polygonLines);
    } else {
        std::vector<int> lineIds;
        if (!parseIds(linesLineEdit->text(), lineIds)) {
            QMessageBox::warning(this,
                                  tr("Add Polygon"),
                                  tr("Please provide at least three line IDs."));
            return;
        }

        if (lineIds.size() < 3) {
            QMessageBox::warning(this,
                                  tr("Add Polygon"),
                                  tr("Please provide at least three distinct line IDs."));
            return;
        }

        std::set<int> uniqueLineIds;
        std::vector<Line *> inputLines;
        inputLines.reserve(lineIds.size());
        for (int id : lineIds) {
            if (!uniqueLineIds.insert(id).second) {
                QMessageBox::warning(this,
                                      tr("Add Polygon"),
                                      tr("Line ID %1 is duplicated.").arg(id));
                return;
            }

            Line *line = findLineById(id);
            if (!line) {
                QMessageBox::warning(this,
                                      tr("Add Polygon"),
                                      tr("Line ID %1 was not found.").arg(id));
                return;
            }

            inputLines.push_back(line);
        }

        if (!orderLinesIntoPolygon(inputLines, polygonLines, polygonVertices)) {
            QMessageBox::warning(this,
                                  tr("Add Polygon"),
                                  tr("The provided lines do not form a closed polygon."));
            return;
        }
    }

    Polygon *polygon = createPolygon(polygonVertices, polygonLines);
    if (!polygon) {
        for (Line *line : newlyCreatedLines)
            deleteLine(line);

        QMessageBox::warning(this,
                              tr("Add Polygon"),
                              tr("Failed to create the polygon."));
        return;
    }

    if (QGraphicsItem *item = polygon->graphicsItem()) {
        m_scene->clearSelection();
        item->setSelected(true);
    }

    updateSelectionLabels(polygon);
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

void MainWindow::on_actionFind_Polygon_triggered()
{
    if (m_polygons.empty()) {
        QMessageBox::information(this,
                                 tr("Find Polygon"),
                                 tr("There are no polygons to select."));
        return;
    }

    int maxId = 0;
    bool hasPolygon = false;
    for (const auto &polygon : m_polygons) {
        if (polygon) {
            maxId = std::max(maxId, polygon->id());
            hasPolygon = true;
        }
    }

    if (!hasPolygon) {
        QMessageBox::information(this,
                                 tr("Find Polygon"),
                                 tr("There are no polygons to select."));
        return;
    }

    bool ok = false;
    const int idToFind = QInputDialog::getInt(this,
                                              tr("Find Polygon"),
                                              tr("Enter the polygon ID:"),
                                              0,
                                              0,
                                              std::max(0, maxId),
                                              1,
                                              &ok);

    if (!ok)
        return;

    Polygon *polygon = findPolygonById(idToFind);
    if (!polygon) {
        QMessageBox::warning(this,
                              tr("Find Polygon"),
                              tr("No polygon with ID %1 was found.").arg(idToFind));
        return;
    }

    if (m_scene)
        m_scene->clearSelection();

    if (QGraphicsItem *item = polygon->graphicsItem()) {
        item->setSelected(true);
        if (ui->graphicsView)
            ui->graphicsView->centerOn(item);
    }

    updateSelectionLabels(polygon);
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
        m_polygons.clear();
        m_nextLineId = 0;
        m_nextPolygonId = 0;
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

void MainWindow::on_actionDelete_Polygon_triggered()
{
    if (m_polygons.empty()) {
        QMessageBox::information(this,
                                 tr("Delete Polygon"),
                                 tr("There are no polygons to delete."));
        return;
    }

    int maxId = 0;
    bool hasPolygon = false;
    for (const auto &polygon : m_polygons) {
        if (polygon) {
            maxId = std::max(maxId, polygon->id());
            hasPolygon = true;
        }
    }

    if (!hasPolygon) {
        QMessageBox::information(this,
                                 tr("Delete Polygon"),
                                 tr("There are no polygons to delete."));
        return;
    }

    bool ok = false;
    const int idToDelete = QInputDialog::getInt(this,
                                                tr("Delete Polygon"),
                                                tr("Enter the polygon ID:"),
                                                0,
                                                0,
                                                std::max(0, maxId),
                                                1,
                                                &ok);

    if (!ok)
        return;

    Polygon *polygon = findPolygonById(idToDelete);
    if (!polygon) {
        QMessageBox::warning(this,
                              tr("Delete Polygon"),
                              tr("No polygon with ID %1 was found.").arg(idToDelete));
        return;
    }

    if (m_scene)
        m_scene->clearSelection();

    deletePolygon(polygon);
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
    m_polygons.clear();
    m_nextLineId = 0;
    m_nextPolygonId = 0;
    resetSelectionLabels();
}

void MainWindow::on_actionDelete_All_Polygons_triggered()
{
    if (m_polygons.empty()) {
        QMessageBox::information(this,
                                 tr("Delete All Polygons"),
                                 tr("There are no polygons to delete."));
        return;
    }

    const auto reply = QMessageBox::question(this,
                                             tr("Delete All Polygons"),
                                             tr("Are you sure you want to delete all polygons?"),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (m_scene)
        m_scene->clearSelection();

    m_polygons.clear();
    m_nextPolygonId = 0;
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
    } else if (Polygon *polygon = findPolygonByGraphicsItem(item)) {
        updateSelectionLabels(polygon);
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
    } else if (Polygon *polygon = findPolygonByGraphicsItem(item)) {
        updateSelectionLabels(polygon);
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

void MainWindow::handleCreateLineFromContextMenu(QGraphicsItem *firstVertexItem, QGraphicsItem *secondVertexItem)
{
    if (!m_scene || !firstVertexItem || !secondVertexItem || firstVertexItem == secondVertexItem)
        return;

    Vertex *firstVertex = findVertexByGraphicsItem(firstVertexItem);
    Vertex *secondVertex = findVertexByGraphicsItem(secondVertexItem);

    if (!firstVertex || !secondVertex)
        return;

    if (Line *existingLine = findLineByVertices(firstVertex, secondVertex)) {
        if (QGraphicsItem *existingLineItem = existingLine->graphicsItem()) {
            m_scene->clearSelection();
            existingLineItem->setSelected(true);
        }

        updateSelectionLabels(existingLine);
        return;
    }

    Line *line = createLine(firstVertex, secondVertex);
    if (!line)
        return;

    if (QGraphicsItem *lineGraphicsItem = line->graphicsItem()) {
        m_scene->clearSelection();
        lineGraphicsItem->setSelected(true);
    }

    updateSelectionLabels(line);
}

void MainWindow::handleDeletePolygonFromContextMenu(QGraphicsItem *polygonItem)
{
    if (!m_scene || !polygonItem)
        return;

    if (Polygon *polygon = findPolygonByGraphicsItem(polygonItem)) {
        m_scene->clearSelection();
        deletePolygon(polygon);
        resetSelectionLabels();
    }
}

void MainWindow::handleCreatePolygonFromContextMenu(const QList<QGraphicsItem *> &vertexItems)
{
    if (!m_scene || vertexItems.size() < 3)
        return;

    std::vector<Vertex *> selectedVertices;
    selectedVertices.reserve(vertexItems.size());

    std::unordered_set<Vertex *> uniqueVertices;
    uniqueVertices.reserve(static_cast<std::size_t>(vertexItems.size()));

    for (QGraphicsItem *item : vertexItems) {
        if (!item)
            continue;

        Vertex *vertex = findVertexByGraphicsItem(item);
        if (!vertex)
            continue;

        if (uniqueVertices.insert(vertex).second)
            selectedVertices.push_back(vertex);
    }

    if (selectedVertices.size() < 3) {
        QMessageBox::warning(this,
                              tr("Create Polygon"),
                              tr("Select at least three unique vertices."));
        return;
    }

    std::unordered_set<Vertex *> vertexSet;
    vertexSet.reserve(selectedVertices.size());
    for (Vertex *vertex : selectedVertices)
        vertexSet.insert(vertex);

    std::vector<Line *> candidateLines;
    candidateLines.reserve(selectedVertices.size());

    for (const auto &line : m_lines) {
        if (!line)
            continue;

        Vertex *start = line->startVertex();
        Vertex *end = line->endVertex();
        if (vertexSet.count(start) && vertexSet.count(end))
            candidateLines.push_back(line.get());
    }

    std::vector<Line *> orderedLines;
    std::vector<Vertex *> orderedVertices;
    std::vector<Line *> newlyCreatedLines;

    if (candidateLines.size() == selectedVertices.size()) {
        std::unordered_map<Vertex *, int> degreeCount;
        degreeCount.reserve(selectedVertices.size());
        for (Line *line : candidateLines) {
            if (!line)
                continue;

            degreeCount[line->startVertex()]++;
            degreeCount[line->endVertex()]++;
        }

        for (Vertex *vertex : selectedVertices) {
            if (degreeCount[vertex] != 2) {
                QMessageBox::warning(this,
                                      tr("Create Polygon"),
                                      tr("Each selected vertex must connect to exactly two selected lines."));
                return;
            }
        }

        if (!orderLinesIntoPolygon(candidateLines, orderedLines, orderedVertices)) {
            QMessageBox::warning(this,
                                  tr("Create Polygon"),
                                  tr("Failed to order the selected vertices into a polygon."));
            return;
        }
    } else if (candidateLines.size() > selectedVertices.size()) {
        QMessageBox::warning(this,
                              tr("Create Polygon"),
                              tr("The selected vertices must be connected by exactly one closed polygon."));
        return;
    } else {
        orderedVertices = sortVerticesCounterClockwise(selectedVertices);
        if (orderedVertices.size() < 3) {
            QMessageBox::warning(this,
                                  tr("Create Polygon"),
                                  tr("Failed to determine a valid polygon from the selected vertices."));
            return;
        }

        orderedLines.reserve(orderedVertices.size());
        newlyCreatedLines.reserve(orderedVertices.size());
        for (std::size_t i = 0; i < orderedVertices.size(); ++i) {
            Vertex *startVertex = orderedVertices[i];
            Vertex *endVertex = orderedVertices[(i + 1) % orderedVertices.size()];
            Line *line = findLineByVertices(startVertex, endVertex);
            if (!line) {
                line = createLine(startVertex, endVertex);
                if (!line) {
                    QMessageBox::warning(this,
                                          tr("Create Polygon"),
                                          tr("Failed to create a line between vertex %1 and vertex %2.").arg(startVertex->id()).arg(endVertex->id()));
                    for (Line *createdLine : newlyCreatedLines)
                        deleteLine(createdLine);
                    return;
                }
                newlyCreatedLines.push_back(line);
            }

            orderedLines.push_back(line);
        }

        ensureCounterClockwise(orderedVertices, orderedLines);
    }

    if (orderedLines.size() < 3 || orderedVertices.size() != orderedLines.size()) {
        QMessageBox::warning(this,
                              tr("Create Polygon"),
                              tr("Failed to create a polygon from the selected vertices."));
        for (Line *line : newlyCreatedLines)
            deleteLine(line);
        return;
    }

    Polygon *polygon = createPolygon(orderedVertices, orderedLines);
    if (!polygon) {
        for (Line *line : newlyCreatedLines)
            deleteLine(line);

        QMessageBox::warning(this,
                              tr("Create Polygon"),
                              tr("Failed to create the polygon."));
        return;
    }

    if (QGraphicsItem *item = polygon->graphicsItem()) {
        m_scene->clearSelection();
        item->setSelected(true);
    }

    updateSelectionLabels(polygon);
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

void MainWindow::updateSelectionLabels(Polygon *polygon)
{
    if (!polygon)
        return;

    ui->label_selected_item->setText(tr("polygon"));
    ui->label_selected_item_id->setText(QString::number(polygon->id()));

    QStringList vertexDescriptions;
    for (Vertex *vertex : polygon->vertices()) {
        if (!vertex)
            continue;

        const QGraphicsItem *graphicsItem = vertex->graphicsItem();
        const QPointF pos = graphicsItem ? graphicsItem->scenePos() : vertex->position();
        const QString xText = QString::number(pos.x(), 'f', 2);
        const QString yText = QString::number(pos.y(), 'f', 2);
        vertexDescriptions.append(tr("v%1 (%2, %3)").arg(vertex->id()).arg(xText, yText));
    }

    ui->label_selected_item_pos->setText(vertexDescriptions.join(QStringLiteral("; ")));
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
    m_lines.clear();
    m_polygons.clear();
    m_nextLineId = 0;
    m_nextPolygonId = 0;

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
    m_lines.clear();
    m_polygons.clear();
    m_nextLineId = 0;
    m_nextPolygonId = 0;
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
    m_polygons.clear();
    m_nextLineId = 0;
    m_nextPolygonId = 0;

    for (const auto &vertexData : importedVertices)
        createVertexWithId(vertexData.first, vertexData.second);

    resetSelectionLabels();
}

void MainWindow::on_actionExport_Vertex_Line_triggered()
{
    if (!m_scene)
        return;

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          tr("Export Vertices, Lines, and Polygons"),
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

    QJsonArray polygonArray;
    for (const auto &polygon : m_polygons) {
        if (!polygon)
            continue;

        QJsonObject polygonObject;
        polygonObject.insert(QStringLiteral("id"), polygon->id());

        QJsonArray polygonVertexIds;
        for (const Vertex *vertex : polygon->vertices()) {
            if (!vertex)
                continue;

            polygonVertexIds.append(vertex->id());
        }
        polygonObject.insert(QStringLiteral("vertexIds"), polygonVertexIds);

        QJsonArray polygonLineIds;
        for (const Line *line : polygon->lines()) {
            if (!line)
                continue;

            polygonLineIds.append(line->id());
        }
        polygonObject.insert(QStringLiteral("lineIds"), polygonLineIds);

        polygonArray.append(polygonObject);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("vertices"), vertexArray);
    rootObject.insert(QStringLiteral("lines"), lineArray);
    rootObject.insert(QStringLiteral("polygons"), polygonArray);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this,
                              tr("Export Vertices, Lines, and Polygons"),
                              tr("Failed to open %1 for writing.").arg(QDir::toNativeSeparators(fileName)));
        return;
    }

    const QJsonDocument document(rootObject);
    file.write(document.toJson(QJsonDocument::Indented));
    file.close();
}

void MainWindow::on_actionSnapShot_All_triggered()
{
    const auto options = requestSnapshotOptions(this,
                                                tr("Save Snapshot (All)"),
                                                tr("snapshot_all.png"));
    if (!options)
        return;

    const QPixmap pixmap = grab();
    if (pixmap.isNull()) {
        QMessageBox::warning(this,
                              tr("Save Snapshot"),
                              tr("Failed to capture the snapshot."));
        return;
    }

    if (!pixmap.save(options->filePath, nullptr, options->quality)) {
        QMessageBox::warning(this,
                              tr("Save Snapshot"),
                              tr("Failed to save %1.").arg(QDir::toNativeSeparators(options->filePath)));
        return;
    }

    if (statusBar())
        statusBar()->showMessage(tr("Snapshot saved to %1").arg(QDir::toNativeSeparators(options->filePath)),
                                 5000);
}

void MainWindow::on_actionSnapShot_View_triggered()
{
    if (!ui || !ui->graphicsView) {
        QMessageBox::warning(this,
                              tr("Save Snapshot"),
                              tr("Graphics view is not available."));
        return;
    }

    const auto options = requestSnapshotOptions(this,
                                                tr("Save Snapshot (View)"),
                                                tr("snapshot_view.png"));
    if (!options)
        return;

    const QPixmap pixmap = ui->graphicsView->grab();
    if (pixmap.isNull()) {
        QMessageBox::warning(this,
                              tr("Save Snapshot"),
                              tr("Failed to capture the graphics view."));
        return;
    }

    if (!pixmap.save(options->filePath, nullptr, options->quality)) {
        QMessageBox::warning(this,
                              tr("Save Snapshot"),
                              tr("Failed to save %1.").arg(QDir::toNativeSeparators(options->filePath)));
        return;
    }

    if (statusBar())
        statusBar()->showMessage(tr("Snapshot saved to %1").arg(QDir::toNativeSeparators(options->filePath)),
                                 5000);
}

void MainWindow::on_actionImport_Vertex_Line_triggered()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Import Vertices, Lines, and Polygons"),
                                                          QString(),
                                                          tr("JSON Files (*.json);;All Files (*)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,
                              tr("Import Vertices, Lines, and Polygons"),
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
                              tr("Import Vertices, Lines, and Polygons"),
                              tr("Failed to parse %1: %2").arg(QDir::toNativeSeparators(fileName), errorMessage));
        return;
    }

    const QJsonObject rootObject = document.object();
    const QJsonValue verticesValue = rootObject.value(QStringLiteral("vertices"));
    const QJsonValue linesValue = rootObject.value(QStringLiteral("lines"));
    const QJsonValue polygonsValue = rootObject.value(QStringLiteral("polygons"));

    if (!verticesValue.isArray() || !linesValue.isArray() || !polygonsValue.isArray()) {
        QMessageBox::warning(this,
                              tr("Import Vertices, Lines, and Polygons"),
                              tr("The JSON file must contain 'vertices', 'lines', and 'polygons' arrays."));
        return;
    }

    const QJsonArray verticesArray = verticesValue.toArray();
    const QJsonArray linesArray = linesValue.toArray();
    const QJsonArray polygonsArray = polygonsValue.toArray();

    struct VertexImportData {
        int id;
        QPointF position;
    };
    struct LineImportData {
        int id;
        int startId;
        int endId;
    };
    struct PolygonImportData {
        int id;
        std::vector<int> vertexIds;
        std::vector<int> lineIds;
    };

    std::vector<VertexImportData> importedVertices;
    importedVertices.reserve(verticesArray.size());
    std::set<int> vertexIds;

    for (const QJsonValue &value : verticesArray) {
        if (!value.isObject()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Each vertex entry must be a JSON object."));
            return;
        }

        const QJsonObject vertexObject = value.toObject();
        const QJsonValue idValue = vertexObject.value(QStringLiteral("id"));
        const QJsonValue xValue = vertexObject.value(QStringLiteral("x"));
        const QJsonValue yValue = vertexObject.value(QStringLiteral("y"));

        if (!idValue.isDouble() || !xValue.isDouble() || !yValue.isDouble()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Vertex entries must contain numeric 'id', 'x', and 'y' fields."));
            return;
        }

        const int id = idValue.toInt();
        if (!vertexIds.insert(id).second) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
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
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Each line entry must be a JSON object."));
            return;
        }

        const QJsonObject lineObject = value.toObject();
        const QJsonValue idValue = lineObject.value(QStringLiteral("id"));
        const QJsonValue startValue = lineObject.value(QStringLiteral("startVertexId"));
        const QJsonValue endValue = lineObject.value(QStringLiteral("endVertexId"));

        if (!idValue.isDouble() || !startValue.isDouble() || !endValue.isDouble()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Line entries must contain numeric 'id', 'startVertexId', and 'endVertexId' fields."));
            return;
        }

        const int id = idValue.toInt();
        const int startId = startValue.toInt();
        const int endId = endValue.toInt();

        if (!lineIds.insert(id).second) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Duplicate line id %1 detected.").arg(id));
            return;
        }

        if (startId == endId) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Line %1 references the same vertex for both ends.").arg(id));
            return;
        }

        if (!vertexIds.count(startId) || !vertexIds.count(endId)) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Line %1 references undefined vertices.").arg(id));
            return;
        }

        importedLines.push_back({id, startId, endId});
        maxLineId = std::max(maxLineId, id);
    }

    std::vector<PolygonImportData> importedPolygons;
    importedPolygons.reserve(polygonsArray.size());
    std::set<int> polygonIds;
    int maxPolygonId = -1;

    for (const QJsonValue &value : polygonsArray) {
        if (!value.isObject()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Each polygon entry must be a JSON object."));
            return;
        }

        const QJsonObject polygonObject = value.toObject();
        const QJsonValue idValue = polygonObject.value(QStringLiteral("id"));
        const QJsonValue vertexIdsValue = polygonObject.value(QStringLiteral("vertexIds"));
        const QJsonValue lineIdsValue = polygonObject.value(QStringLiteral("lineIds"));

        if (!idValue.isDouble() || !vertexIdsValue.isArray() || !lineIdsValue.isArray()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Polygon entries must contain numeric 'id' and arrays of 'vertexIds' and 'lineIds'."));
            return;
        }

        const int id = idValue.toInt();
        if (!polygonIds.insert(id).second) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Duplicate polygon id %1 detected.").arg(id));
            return;
        }

        const QJsonArray polygonVertexIdsArray = vertexIdsValue.toArray();
        const QJsonArray polygonLineIdsArray = lineIdsValue.toArray();

        if (polygonVertexIdsArray.size() < 3 || polygonLineIdsArray.size() < 3) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Polygon %1 must reference at least three vertices and three lines.").arg(id));
            return;
        }

        if (polygonVertexIdsArray.size() != polygonLineIdsArray.size()) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Polygon %1 must have matching counts of vertices and lines.").arg(id));
            return;
        }

        std::vector<int> polygonVertexIds;
        polygonVertexIds.reserve(polygonVertexIdsArray.size());
        for (const QJsonValue &vertexIdValue : polygonVertexIdsArray) {
            if (!vertexIdValue.isDouble()) {
                QMessageBox::warning(this,
                                      tr("Import Vertices, Lines, and Polygons"),
                                      tr("Polygon %1 contains a non-numeric vertex id.").arg(id));
                return;
            }

            const int vertexId = vertexIdValue.toInt();
            if (!vertexIds.count(vertexId)) {
                QMessageBox::warning(this,
                                      tr("Import Vertices, Lines, and Polygons"),
                                      tr("Polygon %1 references undefined vertex %2.").arg(id).arg(vertexId));
                return;
            }

            polygonVertexIds.push_back(vertexId);
        }

        std::vector<int> polygonLineIds;
        polygonLineIds.reserve(polygonLineIdsArray.size());
        for (const QJsonValue &lineIdValue : polygonLineIdsArray) {
            if (!lineIdValue.isDouble()) {
                QMessageBox::warning(this,
                                      tr("Import Vertices, Lines, and Polygons"),
                                      tr("Polygon %1 contains a non-numeric line id.").arg(id));
                return;
            }

            const int lineId = lineIdValue.toInt();
            if (!lineIds.count(lineId)) {
                QMessageBox::warning(this,
                                      tr("Import Vertices, Lines, and Polygons"),
                                      tr("Polygon %1 references undefined line %2.").arg(id).arg(lineId));
                return;
            }

            polygonLineIds.push_back(lineId);
        }

        importedPolygons.push_back({id, std::move(polygonVertexIds), std::move(polygonLineIds)});
        maxPolygonId = std::max(maxPolygonId, id);
    }

    m_scene->clearSelection();
    m_lines.clear();
    m_vertices.clear();
    m_polygons.clear();
    m_nextLineId = 0;
    m_nextPolygonId = 0;

    for (const auto &vertexData : importedVertices)
        createVertexWithId(vertexData.id, vertexData.position);

    for (const auto &lineData : importedLines) {
        Vertex *startVertex = findVertexById(lineData.startId);
        Vertex *endVertex = findVertexById(lineData.endId);
        if (!createLineWithId(lineData.id, startVertex, endVertex)) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Failed to create line %1.").arg(lineData.id));
            break;
        }
    }

    for (const auto &polygonData : importedPolygons) {
        std::vector<Vertex *> polygonVertices;
        polygonVertices.reserve(polygonData.vertexIds.size());
        for (int vertexId : polygonData.vertexIds) {
            Vertex *vertex = findVertexById(vertexId);
            if (!vertex) {
                QMessageBox::warning(this,
                                      tr("Import Vertices, Lines, and Polygons"),
                                      tr("Failed to find vertex %1 for polygon %2.").arg(vertexId).arg(polygonData.id));
                polygonVertices.clear();
                break;
            }
            polygonVertices.push_back(vertex);
        }

        if (polygonVertices.size() != polygonData.vertexIds.size())
            continue;

        std::vector<Line *> polygonLines;
        polygonLines.reserve(polygonData.lineIds.size());
        for (int lineId : polygonData.lineIds) {
            Line *line = findLineById(lineId);
            if (!line) {
                QMessageBox::warning(this,
                                      tr("Import Vertices, Lines, and Polygons"),
                                      tr("Failed to find line %1 for polygon %2.").arg(lineId).arg(polygonData.id));
                polygonLines.clear();
                break;
            }
            polygonLines.push_back(line);
        }

        if (polygonLines.size() != polygonData.lineIds.size())
            continue;

        if (!createPolygonWithId(polygonData.id, polygonVertices, polygonLines)) {
            QMessageBox::warning(this,
                                  tr("Import Vertices, Lines, and Polygons"),
                                  tr("Failed to create polygon %1.").arg(polygonData.id));
        }
    }

    m_nextLineId = maxLineId >= 0 ? maxLineId + 1 : 0;
    m_nextPolygonId = maxPolygonId >= 0 ? maxPolygonId + 1 : 0;
    resetSelectionLabels();
}
