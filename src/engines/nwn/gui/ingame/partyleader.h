/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010-2011 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file engines/nwn/gui/ingame/partyleader.h
 *  The NWN ingame party leader panel.
 */

#ifndef ENGINES_NWN_GUI_INGAME_PARTYLEADER_H
#define ENGINES_NWN_GUI_INGAME_PARTYLEADER_H

#include "common/ustring.h"

#include "events/notifyable.h"

#include "engines/nwn/gui/ingame/charinfo.h"

namespace Engines {

namespace NWN {

class Module;

/** The NWN ingame party leader bar. */
class PartyLeader : public CharacterInfo, public Events::Notifyable {
public:
	PartyLeader(Module &module);
	~PartyLeader();

	/** Set the portrait image. */
	void setPortrait(const Common::UString &portrait);

	/** Set the character name. */
	void setName(const Common::UString &name);
	/** Set the area the character is in. */
	void setArea(const Common::UString &area);

	/** Set the health bar color. */
	void setHealthColor(float r, float g, float b, float a);
	/** Set the character health. */
	void setHealth(int32 current, int32 max);

protected:
	void callbackActive(Widget &widget);

private:
	Module *_module;

	QuadWidget *_portrait;
	QuadWidget *_health;

	Common::UString _currentPortrait;

	Common::UString _name;
	Common::UString _area;

	int32 _currentHP;
	int32 _maxHP;


	void updatePortraitTooltip();

	void notifyResized(int oldWidth, int oldHeight, int newWidth, int newHeight);
};

} // End of namespace NWN

} // End of namespace Engines

#endif // ENGINES_NWN_GUI_INGAME_PARTYLEADER_H
