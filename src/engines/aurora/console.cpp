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

/** @file engines/aurora/console.cpp
 *  Generic Aurora engines (debug) console.
 */

#include <cstdarg>
#include <cstdio>

#include <boost/bind.hpp>

#include "common/util.h"
#include "common/filepath.h"
#include "common/readline.h"

#include "aurora/resman.h"

#include "graphics/graphics.h"
#include "graphics/font.h"

#include "sound/sound.h"

#include "events/events.h"

#include "graphics/aurora/textureman.h"
#include "graphics/aurora/cursorman.h"
#include "graphics/aurora/text.h"
#include "graphics/aurora/guiquad.h"

#include "engines/aurora/console.h"
#include "engines/aurora/util.h"


static const uint32 kDoubleClickTime = 500;

static const char *kPrompt = " >";

static const uint32 kCommandHistorySize = 100;
static const uint32 kConsoleHistory     = 500;
static const uint32 kConsoleLines       =  25;

namespace Engines {

ConsoleWindow::ConsoleWindow(const Common::UString &font, uint32 lines, uint32 history,
                             int fontHeight) : _font(FontMan.get(font, fontHeight)),
	_historySizeMax(history), _historySizeCurrent(0), _historyStart(0),
	_cursorPosition(0), _overwrite(false),
	_cursorBlinkState(false), _lastCursorBlink(0) {

	assert(lines >= 2);
	assert(history >= lines);

	setTag("ConsoleWindow");
	setClickable(true);

	_lineHeight = _font.getFont().getHeight() + _font.getFont().getLineSpacing();
	_size.y     = floorf(lines * _lineHeight);

	_prompt = new Graphics::Aurora::Text(_font, "");
	_input  = new Graphics::Aurora::Text(_font, "");

	const float cursorHeight = _font.getFont().getHeight();
	_cursor = new Graphics::Aurora::GUIQuad("", glm::vec2(0.0, 1.0), glm::vec2(0.0, cursorHeight));
	_cursor->setXOR(true);

	_highlight = new Graphics::Aurora::GUIQuad("", glm::vec2(0.0, 0.0), glm::vec2(0.0, cursorHeight));
	_highlight->setColor(1.0, 1.0, 1.0, 0.0);
	_highlight->setXOR(true);

	_lines.reserve(lines - 1);
	for (uint32 i = 0; i < (lines - 1); i++)
		_lines.push_back(new Graphics::Aurora::Text(_font, ""));

	notifyResized(glm::ivec2(0, 0), GfxMan.getScreenSize());

	updateScrollbarLength();
	updateScrollbarPosition();

	clearHighlight();

	calculateDistance();
}

ConsoleWindow::~ConsoleWindow() {
	_redirect.flush();
	_redirect.close();

	for (std::vector<Graphics::Aurora::Text *>::iterator l = _lines.begin();
	     l != _lines.end(); ++l)
		delete *l;

	delete _highlight;
	delete _cursor;
	delete _prompt;
	delete _input;
}

void ConsoleWindow::show() {
	GfxMan.lockFrame();

	for (std::vector<Graphics::Aurora::Text *>::iterator l = _lines.begin();
	     l != _lines.end(); ++l)
		(*l)->show();

	_highlight->show();
	_cursor->show();
	_prompt->show();
	_input->show();

	Graphics::GUIFrontElement::show();

	GfxMan.unlockFrame();
}

void ConsoleWindow::hide() {
	GfxMan.lockFrame();

	for (std::vector<Graphics::Aurora::Text *>::iterator l = _lines.begin();
	     l != _lines.end(); ++l)
		(*l)->hide();

	_highlight->hide();
	_cursor->hide();
	_prompt->hide();
	_input->hide();

	Graphics::GUIFrontElement::hide();

	GfxMan.unlockFrame();
}

void ConsoleWindow::showPrompt() {
	if (!isVisible())
		return;

	GfxMan.lockFrame();

	_cursor->show();
	_prompt->show();
	_input->show();

	GfxMan.unlockFrame();
}

void ConsoleWindow::hidePrompt() {
	if (!isVisible())
		return;

	GfxMan.lockFrame();

	_cursor->hide();
	_prompt->hide();
	_input->hide();

	GfxMan.unlockFrame();
}

bool ConsoleWindow::isIn(const glm::vec2 &point) const {
	return Common::insideOf(point, _position.xy(), _position.xy() + _size);
}

bool ConsoleWindow::isIn(const glm::vec3 &position) const {
	return isIn(position.xy());
}

glm::vec2 ConsoleWindow::getSize() const {
	return _size;
}

glm::vec2 ConsoleWindow::getContentSize() const {
	return _size - glm::vec2(15.0, _lineHeight);
}

uint32 ConsoleWindow::getLines() const {
	return _lines.size();
}

uint32 ConsoleWindow::getColumns() const {
	return floorf(getContentSize().y / _font.getFont().getWidth('m'));
}

void ConsoleWindow::setPrompt(const Common::UString &prompt) {
	GfxMan.lockFrame();

	_prompt->set(prompt);

	_input->setPosition(glm::vec3(_position + glm::vec2(_prompt->getSize().x, 0.0), -1001.0));
	recalcCursor();

	GfxMan.unlockFrame();
}

void ConsoleWindow::setInput(const Common::UString &input, uint32 cursorPos,
		bool overwrite) {

	GfxMan.lockFrame();

	_inputText      = input;
	_cursorPosition = cursorPos;
	_overwrite      = overwrite;

	_cursorBlinkState = false;
	_lastCursorBlink  = 0;

	_input->set(input);
	recalcCursor();

	GfxMan.unlockFrame();
}

void ConsoleWindow::clear() {
	GfxMan.lockFrame();

	_history.clear();
	_historySizeCurrent = 0;

	_historyStart = 0;

	updateScrollbarLength();
	updateScrollbarPosition();

	for (std::vector<Graphics::Aurora::Text *>::iterator l = _lines.begin();
	     l != _lines.end(); ++l)
		(*l)->set("");
	GfxMan.unlockFrame();
}

void ConsoleWindow::print(const Common::UString &line) {
	std::vector<Common::UString> lines;

	_font.getFont().split(line, lines, _size.x - 15.0);
	for (std::vector<Common::UString>::iterator l = lines.begin(); l != lines.end(); ++l)
		printLine(*l);
}

void ConsoleWindow::printLine(const Common::UString &line) {
	if (_redirect.isOpen()) {
		_redirect.writeString(line);
		_redirect.writeByte('\n');
		return;
	}

	_history.push_back(line);
	if (_historySizeCurrent >= _historySizeMax)
		_history.pop_front();
	else
		_historySizeCurrent++;

	updateScrollbarLength();
	redrawLines();
}

bool ConsoleWindow::setRedirect(Common::UString redirect) {
	_redirect.flush();
	_redirect.close();

	if (redirect.empty())
		return true;

	redirect = Common::FilePath::makeAbsolute(redirect);
	if (!_redirect.open(redirect)) {
		Common::UString error =
			Common::UString::sprintf("Failed opening file \"%s\" for writing.", redirect.c_str());

		print(error);
		return false;
	}

	return true;
}

void ConsoleWindow::updateHighlight() {
	if ((_highlightLength == 0) || (_highlightPosition.y >= kConsoleLines)) {
		_highlight->setColor(1.0, 1.0, 1.0, 0.0);
		return;
	}

	const float charWidth = _font.getFont().getWidth(' ');

	const int32 start = _highlightPosition.x;
	const int32 end   = _highlightPosition.x + _highlightLength;

	const  int32 x      = MIN(start, end);
	const uint32 length = ABS(start - end);

	const glm::vec2 hsize = _highlight->getSize();
	_highlight->setSize(glm::vec2(length * charWidth, hsize.y));
	_highlight->setPosition(glm::vec3(_position + glm::vec2(x * charWidth, _highlightPosition.y * _lineHeight), -1002.0));
	_highlight->setColor(1.0, 1.0, 1.0, 1.0);
}

glm::vec2 ConsoleWindow::getPosition(const glm::ivec2 &cursor) {
	const glm::vec2 rpos = CursorMan.toScreenCoordinates(cursor);

	return (rpos - _position) / glm::vec2(_font.getFont().getWidth(' '), _lineHeight);
}

glm::uvec2 ConsoleWindow::highlightClip(const glm::uvec2 &position) const {
	glm::uvec2 min = glm::uvec2(0, 0);
	glm::uvec2 max = glm::uvec2(0, _lines.size());

	if (position.y == 0) {
		min.x = _prompt->get().size();
		max.x = _prompt->get().size() + _input->get().size();
	} else {
		min.x = 0;
		max.x = _lines[_lines.size() - position.y]->get().size();
	}

	return glm::clamp(position, min, max);
}

void ConsoleWindow::startHighlight(const glm::ivec2 &cursor) {
	clearHighlight();

	const glm::vec2 lpos = getPosition(cursor);
	if (!isIn(lpos))
		return;

	_highlightPosition = highlightClip(glm::uvec2(lpos));

	updateHighlight();
}

void ConsoleWindow::stopHighlight(const glm::ivec2 &cursor) {
	const glm::vec2 lpos = getPosition(cursor);
	if (!isIn(lpos))
		return;

	const glm::uvec2 epos = highlightClip(glm::uvec2(lpos.x, _highlightPosition.y));

	_highlightLength = epos.x - _highlightPosition.x;

	updateHighlight();
}

void ConsoleWindow::highlightWord(const glm::ivec2 &cursor) {
	clearHighlight();

	const glm::vec2 lpos = getPosition(cursor);
	if (!isIn(lpos))
		return;

	const glm::uvec2 wpos = highlightClip(glm::uvec2(lpos));

	const Common::UString &line = (wpos.y == 0) ? _input->get() :
	                                              _lines[_lines.size() - wpos.y]->get();
	const uint32 pos = (wpos.y == 0) ? (wpos.x - _prompt->get().size()) : wpos.x;

	uint32 wordStart = findWordStart(line, pos);
	uint32 wordEnd   = findWordEnd  (line, pos);

	_highlightPosition.x = (wpos.y == 0) ? (wordStart + _prompt->get().size()) : wordStart;
	_highlightPosition.y =  wpos.y;
	_highlightLength     = wordEnd - wordStart;

	updateHighlight();
}

void ConsoleWindow::highlightLine(const glm::ivec2 &cursor) {
	clearHighlight();

	const glm::vec2 lpos = getPosition(cursor);
	if (!isIn(lpos))
		return;

	_highlightPosition = highlightClip(glm::uvec2(0, lpos.y));

	const Common::UString &line = (_highlightPosition.y == 0) ?
		_input->get() : _lines[_lines.size() - _highlightPosition.y]->get();
	_highlightLength = line.size();

	updateHighlight();
}

void ConsoleWindow::clearHighlight() {
	_highlightPosition.x      = 0;
	_highlightPosition.y      = 0;
	_highlightLength = 0;

	updateHighlight();
}

Common::UString ConsoleWindow::getHighlight() const {
	if ((_highlightLength == 0) || (_highlightPosition.y >= kConsoleLines))
		return "";

	int32 start = _highlightPosition.x;
	int32 end   = _highlightPosition.x + _highlightLength;

	if (start > end)
		SWAP(start, end);

	Common::UString line;
	if (_highlightPosition.y == 0) {
		start = start - _prompt->get().size();
		end   = end   - _prompt->get().size();
		line  = _input->get();
	} else
		line = _lines[_lines.size() - _highlightPosition.y]->get();

	start = MAX(0, start);
	end   = MAX(0, end  );

	return line.substr(line.getPosition(start), line.getPosition(end));
}

void ConsoleWindow::scrollUp(uint32 n) {
	if ((_historyStart + _lines.size()) >= _historySizeCurrent)
		return;

	_historyStart += MIN<uint32>(n, _historySizeCurrent - _lines.size() - _historyStart);

	updateScrollbarPosition();
	redrawLines();
}

void ConsoleWindow::scrollDown(uint32 n) {
	if (_historyStart == 0)
		return;

	_historyStart -= MIN(n, _historyStart);

	updateScrollbarPosition();
	redrawLines();
}

void ConsoleWindow::scrollTop() {
	if (_historySizeCurrent <= _lines.size())
		return;

	const uint32 bottom = _historySizeCurrent - _lines.size();
	if (bottom == _historyStart)
		return;

	_historyStart = bottom;

	updateScrollbarPosition();
	redrawLines();
}

void ConsoleWindow::scrollBottom() {
	if (_historyStart == 0)
		return;

	_historyStart = 0;

	updateScrollbarPosition();
	redrawLines();
}

void ConsoleWindow::calculateDistance() {
	_distance = -1000.0;
}

void ConsoleWindow::render(Graphics::RenderPass pass) {
	if (pass == Graphics::kRenderPassOpaque)
		return;

	uint32 now = EventMan.getTimestamp();
	if ((now - _lastCursorBlink) > 500) {
		_cursorBlinkState = !_cursorBlinkState;
		_lastCursorBlink = now;

		_cursor->setColor(1.0, 1.0, 1.0, _cursorBlinkState ? 1.0 : 0.0);
	}

	TextureMan.reset();

	const glm::vec2 botLeft  = glm::vec2(_position);
	const glm::vec2 topLeft  = botLeft  + glm::vec2(0.0, _size.y);
	const glm::vec2 botRight = botLeft  + glm::vec2(_size.x, 0.0);
	const glm::vec2 topRight = botLeft  + _size;

	const glm::vec2 edgeMargin   = glm::vec2( 0.0, 3.0);
	const glm::vec2 scrollMargin = glm::vec2(12.0, 0.0);

  const glm::vec2 scrollSize     = glm::vec2(8.0, _scrollbarLength);
	const glm::vec2 scrollBotLeft  = botRight      + glm::vec2(-10.0, 2.0 + _scrollbarPosition);
	const glm::vec2 scrollTopLeft  = scrollBotLeft + glm::vec2(0.0, scrollSize.y);
	const glm::vec2 scrollBotRight = scrollBotLeft + glm::vec2(scrollSize.x, 0.0);
	const glm::vec2 scrollTopRight = scrollBotLeft + scrollSize;

	// Backdrop
	glColor4f(0.0, 0.0, 0.0, 0.75);
	glBegin(GL_QUADS);
		glVertex2fv(glm::value_ptr(botLeft));
		glVertex2fv(glm::value_ptr(botRight));
		glVertex2fv(glm::value_ptr(topRight));
		glVertex2fv(glm::value_ptr(topLeft));
	glEnd();

	// Bottom edge
	glColor4f(0.0, 0.0, 0.0, 1.0);
	glBegin(GL_QUADS);
		glVertex2fv(glm::value_ptr(botLeft  - edgeMargin));
		glVertex2fv(glm::value_ptr(botRight - edgeMargin));
		glVertex2fv(glm::value_ptr(botRight));
		glVertex2fv(glm::value_ptr(botLeft));
	glEnd();

	// Scrollbar background
	glColor4f(0.0, 0.0, 0.0, 1.0);
	glBegin(GL_QUADS);
		glVertex2fv(glm::value_ptr(botRight - scrollMargin));
		glVertex2fv(glm::value_ptr(botRight));
		glVertex2fv(glm::value_ptr(topRight));
		glVertex2fv(glm::value_ptr(topRight - scrollMargin));
	glEnd();

	// Scrollbar
	glColor4f(0.5, 0.5, 0.5, 0.5);
	glBegin(GL_QUADS);
		glVertex2fv(glm::value_ptr(scrollBotLeft));
		glVertex2fv(glm::value_ptr(scrollBotRight));
		glVertex2fv(glm::value_ptr(scrollTopRight));
		glVertex2fv(glm::value_ptr(scrollTopLeft));
	glEnd();

	glColor4f(1.0, 1.0, 1.0, 1.0);
}

void ConsoleWindow::notifyResized(const glm::ivec2 &oldSize, const glm::ivec2 &newSize) {
	_size.x = newSize.x;

	_position.x = -(newSize.x / 2.0);
	_position.y =  (newSize.y / 2.0) - _size.y;

	float textY = (newSize.y / 2.0) - _lineHeight;
	for (uint32 i = 0; i < _lines.size(); i++, textY -= _lineHeight)
		_lines[i]->setPosition(glm::vec3(_position.x, textY, -1001.0));

	_prompt->setPosition(glm::vec3(_position, -1001.0));
	_input ->setPosition(glm::vec3(_position.x + _prompt->getSize().x, _position.y, -1001.0));

	recalcCursor();
}

uint32 ConsoleWindow::findWordStart(const Common::UString &line, uint32 pos) {
	Common::UString::iterator it = line.getPosition(pos);
	if ((it == line.end()) || (*it == ' '))
		return 0;

	while ((it != line.begin()) && (*it != ' '))
		--it;

	if (*it == ' ')
		++it;

	return line.getPosition(it);
}

uint32 ConsoleWindow::findWordEnd(const Common::UString &line, uint32 pos) {
	Common::UString::iterator it = line.getPosition(pos);
	if ((it == line.end()) || (*it == ' '))
		return 0;

	while ((it != line.end()) && (*it != ' '))
		++it;

	return line.getPosition(it);
}

void ConsoleWindow::recalcCursor() {
	Common::UString input = _inputText;
	input.truncate(_cursorPosition);

	glm::vec2 cursor = _position;
	cursor.x += _prompt->getSize().x + _font.getFont().getWidth(input) - 1.0;
	_cursor->setPosition(glm::vec3(cursor, -1002.0));

	glm::vec2 cursorSize = _cursor->getSize();
	cursorSize.x = 1.0 + (_overwrite ? _font.getFont().getWidth(' ') : 0.0);
	_cursor->setSize(cursorSize);
}

void ConsoleWindow::redrawLines() {
	GfxMan.lockFrame();

	std::list<Common::UString>::reverse_iterator h = _history.rbegin();
	for (uint32 i = 0; (i < _historyStart) && (h != _history.rend()); i++, ++h);

	for (int i = _lines.size() - 1; (i >= 0) && (h != _history.rend()); i--, ++h)
		_lines[i]->set(*h);

	GfxMan.unlockFrame();
}

void ConsoleWindow::updateScrollbarLength() {
	float length = 1.0;

	if (_historySizeCurrent > 0)
		length = ((float) _lines.size()) / _historySizeCurrent;

	const float height = _size.y - 4.0;
	_scrollbarLength = floorf(CLIP(length * height, 8.0f, height));
}

void ConsoleWindow::updateScrollbarPosition() {
	float position = 0.0;

	int max = _historySizeCurrent - _lines.size();
	if (max > 0)
		position = ((float) _historyStart) / max;

	const float span = (_size.y - 4.0) - _scrollbarLength;
	_scrollbarPosition = floorf(CLIP(position * span, 0.0f, span));
}


Console::Console(const Common::UString &font, int fontHeight) : _neverShown(true),
	_visible(false), _tabCount(0), _printedCompleteWarning(false), _lastClickCount(-1),
	_lastClickButton(0), _lastClickTime(0), _lastClickX(0), _lastClickY(0),
	_maxSizeVideos(0), _maxSizeSounds(0) {

	_readLine = new Common::ReadLine(kCommandHistorySize);
	_console  = new ConsoleWindow(font, kConsoleLines, kConsoleHistory, fontHeight);

	_readLine->historyIgnoreDups(true);

	registerCommand("help"       , boost::bind(&Console::cmdHelp       , this, _1),
			"Usage: help [<command>]\nPrint help text");
	registerCommand("clear"      , boost::bind(&Console::cmdClear      , this, _1),
			"Usage: clear\nClear the console window");
	registerCommand("exit"       , boost::bind(&Console::cmdExit       , this, _1),
			"Usage: exit\nLeave the console window, returning to the game");
	registerCommand("quitxoreos"    , boost::bind(&Console::cmdQuit    , this, _1),
			"Usage: quitxoreos\nShut down xoreos");
	registerCommand("dumpreslist", boost::bind(&Console::cmdDumpResList, this, _1),
			"Usage: dumpreslist <file>\nDump the current list of resources to file");
	registerCommand("dumpres"    , boost::bind(&Console::cmdDumpRes    , this, _1),
			"Usage: dumpres <resource>\nDump a resource to file");
	registerCommand("dumptga"    , boost::bind(&Console::cmdDumpTGA    , this, _1),
			"Usage: dumptga <resource>\nDump an image resource into a TGA");
	registerCommand("dump2da"    , boost::bind(&Console::cmdDump2DA    , this, _1),
			"Usage: dump2da <2da>\nDump a 2DA to file");
	registerCommand("dumpall2da" , boost::bind(&Console::cmdDumpAll2DA , this, _1),
			"Usage: dumpall2da\nDump all 2DA to file");
	registerCommand("listvideos" , boost::bind(&Console::cmdListVideos , this, _1),
			"Usage: listvideos\nList all available videos");
	registerCommand("playvideo"  , boost::bind(&Console::cmdPlayVideo  , this, _1),
			"Usage: playvideo <video>\nPlay the specified video");
	registerCommand("listsounds" , boost::bind(&Console::cmdListSounds , this, _1),
			"Usage: listsounds\nList all available sounds");
	registerCommand("playsound"  , boost::bind(&Console::cmdPlaySound  , this, _1),
			"Usage: playsound <sound>\nPlay the specified sound");
	registerCommand("silence"    , boost::bind(&Console::cmdSilence    , this, _1),
			"Usage: silence\nStop all playing sounds and music");

	_console->setPrompt(kPrompt);

	_console->print("Console ready...");
}

Console::~Console() {
	hide();

	delete _console;
	delete _readLine;
}

void Console::show() {
	if (_visible)
		return;

	if (_neverShown)
		_console->print("Type 'exit' to return to the game. Type 'help' for a list of commands.");

	_console->show();
	_visible    = true;
	_neverShown = false;

	updateCaches();
	showCallback();
}

void Console::hide() {
	if (!_visible)
		return;

	_console->hide();
	_visible = false;
}

bool Console::isVisible() const {
	return _visible;
}

glm::vec2 Console::getSize() const {
	return _console->getContentSize();
}

uint32 Console::getLines() const {
	return _console->getLines();
}

uint32 Console::getColumns() const {
	return _console->getColumns();
}

bool Console::processEvent(Events::Event &event) {
	if (!isVisible())
		return false;

	if ((event.type == Events::kEventMouseDown) &&
		   ((event.button.button != SDL_BUTTON_WHEELUP) &&
		    (event.button.button != SDL_BUTTON_WHEELDOWN))) {

		const uint8 button     = event.button.button;
		const uint8 pasteMask1 = SDL_BUTTON_MMASK;
		const uint8 pasteMask2 = SDL_BUTTON_LMASK | SDL_BUTTON_RMASK;

		if (((button & pasteMask1) == pasteMask1) || ((button & pasteMask2) == pasteMask2)) {
			_readLine->addInput(_console->getHighlight());
			_console->setInput(_readLine->getCurrentLine(),
					_readLine->getCursorPosition(), _readLine->getOverwrite());
			return true;
		}

		if (button & SDL_BUTTON_LMASK) {
			_console->startHighlight(glm::ivec2(event.button.x, event.button.y));
			return true;
		}
	}

	if ((event.type == Events::kEventMouseMove) &&
		   ((event.button.button != SDL_BUTTON_WHEELUP) &&
		    (event.button.button != SDL_BUTTON_WHEELDOWN))) {

		if (event.motion.state & SDL_BUTTON_LMASK) {
			_console->stopHighlight(glm::ivec2(event.button.x, event.button.y));
			return true;
		}
	}

	if ((event.type == Events::kEventMouseUp) &&
		   ((event.button.button != SDL_BUTTON_WHEELUP) &&
		    (event.button.button != SDL_BUTTON_WHEELDOWN))) {

		uint32 curTime = EventMan.getTimestamp();

		if (((curTime - _lastClickTime) < kDoubleClickTime) &&
		    (_lastClickButton == event.button.button) &&
		    (_lastClickX == event.button.x) && (_lastClickY == event.button.y))
			_lastClickCount = (_lastClickCount + 1) % 3;
		else
			_lastClickCount = 0;

		_lastClickButton = event.button.button;
		_lastClickTime   = curTime;
		_lastClickX      = event.button.x;
		_lastClickY      = event.button.y;

		if (event.button.button & SDL_BUTTON_LMASK) {
			if      (_lastClickCount == 0)
				_console->stopHighlight(glm::ivec2(event.button.x, event.button.y));
			else if (_lastClickCount == 1)
				_console->highlightWord(glm::ivec2(event.button.x, event.button.y));
			else if (_lastClickCount == 2)
				_console->highlightLine(glm::ivec2(event.button.x, event.button.y));

			return true;
		}

	}

	if (event.type == Events::kEventKeyDown) {
		_console->clearHighlight();

		if (event.key.keysym.sym != SDLK_TAB) {
			_tabCount = 0;
			_printedCompleteWarning = false;
		} else
			_tabCount++;

		if (event.key.keysym.sym == SDLK_ESCAPE) {
			hide();
			return true;
		}

		if ((event.key.keysym.sym == SDLK_l) && (event.key.keysym.mod & KMOD_CTRL)) {
			clear();
			return true;
		}

		if ((event.key.keysym.sym == SDLK_PAGEUP) && (event.key.keysym.mod & KMOD_SHIFT)) {
			_console->scrollUp(kConsoleLines / 2);
			return true;
		}

		if ((event.key.keysym.sym == SDLK_PAGEDOWN) && (event.key.keysym.mod & KMOD_SHIFT)) {
			_console->scrollDown(kConsoleLines / 2);
			return true;
		}

		if (event.key.keysym.sym == SDLK_PAGEUP) {
			_console->scrollUp();
			return true;
		}

		if (event.key.keysym.sym == SDLK_PAGEDOWN) {
			_console->scrollDown();
			return true;
		}

		if ((event.key.keysym.sym == SDLK_HOME) && (event.key.keysym.mod & KMOD_SHIFT)) {
			_console->scrollTop();
			return true;
		}

		if ((event.key.keysym.sym == SDLK_END) && (event.key.keysym.mod & KMOD_SHIFT)) {
			_console->scrollBottom();
			return true;
		}


	} else if (event.type == Events::kEventMouseDown) {
		if (event.button.button == SDL_BUTTON_WHEELUP) {
			_console->scrollUp();
			return true;
		}

		if (event.button.button == SDL_BUTTON_WHEELDOWN) {
			_console->scrollDown();
			return true;
		}
	}

	Common::UString command;
	if (!_readLine->processEvent(event, command))
		return false;

	_console->setInput(_readLine->getCurrentLine(),
			_readLine->getCursorPosition(), _readLine->getOverwrite());

	// Check whether we have tab-completion hints
	if (printHints(command))
		return true;

	execute(command);
	return true;
}

void Console::execute(const Common::UString &line) {
	if (line.empty())
		return;

	// Add the line to console
	_console->print(Common::UString(kPrompt) + " " + line);


	// Split command from redirect target

	Common::UString command;
	Common::UString redirect;
	line.split(line.findFirst('>'), command, redirect, true);

	command.trim();
	redirect.trim();


	// Split command from arguments

	CommandLine cl;

	command.split(command.findFirst(' '), cl.cmd, cl.args, true);

	cl.cmd.trim();
	cl.args.trim();


	// Find the command
	CommandMap::iterator cmd = _commands.find(cl.cmd);
	if (cmd == _commands.end()) {
		printf("Unknown command \"%s\". Type 'help' for a list of available commands.",
				cl.cmd.c_str());
		return;
	}

	// Set redirect
	if (!_console->setRedirect(redirect))
		return;

	// Execute
	_console->hidePrompt();
	cmd->second.callback(cl);
	_console->showPrompt();

	// Reset redirect
	_console->setRedirect();
}

bool Console::printHints(const Common::UString &command) {
	if (_tabCount < 2)
		return false;

	uint32 maxSize;
	uint32 count;
	const std::list<Common::UString> &hints = _readLine->getCompleteHint(maxSize, count);
	if (count == 0)
		return false;

	maxSize = MAX<uint32>(maxSize, 3) + 2;
	uint32 lineSize = getColumns() / maxSize;
	uint32 lines = count / lineSize;

	if (lines >= (kConsoleLines - 3)) {
		if (!_printedCompleteWarning)
			printf("%d completion candidates", count);

		_printedCompleteWarning = true;

		if (_tabCount < 4)
			return true;
	}

	_console->scrollBottom();
	_console->print(Common::UString(kPrompt) + " " + command);
	printList(hints, maxSize);

	_tabCount = 0;
	_printedCompleteWarning = false;

	return true;
}

void Console::clear() {
	_console->clear();
}

void Console::print(const Common::UString &line) {
	_console->print(line);
}

void Console::printf(const char *s, ...) {
	char buf[STRINGBUFLEN];
	va_list va;

	va_start(va, s);
	vsnprintf(buf, STRINGBUFLEN, s, va);
	va_end(va);

	print(buf);
}

void Console::printException(Common::Exception &e, const Common::UString &prefix) {
	Common::Exception::Stack &stack = e.getStack();

	if (stack.empty()) {
		print("FATAL ERROR");
		return;
	}

	printf("%s%s", prefix.c_str(), stack.top().c_str());

	stack.pop();

	while (!stack.empty()) {
		printf("'- Because: %s", stack.top().c_str());
		stack.pop();
	}
}

void Console::updateCaches() {
	updateVideos();
	updateSounds();
}

void Console::updateVideos() {
	_videos.clear();
	_maxSizeVideos = 0;

	std::list<Aurora::ResourceManager::ResourceID> videos;
	ResMan.getAvailableResources(Aurora::kResourceVideo, videos);

	for (std::list<Aurora::ResourceManager::ResourceID>::const_iterator v = videos.begin();
	     v != videos.end(); ++v) {

		_videos.push_back(v->name);

		_maxSizeVideos = MAX(_maxSizeVideos, _videos.back().size());
	}

	setArguments("playvideo", _videos);
}

void Console::updateSounds() {
	_sounds.clear();
	_maxSizeSounds = 0;

	std::list<Aurora::ResourceManager::ResourceID> sounds;
	ResMan.getAvailableResources(Aurora::kFileTypeWAV, sounds);

	for (std::list<Aurora::ResourceManager::ResourceID>::const_iterator s = sounds.begin();
	     s != sounds.end(); ++s) {

		_sounds.push_back(s->name);

		_maxSizeSounds = MAX(_maxSizeSounds, _sounds.back().size());
	}

	setArguments("playsound", _sounds);
}

void Console::cmdHelp(const CommandLine &cli) {
	if (cli.args.empty()) {
		printFullHelp();
		return;
	}

	printCommandHelp(cli.args);
}

void Console::cmdClear(const CommandLine &cl) {
	clear();
}

void Console::cmdExit(const CommandLine &cl) {
	hide();
}

void Console::cmdQuit(const CommandLine &cl) {
	print("Bye...");
	EventMan.requestQuit();
}

void Console::cmdDumpResList(const CommandLine &cl) {
	if (cl.args.empty()) {
		printCommandHelp(cl.cmd);
		return;
	}

	if (dumpResList(cl.args))
		printf("Dumped list of resources to file \"%s\"", cl.args.c_str());
	else
		printf("Failed dumping list of resources to file \"%s\"", cl.args.c_str());
}

void Console::cmdDumpRes(const CommandLine &cl) {
	if (cl.args.empty()) {
		printCommandHelp(cl.cmd);
		return;
	}

	if (dumpResource(cl.args))
		printf("Dumped resource \"%s\"", cl.args.c_str());
	else
		printf("Failed dumping resource \"%s\"", cl.args.c_str());
}

void Console::cmdDumpTGA(const CommandLine &cl) {
	if (cl.args.empty()) {
		printCommandHelp(cl.cmd);
		return;
	}

	if (dumpTGA(cl.args))
		printf("Dumped TGA \"%s\"", cl.args.c_str());
	else
		printf("Failed dumping TGA \"%s\"", cl.args.c_str());
}

void Console::cmdDump2DA(const CommandLine &cl) {
	if (cl.args.empty()) {
		printCommandHelp(cl.cmd);
		return;
	}

	if (dump2DA(cl.args))
		printf("Dumped 2DA \"%s\"", cl.args.c_str());
	else
		printf("Failed dumping 2DA \"%s\"", cl.args.c_str());
}

void Console::cmdDumpAll2DA(const CommandLine &cl) {
	std::list<Aurora::ResourceManager::ResourceID> twoda;
	ResMan.getAvailableResources(Aurora::kFileType2DA, twoda);

	std::list<Aurora::ResourceManager::ResourceID>::const_iterator t;
	for (t = twoda.begin(); t != twoda.end(); ++t) {
		if (dump2DA(t->name))
			printf("Dumped 2DA \"%s\"", t->name.c_str());
		else
			printf("Failed dumping 2DA \"%s\"", t->name.c_str());
	}
}

void Console::cmdListVideos(const CommandLine &cl) {
	updateVideos();
	printList(_videos, _maxSizeVideos);
}

void Console::cmdPlayVideo(const CommandLine &cl) {
	if (cl.args.empty()) {
		printCommandHelp(cl.cmd);
		return;
	}

	playVideo(cl.args);
}

void Console::cmdListSounds(const CommandLine &cl) {
	updateSounds();
	printList(_sounds, _maxSizeSounds);
}

void Console::cmdPlaySound(const CommandLine &cl) {
	if (cl.args.empty()) {
		printCommandHelp(cl.cmd);
		return;
	}

	playSound(cl.args, Sound::kSoundTypeSFX);
}

void Console::cmdSilence(const CommandLine &cl) {
	SoundMan.stopAll();
}

void Console::printCommandHelp(const Common::UString &cmd) {
	CommandMap::const_iterator c = _commands.find(cmd);
	if (c == _commands.end()) {
		printFullHelp();
		return;
	}

	print(c->second.help);
}

void Console::printFullHelp() {
	print("Available commands (help <command> for further help on each command):");

	uint32 maxSize = 0;
	std::list<Common::UString> commands;
	for (CommandMap::const_iterator c = _commands.begin(); c != _commands.end(); ++c) {
		commands.push_back(c->second.cmd);

		maxSize = MAX(maxSize, commands.back().size());
	}

	printList(commands, maxSize);
}

void Console::printList(const std::list<Common::UString> &list, uint32 maxSize) {
	const uint32 columns = getColumns();

	if (maxSize > 0)
		maxSize = MAX<uint32>(maxSize, 3);

	uint32 lineSize = 1;
	if      (maxSize >= (columns - 2))
		maxSize  = columns;
	else if (maxSize > 0)
		lineSize = columns / (maxSize + 2);

	std::list<Common::UString>::const_iterator l = list.begin();
	while (l != list.end()) {
		Common::UString line;

		for (uint32 i = 0; (i < lineSize) && (l != list.end()); i++, ++l) {
			Common::UString item = *l;

			uint32 itemSize = item.size();

			if (itemSize > maxSize) {
				item.truncate(maxSize - 3);
				item += "...";
				itemSize = maxSize;
			}

			uint32 pad = (maxSize + 2) - itemSize;
			while (pad-- > 0)
				item += ' ';

			line += item;
		}

		print(line);
	}

}

void Console::setArguments(const Common::UString &cmd,
		const std::list<Common::UString> &args) {

	_readLine->setArguments(cmd, args);
}

void Console::setArguments(const Common::UString &cmd) {
	_readLine->setArguments(cmd);
}

void Console::showCallback() {
}

bool Console::registerCommand(const Common::UString &cmd, const CommandCallback &callback,
                              const Common::UString &help) {


	std::pair<CommandMap::iterator, bool> result;

	result = _commands.insert(std::make_pair(cmd, Command()));
	if (!result.second)
		return false;

	result.first->second.cmd  = cmd;
	result.first->second.help = help;

	result.first->second.callback = callback;

	_readLine->addCommand(cmd);

	updateHelpArguments();

	return true;
}

void Console::updateHelpArguments() {
	std::list<Common::UString> commands;
	for (CommandMap::const_iterator c = _commands.begin(); c != _commands.end(); ++c)
		commands.push_back(c->second.cmd);

	_readLine->setArguments("help", commands);
}

} // End of namespace Engines
