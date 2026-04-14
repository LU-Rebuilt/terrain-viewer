#pragma once

#include "terrain_gl_widget.h"
#include "netdevil/zone/terrain/terrain_reader.h"

#include <QMainWindow>
#include <QLabel>
#include <QTableWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QCloseEvent>

namespace terrain_viewer {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    bool loadFile(const std::string& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onOpen();
    void onTerrainLoaded();
    void onColorModeChanged(int index);
    void onChunkSelected(int row, int col);

private:
    void buildChunkGrid();
    void saveSettings();
    void restoreSettings();

    TerrainGLWidget* glWidget_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QTableWidget* chunkGrid_ = nullptr;
    QTextEdit* chunkDetail_ = nullptr;
    QComboBox* colorModeCombo_ = nullptr;
    QCheckBox* showSolidCheck_ = nullptr;
    QCheckBox* showWireCheck_ = nullptr;
    QCheckBox* showFlairsCheck_ = nullptr;

    bool buildingGrid_ = false;
};

} // namespace terrain_viewer
