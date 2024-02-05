#pragma once

#include "Jovial/Components/2D/Camera2D.h"
#include "Jovial/Components/2D/DefaultPlugins2D.h"
#include "Jovial/Core/Logger.h"
#include "Jovial/FileSystem/Path.h"
#include "Jovial/JovialEngine.h"
#include "Jovial/Renderer/TextRenderer.h"
#include "Jovial/SavingLoading/Jon.h"
#include "Jovial/Shapes/Color.h"
#include "Jovial/Shapes/Rect.h"
#include "TileMap.h"

using namespace jovial;

class TileMapEditor : public NodeLayer {
public:
    TileMapEditor() : tile_map(new WangTileMap({}, nullptr, TextureBank::get("test"), {32, 32})) {

#if 1
        delete tile_map;
        jon::JonLoader loader(fs::Path::res() + "map.jon");
        tile_map = loader.load<WangTileMap *>("map");
        for (auto i: tile_map->wang_tiles) {
            printj(i);
        }
#else
        tile_map->load_tiles();
        glm::ivec2 pos(1, 10);
        for (auto t: tile_map->tile_uvs) {
            tile_map->place({t.first.x + pos.x, pos.y - t.first.y}, t.first);
        }
#endif

        add_child(tile_map);
    }

    void death() override {
        delete tile_map;
        tile_map = nullptr;
    }

    void update() override {
        const char *edit_mode_str;
        switch (edit_mode) {
            case DRAWING:
                edit_mode_str = "Drawing";
                break;
            case EDITING:
                edit_mode_str = "Editing";
                break;
            default:
                JV_CORE_WARN("Unknown edit mode {}", (int) edit_mode);
                edit_mode = DRAWING;
        }


        TextDrawProps props = {
                .color = Colors::White,
                .font_size = 16.0f / Camera2D::get_current_zoom(),
        };
        float offset = (1 - Camera2D::get_current_zoom()) * 16.0f;
        auto pos = as_ui({0, 0});// - Vector2(-offset, offset);
        FontBank::draw(pos, edit_mode_str, props);

        if (Input::is_just_pressed(Actions::S) && Input::is_pressed(Actions::LeftControl)) {
            std::stringstream output;
            output << "{";
            for (auto &t: tile_map->wang_tiles) {
                output << "glm::ivec2{" << t.x << "," << t.y << "},";
            }
            output << "},\n";

            jon::JonSaver saver;
            saver.save("map", tile_map);
            saver.write_to(fs::Path::res() + "map.jon");

            printj("Wang Tiles: {}", output.str());
        }

        if (Input::is_just_pressed(Actions::B)) {
            save_all_edits();
            edit_mode = DRAWING;
        } else if (Input::is_just_pressed(Actions::E)) {
            edit_mode = EDITING;
        }

        if (edit_mode == DRAWING) {
            draw();
        } else if (edit_mode == EDITING) {
            edit();
        }
    }

    void draw() const {
        if (Input::is_pressed(Actions::LeftMouseButton)) {
            // tile_map.place(tile_map.world_to_coord(Input::get_mouse_position()), tile);
            tile_map->place_wang(tile_map->world_to_coord(Input::get_mouse_position()));
        } else if (Input::is_pressed(Actions::RightMouseButton)) {
            tile_map->erase_wang(tile_map->world_to_coord(Input::get_mouse_position()));
        }
    }

    void edit() {
        auto color = HEX_COLOR(0xee004090);

        for (auto t: edited_tiles) {
            Vector2 pos = tile_map->coord_to_world(t.first);
            int bits = t.second;

            auto rects = get_edit_squares(pos, bits);

            if (bits & WangTileMap::UP) {
                rendering::draw_rect2(rects[0], {color});
            }
            if (bits & WangTileMap::RIGHT) {
                rendering::draw_rect2(rects[1], {color});
            }
            if (bits & WangTileMap::DOWN) {
                rendering::draw_rect2(rects[2], {color});
            }
            if (bits & WangTileMap::LEFT) {
                rendering::draw_rect2(rects[3], {color});
            }
            rendering::draw_rect2(rects[4], {color});
        }

        glm::ivec2 coord = tile_map->world_to_coord(Input::get_mouse_position());
        if (!tile_map->contains(coord)) {
            return;
        }
        Vector2 pos = tile_map->coord_to_world(coord);
        int bits = 0;
        if (edited_tiles.find(coord) != edited_tiles.end()) {
            bits = edited_tiles[coord];
        }

        auto rects = get_edit_squares(pos, bits);
        bool is_single_tile = false;
        if (Input::is_pressed(Actions::LeftMouseButton)) {
            if (rects[0].overlaps(Input::get_mouse_position())) {
                bits |= WangTileMap::UP;
            }
            if (rects[1].overlaps(Input::get_mouse_position())) {
                bits |= WangTileMap::RIGHT;
            }
            if (rects[2].overlaps(Input::get_mouse_position())) {
                bits |= WangTileMap::DOWN;
            }
            if (rects[3].overlaps(Input::get_mouse_position())) {
                bits |= WangTileMap::LEFT;
            }
            if (rects[4].overlaps(Input::get_mouse_position())) {
                is_single_tile = true;
            }

            if (bits || is_single_tile) {
                edited_tiles[coord] = bits;
                tile_map->wang_tiles[bits] = tile_map->tiles[coord];
            }
        } else if (Input::is_pressed(Actions::RightMouseButton)) {
            erase_edit(coord, rects, bits);
        }
    }

    void save_all_edits() {
        for (auto &t: edited_tiles) {
            tile_map->wang_tiles[t.second] = tile_map->tiles[t.first];
        }
    }

    void erase_edit(glm::ivec2 coord, std::vector<Rect2> &rects, int bits) {
        if (rects[0].overlaps(Input::get_mouse_position()) && bits & WangTileMap::UP) {
            bits ^= WangTileMap::UP;
        }
        if (rects[1].overlaps(Input::get_mouse_position()) && bits & WangTileMap::RIGHT) {
            bits ^= WangTileMap::RIGHT;
        }
        if (rects[2].overlaps(Input::get_mouse_position()) && bits & WangTileMap::DOWN) {
            bits ^= WangTileMap::DOWN;
        }
        if (rects[3].overlaps(Input::get_mouse_position()) && bits & WangTileMap::LEFT) {
            bits ^= WangTileMap::LEFT;
        }
        if (rects[4].overlaps(Input::get_mouse_position())) {
            edited_tiles.erase(coord);
        } else if (bits) {
            edited_tiles[coord] = bits;
        }

        save_all_edits();
    }

    std::vector<Rect2> get_edit_squares(Vector2 pos, int bits) const {
        std::vector<Rect2> rects;

        rects.emplace_back(
                pos + Vector2(tile_map->tile_size.x / 3, 2 * tile_map->tile_size.y / 3),
                pos + Vector2(2 * tile_map->tile_size.x / 3, tile_map->tile_size.y));

        rects.emplace_back(
                pos + Vector2(2 * tile_map->tile_size.x / 3, tile_map->tile_size.y / 3),
                pos + Vector2(tile_map->tile_size.x, 2 * tile_map->tile_size.y / 3));

        rects.emplace_back(
                pos + Vector2(tile_map->tile_size.x / 3, 0),
                pos + Vector2(2 * tile_map->tile_size.x / 3, tile_map->tile_size.y / 3));

        rects.emplace_back(
                pos + Vector2(0, tile_map->tile_size.y / 3),
                pos + Vector2(tile_map->tile_size.x / 3, 2 * tile_map->tile_size.y / 3));

        rects.emplace_back(
                pos + Vector2(tile_map->tile_size.x / 3, tile_map->tile_size.y / 3),
                pos + Vector2(2 * tile_map->tile_size.x / 3, 2 * tile_map->tile_size.y / 3));

        return rects;
    }

    enum {
        DRAWING,
        EDITING,
    } edit_mode = DRAWING;

    std::unordered_map<glm::ivec2, int> edited_tiles;
    WangTileMap *tile_map;
};
