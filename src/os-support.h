/* Gipmsg - Local networt message communication software
 *  
 * Copyright (C) 2012 tsl0922<tsl0922@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _OS_SUPPORT_H_
#define _OS_SUPPORT_H_

#ifdef _LINUX
	typedef int SOCKET;
	typedef unsigned long  ulong;
#endif

#ifdef WIN32
    typedef int socklen_t;
	typedef unsigned long  ulong;
#endif

/*
 * General Socket Defines
 */
#ifdef _LINUX
#define BIND(a,b,c)            bind(a,b,c)
#define ACCEPT(a,b,c)          accept(a,b,c)
#define CONNECT(a,b,c)         connect(a,b,c)
#define CLOSE(a)               close(a)
#define READ(a,b,c)            read(a,b,c)
#define RECV(a,b,c,d)          recv(a,b,c,d)
#define RECVFROM(a,b,c,d,e,f)  recvfrom(a,b,c,d,(struct sockaddr *)e,f)
#define SELECT(a,b,c,d,e)      select(a,b,c,d,e)
#define SEND(a,b,c,d)          send(a,b,c,d)
#define SENDTO(a,b,c,d,e,f)    sendto(a,b,c,d,e,f)
#define WRITE(a,b,c)           write(a,b,c)
#define GETSOCKOPT(a,b,c,d,e)  getsockopt(a,b,c,d,e)
#define SETSOCKOPT(a,b,c,d,e)  setsockopt(a,b,c,d,e)
#define GETHOSTNAME(a,b)       gethostname(a,b)
#define GETHOSTBYNAME(a)       gethostbyname(a)
#endif

#ifdef WIN32
#define BIND(a,b,c)            bind(a,b,c)
#define ACCEPT(a,b,c)          accept(a,b,c)
#define CONNECT(a,b,c)         connect(a,b,c)
#define CLOSE(a)               closesocket(a)
#define READ(a,b,c)            read(a,b,c)
#define RECV(a,b,c,d)          recv(a,b,c,d)
#define RECVFROM(a,b,c,d,e,f)  recvfrom(a,b,c,d,(struct sockaddr *)e,f)
#define SELECT(a,b,c,d,e)      select(a,b,c,d,e)
#define SEND(a,b,c,d)          send(a,b,c,d)
#define SENDTO(a,b,c,d,e,f)    sendto(a,b,c,d,e,f)
#define WRITE(a,b,c)           write(a,b,c)
#define GETSOCKOPT(a,b,c,d,e)  getsockopt(a,b,c,d,e)
#define SETSOCKOPT(a,b,c,d,e)  setsockopt(a,b,c,d,e)
#define GETHOSTNAME(a,b)       gethostname(a,b)
#define GETHOSTBYNAME(a)       gethostbyname(a)
#define mkdir(a,b);            mkdir(a)
#endif

#if defined(WIN32)
  #define GET_CLOCK_COUNT(x) QueryPerformanceCounter((LARGE_INTEGER *)x)
#else
  #define GET_CLOCK_COUNT(x) gettimeofday(x, NULL)
#endif

typedef enum {false, true} bool;

#endif /*_OS_SUPPORT_H_*/

