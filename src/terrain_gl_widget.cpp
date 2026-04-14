#include "terrain_gl_widget.h"
#include "gl_helpers.h"

#include <QOpenGLFunctions>
#include <cmath>
#include <algorithm>

namespace terrain_viewer {

using gl_viewport::RenderMesh;

TerrainGLWidget::TerrainGLWidget(QWidget* parent) : BaseGLViewport(parent) {}

void TerrainGLWidget::onInitGL() {
    glDisable(GL_FOG);
    // Keep blend disabled for terrain — we use opaque rendering with per-vertex colors
    glDisable(GL_BLEND);
}

void TerrainGLWidget::drawBackground() {
    gl_viewport::drawAxes(30.0f);
}

void TerrainGLWidget::setColorMode(ColorMode mode) {
    if (mode != colorMode_) {
        colorMode_ = mode;
        rebuildMeshes();
    }
}

void TerrainGLWidget::loadTerrain(const lu::assets::TerrainFile& terrain) {
    terrain_ = terrain;
    rebuildMeshes();
    fitToVisible();
    emit terrainLoaded();
}

static lu::assets::TerrainColorMode toAssetMode(ColorMode m) {
    switch (m) {
    case ColorMode::ColorMap: return lu::assets::TerrainColorMode::ColorMap;
    case ColorMode::SceneMap: return lu::assets::TerrainColorMode::SceneMap;
    case ColorMode::TextureWeights: return lu::assets::TerrainColorMode::TextureWeights;
    default: return lu::assets::TerrainColorMode::HeightGradient;
    }
}

void TerrainGLWidget::rebuildMeshes() {
    clearMeshes();
    totalVerts_ = totalTris_ = 0;

    // Generate mesh geometry from lu_assets
    auto mesh = lu::assets::terrain_generate_mesh(terrain_);

    // Generate per-vertex colors from lu_assets (or flat grey)
    lu::assets::TerrainVertexColors colors;
    if (colorMode_ != ColorMode::Flat) {
        colors = lu::assets::terrain_generate_colors(terrain_, toAssetMode(colorMode_));
    }

    // Build one RenderMesh per chunk for individual selection/toggle.
    // The mesh from terrain_generate_mesh is flat, so we need to split by chunk.
    // Easier: generate per-chunk directly.
    size_t vertOffset = 0;
    size_t colorOffset = 0;
    size_t triOffset = 0;

    for (size_t ci = 0; ci < terrain_.chunks.size(); ci++) {
        const auto& chunk = terrain_.chunks[ci];
        if (chunk.heightmap.empty() || chunk.width <= 1 || chunk.height <= 1) continue;

        uint32_t w = chunk.width, h = chunk.height;
        uint32_t chunkVerts = w * h;
        uint32_t chunkTris = (w - 1) * (h - 1) * 2;

        RenderMesh rm;
        rm.wireColor[0] = 0.3f; rm.wireColor[1] = 0.3f; rm.wireColor[2] = 0.3f;
        rm.color[0] = 0.6f; rm.color[1] = 0.6f; rm.color[2] = 0.6f; rm.color[3] = 1.0f;

        // Copy vertices for this chunk
        size_t vStart = vertOffset * 3;
        size_t vEnd = vStart + chunkVerts * 3;
        if (vEnd <= mesh.vertices.size()) {
            rm.vertices.assign(mesh.vertices.begin() + vStart, mesh.vertices.begin() + vEnd);
        }

        // Copy per-vertex colors into normals array (used for per-vertex coloring)
        if (colorMode_ != ColorMode::Flat && colorOffset * 3 + chunkVerts * 3 <= colors.colors.size()) {
            rm.normals.assign(colors.colors.begin() + colorOffset * 3,
                              colors.colors.begin() + (colorOffset + chunkVerts) * 3);
        }

        // Copy indices, rebased to 0
        size_t iStart = triOffset * 3;
        size_t iEnd = iStart + chunkTris * 3;
        if (iEnd <= mesh.indices.size()) {
            rm.indices.reserve(chunkTris * 3);
            uint32_t base = static_cast<uint32_t>(vertOffset);
            for (size_t i = iStart; i < iEnd; i++)
                rm.indices.push_back(mesh.indices[i] - base);
        }

        char buf[128];
        snprintf(buf, sizeof(buf), "Chunk %zu (id=%u, %ux%u, offset=%.0f,%.0f)",
            ci, chunk.chunk_id, w, h, chunk.offset_x, chunk.offset_z);
        rm.label = buf;

        totalVerts_ += static_cast<int>(chunkVerts);
        totalTris_ += static_cast<int>(chunkTris);
        addMesh(std::move(rm));

        vertOffset += chunkVerts;
        colorOffset += chunkVerts;
        triOffset += chunkTris;
    }

    update();
}

void TerrainGLWidget::drawOverlay() {
    if (!showFlairs) return;

    // Draw flair positions as bright colored markers.
    // Many flair colors are dark greens/browns — boost brightness for visibility.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for (const auto& chunk : terrain_.chunks) {
        for (const auto& flair : chunk.flairs) {
            float r = std::min(1.0f, flair.r / 255.0f + 0.3f);
            float g = std::min(1.0f, flair.g / 255.0f + 0.3f);
            float b = std::min(1.0f, flair.b / 255.0f + 0.3f);
            glColor4f(r, g, b, 0.9f);
            glVertex3f(flair.position.x, flair.position.y, flair.position.z);
        }
    }
    glEnd();

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (const auto& chunk : terrain_.chunks) {
        for (const auto& flair : chunk.flairs) {
            float s = std::max(2.0f, flair.scale * 3.0f);
            float r = std::min(1.0f, flair.r / 255.0f + 0.3f);
            float g = std::min(1.0f, flair.g / 255.0f + 0.3f);
            float b = std::min(1.0f, flair.b / 255.0f + 0.3f);
            glColor4f(r, g, b, 0.9f);
            float x = flair.position.x, y = flair.position.y, z = flair.position.z;
            glVertex3f(x-s, y, z); glVertex3f(x+s, y, z);
            glVertex3f(x, y-s, z); glVertex3f(x, y+s, z);
            glVertex3f(x, y, z-s); glVertex3f(x, y, z+s);
        }
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

void TerrainGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    camera().applyGLTransform();
    drawBackground();

    for (int mi = 0; mi < meshCount(); ++mi) {
        const auto& mesh = meshes_[mi];
        if (mesh.vertices.empty() || mesh.indices.empty() || !mesh.visible) continue;
        bool selected = (mi == selectedIdx_);

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, mesh.vertices.data());

        // Use per-vertex colors if available (stored in normals as RGB)
        bool hasVertexColors = !mesh.normals.empty() && mesh.normals.size() == mesh.vertices.size();
        if (hasVertexColors) {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(3, GL_FLOAT, 0, mesh.normals.data());
        }

        // Solid fill
        if (showSolid) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
            if (selected) {
                if (hasVertexColors) glDisableClientState(GL_COLOR_ARRAY);
                glColor4f(1.0f, 0.4f, 0.1f, 0.6f);
            } else if (!hasVertexColors) {
                glColor4f(mesh.color[0], mesh.color[1], mesh.color[2], mesh.color[3]);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()),
                           GL_UNSIGNED_INT, mesh.indices.data());
            glDisable(GL_POLYGON_OFFSET_FILL);
            if (selected && hasVertexColors) glEnableClientState(GL_COLOR_ARRAY);
        }

        if (hasVertexColors) glDisableClientState(GL_COLOR_ARRAY);

        // Wireframe
        if (showWireframe) {
            if (selected) glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
            else glColor4f(mesh.wireColor[0], mesh.wireColor[1], mesh.wireColor[2], 1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(selected ? 2.0f : 1.0f);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()),
                           GL_UNSIGNED_INT, mesh.indices.data());
            glLineWidth(1.0f);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
}

} // namespace terrain_viewer
