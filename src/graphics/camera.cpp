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

/** @file graphics/camera.cpp
 *  Camera management.
 */

#include "common/util.h"
#include "common/maths.h"

#include "graphics/camera.h"
#include "graphics/graphics.h"

#include "events/events.h"
#include "events/notifications.h"

DECLARE_SINGLETON(Graphics::CameraManager)

namespace Graphics {

CameraManager::CameraManager() : _lastChanged(0) {
	_position    = glm::vec3(0.0, 0.0, 0.0);
	_orientation = glm::vec3(0.0, 0.0, 0.0);
}

void CameraManager::lock() {
	_mutex.lock();
}

void CameraManager::unlock() {
	_mutex.unlock();
}

glm::vec3 CameraManager::getPosition() const {
	return _position;
}

glm::vec3 CameraManager::getOrientation() const {
	return _orientation;
}

void CameraManager::reset() {
	Common::StackLock cameraLock(_mutex);

	_position    = glm::vec3(0.0, 0.0, 0.0);
	_orientation = glm::vec3(0.0, 0.0, 0.0);

	_lastChanged = EventMan.getTimestamp();

	GfxMan.recalculateObjectDistances();

	NotificationMan.cameraMoved();
}

void CameraManager::setPosition(const glm::vec3 &position) {
	Common::StackLock cameraLock(_mutex);

	_position    = position;

	_lastChanged = EventMan.getTimestamp();

	GfxMan.recalculateObjectDistances();

	NotificationMan.cameraMoved();
}

void CameraManager::setOrientation(const glm::vec3 &orientation) {
	Common::StackLock cameraLock(_mutex);

	_orientation = glm::mod(orientation, 360.0f);

	_lastChanged = EventMan.getTimestamp();

	GfxMan.recalculateObjectDistances();

	NotificationMan.cameraMoved();
}

void CameraManager::setOrientation(const glm::vec2 &orientation) {
	const glm::vec3 o = Common::vector2orientation(orientation);

	setOrientation(glm::vec3(o.x, 360.0 - o.y, o.z));
}

void CameraManager::turn(const glm::vec3 &amount) {
	setOrientation(_orientation + amount);
}

void CameraManager::move(const glm::vec3 &amount) {
	setPosition(_position + amount);
}

void CameraManager::move(float n) {
	float x = n * sin(Common::deg2rad(_orientation[1]));
	float y = n * sin(Common::deg2rad(_orientation[0]));
	float z = n * cos(Common::deg2rad(_orientation[1])) *
	              cos(Common::deg2rad(_orientation[0]));

	move(glm::vec3(x, y, z));
}

void CameraManager::strafe(float n) {
	float x = n * sin(Common::deg2rad(_orientation[1] + 90.0)) *
	              cos(Common::deg2rad(_orientation[2]));
	float y = n * sin(Common::deg2rad(_orientation[2]));
	float z = n * cos(Common::deg2rad(_orientation[1] + 90.0));

	move(glm::vec3(x, y, z));
}

void CameraManager::fly(float n) {
	float x = n * cos(Common::deg2rad(_orientation[2] + 90.0));
	float y = n * sin(Common::deg2rad(_orientation[0] + 90.0)) *
	              sin(Common::deg2rad(_orientation[2] + 90.0));
	float z = n * cos(Common::deg2rad(_orientation[0] + 90.0));

	move(glm::vec3(x, y, z));
}

uint32 CameraManager::lastChanged() const {
	return _lastChanged;
}

} // End of namespace Graphics
