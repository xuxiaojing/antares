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

#include "drawing/interface.hpp"

#include <sfz/sfz.hpp>

#include "config/keys.hpp"
#include "data/interface.hpp"
#include "drawing/color.hpp"
#include "drawing/styled-text.hpp"
#include "drawing/text.hpp"
#include "video/driver.hpp"

using sfz::BytesSlice;
using sfz::Exception;
using sfz::String;
using sfz::StringSlice;
using sfz::format;
using std::unique_ptr;
using std::vector;

namespace utf8 = sfz::utf8;

namespace antares {

namespace {

const int32_t kInterfaceLargeHBorder    = 13;
const int32_t kInterfaceSmallHBorder    = 3;
const int32_t kInterfaceVEdgeHeight     = 1;
const int32_t kInterfaceVCornerHeight   = 2;
const int32_t kInterfaceVLipHeight      = 1;
const int32_t kInterfaceHTop            = 2;
const int32_t kLabelBottomHeight        = 6;
const int32_t kInterfaceContentBuffer   = 2;

const int32_t kIndicatorVOffset         = 4;
const int32_t kRadioIndicatorHOffset    = 4;
const int32_t kCheckIndicatorHOffset    = 4;

const int32_t kMaxKeyNameLength         = 4;  // how many chars can be in name of key for plainButton

// DrawInterfaceString:
//  Relies on roman alphabet for upper/lower casing.  NOT WORLD-READY!

const Font* interface_font(interfaceStyleType style) {
    if ( style == kSmall) {
        return small_button_font;
    } else {
        return button_font;
    }
}

void DrawInterfaceString(
        Point p, StringSlice s, interfaceStyleType style, const RgbColor& color) {
    interface_font(style)->draw_sprite(p, s, color);
}

int16_t GetInterfaceStringWidth(const StringSlice& s, interfaceStyleType style) {
    return interface_font(style)->string_width(s);
}

// GetInterfaceFontWidth:       -- NOT WORLD-READY! --
//
//  We're not using fontInfo.widMax because we know we're never going to use the ultra-wide
//  characters like &oelig; and the like, and we're not using a mono-spaced font.  Therefore, we're
//  using the width of 'R' which is about as wide as our normal letters get.
//

int16_t GetInterfaceFontWidth(interfaceStyleType style) {
    return interface_font(style)->logicalWidth;
}

int16_t GetInterfaceFontHeight(interfaceStyleType style) {
    return interface_font(style)->height;
}

int16_t GetInterfaceFontAscent( interfaceStyleType style) {
    return interface_font(style)->ascent;
}

enum inlineKindType {
    kNoKind = 0,
    kVPictKind = 1,
    kVClearPictKind = 2
};

inline void mDrawPuffUpRect(Rect r, uint8_t mcolor, int mshade) {
    const RgbColor color = GetRGBTranslateColorShade(mcolor, mshade);
    VideoDriver::driver()->fill_rect(r, color);
    const RgbColor lighter = GetRGBTranslateColorShade(mcolor, mshade + kLighterColor);
    VideoDriver::driver()->fill_rect(Rect(r.left, r.top, r.left + 1, r.bottom), lighter);
    VideoDriver::driver()->fill_rect(Rect(r.left, r.top, r.right - 1, r.top + 1), lighter);
    const RgbColor darker = GetRGBTranslateColorShade(mcolor, mshade + kDarkerColor);
    VideoDriver::driver()->fill_rect(Rect(r.right - 1, r.top, r.right, r.bottom), darker);
    VideoDriver::driver()->fill_rect(Rect(r.left + 1, r.bottom - 1, r.right, r.bottom), darker);
}

inline void mDrawPuffDownRect(Rect r, uint8_t mcolor, int mshade) {
    VideoDriver::driver()->fill_rect(r, RgbColor::kBlack);
    const RgbColor darker = GetRGBTranslateColorShade(mcolor, mshade + kDarkerColor);
    VideoDriver::driver()->fill_rect(
            Rect(r.left - 1, r.top - 1, r.left, r.bottom + 1), darker);
    VideoDriver::driver()->fill_rect(
            Rect(r.left - 1, r.top - 1, r.right, r.top), darker);
    const RgbColor lighter = GetRGBTranslateColorShade(mcolor, mshade + kLighterColor);
    VideoDriver::driver()->fill_rect(
            Rect(r.right, r.top - 1, r.right + 1, r.bottom + 1), lighter);
    VideoDriver::driver()->fill_rect(
            Rect(r.left, r.bottom, r.right + 1, r.bottom + 1), lighter);
}

inline void mDrawPuffUpTopBorder(Rect r, uint8_t hue, int shade, int h_border) {
    // For historical reasons, this function assumes r has closed intervals.
    ++r.right;
    ++r.bottom;

    Rect outer(
            r.left - h_border, r.top - kInterfaceVEdgeHeight - kInterfaceVCornerHeight,
            r.right + h_border, r.top - kInterfaceVLipHeight);
    const RgbColor color = GetRGBTranslateColorShade(hue, shade);
    VideoDriver::driver()->fill_rect(Rect(outer.left, outer.top, r.left, r.top), color);
    VideoDriver::driver()->fill_rect(Rect(r.right, outer.top, outer.right, r.top), color);
    VideoDriver::driver()->fill_rect(Rect(r.left, outer.top, r.right, outer.bottom), color);

    const RgbColor darker = GetRGBTranslateColorShade(hue, shade + kDarkerColor);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, r.top, r.left + 1, r.top + 1), darker);
    VideoDriver::driver()->fill_rect(
            Rect(r.left, outer.bottom, r.right, outer.bottom + 1), darker);
    VideoDriver::driver()->fill_rect(
            Rect(r.right - 1, r.top, outer.right, r.top + 1), darker);
    VideoDriver::driver()->fill_rect(
            Rect(outer.right - 1, outer.top + 1, outer.right, r.top), darker);

    const RgbColor lighter = GetRGBTranslateColorShade(hue, shade + kLighterColor);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, outer.top, outer.left + 1, r.top), lighter);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, outer.top, outer.right, outer.top + 1), lighter);
}

inline void mDrawPuffUpBottomBorder(Rect r, uint8_t hue, int shade, int h_border) {
    // For historical reasons, this function assumes r has closed intervals.
    ++r.right;
    ++r.bottom;

    Rect outer(
            r.left - h_border, r.bottom + kInterfaceVLipHeight,
            r.right + h_border, r.bottom + kInterfaceVEdgeHeight + kInterfaceVCornerHeight);

    const RgbColor color = GetRGBTranslateColorShade(hue, shade);
    VideoDriver::driver()->fill_rect(Rect(outer.left, r.bottom, r.left, outer.bottom), color);
    VideoDriver::driver()->fill_rect(Rect(r.right, r.bottom, outer.right, outer.bottom), color);
    VideoDriver::driver()->fill_rect(Rect(r.left, outer.top, r.right, outer.bottom), color);

    const RgbColor lighter = GetRGBTranslateColorShade(hue, shade + kLighterColor);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, r.bottom - 1, outer.left + 1, outer.bottom), lighter);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, r.bottom - 1, r.left + 1, r.bottom), lighter);
    VideoDriver::driver()->fill_rect(
            Rect(r.left, outer.top - 1, r.right, outer.top), lighter);
    VideoDriver::driver()->fill_rect(
            Rect(r.right - 1, r.bottom - 1, outer.right, r.bottom), lighter);

    const RgbColor darker = GetRGBTranslateColorShade(hue, shade + kDarkerColor);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left + 1, outer.bottom - 1, outer.right, outer.bottom), darker);
    VideoDriver::driver()->fill_rect(
            Rect(outer.right - 1, r.bottom - 1, outer.right, outer.bottom), darker);
}

inline void mDrawPuffUpTBorder(Rect r, uint8_t mcolor, int mshade, int msheight, int h_border) {
    ++r.right;
    ++r.bottom;

    const RgbColor color = GetRGBTranslateColorShade(mcolor, mshade);
    VideoDriver::driver()->fill_rect(Rect(
                r.left - h_border, r.top + msheight, r.left + 1,
                r.top + msheight + kLabelBottomHeight + 1), color);
    VideoDriver::driver()->fill_rect(Rect(
                r.right - 1, r.top + msheight, r.right + h_border,
                r.top + msheight + kLabelBottomHeight + 1), color);
    VideoDriver::driver()->fill_rect(Rect(
                r.left, r.top + msheight + kInterfaceVLipHeight, r.right,
                r.top + msheight + kLabelBottomHeight - kInterfaceVLipHeight + 1), color);

    const RgbColor lighter = GetRGBTranslateColorShade(mcolor, mshade + kLighterColor);
    VideoDriver::driver()->fill_rect(Rect(
                r.left - h_border, r.top + msheight, r.left - h_border + 1,
                r.top + msheight + kLabelBottomHeight + 1), lighter);
    VideoDriver::driver()->fill_rect(Rect(
                r.left - h_border, r.top + msheight, r.left + 1, r.top + msheight + 1), lighter);
    VideoDriver::driver()->fill_rect(Rect(
                r.left, r.top + msheight + kInterfaceVLipHeight, r.right,
                r.top + msheight + kInterfaceVLipHeight + 1), lighter);
    VideoDriver::driver()->fill_rect(Rect(
                r.right - 1, r.top + msheight, r.right + h_border - 1,
                r.top + msheight + 1), lighter);

    const RgbColor darker = GetRGBTranslateColorShade(mcolor, mshade + kDarkerColor);
    VideoDriver::driver()->fill_rect(Rect(
                r.left - h_border + 1, r.top + msheight + kLabelBottomHeight, r.left + 1,
                r.top + msheight + kLabelBottomHeight + 1), darker);
    VideoDriver::driver()->fill_rect(Rect(
                r.left, r.top + msheight + kLabelBottomHeight - kInterfaceVLipHeight, r.right,
                r.top + msheight + kLabelBottomHeight - kInterfaceVLipHeight + 1), darker);
    VideoDriver::driver()->fill_rect(Rect(
                r.right - 1, r.top + msheight + kLabelBottomHeight, r.right + h_border,
                r.top + msheight + kLabelBottomHeight + 1), darker);
    VideoDriver::driver()->fill_rect(Rect(
                r.right + h_border - 1, r.top + msheight, r.right + h_border,
                r.top + msheight + kLabelBottomHeight + 1), darker);
}

template <typename T>
void draw_plain_rect(Point origin, const T& item) {
    Rect            tRect, uRect;
    int16_t         vcenter, thisHBorder = kInterfaceSmallHBorder;
    uint8_t         color = item.hue;
    interfaceStyleType style = item.style;

    if (style == kLarge) {
        thisHBorder = kInterfaceLargeHBorder;
    }
    tRect = item.bounds();
    tRect.offset(origin.h, origin.v);
    tRect.left -= kInterfaceContentBuffer;
    tRect.top -= kInterfaceContentBuffer;
    tRect.right += kInterfaceContentBuffer;
    tRect.bottom += kInterfaceContentBuffer;

    // top border
    mDrawPuffUpTopBorder(tRect, color, DARK, thisHBorder);
    // bottom border

    mDrawPuffUpBottomBorder(tRect, color, DARK, thisHBorder);

    // main part left border

    vcenter = ( tRect.bottom - tRect.top) / 2;

    uRect = Rect(tRect.left - thisHBorder,
        tRect.top + kInterfaceHTop,
        tRect.left + 1,
        tRect.top + vcenter - kInterfaceVLipHeight + 1);
    mDrawPuffUpRect( uRect, color, DARKER);

    uRect = Rect(tRect.left - thisHBorder,
        tRect.bottom - vcenter + kInterfaceVLipHeight,
        tRect.left + 1,
        tRect.bottom - kInterfaceHTop + 1);
    mDrawPuffUpRect( uRect, color, VERY_DARK);

    // right border

    uRect = Rect(tRect.right,
        tRect.top + kInterfaceHTop,
        tRect.right + thisHBorder + 1,
        tRect.top + vcenter - kInterfaceVLipHeight + 1);
    mDrawPuffUpRect( uRect, color, DARKER);

    uRect = Rect(tRect.right,
        tRect.bottom - vcenter + kInterfaceVLipHeight,
        tRect.right + thisHBorder + 1,
        tRect.bottom - kInterfaceHTop + 1);
    mDrawPuffUpRect( uRect, color, VERY_DARK);
}

void draw_tab_box(Point origin, const TabBox& item) {
    Rect                uRect;
    int16_t             vcenter, h_border = kInterfaceSmallHBorder;
    uint8_t             shade;
    uint8_t             color = item.hue;
    interfaceStyleType  style = item.style;
    int16_t             top_right_border_size = item.top_right_border_size;

    Rect r = item.bounds();
    r.offset(origin.h, origin.v);
    if ( style == kLarge) h_border = kInterfaceLargeHBorder;
    r.left -= kInterfaceContentBuffer;
    r.top -= kInterfaceContentBuffer;
    r.right += kInterfaceContentBuffer;
    r.bottom += kInterfaceContentBuffer;
    Rect outer(
            r.left - h_border, r.top - 3 - kInterfaceVCornerHeight,
            r.right + h_border, r.top - kInterfaceVLipHeight);

    // top border
    shade = MEDIUM;
    const RgbColor rgb = GetRGBTranslateColorShade(color, shade);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, outer.top, r.left, r.top), rgb);
    VideoDriver::driver()->fill_rect(
            Rect(r.right, outer.top, outer.right, r.top), rgb);
    VideoDriver::driver()->fill_rect(
            Rect(r.left, outer.top, r.left + 6, outer.bottom), rgb);
    VideoDriver::driver()->fill_rect(
            Rect(r.right - top_right_border_size, outer.top, r.right, outer.bottom), rgb);

    const RgbColor darker = GetRGBTranslateColorShade(color, shade + kDarkerColor);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, r.top, r.left + 1, r.top + 1), darker);
    VideoDriver::driver()->fill_rect(
            Rect(r.left, outer.bottom, r.left + 6, outer.bottom + 1), darker);
    VideoDriver::driver()->fill_rect(
            Rect(r.right - top_right_border_size, outer.bottom, r.right + 1, outer.bottom + 1),
            darker);
    VideoDriver::driver()->fill_rect(
            Rect(r.right, r.top, outer.right + 1, r.top + 1), darker);
    VideoDriver::driver()->fill_rect(
            Rect(outer.right, outer.top, outer.right + 1, r.top), darker);

    const RgbColor lighter = GetRGBTranslateColorShade(color, shade + kLighterColor);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, outer.top, outer.left + 1, r.top), lighter);
    VideoDriver::driver()->fill_rect(
            Rect(outer.left, outer.top, r.left + 6, outer.top + 1), lighter);
    VideoDriver::driver()->fill_rect(
            Rect(r.right - top_right_border_size, outer.top, outer.right + 1, outer.top + 1),
            lighter);

    // bottom border

    mDrawPuffUpBottomBorder(r, color, DARK, h_border);

    // main part left border

    vcenter = (r.bottom - r.top) / 2;

    uRect = Rect(outer.left,
        r.top + kInterfaceHTop,
        r.left + 1,
        r.top + vcenter - kInterfaceVLipHeight + 1);
    mDrawPuffUpRect(uRect, color, DARKER);

    uRect = Rect(outer.left,
        r.bottom - vcenter + kInterfaceVLipHeight,
        r.left + 1,
        r.bottom - kInterfaceHTop + 1);
    mDrawPuffUpRect(uRect, color, VERY_DARK);

    // right border

    uRect = Rect(r.right,
        r.top + kInterfaceHTop,
        outer.right + 1,
        r.top + vcenter - kInterfaceVLipHeight + 1);
    mDrawPuffUpRect(uRect, color, DARKER);

    uRect = Rect(r.right,
        r.bottom - vcenter + kInterfaceVLipHeight,
        outer.right + 1,
        r.bottom - kInterfaceHTop + 1);
    mDrawPuffUpRect(uRect, color, VERY_DARK);
}

void draw_button(Point origin, const PlainButton& item) {
    Rect            tRect, uRect, vRect;
    int16_t         vcenter, swidth, sheight, thisHBorder = kInterfaceSmallHBorder;
    uint8_t         shade;
    RgbColor        color;

    if (item.style == kLarge) {
        thisHBorder = kInterfaceLargeHBorder;
    }
    tRect = item.bounds();
    tRect.offset(origin.h, origin.v);

    uRect = tRect;
    uRect.right++;
    uRect.bottom++;

    tRect.left -= kInterfaceContentBuffer;
    tRect.top -= kInterfaceContentBuffer;
    tRect.right += kInterfaceContentBuffer;
    tRect.bottom += kInterfaceContentBuffer;

    // top border

    if (item.status == kDimmed) {
        shade = VERY_DARK;
    } else {
        shade = MEDIUM;
    }

    mDrawPuffUpTopBorder(tRect, item.hue, shade, thisHBorder);
    // bottom border

    mDrawPuffUpBottomBorder(tRect, item.hue, shade, thisHBorder);

    // side border top

    vcenter = ( tRect.bottom - tRect.top) / 2;

    uRect = Rect(tRect.left - thisHBorder, tRect.top + kInterfaceHTop, tRect.left + 1,
            tRect.bottom - kInterfaceHTop + 1);
    vRect = Rect(tRect.right, tRect.top + kInterfaceHTop, tRect.right + thisHBorder + 1,
            tRect.bottom - kInterfaceHTop + 1);
    if (item.status == kIH_Hilite) {
        shade = LIGHT;
        mDrawPuffUpRect(uRect, item.hue, shade);
        mDrawPuffUpRect(vRect, item.hue, shade);
    } else {
        if (item.status == kDimmed) {
            shade = VERY_DARK;
        } else {
            shade = MEDIUM + kSlightlyLighterColor;
        }
        mDrawPuffUpRect(uRect, item.hue, shade);
        mDrawPuffUpRect(vRect, item.hue, shade);
    }


    if (item.key == 0)
    {
        uRect = Rect(tRect.left +  kInterfaceContentBuffer,
            tRect.top + kInterfaceContentBuffer,
            tRect.left +  kInterfaceContentBuffer,
            tRect.bottom - kInterfaceContentBuffer);

        if (item.status == kIH_Hilite)
            shade = LIGHT;
        else shade = DARK;//DARKEST + kSlightlyLighterColor;
        uRect = Rect(tRect.left +  kInterfaceContentBuffer, tRect.top + kInterfaceContentBuffer,
                        tRect.right - kInterfaceContentBuffer + 1,
                        tRect.bottom - kInterfaceContentBuffer + 1);

        color = GetRGBTranslateColorShade(item.hue, shade);
        VideoDriver::driver()->fill_rect(uRect, color);

        if (item.status == kIH_Hilite) {
            color = GetRGBTranslateColorShade(item.hue, DARKEST);
        } else if (item.status == kDimmed) {
            color = GetRGBTranslateColorShade(item.hue, VERY_DARK);
        } else {
            color = GetRGBTranslateColorShade(item.hue, LIGHTER);
        }
        StringSlice s = item.label;
        swidth = GetInterfaceStringWidth(s, item.style);
        swidth = tRect.left + ( tRect.right - tRect.left) / 2 - swidth / 2;
        sheight = GetInterfaceFontAscent(item.style) + kInterfaceTextVBuffer + tRect.top;
        DrawInterfaceString(Point(swidth, sheight), s, item.style, color);
    } else
    {
        // draw the key code
        {
            if (item.status == kDimmed)
                shade = VERY_DARK;
            else shade = LIGHT;
            String s;
            GetKeyNumName(item.key, &s);
            swidth = GetInterfaceFontWidth(item.style) * kMaxKeyNameLength;

            uRect = Rect(tRect.left +  kInterfaceContentBuffer, tRect.top + kInterfaceContentBuffer,
                    tRect.left + kInterfaceContentBuffer + swidth + kInterfaceTextHBuffer * 2 + 1,
                    tRect.bottom - kInterfaceContentBuffer + 1);
            mDrawPuffUpRect(uRect, item.hue, shade);

            if (item.status == kIH_Hilite)
                shade = LIGHT;
            else shade = DARK;//DARKEST;
            vRect = Rect(
                    tRect.left + kInterfaceContentBuffer + swidth + kInterfaceTextHBuffer * 2 + 2,
                    tRect.top + kInterfaceContentBuffer,
                    tRect.right - kInterfaceContentBuffer + 1,
                    tRect.bottom - kInterfaceContentBuffer + 1);
            color = GetRGBTranslateColorShade(item.hue, shade);
            VideoDriver::driver()->fill_rect(vRect, color);

            swidth = GetInterfaceStringWidth(s, item.style);
            swidth = uRect.left + ( uRect.right - uRect.left) / 2 - swidth / 2;
            if (item.status == kDimmed) {
                color = GetRGBTranslateColorShade(item.hue, VERY_DARK);
            } else {
                color = GetRGBTranslateColorShade(item.hue, DARKEST);
            }

            DrawInterfaceString(
                    Point(swidth, uRect.top + GetInterfaceFontAscent(item.style)), s, item.style,
                    color);
        }

        // draw the button title
        {
            if (item.status == kIH_Hilite) {
                color = GetRGBTranslateColorShade(item.hue, DARKEST);
            } else if (item.status == kDimmed) {
                color = GetRGBTranslateColorShade(item.hue, DARKEST + kSlightlyLighterColor);
            } else {
                color = GetRGBTranslateColorShade(item.hue, LIGHTER);
            }

            StringSlice s = item.label;
            swidth = GetInterfaceStringWidth(s, item.style);
            swidth = uRect.right + ( tRect.right - uRect.right) / 2 - swidth / 2;
            sheight = GetInterfaceFontAscent(item.style) + kInterfaceTextVBuffer + tRect.top;
            DrawInterfaceString(Point(swidth, sheight), s, item.style, color);
        }
    }
}

void draw_tab_box_button(Point origin, const TabBoxButton& item) {
    Rect            tRect;
    int16_t         vcenter, swidth, sheight, h_border = kInterfaceSmallHBorder;
    uint8_t         shade;
    RgbColor        color;

    if (item.style == kLarge) {
        h_border = kInterfaceLargeHBorder;
    }
    tRect = item.bounds();
    tRect.offset(origin.h, origin.v);

    tRect.left -= kInterfaceContentBuffer;
    tRect.top -= kInterfaceContentBuffer;
    tRect.right += kInterfaceContentBuffer;
    tRect.bottom += kInterfaceContentBuffer;

    // top border

    if (item.status == kDimmed) {
        shade = VERY_DARK;
    } else {
        shade = MEDIUM;
    }

    mDrawPuffUpTopBorder(tRect, item.hue, shade, h_border);

    // side border top

    vcenter = (tRect.bottom - tRect.top) / 2;

    Rect left(
            tRect.left - h_border, tRect.top + kInterfaceHTop, tRect.left + 1,
            tRect.bottom - kInterfaceHTop + 1);
    Rect right(
            tRect.right, tRect.top + kInterfaceHTop, tRect.right + h_border + 1,
            tRect.bottom - kInterfaceHTop + 1);
    if (!item.on) {
        if (item.status == kIH_Hilite) {
            shade = LIGHT;
            mDrawPuffUpRect(left, item.hue, shade);
            mDrawPuffUpRect(right, item.hue, shade);
        } else {
            if (item.status == kDimmed) {
                shade = VERY_DARK;
            }
            else shade = DARK;
            mDrawPuffUpRect(left, item.hue, shade);
            mDrawPuffUpRect(right, item.hue, shade);
        }
        left = Rect(left.left, left.bottom, left.right, left.bottom + 3);
        right = Rect(right.left, right.bottom, right.right, right.bottom + 3);
        VideoDriver::driver()->fill_rect(left, RgbColor::kBlack);
        VideoDriver::driver()->fill_rect(right, RgbColor::kBlack);
        shade = MEDIUM;
        color = GetRGBTranslateColorShade(item.hue, shade);
        VideoDriver::driver()->fill_rect(
                Rect(left.left - 3, left.bottom, right.right + 3, left.bottom + 3), color);

        const RgbColor lighter = GetRGBTranslateColorShade(item.hue, shade + kLighterColor);
        VideoDriver::driver()->fill_rect(
                Rect(left.left - 3, left.bottom - 1, right.right + 3, left.bottom), lighter);
        const RgbColor darker = GetRGBTranslateColorShade(item.hue, shade + kDarkerColor);
        VideoDriver::driver()->fill_rect(
                Rect(left.left - 3, left.bottom + 3, right.right + 3, left.bottom + 4), darker);
    } else {
        if (item.status == kIH_Hilite) {
            shade = LIGHT;
        } else if (item.status == kDimmed) {
            shade = VERY_DARK;
        } else {
            shade = MEDIUM;
        }
        left.bottom += 7;
        right.bottom += 7;
        color = GetRGBTranslateColorShade(item.hue, shade);
        VideoDriver::driver()->fill_rect(left, color);
        VideoDriver::driver()->fill_rect(right, color);

        const RgbColor lighter = GetRGBTranslateColorShade(item.hue, shade + kLighterColor);
        VideoDriver::driver()->fill_rect(
                Rect(left.left, left.top, left.right - 1, left.top + 1), lighter);
        VideoDriver::driver()->fill_rect(
                Rect(left.left, left.top, left.left + 1, left.bottom - 5), lighter);
        VideoDriver::driver()->fill_rect(
                Rect(left.left - 3, left.bottom - 5, left.left + 1, left.bottom - 4), lighter);
        VideoDriver::driver()->fill_rect(
                Rect(right.left, right.top, right.right - 1, right.top + 1), lighter);
        VideoDriver::driver()->fill_rect(
                Rect(right.right, right.bottom - 5, right.right + 3, right.bottom - 4), lighter);
        VideoDriver::driver()->fill_rect(
                Rect(right.left, right.top, right.left + 1, right.bottom - 1), lighter);

        const RgbColor darker = GetRGBTranslateColorShade(item.hue, shade + kDarkerColor);
        VideoDriver::driver()->fill_rect(
                Rect(left.left - 3, left.bottom - 1, left.right, left.bottom), darker);
        VideoDriver::driver()->fill_rect(
                Rect(left.right - 1, left.top, left.right, left.bottom), darker);
        VideoDriver::driver()->fill_rect(
                Rect(right.right - 1, right.top, right.right, right.bottom - 4), darker);
        VideoDriver::driver()->fill_rect(
                Rect(right.left, right.bottom - 1, right.right + 3, right.bottom), darker);

        Rect uRect(left.left - 3, left.bottom - 4, left.right - 1, left.bottom - 1);
        const RgbColor color = GetRGBTranslateColorShade(item.hue, shade);
        VideoDriver::driver()->fill_rect(uRect, color);
        Rect vRect(right.left + 1, right.bottom - 4, right.right + 3, right.bottom - 1);
        VideoDriver::driver()->fill_rect(vRect, color);
        uRect.top--;
        uRect.bottom++;
        uRect.left = uRect.right + 1;
        uRect.right = vRect.left - 1;
        VideoDriver::driver()->fill_rect(uRect, RgbColor::kBlack);
    }

    if (item.key == 0) {
        Rect uRect(tRect.left +  kInterfaceContentBuffer,
            tRect.top + kInterfaceContentBuffer,
            tRect.left +  kInterfaceContentBuffer,
            tRect.bottom - kInterfaceContentBuffer);

        if (item.on) {
            shade = MEDIUM;
        } else if (item.status == kIH_Hilite) {
            shade = LIGHT;
        } else {
            shade = DARKER;//DARKEST + kSlightlyLighterColor;
        }
        uRect = Rect(tRect.left +  kInterfaceContentBuffer, tRect.top + kInterfaceContentBuffer,
                        tRect.right - kInterfaceContentBuffer + 1,
                        tRect.bottom - kInterfaceContentBuffer + 1);
        color = GetRGBTranslateColorShade(item.hue, shade);
        VideoDriver::driver()->fill_rect(uRect, color);

        if (!item.on) {
            if (item.status == kIH_Hilite) {
                color = GetRGBTranslateColorShade(item.hue, DARKEST);
            } else if (item.status == kDimmed) {
                color = GetRGBTranslateColorShade(item.hue, VERY_DARK);
            } else {
                color = GetRGBTranslateColorShade(item.hue, LIGHT);
            }
        } else {
            color = GetRGBTranslateColorShade(item.hue, VERY_LIGHT);
        }

        StringSlice s = item.label;
        swidth = GetInterfaceStringWidth(s, item.style);
        swidth = tRect.left + (tRect.right - tRect.left) / 2 - swidth / 2;
        sheight = GetInterfaceFontAscent(item.style) + kInterfaceTextVBuffer + tRect.top;
        DrawInterfaceString(Point(swidth, sheight), s, item.style, color);
    } else {
        // draw the key code
        if (item.on) {
            shade = MEDIUM + kLighterColor;
        } else if (item.status == kIH_Hilite) {
            shade = VERY_LIGHT;
        } else {
            shade = DARK;//DARKEST + kSlightlyLighterColor;
        }
        String s;
        GetKeyNumName(item.key, &s);
        swidth = GetInterfaceFontWidth(item.style) * kMaxKeyNameLength;

        Rect uRect(
                tRect.left +  kInterfaceContentBuffer, tRect.top + kInterfaceContentBuffer,
                tRect.left + kInterfaceContentBuffer + swidth + kInterfaceTextHBuffer * 2 + 1,
                tRect.bottom - kInterfaceContentBuffer + 1);
        mDrawPuffUpRect(uRect, item.hue, shade);

        if (item.on) {
            shade = MEDIUM;
        } else if (item.status == kIH_Hilite) {
            shade = VERY_LIGHT;
        } else {
            shade = DARKER;//DARKEST + kSlightlyLighterColor;
        }
        Rect vRect(
                tRect.left + kInterfaceContentBuffer + swidth + kInterfaceTextHBuffer * 2 + 2,
                tRect.top + kInterfaceContentBuffer,
                tRect.right - kInterfaceContentBuffer + 1,
                tRect.bottom - kInterfaceContentBuffer + 1);
        color = GetRGBTranslateColorShade(item.hue, shade);
        VideoDriver::driver()->fill_rect(vRect, color);

        swidth = GetInterfaceStringWidth(s, item.style);
        swidth = uRect.left + (uRect.right - uRect.left) / 2 - swidth / 2;
        if (item.status == kDimmed) {
            color = GetRGBTranslateColorShade(item.hue, VERY_DARK);
        } else {
            color = GetRGBTranslateColorShade(item.hue, DARKEST);
        }

        DrawInterfaceString(
                Point(swidth, uRect.top + GetInterfaceFontAscent(item.style)), s, item.style,
                color);

        // draw the button title
        if (!item.on) {
            if (item.status == kIH_Hilite) {
                color = GetRGBTranslateColorShade(item.hue, DARKEST);
            } else if (item.status == kDimmed) {
                color = GetRGBTranslateColorShade(item.hue, VERY_DARK);
            } else {
                color = GetRGBTranslateColorShade(item.hue, LIGHT);
            }
        } else {
            color = GetRGBTranslateColorShade(item.hue, VERY_LIGHT);
        }

        {
            StringSlice s = item.label;
            swidth = GetInterfaceStringWidth(s, item.style);
            swidth = uRect.right + (tRect.right - uRect.right) / 2 - swidth / 2;
            sheight = GetInterfaceFontAscent(item.style) + kInterfaceTextVBuffer + tRect.top;
            DrawInterfaceString(Point(swidth, sheight), s, item.style, color);
        }
    }
}

/*
void DrawPlayerInterfaceRadioButton(Rect bounds, const RadioButton& item, PixMap* pix) {
    Rect            tRect, uRect, vRect, wRect;
    int16_t         vcenter, swidth, sheight, thisHBorder = kInterfaceSmallHBorder;
    uint8_t         shade;
    RgbColor        color;

    if ( item.style == kLarge) thisHBorder = kInterfaceLargeHBorder;
    tRect = bounds;

    tRect.left -= kInterfaceContentBuffer;
    tRect.top -= kInterfaceContentBuffer;
    tRect.right += kInterfaceContentBuffer;
    tRect.bottom += kInterfaceContentBuffer;

    // top border

    if ( item.status == kDimmed)
        shade = VERY_DARK;
    else shade = MEDIUM;

    mDrawPuffUpTopBorder( tRect, uRect, item.hue, shade, thisHBorder, pix);
    // bottom border

    mDrawPuffUpBottomBorder( tRect, uRect, item.hue, shade, thisHBorder, pix);

    // side border top

    vcenter = ( tRect.bottom - tRect.top) / 2;
    swidth = (tRect.top - kInterfaceVEdgeHeight - kInterfaceVCornerHeight + kIndicatorVOffset);
    sheight = (tRect.bottom + kInterfaceVEdgeHeight + kInterfaceVCornerHeight - kIndicatorVOffset) -
            swidth;

    wRect = Rect(tRect.left - thisHBorder - kCheckIndicatorHOffset - sheight, swidth,
            tRect.left - thisHBorder - kCheckIndicatorHOffset + 1, swidth + sheight + 1);

    uRect = Rect(tRect.left - thisHBorder - kCheckIndicatorHOffset - 2,
            tRect.top + kInterfaceHTop,
            tRect.left + 1,
            *//*tRect.top + vcenter - kInterfaceVLipHeight + 1*//*
            tRect.bottom - kInterfaceHTop + 1);
    vRect = Rect(tRect.right, tRect.top + kInterfaceHTop, tRect.right + thisHBorder + 1,
            *//*tRect.top + vcenter - kInterfaceVLipHeight + 1*//*
            tRect.bottom - kInterfaceHTop + 1);

    if ( item.status == kIH_Hilite)
    {
        shade = LIGHT;
        mDrawPuffUpRect( uRect, item.hue, shade, pix);
        mDrawPuffUpRect( vRect, item.hue, shade, pix);

        wRect.left += 2;
        wRect.right += 2;
        FrameOval(pix, wRect, RgbColor::kBlack);
        wRect.left -= 2;
        wRect.right -= 2;
        mDrawPuffUpOval(wRect, item.hue, shade, pix);

        wRect.inset(3, 3);
        mDrawPuffDownOval(wRect, item.hue, shade, pix);
        wRect.inset(1, 1);

        if (!item.on) {
            PaintOval(pix, wRect, RgbColor::kBlack);
        } else {
            const RgbColor color = GetRGBTranslateColorShade(item.hue, VERY_LIGHT);
            PaintOval(pix, wRect, color);
        }
    } else
    {
        if ( item.status == kDimmed)
            shade = VERY_DARK;
        else shade = MEDIUM + kSlightlyLighterColor;
        mDrawPuffUpRect( uRect, item.hue, shade, pix);
        mDrawPuffUpRect( vRect, item.hue, shade, pix);
        wRect.left += 2;
        wRect.right += 2;
        FrameOval(pix, wRect, RgbColor::kBlack);
        wRect.left -= 2;
        wRect.right -= 2;
        mDrawPuffUpOval(wRect, item.hue, shade, pix);

        wRect.inset(3, 3);
        mDrawPuffDownOval(wRect, item.hue, shade, pix);
        wRect.inset(1, 1);
        if (!item.on) {
            PaintOval(pix, wRect, RgbColor::kBlack);
        } else if (item.status == kActive) {
            const RgbColor color = GetRGBTranslateColorShade(item.hue, LIGHT);
            PaintOval(pix, wRect, color);
        } else {
            const RgbColor color = GetRGBTranslateColorShade(item.hue, MEDIUM);
            PaintOval(pix, wRect, color);
        }
    }

    uRect = Rect(tRect.left +  kInterfaceContentBuffer,
        tRect.top + kInterfaceContentBuffer,
        tRect.left +  kInterfaceContentBuffer,
        tRect.bottom - kInterfaceContentBuffer);

    if ( item.status == kIH_Hilite)
        shade = LIGHT;
    else shade = DARKEST + kSlightlyLighterColor;
    uRect = Rect(tRect.left +  kInterfaceContentBuffer, tRect.top + kInterfaceContentBuffer,
                    tRect.right - kInterfaceContentBuffer + 1,
                    tRect.bottom - kInterfaceContentBuffer + 1);
    color = GetRGBTranslateColorShade(item.hue, shade);
    pix->view(uRect).fill(color);

    if (item.status == kIH_Hilite) {
        color = GetRGBTranslateColorShade(item.hue, DARKEST);
    } else if (item.status == kDimmed) {
        color = GetRGBTranslateColorShade(item.hue, DARK);
    } else {
        color = GetRGBTranslateColorShade(item.hue, LIGHT);
    }
    StringSlice s = item.label;
    swidth = GetInterfaceStringWidth( s, item.style);
    swidth = tRect.left + ( tRect.right - tRect.left) / 2 - swidth / 2;
    sheight = GetInterfaceFontAscent(item.style) + kInterfaceTextVBuffer + tRect.top;
    DrawInterfaceString(Point(swidth, sheight), s, item.style, pix, color);
}
*/

void draw_checkbox(Point origin, const CheckboxButton& item) {
    Rect            tRect, uRect, vRect, wRect;
    int16_t         vcenter, swidth, sheight, thisHBorder = kInterfaceSmallHBorder;
    uint8_t         shade;
    RgbColor        color;

    if ( item.style == kLarge) thisHBorder = kInterfaceLargeHBorder;
    tRect = item.bounds();
    tRect.offset(origin.h, origin.v);

    tRect.left -= kInterfaceContentBuffer;
    tRect.top -= kInterfaceContentBuffer;
    tRect.right += kInterfaceContentBuffer;
    tRect.bottom += kInterfaceContentBuffer;

    // top border

    if ( item.status == kDimmed)
        shade = VERY_DARK;
    else shade = MEDIUM;

    mDrawPuffUpTopBorder(tRect, item.hue, shade, thisHBorder);
    // bottom border

    mDrawPuffUpBottomBorder(tRect, item.hue, shade, thisHBorder);

    // side border top

    vcenter = ( tRect.bottom - tRect.top) / 2;
    swidth = (tRect.top - kInterfaceVEdgeHeight - kInterfaceVCornerHeight + kIndicatorVOffset);
    sheight = (tRect.bottom + kInterfaceVEdgeHeight + kInterfaceVCornerHeight - kIndicatorVOffset) -
            swidth;

    wRect = Rect(tRect.left - thisHBorder - kCheckIndicatorHOffset - sheight, swidth,
            tRect.left - thisHBorder - kCheckIndicatorHOffset + 1, swidth + sheight + 1);

    uRect = Rect(tRect.left - thisHBorder - kCheckIndicatorHOffset + 2,
            tRect.top + kInterfaceHTop,
            tRect.left + 1,
            /*tRect.top + vcenter - kInterfaceVLipHeight + 1*/
            tRect.bottom - kInterfaceHTop + 1);
    vRect = Rect(tRect.right, tRect.top + kInterfaceHTop, tRect.right + thisHBorder + 1,
            /*tRect.top + vcenter - kInterfaceVLipHeight + 1*/
            tRect.bottom - kInterfaceHTop + 1);

    if (item.status == kIH_Hilite) {
        shade = LIGHT;
        mDrawPuffUpRect(uRect, item.hue, shade);
        mDrawPuffUpRect(vRect, item.hue, shade);
        mDrawPuffUpRect(wRect, item.hue, shade);
        wRect.inset(3, 3);
        mDrawPuffDownRect(wRect, item.hue, shade);
        wRect.inset(1, 1);
        if ( !item.on) {
            color = RgbColor::kBlack;
        } else {
            color = GetRGBTranslateColorShade(item.hue, VERY_LIGHT);
        }
        VideoDriver::driver()->fill_rect(wRect, color);
    } else {
        if ( item.status == kDimmed)
            shade = VERY_DARK;
        else shade = MEDIUM + kSlightlyLighterColor;
        mDrawPuffUpRect( uRect, item.hue, shade);
        mDrawPuffUpRect( vRect, item.hue, shade);
        mDrawPuffUpRect( wRect, item.hue, shade);
        wRect.inset(3, 3);
        mDrawPuffDownRect( wRect, item.hue, shade);
        wRect.inset(1, 1);
        if (!item.on) {
            color = RgbColor::kBlack;
        } else if (item.status == kActive) {
            color = GetRGBTranslateColorShade(item.hue, LIGHT);
        } else {
            color = GetRGBTranslateColorShade(item.hue, MEDIUM);
        }
        VideoDriver::driver()->fill_rect(wRect, color);
    }

    uRect = Rect(tRect.left +  kInterfaceContentBuffer,
        tRect.top + kInterfaceContentBuffer,
        tRect.left +  kInterfaceContentBuffer,
        tRect.bottom - kInterfaceContentBuffer);

    if ( item.status == kIH_Hilite)
        shade = LIGHT;
    else shade = DARKEST + kSlightlyLighterColor;
    uRect = Rect(tRect.left +  kInterfaceContentBuffer, tRect.top + kInterfaceContentBuffer,
                    tRect.right - kInterfaceContentBuffer + 1,
                    tRect.bottom - kInterfaceContentBuffer + 1);
    color = GetRGBTranslateColorShade(item.hue, shade);
    VideoDriver::driver()->fill_rect(uRect, color);

    if (item.status == kIH_Hilite) {
        color = GetRGBTranslateColorShade(item.hue, DARKEST);
    } else if ( item.status == kDimmed) {
        color = GetRGBTranslateColorShade(item.hue, DARK);
    } else {
        color = GetRGBTranslateColorShade(item.hue, LIGHT);
    }

    StringSlice s = item.label;
    swidth = GetInterfaceStringWidth( s, item.style);
    swidth = tRect.left + ( tRect.right - tRect.left) / 2 - swidth / 2;
    sheight = GetInterfaceFontAscent(item.style) + kInterfaceTextVBuffer + tRect.top;
    DrawInterfaceString(Point(swidth, sheight), s, item.style, color);
}

void draw_labeled_box(Point origin, const LabeledRect& item) {
    Rect            tRect, uRect;
    int16_t         vcenter, swidth, sheight, thisHBorder = kInterfaceSmallHBorder;
    uint8_t         shade;
    RgbColor        color;

    if (item.style == kLarge) {
        thisHBorder = kInterfaceLargeHBorder;
    }
    tRect = item.bounds();
    tRect.offset(origin.h, origin.v);
    tRect.left -= kInterfaceContentBuffer;
    tRect.top -= kInterfaceContentBuffer + GetInterfaceFontHeight(item.style) +
            kInterfaceTextVBuffer * 2 + kLabelBottomHeight;
    tRect.right += kInterfaceContentBuffer;
    tRect.bottom += kInterfaceContentBuffer;

    shade = DARK;

    mDrawPuffUpTopBorder(tRect, item.hue, shade, thisHBorder);
    // bottom border

    mDrawPuffUpBottomBorder(tRect, item.hue, shade, thisHBorder);

    // draw the string

    StringSlice s = item.label;
    swidth = GetInterfaceStringWidth( s, item.style) + kInterfaceTextHBuffer * 2;
    swidth = ( tRect.right - tRect.left) - swidth;
    sheight = GetInterfaceFontHeight( item.style) + kInterfaceTextVBuffer * 2;

    uRect = Rect(tRect.left + kInterfaceTextHBuffer - 1,
        tRect.top + kInterfaceHTop,
        tRect.right - swidth - kInterfaceTextHBuffer + 1,
        tRect.top + sheight - kInterfaceHTop);
    color = GetRGBTranslateColorShade(item.hue, VERY_DARK);
    VideoDriver::driver()->fill_rect(uRect, color);

    color = GetRGBTranslateColorShade(item.hue, LIGHT);

    DrawInterfaceString(
            Point(
                tRect.left + kInterfaceTextHBuffer,
                tRect.top + GetInterfaceFontAscent( item.style) + kInterfaceTextVBuffer),
            s, item.style, color);

    // string left border

    shade = MEDIUM;
    vcenter = sheight / 2;

    uRect = Rect(tRect.left - thisHBorder,
            tRect.top + kInterfaceHTop,
            tRect.left + 1, tRect.top + sheight - kInterfaceHTop + 1);
    mDrawPuffUpRect( uRect, item.hue, shade);

    // string right border

    shade = MEDIUM;
    uRect = Rect(tRect.right - swidth,
        tRect.top + kInterfaceHTop,
        tRect.right - 2,
        tRect.top + sheight - kInterfaceHTop + 1);
    mDrawPuffUpRect( uRect, item.hue, shade);
    uRect = Rect(tRect.right,
        tRect.top + kInterfaceHTop,
        tRect.right + thisHBorder + 1,
        tRect.top + sheight - kInterfaceHTop + 1);
    mDrawPuffUpRect( uRect, item.hue, shade);

    // string bottom border

    mDrawPuffUpTBorder(tRect, item.hue, DARK, sheight, thisHBorder);

    // main part left border

    tRect.top += sheight + kLabelBottomHeight;

    vcenter = ( tRect.bottom - tRect.top) / 2;

    uRect = Rect(tRect.left - thisHBorder,
        tRect.top + kInterfaceHTop,
        tRect.left + 1,
        tRect.top + vcenter - kInterfaceVLipHeight + 1);
    mDrawPuffUpRect( uRect, item.hue, DARKER);

    uRect = Rect(tRect.left - thisHBorder,
        tRect.bottom - vcenter + kInterfaceVLipHeight,
        tRect.left + 1,
        tRect.bottom - kInterfaceHTop + 1);
    mDrawPuffUpRect( uRect, item.hue, VERY_DARK);

    // right border

    uRect = Rect(tRect.right,
        tRect.top + kInterfaceHTop,
        tRect.right + thisHBorder + 1,
        tRect.top + vcenter - kInterfaceVLipHeight + 1);
    mDrawPuffUpRect( uRect, item.hue, DARKER);

    uRect = Rect(tRect.right,
        tRect.bottom - vcenter + kInterfaceVLipHeight,
        tRect.right + thisHBorder + 1,
        tRect.bottom - kInterfaceHTop + 1);
    mDrawPuffUpRect( uRect, item.hue, VERY_DARK);
}

void draw_text_rect(Point origin, const TextRect& item) {
    vector<inlinePictType> inlinePict;
    Rect bounds = item.bounds();
    bounds.offset(origin.h, origin.v);
    draw_text_in_rect(bounds, item.text, item.style, item.hue, inlinePict);
}

}  // namespace

void draw_text_in_rect(
        Rect tRect, const StringSlice& text, interfaceStyleType style,
        uint8_t textcolor, vector<inlinePictType>& inlinePict) {
    RgbColor color = GetRGBTranslateColorShade(textcolor, VERY_LIGHT);
    StyledText interface_text(interface_font(style));
    interface_text.set_fore_color(color);
    interface_text.set_interface_text(text);
    interface_text.wrap_to(tRect.width(), kInterfaceTextHBuffer, kInterfaceTextVBuffer);
    inlinePict = interface_text.inline_picts();
    for (int i = 0; i < inlinePict.size(); ++i) {
        inlinePict[i].bounds.offset(tRect.left, tRect.top);
    }
    tRect.offset(0, -kInterfaceTextVBuffer);
    interface_text.draw(tRect);
}

void populate_inline_picts(
        Rect rect, StringSlice text, interfaceStyleType style,
        vector<inlinePictType>& inline_pict) {
    StyledText interface_text(interface_font(style));
    interface_text.set_interface_text(text);
    interface_text.wrap_to(rect.width(), kInterfaceTextHBuffer, kInterfaceTextVBuffer);
    inline_pict = interface_text.inline_picts();
    for (int i = 0; i < inline_pict.size(); ++i) {
        inline_pict[i].bounds.offset(rect.left, rect.top);
    }
}

int16_t GetInterfaceTextHeightFromWidth(
        const StringSlice& text, interfaceStyleType style, int16_t boundsWidth) {
    StyledText interface_text(interface_font(style));
    interface_text.set_interface_text(text);
    interface_text.wrap_to(boundsWidth, kInterfaceTextHBuffer, kInterfaceTextVBuffer);
    return interface_text.height();
}

void draw_picture_rect(Point origin, const PictureRect& item) {
    Rect bounds = item.bounds();
    bounds.offset(origin.h, origin.v);
    if (item.visible_bounds) {
        draw_plain_rect(origin, item);
    }
    item.sprite->draw(bounds.left, bounds.top);
}

namespace {

struct DrawInterfaceItemVisitor : InterfaceItem::Visitor {
    Point p;
    DrawInterfaceItemVisitor(Point origin): p(origin) { }

    virtual void visit_plain_rect(const PlainRect& i) const { draw_plain_rect(p, i); }
    virtual void visit_labeled_rect(const LabeledRect& i) const { draw_labeled_box(p, i); }
    virtual void visit_text_rect(const TextRect& i) const { draw_text_rect(p, i); }
    virtual void visit_picture_rect(const PictureRect& i) const { draw_picture_rect(p, i); }
    virtual void visit_plain_button(const PlainButton& i) const { draw_button(p, i); }
    virtual void visit_radio_button(const RadioButton& i) const { }
    virtual void visit_checkbox_button(const CheckboxButton& i) const { draw_checkbox(p, i); }
    virtual void visit_tab_box(const TabBox& i) const { draw_tab_box(p, i); }
    virtual void visit_tab_box_button(const TabBoxButton& i) const { draw_tab_box_button(p, i); }
};

struct GetBoundsInterfaceItemVisitor : InterfaceItem::Visitor {
    Rect* bounds;
    GetBoundsInterfaceItemVisitor(Rect* bounds): bounds(bounds) { }

    void initialize_bounds(const InterfaceItem& item) const {
        *bounds = item.bounds();
        bounds->left -= kInterfaceContentBuffer;
        bounds->top -= kInterfaceContentBuffer;
        bounds->right += kInterfaceContentBuffer + 1;
        bounds->bottom += kInterfaceContentBuffer + 1;
    }

    template <typename T>
    int h_border(T& t) const {
        return t.style == kLarge ? kInterfaceLargeHBorder : kInterfaceSmallHBorder;
    }

    virtual void visit_plain_rect(const PlainRect& item) const {
        initialize_bounds(item);
        bounds->left -= h_border(item);
        bounds->right += h_border(item);
        bounds->top -= kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
    }

    virtual void visit_labeled_rect(const LabeledRect& item) const {
        initialize_bounds(item);
        bounds->left -= h_border(item);
        bounds->right += h_border(item);
        bounds->top -= GetInterfaceFontHeight(item.style) + kInterfaceTextVBuffer * 2 +
            kLabelBottomHeight + kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
    }

    virtual void visit_text_rect(const TextRect& item) const {
        initialize_bounds(item);
        bounds->left -= h_border(item);
        bounds->right += h_border(item);
        bounds->top -= kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
    }

    virtual void visit_picture_rect(const PictureRect& item) const {
        initialize_bounds(item);
        bounds->left -= h_border(item);
        bounds->right += h_border(item);
        bounds->top -= kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
    }

    virtual void visit_plain_button(const PlainButton& item) const {
        initialize_bounds(item);
        bounds->left -= h_border(item);
        bounds->right += h_border(item);
        bounds->top -= kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
    }

    virtual void visit_radio_button(const RadioButton& item) const {
        initialize_bounds(item);
        bounds->left -= bounds->bottom - bounds->top + 2 * kInterfaceVEdgeHeight +
            2 * kInterfaceVCornerHeight - 2 * kIndicatorVOffset + h_border(item) +
            kRadioIndicatorHOffset;
        bounds->right += h_border(item);
        bounds->top -= kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
    }

    virtual void visit_checkbox_button(const CheckboxButton& item) const {
        initialize_bounds(item);
        bounds->left -= bounds->bottom - bounds->top + 2 * kInterfaceVEdgeHeight +
            2 * kInterfaceVCornerHeight - 2 * kIndicatorVOffset + h_border(item) +
            kCheckIndicatorHOffset;
        bounds->right += h_border(item);
        bounds->top -= kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
    }

    virtual void visit_tab_box(const TabBox& item) const {
        initialize_bounds(item);
        bounds->left -= h_border(item);
        bounds->right += h_border(item);
        bounds->top -= kInterfaceVEdgeHeight + kInterfaceVCornerHeight + 2;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
    }

    virtual void visit_tab_box_button(const TabBoxButton& item) const {
        initialize_bounds(item);
        bounds->left -= h_border(item) + 5;
        bounds->right += h_border(item) + 5;
        bounds->top -= kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
        bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight + 2;
    }

    /*
        case kListRect:
            bounds->left -= thisHBorder;
            bounds->right += thisHBorder;
            bounds->top -= GetInterfaceFontAscent(item.style) + kInterfaceTextVBuffer * 2 +
                            kLabelBottomHeight + kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
            bounds->bottom += kInterfaceVEdgeHeight + kInterfaceVCornerHeight;
            break;
            */
};

} // namespace

void draw_interface_item(const InterfaceItem& item) {
    item.accept(DrawInterfaceItemVisitor({0, 0}));
}

void draw_interface_item(const InterfaceItem& item, Point origin) {
    item.accept(DrawInterfaceItemVisitor(origin));
}

void GetAnyInterfaceItemGraphicBounds(const InterfaceItem& item, Rect *bounds) {
    item.accept(GetBoundsInterfaceItemVisitor(bounds));
}

}  // namespace antares
