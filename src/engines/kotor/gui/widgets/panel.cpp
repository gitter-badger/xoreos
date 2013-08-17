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

/** @file engines/kotor/gui/widgets/panel.cpp
 *  A KotOR panel widget.
 */

#include "aurora/gfffile.h"

#include "graphics/aurora/guiquad.h"

#include "engines/kotor/gui/widgets/panel.h"

namespace Engines {

namespace KotOR {

WidgetPanel::WidgetPanel(::Engines::GUI &gui, const Common::UString &tag) :
	KotORWidget(gui, tag) {
}

WidgetPanel::WidgetPanel(::Engines::GUI &gui, const Common::UString &tag,
                         const Common::UString &texture,
                         const glm::vec2 &position, const glm::vec2 &size) : KotORWidget(gui, tag) {

	_size = size;

	Widget::setPosition(glm::vec3(position, 0.0));

	_quad = new Graphics::Aurora::GUIQuad(texture, glm::vec2(0.0, 0.0), size);
	_quad->setPosition(glm::vec3(position, 0.0));
}

WidgetPanel::~WidgetPanel() {
}

void WidgetPanel::load(const Aurora::GFFStruct &gff) {
	KotORWidget::load(gff);
}

} // End of namespace KotOR

} // End of namespace Engines
