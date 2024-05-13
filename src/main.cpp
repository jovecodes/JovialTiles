#include "Jovial/Components/2D/Camera2D.h"
#include "Jovial/Components/2D/DefaultPlugins2D.h"
#include "Jovial/Components/Util/LazyAssets.h"
#include "Jovial/JovialEngine.h"
#include "Jovial/Renderer/Renderer.h"
#include "Jovial/SavingLoading/Jon.h"

#define JOVIAL_TILEMAP_IMPLEMENTATION
#include "JovialTileMap.h"
#include "TileMapEditor.h"

using namespace jovial;

#define WINDOW_NAME "Jovial Tiles"
#define WINDOW_SIZE Vector2(1280, 720)
#define WINDOW_RES (WINDOW_SIZE / 2.0f)

LazyTexture TestTileset(fs::Path(JV_ASSETS_DIR JV_SEP "test.png"));
LazyFont MinecraftFont(fs::Path(JV_FONTS_DIR JV_SEP "Minecraft.ttf"), 16.0);

class CameraControl : public Node {
public:
    CameraControl() {
        camera.set_current();
    }

    Camera2D camera;

private:
    void birth() override {
        rendering::set_aspect_mode(rendering::AspectModes::KeepViewport);
        editor.init(TestTileset, MinecraftFont.get(), {16, 16});

        // tilemap.texture = TestTileset.get();
        // tilemap.tile_size = {16, 16};
        //
        // tilemap.load_tiles();
        // tilemap.place({0, 0}, {0, 0});
    }

    void update() override {
        if (Input::is_just_pressed(Actions::L)) {
            String str = fs::read_entire_file(fs::Path::res() + "test.jon");
            jon::Lexer token_stream(str.view());
            jon::Parser parser(token_stream);

            jon::JonNode node{};
            if (parser.nodes->get_if_contains(C_STR_VIEW("tilemap"), node)) {
                editor.tile_map.clear();
                editor.tile_map.load_wang_from_jon(node);
            } else {
                JV_ERROR("Could not load jon!");
            }
        }

        if (Input::is_just_pressed(Actions::C)) {
            editor.tile_map.clear();
        }

        if (Input::is_pressed(Actions::MiddleMouseButton)) {
            camera.transform.position -= Input::get_mouse_delta() / camera.zoom * 0.2f;
        }

        if (Input::is_just_pressed(Actions::S)) {
            jon::Generator generator;
            generator.save("tilemap", &editor.tile_map);
            generator.generate(fs::Path::res() + "test.jon");
        }

        // if (Input::get_scroll() != 0.0f) {
        //     camera.zoom = math::CLAMP(camera.zoom + Input::get_scroll() / 10, 0.1f, 5.0f);
        // }

        editor.update();

        // if (Input::is_pressed(Actions::LeftMouseButton)) {
        //     tilemap.place(tilemap.world_to_coord(Input::get_mouse_position()), {0, 0});
        // }
        // if (Input::is_pressed(Actions::RightMouseButton)) {
        //     tilemap.erase(tilemap.world_to_coord(Input::get_mouse_position()));
        // }

        // editor.draw();
    }

    void death() override {
        MinecraftFont.unload();
        TestTileset.unload();
    }

    TileMapEditor editor;
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

int main() {
    Game game;
    game.run();
}
