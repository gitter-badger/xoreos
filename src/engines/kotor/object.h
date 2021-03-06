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

/** @file engines/kotor/object.h
 *  An object within a KotOR area.
 */

#ifndef ENGINES_KOTOR_OBJECT_H
#define ENGINES_KOTOR_OBJECT_H

#include <list>

#include "common/types.h"
#include "common/ustring.h"

namespace Engines {

namespace KotOR {

/** An object within a KotOR area. */
class Object {
public:
	Object();
	virtual ~Object();

	virtual void show() = 0;
	virtual void hide() = 0;

	const Common::UString &getTag() const;
	const Common::UString &getName() const;
	const Common::UString &getDescription() const;

	const Common::UString &getPortrait() const;

	bool isStatic() const;
	bool isUsable() const;

	bool isClickable() const;

	const std::list<uint32> &getIDs() const;

	virtual void getPosition(float &x, float &y, float &z) const;
	virtual void getOrientation(float &x, float &y, float &z) const;

	virtual void setPosition(float x, float y, float z);
	virtual void setOrientation(float x, float y, float z);

	virtual void enter() = 0;
	virtual void leave() = 0;

	virtual void highlight(bool enabled) = 0;

protected:
	bool _loaded;

	Common::UString _tag;
	Common::UString _name;
	Common::UString _description;

	Common::UString _portrait;

	bool _static;
	bool _usable;

	std::list<uint32> _ids;

	float _position[3];
	float _orientation[3];
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTOR_OBJECT_H
