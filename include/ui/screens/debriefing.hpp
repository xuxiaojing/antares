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

#ifndef ANTARES_UI_SCREENS_DEBRIEFING_HPP_
#define ANTARES_UI_SCREENS_DEBRIEFING_HPP_

#include <sfz/sfz.hpp>

#include "drawing/styled-text.hpp"
#include "ui/card.hpp"

namespace antares {

class PixMap;
class Sprite;

class DebriefingScreen : public Card {
  public:
    DebriefingScreen(int text_id);

    DebriefingScreen(
            int text_id, int your_length, int par_length, int your_loss, int par_loss,
            int your_kill, int par_kill);

    virtual void become_front();
    virtual void resign_front();
    virtual void draw() const;

    virtual void mouse_down(const MouseDownEvent& event);
    virtual void key_down(const KeyDownEvent& event);
    virtual void gamepad_button_down(const GamepadButtonDownEvent& event);

    virtual bool next_timer(int64_t& time);
    virtual void fire_timer();

    static sfz::String build_score_text(
            int your_length, int par_length, int your_loss, int par_loss, int your_kill,
            int par_kill);
    
  private:
    LabeledRect initialize(int text_id, bool do_score);

    enum State {
        TYPING,
        DONE,
    };
    friend void print_to(sfz::PrintTarget out, State state);
    State _state;

    sfz::String _message;
    std::unique_ptr<StyledText> _score;
    Rect _pix_bounds;
    Rect _message_bounds;
    Rect _score_bounds;
    std::unique_ptr<PixMap> _pix;
    std::unique_ptr<Sprite> _sprite;

    int64_t _next_update;
    int _typed_chars;

    LabeledRect _data_item;

    DISALLOW_COPY_AND_ASSIGN(DebriefingScreen);
};

void print_to(sfz::PrintTarget out, DebriefingScreen::State state);

}  // namespace antares

#endif  // ANTARES_UI_SCREENS_DEBRIEFING_HPP_
