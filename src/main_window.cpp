#include "main_window.h"

#include <QMenuBar>
#include <QDockWidget>
#include "file_browser.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QHeaderView>

#include <filesystem>
#include <fstream>

namespace terrain_viewer {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Terrain Viewer");
    resize(1280, 800);

    glWidget_ = new TerrainGLWidget(this);
    setCentralWidget(glWidget_);

    // Menu
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Open...", QKeySequence::Open, this, &MainWindow::onOpen);
    fileMenu->addSeparator();
    fileMenu->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);

    // Right dock: controls + chunk grid + detail
    auto* dock = new QDockWidget("Layers && Chunks", this);
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(4, 4, 4, 4);

    // Color mode
    auto* layerGroup = new QGroupBox("Display");
    auto* layerForm = new QFormLayout(layerGroup);

    colorModeCombo_ = new QComboBox;
    colorModeCombo_->addItem("Height Gradient");
    colorModeCombo_->addItem("Color Map (baked)");
    colorModeCombo_->addItem("Scene Map (IDs)");
    colorModeCombo_->addItem("Texture Weights");
    colorModeCombo_->addItem("Flat Grey");
    connect(colorModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onColorModeChanged);
    layerForm->addRow("Color:", colorModeCombo_);

    showSolidCheck_ = new QCheckBox("Solid Fill");
    showSolidCheck_->setChecked(true);
    connect(showSolidCheck_, &QCheckBox::toggled, [this](bool c) {
        glWidget_->showSolid = c; glWidget_->update(); });
    layerForm->addRow(showSolidCheck_);

    showWireCheck_ = new QCheckBox("Wireframe");
    showWireCheck_->setChecked(true);
    connect(showWireCheck_, &QCheckBox::toggled, [this](bool c) {
        glWidget_->showWireframe = c; glWidget_->update(); });
    layerForm->addRow(showWireCheck_);

    showFlairsCheck_ = new QCheckBox("Show Flairs");
    showFlairsCheck_->setChecked(true);
    connect(showFlairsCheck_, &QCheckBox::toggled, [this](bool c) {
        glWidget_->showFlairs = c; glWidget_->update(); });
    layerForm->addRow(showFlairsCheck_);

    layout->addWidget(layerGroup);

    // Chunk grid
    auto* chunkLabel = new QLabel("Chunk Grid:");
    layout->addWidget(chunkLabel);

    chunkGrid_ = new QTableWidget;
    chunkGrid_->setSelectionMode(QAbstractItemView::SingleSelection);
    chunkGrid_->setMaximumHeight(250);
    chunkGrid_->horizontalHeader()->setDefaultSectionSize(32);
    chunkGrid_->verticalHeader()->setDefaultSectionSize(24);
    connect(chunkGrid_, &QTableWidget::cellClicked, this, &MainWindow::onChunkSelected);
    layout->addWidget(chunkGrid_);

    // Chunk detail
    chunkDetail_ = new QTextEdit;
    chunkDetail_->setReadOnly(true);
    chunkDetail_->setFont(QFont("monospace", 9));
    chunkDetail_->setPlaceholderText("Click a chunk in the grid above");
    layout->addWidget(chunkDetail_);

    dock->setObjectName("LayersAndChunks");
    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    // Status bar
    statusLabel_ = new QLabel("No file loaded");
    statusBar()->addWidget(statusLabel_, 1);

    connect(glWidget_, &TerrainGLWidget::terrainLoaded, this, &MainWindow::onTerrainLoaded);
    connect(glWidget_, &TerrainGLWidget::meshClicked, this, [this](int idx) {
        if (idx >= 0)
            statusBar()->showMessage(QString("Selected: %1").arg(
                QString::fromStdString(glWidget_->meshAt(idx).label)));
    });

    restoreSettings();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::saveSettings() {
    QSettings s;
    s.setValue("window/geometry", saveGeometry());
    s.setValue("window/state", saveState());
    s.setValue("colorMode", colorModeCombo_->currentIndex());
    s.setValue("showSolid", showSolidCheck_->isChecked());
    s.setValue("showWire", showWireCheck_->isChecked());
    s.setValue("showFlairs", showFlairsCheck_->isChecked());
}

void MainWindow::restoreSettings() {
    QSettings s;
    if (s.contains("window/geometry"))
        restoreGeometry(s.value("window/geometry").toByteArray());
    if (s.contains("window/state"))
        restoreState(s.value("window/state").toByteArray());
    if (s.contains("colorMode"))
        colorModeCombo_->setCurrentIndex(s.value("colorMode").toInt());
    if (s.contains("showSolid"))
        showSolidCheck_->setChecked(s.value("showSolid").toBool());
    if (s.contains("showWire"))
        showWireCheck_->setChecked(s.value("showWire").toBool());
    if (s.contains("showFlairs"))
        showFlairsCheck_->setChecked(s.value("showFlairs").toBool());
}

bool MainWindow::loadFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        QMessageBox::warning(this, "Error", QString("Cannot open: %1").arg(QString::fromStdString(path)));
        return false;
    }
    auto sz = f.tellg(); f.seekg(0);
    std::vector<uint8_t> data(static_cast<size_t>(sz));
    f.read(reinterpret_cast<char*>(data.data()), sz);

    try {
        auto terrain = lu::assets::terrain_parse(data);
        glWidget_->loadTerrain(terrain);
        setWindowTitle(QString("Terrain Viewer — %1")
            .arg(QString::fromStdString(std::filesystem::path(path).filename().string())));
        return true;
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Parse Error",
            QString("Failed to parse %1:\n%2").arg(QString::fromStdString(path)).arg(e.what()));
        return false;
    }
}

void MainWindow::onOpen() {
    QSettings settings;
    QString lastDir = settings.value("last_open_dir").toString();
    QString path = qt_common::FileBrowserDialog::getOpenFileName(this, "Open Terrain File", lastDir,
        "RAW Terrain (*.raw);;All Files (*)");
    if (!path.isEmpty()) {
        settings.setValue("last_open_dir", QFileInfo(path).absolutePath());
        loadFile(path.toStdString());
    }
}

void MainWindow::onTerrainLoaded() {
    const auto& t = glWidget_->terrain();
    statusLabel_->setText(QString("v%1 | %2 chunks (%3x%4) | %5 verts, %6 tris")
        .arg(t.version).arg(t.chunk_count).arg(t.chunks_width).arg(t.chunks_height)
        .arg(glWidget_->totalVertices()).arg(glWidget_->totalTriangles()));
    buildChunkGrid();
}

void MainWindow::onColorModeChanged(int index) {
    glWidget_->setColorMode(static_cast<ColorMode>(index));
}

void MainWindow::buildChunkGrid() {
    buildingGrid_ = true;
    chunkGrid_->clear();

    const auto& t = glWidget_->terrain();
    int cols = static_cast<int>(t.chunks_width);
    int rows = static_cast<int>(t.chunks_height);
    if (cols <= 0) cols = 1;
    if (rows <= 0) rows = 1;

    chunkGrid_->setColumnCount(cols);
    chunkGrid_->setRowCount(rows);

    for (int c = 0; c < cols; c++)
        chunkGrid_->setHorizontalHeaderItem(c, new QTableWidgetItem(QString::number(c)));
    for (int r = 0; r < rows; r++)
        chunkGrid_->setVerticalHeaderItem(r, new QTableWidgetItem(QString::number(r)));

    for (size_t ci = 0; ci < t.chunks.size(); ci++) {
        int r = static_cast<int>(ci / cols);
        int c = static_cast<int>(ci % cols);
        if (r >= rows || c >= cols) continue;

        const auto& chunk = t.chunks[ci];
        auto* item = new QTableWidgetItem();

        if (chunk.heightmap.empty() || chunk.width <= 1 || chunk.height <= 1) {
            item->setText("-");
            item->setBackground(QColor(60, 60, 60));
            item->setForeground(QColor(100, 100, 100));
        } else {
            item->setText(QString::number(ci));
            if (!chunk.scene_map.empty())
                item->setBackground(QColor(40, 80, 40));
            else
                item->setBackground(QColor(60, 80, 100));
            item->setForeground(QColor(200, 200, 200));
        }

        item->setTextAlignment(Qt::AlignCenter);
        item->setData(Qt::UserRole, static_cast<int>(ci));
        chunkGrid_->setItem(r, c, item);
    }

    chunkGrid_->resizeColumnsToContents();
    chunkGrid_->resizeRowsToContents();
    buildingGrid_ = false;
}

void MainWindow::onChunkSelected(int row, int col) {
    auto* item = chunkGrid_->item(row, col);
    if (!item) return;

    int ci = item->data(Qt::UserRole).toInt();
    const auto& t = glWidget_->terrain();
    if (ci < 0 || ci >= static_cast<int>(t.chunks.size())) return;
    const auto& chunk = t.chunks[ci];

    // Find and select the corresponding mesh
    int meshIdx = 0;
    for (int i = 0; i < ci; i++) {
        const auto& ch = t.chunks[i];
        if (!ch.heightmap.empty() && ch.width > 1 && ch.height > 1) meshIdx++;
    }
    if (!chunk.heightmap.empty() && chunk.width > 1 && chunk.height > 1) {
        glWidget_->setSelectedIndex(meshIdx);
        glWidget_->fitToMesh(meshIdx);
    }

    // Detail text
    QString d;
    d += QString("Chunk %1 (grid %2,%3)\n").arg(ci).arg(col).arg(row);
    d += QString("ID: %1\n").arg(chunk.chunk_id);
    d += QString("Size: %1 x %2\n").arg(chunk.width).arg(chunk.height);
    d += QString("Offset: (%1, %2)\n")
        .arg(static_cast<double>(chunk.offset_x), 0, 'f', 1)
        .arg(static_cast<double>(chunk.offset_z), 0, 'f', 1);
    d += QString("Scale: %1\n").arg(static_cast<double>(chunk.scale), 0, 'f', 3);
    d += QString("Textures: %1, %2, %3, %4\n")
        .arg(chunk.texture_ids[0]).arg(chunk.texture_ids[1])
        .arg(chunk.texture_ids[2]).arg(chunk.texture_ids[3]);

    if (!chunk.heightmap.empty()) {
        float hmin = 1e30f, hmax = -1e30f;
        for (float h : chunk.heightmap) { hmin = std::min(hmin, h); hmax = std::max(hmax, h); }
        d += QString("Height: %1 - %2\n")
            .arg(static_cast<double>(hmin), 0, 'f', 1)
            .arg(static_cast<double>(hmax), 0, 'f', 1);
    }

    d += "\nLayers:\n";
    if (!chunk.heightmap.empty())
        d += QString("  Heightmap: %1x%2\n").arg(chunk.width).arg(chunk.height);
    if (chunk.color_map_res > 0)
        d += QString("  Color Map: %1x%1 (%2 bytes)\n").arg(chunk.color_map_res).arg(chunk.color_map.size());
    if (!chunk.light_map.empty())
        d += QString("  Light Map: %1 bytes\n").arg(chunk.light_map.size());
    if (chunk.tex_map_res > 0)
        d += QString("  Texture Map: %1x%1 (%2 bytes)\n").arg(chunk.tex_map_res).arg(chunk.texture_map.size());
    if (!chunk.blend_map.empty())
        d += QString("  Blend Map: %1 bytes\n").arg(chunk.blend_map.size());
    if (!chunk.scene_map.empty())
        d += QString("  Scene Map: %1 entries\n").arg(chunk.scene_map.size());

    if (!chunk.flairs.empty()) {
        d += QString("\nFlairs: %1\n").arg(chunk.flairs.size());
        for (size_t fi = 0; fi < chunk.flairs.size() && fi < 15; fi++) {
            const auto& flair = chunk.flairs[fi];
            d += QString("  [%1] id=%2 (%3,%4,%5) s=%6\n")
                .arg(fi).arg(flair.id)
                .arg(static_cast<double>(flair.position.x), 0, 'f', 0)
                .arg(static_cast<double>(flair.position.y), 0, 'f', 0)
                .arg(static_cast<double>(flair.position.z), 0, 'f', 0)
                .arg(static_cast<double>(flair.scale), 0, 'f', 1);
        }
        if (chunk.flairs.size() > 15)
            d += QString("  ... %1 more\n").arg(chunk.flairs.size() - 15);
    }

    chunkDetail_->setText(d);
}

} // namespace terrain_viewer
