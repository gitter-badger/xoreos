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

/** @file common/boundingbox.cpp
 *  A bounding box.
 */

#include <cstdio>

#include "common/boundingbox.h"
#include "common/util.h"
#include "common/maths.h"

namespace Common {

static const glm::vec3 coordSigns[] = {
	glm::vec3(+1, +1, +1),
	glm::vec3(+1, +1, -1),
	glm::vec3(+1, -1, +1),
	glm::vec3(+1, -1, -1),
	glm::vec3(-1, +1, +1),
	glm::vec3(-1, +1, -1),
	glm::vec3(-1, -1, +1),
	glm::vec3(-1, -1, -1),
};

BoundingBox::BoundingBox() {
	clear();
}

BoundingBox::~BoundingBox() {
}

void BoundingBox::clear() {
	_empty    = true;
	_absolute = true;

	// Set to boundaries
	for (int i = 0; i < 8; ++i)
		_coords[i] = coordSigns[i] * FLT_MAX;

	_min = glm::vec3();
	_max = glm::vec3();

	_origin = glm::mat4();
}

bool BoundingBox::isEmpty() const {
	return _empty;
}

const glm::mat4 &BoundingBox::getOrigin() const {
	return _origin;
}

glm::vec3 BoundingBox::getMin() const {
	// Minimum, relative to the origin

	if (_absolute) {
		return _min;
	}

	const glm::vec4 min = _origin * glm::vec4(_min, 1.0);
	const glm::vec4 max = _origin * glm::vec4(_max, 1.0);

	return glm::min(min.xyz() / min.w, max.xyz() / max.w);
}

glm::vec3 BoundingBox::getMax() const {
	// Maximum, relative to the origin

	if (_absolute) {
		return _max;
	}

	const glm::vec4 min = _origin * glm::vec4(_min, 1.0);
	const glm::vec4 max = _origin * glm::vec4(_max, 1.0);

	return glm::max(min.xyz() / min.w, max.xyz() / max.w);
}

glm::vec3 BoundingBox::getSize() const {
	return glm::abs(_max - _min);
}

bool BoundingBox::isIn(const glm::vec2 &point) const {
	if (_empty)
		return false;

	const glm::vec2 min = getMin().xy();
	const glm::vec2 max = getMax().xy();

	return insideOf(point, min, max);
}

bool BoundingBox::isIn(const glm::vec3 &point) const {
	if (_empty)
		return false;

	return insideOf(point, getMin(), getMax());
}

bool BoundingBox::getIntersection(float fDst1, float fDst2,
	                                const std::pair<glm::vec3, glm::vec3> &line,
	                                glm::vec3 &point) const {

	if ((fDst1 * fDst2) >= 0.0f)
		return false;
	if (fDst1 == fDst2)
		return false;

	point = line.first + ((line.second - line.first) * (-fDst1 / (fDst2 - fDst1)));

	return true;
}

bool BoundingBox::inBox(const glm::vec3 &point, const glm::vec3 &min,
	                      const glm::vec3 &max, int axis) const {

	if (((axis == 1) && insideOf(point.yz(), min.yz(), max.yz())) ||
	    ((axis == 2) && insideOf(point.xz(), min.xz(), max.xz())) ||
	    ((axis == 3) && insideOf(point.xy(), min.xy(), max.xy())))
		return true;

	return false;
}

bool BoundingBox::isIn(const std::pair<glm::vec3,glm::vec3> &line) const {
	if (_empty)
		return false;

	const glm::vec3 min = getMin();
	const glm::vec3 max = getMax();

	// if, on at least one axis, both line's ends are less than min
	if (glm::any(glm::lessThan(line.first, min) &
	    glm::lessThan(line.second, min)))
		return false;

	// if, on at least one axis, both line's ends are greater than max
	if (glm::any(glm::greaterThan(line.first, max) &
	    glm::greaterThan(line.second, max)))
		return false;

	// if at least one of the line's ends are inside the box
	if (insideOf(line.first, min, max) ||
	    insideOf(line.second, min, max))
		return true;

	glm::vec3 point;
	if (getIntersection(line.first.x - min.x, line.second.x - min.x, line, point) &&
	    inBox(point, min, max, 1))
		return true;
	if (getIntersection(line.first.y - min.y, line.second.y - min.y, line, point) &&
	    inBox(point, min, max, 2))
		return true;
	if (getIntersection(line.first.z - min.z, line.second.z - min.z, line, point) &&
	    inBox(point, min, max, 3))
		return true;
	if (getIntersection(line.first.x - max.x, line.second.x - max.x, line, point) &&
	    inBox(point, min, max, 1))
		return true;
	if (getIntersection(line.first.y - max.y, line.second.y - max.y, line, point) &&
	    inBox(point, min, max, 2))
		return true;
	if (getIntersection(line.first.z - max.z, line.second.z - max.z, line, point) &&
	    inBox(point, min, max, 3))
		return true;

return false;
}

void BoundingBox::add(const glm::vec3 &point) {
	glm::vec3 min = _empty ? _coords[0] : _min;
	glm::vec3 max = _empty ? _coords[7] : _max;

	for (int i = 0; i < 8; ++i) {
		_coords[i] = coordSigns[i] * glm::min(coordSigns[i] * _coords[i], coordSigns[i] * point);
		min = glm::min(min, _coords[i]);
		max = glm::max(max, _coords[i]);
	}

	_min = min;
	_max = max;
	_empty = false;
}

void BoundingBox::add(const BoundingBox &box) {
	if (box._empty)
		// Don't add an empty bounding box :P
		return;

	for (int i = 0; i < 8; i++)
		add(box._coords[i]);
}

void BoundingBox::translate(const glm::vec3 &amount) {
	_origin = glm::translate(_origin, amount);
}

void BoundingBox::scale(const glm::vec3 &amount) {
	_origin = glm::scale(_origin, amount);
	_absolute = false;
}

void BoundingBox::rotate(float angle, const glm::vec3 &point) {
	_origin = glm::rotate(_origin, angle, point);
	_absolute = false;
}

void BoundingBox::transform(const glm::mat4 &m) {
	_origin *= m;
	_absolute = false;
}

void BoundingBox::absolutize() {
	if (_empty)
		// Nothing to do
		return;

	glm::vec3 coords[8];
	for (int i = 0; i < 8; i++) {
		const glm::vec4 coord = _origin * glm::vec4(_coords[i], 1.0);
		coords[i] = coord.xyz() / coord.w;
	}

	clear();

	for (int i = 0; i < 8; i++)
		add(coords[i]);

	_absolute = true;
}

BoundingBox BoundingBox::getAbsolute() const {
	BoundingBox box = *this;

	box.absolutize();

	return box;
}

} // End of namespace Common
