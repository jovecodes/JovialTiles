#pragma once

#include "Jovial/Components/2D/Camera2D.h"
#include "Jovial/Components/2D/NodeTransform2D.h"
#include "Jovial/Components/Util/Visibility.h"
#include "Jovial/Core/Maths.h"
#include "Jovial/JovialEngine.h"
#include "Jovial/Renderer/Sprite2DRenderer.h"

using namespace jovial;

class TileMap : public Node {
public:
    NodeTransform2D transfrom;
    Texture *texture;
    std::unordered_map<glm::ivec2, glm::ivec2> tiles;
    std::unordered_map<glm::ivec2, Rect2> tile_uvs;
    Vector2 tile_size;

    TileMap(NodeTransform2D *parent_transform, Texture *texture, Vector2 tile_size)
        : transfrom(parent_transform), texture(texture), tile_size(tile_size) {}

public:
    virtual void place(glm::ivec2 coord, glm::ivec2 tile) {
        tiles.insert_or_assign(coord, tile);
    }

    virtual void add_tile(glm::ivec2 tile, Rect2 uv) {
        tile_uvs.emplace(tile, uv);
    }

    bool contains_uv(glm::ivec2 coord) {
        return tile_uvs.find(coord) != tile_uvs.end();
    }

    bool contains(glm::ivec2 coord) {
        return tiles.find(coord) != tiles.end();
    }

    void erase(glm::ivec2 coord) {
        tiles.erase(coord);
    }

    glm::ivec2 world_to_coord(Vector2 world) const {
        return world / tile_size;
    }

    Vector2 coord_to_world(glm::ivec2 coord) const {
        return Vector2(coord.x, coord.y) * tile_size;
    }

    bool is_tile_visible(glm::ivec2 coord) const {
        auto tpos = coord_to_world(coord);
        Rect2 tile(tpos, tpos + tile_size);

        return util::is_visible(tile);
    }

    void load_tiles() {
        float tiles_x = (float) texture->get_width() / tile_size.x;
        float tiles_y = (float) texture->get_height() / tile_size.y;
        float w = 1.0f / tiles_x;
        float h = 1.0f / tiles_y;

        for (int x = 0; x < (int) tiles_x; ++x) {
            for (int y = 0; y < (int) tiles_y; ++y) {
                Rect2 uv = {w * (float) x, h * (float) y,
                            w * ((float) x + 1), h * ((float) y + 1)};
                add_tile({x, y}, uv);
            }
        }
    }

protected:
    void update() override {
        for (auto &tile: tiles) {
            if (!is_tile_visible(tile.first)) {
                continue;
            }
            if (tile_uvs.find(tile.second) == tile_uvs.end()) {
                JV_CORE_FATAL("Tilemap does not contain key {}", tile.second);
            }

            rendering::TextureDrawProperties props;
            props.uv = tile_uvs[tile.second];
            rendering::draw_texture(texture, coord_to_world(tile.first), props);
        }
    }
};

class WangTileMap : public TileMap {
public:
    WangTileMap(std::array<glm::ivec2, 16> wang_tiles, NodeTransform2D *parent_transform, Texture *texture, Vector2 tile_size)
        : wang_tiles(wang_tiles), TileMap(parent_transform, texture, tile_size) {}

    std::array<glm::ivec2, 16> wang_tiles;

    static const int UP = 0b0001;
    static const int RIGHT = 0b0010;
    static const int DOWN = 0b0100;
    static const int LEFT = 0b1000;

public:
    int calc_wang(glm::ivec2 coord) {
        int bits = 0;

        if (contains(coord + glm::ivec2(0, 1))) {
            bits |= UP;
        }
        if (contains(coord + glm::ivec2(1, 0))) {
            bits |= RIGHT;
        }
        if (contains(coord + glm::ivec2(0, -1))) {
            bits |= DOWN;
        }
        if (contains(coord + glm::ivec2(-1, 0))) {
            bits |= LEFT;
        }
        return bits;
    }

    void rewang(glm::ivec2 coord) {
        if (!contains(coord)) return;

        int bits = calc_wang(coord);
        place(coord, wang_tiles[bits]);
    }

    void rewang_around(glm::ivec2 coord) {
        rewang(coord + glm::ivec2(0, 1));
        rewang(coord + glm::ivec2(1, 0));
        rewang(coord + glm::ivec2(0, -1));
        rewang(coord + glm::ivec2(-1, 0));
    }

    void erase_wang(glm::ivec2 coord) {
        erase(coord);
        rewang_around(coord);
    }

    void place_wang(glm::ivec2 coord) {
        int bits = calc_wang(coord);
        place(coord, wang_tiles[bits]);

        rewang_around(coord);
    }
};
