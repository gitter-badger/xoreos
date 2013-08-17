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

/** @file engines/nwn/gui/options/resolution.cpp
 *  The NWN resolution options menu.
 */

#include "common/configman.h"

#include "graphics/graphics.h"

#include "engines/nwn/gui/widgets/panel.h"
#include "engines/nwn/gui/widgets/listbox.h"

#include "engines/nwn/gui/options/resolution.h"

namespace Engines {

namespace NWN {

OptionsResolutionMenu::OptionsResolutionMenu(bool isMain) {
	load("options_vidmodes");

	if (isMain) {
		WidgetPanel *backdrop = new WidgetPanel(*this, "PNL_MAINMENU", "pnl_mainmenu");
		backdrop->setPosition(glm::vec3(0.0, 0.0, 100.0));
		addWidget(backdrop);
	}

	initResolutions();
}

OptionsResolutionMenu::~OptionsResolutionMenu() {
}

void OptionsResolutionMenu::show() {
	initResolutionsBox(*getListBox("VideoModeList", true));

	_size = GfxMan.getScreenSize();

	GUI::show();
}

void OptionsResolutionMenu::fixWidgetType(const Common::UString &tag, WidgetType &type) {
	if (tag == "VideoModeList")
		type = kWidgetTypeListBox;
}

void OptionsResolutionMenu::initWidget(Widget &widget) {
	if (widget.getTag() == "VideoModeList") {
		dynamic_cast<WidgetListBox &>(widget).setMode(WidgetListBox::kModeSelectable);
		return;
	}
}

void OptionsResolutionMenu::callbackActive(Widget &widget) {
	if ((widget.getTag() == "CancelButton") ||
	    (widget.getTag() == "XButton")) {

		revertChanges();
		_returnCode = 1;
		return;
	}

	if (widget.getTag() == "OkButton") {

		setResolution(getListBox("VideoModeList", true)->getSelected());
		adoptChanges();
		_returnCode = 2;
		return;
	}

	if (widget.getTag() == "ApplyButton") {
		setResolution(getListBox("VideoModeList", true)->getSelected());
		return;
	}
}

void OptionsResolutionMenu::initResolutions() {
	// Add all standard resolutions to the list
	_resolutions.reserve(33);
	_resolutions.push_back(glm::ivec2(7680, 4800));
	_resolutions.push_back(glm::ivec2(7680, 4320));
	_resolutions.push_back(glm::ivec2(6400, 4800));
	_resolutions.push_back(glm::ivec2(6400, 4096));
	_resolutions.push_back(glm::ivec2(5120, 4096));
	_resolutions.push_back(glm::ivec2(5120, 3200));
	_resolutions.push_back(glm::ivec2(4096, 3072));
	_resolutions.push_back(glm::ivec2(4096, 1716));
	_resolutions.push_back(glm::ivec2(3840, 2400));
	_resolutions.push_back(glm::ivec2(3200, 2400));
	_resolutions.push_back(glm::ivec2(3200, 2048));
	_resolutions.push_back(glm::ivec2(2560, 2048));
	_resolutions.push_back(glm::ivec2(2560, 1600));
	_resolutions.push_back(glm::ivec2(2560, 1440));
	_resolutions.push_back(glm::ivec2(2048, 1536));
	_resolutions.push_back(glm::ivec2(2048, 1152));
	_resolutions.push_back(glm::ivec2(2048, 1080));
	_resolutions.push_back(glm::ivec2(1920, 1200));
	_resolutions.push_back(glm::ivec2(1920, 1080));
	_resolutions.push_back(glm::ivec2(1680, 1050));
	_resolutions.push_back(glm::ivec2(1600, 1200));
	_resolutions.push_back(glm::ivec2(1600,  900));
	_resolutions.push_back(glm::ivec2(1440,  900));
	_resolutions.push_back(glm::ivec2(1400, 1050));
	_resolutions.push_back(glm::ivec2(1280, 1024));
	_resolutions.push_back(glm::ivec2(1280,  800));
	_resolutions.push_back(glm::ivec2(1280,  720));
	_resolutions.push_back(glm::ivec2(1152,  864));
	_resolutions.push_back(glm::ivec2(1024,  768));
	_resolutions.push_back(glm::ivec2(800 ,  600));
	_resolutions.push_back(glm::ivec2(640 ,  480));
	_resolutions.push_back(glm::ivec2(320 ,  240));
	_resolutions.push_back(glm::ivec2(320 ,  200));
}

void OptionsResolutionMenu::initResolutionsBox(WidgetListBox &resList) {
	_useableResolutions.clear();

	const glm::ivec2 maxSize = GfxMan.getSystemSize();
	const glm::ivec2 curSize = GfxMan.getScreenSize();

	// Find the max allowed resolution in the list
	uint maxRes = 0;
	for (uint i = 0; i < _resolutions.size(); i++) {
		if (glm::all(glm::lessThanEqual(_resolutions[i], maxSize))) {
			maxRes = i;
			break;
		}
	}

	// Find the current resolution in the list
	uint currentResolution = 0xFFFFFFFF;
	for (uint i = maxRes; i < _resolutions.size(); i++) {
		if (glm::all(glm::equal(_resolutions[i], curSize))) {
			currentResolution = i - maxRes;
			break;
		}
	}

	// Doesn't exist, add it at the top
	if (currentResolution == 0xFFFFFFFF) {
		currentResolution = 0;
		_useableResolutions.push_back(curSize);
	}

	// Put the rest of the useable resolutions into the list
	for (uint i = maxRes; i < _resolutions.size(); i++)
		_useableResolutions.push_back(_resolutions[i]);


	resList.lock();

	resList.clear();
	for (std::vector<glm::ivec2>::const_iterator r = _useableResolutions.begin(); r != _useableResolutions.end(); ++r)
		resList.add(new WidgetListItemTextLine(*this, "fnt_dialog16x16",
					Common::UString::sprintf("%dx%d", r->x, r->y), 0.0));

	resList.unlock();

	resList.select(currentResolution);
}

void OptionsResolutionMenu::setResolution(uint n) {
	if (n >= _useableResolutions.size())
		return;

	GfxMan.setScreenSize(_useableResolutions[n]);
}

void OptionsResolutionMenu::adoptChanges() {
	const glm::ivec2 curSize = GfxMan.getScreenSize();
	ConfigMan.setInt("width" , curSize.x, true);
	ConfigMan.setInt("height", curSize.y, true);
}

void OptionsResolutionMenu::revertChanges() {
	GfxMan.setScreenSize(_size);
}

} // End of namespace NWN

} // End of namespace Engines
