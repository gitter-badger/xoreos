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

/** @file engines/kotor/gui/widgets/kotorwidget.cpp
 *  A KotOR widget.
 */

#include "common/util.h"

#include "aurora/types.h"
#include "aurora/gfffile.h"
#include "aurora/talkman.h"

#include "graphics/aurora/guiquad.h"
#include "graphics/aurora/text.h"
#include "graphics/aurora/highlightabletext.h"
#include "graphics/aurora/highlightableguiquad.h"

#include "engines/kotor/gui/widgets/kotorwidget.h"

namespace Engines {

namespace KotOR {

KotORWidget::Extend::Extend() : position(0.0, 0.0), size(0.0, 0.0) {
}


KotORWidget::Border::Border() : fillStyle(0), dimension(0), innerOffset(0),
	r(0.0), g(0.0), b(0.0), pulsing(false) {

}


KotORWidget::Text::Text() : strRef(Aurora::kStrRefInvalid), align(0.0, 0.0),
	r(1.0), g(1.0), b(1.0), pulsing(false) {

}


KotORWidget::KotORWidget(::Engines::GUI &gui, const Common::UString &tag) :
	Widget(gui, tag), _size(0.0, 0.0), _r(1.0), _g(1.0), _b(1.0), _a(1.0),
	_quad(0), _text(0) {

}

KotORWidget::~KotORWidget() {
	delete _text;
	delete _quad;
}

void KotORWidget::show() {
	if (isInvisible())
		return;

	Widget::show();

	if (_quad)
		_quad->show();
	if (_text)
		_text->show();
}

void KotORWidget::hide() {
	if (isInvisible())
		return;

	if (_quad)
		_quad->hide();
	if (_text)
		_text->hide();

	Widget::hide();
}

void KotORWidget::setTag(const Common::UString &tag) {
	Widget::setTag(tag);

	if (_quad)
		_quad->setTag(getTag());
	if (_text)
		_text->setTag(getTag());
}

void KotORWidget::setPosition(const glm::vec3 &position) {
	const glm::vec3 opos = getPosition();

	Widget::setPosition(position);
	const glm::vec3 npos = getPosition();

	if (_quad) {
		const glm::vec3 qpos = _quad->getPosition();

		_quad->setPosition(qpos - opos + npos);
	}

	if (_text) {
		const glm::vec3 tpos = _text->getPosition();

		_text->setPosition(tpos - opos + npos);
	}
}

glm::vec2 KotORWidget::getSize() const {
	return _size;
}

void KotORWidget::setFill(const Common::UString &fill) {
	if (!_quad) {
		const glm::vec3 position = getPosition();

		_quad = new Graphics::Aurora::GUIQuad("", glm::vec2(0.0, 0.0), _size);
		_quad->setPosition(position);
		_quad->setTag(getTag());
		_quad->setClickable(true);

		if (isVisible())
			_quad->show();
	}

	_quad->setTexture(fill);
	_quad->setColor(1.0, 1.0, 1.0, 1.0);
}

void KotORWidget::load(const Aurora::GFFStruct &gff) {
	const glm::vec3 color = gff.getVector("COLOR");
	_r = color.r;
	_g = color.g;
	_b = color.b;
	_a = gff.getDouble("ALPHA", 1.0);

	Extend extend = createExtend(gff);

	_size = extend.size;

	Widget::setPosition(glm::vec3(extend.position, 0.0));

	Border border = createBorder(gff);

	if (!border.fill.empty()) {
		_quad = new Graphics::Aurora::HighlightableGUIQuad(border.fill, glm::vec2(0.0, 0.0), extend.size);
	} else {
		_quad = new Graphics::Aurora::GUIQuad(border.fill, glm::vec2(0.0, 0.0), extend.size);
	}

	_quad->setPosition(glm::vec3(extend.position, 0.0));
	_quad->setTag(getTag());
	_quad->setClickable(true);

	if (border.fill.empty())
		_quad->setColor(0.0, 0.0, 0.0, 0.0);

	Text text = createText(gff);

	if (!text.text.empty() && !text.font.empty()) {
		_text = new Graphics::Aurora::HighlightableText(FontMan.get(text.font), text.text,
		                                   text.r, text.g, text.b, 1.0);

		const glm::vec2 span = extend.size - _text->getSize();
		const glm::vec2 tpos = extend.position + text.align * span;

		_text->setPosition(glm::vec3(tpos, -1.0));
		_text->setTag(getTag());
		_text->setClickable(true);
	}
}

void KotORWidget::setColor(float r, float g, float b, float a) {
		_quad->setColor(r, g, b, a);
}

void KotORWidget::setText(const Common::UString &text) {
		const glm::vec2 epos = glm::vec2(Widget::getPosition());
		const glm::vec2 tpos = glm::vec2(_text->getPosition());

		const glm::vec2 align = (tpos - epos) / (_size - _text->getSize());
		_text->set(text);

		const glm::vec2 span = _size - _text->getSize();
		const glm::vec2 npos = epos + align * span;

		_text->setPosition(glm::vec3(npos, -1.0));
}

KotORWidget::Extend KotORWidget::createExtend(const Aurora::GFFStruct &gff) {
	Extend extend;

	if (gff.hasField("EXTENT")) {
		const Aurora::GFFStruct &e = gff.getStruct("EXTENT");

		extend.position = glm::vec2(e.getSint("LEFT"), e.getSint("TOP"));
		extend.size = glm::vec2(e.getSint("WIDTH"), e.getSint("HEIGHT"));
	}

	return extend;
}

KotORWidget::Border KotORWidget::createBorder(const Aurora::GFFStruct &gff) {
	Border border;

	if (gff.hasField("BORDER")) {
		const Aurora::GFFStruct &b = gff.getStruct("BORDER");

		border.corner = b.getString("CORNER");
		border.edge   = b.getString("EDGE");
		border.fill   = b.getString("FILL");

		border.fillStyle   = b.getUint("FILLSTYLE");
		border.dimension   = b.getUint("DIMENSION");
		border.innerOffset = b.getUint("INNEROFFSET");

		const glm::vec3 color = b.getVector("COLOR");
		border.r = color.r;
		border.g = color.g;
		border.b = color.b;

		border.pulsing = b.getBool("PULSING");
	}
	return border;
}

KotORWidget::Text KotORWidget::createText(const Aurora::GFFStruct &gff) {
	Text text;

	if (gff.hasField("TEXT")) {
		const Aurora::GFFStruct &t = gff.getStruct("TEXT");

		text.font   = t.getString("FONT");
		text.text   = t.getString("TEXT");
		text.strRef = t.getUint("STRREF", Aurora::kStrRefInvalid);

		const uint32 alignment = t.getUint("ALIGNMENT");

		const glm::vec3 color = t.getVector("COLOR");
		text.r = color.r;
		text.g = color.g;
		text.b = color.b;

		text.pulsing = t.getBool("PULSING");


		if (text.text == "(Unitialized)")
			text.text.clear();

		if (text.strRef != Aurora::kStrRefInvalid)
			text.text = TalkMan.getString(text.strRef);

		// TODO: KotORWidget::getText(): Alignment
		if (alignment == 18) {
			text.align = glm::vec2(0.5, 0.5);
		}
	}

	return text;
}

Graphics::Aurora::Highlightable* KotORWidget::getTextHighlightableComponent() const {
	return static_cast<Graphics::Aurora::Highlightable*>(_text);
}

Graphics::Aurora::Highlightable* KotORWidget::getQuadHighlightableComponent() const {
	return dynamic_cast<Graphics::Aurora::Highlightable*>(_quad);
}

} // End of namespace KotOR

} // End of namespace Engines
