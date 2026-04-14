#pragma once
// terrain_gl_widget.h — Terrain viewer GL viewport subclass.
// Renders terrain heightmap mesh with color map, height coloring, or wireframe.

#include "gl_viewport_widget.h"
#include "netdevil/zone/terrain/terrain_reader.h"

namespace terrain_viewer {

enum class ColorMode {
    HeightGradient,   // Color by Y height (blue=low, green=mid, red=high)
    ColorMap,         // Use the terrain's baked color map (RGBA)
    SceneMap,         // Color by scene ID (official 146-color palette)
    TextureWeights,   // Texture layer weights (RGBA → RGB)
    Flat,             // Uniform grey
};

class TerrainGLWidget : public gl_viewport::BaseGLViewport {
    Q_OBJECT
public:
    explicit TerrainGLWidget(QWidget* parent = nullptr);

    void loadTerrain(const lu::assets::TerrainFile& terrain);
    void setColorMode(ColorMode mode);
    ColorMode colorMode() const { return colorMode_; }

    const lu::assets::TerrainFile& terrain() const { return terrain_; }

    int chunkCount() const { return static_cast<int>(terrain_.chunks.size()); }
    int totalVertices() const { return totalVerts_; }
    int totalTriangles() const { return totalTris_; }

    bool showFlairs = true;

signals:
    void terrainLoaded();

protected:
    void onInitGL() override;
    void drawBackground() override;
    void drawOverlay() override;  // flair markers
    void paintGL() override;      // per-vertex color rendering

private:
    void rebuildMeshes();

    lu::assets::TerrainFile terrain_;
    ColorMode colorMode_ = ColorMode::HeightGradient;
    int totalVerts_ = 0;
    int totalTris_ = 0;
    float heightMin_ = 0, heightMax_ = 1;
};

} // namespace terrain_viewer
