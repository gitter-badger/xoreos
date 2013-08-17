/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names can be
 * found in the AUTHORS file distributed with this source
 * distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * The Infinity, Aurora, Odyssey, Eclipse and Lycium engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

#include "graphics/aurora/highlightableguiquad.h"

namespace Graphics {

namespace Aurora {

HighlightableGUIQuad::HighlightableGUIQuad(const Common::UString &texture,
	        const glm::vec2 &p1, const glm::vec2 &p2,
	        const glm::vec2 &t1, const glm::vec2 &t2) :
	GUIQuad (texture, p1, p2, t1, t2) {
}

HighlightableGUIQuad::~HighlightableGUIQuad() {
}

void HighlightableGUIQuad::render(RenderPass pass) {
	if (isHighlightable() && isHightlighted()) {
		float initialR, initialG, initialB, initialA, r, g, b, a;
		getColor(initialR, initialG, initialB, initialA);
		incrementColor(initialR, initialG, initialB, initialA, r, g, b, a);
		setColor(r, g, b, a);
	}
	Graphics::Aurora::GUIQuad::render(pass);
}

} // End of namespace Aurora

} // End of namespace Graphics
