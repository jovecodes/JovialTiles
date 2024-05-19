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
        bool using_vsize = true;

    public:
        TileMap() = default;

        TileMap(const Texture &texture, Vector2 tile_size)
            : texture(texture), tile_size(tile_size) {}

        bool load_from_jon(jon::JonNode &object);

    public:
        inline void place(Vector2i coord, Vector2i tile) {
            tiles.insert(coord, tile);
        }

        inline void add_tile(Vector2i tile, Rect2 uv) {
            tile_uvs.insert(tile, uv);
        }

        inline virtual void place_auto(Vector2i coord) {
            JV_CORE_ERROR("method: 'place_auto' not available on base TileMap class");
        }
        inline virtual void erase_auto(Vector2i coord) {
            JV_CORE_ERROR("method: 'erase_auto' not available on base TileMap class");
        }

        [[nodiscard]] inline bool has_uv(Vector2i coord) const {
            return tile_uvs.has(coord);
        }

        [[nodiscard]] inline bool has(Vector2i coord) const {
            return tiles.has(coord);
        }

        inline void erase(Vector2i coord) {
            tiles.erase(coord);
        }

        inline void clear() {
            tiles.clear();
        }

        [[nodiscard]] inline Vector2i world_to_coord(Vector2 world) const {
            return (Vector2) (world - position) / tile_size;
        }

        [[nodiscard]] inline Vector2 coord_to_world(Vector2i coord) const {
            return (Vector2) coord * tile_size + position;
        }

        [[nodiscard]] inline bool is_tile_visible(Vector2i coord) const {
            auto tpos = coord_to_world(coord);
            Rect2 tile(tpos, tpos + tile_size);
            return tile.overlaps(Camera2D::get_visable_rect(using_vsize));
        }

        void load_tiles();

        void draw(TextureDrawProps props = {});

        static inline Array<Vector2i, 4> coords_cardinal_to(Vector2i coord) {
            return {
                    Vector2i(0, 1),
                    Vector2i(1, 0),
                    Vector2i(0, -1),
                    Vector2i(-1, 0),
            };
        }
        static inline Array<Vector2i, 8> coords_around(Vector2i coord) {
            return {
                    Vector2i(0, 1),
                    Vector2i(1, 0),
                    Vector2i(0, -1),
                    Vector2i(-1, 0),
                    Vector2i(1, 1),
                    Vector2i(1, -1),
                    Vector2i(-1, -1),
                    Vector2i(-1, 1),
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

        inline void rewang_all() {
            for (auto &tile: tiles) {
                rewang(tile.key);
            }
        }

        inline void rewang(Vector2i coord) {
            if (!has(coord)) return;

            int bits = calc_wang(coord);
            place(coord, wang_tiles[bits]);
        }

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

        inline void place_auto(Vector2i coord) override {
            place_wang(coord);
        }
        inline void erase_auto(Vector2i coord) override {
            erase_wang(coord);
        }

        bool load_wang_from_jon(jon::JonNode &object);
    };

    class BlobTileMap : public TileMap {
    public:
        BlobTileMap(const Array<Vector2i, 256> &blob_tiles, const Texture &texture, Vector2 tile_size)
            : blob_tiles(blob_tiles), TileMap(texture, tile_size) {}

        BlobTileMap() = default;

        Array<Vector2i, 256> blob_tiles;

        static const int N = 0b00000001;
        static const int E = 0b00000010;
        static const int S = 0b00000100;
        static const int W = 0b00001000;
        static const int NE = 0b00010000;
        static const int SE = 0b00100000;
        static const int SW = 0b01000000;
        static const int NW = 0b10000000;

    public:
        int calc_blob(Vector2i coord);

        static inline Vector2i get_direction_vector(int direction) {
            switch (direction) {
                case N:
                    return {0, 1};
                case E:
                    return {1, 0};
                case S:
                    return {0, -1};
                case W:
                    return {-1, 0};
                case NE:
                    return {1, 1};
                case SE:
                    return {1, -1};
                case SW:
                    return {-1, -1};
                case NW:
                    return {-1, 1};
                default:
                    JV_CORE_ERROR("Invalid direction: ", direction, " for blob tileset direction!");
            }
            JV_CORE_UNREACHABLE
        }

        inline void reblob_all() {
            for (auto &tile: tiles) {
                reblob(tile.key);
            }
        }

        inline void reblob(Vector2i coord) {
            if (!has(coord)) return;

            int bits = calc_blob(coord);
            place(coord, blob_tiles[bits]);
        }


        inline void reblob_around(Vector2i coord) {
            for (auto dir: coords_around(coord))
                reblob(coord + dir);
        }

        inline void erase_blob(Vector2i coord) {
            erase(coord);
            reblob_around(coord);
        }

        inline void place_blob(Vector2i coord) {
            int bits = calc_blob(coord);
            place(coord, blob_tiles[bits]);

            reblob_around(coord);
        }

        inline void place_auto(Vector2i coord) override {
            place_blob(coord);
        }
        inline void erase_auto(Vector2i coord) override {
            erase_blob(coord);
        }

        bool load_blob_from_jon(jon::JonNode &object);
    };

    class RuleTileMap : public TileMap {
    public:
        struct Rule {
            Vec<Pair<Vector2i, bool>> needed;
            Vector2i output;
        };

        int effect_range = 1;
        Vec<Rule> rules;

        Vector2i parse_header(StrView &rule_file) {
            rule_file.trim_lead();

            if (rule_file[0] != '[') JV_CORE_FATAL("Expected start of rule: '[' but found: '", rule_file[0], "'")
            rule_file.c_str += 1;
            rule_file.trim_lead();

            Vector2i uv;

            StrView x = rule_file.chop_to(',');
            uv.x = x.to_int();
            rule_file.c_str += x.len;
            rule_file.trim_lead();

            StrView y = rule_file.chop_to(']');
            uv.y = y.to_int();
            rule_file.c_str += y.len;
            rule_file.c_str += 1;// skip ']'
            rule_file.trim_lead();

            return uv;
        }

        void parse(StrView rule_file) {
            rule_file.trim_lead();
            if (rule_file.is_empty()) return;
            Vector2i uv = parse_header(rule_file);

            Vector2i rule_pos;
            int rule_pos_index = rule_file.find_char('x');
            for (int i = 0; i < rule_pos_index; ++i) {
                if (rule_file[i] == '.' || rule_file[i] == '#' || rule_file[i] == '?') {
                    rule_pos.x += 1;
                }
                if (rule_file[i] == '\n') {
                    rule_pos.x = 0;
                    rule_pos.y += 1;
                }
            }

            Rule rule;
            Vector2i pos(0, 0);
            while (true) {
                if (rule_file[0] == '?' || rule_file[0] == 'x') {
                    pos.x += 1;
                } else if (rule_file[0] == '.') {
                    Vector2i rel = pos - rule_pos;
                    rel.y = -rel.y;
                    rule.needed.push_back({rel, false});
                    pos.x += 1;
                } else if (rule_file[0] == '#') {
                    Vector2i rel = pos - rule_pos;
                    rel.y = -rel.y;
                    rule.needed.push_back({rel, true});
                    pos.x += 1;
                } else if (rule_file[0] == '\n') {
                    pos.x = 0;
                    pos.y += 1;
                } else {
                    break;
                }
                rule_file.c_str += 1;
                while (rule_file[0] == ' ') {
                    rule_file.c_str += 1;
                }
            }
            // uv.y = -uv.y;
            rule.output = uv;
            print("Output: ", rule.output, ", Needs: ", rule.needed);
            rules.push_back(rule);

            rule_file.trim_lead();
            if (rule_file.is_empty() || rule_file[0] == '\0') {
                return;
            }

            parse(rule_file);
        }

        inline bool rule_works(const Rule &rule, Vector2i coord) {
            for (auto &need: rule.needed) {
                if (!has(coord + need.first)) {
                    return false;
                }
            }
            return true;
        }

        void rerule(Vector2i coord) {
            for (auto &rule: rules) {
                if (rule_works(rule, coord)) {
                    place(coord, rule.output);
                    return;
                }
            }
            place(coord, {0, 0});
        }

        inline void place_rule(Vector2i coord) {
            for (auto &rule: rules) {
                if (rule_works(rule, coord)) {
                    place(coord, rule.output);
                    for (int i = effect_range; i > 0; --i) {
                        for (auto dir: BlobTileMap::coords_around(coord)) {
                            if (has(coord + dir)) {
                                rerule(coord + dir);
                            }
                        }
                    }
                    return;
                }
            }
            place(coord, {0, 0});
        }

        inline void place_auto(Vector2i coord) override {
            place_rule(coord);
        }
    };

    namespace jon {
        template<>
        struct JonObject<WangTileMap *> {
            static String save(Generator &generator, WangTileMap *v);
        };
    }// namespace jon

#define JOVIAL_TILEMAP_IMPLEMENTATION
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

    void TileMap::draw(TextureDrawProps props) {
        if (!visable) return;

        for (auto &tile: tiles) {
            if (is_tile_visible(tile.key)) {
                Rect2 uv;
                if (!tile_uvs.get_if_contains(tile.value, uv)) {
                    JV_CORE_FATAL("Tilemap does not contain key: ", tile.value);
                }

                props.uv = tile_uvs.get(tile.value);
                draw_texture(texture, coord_to_world(tile.key), props);
            }
        }
    }

    namespace jon {
        String JonObject<TileMap *>::save(Generator &generator, TileMap *v) {
            String res;
            res += generator.push_object();

            res += generator.save_str("size", v->tile_size);

            res += generator.get_indent();
            res += "tiles ";
            res += generator.push_array();
            res += "\n";
            for (auto &tile: v->tiles) {
                res += generator.get_indent();
                res += JonObject<Vector2i>::save(generator, tile.key);
                res += "\n";
                res += generator.get_indent();
                res += JonObject<Vector2i>::save(generator, tile.value);
                res += "\n";
            }
            res += generator.pop_array();
            res += generator.pop_object();
            return res;
        }
    }// namespace jon

    int WangTileMap::calc_wang(Vector2i coord) {
        int bits = 0;

        if (has(coord + Vector2i(0, 1))) {
            bits |= UP;
        }
        if (has(coord + Vector2i(1, 0))) {
            bits |= RIGHT;
        }
        if (has(coord + Vector2i(0, -1))) {
            bits |= DOWN;
        }
        if (has(coord + Vector2i(-1, 0))) {
            bits |= LEFT;
        }
        return bits;
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

    namespace jon {
        String JonObject<WangTileMap *>::save(Generator &generator, WangTileMap *v) {
            String res;
            res += generator.push_object();

            res += generator.save_str("size", v->tile_size);
            res += generator.save_str("wang", v->wang_tiles);

            res += generator.get_indent();
            res += "tiles ";
            res += generator.push_array();
            res += "\n";
            for (auto &tile: v->tiles) {
                res += generator.get_indent();
                res += JonObject<Vector2i>::save(generator, tile.key);
                res += "\n";
                res += generator.get_indent();
                res += JonObject<Vector2i>::save(generator, tile.value);
                res += "\n";
            }
            res += generator.pop_array();
            res += generator.pop_object();
            return res;
        }
    }// namespace jon


    int BlobTileMap::calc_blob(Vector2i coord) {
        int bits = 0;
        for (int direction = 1; direction <= NW; direction *= 2) {
            if (has(coord + get_direction_vector(direction))) {
                bits |= direction;
            }
        }
        return bits;
    }


#endif

}// namespace jovial
