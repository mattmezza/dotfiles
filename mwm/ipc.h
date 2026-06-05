/* See LICENSE file for copyright and license details. */
#ifndef MWM_IPC_H
#define MWM_IPC_H

/*
 * mwm IPC protocol (pure X11, no sockets, no extra deps)
 * ------------------------------------------------------
 * mwmc (the client):
 *   1. opens its own X connection and creates a tiny unmapped window W,
 *      selecting PropertyChangeMask on it;
 *   2. stores the command as NUL-separated argv in property MWM_ATOM_CMD on W;
 *   3. sends a ClientMessage of type MWM_ATOM_MSG to the root window with
 *      data.l[0] == W;
 *   4. blocks until mwm writes property MWM_ATOM_REPLY back onto W, prints it,
 *      and exits non-zero when the reply starts with "error:".
 *
 * mwm (the window manager) handles MWM_ATOM_MSG in its event loop, reads the
 * command off the requestor window, dispatches it, and writes the reply back
 * onto the same window. No global root-property contention, so any number of
 * mwmc invocations can run concurrently.
 */

#define MWM_ATOM_MSG    "_MWM_MSG"     /* ClientMessage type sent to root      */
#define MWM_ATOM_CMD    "_MWM_CMD"     /* UTF8 prop on requestor: NUL-sep argv */
#define MWM_ATOM_REPLY  "_MWM_REPLY"   /* UTF8 prop on requestor: command reply*/

#endif /* MWM_IPC_H */
