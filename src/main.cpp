#include "Jovial/Components/2D/Camera2D.h"
#include "Jovial/Components/2D/DefaultPlugins2D.h"
#include "Jovial/Components/Util/LazyAssets.h"
#include "Jovial/JovialEngine.h"
#include "Jovial/Renderer/Renderer.h"
#include "Jovial/SavingLoading/Jon.h"
#include "Jovial/Std/Os.h"

#define JOVIAL_TILEMAP_IMPLEMENTATION
#include "JovialTileMap.h"
#include "TileMapEditor.h"

#include "../assets.h"

using namespace jovial;

#define WINDOW_NAME "Jovial Tiles"
#define WINDOW_SIZE Vector2(1280, 720)
#define WINDOW_RES (TILE_SIZE * 20.0f)

fs::Path TEXTURE_PATH;
Vector2 TILE_SIZE{16, 16};
TileMapEditor::TileMapMode MODE;

class CameraControl : public Node {
public:
    CameraControl() {
        camera.set_current();
    }

    Camera2D camera;
    Texture tileset_texture;
    Font font;

private:
    void birth() override {
        // editor.mode = MODE;

        print("Path: ", TEXTURE_PATH.str);
        Image image(TEXTURE_PATH);
        tileset_texture = Texture(image, Texture::Nearest);

        font = Font(Minecraft_ttf_h, Minecraft_ttf_h_len, 16.0);

        JV_CORE_ASSERT(tileset_texture.id, "Invalid tileset texture!")
        rendering::set_aspect_mode(rendering::AspectModes::Expand);
        // editor.init(tileset_texture, &font, TILE_SIZE);

        tilemap.texture = tileset_texture;
        tilemap.tile_size = {16, 16};
        tilemap.using_vsize = false;

        tilemap.load_tiles();
        tilemap.place_auto({0, 0});
        tilemap.place({10, 10}, {1, 0});
        fs::Path path = fs::Path("./src/tilemap.rules");
        print("Path: ", path.str);
        String rule_file = fs::read_entire_file(path);

        tilemap.parse(rule_file.view());
    }

    void update() override {
        // if (Input::is_just_pressed(Actions::L)) {
        //     String str = fs::read_entire_file(fs::Path::res() + "test.jon");
        //     jon::Lexer token_stream(str.view());
        //     jon::Parser parser(token_stream);
        //
        //     jon::JonNode node{};
        // if (parser.nodes->get_if_contains(C_STR_VIEW("tilemap"), node)) {
        //     editor.tile_map->clear();
        //     // editor.tile_map.load_wang_from_jon(node);
        // } else {
        //     JV_ERROR("Could not load jon!");
        // }
        // }

        // editor.update();
        //
        // if (Input::is_just_pressed(Actions::C)) {
        //     editor.tile_map->clear();
        // }
        if (Input::is_pressed(Actions::LeftMouseButton)) {
            tilemap.place_auto(tilemap.world_to_coord(Input::get_mouse_position()));
        }

        if (Input::is_pressed(Actions::MiddleMouseButton)) {
            auto dir = Input::get_mouse_delta() / camera.zoom * 0.2f;
            camera.transform.position -= math::POW(dir, 2) * dir.sign();
        }

        if (Input::get_scroll() != 0.0f) {
            camera.zoom = math::CLAMP(camera.zoom + Input::get_scroll() / 10, 0.1f, 5.0f);
        }

        tilemap.visable = true;
        tilemap.draw();
    }

    void death() override {
        destroy_font(&font);
        rendering::unload_texture(tileset_texture);
    }

    RuleTileMap tilemap;
    // TileMapEditor editor;
};

class Game : public Jovial {
public:
    Game() {
        push_plugin(new Window({WINDOW_NAME, WINDOW_SIZE, WINDOW_RES}));
        push_plugins(&plugins::default_plugins_2d);

        // push_layer(new TileMapEditor);
        push_plugin(new NodePlugin(new CameraControl));
    }
};

int main(int argc, char **argv) {
    JV_ASSERT(argc >= 2, "Expected usage: jovial_tiles <your_tilemap.png> [-size x y] [-wang] [-blob]");

    TEXTURE_PATH = os::cwd();
    TEXTURE_PATH += String(argv[1]);

    for (long i = 2; i < argc; ++i) {
        String arg = String(argv[i]);
        if (arg == "-size") {
            JV_ASSERT(argc >= i + 2);
            TILE_SIZE.x = (float) String(argv[i + 1]).to_float();
            TILE_SIZE.y = (float) String(argv[i + 2]).to_float();
        }
        if (arg == "-wang") {
            MODE = TileMapEditor::TileMapMode::Wang;
        }
        if (arg == "-blob") {
            MODE = TileMapEditor::TileMapMode::Blob;
        }
    }

    Game game;
    game.run();
}
