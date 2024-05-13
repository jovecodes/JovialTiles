#pragma once

#include "Jovial/Components/2D/Camera2D.h"
#include "Jovial/Core/Assert.h"
#include "Jovial/JovialEngine.h"
#include "Jovial/Renderer/2DRenderer.h"
#include "Jovial/SavingLoading/Jon.h"
#include "Jovial/SavingLoading/JonStd.h"
#include "Jovial/Std/Array.h"
#include "Jovial/Std/HashMap.h"

namespace jovial {

    class TileMap {
    public:
        Vector2 position;
        Texture texture;
        HashMap<Vector2i, Vector2i> tiles{};
        HashMap<Vector2i, Rect2> tile_uvs;
        Vector2 tile_size;
        bool visable = true;

    public:
        TileMap() = default;

        TileMap(const Texture &texture, Vector2 tile_size)
            : texture(texture), tile_size(tile_size) {}

        bool load_from_jon(jon::JonNode &object);

    public:
        inline virtual void place(Vector2i coord, Vector2i tile) {
            tiles.insert(coord, tile);
        }

        inline virtual void add_tile(Vector2i tile, Rect2 uv) {
            tile_uvs.insert(tile, uv);
        }

        inline bool contains_uv(Vector2i coord) {
            return tile_uvs.contains(coord);
        }

        inline bool contains(Vector2i coord) {
            return tiles.contains(coord);
        }

        inline void erase(Vector2i coord) {
            tiles.erase(coord);
        }

        inline void clear() {
            tiles.clear();
        }

        [[nodiscard]] inline Vector2i world_to_coord(Vector2 world) const {
            return world / tile_size;
        }

        [[nodiscard]] inline Vector2 coord_to_world(Vector2i coord) const {
            return Vector2(coord.x, coord.y) * tile_size + position;
        }

        [[nodiscard]] inline bool is_tile_visible(Vector2i coord) const {
            auto tpos = coord_to_world(coord);
            Rect2 tile(tpos, tpos + tile_size);
            return tile.overlaps(Camera2D::get_visable_rect());
        }

        void load_tiles();

        void draw(rendering::TextureDrawProps props = {});

        static inline Array<Vector2i, 4> coords_cardinal_to(Vector2i coord) {
            return {
                    Vector2i(0, 1),
                    Vector2i(1, 0),
                    Vector2i(0, -1),
                    Vector2i(-1, 0),
            };
        }
    };

    namespace jon {
        template<>
        struct JonObject<TileMap *> {
            static String save(Generator &generator, TileMap *v);
        };
    }// namespace jon


    class WangTileMap : public TileMap {
    public:
        WangTileMap(const Array<Vector2i, 16> &wang_tiles, const Texture &texture, Vector2 tile_size)
            : wang_tiles(wang_tiles), TileMap(texture, tile_size) {}

        WangTileMap() = default;

        Array<Vector2i, 16> wang_tiles;

        static const int UP = 0b0001;
        static const int RIGHT = 0b0010;
        static const int DOWN = 0b0100;
        static const int LEFT = 0b1000;

    public:
        int calc_wang(Vector2i coord);

        inline void rewang(Vector2i coord) {
            if (!contains(coord)) return;

            int bits = calc_wang(coord);
            place(coord, wang_tiles[bits]);
        }

        void rewang_all();

        inline void rewang_around(Vector2i coord) {
            for (auto dir: coords_cardinal_to(coord))
                rewang(coord + dir);
        }

        inline void erase_wang(Vector2i coord) {
            erase(coord);
            rewang_around(coord);
        }

        inline void place_wang(Vector2i coord) {
            int bits = calc_wang(coord);
            place(coord, wang_tiles[bits]);

            rewang_around(coord);
        }

        bool load_wang_from_jon(jon::JonNode &object);
    };

    namespace jon {
        template<>
        struct JonObject<WangTileMap *> {
            static String save(Generator &generator, WangTileMap *v);
        };
    }// namespace jon

#ifdef JOVIAL_TILEMAP_IMPLEMENTATION

    bool TileMap::load_from_jon(jon::JonNode &object) {
        JV_CORE_ASSERT(object.kind == jon::JonNode::Object);

        jon::JonNode temp{};

        if (object.as.object->get_if_contains(C_STR_VIEW("size"), temp)) {
            JV_CORE_ASSERT(temp.kind == jon::JonNode::Vec2);
            tile_size = temp.as.val.vec2;
        } else {
            JV_CORE_ERROR("Could not load tilemap from jon object because it did not specify tile size");
            return false;
        }

        if (object.as.object->get_if_contains(C_STR_VIEW("tiles"), temp)) {
            JV_CORE_ASSERT(temp.kind == jon::JonNode::Array);
            bool on_coord = true;
            Vector2i coord;
            for (auto &val: *temp.as.arr) {
                if (on_coord) {
                    coord = val.vec2i;
                } else {
                    place(coord, val.vec2i);
                }
                on_coord = !on_coord;
            }
        } else {
            JV_CORE_ERROR("Could not load tilemap from jon object because it did not specify the tiles");
            return false;
        }
        return true;
    }

    void TileMap::load_tiles() {
        float tiles_x = (float) texture.width / tile_size.x;
        float tiles_y = (float) texture.height / tile_size.y;
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

    void TileMap::draw(rendering::TextureDrawProps props) {
        if (!visable) return;

        for (int i = 0; i < JV_HASH_TABLE_SIZE; ++i) {
            auto *tile = tiles.table[i];
            while (tile != nullptr) {
                if (is_tile_visible(tile->key)) {
                    Rect2 uv;
                    if (!tile_uvs.get_if_contains(tile->value, uv)) {
                        JV_CORE_FATAL("Tilemap does not contain key: ", tile->value);
                    }

                    props.uv = tile_uvs.get(tile->value);
                    rendering::draw_texture(texture, coord_to_world(tile->key), props);
                }

                tile = tile->next;
            }
        }
    }

    jovial::String ::JonObject<TileMap *>::save(Generator &generator, TileMap *v) {
        String res;
        res += generator.push_object();

        res += generator.save_str("size", v->tile_size);

        res += generator.get_indent();
        res += "tiles ";
        res += generator.push_array();
        res += "\n";
        for (int i = 0; i < JV_HASH_TABLE_SIZE; ++i) {
            auto *tile = v->tiles.table[i];
            while (tile != nullptr) {
                res += generator.get_indent();
                res += JonObject<Vector2i>::save(generator, tile->key);
                res += "\n";
                res += generator.get_indent();
                res += JonObject<Vector2i>::save(generator, tile->value);
                res += "\n";
                tile = tile->next;
            }
        }
        res += generator.pop_array();
        res += generator.pop_object();
        return res;
    }
    int WangTileMap::calc_wang(Vector2i coord) {
        int bits = 0;

        if (contains(coord + Vector2i(0, 1))) {
            bits |= UP;
        }
        if (contains(coord + Vector2i(1, 0))) {
            bits |= RIGHT;
        }
        if (contains(coord + Vector2i(0, -1))) {
            bits |= DOWN;
        }
        if (contains(coord + Vector2i(-1, 0))) {
            bits |= LEFT;
        }
        return bits;
    }

    void WangTileMap::rewang_all() {
        for (int i = 0; i < JV_HASH_TABLE_SIZE; ++i) {
            auto *tile = tiles.table[i];
            while (tile != nullptr) {
                rewang(tile->key);
                tile = tile->next;
            }
        }
    }

    bool WangTileMap::load_wang_from_jon(jon::JonNode &object) {
        if (!load_from_jon(object)) return false;

        jon::JonNode temp{};
        if (object.as.object->get_if_contains(C_STR_VIEW("wang"), temp)) {
            JV_CORE_ASSERT(temp.kind == jon::JonNode::Array);
            for (int i = 0; i < temp.as.arr->size(); ++i) {
                wang_tiles[i] = (*temp.as.arr)[i].vec2i;
            }
        } else {
            JV_CORE_ERROR("Could not load wang tilemap from jon object because it did not specify wang tiles");
            return false;
        }

        return true;
    }

    String JonObject<WangTileMap *>::save(Generator &generator, WangTileMap *v) {
        String res;
        res += generator.push_object();

        res += generator.save_str("size", v->tile_size);
        res += generator.save_str("wang", v->wang_tiles);

        res += generator.get_indent();
        res += "tiles ";
        res += generator.push_array();
        res += "\n";
        for (int i = 0; i < JV_HASH_TABLE_SIZE; ++i) {
            auto *tile = v->tiles.table[i];
            while (tile != nullptr) {
                res += generator.get_indent();
                res += JonObject<Vector2i>::save(generator, tile->key);
                res += "\n";
                res += generator.get_indent();
                res += JonObject<Vector2i>::save(generator, tile->value);
                res += "\n";
                tile = tile->next;
            }
        }
        res += generator.pop_array();
        res += generator.pop_object();
        return res;
    }

#endif

}// namespace jovial
