/*   $OSSEC, client-config.h, v0.2, 2005/11/09, Daniel B. Cid$   */

/* Copyright (C) 2003-2006 Daniel B. Cid <dcid@ossec.net>
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 3) as published by the FSF - Free Software
 * Foundation
 */


#ifndef __CAGENTD_H

#define __CAGENTD_H

/* Configuration structure */
typedef struct _agent
{
    int port;
    int m_queue;
    int sock;
    int execdq;
	char *rip; /* remote (server) ip */
}agent;


#endif


/* EOF */
