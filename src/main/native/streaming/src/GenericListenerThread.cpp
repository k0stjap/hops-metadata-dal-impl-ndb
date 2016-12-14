/*
 * Copyright (C) 2016 Hops.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* 
 * File:   GenericListenerThread.cpp
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#include "GenericListenerThread.h"

template<class Listener>
GenericListenerThread::GenericListenerThread(unsigned int nListeners)
  : started(false), running(true), 
    nListeners(nListeners),
    listeners(new Listener*[nListeners]),
    syncObj(new DataflowController*[nListeners]),
    pollTimeout(ULONG_MAX),
    blockingListenerId(0)
{}

template<class Listener>
GenericListenerThread::~GenericListenerThread()
{
  assert(!running);
  for (unsigned int i = 0; i < nListeners; i++)
    if (listeners[i] != (Listener *) NULL)
      delete listeners[i];
  delete [] listeners;
  delete [] syncObj;
}

template<class Listener>
void 
GenericListenerThread::registerListener(unsigned int n,
					Listener *listener,
					DataflowController *dfc)
{
  // synchronization between the method calling thread and the
  // object's "own" thread is not implemented:
  assert(!started);
  assert(n <= nListeners);
  listeners[n] = listener;
  syncObj[n] = dfc;
  //
  unsigned long const listenerTimeout = listener->getTimeout();
  if (listenerTimeout < pollTimeout)
    pollTimeout = listenerTimeout;
}

// run on full round of handleEventsNonBlocking() for non-zero
// listeners in the array of nListeners elements, discarding
// (replacing with NULL pointer) those that had no events. Return the
// total number of processed events. Note that if a non-zero value is
// returned, some of the listeners are (still) non-zero.
static unsigned int eventProcessingRound(unsigned int const nListeners,
					 Listener * const * const listeners,
					 DataflowController * const * const syncObj,
					 char *const pendingWakeup)
{
  unsigned int totalProcessed = 0;
  for (unsigned int i = 0; i < nListeners; i++) {
    Listener *gl = listeners[i];
    if (gl != (Listener *) NULL) {
      unsigned int nProcessed;
      nProcessed = gl->handleEventsNonBlocking();

      if (nProcessed > 0) {
	totalProcessed += nprocessed;
	// regardless whether we had a pending wakeup, try it now:
	pendingWakeup[i] = syncObj[i]->checkConsumer();
      } else {
	listeners[i] = (Listener *) NULL;
	// .. and even if no messages are sent this time, we still
	// might have a pending wakeup:
	if (pendingWakeup[i])
	  pendingWakeup[i] = syncObj[i]->checkConsumer();
      }
    } else if (pendingWakeup[i]) {
      pendingWakeup[i] = syncObj[i]->checkConsumer();
    }
  }
  return (totalProcessed);
}

//
void* GenericListenerThread::runproc(void* gptr)
{
  unsigned int i;
  GenericListenerThread *glt = reinterpret_cast<GenericListenerThread *>(gptr);
  const unsigned int nListeners = glt->nListeners;
  Listener * const * const listeners = glt->listeners;
  DataflowController * const * const syncObj = glt->syncObj;
  Listener ** const runningListeners = new Listener*[nListeners];
  // one boolean flag per listener telling whether a particular
  // listener needs a wakeup (because previous attempt by
  // DataflowController::checkConsumer() was unsuccessful)
  char * const pendingWakeup = new char[nListeners];

  assert(nListeners > 0);
  for (i = 0; i < nListeners; i++) {
    runningListeners[i] = listeners[i];
    pendingWakeup[i] = false;
  }

  // loop invariant: runningListeners[] keep track of listeners that
  // might still have events to process;
  while (glt->running) {
    // keep running the handleEventsNonBlocking() methods, as long as
    // there are processed events somewhere:
    unsigned int accProcessedEvents = 0;
    unsigned int processedEvents;
    do {
      processedEvents = 
	eventProcessingRound(nListeners, runningListeners, syncObj, pendingWakeup);
      accProcessedEvents += processedEvents;
    } while (processedEvents > 0);

    // once run out of work, "rescan" all listeners once - provided we
    // did find some work in this round at all:
    if (accProcessedEvents > 0) {
      for (i = 0; i < nListeners; i++)
	runningListeners[i] = listeners[i];
      if (eventProcessingRound(nListeners, runningListeners, 
			       syncObj, pendingWakeup) > 0)
	continue;	    // .. with those that did have events now;
    }
    
    // now we're definitely out of work: wait for it with the listener
    // with the smallest timeout value. But before that check the
    // consumers' status the "hard way", i.e. forcing proper delivery
    // of wakeup signals where needed:
    for (i = 0; i < nListeners; i++) {
      if (pendingWakeup[i]) {
	syncObj[i]->checkConsumerSync();
	pendingWakeup[i] = false;
      }
    }
    //
    unsigned int np = listeners[blockingListenerId]->handleEventsTimeout();

    // reset the runningListeners[] (since some time lapsed):
    for (i = 0; i < nListeners; i++)
      runningListeners[i] = listeners[i];
    if (np == 0)
      // .. but no point yet to re-try this particular one:
      runningListeners[blockingListenerId] = (Listener *) NULL;
  }

  delete [] runningListeners;
  delete [] pendingWakeup;
  return ((void *) NULL);
}

bool GenericListenerThread::start()
{
  assert(!started);
  if (pthread_create(&nativeThread, NULL, (void* (*)(void*)) runproc, (void *) this)) {
    LOG_ERROR("failed to create a listener thread");
    return (false);
  } else {
    LOG_INFO("started a listener thread");
    started = true;
    return (true);
  }
}

bool GenericListenerThread::stop()
{
  assert(started);
  LOG_INFO("stop a listener thread");
  running = false;
  if (pthread_join(nativeThread, (void **) NULL)) {
    LOG_ERROR("failed to join with a listener thread");
    return (false);
  } else {
    return (true);
  }
}
