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

/** @file engines/nwn/gui/main/newgamefog.h
 *  The fog behind the new game dialogs.
 */

#ifndef ENGINES_NWN_GUI_MAIN_NEWGAMEFOG_H
#define ENGINES_NWN_GUI_MAIN_NEWGAMEFOG_H

#include <vector>

#include "common/types.h"

#include "graphics/aurora/types.h"

namespace Engines {

namespace NWN {

class NewGameFogs {
public:
	NewGameFogs(uint count);
	~NewGameFogs();

	void show();
	void hide();

private:
	std::vector<Graphics::Aurora::Model *> _fogs;
};

} // End of namespace NWN

} // End of namespace Engines

#endif // ENGINES_NWN_GUI_MAIN_NEWGAMEFOG_H
