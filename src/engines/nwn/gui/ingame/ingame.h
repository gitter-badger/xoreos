/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010-2011 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file engines/nwn/gui/ingame/ingame.h
 *  The NWN ingame GUI elements.
 */

#ifndef ENGINES_NWN_GUI_INGAME_INGAME_H
#define ENGINES_NWN_GUI_INGAME_INGAME_H

#include <vector>

#include "common/types.h"

#include "events/types.h"

namespace Common {
	class UString;
}

namespace Engines {

namespace NWN {

class Module;

class IngameMainMenu;

class Creature;

class CharacterInfo;
class Quickbar;
class Quickchat;
class Compass;

/** The NWN ingame GUI elements. */
class IngameGUI {
public:
	IngameGUI(Module &module);
	~IngameGUI();

	int showMain(); ///< Show the ingame main menu.

	void show(); ///< Show the ingame GUI elements.
	void hide(); ///< Hide the ingame GUI elements.

	void evaluateEvent(const Events::Event &event);

	/** Set the current area. */
	void setArea(const Common::UString &area);

	/** Update the party member. */
	void updatePartyMember(uint partyMember, const Creature &creature);

	/** Update the compass. */
	void updateCompass();

private:
	IngameMainMenu *_main; ///< The ingame main menu.

	Quickbar  *_quickbar;  ///< The quick bar.
	Quickchat *_quickchat; ///< The quick chat.
	Compass   *_compass;   ///< The compass.

	std::vector<CharacterInfo *> _party; ///< The party member character panels.


	/** Set the party member's portrait. */
	void setPortrait(uint partyMember, const Common::UString &portrait);

	/** Set the party member's name. */
	void setName(uint partyMember, const Common::UString &name);

	/** Set the party member's health. */
	void setHealth(uint partyMember, uint32 current, uint32 max);

	/** Set party member to "healthy" (red health bar). */
	void setHealthy (uint partyMember);
	/** Set party member to "sick" (brown health bar). */
	void setSick    (uint partyMember);
	/** Set party member to "poisoned" (green health bar). */
	void setPoisoned(uint partyMember);
};

} // End of namespace NWN

} // End of namespace Engines

#endif // ENGINES_NWN_GUI_INGAME_INGAME_H
