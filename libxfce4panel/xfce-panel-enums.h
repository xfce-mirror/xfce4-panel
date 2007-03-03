/* $Id$
 *
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __XFCE_PANEL_ENUMS_H__
#define __XFCE_PANEL_ENUMS_H__

G_BEGIN_DECLS
/**
 * XfceScreenPosition
 * @XFCE_SCREEN_POSITION_NONE       : No position has been set.
 * @XFCE_SCREEN_POSITION_NW_H       : North West Horizontal
 * @XFCE_SCREEN_POSITION_N          : North
 * @XFCE_SCREEN_POSITION_NE_H       : North East Horizontal
 * @XFCE_SCREEN_POSITION_NW_V       : North West Vertical
 * @XFCE_SCREEN_POSITION_W          : West
 * @XFCE_SCREEN_POSITION_SW_V       : South West Vertical
 * @XFCE_SCREEN_POSITION_NE_V       : North East Vertical
 * @XFCE_SCREEN_POSITION_E          : East
 * @XFCE_SCREEN_POSITION_SE_V       : South East Vertical
 * @XFCE_SCREEN_POSITION_SW_H       : South West Horizontal
 * @XFCE_SCREEN_POSITION_S          : South
 * @XFCE_SCREEN_POSITION_SE_H       : South East Horizontal
 * @XFCE_SCREEN_POSITION_FLOATING_H : Floating Horizontal
 * @XFCE_SCREEN_POSITION_FLOATING_V : Floating Vertical
 *
 * There are three screen positions for each side of the screen:
 * LEFT/TOP, CENTER and RIGHT/BOTTOM. The XfceScreenPosition is expressed
 * as navigational direction, with possible addition of H or V to denote
 * horizontal and vertical orientation. Additionally there are two floating
 * positions, horizontal and vertical.
 **/
typedef enum /*<enum,prefix=XFCE_SCREEN_POSITION >*/
{
    XFCE_SCREEN_POSITION_NONE,

    /* top */
    XFCE_SCREEN_POSITION_NW_H,          /* North West Horizontal */
    XFCE_SCREEN_POSITION_N,             /* North                 */
    XFCE_SCREEN_POSITION_NE_H,          /* North East Horizontal */

    /* left */
    XFCE_SCREEN_POSITION_NW_V,          /* North West Vertical   */
    XFCE_SCREEN_POSITION_W,             /* West                  */
    XFCE_SCREEN_POSITION_SW_V,          /* South West Vertical   */

    /* right */
    XFCE_SCREEN_POSITION_NE_V,          /* North East Vertical   */
    XFCE_SCREEN_POSITION_E,             /* East                  */
    XFCE_SCREEN_POSITION_SE_V,          /* South East Vertical   */

    /* bottom */
    XFCE_SCREEN_POSITION_SW_H,          /* South West Horizontal */
    XFCE_SCREEN_POSITION_S,             /* South                 */
    XFCE_SCREEN_POSITION_SE_H,          /* South East Horizontal */

    /* floating */
    XFCE_SCREEN_POSITION_FLOATING_H,    /* Floating Horizontal */
    XFCE_SCREEN_POSITION_FLOATING_V     /* Floating Vertical */
}
XfceScreenPosition;

/**
 * xfce_screen_position_is_horizontal
 * @position : the #XfceScreenPosition
 *
 * Returns: %TRUE if horizontal, %FALSE otherwise
 **/
#define xfce_screen_position_is_horizontal(position)   \
    (position <= XFCE_SCREEN_POSITION_NE_H ||          \
     (position >= XFCE_SCREEN_POSITION_SW_H &&         \
      position <= XFCE_SCREEN_POSITION_FLOATING_H))

/**
 * xfce_screen_position_get_orientation
 * @position : the #XfceScreenPosition
 *
 * Returns: the #GtkOrientation corresponding to @position.
 **/
#define xfce_screen_position_get_orientation(position) \
    (xfce_screen_position_is_horizontal (position) ? \
        GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL)

/**
 * xfce_screen_position_is_floating
 * @position : the #XfceScreenPosition
 *
 * Returns: %TRUE if floating, %FALSE otherwise.
 **/
#define xfce_screen_position_is_floating(position) \
    (position >= XFCE_SCREEN_POSITION_FLOATING_H || \
     position == XFCE_SCREEN_POSITION_NONE)

/**
 * xfce_screen_position_is_top
 * @position : the #XfceScreenPosition
 *
 * Returns: %TRUE if on the top of the screen, %FALSE otherwise
 **/
#define xfce_screen_position_is_top(position) \
    (position >= XFCE_SCREEN_POSITION_NW_H && \
     position <= XFCE_SCREEN_POSITION_NE_H)

/**
 * xfce_screen_position_is_left
 * @position : the #XfceScreenPosition
 *
 * Returns: %TRUE if on the left of the screen, %FALSE otherwise
 **/
#define xfce_screen_position_is_left(position) \
    (position >= XFCE_SCREEN_POSITION_NW_V && \
     position <= XFCE_SCREEN_POSITION_SW_V)

/**
 * xfce_screen_position_is_right
 * @position : the #XfceScreenPosition
 *
 * Returns: %TRUE if on the right of the screen, %FALSE otherwise
 **/
#define xfce_screen_position_is_right(position) \
    (position >= XFCE_SCREEN_POSITION_NE_V && \
     position <= XFCE_SCREEN_POSITION_SE_V)

/**
 * xfce_screen_position_is_bottom
 * @position : the #XfceScreenPosition
 *
 * Returns: %TRUE if on the bottom of the screen, %FALSE otherwise
 **/
#define xfce_screen_position_is_bottom(position) \
    (position >= XFCE_SCREEN_POSITION_SW_H)

G_END_DECLS

#endif /* !__PANEL_ENUMS_H__ */
