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

/** @file engines/nwn/gui/ingame/quickchat.cpp
 *  The NWN ingame quickchat.
 */

#include "aurora/talkman.h"

#include "graphics/graphics.h"

#include "graphics/aurora/modelnode.h"
#include "graphics/aurora/model.h"

#include "engines/nwn/gui/widgets/panel.h"
#include "engines/nwn/gui/widgets/label.h"

#include "engines/nwn/gui/ingame/quickchat.h"

namespace Engines {

namespace NWN {

ChatModeButton::ChatModeButton(::Engines::GUI &gui, const Common::UString &tag,
                               const Common::UString &model, ChatMode mode) :
	WidgetButton(gui, tag, model) {

	_label = new WidgetLabel(*_gui, getTag() + "#Label", "fnt_dialog16x16", "");
	_label->setColor(1.0, 1.0, 1.0, 0.6);

	setMode(mode);

	setPosition(glm::vec3(0.0, 0.0, 0.0));

	addSub(*_label);
}

ChatModeButton::~ChatModeButton() {
}

void ChatModeButton::show() {
	_label->show();

	WidgetButton::show();
}

void ChatModeButton::hide() {
	_label->hide();

	WidgetButton::hide();
}

void ChatModeButton::setMode(ChatMode mode) {
	_mode = mode;

	_label->setText(TalkMan.getString(66751 + (int) _mode) + ":");
}

void ChatModeButton::setPosition(const glm::vec3 &position) {
	WidgetButton::setPosition(position);

	const glm::vec3 p = getPosition();

	Graphics::Aurora::ModelNode *node = 0;

	glm::vec3 t = glm::vec3(0.0, 0.0, 0.0);
	if ((node = _model->getNode("text")))
		t = node->getPosition();

	_label->setPosition(p + glm::vec3(t.x, t.y - (_label->getSize().y / 2.0), -t.z));
}

void ChatModeButton::setTag(const Common::UString &tag) {
	WidgetButton::setTag(tag);

	_label->setTag(getTag() + "#Label");
}


Quickchat::Quickchat(float position) {
	// Prompt

	_prompt = new WidgetPanel(*this, "QCPrompt", "pnl_chat_prompt");

	_prompt->setPosition(glm::vec3(0.0, position, 0.0));

	addWidget(_prompt);


	// Mode button

	ChatModeButton *modeButton =
		new ChatModeButton(*this, "QCMode", "ctl_btn_chatmode", kModeTalk);

	modeButton->setPosition(glm::vec3(0.0, position, -10.0));

	addWidget(modeButton);

	notifyResized(glm::ivec2(0, 0), GfxMan.getScreenSize());
}

Quickchat::~Quickchat() {
}

glm::vec2 Quickchat::getSize() const {
	return _prompt->getSize();
}

void Quickchat::callbackActive(Widget &widget) {
}

void Quickchat::notifyResized(const glm::ivec2 &oldSize, const glm::ivec2 &newSize) {
	setPosition(-glm::vec3(newSize, 20) / 2.0f);
}

} // End of namespace NWN

} // End of namespace Engines
