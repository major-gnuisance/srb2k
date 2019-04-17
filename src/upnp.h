// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_tcp.h
/// \brief TCP driver, sockets.

#ifdef HAVE_MINIUPNPC

extern boolean UPNP_support;

/**	\brief InitUPnP function

	\return	none
*/
void InitUPnP(void);

/**	\brief AddPortMapping function

	\return	none
*/
void AddPortMapping(const char *addr, const char *port);

/**	\brief DeletePortMapping function

	\return	none
*/
void DeletePortMapping(const char *port);

#endif
