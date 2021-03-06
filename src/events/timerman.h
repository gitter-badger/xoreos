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

/** @file events/timerman.h
 *  The global timer manager.
 */

#ifndef EVENTS_TIMERMAN_H
#define EVENTS_TIMERMAN_H

#include <SDL_timer.h>

#include <list>

#include <boost/function.hpp>

#include "common/types.h"
#include "common/singleton.h"
#include "common/mutex.h"

#include "events/types.h"

namespace Events {

/** A timer callback function.
 *
 *  Its argument is the current calling interval.
 *  The return value is the new interval. 0 means the timer is stopped.
 */
typedef boost::function<uint32 (uint32)> TimerFunc;

class TimerID;
class TimerHandle;

/** The global timer manager.
 *
 *  Allows registering functions to be called at specific intervals.
 */
class TimerManager : public Common::Singleton<TimerManager> {
public:
	TimerManager();
	~TimerManager();

	void init();

	/** Add a function to be called regularily.
	 *
	 *  @param interval The interval in ms. The granularity is
	 *                  platform-dependent. The most common number is 10ms.
	 *  @param handle The timer handle to use.
	 *  @param func The function to call.
	 */
	void addTimer(uint32 interval, TimerHandle &handle, const TimerFunc &func);

	/** Remove that timer function. */
	void removeTimer(TimerHandle &handle);

private:
	Common::Mutex _mutex;

	std::list<TimerID> _timers;

	void removeTimer(TimerID &id);

	static uint32 timerCallback(uint32 interval, void *data);
};

class TimerID {
private:
	SDL_TimerID _id;
	TimerFunc _func;

	friend class TimerManager;
};

class TimerHandle {
public:
	TimerHandle();
	~TimerHandle();

private:
	bool _empty;

	std::list<TimerID>::iterator _iterator;

	friend class TimerManager;
};

} // End of namespace Events

/** Shortcut for accessing the timer manager. */
#define TimerMan Events::TimerManager::instance()

#endif // EVENTS_TIMERMAN_H
