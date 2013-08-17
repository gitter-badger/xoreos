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

/** @file engines/nwn/gui/widgets/quadwidget.cpp
 *  A NWN quad widget.
 */

#include "common/util.h"
#include "common/ustring.h"

#include "graphics/aurora/guiquad.h"

#include "engines/nwn/gui/widgets/quadwidget.h"

namespace Engines {

namespace NWN {

QuadWidget::QuadWidget(::Engines::GUI &gui, const Common::UString &tag,
                       const Common::UString &texture,
	                     const glm::vec2 &p1, const glm::vec2 &p2,
	                     const glm::vec2 &t1, const glm::vec2 &t2) :
	NWNWidget(gui, tag) {

	_quad = new Graphics::Aurora::GUIQuad(texture, p1, p2, t1, t2);
	_quad->setTag(tag);
	_quad->setClickable(true);

	_size = glm::abs(p2 - p1);
}

QuadWidget::~QuadWidget() {
	delete _quad;
}

void QuadWidget::show() {
	_quad->show();
}

void QuadWidget::hide() {
	_quad->hide();
}

void QuadWidget::setPosition(const glm::vec3 &position) {
	NWNWidget::setPosition(position);

	const glm::vec3 p = getPosition();
	_quad->setPosition(p);
}

void QuadWidget::setColor(float r, float g, float b, float a) {
	_quad->setColor(r, g, b, a);
}

void QuadWidget::setTexture(const Common::UString &texture) {
	_quad->setTexture(texture);
}

void QuadWidget::setSize(const glm::vec2 &size) {
	_quad->setSize(size);
}

glm::vec2 QuadWidget::getSize() const {
	return _size;
}

} // End of namespace NWN

} // End of namespace Engines
