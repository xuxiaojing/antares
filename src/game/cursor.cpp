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

#include "game/cursor.hpp"

#include <sfz/sfz.hpp>

#include "drawing/color.hpp"
#include "drawing/pix-table.hpp"
#include "game/globals.hpp"
#include "game/time.hpp"
#include "video/driver.hpp"

using sfz::Exception;
using sfz::format;
using std::unique_ptr;

namespace antares {

static const int kCursorBoundsSize = 16;
static const int kTimeout = 1000000;

Cursor::Cursor(): _sprite(500, GRAY) { }

void Cursor::draw() const {
    draw_at(VideoDriver::driver()->get_mouse());
}

void Cursor::draw_at(Point where) const {
    if (world.contains(where)) {
        where.offset(-_sprite.at(0).center().h, -_sprite.at(0).center().v);
        _sprite.at(0).sprite().draw(where.h, where.v);
    }
}

GameCursor::GameCursor():
        show(true),
        _show_crosshairs_until(now_usecs() + kTimeout) { }

bool GameCursor::active() const {
    return show && (_show_crosshairs_until > now_usecs());
}

Point GameCursor::clamped_location() {
    return clamp(VideoDriver::driver()->get_mouse());
}

Point GameCursor::clamp(Point p) {
    // Do the cursor, too, unless this is a replay.
    if (p.h > (viewport.right - kCursorBoundsSize - 1)) {
        p.h = viewport.right - kCursorBoundsSize - 1;
    }
    if (p.v < (viewport.top + kCursorBoundsSize)) {
        p.v = viewport.top + kCursorBoundsSize;
    } else if (p.v > (play_screen.bottom - kCursorBoundsSize - 1)) {
        p.v = play_screen.bottom - kCursorBoundsSize - 1;
    }
    return p;
}

void GameCursor::mouse_down(const MouseDownEvent& event) {
    if (event.where().h >= viewport.left) {
        wake();
    }
}

void GameCursor::mouse_up(const MouseUpEvent& event) {
    if (event.where().h >= viewport.left) {
        wake();
    }
}

void GameCursor::mouse_move(const MouseMoveEvent& event) {
    if (event.where().h >= viewport.left) {
        wake();
    }
}

void GameCursor::wake() {
    _show_crosshairs_until = now_usecs() + kTimeout;
}

bool HintLine::show_hint_line = false;
Point HintLine::hint_line_start;
Point HintLine::hint_line_end;
RgbColor HintLine::hint_line_color;
RgbColor HintLine::hint_line_color_dark;

void HintLine::show(Point fromWhere, Point toWhere, uint8_t color, uint8_t brightness) {
    hint_line_start = fromWhere;
    hint_line_end = toWhere;
    show_hint_line = true;

    hint_line_color = GetRGBTranslateColorShade(color, brightness);
    hint_line_color_dark = GetRGBTranslateColorShade(color, VERY_DARK);
}

void HintLine::hide() {
    show_hint_line = false;
}

void HintLine::reset() {
    show_hint_line = false;
    hint_line_start.h = hint_line_start.v = -1;
    hint_line_end.h = hint_line_end.v = -1;
    hint_line_color = hint_line_color_dark = RgbColor::kBlack;
}

void GameCursor::draw() const {
    if (!show) {
        return;
    }

    Point where = VideoDriver::driver()->get_mouse();

    where = clamp(where);
    if (active()) {
        const Rect clip_rect = viewport;
        const RgbColor color = GetRGBTranslateColorShade(SKY_BLUE, MEDIUM);

        Point top_a = Point(where.h, clip_rect.top);
        Point top_b = Point(where.h, (where.v - kCursorBoundsSize));
        Point bottom_a = Point(where.h, (where.v + kCursorBoundsSize));
        Point bottom_b = Point(where.h, clip_rect.bottom - 1);
        Point left_a = Point(clip_rect.left, where.v);
        Point left_b = Point((where.h - kCursorBoundsSize), where.v);
        Point right_a = Point(std::max(viewport.left, where.h + kCursorBoundsSize), where.v);
        Point right_b = Point(clip_rect.right - 1, where.v);

        if (top_a.h >= viewport.left) {
            VideoDriver::driver()->draw_line(top_a, top_b, color);
            VideoDriver::driver()->draw_line(bottom_a, bottom_b, color);
        }
        VideoDriver::driver()->draw_line(right_a, right_b, color);
        if (left_b.h >= viewport.left) {
            VideoDriver::driver()->draw_line(left_a, left_b, color);
        }
    }

    if (where.h < viewport.left) {
        draw_at(where);
    }
}

void HintLine::draw() {
    if (show_hint_line) {
        Point start = hint_line_start;
        Point end = hint_line_end;

        start.offset(0, 2);
        end.offset(0, 2);
        VideoDriver::driver()->draw_line(start, end, hint_line_color_dark);

        start.offset(0, -1);
        end.offset(0, -1);
        VideoDriver::driver()->draw_line(start, end, hint_line_color);

        start.offset(0, -1);
        end.offset(0, -1);
        VideoDriver::driver()->draw_line(start, end, hint_line_color);
    }
}

}  // namespace antares
