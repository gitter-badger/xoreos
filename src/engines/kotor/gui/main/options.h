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

/** @file engines/kotor/gui/main/options.h
 *  The options menu.
 */

#ifndef OPTIONSMENU_H
#define OPTIONSMENU_H

#include "engines/kotor/gui/gui.h"

#include "graphics/aurora/highlightable.h"

#include "gui/options/gameplay.h"

namespace Engines {

namespace KotOR {

class OptionsMenu : public GUI {
public:
	OptionsMenu();
	~OptionsMenu();

protected:
	void callbackActive(Widget &widget);

private:
	void adoptChanges();

	GUI *_gameplay;
	GUI *_feedback;
	GUI *_autopause;
	GUI *_graphics;
	GUI *_sound;
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // OPTIONSMENU_H
