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

/** @file graphics/aurora/text.h
 *  A text object.
 */

#ifndef GRAPHICS_AURORA_TEXT_H
#define GRAPHICS_AURORA_TEXT_H

#include "common/ustring.h"
#include "common/maths.h"

#include "graphics/types.h"
#include "graphics/guifrontelement.h"

#include "graphics/aurora/fontman.h"

namespace Graphics {

namespace Aurora {

/** A text object. */
class Text : public GUIFrontElement {
public:
	Text(const FontHandle &font, const Common::UString &str,
	     float r = 1.0, float g = 1.0, float b = 1.0, float a = 1.0, float align = 0.0);
	~Text();

	const Common::UString &get() const;
	void getPosition(float &x, float &y, float &z) const;
	void getColor(float &r, float &g, float &b, float &a) const;

	void set(const Common::UString &str, float maxWidth = 0.0, float maxHeight = 0.0);
	void setPosition(float x, float y, float z = -FLT_MAX);
	void setColor(float r, float g, float b, float a);
	void unsetColor();
	void setAlign(float align);

	bool isEmpty();

	uint getLineCount() const;

	float getWidth()  const;
	float getHeight() const;

	// Renderable
	void calculateDistance();
	void render(RenderPass pass);
	bool isIn(float x, float y) const;

private:
	float _r, _g, _b, _a;
	FontHandle _font;

	float _x;
	float _y;

	uint32 _lineCount;

	float _width;
	float _height;

	float _align;

	Common::UString _str;
	ColorPositions  _colors;


	void parseColors(const Common::UString &str, Common::UString &parsed,
	                 ColorPositions &colors);
};

} // End of namespace Aurora

} // End of namespace Graphics

#endif // GRAPHICS_AURORA_TEXT_H
