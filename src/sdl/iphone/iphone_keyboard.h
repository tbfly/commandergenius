/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sf.net/svnroot/scummvm/scummvm/trunk/backends/platform/iphone/iphone_keyboard.h $
 * $Id: iphone_keyboard.h 39020 2009-03-01 08:41:03Z vinterstum $
 *
 */

#import <UIKit/UIKit.h>
#import <UIKit/UITextView.h>

@interface SoftKeyboard : UIView {
	id inputDelegate;
	UITextView* inputView;
}

- (id)initWithFrame:(CGRect)frame;
- (UITextView*)inputView;
- (void)setInputDelegate:(id)delegate;
- (void)handleKeyPress:(unichar)c;

@end
