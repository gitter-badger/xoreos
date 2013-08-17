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

/** @file common/boundingbox.h
 *  A bounding box.
 */

#ifndef COMMON_BOUNDINGBOX_H
#define COMMON_BOUNDINGBOX_H

#include "common/types.h"

namespace Common {

/** A bouding box around 3D points. */
class BoundingBox {
public:
	BoundingBox();
	~BoundingBox();

	void clear();

	bool isEmpty() const;

	const glm::mat4 &getOrigin() const;

	glm::vec3 getMin() const;
	glm::vec3 getMax() const;

	glm::vec3 getSize() const; ///< Get the [width,height,depth] of the bounding box.

	bool isIn(const glm::vec2 &point) const;
	bool isIn(const glm::vec3 &point) const;

	bool isIn(const std::pair<glm::vec3,glm::vec3> &line) const;

	void add(const glm::vec3 &point);
	void add(const BoundingBox &box);

	void translate(const glm::vec3 &amount);
	void scale    (const glm::vec3 &amount);

	void rotate(float angle, const glm::vec3 &point);

	void transform(const glm::mat4 &m);

	/** Apply the origin transformations directly to the coordinates. */
	void absolutize();

	/** Return a copy with the origin transformations directly applied to the coordinates. */
	BoundingBox getAbsolute() const;

private:
	bool _empty;
	bool _absolute;

	glm::mat4 _origin;

	glm::vec3 _coords[8];

	glm::vec3 _min;
	glm::vec3 _max;

	bool getIntersection(float fDst1, float fDst2,
	                     const std::pair<glm::vec3,glm::vec3> &line,
	                     glm::vec3 &point) const;
	bool inBox(const glm::vec3 &point, const glm::vec3 &min,
	           const glm::vec3 &max, int axis) const;
};

} // End of namespace Common

#endif // COMMON_BOUNDINGBOX_H
