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

/** @file engines/nwn/gui/ingame/dialog.cpp
 *  The NWN ingame dialog panel.
 */

// TODO: Make dialog boxes resizeable and/or repositionable?
// TODO: Actually, in the original, the dialog boxes do resize themselves up to a point...

#include "common/util.h"
#include "common/configman.h"

#include "aurora/talkman.h"
#include "aurora/ssffile.h"
#include "aurora/dlgfile.h"

#include "events/events.h"

#include "graphics/graphics.h"
#include "graphics/font.h"

#include "graphics/aurora/text.h"
#include "graphics/aurora/textureman.h"
#include "graphics/aurora/cursorman.h"

#include "engines/aurora/tokenman.h"

#include "engines/nwn/types.h"
#include "engines/nwn/object.h"
#include "engines/nwn/creature.h"
#include "engines/nwn/module.h"

#include "engines/nwn/gui/widgets/portrait.h"

#include "engines/nwn/gui/ingame/dialog.h"

static const float kDialogWidth  = 350.0;
static const float kDialogHeight = 254.0;

static const float kLightBlueR = 101.0 / 255.0;
static const float kLightBlueG = 176.0 / 255.0;
static const float kLightBlueB = 252.0 / 255.0;

static const uint32 kContinue  = 1741;
static const uint32 kEndDialog = 1742;

namespace Engines {

namespace NWN {

DialogBox::Reply::Reply(const Common::UString &r, uint32 i) : reply(r), id(i) {
}


DialogBox::ReplyLine::ReplyLine() : count(0), line(0) {
}

DialogBox::ReplyLine::ReplyLine(std::list<Reply>::const_iterator &i) :
	count(0), line(0), reply(i) {

}


DialogBox::DialogBox(const glm::vec2 &size) : _size(size),
	_position(0.0, 0.0, 0.0), _replyCount(0), _replyCountWidth(0.0) {

	const Common::UString fontName =
		ConfigMan.getBool("largefonts") ? "fnt_dialog_big16" : "fnt_dialog16x16";
	_font = FontMan.get(fontName);

	_portrait = new Portrait("", Portrait::kSizeMedium);

	_name = new Graphics::Aurora::Text(FontMan.get("fnt_galahad14"), " ",
	                                   kLightBlueR, kLightBlueG, kLightBlueB);

	_highlightedReply = _replyLines.end();
	_pickedReply      = _replies.end();
}

DialogBox::~DialogBox() {
	clearReplies();
	clearEntry();

	delete _name;
	delete _portrait;
}

void DialogBox::show() {
	GfxMan.lockFrame();

	_portrait->show();
	_name->show();

	showEntry();
	showReplies();

	Graphics::GUIFrontElement::show();

	GfxMan.unlockFrame();
}

void DialogBox::hide() {
	GfxMan.lockFrame();

	hideReplies();
	hideEntry();

	_name->hide();
	_portrait->hide();

	Graphics::GUIFrontElement::hide();

	GfxMan.unlockFrame();
}

bool DialogBox::isIn(const glm::vec2 &point) const {
	return Common::insideOf(point, _position.xy(), _position.xy() + _size);
}

glm::vec2 DialogBox::getSize() const {
	return _size;
}

glm::vec3 DialogBox::getPosition() const {
	return _position;
}

void DialogBox::setPosition(const glm::vec3 &position) {
	GfxMan.lockFrame();

	_position = position;

	// Portrait

	const glm::vec3 ppos = _position + glm::vec3(3.0, _size.y - _portrait->getSize().y - 3.0, -10.0);

	_portrait->setPosition(ppos);

	// Name

	const glm::vec3 npos = ppos + glm::vec3(_portrait->getSize() + glm::vec2(5.0, -_name->getSize().y),
	                                        0.0);

	_name->setPosition(npos);

	// NPC Entry

	glm::vec3 epos = npos + glm::vec3(0.0, -4.0, 0.0);

	for (std::list<Graphics::Aurora::Text *>::iterator e = _entryLines.begin();
	     e != _entryLines.end(); ++e) {
		epos.y -= _font.getFont().getHeight() + _font.getFont().getLineSpacing();

		(*e)->setPosition(epos);
	}

	// PC Replies

	glm::vec3 rpos = glm::vec3(_position.x + 5.0, glm::min(epos.y, ppos.y) - 4.0, ppos.z);
	const float replyCountRight = rpos.x + _replyCountWidth;

	for (std::list<ReplyLine>::iterator r = _replyLines.begin(); r != _replyLines.end(); ++r) {
		rpos.y -= _font.getFont().getHeight() + _font.getFont().getLineSpacing();

		if (r->count) {
			rpos.x = replyCountRight - r->count->getSize().x;

			r->count->setPosition(rpos);
		}

		rpos.x = replyCountRight;

		if (r->line)
			r->line->setPosition(rpos);
	}

	resort();

	GfxMan.unlockFrame();
}

void DialogBox::clear() {
	clearReplies();
	clearEntry();

	setPortrait("");
	setName("");
}

void DialogBox::setPortrait(const Common::UString &portrait) {
	_portrait->setPortrait(portrait);
}

void DialogBox::setName(const Common::UString &name) {
	// TODO: DialogBox::setName(): Check whether the name overflows the box

	_name->set(name);
}

void DialogBox::showEntry() {
	for (std::list<Graphics::Aurora::Text *>::iterator e = _entryLines.begin();
	     e != _entryLines.end(); ++e)
		(*e)->show();
}

void DialogBox::hideEntry() {
	for (std::list<Graphics::Aurora::Text *>::iterator e = _entryLines.begin();
	     e != _entryLines.end(); ++e)
		(*e)->hide();
}

void DialogBox::clearEntry() {
	if (_entry.empty() && _entryLines.empty())
		return;

	GfxMan.lockFrame();

	hideEntry();

	for (std::list<Graphics::Aurora::Text *>::iterator e = _entryLines.begin();
	     e != _entryLines.end(); ++e)
		delete *e;

	_entryLines.clear();
	_entry.clear();

	GfxMan.unlockFrame();
}

void DialogBox::setEntry(const Common::UString &entry) {
	GfxMan.lockFrame();

	clearEntry();

	if (entry.empty()) {
		GfxMan.unlockFrame();
		return;
	}

	_entry = TokenMan.parse(entry);

	// TODO: Check entry length, scrollbars

	const float maxWidth = _size.x - 2.0 - 2.0 - _portrait->getSize().x - 5.0;

	std::vector<Common::UString> lines;
	_font.getFont().split(_entry, lines, maxWidth);

	for (std::vector<Common::UString>::iterator l = lines.begin(); l != lines.end(); ++l)
		_entryLines.push_back(new Graphics::Aurora::Text(_font, *l));

	setPosition(_position);

	if (isVisible())
		showEntry();

	GfxMan.unlockFrame();
}

void DialogBox::showReplies() {
	for (std::list<ReplyLine>::iterator r = _replyLines.begin(); r != _replyLines.end(); ++r) {
		if (r->count)
			r->count->show();

		if (r->line)
			r->line->show();
	}
}

void DialogBox::hideReplies() {
	for (std::list<ReplyLine>::iterator r = _replyLines.begin(); r != _replyLines.end(); ++r) {
		if (r->count)
			r->count->hide();

		if (r->line)
			r->line->hide();
	}
}

void DialogBox::clearReplies() {
	hideReplies();

	setHighlight(_replyLines.end());
	_pickedReply = _replies.end();

	for (std::list<ReplyLine>::iterator r = _replyLines.begin(); r != _replyLines.end(); ++r) {
		delete r->count;
		delete r->line;
	}

	_replyLines.clear();
	_replies.clear();

	_replyCount = 0;
	_replyCountWidth = 0.0;
}

void DialogBox::addReply(const Common::UString &reply, uint32 id) {
	_replies.push_back(Reply(reply, id));
}

void DialogBox::finishReplies() {
	// Clear the current reply lines

	for (std::list<ReplyLine>::iterator r = _replyLines.begin(); r != _replyLines.end(); ++r) {
		delete r->count;
		delete r->line;
	}

	_replyLines.clear();


	_replyCount      = 0;
	_replyCountWidth = 0.0;

	// Create the reply number texts

	for (std::list<Reply>::const_iterator r = _replies.begin(); r != _replies.end(); ++r) {
		_replyLines.push_back(ReplyLine(r));

		_replyLines.back().count =
			new Graphics::Aurora::Text(_font, Common::UString::sprintf("%d. ", ++_replyCount),
			                           kLightBlueR, kLightBlueG, kLightBlueB);

		_replyCountWidth = MAX(_replyCountWidth, _replyLines.back().count->getSize().x);
	}

	// Create the reply line texts

	const float maxWidth = _size.x - 6.0 - _replyCountWidth;

	for (std::list<ReplyLine>::iterator r = _replyLines.begin(); r != _replyLines.end(); ++r) {
		std::vector<Common::UString> lines;

		std::list<Reply>::const_iterator reply = r->reply;

		_font.getFont().split(TokenMan.parse(reply->reply), lines, maxWidth);

		std::vector<Common::UString>::iterator line = lines.begin();
		if (line == lines.end())
			continue;

		r->line = new Graphics::Aurora::Text(_font, *line,
		                                     kLightBlueR, kLightBlueG, kLightBlueB);

		for (++line; line != lines.end(); ++line) {
			r = _replyLines.insert(++r, ReplyLine(reply));

			r->line = new Graphics::Aurora::Text(_font, *line,
			                                     kLightBlueR, kLightBlueG, kLightBlueB);
		}

	}

	setPosition(_position);

	if (isVisible())
		showReplies();
}

void DialogBox::mouseMove(const glm::ivec2 &point) {
	const glm::vec2 spos = CursorMan.toScreenCoordinates(point);

	if (!isIn(spos)) {
		setHighlight(_replyLines.end());
		return;
	}

	std::list<ReplyLine>::iterator highlight;
	for (highlight = _replyLines.begin(); highlight != _replyLines.end(); ++highlight)
		if ((highlight->count && highlight->count->isIn(spos)) ||
		    (highlight->line  && highlight->line->isIn (spos)))
			break;

	setHighlight(highlight);
}

void DialogBox::mouseClick(const glm::ivec2 &point) {
	mouseMove(point);

	if (_highlightedReply == _replyLines.end())
		_pickedReply = _replies.end();
	else
		_pickedReply = _highlightedReply->reply;
}

void DialogBox::pickReply(uint32 n) {
	if (n >= _replyCount) {
		_pickedReply = _replies.end();
		return;
	}

	_pickedReply = _replies.begin();
	std::advance(_pickedReply, n);
}

uint32 DialogBox::getPickedID() const {
	if (_pickedReply == _replies.end())
		return Aurora::DLGFile::kInvalidLine;

	return _pickedReply->id;
}

void DialogBox::setHighlight(const std::list<ReplyLine>::iterator &h) {
	if (_highlightedReply != _replyLines.end()) {
		uint32 id = _highlightedReply->reply->id;

		for (std::list<ReplyLine>::iterator r = _replyLines.begin();
		     r != _replyLines.end(); ++r) {

			if (r->reply->id != id)
				continue;

			if (r->count)
				r->count->setColor(kLightBlueR, kLightBlueG, kLightBlueB, 1.0);
			if (r->line)
				r->line->setColor(kLightBlueR, kLightBlueG, kLightBlueB, 1.0);
		}

	}

	_highlightedReply = h;

	if (_highlightedReply != _replyLines.end()) {
		if (_highlightedReply->count)
			_highlightedReply->count->setColor(1.0, 1.0, 1.0, 1.0);
		if (_highlightedReply->line)
			_highlightedReply->line->setColor(1.0, 1.0, 1.0, 1.0);
	}

	if (_highlightedReply != _replyLines.end()) {
		uint32 id = _highlightedReply->reply->id;

		for (std::list<ReplyLine>::iterator r = _replyLines.begin();
		     r != _replyLines.end(); ++r) {

			if (r->reply->id != id)
				continue;

			if (r->count)
				r->count->setColor(1.0, 1.0, 1.0, 1.0);
			if (r->line)
				r->line->setColor(1.0, 1.0, 1.0, 1.0);
		}

	}

}

void DialogBox::calculateDistance() {
	_distance = _position.z;
}

void DialogBox::render(Graphics::RenderPass pass) {
	if (pass == Graphics::kRenderPassOpaque)
		return;

	TextureMan.reset();

	const glm::vec2 botLeft  = glm::vec2(_position);
	const glm::vec2 topLeft  = botLeft  + glm::vec2(0.0, _size.y);
	const glm::vec2 botRight = botLeft  + glm::vec2(_size.x, 0.0);
	const glm::vec2 topRight = botLeft  + _size;

	// Backdrop
	glColor4f(0.0, 0.0, 0.0, 0.5);
	glBegin(GL_QUADS);
		glVertex2fv(glm::value_ptr(botLeft));
		glVertex2fv(glm::value_ptr(botRight));
		glVertex2fv(glm::value_ptr(topRight));
		glVertex2fv(glm::value_ptr(topLeft));
	glEnd();

	// Edges
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_LINE_STRIP);
		glVertex2fv(glm::value_ptr(botLeft));
		glVertex2fv(glm::value_ptr(botRight));
		glVertex2fv(glm::value_ptr(topRight));
		glVertex2fv(glm::value_ptr(topLeft));
		glVertex2fv(glm::value_ptr(botLeft));
	glEnd();

	glColor4f(1.0, 1.0, 1.0, 1.0);
}


Dialog::Dialog(const Common::UString &conv, Creature &pc, Object &obj,
               Module &module, bool playHello) :
	_conv(conv), _pc(&pc), _object(&obj), _module(&module) {

	_object->setPCSpeaker(&pc);

	_dlg = new Aurora::DLGFile(conv, _object);
	_dlg->startConversation();

	_dlgBox = new DialogBox(glm::vec2(kDialogWidth, kDialogHeight));

	updateBox();
	playSound(playHello);
	playAnimation();

	notifyResized(glm::ivec2(0, 0), GfxMan.getScreenSize());
}

Dialog::~Dialog() {
	abort();

	delete _dlg;
	delete _dlgBox;
}

bool Dialog::hasEnded() const {
	return _dlg->hasEnded();
}

void Dialog::show() {
	_dlgBox->show();
}

void Dialog::hide() {
	_dlgBox->hide();
}

void Dialog::abort() {
	stopAnimation();

	hide();

	_object->setPCSpeaker(0);
	_object->stopSound();

	_dlg->abortConversation();
}

void Dialog::addEvent(const Events::Event &event) {
	_eventQueue.push_back(event);
}

int Dialog::processEventQueue() {
	bool hasMove = false;

	for (std::list<Events::Event>::const_iterator e = _eventQueue.begin();
	     e != _eventQueue.end(); ++e) {

		if      (e->type == Events::kEventMouseMove)
			hasMove = true;
		else if (e->type == Events::kEventKeyDown)
			keyPressed(*e);
		else if (e->type == Events::kEventMouseDown)
			mouseClick(*e);
	}

	_eventQueue.clear();

	if (hasMove)
		mouseMove();

	return hasEnded() ? 1 : 0;
}

void Dialog::mouseMove() {
	const glm::ivec2 cursor = CursorMan.getPosition();

	_dlgBox->mouseMove(cursor);
}

void Dialog::mouseClick(const Events::Event &event) {
	if (event.button.button != SDL_BUTTON_LMASK)
		return;

	_dlgBox->mouseClick(glm::ivec2(event.button.x, event.button.y));
	checkPicked();
}

void Dialog::keyPressed(const Events::Event &event) {
	if (event.key.keysym.sym == SDLK_ESCAPE) {
		abort();
		return;
	}

	if (event.key.keysym.sym == SDLK_1)
		_dlgBox->pickReply(0);
	else if (event.key.keysym.sym == SDLK_2)
		_dlgBox->pickReply(1);
	else if (event.key.keysym.sym == SDLK_3)
		_dlgBox->pickReply(2);
	else if (event.key.keysym.sym == SDLK_4)
		_dlgBox->pickReply(3);
	else if (event.key.keysym.sym == SDLK_5)
		_dlgBox->pickReply(4);
	else if (event.key.keysym.sym == SDLK_6)
		_dlgBox->pickReply(5);
	else if (event.key.keysym.sym == SDLK_7)
		_dlgBox->pickReply(6);
	else if (event.key.keysym.sym == SDLK_8)
		_dlgBox->pickReply(7);
	else if (event.key.keysym.sym == SDLK_9)
		_dlgBox->pickReply(8);
	else if (event.key.keysym.sym == SDLK_0)
		_dlgBox->pickReply(9);

	checkPicked();
}

void Dialog::checkPicked() {
	uint32 picked = _dlgBox->getPickedID();
	if (picked == Aurora::DLGFile::kInvalidLine)
		return;

	_dlg->pickReply(picked);
	if (_dlg->hasEnded()) {
		stopAnimation();
		return;
	}

	updateBox();
	playSound(false);
	playAnimation();

	// Update the highlighted reply
	mouseMove();
}

void Dialog::notifyResized(const glm::ivec2 &oldSize, const glm::ivec2 &newSize) {
	const float x = -(newSize.x / 2.0)                        + 10.0;
	const float y =  (newSize.y / 2.0) - _dlgBox->getSize().y - 20.0;

	_dlgBox->setPosition(glm::vec3(x, y, 0.0));
}

void Dialog::updateBox() {
	GfxMan.lockFrame();

	_dlgBox->clear();

	// Entry


	const Aurora::DLGFile::Line *entry = _dlg->getCurrentEntry();
	if (entry) {
		// Name and portrait

		Object *speaker = getSpeaker();

		if (speaker) {
			_dlgBox->setPortrait(speaker->getPortrait());
			_dlgBox->setName(speaker->getName());
		} else
			_dlgBox->setName("[INVALID NPC]");

		// Text
		_dlgBox->setEntry(entry->text.getString());
	}

	// Replies

	const std::vector<const Aurora::DLGFile::Line *> &replies = _dlg->getCurrentReplies();
	if (!replies.empty()) {
		for (std::vector<const Aurora::DLGFile::Line *>::const_iterator r = replies.begin();
				 r != replies.end(); ++r) {

			Common::UString text = (*r)->text.getString();
			if (text.empty())
				text = TalkMan.getString((*r)->isEnd ? kEndDialog : kContinue);

			_dlgBox->addReply(text, (*r)->id);
		}
	} else
		_dlgBox->addReply(TalkMan.getString(kEndDialog), Aurora::DLGFile::kEndLine);

	_dlgBox->finishReplies();

	GfxMan.unlockFrame();
}

Object *Dialog::getSpeaker() {
	const Aurora::DLGFile::Line *entry = _dlg->getCurrentEntry();
	if (!entry)
		return 0;

	if (!entry->speaker.empty())
		return dynamic_cast<Object *>(_module->findObject(entry->speaker));

	return _object;
}

void Dialog::playSound(bool greeting) {
	const Aurora::DLGFile::Line *entry = _dlg->getCurrentEntry();
	if (!entry)
		return;

	Common::UString sound = entry->sound;

	bool isSSF = false;
	if (sound.empty() && greeting) {
		const Aurora::SSFFile *ssf = _object->getSSF();

		if (ssf) {
			isSSF = true;

			sound = ssf->getSound(kSSFHello).fileName;
		}
	}

	_object->playSound(sound, isSSF);
}

struct TalkAnim {
	TalkAnimation id;
	const char *name;
};

static const TalkAnim kTalkAnimations[] = {
	{kTalkAnimationDefault , "tlknorm" },
	{kTalkAnimationNormal  , "tlknorm" },
	{kTalkAnimationPleading, "tlkplead"},
	{kTalkAnimationForceful, "tlkforce"},
	{kTalkAnimationLaugh   , "tlklaugh"}
};

void Dialog::playAnimation() {
	Object *speaker = getSpeaker();
	if (!speaker)
		return;

	const Aurora::DLGFile::Line *entry = _dlg->getCurrentEntry();
	if (!entry || (entry->animation == kTalkAnimationNone)) {
		stopAnimation();
		return;
	}

	const char *anim = 0;
	for (int i = 0; i < ARRAYSIZE(kTalkAnimations); i++) {
		if (entry->animation == (uint32)kTalkAnimations[i].id) {
			anim = kTalkAnimations[i].name;
			break;
		}
	}

	if (!anim) {
		warning("Dialog::playAnimation(): Animation %d", entry->animation);
		stopAnimation();
		return;
	}

	speaker->playAnimation(anim, false, -1);
}

void Dialog::stopAnimation() {
	Object *speaker = getSpeaker();
	if (!speaker)
		return;

	speaker->playAnimation();
}

} // End of namespace NWN

} // End of namespace Engines
