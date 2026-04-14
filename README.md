# terrain-viewer

Qt6/OpenGL viewer for LEGO Universe zone terrain heightmaps.

> **Note:** This project was developed with significant AI assistance (Claude by Anthropic). All code has been reviewed and validated by the project maintainer, but AI-generated code may contain subtle issues. Contributions and reviews are welcome.

Part of the [LU-Rebuilt](https://github.com/LU-Rebuilt) project.

## Usage

```
terrain_viewer [file.raw]
```

**Features:**
- Heightmap mesh rendering with per-vertex colors
- Multiple color modes: heightmap, texture IDs, scene colors (146-color palette from DarkflameServer)
- Chunk grid selection with chunk detail panel
- Flair (decoration) markers toggle
- Per-chunk metadata display (position, heightmap range, texture count)
- Settings persistence across sessions

**Keyboard shortcuts:**
- `Ctrl+O` — Open file
- `Esc` — Deselect

## Building

```bash
cmake -B build
cmake --build build -j$(nproc)
```

For local development:

```bash
cmake -B build -DFETCHCONTENT_SOURCE_DIR_LU_ASSETS=/path/to/local/lu-assets \
               -DFETCHCONTENT_SOURCE_DIR_TOOL_COMMON=/path/to/local/tool-common
```

## Acknowledgments

Format parsers built from:
- **[DarkflameServer](https://github.com/DarkflameServer/DarkflameServer)** — Terrain format reference (PR #1910)
- **[lcdr/lu_formats](https://github.com/lcdr/lu_formats)** — Kaitai Struct RAW terrain format definition
- **Ghidra reverse engineering** of the original LEGO Universe client binary

## License

[GNU Affero General Public License v3.0](https://www.gnu.org/licenses/agpl-3.0.html) (AGPLv3)

