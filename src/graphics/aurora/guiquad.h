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

/** @file graphics/aurora/guiquad.h
 *  A textured quad for a GUI element.
 */

#ifndef GRAPHICS_AURORA_GUIQUAD_H
#define GRAPHICS_AURORA_GUIQUAD_H

#include "common/maths.h"

#include "graphics/guifrontelement.h"

#include "graphics/aurora/textureman.h"

namespace Common {
	class UString;
}

namespace Graphics {

namespace Aurora {

class GUIQuad : public GUIFrontElement {
public:
	GUIQuad(const Common::UString &texture,
	        const glm::vec2 &p1, const glm::vec2 &p2,
	        const glm::vec2 &t1 = glm::vec2(0.0, 0.0),
	        const glm::vec2 &t2 = glm::vec2(1.0, 1.0));
	~GUIQuad();

	/** Get the current position of the quad. */
	glm::vec3 getPosition() const;
	/** Set the current position of the quad. */
	void setPosition(const glm::vec3 &position);

	/** Get the current color of the quad */
	void getColor(float &r, float &g, float &b, float &a) const;
	/** Set the current color of the quad. */
	void setColor(float r, float g, float b, float a);
	/** Set the current texture of the quad. */
	void setTexture(const Common::UString &texture);

	glm::vec2 getSize() const; ///< Return the quad's [width,height].
	void setSize(const glm::vec2 &size); ///< Set the quad's [width,height].

	void setXOR(bool enabled); ///< Enable/Disable XOR mode.

	/** Is the point within the quad? */
	bool isIn(const glm::vec2 &point) const;

	// Renderable
	void calculateDistance();
	void render(RenderPass pass);

private:
	TextureHandle _texture;

	float _r;
	float _g;
	float _b;
	float _a;

	glm::vec2 _p1;
	glm::vec2 _p2;

	glm::vec2 _t1;
	glm::vec2 _t2;

	bool _xor;
};

} // End of namespace Aurora

} // End of namespace Graphics

#endif // GRAPHICS_AURORA_GUIQUAD_H
