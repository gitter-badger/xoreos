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

/** @file graphics/aurora/guiquad.cpp
 *  A textured quad for a GUI element.
 */

#include "common/util.h"
#include "common/ustring.h"

#include "graphics/graphics.h"

#include "graphics/aurora/guiquad.h"
#include "graphics/aurora/texture.h"

namespace Graphics {

namespace Aurora {

GUIQuad::GUIQuad(const Common::UString &texture,
	               const glm::vec2 &p1, const glm::vec2 &p2,
	               const glm::vec2 &t1, const glm::vec2 &t2) :
	_r(1.0), _g(1.0), _b(1.0), _a(1.0),
	_p1(p1), _p2(p2), _t1(t1), _t2(t2),
	_xor(false) {

	try {

		if (!texture.empty())
			_texture = TextureMan.get(texture);

	} catch (...) {
		_texture.clear();

		_r = _g = _b = _a = 0.0;
	}

	_distance = -FLT_MAX;
}

GUIQuad::~GUIQuad() {
	hide();
}

glm::vec3 GUIQuad::getPosition() const {
	return glm::vec3(MIN(_p1.x, _p2.x), MIN(_p1.y, _p2.y), _distance);
}

void GUIQuad::setPosition(const glm::vec3 &position) {
	GfxMan.lockFrame();

	_p2 += glm::vec2(position) - _p1;
	_p1  = glm::vec2(position);

	_distance = position.z;
	resort();

	GfxMan.unlockFrame();
}


void GUIQuad::getColor(float& r, float& g, float& b, float& a) const {
	r = _r;
	g = _g;
	b = _b;
	a = _a;
}


void GUIQuad::setColor(float r, float g, float b, float a) {
	GfxMan.lockFrame();

	_r = r;
	_g = g;
	_b = b;
	_a = a;

	GfxMan.unlockFrame();
}

void GUIQuad::setTexture(const Common::UString &texture) {
	GfxMan.lockFrame();

	try {

		if (texture.empty())
			_texture.clear();
		else
			_texture = TextureMan.get(texture);

	} catch (...) {
		_texture.clear();

		_r = _g = _b = _a = 0.0;
	}

	GfxMan.unlockFrame();
}

glm::vec2 GUIQuad::getSize() const {
	return glm::abs(_p2 - _p1);
}

void GUIQuad::setSize(const glm::vec2 &size) {
	GfxMan.lockFrame();

	_p2 = _p1 + size;

	GfxMan.unlockFrame();
}

void GUIQuad::setXOR(bool enabled) {
	GfxMan.lockFrame();

	_xor = enabled;

	GfxMan.unlockFrame();
}

bool GUIQuad::isIn(const glm::vec2 &point) const {
	return Common::insideOf(point, _p1, _p2);
}

void GUIQuad::calculateDistance() {
}

void GUIQuad::render(RenderPass pass) {
	bool isTransparent = (_a < 1.0) || (!_texture.empty() && _texture.getTexture().hasAlpha());
	if (((pass == kRenderPassOpaque)      &&  isTransparent) ||
			((pass == kRenderPassTransparent) && !isTransparent))
		return;

	TextureMan.set(_texture);

	glColor4f(_r, _g, _b, _a);

	if (_xor) {
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(GL_XOR);
	}

	glBegin(GL_QUADS);
		glTexCoord2f(_t1.x, _t1.y);
		glVertex2f(_p1.x, _p1.y);
		glTexCoord2f(_t2.x, _t1.y);
		glVertex2f(_p2.x, _p1.y);
		glTexCoord2f(_t2.x, _t2.y);
		glVertex2f(_p2.x, _p2.y);
		glTexCoord2f(_t1.x, _t2.y);
		glVertex2f(_p1.x, _p2.y);
	glEnd();

	if (_xor)
		glDisable(GL_COLOR_LOGIC_OP);

	glColor4f(1.0, 1.0, 1.0, 1.0);
}

} // End of namespace Aurora

} // End of namespace Graphics
