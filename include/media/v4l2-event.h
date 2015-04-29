/*
 * v4l2-event.h
 *
 * V4L2 events.
 *
 * Copyright (C) 2009--2010 Nokia Corporation.
 *
 * Contact: Sakari Ailus <sakari.ailus@maxwell.research.nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef V4L2_EVENT_H
#define V4L2_EVENT_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/wait.h>

/*
 * Overview:
 *
 * Events are subscribed per-filehandle. An event specification consists of a
 * type and is optionally associated with an object identified through the
 * 'id' field. So an event is uniquely identified by the (type, id) tuple.
 *
 * The v4l2-fh struct has a list of subscribed events. The v4l2_subscribed_event
 * struct is added to that list, one for every subscribed event.
 *
 * Each v4l2_subscribed_event struct ends with an array of v4l2_kevent structs.
 * This array (ringbuffer, really) is used to store any events raised by the
 * driver. The v4l2_kevent struct links into the 'available' list of the
 * v4l2_fh struct so VIDIOC_DQEVENT will know which event to dequeue first.
 *
 * Finally, if the event subscription is associated with a particular object
 * such as a V4L2 control, then that object needs to know about that as well
 * so that an event can be raised by that object. So the 'node' field can
 * be used to link the v4l2_subscribed_event struct into a list of that
 * object.
 *
 * So to summarize:
 *
 * struct v4l2_fh has two lists: one of the subscribed events, and one of the
 * pending events.
 *
 * struct v4l2_subscribed_event has a ringbuffer of raised (pending) events of
 * that particular type.
 *
 * If struct v4l2_subscribed_event is associated with a specific object, then
 * that object will have an internal list of struct v4l2_subscribed_event so
 * it knows who subscribed an event to that object.
 */

struct v4l2_fh;
struct video_device;

struct v4l2_kevent {
	struct list_head	list;
	struct v4l2_event	event;
};

struct v4l2_subscribed_event {
	struct list_head	list;
	u32			type;
};

struct v4l2_events {
	wait_queue_head_t	wait;
	struct list_head	subscribed;
	struct list_head	free;
	struct list_head	available;
	unsigned int		navailable;
	unsigned int		nallocated;
	u32			sequence;
};

int v4l2_event_init(struct v4l2_fh *fh);
int v4l2_event_alloc(struct v4l2_fh *fh, unsigned int n);
void v4l2_event_free(struct v4l2_fh *fh);
int v4l2_event_dequeue(struct v4l2_fh *fh, struct v4l2_event *event,
		       int nonblocking);
void v4l2_event_queue(struct video_device *vdev, const struct v4l2_event *ev);
int v4l2_event_pending(struct v4l2_fh *fh);
int v4l2_event_subscribe(struct v4l2_fh *fh,
			 struct v4l2_event_subscription *sub);
int v4l2_event_unsubscribe(struct v4l2_fh *fh,
			   struct v4l2_event_subscription *sub);

#endif /* V4L2_EVENT_H */
