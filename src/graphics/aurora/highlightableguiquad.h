/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HIGHLIGHTABLE_GUI_QUAD_H
#define HIGHLIGHTABLE_GUI_QUAD_H

#include "graphics/aurora/guiquad.h"
#include "graphics/aurora/highlightable.h"

namespace Graphics {

namespace Aurora {

class HighlightableGUIQuad: public GUIQuad, public Highlightable {

public:
	HighlightableGUIQuad(const Common::UString &texture,
	        float  x1      , float  y1      , float  x2      , float  y2,
	        float tX1 = 0.0, float tY1 = 0.0, float tX2 = 1.0, float tY2 = 1.0);
	~HighlightableGUIQuad();

	void render (RenderPass pass);
};

} // End of namespace Aurora

} // End of namespace Graphics

#endif // HIGHLIGHTABLE_GUI_QUAD_H
