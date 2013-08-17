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

/** @file graphics/aurora/cursor.cpp
 *  A cursor as used in the Aurora engines.
 */

#include "common/util.h"
#include "common/error.h"
#include "common/stream.h"

#include "aurora/types.h"
#include "aurora/resman.h"

#include "graphics/images/decoder.h"
#include "graphics/images/txi.h"
#include "graphics/images/tga.h"
#include "graphics/images/dds.h"
#include "graphics/images/winiconimage.h"

#include "graphics/aurora/cursor.h"
#include "graphics/aurora/cursorman.h"
#include "graphics/aurora/texture.h"

namespace Graphics {

namespace Aurora {

Cursor::Cursor(const Common::UString &name, const glm::ivec2 &hotspot) :
	_name(name), _hotspot(hotspot) {

	load();
}

Cursor::~Cursor() {
}

void Cursor::render() {
	TextureMan.activeTexture(0);
	TextureMan.set(_texture);

	const glm::ivec2 cursor = CursorMan.getPosition();

	glTranslatef(cursor.x - _hotspot.x, -cursor.y - _size.y + _hotspot.x, 0.0);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex2f(0.0, 0.0);
		glTexCoord2f(1.0, 0.0);
		glVertex2f(_size.y, 0.0);
		glTexCoord2f(1.0, 1.0);
		glVertex2f(_size.y, _size.x);
		glTexCoord2f(0.0, 1.0);
		glVertex2f(0.0, _size.x);
	glEnd();
}

void Cursor::load() {
	::Aurora::FileType type;

	Common::SeekableReadStream *img = ResMan.getResource(::Aurora::kResourceCursor, _name, &type);
	if (!img)
		throw Common::Exception("No such cursor resource \"%s\"", _name.c_str());

	_hotspot = glm::ivec2(0, 0);

	ImageDecoder *image;
	// Loading the different image formats
	if      (type == ::Aurora::kFileTypeTGA)
		image = new TGA(*img);
	else if (type == ::Aurora::kFileTypeDDS)
		image = new DDS(*img);
	else if (type == ::Aurora::kFileTypeCUR) {
		WinIconImage *cursor = new WinIconImage(*img);

		if (_hotspot.x < 0)
			_hotspot.x = cursor->getHotspotX();
		if (_hotspot.y < 0)
			_hotspot.y = cursor->getHotspotY();

		image = cursor;
	} else {
		delete img;
		throw Common::Exception("Unsupported cursor resource type %d", (int) type);
	}

	delete img;

	_size  = image->getMipMap(0).size;

	TXI txi;
	txi.getFeatures().filter = false;

	try {
		Texture *texture = new Texture(image, &txi);

		image = 0;

		try {
			_texture = TextureMan.add(texture, _name);
		} catch(...) {
			delete texture;
			throw;
		}

	} catch (...) {
		delete image;
		throw;
	}

	_hotspot = glm::clamp(_hotspot, glm::ivec2(0, 0), glm::ivec2(_size) - 1);
}

} // End of namespace Aurora

} // End of namespace Graphics
