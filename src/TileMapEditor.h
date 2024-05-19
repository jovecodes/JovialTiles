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
        : tile_map(new WangTileMap({}, texture, {32, 32})),
          editable_tile_map(new WangTileMap({}, texture, {32, 32})),
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

        delete tile_map;
        delete editable_tile_map;

        switch (mode) {
            case TileMapMode::Wang:
                tile_map = new WangTileMap();
                editable_tile_map = new WangTileMap();
                break;
            case TileMapMode::Blob:
                tile_map = new BlobTileMap();
                editable_tile_map = new BlobTileMap();
                break;
            default:
                JV_CORE_FATAL("Unknown tileset mode: ", (int) mode, "!")
        }

        tile_map->tile_size = tile_size;
        tile_map->texture = texture;
        tile_map->load_tiles();
        tile_map->using_vsize = false;

        editable_tile_map->tile_size = tile_size;
        editable_tile_map->texture = texture;
        editable_tile_map->load_tiles();
        editable_tile_map->using_vsize = false;

        editable_tile_map->visable = false;
        Vector2i pos(0, 0);
        for (auto &t: editable_tile_map->tile_uvs) {
            editable_tile_map->place(Vector2i{t.key.x, -t.key.y} + pos, t.key);
        }

        switch (mode) {
            case TileMapMode::Wang: {
                for (int i = 0; i < ((WangTileMap *) tile_map)->wang_tiles.length; ++i) {
                    auto it = ((WangTileMap *) tile_map)->wang_tiles[i];
                    edited_tiles.insert({it.x + pos.x, pos.y - it.y}, i);
                }
            } break;
            case TileMapMode::Blob: {
                for (int i = 0; i < ((BlobTileMap *) tile_map)->blob_tiles.length; ++i) {
                    auto it = ((BlobTileMap *) tile_map)->blob_tiles[i];
                    edited_tiles.insert({it.x + pos.x, pos.y - it.y}, i);
                }
            } break;
        }
    }

    void update() {
        const char *edit_mode_str;
        switch (edit_mode) {
            case DRAWING: {
                edit_mode_str = "Drawing";
                editable_tile_map->visable = false;
            } break;
            case EDITING: {
                edit_mode_str = "Editing";
                editable_tile_map->visable = true;
            } break;
            default: {
                JV_CORE_WARN("Unknown edit mode {}", (int) edit_mode);
                edit_mode = DRAWING;
                editable_tile_map->visable = false;
            }
        }

        TextDrawProps props = {
                .color = Colors::White,
                .font_size = 16.0f / Camera2D::get_current_zoom(),
        };
        float offset = (1 - Camera2D::get_current_zoom()) * 16.0f;
        auto pos = as_ui({0, 10});// - Vector2(-offset, offset);
        draw_circle({2, pos + Vector2(5, 0)}, 16, {.color = Colors::Red});
        font->draw(pos, edit_mode_str, props);

        if (Input::is_just_pressed(Actions::S) && Input::is_pressed(Actions::LeftControl)) {
            jon::Generator generator;
            switch (mode) {
                case TileMapMode::Wang:
                    generator.save("tilemap", (WangTileMap *) tile_map);
                    break;
                default:
                    JV_TODO();
            }
            generator.generate(fs::Path::res() + "map.jon");
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

        tile_map->draw();

        Rect2 rect = Camera2D::get_visable_rect(false);
        editable_tile_map->position = rect.position() + rect.size() / 2.0f;
        editable_tile_map->draw({.z_index = 5});
    }

    void draw() const {
        if (Input::is_pressed(Actions::LeftMouseButton)) {
            // tile_map.place(tile_map.world_to_coord(Input::get_mouse_position()), tile);
            tile_map->place_auto(tile_map->world_to_coord(Input::get_mouse_position() / Camera2D::get_current_zoom()));
        } else if (Input::is_pressed(Actions::RightMouseButton)) {
            tile_map->erase_auto(tile_map->world_to_coord(Input::get_mouse_position() / Camera2D::get_current_zoom()));
        }
    }

    void edit() {
        draw_rect2(Camera2D::get_visable_rect(false), {.color = Color(0, 0, 0, 126), .z_index = 1});
        auto color = Color(55, 55, 255, 170);
        ShapeDrawProps props{.color = color, .z_index = 100};

        for (auto &t: edited_tiles) {
            Vector2 pos = editable_tile_map->coord_to_world(t.key);
            int bits = t.value;

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
            if (mode == TileMapMode::Blob) {
                if (bits & BlobTileMap::NE) {
                    draw_rect2(rects[5], props);
                }
                if (bits & BlobTileMap::SE) {
                    draw_rect2(rects[6], props);
                }
                if (bits & BlobTileMap::SW) {
                    draw_rect2(rects[7], props);
                }
                if (bits & BlobTileMap::NW) {
                    draw_rect2(rects[8], props);
                }
            }
        }

        Vector2i coord = editable_tile_map->world_to_coord(Input::get_mouse_position() / Camera2D::get_current_zoom());

        if (editable_tile_map->has(coord)) {
            Vector2 pos = editable_tile_map->coord_to_world(coord);

            int bits = 0;
            edited_tiles.get_if_contains(coord, bits);

            auto rects = get_edit_squares(pos, bits);
            bool is_single_tile = false;
            if (Input::is_pressed(Actions::LeftMouseButton)) {
                if (rects[0].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                    bits |= WangTileMap::UP;
                }
                if (rects[1].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                    bits |= WangTileMap::RIGHT;
                }
                if (rects[2].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                    bits |= WangTileMap::DOWN;
                }
                if (rects[3].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                    bits |= WangTileMap::LEFT;
                }
                if (rects[4].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                    is_single_tile = true;
                }
                if (mode == TileMapMode::Blob) {
                    if (rects[5].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                        bits |= BlobTileMap::NE;
                    }
                    if (rects[6].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                        bits |= BlobTileMap::SE;
                    }
                    if (rects[7].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                        bits |= BlobTileMap::SW;
                    }
                    if (rects[8].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
                        bits |= BlobTileMap::NW;
                    }
                }

                if (bits || is_single_tile) {
                    edited_tiles.insert(coord, bits);

                    switch (mode) {
                        case TileMapMode::Wang:
                            ((WangTileMap *) tile_map)->wang_tiles[bits] = editable_tile_map->tiles.get(coord);
                            break;
                        case TileMapMode::Blob:
                            ((BlobTileMap *) tile_map)->blob_tiles[bits] = editable_tile_map->tiles.get(coord);
                            break;
                    }
                }
            } else if (Input::is_pressed(Actions::RightMouseButton)) {
                erase_edit(coord, rects, bits);
            }
        }
    }

    void save_all_edits() {
        for (auto &t: edited_tiles) {
            switch (mode) {
                case TileMapMode::Wang:
                    ((WangTileMap *) tile_map)->wang_tiles[t.value] = editable_tile_map->tiles.get(t.key);
                    break;
                case TileMapMode::Blob:
                    ((BlobTileMap *) tile_map)->blob_tiles[t.value] = editable_tile_map->tiles.get(t.key);
                    break;
            }
        }
    }

    void erase_edit(Vector2i coord, Vec<Rect2> &rects, int bits) {
        if (rects[0].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom()) && bits & WangTileMap::UP) {
            bits ^= WangTileMap::UP;
        }
        if (rects[1].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom()) && bits & WangTileMap::RIGHT) {
            bits ^= WangTileMap::RIGHT;
        }
        if (rects[2].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom()) && bits & WangTileMap::DOWN) {
            bits ^= WangTileMap::DOWN;
        }
        if (rects[3].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom()) && bits & WangTileMap::LEFT) {
            bits ^= WangTileMap::LEFT;
        }
        if (mode == TileMapMode::Blob) {
            if (rects[5].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom()) && bits & BlobTileMap::NE) {
                bits ^= BlobTileMap::NE;
            }
            if (rects[6].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom()) && bits & BlobTileMap::SE) {
                bits ^= BlobTileMap::SE;
            }
            if (rects[7].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom()) && bits & BlobTileMap::SW) {
                bits ^= BlobTileMap::SW;
            }
            if (rects[8].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom()) && bits & BlobTileMap::NW) {
                bits ^= BlobTileMap::NW;
            }
        }
        if (rects[4].overlaps(Input::get_mouse_position() / Camera2D::get_current_zoom())) {
            edited_tiles.erase(coord);
        } else if (bits) {
            edited_tiles.insert(coord, bits);
        }

        save_all_edits();
    }

    [[nodiscard]] Vec<Rect2> get_edit_squares(Vector2 pos, int bits) const {
        auto tile_size = editable_tile_map->tile_size;
        Vec<Rect2> res = {
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

        if (mode == TileMapMode::Blob) {
            res.push_back({pos + Vector2(2 * tile_size.x / 3, 2 * tile_size.y / 3),
                           pos + Vector2(tile_size.x, tile_size.y)});

            res.push_back({pos + Vector2(2 * tile_size.x / 3, 0.0f),
                           pos + Vector2(tile_size.x, tile_size.y / 3)});

            res.push_back({pos + Vector2(0.0f, 0.0f),
                           pos + Vector2(tile_size.x / 3, tile_size.y / 3)});

            res.push_back({pos + Vector2(0.0f, 2 * tile_size.y / 3),
                           pos + Vector2(tile_size.x / 3, tile_size.y)});

            // static const int NE = 0b00010000;
            // static const int SE = 0b00100000;
            // static const int SW = 0b01000000;
            // static const int NW = 0b10000000;
        }

        return res;
    }

    ~TileMapEditor() {
        delete tile_map;
        delete editable_tile_map;
    }

    enum {
        DRAWING,
        EDITING,
    } edit_mode = DRAWING;

    Font *font{};
    HashMap<Vector2i, int> edited_tiles;// Table of the edited keys (the blue boxes that are used for wanging)

    enum class TileMapMode {
        Wang,
        Blob,
    };
    TileMapMode mode = TileMapEditor::TileMapMode::Wang;
    TileMap *editable_tile_map = nullptr;// The tile map that is shown in edit mode that allows you to change the edited_tiles
    TileMap *tile_map = nullptr;         // The displayed tile map that you paint in drawing mode
};
