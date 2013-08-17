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

/** @file graphics/graphics.cpp
 *  The global graphics manager.
 */

#include <boost/bind.hpp>

#include "common/version.h"
#include "common/util.h"
#include "common/maths.h"
#include "common/error.h"
#include "common/ustring.h"
#include "common/file.h"
#include "common/configman.h"
#include "common/threads.h"

#include "events/requests.h"
#include "events/events.h"
#include "events/notifications.h"

#include "graphics/graphics.h"
#include "graphics/util.h"
#include "graphics/cursor.h"
#include "graphics/fpscounter.h"
#include "graphics/queueman.h"
#include "graphics/glcontainer.h"
#include "graphics/renderable.h"
#include "graphics/camera.h"

#include "graphics/images/decoder.h"
#include "graphics/images/screenshot.h"

DECLARE_SINGLETON(Graphics::GraphicsManager)

namespace Graphics {

PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;

GraphicsManager::GraphicsManager() : _projection(), _projectionInv() {
	_ready = false;

	_needManualDeS3TC        = false;
	_supportMultipleTextures = false;

	_fullScreen = false;

	_fsaa    = 0;
	_fsaaMax = 0;

	_gamma = 1.0;

	_screen = 0;

	_fpsCounter = new FPSCounter(3);

	_frameLock = 0;

	_cursor = 0;
	_cursorState = kCursorStateStay;

	_takeScreenshot = false;

	_renderableID = 0;

	_hasAbandoned = false;

	_lastSampled = 0;

	glCompressedTexImage2D = 0;
}

GraphicsManager::~GraphicsManager() {
	deinit();

	delete _fpsCounter;
}

void GraphicsManager::init() {
	Common::enforceMainThread();

	uint32 sdlInitFlags = SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;

	// TODO: Is this actually needed on any systems? It seems to make MacOS X fail to
	//       receive any events, too.
/*
// Might be needed on unixoid OS, but it crashes Windows. Nice.
#ifndef WIN32
	sdlInitFlags |= SDL_INIT_EVENTTHREAD;
#endif
*/

	if (SDL_Init(sdlInitFlags) < 0)
		throw Common::Exception("Failed to initialize SDL: %s", SDL_GetError());

	// Set the window title to our name
	setWindowTitle(XOREOS_NAMEVERSION);

	glm::ivec2 size;
	size.x  = ConfigMan.getInt ("width"     , 800);
	size.y  = ConfigMan.getInt ("height"    , 600);
	bool fs = ConfigMan.getBool("fullscreen", false);

	initSize(size, fs);
	setupScene();

	// Try to change the FSAA settings to the config value
	if (_fsaa != ConfigMan.getInt("fsaa"))
		if (!setFSAA(ConfigMan.getInt("fsaa")))
			// If that fails, set the config to the current level
			ConfigMan.setInt("fsaa", _fsaa);

	// Set the gamma correction to what the config specifies
	if (ConfigMan.hasKey("gamma"))
		setGamma(ConfigMan.getDouble("gamma", 1.0));

	_ready = true;
}

void GraphicsManager::deinit() {
	Common::enforceMainThread();

	if (!_ready)
		return;

	QueueMan.clearAllQueues();

	SDL_Quit();

	_ready = false;

	_needManualDeS3TC        = false;
	_supportMultipleTextures = false;
}

bool GraphicsManager::ready() const {
	return _ready;
}

bool GraphicsManager::needManualDeS3TC() const {
	return _needManualDeS3TC;
}

bool GraphicsManager::supportMultipleTextures() const {
	return _supportMultipleTextures;
}

int GraphicsManager::getMaxFSAA() const {
	return _fsaaMax;
}

int GraphicsManager::getCurrentFSAA() const {
	return _fsaa;
}

uint32 GraphicsManager::getFPS() const {
	return _fpsCounter->getFPS();
}

void GraphicsManager::initSize(const glm::ivec2 &size, bool fullscreen) {
	int bpp = SDL_GetVideoInfo()->vfmt->BitsPerPixel;
	if ((bpp != 16) && (bpp != 24) && (bpp != 32))
		throw Common::Exception("Need 16, 24 or 32 bits per pixel");

	_systemSize.x = SDL_GetVideoInfo()->current_w;
	_systemSize.y = SDL_GetVideoInfo()->current_h;

	uint32 flags = SDL_OPENGL;

	_fullScreen = fullscreen;
	if (_fullScreen)
		flags |= SDL_FULLSCREEN;

	// The way we try to find an optimal color mode is a bit complex:
	// We only want 16bpp as a fallback, but otherwise prefer the native value.
	// So, if we're currently in 24bpp or 32bpp, we try that one first, then the
	// other one and 16bpp only as a last resort.
	// If we're currently in 16bpp mode, we try the higher two first as well,
	// before being okay with native 16bpp mode.

	const int colorModes[] = { bpp == 16 ? 32 : bpp, bpp == 24 ? 32 : 24, 16 };

	bool foundMode = false;
	for (int i = 0; i < ARRAYSIZE(colorModes); i++) {
		if (setupSDLGL(size, colorModes[i], flags)) {
			foundMode = true;
			break;
		}
	}

	if (!foundMode)
		throw Common::Exception("Failed setting the video mode: %s", SDL_GetError());

	// Initialize glew, for the extension entry points
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK)
		throw Common::Exception("Failed initializing glew: %s", glewGetErrorString(glewErr));

	// Check if we have all needed OpenGL extensions
	checkGLExtensions();
}

bool GraphicsManager::setFSAA(int level) {
	// Force calling it from the main thread
	if (!Common::isMainThread()) {
		Events::MainThreadFunctor<bool> functor(boost::bind(&GraphicsManager::setFSAA, this, level));

		return RequestMan.callInMainThread(functor);
	}

	if (_fsaa == level)
		// Nothing to do
		return true;

	// Check if we have the support for that level
	if (level > _fsaaMax)
		return false;

	// Backup the old level and set the new level
	int oldFSAA = _fsaa;
	_fsaa = level;

	destroyContext();

	// Set the multisample level
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, (_fsaa > 0) ? 1 : 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, _fsaa);

	uint32 flags = _screen->flags;

	// Now try to change the screen
	_screen = SDL_SetVideoMode(0, 0, 0, flags);

	if (!_screen) {
		// Failed changing, back up

		_fsaa = oldFSAA;

		// Set the multisample level
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, (_fsaa > 0) ? 1 : 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, _fsaa);
		_screen = SDL_SetVideoMode(0, 0, 0, flags);

		// There's no reason how this could possibly fail, but ok...
		if (!_screen)
			throw Common::Exception("Failed reverting to the old FSAA settings");
	}

	rebuildContext();

	return _fsaa == level;
}

int GraphicsManager::probeFSAA(const glm::ivec2 &size, int bpp, uint32 flags) {
	// Find the max supported FSAA level

	for (int i = 32; i >= 2; i >>= 1) {
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE    ,   8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE  ,   8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE   ,   8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE  ,   8);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,   1);

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, i);

		if (SDL_SetVideoMode(size.x, size.y, bpp, flags))
			return i;
	}

	return 0;
}

bool GraphicsManager::setupSDLGL(const glm::ivec2 &size, int bpp, uint32 flags) {
	_fsaaMax = probeFSAA(size, bpp, flags);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE    ,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE  ,   8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE   ,   8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE  ,   8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,   1);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

	_screen = SDL_SetVideoMode(size.x, size.y, bpp, flags);
	if (!_screen)
		return false;

	return true;
}

void GraphicsManager::checkGLExtensions() {
	if (!GLEW_EXT_texture_compression_s3tc) {
		warning("Your graphics card does not support the needed extension "
		        "for S3TC DXT1, DXT3 and DXT5 texture decompression");
		warning("Switching to manual S3TC DXTn decompression. "
		        "This will be slower and will take up more video memory");
		_needManualDeS3TC = true;
	}

	if (!_needManualDeS3TC) {
		// Make sure we use the right glCompressedTexImage2D function
		glCompressedTexImage2D = GLEW_GET_FUN(__glewCompressedTexImage2D) ?
			(PFNGLCOMPRESSEDTEXIMAGE2DPROC)GLEW_GET_FUN(__glewCompressedTexImage2D) :
			(PFNGLCOMPRESSEDTEXIMAGE2DPROC)GLEW_GET_FUN(__glewCompressedTexImage2DARB);

		if (!GLEW_ARB_texture_compression || !glCompressedTexImage2D) {
			warning("Your graphics card doesn't support the compressed texture API");
			warning("Switching to manual S3TC DXTn decompression. "
			        "This will be slower and will take up more video memory");

			_needManualDeS3TC = true;
		}
	}

	if (!GLEW_ARB_multitexture) {
		warning("Your graphics card does no support applying multiple textures onto "
		        "one surface");
		warning("Xoreos will only use one texture. Certain surfaces may look weird");

		_supportMultipleTextures = false;
	} else
		_supportMultipleTextures = true;
}

void GraphicsManager::setWindowTitle(const Common::UString &title) {
	SDL_WM_SetCaption(title.c_str(), 0);
}

float GraphicsManager::getGamma() const {
	return _gamma;
}

void GraphicsManager::setGamma(float gamma) {
	// Force calling it from the main thread
	if (!Common::isMainThread()) {
		Events::MainThreadFunctor<void> functor(boost::bind(&GraphicsManager::setGamma, this, gamma));

		return RequestMan.callInMainThread(functor);
	}

	_gamma = gamma;

	SDL_SetGamma(gamma, gamma, gamma);
}

void GraphicsManager::setupScene() {
	if (!_screen)
		throw Common::Exception("No screen initialized");

	const glm::ivec2 screenSize = getScreenSize();

	glClearColor(0, 0, 0, 0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, screenSize.x, screenSize.y);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0, 0.0, 0.0, 0.5);
	glClearDepth(1.0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glAlphaFunc(GL_GREATER, 0.1);
	glEnable(GL_ALPHA_TEST);

	glEnable(GL_CULL_FACE);

	perspective(60.0, ((float) screenSize.x) / ((float) screenSize.y), 1.0, 1000.0);
}

void GraphicsManager::perspective(float fovy, float aspect, float zNear, float zFar) {
	const float f = 1.0 / (tanf(Common::deg2rad(fovy) / 2.0));

	const float t1 = (zFar + zNear) / (zNear - zFar);
	const float t2 = (2 * zFar * zNear) / (zNear - zFar);

	glm::column(_projection, 0) = glm::vec4(f / aspect, 0.0f, 0.0f,  0.0f);
	glm::column(_projection, 1) = glm::vec4(0.0f,          f, 0.0f,  0.0f);
	glm::column(_projection, 2) = glm::vec4(0.0f,       0.0f,   t1, -1.0f);
	glm::column(_projection, 3) = glm::vec4(0.0f,       0.0f,   t2,  0.0f);

	_projectionInv = glm::inverse(_projection);
}

bool GraphicsManager::project(const glm::vec3 &world, glm::vec3 &screen) {
	// This is our projection matrix
	glm::mat4 proj = _projection;


	// Generate the model matrix
	glm::mat4 model = glm::mat4();

	CameraMan.lock();
	glm::vec3 cPos    = -CameraMan.getPosition();
	glm::vec3 cOrient = -CameraMan.getOrientation();
	CameraMan.unlock();

	cOrient.y = -cOrient.y;
	cPos.z    = -cPos.z;

	// Apply camera orientation
	model = glm::rotate(model, cOrient.x, glm::vec3(1.0, 0.0, 0.0));
	model = glm::rotate(model, cOrient.y, glm::vec3(0.0, 1.0, 0.0));
	model = glm::rotate(model, cOrient.z, glm::vec3(0.0, 0.0, 1.0));

	// Apply camera position
	model = glm::translate(model, cPos);


	// Generate a matrix for the coordinates
	glm::vec4 coords = glm::vec4(world, 1.0f);


	// Multiply them
	glm::vec4 v = proj * model * coords;


	// Projection divide
	if (v.w == 0.0)
		return false;

	v /= v.w;

	// Viewport coordinates
	const glm::vec4 view = glm::vec4(0.0f, 0.0f, getScreenSize());

	screen.x = view.x + view.z * (v.x + 1.0) / 2.0;
	screen.y = view.y + view.w * (v.y + 1.0) / 2.0;
	screen.z =                   (v.z + 1.0) / 2.0;

	screen.x -= view.z / 2.0;
	screen.y -= view.w / 2.0;
	return true;
}

bool GraphicsManager::unproject(const glm::vec2 &screen, std::pair<glm::vec3, glm::vec3> &line) const {
	try {
		// Generate the inverse of the model matrix

		glm::mat4 model;

		CameraMan.lock();
		glm::vec3 cPos    = CameraMan.getPosition();
		glm::vec3 cOrient = CameraMan.getOrientation();
		CameraMan.unlock();

		cOrient.y = -cOrient.y;
		cPos.z    = -cPos.z;

		// Apply camera position
		model = glm::translate(model, cPos);

		// Apply camera orientation
		model = glm::rotate(model, cOrient.x, glm::vec3(0.0, 0.0, 1.0));
		model = glm::rotate(model, cOrient.y, glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, cOrient.z, glm::vec3(1.0, 0.0, 0.0));


		// Multiply with the inverse of our projection matrix
		model *= _projectionInv;


		// Viewport coordinates

		const glm::vec4 view = glm::vec4(0.0f, 0.0f, getScreenSize());

		float zNear = 0.0;
		float zFar  = 1.0;


		// Generate a matrix for the coordinates at the near plane

		glm::vec4 coordsNear;

		coordsNear.x = (2 * (screen.x - view.x) / view.z) - 1.0;
		coordsNear.y = (2 * (screen.y - view.y) / view.w) - 1.0;
		coordsNear.z = (2 * zNear) - 1.0;
		coordsNear.w = 1.0;



		// Generate a matrix for the coordinates at the far plane

		glm::vec4 coordsFar;

		coordsFar.x = (2 * (screen.x - view.x) / view.z) - 1.0;
		coordsFar.y = (2 * (screen.y - view.y) / view.w) - 1.0;
		coordsFar.z = (2 * zFar) - 1.0;
		coordsFar.w = 1.0;


		// Unproject
		glm::vec4 oNear = model * coordsNear;
		glm::vec4 oFar  = model * coordsFar;
		if ((oNear.w == 0.0) || (oFar.w == 0.0))
			return false;


		// And return the values

		oNear /= oNear.w;

		line.first = glm::vec3(oNear);

		oFar /= oFar.w;

		line.second = glm::vec3(oFar);

	} catch (Common::Exception &e) {
		Common::printException(e, "WARNING: ");
		return false;
	} catch (...) {
		return false;
	}

	return true;
}

void GraphicsManager::lockFrame() {
	Common::StackLock frameLock(_frameLockMutex);

	_frameLock++;
}

void GraphicsManager::unlockFrame() {
	Common::StackLock frameLock(_frameLockMutex);

	assert(_frameLock != 0);

	_frameLock--;
}

void GraphicsManager::recalculateObjectDistances() {
	// World objects
	QueueMan.lockQueue(kQueueVisibleWorldObject);

	const std::list<Queueable *> &objects = QueueMan.getQueue(kQueueVisibleWorldObject);
	for (std::list<Queueable *>::const_iterator o = objects.begin(); o != objects.end(); ++o)
		static_cast<Renderable *>(*o)->calculateDistance();

	QueueMan.sortQueue(kQueueVisibleWorldObject);
	QueueMan.unlockQueue(kQueueVisibleWorldObject);

	// GUI front objects
	QueueMan.lockQueue(kQueueVisibleGUIFrontObject);

	const std::list<Queueable *> &gui = QueueMan.getQueue(kQueueVisibleGUIFrontObject);
	for (std::list<Queueable *>::const_iterator g = gui.begin(); g != gui.end(); ++g)
		static_cast<Renderable *>(*g)->calculateDistance();

	QueueMan.sortQueue(kQueueVisibleGUIFrontObject);
	QueueMan.unlockQueue(kQueueVisibleGUIFrontObject);
}

uint32 GraphicsManager::createRenderableID() {
	Common::StackLock lock(_renderableIDMutex);

	return ++_renderableID;
}

void GraphicsManager::abandon(TextureID *ids, uint32 count) {
	if (count == 0)
		return;

	Common::StackLock lock(_abandonMutex);

	_abandonTextures.reserve(_abandonTextures.size() + count);
	while (count-- > 0)
		_abandonTextures.push_back(*ids++);

	_hasAbandoned = true;
}

void GraphicsManager::abandon(ListID ids, uint32 count) {
	if (count == 0)
		return;

	Common::StackLock lock(_abandonMutex);

	while (count-- > 0)
		_abandonLists.push_back(ids++);

	_hasAbandoned = true;
}

void GraphicsManager::setCursor(Cursor *cursor) {
	lockFrame();

	_cursor = cursor;

	unlockFrame();
}

void GraphicsManager::takeScreenshot() {
	lockFrame();

	_takeScreenshot = true;

	unlockFrame();
}

Renderable *GraphicsManager::getGUIObjectAt(const glm::vec2 &screen) const {
	if (QueueMan.isQueueEmpty(kQueueVisibleGUIFrontObject))
		return 0;

	// Map the screen coordinates to our OpenGL GUI screen coordinates
	glm::vec2 point = screen - glm::vec2(getScreenSize()) / 2.0f;
	point.y = -point.y;

	Renderable *object = 0;

	QueueMan.lockQueue(kQueueVisibleGUIFrontObject);
	const std::list<Queueable *> &gui = QueueMan.getQueue(kQueueVisibleGUIFrontObject);

	// Go through the GUI elements, from nearest to furthest
	for (std::list<Queueable *>::const_iterator g = gui.begin(); g != gui.end(); ++g) {
		Renderable &r = static_cast<Renderable &>(**g);

		if (!r.isClickable())
			// Object isn't clickable, don't check
			continue;

		// If the coordinates are "in" that object, return it
		if (r.isIn(point)) {
			object = &r;
			break;
		}
	}

	QueueMan.unlockQueue(kQueueVisibleGUIFrontObject);
	return object;
}

Renderable *GraphicsManager::getWorldObjectAt(const glm::vec2 &screen) const {
	if (QueueMan.isQueueEmpty(kQueueVisibleWorldObject))
		return 0;

	// Map the screen coordinates to OpenGL world screen coordinates
	glm::vec2 point = screen;
	point.y = getScreenSize().y - point.y;

	std::pair<glm::vec3, glm::vec3> line;
	if (!unproject(point, line))
		return 0;

	Renderable *object = 0;

	QueueMan.lockQueue(kQueueVisibleWorldObject);
	const std::list<Queueable *> &objects = QueueMan.getQueue(kQueueVisibleWorldObject);

	for (std::list<Queueable *>::const_iterator o = objects.begin(); o != objects.end(); ++o) {
		Renderable &r = static_cast<Renderable &>(**o);

		if (!r.isClickable())
			// Object isn't clickable, don't check
			continue;

		// If the line intersects with the object, return it
		if (r.isIn(line)) {
			object = &r;
			break;
		}
	}

	QueueMan.unlockQueue(kQueueVisibleWorldObject);
	return object;
}

Renderable *GraphicsManager::getObjectAt(const glm::vec2 &screen) {
	Renderable *object = 0;

	if ((object = getGUIObjectAt(screen)))
		return object;

	if ((object = getWorldObjectAt(screen)))
		return object;

	return 0;
}

void GraphicsManager::buildNewTextures() {
	QueueMan.lockQueue(kQueueNewTexture);
	const std::list<Queueable *> &text = QueueMan.getQueue(kQueueNewTexture);
	if (text.empty()) {
		QueueMan.unlockQueue(kQueueNewTexture);
		return;
	}

	for (std::list<Queueable *>::const_iterator t = text.begin(); t != text.end(); ++t)
		static_cast<GLContainer *>(*t)->rebuild();

	QueueMan.clearQueue(kQueueNewTexture);
	QueueMan.unlockQueue(kQueueNewTexture);
}

void GraphicsManager::beginScene() {
	// Switch cursor on/off
	if (_cursorState != kCursorStateStay)
		handleCursorSwitch();

	if (_fsaa > 0)
		glEnable(GL_MULTISAMPLE_ARB);

	// Clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
}

bool GraphicsManager::playVideo() {
	if (QueueMan.isQueueEmpty(kQueueVisibleVideo))
		return false;

	const glm::ivec2 screenSize = getScreenSize();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glScalef(2.0 / screenSize.x, 2.0 / screenSize.y, 0.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	QueueMan.lockQueue(kQueueVisibleVideo);
	const std::list<Queueable *> &videos = QueueMan.getQueue(kQueueVisibleVideo);

	for (std::list<Queueable *>::const_iterator v = videos.begin(); v != videos.end(); ++v) {
		glPushMatrix();
		static_cast<Renderable *>(*v)->render(kRenderPassAll);
		glPopMatrix();
	}

	QueueMan.unlockQueue(kQueueVisibleVideo);
	return true;
}

bool GraphicsManager::renderWorld() {
	if (QueueMan.isQueueEmpty(kQueueVisibleWorldObject))
		return false;

	CameraMan.lock();
	glm::vec3 cPos    = -CameraMan.getPosition();
	glm::vec3 cOrient = -CameraMan.getOrientation();
	CameraMan.unlock();

	cOrient.y = -cOrient.y;
	cPos.z    = -cPos.z;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMultMatrixf(glm::value_ptr(_projection));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Apply camera orientation
	glRotatef(cOrient.x, 1.0, 0.0, 0.0);
	glRotatef(cOrient.y, 0.0, 1.0, 0.0);
	glRotatef(cOrient.z, 0.0, 0.0, 1.0);

	// Apply camera position
	glTranslatef(cPos.x, cPos.y, cPos.z);

	QueueMan.lockQueue(kQueueVisibleWorldObject);
	const std::list<Queueable *> &objects = QueueMan.getQueue(kQueueVisibleWorldObject);

	buildNewTextures();

	// Get the current time
	uint32 now = EventMan.getTimestamp();
	if (_lastSampled == 0)
		_lastSampled = now;

	// Calc elapsed time
	float elapsedTime = (now - _lastSampled) / 1000.0f;
	_lastSampled = now;

	// If game paused, skip the advanceTime loop below

	// Advance time for animation queues
	for (std::list<Queueable *>::const_reverse_iterator o = objects.rbegin();
	     o != objects.rend(); ++o) {
		static_cast<Renderable *>(*o)->advanceTime(elapsedTime);
	}

	// Draw opaque objects
	for (std::list<Queueable *>::const_reverse_iterator o = objects.rbegin();
	     o != objects.rend(); ++o) {

		glPushMatrix();
		static_cast<Renderable *>(*o)->render(kRenderPassOpaque);
		glPopMatrix();
	}

	// Draw transparent objects
	for (std::list<Queueable *>::const_reverse_iterator o = objects.rbegin();
	     o != objects.rend(); ++o) {

		glPushMatrix();
		static_cast<Renderable *>(*o)->render(kRenderPassTransparent);
		glPopMatrix();
	}

	QueueMan.unlockQueue(kQueueVisibleWorldObject);
	return true;
}

bool GraphicsManager::renderGUIFront() {
	if (QueueMan.isQueueEmpty(kQueueVisibleGUIFrontObject))
		return false;

	const glm::ivec2 screenSize = getScreenSize();

	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glScalef(2.0 / screenSize.x, 2.0 / screenSize.y, 0.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	QueueMan.lockQueue(kQueueVisibleGUIFrontObject);
	const std::list<Queueable *> &gui = QueueMan.getQueue(kQueueVisibleGUIFrontObject);

	buildNewTextures();

	for (std::list<Queueable *>::const_reverse_iterator g = gui.rbegin();
	     g != gui.rend(); ++g) {

		glPushMatrix();
		static_cast<Renderable *>(*g)->render(kRenderPassAll);
		glPopMatrix();
	}

	QueueMan.unlockQueue(kQueueVisibleGUIFrontObject);

	glEnable(GL_DEPTH_TEST);
	return true;
}

bool GraphicsManager::renderCursor() {
	if (!_cursor)
		return false;

	buildNewTextures();

	const glm::ivec2 screenSize = getScreenSize();

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glScalef(2.0 / screenSize.x, 2.0 / screenSize.y, 0.0);
	glTranslatef(- (screenSize.x / 2.0), screenSize.y / 2.0, 0.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	_cursor->render();
	glEnable(GL_DEPTH_TEST);
	return true;
}

void GraphicsManager::endScene() {
	SDL_GL_SwapBuffers();

	if (_takeScreenshot) {
		Graphics::takeScreenshot();
		_takeScreenshot = false;
	}

	_fpsCounter->finishedFrame();

	if (_fsaa > 0)
		glDisable(GL_MULTISAMPLE_ARB);
}

void GraphicsManager::renderScene() {
	Common::enforceMainThread();

	cleanupAbandoned();

	if (_frameLock > 0)
		return;

	beginScene();

	if (playVideo()) {
		endScene();
		return;
	}

	renderWorld();
	renderGUIFront();
	renderCursor();

	endScene();
}

glm::ivec2 GraphicsManager::getScreenSize() const {
	if (!_screen)
		return glm::ivec2();

	return glm::ivec2(_screen->w, _screen->h);
}

glm::ivec2 GraphicsManager::getSystemSize() const {
	return _systemSize;
}

bool GraphicsManager::isFullScreen() const {
	return _fullScreen;
}

void GraphicsManager::rebuildGLContainers() {
	QueueMan.lockQueue(kQueueGLContainer);

	const std::list<Queueable *> &cont = QueueMan.getQueue(kQueueGLContainer);
	for (std::list<Queueable *>::const_iterator c = cont.begin(); c != cont.end(); ++c)
		static_cast<GLContainer *>(*c)->rebuild();

	QueueMan.unlockQueue(kQueueGLContainer);
}

void GraphicsManager::destroyGLContainers() {
	QueueMan.lockQueue(kQueueGLContainer);

	const std::list<Queueable *> &cont = QueueMan.getQueue(kQueueGLContainer);
	for (std::list<Queueable *>::const_iterator c = cont.begin(); c != cont.end(); ++c)
		static_cast<GLContainer *>(*c)->destroy();

	QueueMan.unlockQueue(kQueueGLContainer);
}

void GraphicsManager::destroyContext() {
	// Destroying all GL containers, since we need to
	// reload/rebuild them anyway when the context is recreated
	destroyGLContainers();
}

void GraphicsManager::rebuildContext() {
	// Reintroduce glew to the surface
	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK)
		throw Common::Exception("Failed initializing glew: %s", glewGetErrorString(glewErr));

	// Reintroduce OpenGL to the surface
	setupScene();

	// And reload/rebuild all GL containers
	rebuildGLContainers();

	// Wait for everything to settle
	RequestMan.sync();
}

void GraphicsManager::handleCursorSwitch() {
	Common::StackLock lock(_cursorMutex);

	if      (_cursorState == kCursorStateSwitchOn)
		SDL_ShowCursor(SDL_ENABLE);
	else if (_cursorState == kCursorStateSwitchOff)
		SDL_ShowCursor(SDL_DISABLE);

	_cursorState = kCursorStateStay;
}

void GraphicsManager::cleanupAbandoned() {
	if (!_hasAbandoned)
		return;

	Common::StackLock lock(_abandonMutex);

	if (!_abandonTextures.empty())
		glDeleteTextures(_abandonTextures.size(), &_abandonTextures[0]);

	for (std::list<ListID>::iterator l = _abandonLists.begin(); l != _abandonLists.end(); ++l)
		glDeleteLists(*l, 1);

	_abandonTextures.clear();
	_abandonLists.clear();

	_hasAbandoned = false;
}

void GraphicsManager::toggleFullScreen() {
	setFullScreen(!_fullScreen);
}

void GraphicsManager::setFullScreen(bool fullScreen) {
	if (_fullScreen == fullScreen)
		// Nothing to do
		return;

	// Force calling it from the main thread
	if (!Common::isMainThread()) {
		Events::MainThreadFunctor<void> functor(boost::bind(&GraphicsManager::setFullScreen, this, fullScreen));

		return RequestMan.callInMainThread(functor);
	}

	destroyContext();

	// Save the flags
	uint32 flags = _screen->flags;

	// Now try to change modes
	_screen = SDL_SetVideoMode(0, 0, 0, flags ^ SDL_FULLSCREEN);

	// If we could not go full screen, revert back.
	if (!_screen)
		_screen = SDL_SetVideoMode(0, 0, 0, flags);
	else
		_fullScreen = fullScreen;

	// There's no reason how this could possibly fail, but ok...
	if (!_screen)
		throw Common::Exception("Failed going to fullscreen and then failed reverting.");

	rebuildContext();
}

void GraphicsManager::toggleMouseGrab() {
	// Same as ScummVM's OSystem_SDL::toggleMouseGrab()
	if (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_OFF)
		SDL_WM_GrabInput(SDL_GRAB_ON);
	else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
}

void GraphicsManager::setScreenSize(const glm::ivec2 &size) {
	if (size == getScreenSize())
		// No changes, nothing to do
		return;

	// Force calling it from the main thread
	if (!Common::isMainThread()) {
		Events::MainThreadFunctor<void> functor(boost::bind(&GraphicsManager::setScreenSize, this, size));

		return RequestMan.callInMainThread(functor);
	}

	// Save properties
	uint32 flags       = _screen->flags;
	int    bpp         = _screen->format->BitsPerPixel;
	glm::ivec2 oldSize = getScreenSize();

	destroyContext();

	// Now try to change modes
	_screen = SDL_SetVideoMode(size.x, size.y, bpp, flags);

	if (!_screen) {
		// Could not change mode, revert back.
		_screen = SDL_SetVideoMode(oldSize.x, oldSize.y, bpp, flags);
	}

	// There's no reason how this could possibly fail, but ok...
	if (!_screen)
		throw Common::Exception("Failed changing the resolution and then failed reverting.");

	rebuildContext();

	// Let the NotificationManager notify the Notifyables that the resolution changed
	if (oldSize != getScreenSize())
		NotificationMan.resized(oldSize, getScreenSize());
}

void GraphicsManager::showCursor(bool show) {
	Common::StackLock lock(_cursorMutex);

	_cursorState = show ? kCursorStateSwitchOn : kCursorStateSwitchOff;
}

} // End of namespace Graphics
