#include "Jovial/Components/2D/Camera2D.h"
#include "Jovial/Components/2D/DefaultPlugins2D.h"
#include "Jovial/JovialEngine.h"
#include "Jovial/Renderer/TextRenderer.h"
#include "Jovial/Shapes/Color.h"
#include "Jovial/Shapes/Rect.h"
#include "TileMap.h"
#include "TileMapEditor.h"

using namespace jovial;

#define WINDOW_NAME "Jovial Tiles"
#define WINDOW_SIZE Vector2(1280, 720)
#define WINDOW_RES (WINDOW_SIZE / 2.0f)

static std::vector<TextureBank::NamedTexture> textures_to_load() {
    return {
            {"test", new Texture(fs::Path::assets() + "assets.png")},
    };
}

static std::vector<FontBank::NamedFont> fonts_to_load() {
    return {
            {"normal", new Font(fs::Path::fonts() + "Minecraft.ttf", 16, Texture::Nearest)},
    };
}


class CameraControl : public NodeLayer {
public:
    CameraControl() : camera() {
        camera.set_current();
        add_child(camera);
    }

    Camera2D camera;

private:
    void update() override {
        if (Input::is_pressed(Actions::MiddleMouseButton)) {
            camera.transform.position -= Input::get_mouse_delta() / camera.get_zoom();
        }

        if (Input::get_scroll() != 0.0f) {
            camera.set_zoom(std::clamp(camera.get_zoom() + Input::get_scroll() / 10, 0.1f, 5.0f));
        }
    }
};

class Game : public Jovial {
public:
    Game() {
        push_plugin(new TextureBank(&textures_to_load));
        push_plugin(new FontBank(&fonts_to_load));
        push_plugin(Window::create({WINDOW_NAME, WINDOW_SIZE, WINDOW_RES}));
        push_plugins(&plugins::default_plugins_2d);

        push_layer(new TileMapEditor);
        push_layer(new CameraControl);
    }
};

Jovial *jovial_main() {
    auto *game = new Game();
    return game;
}
