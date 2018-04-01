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

#ifndef TSC_GROUND_HPP
#define TSC_GROUND_HPP
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

namespace TSC {

    class Ground: public sf::Drawable, public sf::Transformable
    {
    public:
        Ground(const std::string& tileset);
    private:
        virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
        void LoadSettingsFile(const std::string& path);

        int m_rows;
        int m_cols;
        sf::VertexArray m_vertices;
        sf::Texture m_tileset;
        std::vector<sf::FloatRect> m_colrects;
    };

}

#endif /* TSC_GROUND_HPP */
