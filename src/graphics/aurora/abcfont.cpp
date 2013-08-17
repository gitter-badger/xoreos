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

/** @file graphics/aurora/abcfont.cpp
 *  An ABC/SBM font, as used by Jade Empire.
 */

#include "common/ustring.h"
#include "common/error.h"
#include "common/stream.h"

#include "aurora/resman.h"

#include "graphics/aurora/texture.h"
#include "graphics/aurora/abcfont.h"

namespace Graphics {

namespace Aurora {

ABCFont::ABCFont(const Common::UString &name) : _base(0) {
	_texture = TextureMan.get(name);

	load(name);
}

ABCFont::~ABCFont() {
}

float ABCFont::getHeight() const {
	return 32.0;
}

float ABCFont::getWidth(uint32 c) const {
	const Char &cC = findChar(c);

	return cC.spaceL + cC.width + cC.spaceR;
}

void ABCFont::draw(uint32 c) const {
	TextureMan.set(_texture);

	const Char &cC = findChar(c);

	glTranslatef(cC.spaceL, 0.0, 0.0);

	glBegin(GL_QUADS);
	for (int i = 0; i < 4; i++) {
		glTexCoord2fv(glm::value_ptr(cC.tpos[i]));
		glVertex2fv  (glm::value_ptr(cC.vpos[i]));
	}
	glEnd();

	glTranslatef(cC.width + cC.spaceR, 0.0, 0.0);
}

void ABCFont::load(const Common::UString &name) {
	Common::SeekableReadStream *abc = ResMan.getResource(name, ::Aurora::kFileTypeABC);
	if (!abc)
		throw Common::Exception("No such font \"%s\"", name.c_str());

	// Init the invalid char
	_invalid.position = glm::uvec2(0, 0);
	_invalid.width    = 0;
	_invalid.spaceL   = 0;
	_invalid.spaceR   = 0;

	_invalid.tpos[0] = glm::vec2(0.0, 0.0);
	_invalid.tpos[1] = glm::vec2(0.0, 0.0);
	_invalid.tpos[2] = glm::vec2(0.0, 0.0);
	_invalid.tpos[3] = glm::vec2(0.0, 0.0);
	_invalid.vpos[0] = glm::vec2(0.0, 0.0);
	_invalid.vpos[1] = glm::vec2(0.0, 0.0);
	_invalid.vpos[2] = glm::vec2(0.0, 0.0);
	_invalid.vpos[3] = glm::vec2(0.0, 0.0);

	bool hasInvalid = false;

	try {
		if (abc->size() != 524280)
			throw Common::Exception("Invalid font (%d)", abc->size());

		_base = abc->readByte();

		abc->skip(7); // Probably random garbage

		// Read the ASCII character
		for (int i = 1; i < 128; i++) {
			Char &c = _ascii[i];

			readCharDesc(c, *abc);
			calcCharVertices(c);

			// Points to the "invalid character"
			if (!hasInvalid && (c.position == glm::uvec2(0, 0))) {
				_invalid   = c;
				hasInvalid = true;
			}
		}

		// Read the UTF16 extended characters
		for (int i = 128; i < 65535; i++) {
			Char c;

			readCharDesc(c, *abc);

			// Points to the "invalid character"
			if ((c.position == glm::uvec2(0, 0))) {
				if (!hasInvalid) {
					calcCharVertices(c);
					_invalid   = c;
					hasInvalid = true;
				}

				continue;
			}

			calcCharVertices(c);
			_extended.insert(std::make_pair(Common::UString::fromUTF16((uint16) i), c));
		}

	} catch (...) {
		delete abc;
		throw;
	}

	delete abc;
}

void ABCFont::readCharDesc(Char &c, Common::SeekableReadStream &abc) {
	uint32 offset = abc.readUint32LE();
	byte   plane  = abc.readByte();

	c.spaceL = abc.readByte();
	c.width  = abc.readByte();
	c.spaceR = abc.readByte();

	if (((offset % 1024) != 0) || (plane > 3))
		throw Common::Exception("Invalid char data (%d, %d)", offset, plane);

	c.position = glm::uvec2(plane, (offset / 1024)) * 32u;
}

void ABCFont::calcCharVertices(Char &c) {
	const glm::vec2 size  = glm::vec2(_texture.getTexture().getSize());
	const glm::vec2 vsize = glm::vec2(c.width, 32.0);
	const glm::vec2 tsize = vsize / size;

	const glm::vec2 tpos = glm::vec2(c.position) / size;

	c.tpos[0] = tpos + glm::vec2(0.0, tsize.y);
	c.tpos[1] = tpos + tsize;
	c.tpos[2] = tpos + glm::vec2(tsize.x, 0.0);
	c.tpos[3] = tpos;

	c.vpos[0] = glm::vec2(0.0, 0.0);
	c.vpos[1] = glm::vec2(vsize.x, 0.0);
	c.vpos[2] = vsize;
	c.vpos[3] = glm::vec2(0.0, vsize.y);
}

const ABCFont::Char &ABCFont::findChar(uint32 c) const {
	if (Common::UString::isASCII(c))
		return _ascii[c];

	std::map<uint32, Char>::const_iterator ch = _extended.find(c);
	if (ch == _extended.end())
		return _invalid;

	return ch->second;
}

} // End of namespace Aurora

} // End of namespace Graphics
