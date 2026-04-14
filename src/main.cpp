// Terrain Viewer — displays .raw terrain heightmap files with layer overlays.

#include "main_window.h"

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char* argv[]) {
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setSamples(4);
    fmt.setVersion(2, 1);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setOrganizationName("LU-Rebuilt");
    app.setApplicationName("TerrainViewer");

    terrain_viewer::MainWindow window;
    window.show();

    if (argc > 1)
        window.loadFile(argv[1]);

    return app.exec();
}
