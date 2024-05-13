#pragma once

#include "Jovial/Components/2D/Camera2D.h"
#include "Jovial/Core/Logger.h"
#include "Jovial/JovialEngine.h"
#include "Jovial/Renderer/TextRenderer.h"
#include "Jovial/SavingLoading/Jon.h"
#include "Jovial/Shapes/Color.h"
#include "Jovial/Shapes/Rect.h"
#include "Jovial/Shapes/ShapeDrawer.h"
#include "JovialTileMap.h"

using namespace jovial;

class TileMapEditor {
public:
    TileMapEditor() = default;

    TileMapEditor(const Texture &texture, Font *font)
        : tile_map(WangTileMap({}, texture, {32, 32})),
          editable_tile_map(WangTileMap({}, texture, {32, 32})),
          font(font) {
        // String file = fs::read_entire_file(fs::Path::res() + "map.jon");
        // jon::Lexer lexer(file);
        // jon::Parser parser(lexer);
        // tile_map = loader.load<WangTileMap *>("map");
        // tile_map->transform.z_index = -1;
        //

        // add_child(tile_map);
        // add_child(editable_tile_map);
    }

    void init(const Texture &texture, Font *font, Vector2 tile_size) {
        this->font = font;

        tile_map.tile_size = tile_size;
        tile_map.texture = texture;
        tile_map.load_tiles();

        editable_tile_map.tile_size = tile_size;
        editable_tile_map.texture = texture;
        editable_tile_map.load_tiles();

        editable_tile_map.visable = false;
        Vector2i pos(1, 10);
        for (int i = 0; i < JV_HASH_TABLE_SIZE; ++i) {
            auto *t = editable_tile_map.tile_uvs.table[i];
            while (t != nullptr) {
                editable_tile_map.place(Vector2i{t->key.x, -t->key.y} + pos, t->key);
                t = t->next;
            }
        }
        for (int i = 0; i < tile_map.wang_tiles.length; ++i) {
            auto it = tile_map.wang_tiles[i];
            edited_tiles.insert({it.x + pos.x, pos.y - it.y}, i);
        }
    }

    void update() {
        const char *edit_mode_str;
        switch (edit_mode) {
            case DRAWING: {
                edit_mode_str = "Drawing";
                editable_tile_map.visable = false;
            } break;
            case EDITING: {
                edit_mode_str = "Editing";
                editable_tile_map.visable = true;
            } break;
            default: {
                JV_CORE_WARN("Unknown edit mode {}", (int) edit_mode);
                edit_mode = DRAWING;
                editable_tile_map.visable = false;
            }
        }


        TextDrawProps props = {
                .color = Colors::White,
                .font_size = 16.0f / Camera2D::get_current_zoom(),
        };
        float offset = (1 - Camera2D::get_current_zoom()) * 16.0f;
        auto pos = as_ui({0, 10});// - Vector2(-offset, offset);
        font->draw(pos, edit_mode_str, props);

        if (Input::is_just_pressed(Actions::S) && Input::is_pressed(Actions::LeftControl)) {
            jon::Generator generator;
            // jon::JonSaver saver;
            // saver.save("map", tile_map);
            // saver.write_to(fs::Path::res() + "map.jon");
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

        tile_map.draw();
        editable_tile_map.draw({.z_index = 5});
    }

    void draw() {
        if (Input::is_pressed(Actions::LeftMouseButton)) {
            // tile_map.place(tile_map.world_to_coord(Input::get_mouse_position()), tile);
            tile_map.place_wang(tile_map.world_to_coord(Input::get_mouse_position()));
        } else if (Input::is_pressed(Actions::RightMouseButton)) {
            tile_map.erase_wang(tile_map.world_to_coord(Input::get_mouse_position()));
        }
    }

    void edit() {
        draw_rect2({{0, 0}, Window::get_current_vsize()}, {.color = Color(0, 0, 0, 126), .z_index = 1});
        auto color = Color(14, 16, 144, 170);
        ShapeDrawProps props{.color = color, .z_index = 100};

        for (int i = 0; i < JV_HASH_TABLE_SIZE; ++i) {
            auto *t = edited_tiles.table[i];
            while (t != nullptr) {
                Vector2 pos = editable_tile_map.coord_to_world(t->key);
                int bits = t->value;

                auto rects = get_edit_squares(pos, bits);

                if (bits & WangTileMap::UP) {
                    draw_rect2(rects[0], props);
                }
                if (bits & WangTileMap::RIGHT) {
                    draw_rect2(rects[1], props);
                }
                if (bits & WangTileMap::DOWN) {
                    draw_rect2(rects[2], props);
                }
                if (bits & WangTileMap::LEFT) {
                    draw_rect2(rects[3], props);
                }
                draw_rect2(rects[4], props);

                t = t->next;
            }
        }

        Vector2i coord = editable_tile_map.world_to_coord(Input::get_mouse_position());
        if (!editable_tile_map.contains(coord)) {
            return;
        }
        Vector2 pos = editable_tile_map.coord_to_world(coord);
        int bits = 0;
        edited_tiles.get_if_contains(coord, bits);

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
                edited_tiles.insert(coord, bits);
                tile_map.wang_tiles[bits] = editable_tile_map.tiles.get(coord);
            }
        } else if (Input::is_pressed(Actions::RightMouseButton)) {
            erase_edit(coord, rects, bits);
        }
    }

    void save_all_edits() {
        for (int i = 0; i < JV_HASH_TABLE_SIZE; ++i) {
            auto *t = edited_tiles.table[i];
            while (t != nullptr) {
                tile_map.wang_tiles[t->value] = editable_tile_map.tiles.get(t->key);
                t = t->next;
            }
        }
    }

    void erase_edit(Vector2i coord, Array<Rect2, 5> &rects, int bits) {
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
            edited_tiles.insert(coord, bits);
        }

        save_all_edits();
    }

    [[nodiscard]] Array<Rect2, 5> get_edit_squares(Vector2 pos, int bits) const {
        auto tile_size = editable_tile_map.tile_size;
        return {
                {pos + Vector2(tile_size.x / 3, 2 * tile_size.y / 3),
                 pos + Vector2(2 * tile_size.x / 3, tile_size.y)},

                {pos + Vector2(2 * tile_size.x / 3, tile_size.y / 3),
                 pos + Vector2(tile_size.x, 2 * tile_size.y / 3)},

                {pos + Vector2(tile_size.x / 3, 0.0f),
                 pos + Vector2(2 * tile_size.x / 3, tile_size.y / 3)},

                {pos + Vector2(0.0f, tile_size.y / 3),
                 pos + Vector2(tile_size.x / 3, 2 * tile_size.y / 3)},

                {pos + Vector2(tile_size.x / 3, tile_size.y / 3),
                 pos + Vector2(2 * tile_size.x / 3, 2 * tile_size.y / 3)},
        };
    }

    enum {
        DRAWING,
        EDITING,
    } edit_mode = DRAWING;

    Font *font{};
    HashMap<Vector2i, int> edited_tiles;// Table of the edited keys (the blue boxes that are used for wanging)
    WangTileMap editable_tile_map;      // The tile map that is shown in edit mode that allows you to change the edited_tiles
    WangTileMap tile_map;               // The displayed tile map that you paint in drawing mode
};
