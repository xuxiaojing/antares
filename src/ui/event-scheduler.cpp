// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2008-2012 The Antares Authors
//
// This file is part of Antares, a tactical space combat game.
//
// Antares is free software: you can redistribute it and/or modify it
// under the terms of the Lesser GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Antares is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Antares.  If not, see http://www.gnu.org/licenses/

#include "ui/event-scheduler.hpp"

#include <sfz/sfz.hpp>

#include "config/preferences.hpp"
#include "drawing/pix-map.hpp"
#include "game/time.hpp"
#include "math/geometry.hpp"
#include "ui/card.hpp"
#include "ui/event.hpp"

using sfz::Exception;
using std::greater;
using std::max;
using std::unique_ptr;

namespace antares {

EventScheduler::EventScheduler():
        _ticks(0),
        _event_tracker(true) { }

void EventScheduler::schedule_snapshot(int64_t at) {
    _snapshot_times.push_back(at);
    push_heap(_snapshot_times.begin(), _snapshot_times.end(), greater<int64_t>());
}

void EventScheduler::schedule_event(unique_ptr<Event> event) {
    _event_heap.emplace_back(std::move(event));
    push_heap(_event_heap.begin(), _event_heap.end(), is_later);
}

void EventScheduler::schedule_key(int32_t key, int64_t down, int64_t up) {
    schedule_event(unique_ptr<Event>(new KeyDownEvent(down, key)));
    schedule_event(unique_ptr<Event>(new KeyUpEvent(up, key)));
}

void EventScheduler::schedule_mouse(
        int button, const Point& where, int64_t down, int64_t up) {
    schedule_event(unique_ptr<Event>(new MouseDownEvent(down, button, where)));
    schedule_event(unique_ptr<Event>(new MouseUpEvent(up, button, where)));
}

void EventScheduler::loop(EventScheduler::MainLoop& loop) {
    while (!loop.done()) {
        int64_t at_usecs;
        const bool has_timer = loop.top()->next_timer(at_usecs);
        const int64_t at_ticks = at_usecs * 60 / 1000000;
        if (!_event_heap.empty() && (!has_timer || (_event_heap.front()->at() <= at_ticks))) {
            unique_ptr<Event> event;
            swap(event, _event_heap.front());
            pop_heap(_event_heap.begin(), _event_heap.end(), is_later);
            _event_heap.pop_back();
            advance_tick_count(loop, event->at());
            event->send(&_event_tracker);
            event->send(loop.top());
        } else {
            if (!has_timer) {
                throw Exception("Event heap empty and timer not set to fire.");
            }
            advance_tick_count(loop, max(_ticks + 1, at_ticks));
            loop.top()->fire_timer();
        }
    }
}

void EventScheduler::advance_tick_count(EventScheduler::MainLoop& loop, int64_t ticks) {
    if (loop.takes_snapshots() && have_snapshots_before(ticks)) {
        loop.draw();
        while (have_snapshots_before(ticks)) {
            _ticks = _snapshot_times.front();
            loop.snapshot(_ticks);
            pop_heap(_snapshot_times.begin(), _snapshot_times.end(), greater<int64_t>());
            _snapshot_times.pop_back();
        }
    }
    _ticks = ticks;
}

bool EventScheduler::have_snapshots_before(int64_t ticks) const {
    return !_snapshot_times.empty() && (_snapshot_times.front() < ticks);
}

bool EventScheduler::is_later(const unique_ptr<Event>& x, const unique_ptr<Event>& y) {
    return x->at() > y->at();
}

}  // namespace antares
