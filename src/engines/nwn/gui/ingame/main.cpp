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

/** @file engines/nwn/gui/ingame/main.cpp
 *  The NWN ingame main menu.
 */

#include "common/version.h"

#include "aurora/talkman.h"

#include "engines/nwn/gui/widgets/label.h"

#include "engines/nwn/gui/dialogs/yesnocancel.h"
#include "engines/nwn/gui/dialogs/okcancel.h"

#include "engines/nwn/gui/options/game.h"
#include "engines/nwn/gui/options/video.h"
#include "engines/nwn/gui/options/sound.h"
#include "engines/nwn/gui/options/controls.h"

#include "engines/nwn/gui/ingame/main.h"

namespace Engines {

namespace NWN {

IngameMainMenu::IngameMainMenu() {
	load("options_main");

	// TODO: Load game
	getWidget("LoadButton", true)->setDisabled(true);

	// TODO: Save game
	getWidget("SaveButton", true)->setDisabled(true);

	// TODO: Save character
	getWidget("SaveCharButton", true)->setDisabled(true);

	_game     = new OptionsGameMenu(false);
	_video    = new OptionsVideoMenu(false);
	_sound    = new OptionsSoundMenu(false);
	_controls = new OptionsControlsMenu(false);

	_quitPrompt = new OKCancelDialog(TalkMan.getString(10308),
	                                 TalkMan.getString(8274), TalkMan.getString(8275));
}

IngameMainMenu::~IngameMainMenu() {
	delete _quitPrompt;

	delete _controls;
	delete _sound;
	delete _video;
	delete _game;
}

void IngameMainMenu::initWidget(Widget &widget) {
	if (widget.getTag() == "BuildNumber") {
		dynamic_cast<WidgetLabel &>(widget).setText(XOREOS_NAMEVERSION);
		dynamic_cast<WidgetLabel &>(widget).setColor(0.6, 0.6, 0.6, 1.0);
		return;
	}

	if (widget.getTag() == "SaveCharButton#Caption") {
		dynamic_cast<WidgetLabel &>(widget).setText(TalkMan.getString(6560));
		return;
	}
}

void IngameMainMenu::callbackActive(Widget &widget) {
	if ((widget.getTag() == "ResumeGameButton") ||
	    (widget.getTag() == "XButton")) {
		_returnCode = 1;
		return;
	}

	if (widget.getTag() == "ExitButton") {
		if (sub(*_quitPrompt, 0, false) == 1) {
			_returnCode = 2;
			return;
		}

		show();
		return;
	}

	if (widget.getTag() == "GameButton") {
		sub(*_game);
		return;
	}

	if (widget.getTag() == "VideoOptionsButton") {
		sub(*_video);
		return;
	}

	if (widget.getTag() == "SoundOptionsButton") {
		sub(*_sound);
		return;
	}

	if (widget.getTag() == "ControlsButton") {
		sub(*_controls);
		return;
	}

}

} // End of namespace NWN

} // End of namespace Engines
