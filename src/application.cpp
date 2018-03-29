/*******************************************************************************
 * This file is part of TSC.
 *
 * TSC is a 2-dimensional platform game.
 * Copyright © 2018 The TSC Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "application.hpp"
#include "pathmap.hpp"
#include "settings.hpp"
#include "scenes/title_scene.hpp"
#include "texture_cache.hpp"
#include "font_store.hpp"
#include "gui.hpp"
#include "util.hpp"
#include "i18n.hpp"
#include <SFML/Graphics.hpp>
#include <xercesc/util/PlatformUtils.hpp>

using namespace TSC;
using namespace std;

// extern declarations
Application* TSC::gp_app = nullptr;

Application::Application(int argc, char* argv[])
    : mp_window(nullptr),
      mp_game_clock(nullptr),
      mp_fps(nullptr),
      mp_fonts(nullptr),
      mp_gui(nullptr),
      mp_gui_font(nullptr),
      m_terminate(false),
      m_render_gui(true),
      m_frame_time(0.0f)
{
    xercesc::XMLPlatformUtils::Initialize();
    Settings::Load();

    mp_fonts = new FontStore();
    mp_game_clock = new sf::Clock();
    mp_fps = new sf::Text();
    mp_fps->setFont(mp_fonts->NormalFont);
    mp_fps->setFillColor(sf::Color::Yellow);
    mp_fps->setCharacterSize(TSC::GUI_FONT_SIZE);
    mp_fps->setPosition(10, 10);
    SetupI18n(Pathmap::GetLocalePath().utf8_str().c_str());
}

Application::~Application()
{
    Settings::Save();

    if (mp_window)
        delete mp_window;
    if (mp_fonts)
        delete mp_fonts;
    if (mp_game_clock)
        delete mp_game_clock;

    xercesc::XMLPlatformUtils::Terminate();
}

int Application::MainLoop()
{
    OpenWindow();
    InitGUI();

    m_scene_stack.push(unique_ptr<TitleScene>(new TitleScene()));

    // Game main loop
    while (!m_terminate && !m_scene_stack.empty()) {
        m_frame_time = mp_game_clock->restart().asSeconds();
        unique_ptr<Scene>& p_scene = m_scene_stack.top();

        // TODO: Hand events to nuklear GUI
        nk_input_begin(mp_gui);
        nk_input_end(mp_gui);

        // Poll events from SFML
        p_scene->ProcessEvents(mp_window);

        // Allow a scene to destroy itself by returning false from Update().
        if (p_scene->Update()) {
            mp_window->clear(sf::Color::Black);

            // Draw scene
            p_scene->Draw(mp_window);

            // Draw GUI on top of it, if enabled
            if (m_render_gui)
                DrawGUI();

            // Draw FPS
            int fps = static_cast<int>(1.0f / m_frame_time);
            mp_fps->setString(sformat(_("FPS: %d"), fps));
            mp_window->draw(*mp_fps);

            // Flip buffers
            mp_window->display();
        }
        else
            m_scene_stack.pop();
    }

    // If the mainloop was ended by m_terminate, end all the scenes
    // that still exist.
    if (!m_scene_stack.empty()) {
        m_scene_stack.pop();
    }

    CleanupGUI();
    mp_window->close();

    return 0;
}

// Advises the programme to terminate the next time the main loop runs.
void Application::Terminate()
{
    // Implementing this function by emptying the scene stack right
    // here is probably not a good idea. If a scene calls it, that
    // would lead to the scene being deleted while a function of the
    // scene is still running.
    m_terminate = true;
}

void Application::OpenWindow()
{
    // Retrieve desired resolution from config. If that resolution is not
    // native to the graphics card, override the config with the best native
    // resolution available.
    sf::VideoMode mode(Settings::screen_width, Settings::screen_height);
    if (!mode.isValid()) {
            mode = sf::VideoMode::getFullscreenModes()[0];
            Settings::screen_width = mode.width;
            Settings::screen_height = mode.height;
    }

    // TRANS: This is the window's title.
    mp_window = new sf::RenderWindow(mode, _("The Secret Chronicles of Dr. M."));

    // Enable vsync if requested
    if (Settings::enable_vsync) {
        mp_window->setVerticalSyncEnabled(true);
    }
}

void Application::InitGUI()
{
    mp_gui                    = new nk_context;
    mp_gui_font               = new nk_user_font;
    mp_gui_font->userdata.ptr = &mp_fonts->NormalFont;
    mp_gui_font->height       = TSC::GUI_FONT_SIZE;
    mp_gui_font->width        = TSC::CalculateGUIFontWidth;
    nk_init_default(mp_gui, mp_gui_font);
}

void Application::CleanupGUI()
{
    nk_free(mp_gui);
    delete mp_gui_font;
    delete mp_gui;
    mp_gui = nullptr;
}

void Application::DrawGUI()
{
    DrawNKGUI(mp_gui, mp_window);
}
