// Ares, a tactical space combat game.
// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "DirectText.hpp"

#include <algorithm>
#include "sfz/sfz.hpp"
#include "Base.h"

#include "ColorTable.hpp"
#include "Error.hpp"
#include "Resource.hpp"

using sfz::Bytes;
using sfz::BytesPiece;
using sfz::String;
using sfz::StringPiece;
using sfz::read;
using sfz::scoped_ptr;

namespace macroman = sfz::macroman;

namespace antares {

directTextType* gDirectText = nil;
long gWhichDirectText = 0;
scoped_ptr<directTextType>* gDirectTextData;

namespace {

uint8_t to_mac_roman(uint32_t code) {
    String string(1, code);
    Bytes bytes(macroman::encode(string));
    return bytes.at(0);
}

}  // namespace

directTextType::directTextType(int32_t id) {
    Resource defn_rsrc("font-descriptions", "nlFD", id);
    BytesPiece in(defn_rsrc.data());

    in.shift(4);
    read(&in, &resID);
    in.shift(2);
    read(&in, &logicalWidth);
    read(&in, &physicalWidth);
    read(&in, &height);
    read(&in, &ascent);

    Resource data_rsrc("font-bitmaps", "nlFM", resID);
    charSet.assign(data_rsrc.data());
}

directTextType::~directTextType() { }

int InitDirectText() {
    gDirectTextData = new scoped_ptr<directTextType>[kDirectFontNum];
    gDirectTextData[0].reset(new directTextType(kTacticalFontResID));
    gDirectTextData[1].reset(new directTextType(kComputerFontResID));
    gDirectTextData[2].reset(new directTextType(kButtonFontResID));
    gDirectTextData[3].reset(new directTextType(kMessageFontResID));
    gDirectTextData[4].reset(new directTextType(kTitleFontResID));
    gDirectTextData[5].reset(new directTextType(kButtonSmallFontResID));

    gDirectText = gDirectTextData[0].get();
    gWhichDirectText = 0;

    return kNoError;
}

void DirectTextCleanup() {
    delete[] gDirectTextData;
}

void mDirectCharWidth(unsigned char& width, uint32_t mchar) {
    const uint8_t* widptr = gDirectText->charSet.data()
        + gDirectText->height * gDirectText->physicalWidth * to_mac_roman(mchar)
        + to_mac_roman(mchar);
    width = *widptr;
}

void mSetDirectFont(long whichFont) {
    gWhichDirectText = whichFont;
    gDirectText = gDirectTextData[gWhichDirectText].get();
}

int mDirectFontHeight() {
    return gDirectText->height;
}

int mDirectFontAscent() {
    return gDirectText->ascent;
}

void mGetDirectStringDimensions(const StringPiece& string, long& width, long& height) {
    height = gDirectText->height;
    width = 0;
    for (size_t i = 0; i < string.size(); ++i) {
        const uint8_t* widptr = gDirectText->charSet.data()
            + gDirectText->height * gDirectText->physicalWidth * to_mac_roman(string.at(i))
            + to_mac_roman(string.at(i));
        width += *widptr;
    }
}

void DrawDirectTextStringClipped(
        const StringPiece& string, const RgbColor& color, PixMap* destMap, const Rect& clip,
        long portLeft, long portTop) {
    // move the pen to the resulting location
    Point pen;
    GetPen(&pen);
    pen.v -= gDirectText->ascent;

    // Top and bottom boundaries of where we draw.
    int topEdge = std::max(0, clip.top - pen.v);
    int bottomEdge = gDirectText->height - std::max(
            0, pen.v + gDirectText->height - clip.bottom + 1);

    int rowBytes = destMap->row_bytes();

    // set hchar = place holder for start of each char we draw
    RgbColor* hchar = destMap->mutable_bytes() + (pen.v + portTop + topEdge) * rowBytes
        + pen.h + (portLeft << 2);

    for (size_t i = 0; i < string.size(); ++i) {
        const uint8_t* sbyte = gDirectText->charSet.data()
            + gDirectText->height * gDirectText->physicalWidth * to_mac_roman(string.at(i))
            + to_mac_roman(string.at(i));

        int width = *sbyte;
        ++sbyte;

        if ((pen.h + width >= clip.left) || (pen.h < clip.right)) {
            // Left and right boundaries of where we draw.
            int leftEdge = std::max(0, clip.left - pen.h);
            int rightEdge = width - std::max(0, pen.h + width - clip.right);

            // skip over the clipped top rows
            sbyte += topEdge * gDirectText->physicalWidth;

            // dbyte = destination pixel
            RgbColor* dbyte = hchar;

            // repeat for every unclipped row
            for (int y = topEdge; y < bottomEdge; ++y) {
                // repeat for every byte of data
                for (int x = leftEdge; x < rightEdge; ++x) {
                    int byte = x / 8;
                    int bit = 0x80 >> (x & 0x7);
                    if (sbyte[byte] & bit) {
                        dbyte[x] = color;
                    }
                }
                sbyte += gDirectText->physicalWidth;
                dbyte += rowBytes;
            }
        }
        // else (not on screen) just increase the current character

        // for every char clipped or no:
        // increase our character pixel starting point by width of this character
        hchar += width;

        // increase our hposition (our position in pixels)
        pen.h += width;
    }
    MoveTo(pen.h, pen.v + gDirectText->ascent);
}

}  // namespace antares
