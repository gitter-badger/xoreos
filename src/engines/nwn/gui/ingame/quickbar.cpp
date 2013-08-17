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

/** @file engines/nwn/gui/ingame/quickbar.cpp
 *  The ingame quickbar.
 */

#include "common/error.h"

#include "graphics/graphics.h"

#include "graphics/aurora/modelnode.h"
#include "graphics/aurora/model.h"

#include "engines/aurora/model.h"

#include "engines/nwn/gui/widgets/panel.h"

#include "engines/nwn/gui/ingame/quickbar.h"

namespace Engines {

namespace NWN {

QuickbarButton::QuickbarButton(::Engines::GUI &gui, uint n) : NWNWidget(gui, ""),
	_buttonNumber(n) {

	Graphics::Aurora::ModelNode *invisible = 0;

	if (_buttonNumber == 11) {

		_model = loadModelGUI("qb_but67end");
		if (!_model)
			throw Common::Exception("Failed to load quickbar model");
		invisible = _model->getNode("Plane72");

	} else {

		_model = loadModelGUI("qb_but67");
		if (!_model)
			throw Common::Exception("Failed to load quickbar model");
		invisible = _model->getNode("Plane52");

	}

	if (invisible)
		invisible->setInvisible(true);

	NWNWidget::setTag(Common::UString::sprintf("Quickbar%d", _buttonNumber));
	_model->setTag(NWNWidget::getTag());

}

QuickbarButton::~QuickbarButton() {
	delete _model;
}

void QuickbarButton::show() {
	_model->show();
}

void QuickbarButton::hide() {
	_model->hide();
}

void QuickbarButton::setPosition(const glm::vec3 &position) {
	NWNWidget::setPosition(position);

	const glm::vec3 p = getPosition();
	_model->setPosition(p);
}

glm::vec2 QuickbarButton::getSize() const {
	return glm::vec2(_model->getSize());
}

void QuickbarButton::setTag(const Common::UString &tag) {
}


Quickbar::Quickbar() {
	getSlotSize();

	WidgetPanel *bottomEdge = new WidgetPanel(*this, "QBBottomEdge", "pnl_quick_bar");
	addWidget(bottomEdge);

	_edgeHeight = bottomEdge->getSize().y;

	for (int i = 0; i < 12; i++) {
		QuickbarButton *button = new QuickbarButton(*this, i);

		button->setPosition(glm::vec3(i * _slotSize.x, bottomEdge->getSize().y, 0.0));
		addWidget(button);
	}

	WidgetPanel *topEdge = new WidgetPanel(*this, "QBTopEdge", "pnl_quick_bar");
	topEdge->setPosition(glm::vec3(0.0, _slotSize.y, 0.0));
	addWidget(topEdge);

	notifyResized(glm::ivec2(0, 0), GfxMan.getScreenSize());
}

Quickbar::~Quickbar() {
}

glm::vec2 Quickbar::getSize() const {
	return glm::vec2(12 * _slotSize.x, _slotSize.y + 2 * _edgeHeight);
}

void Quickbar::callbackActive(Widget &widget) {
}

void Quickbar::getSlotSize() {
	Graphics::Aurora::Model *_model = loadModelGUI("qb_but67");

	_slotSize = glm::floor(glm::vec2(_model->getSize()));

	delete _model;
}

void Quickbar::notifyResized(const glm::ivec2 &oldSize, const glm::ivec2 &newSize) {
	setPosition(glm::vec3(- ((12 * _slotSize.x) / 2.0), - (newSize.y / 2.0), -10.0));
}

} // End of namespace NWN

} // End of namespace Engines
