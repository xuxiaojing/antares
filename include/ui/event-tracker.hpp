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

#ifndef ANTARES_UI_EVENT_TRACKER_HPP_
#define ANTARES_UI_EVENT_TRACKER_HPP_

#include "config/keys.hpp"
#include "math/geometry.hpp"
#include "ui/event.hpp"

namespace antares {

class EventTracker : public EventReceiver {
  public:
    EventTracker(bool strict):
            _strict(strict),
            _button{},
            _mouse(-1, -1) { }

    virtual void key_down(const KeyDownEvent& event);
    virtual void key_up(const KeyUpEvent& event);
    virtual void mouse_down(const MouseDownEvent& event);
    virtual void mouse_up(const MouseUpEvent& event);
    virtual void mouse_move(const MouseMoveEvent& event);

    bool button(int which) const { return _button[which]; }
    const Point& mouse() const { return _mouse; }
    const KeyMap& keys() const { return _keys; }

  private:
    const bool _strict;
    bool _button[3];
    Point _mouse;
    KeyMap _keys;

    DISALLOW_COPY_AND_ASSIGN(EventTracker);
};

}  // namespace antares

#endif  // ANTARES_UI_EVENT_TRACKER_HPP_
