#pragma once

#include "rpi.h"

#define JS_EVENT_BUTTON		0x01	/* button pressed/released */
#define JS_EVENT_AXIS		0x02	/* joystick moved */
#define JS_EVENT_INIT		0x80	/* initial state of device */

typedef struct {
	uint32_t time;	/* event timestamp in milliseconds */
	int16_t value;	/* value */
	uint8_t type;	/* event type */
	uint8_t number;	/* axis/button number */
} js_event;

/**
 * Current state of an axis.
 */
typedef struct  {
    int16_t x, y;
} axis_state;

/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(js_event* event, axis_state* axes)
{
    size_t axis = event->number / 2;

    if (event->number % 2 == 0)
        axes[axis].x = event->value;
    else
        axes[axis].y = event->value;

    return axis;
}
