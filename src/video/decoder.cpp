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

/** @file video/decoder.cpp
 *  Generic video decoder interface.
 */

#include <cassert>

#include "common/error.h"
#include "common/stream.h"
#include "common/threads.h"

#include "graphics/graphics.h"

#include "graphics/images/surface.h"

#include "video/decoder.h"

#include "sound/sound.h"
#include "sound/audiostream.h"
#include "sound/decoders/pcm.h"

namespace Video {

VideoDecoder::VideoDecoder() : Renderable(Graphics::kRenderableTypeVideo),
	_started(false), _finished(false), _needCopy(false),
	_size(0, 0), _surface(0), _texture(0),
	_textureWidth(0.0), _textureHeight(0.0), _scale(kScaleNone),
	_sound(0), _soundRate(0), _soundFlags(0) {

}

VideoDecoder::~VideoDecoder() {
	deinit();

	if (_texture != 0)
		GfxMan.abandon(&_texture, 1);

	delete _surface;

	deinitSound();
}

void VideoDecoder::deinit() {
	hide();

	GLContainer::removeFromQueue(Graphics::kQueueGLContainer);
}

void VideoDecoder::initVideo(const glm::uvec2 &size) {
	_size = size;

	// The real texture dimensions. Have to be a power of 2
	int realWidth  = NEXTPOWER2(size.x);
	int realHeight = NEXTPOWER2(size.y);

	// Dimensions of the actual video part of texture
	_textureWidth  = ((float) size.x) / ((float) realWidth );
	_textureHeight = ((float) size.y) / ((float) realHeight);

	delete _surface;
	_surface = new Graphics::Surface(glm::ivec2(realWidth, realHeight));

	_surface->fill(0, 0, 0, 0);

	rebuild();
}

void VideoDecoder::initSound(uint16 rate, int channels, bool is16) {
	deinitSound();

	_soundRate  = rate;
	_soundFlags = 0;

#ifdef XOREOS_LITTLE_ENDIAN
	_soundFlags |= Sound::FLAG_LITTLE_ENDIAN;
#endif

	if (is16)
		_soundFlags |= Sound::FLAG_16BITS;

	_sound = Sound::makeQueuingAudioStream(_soundRate, channels);

	_soundHandle = SoundMan.playAudioStream(_sound, Sound::kSoundTypeVideo, false);
}

void VideoDecoder::deinitSound() {
	if (!_sound)
		return;

	_sound->finish();
	SoundMan.triggerUpdate();

	SoundMan.stopChannel(_soundHandle);

	delete _sound;
	_sound = 0;
}

void VideoDecoder::queueSound(const byte *data, uint32 dataSize) {
	if (!_sound)
		return;

	assert(data && dataSize);

	Common::MemoryReadStream *dataStream = new Common::MemoryReadStream(data, dataSize, true);
	Sound::RewindableAudioStream *dataPCM = Sound::makePCMStream(dataStream, _soundRate, _soundFlags, _sound->getChannels());

	_sound->queueAudioStream(dataPCM);

	SoundMan.startChannel(_soundHandle);
}

void VideoDecoder::queueSound(Sound::AudioStream *stream) {
	if (!_sound)
		return;

	assert(stream);

	_sound->queueAudioStream(stream);

	SoundMan.startChannel(_soundHandle);
}

void VideoDecoder::finishSound() {
	if (_sound)
		_sound->finish();
}

uint32 VideoDecoder::getNumQueuedStreams() const {
	return _sound ? _sound->numQueuedStreams() : 0;
}

void VideoDecoder::doRebuild() {
	if (!_surface)
		return;

	// Generate the texture ID
	glGenTextures(1, &_texture);

	glBindTexture(GL_TEXTURE_2D, _texture);

	// Texture clamping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// No filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

	const glm::ivec2 surfaceSize = _surface->getSize();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surfaceSize.x, surfaceSize.y,
	             0, GL_BGRA, GL_UNSIGNED_BYTE, _surface->getData());
}

void VideoDecoder::doDestroy() {
	if (_texture == 0)
		return;

	glDeleteTextures(1, &_texture);

	_texture = 0;
}

void VideoDecoder::copyData() {
	if (!_needCopy)
		return;

	if (!_surface)
		throw Common::Exception("No video data while trying to copy");
	if (_texture == 0)
		throw Common::Exception("No texture while trying to copy");

	const glm::ivec2 surfaceSize = _surface->getSize();

	glBindTexture(GL_TEXTURE_2D, _texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surfaceSize.x, surfaceSize.y,
	                GL_BGRA, GL_UNSIGNED_BYTE, _surface->getData());

	_needCopy = false;
}

void VideoDecoder::setScale(Scale scale) {
	_scale = scale;
}

bool VideoDecoder::isPlaying() const {
	return !_finished || SoundMan.isPlaying(_soundHandle);
}

void VideoDecoder::update() {
	if (getTimeToNextFrame() > 0)
		return;

	processData();
	copyData();
}

glm::vec2 VideoDecoder::getQuadDimensions() const {
	if (_scale == kScaleNone)
		// No scaling requested
		return glm::vec2(_size);

	const glm::uvec2 screenSize = glm::uvec2(GfxMan.getScreenSize());

	if ((_scale == kScaleUp) && glm::all(glm::lessThanEqual(_size, screenSize)))
		// Only upscaling requested, but not necessary
		return glm::vec2(_size);

	if ((_scale == kScaleDown) && glm::all(glm::greaterThanEqual(_size, screenSize)))
		// Only downscaling requested, but not necessary
		return glm::vec2(_size);

	const float ratio = _size.x / _size.y;

	const glm::vec2 size = glm::vec2(screenSize.x, screenSize.x / ratio);
	if (size.y <= screenSize.y)
		return size;

  return glm::vec2(screenSize.y * ratio, screenSize.y);
}

void VideoDecoder::calculateDistance() {
}

void VideoDecoder::render(Graphics::RenderPass pass) {
	if (pass == Graphics::kRenderPassTransparent)
		return;

	if (!isPlaying() || !_started || (_texture == 0))
		return;

	// Process and copy the next frame data, if necessary
	update();

	// Get the dimensions of the video surface we want, depending on the scaling requested
	const glm::vec2 size = getQuadDimensions();

	// Create a textured quad with those dimensions
	const glm::vec2 hSize = size / 2.0f;

	glBindTexture(GL_TEXTURE_2D, _texture);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-hSize.x, -hSize.y, -1.0);
		glTexCoord2f(_textureWidth, 0.0);
		glVertex3f( hSize.x, -hSize.y, -1.0);
		glTexCoord2f(_textureWidth, _textureHeight);
		glVertex3f( hSize.x,  hSize.y, -1.0);
		glTexCoord2f(0.0, _textureHeight);
		glVertex3f(-hSize.x,  hSize.y, -1.0);
	glEnd();
}

void VideoDecoder::finish() {
	finishSound();

	_finished = true;
}

void VideoDecoder::start() {
	startVideo();

	show();
}

void VideoDecoder::abort() {
	hide();

	finish();
}

} // End of namespace Video
