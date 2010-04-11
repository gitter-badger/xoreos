/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file events/types.h
 *  Basic event types.
 */

#include <SDL_events.h>

#ifndef EVENTS_TYPES_H
#define EVENTS_TYPES_H

namespace Events {

typedef SDL_Event Event;

/** Custom event types. */
enum EventType {
	kEventNone      = SDL_NOEVENT        , ///< Nothing.
	kEventKeyDown   = SDL_KEYDOWN        , ///< Keyboard key was pressed.
	kEventKeyUp     = SDL_KEYUP          , ///< Keyboard key was released.
	kEventMouseMove = SDL_MOUSEMOTION    , ///< Mouse was moved.
	kEventMouseDown = SDL_MOUSEBUTTONDOWN, ///< Mouse button was pressed.
	kEventMouseUp   = SDL_MOUSEBUTTONUP  , ///< Mouse button was released.
	kEventQuit      = SDL_QUIT           , ///< Application quit was requested.
	kEventResize    = SDL_VIDEORESIZE    , ///< Resize the window.
	kEventUserMIN   = SDL_USEREVENT - 1  , ///< For range checks.
	kEventITC       = SDL_USEREVENT      , ///< Inter-thread communication
	kEventUserMAX   = SDL_NUMEVENTS        ///< For range checks.
};

/** Specific type of the inter-thread communication. */
enum ITCEvent {
	kITCEventFullscreen = 0, ///< Request switching to fullscreen mode.
	kITCEventWindowed,       ///< Request switching to windowed mode.
	kITCEventResize,         ///< Request changing the display size.
	kITCEventMAX             ///< For range checks.
};

} // End of namespace Events

#endif // EVENTS_TYPES_H