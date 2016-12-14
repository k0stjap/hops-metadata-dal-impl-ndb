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
 * File:   GenericMsgHandlingThread.cpp
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

template<typename MsgType, typename MsgHandler>
GenericMsgHandlingThread<MsgType,MsgHandler>::
GenericMsgHandlingThread(unsigned int nQueues,
			 MsgHandler *msgHandler)
  : nQs(nQueues),
    mqs(new GenericMsgP2PQueueHead<MsgType>[nQueues]),
    lastMsg(new MsgType*[nQueues]),
    msgHandler(msgHandler),
    syncObj(new DataflowController()),
    started(false), running(true)
{
  for (unsigned int i = 0; i < nQueues; i++)
    lastMsg[i] = new MsgType();
}

template<typename MsgType, typename MsgHandler>
GenericMsgHandlingThread<MsgType,MsgHandler>::~GenericMsgHandlingThread()
{
  assert(!running);
  
  delete [] mqs;
  delete syncObj;
}

template<typename MsgType, typename MsgHandler>
DataflowController*
GenericMsgHandlingThread<MsgType,MsgHandler>::getDataflowController()
{
  assert(!started);
  return (syncObj);
}

template<typename MsgType, typename MsgHandler>
GenericMsgP2PQueueHead<MsgType>*
GenericMsgHandlingThread<MsgType,MsgHandler>::getQueueHead(unsigned int n)
{
  assert(!started);
  assert(n < nQs);
  return (&mqs[n]);
}

bool GenericMsgHandlingThread<MsgType,MsgHandler>::start()
{
  assert(!started);
  if (pthread_create(&nativeThread, NULL, (void* (*)(void*)) runproc, (void *) this)) {
    LOG_ERROR("failed to create a message handling thread");
    return (false);
  } else {
    LOG_INFO("started a message handling thread");
    started = true;
    return (true);
  }
}

bool GenericMsgHandlingThread<MsgType,MsgHandler>::stop()
{
  assert(started);
  LOG_INFO("stop a message handling thread");
  running = false;
  if (pthread_join(nativeThread, (void **) NULL)) {
    LOG_ERROR("failed to join with a message handling thread");
    return (false);
  } else {
    return (true);
  }
}

void* GenericMsgHandlingThread<MsgType,MsgHandler>::runproc(void* gptr)
{
  GenericMsgHandlingThread<MsgType,MsgHandler> *gmht = 
    reinterpret_cast<GenericMsgHandlingThread<MsgType,MsgHandler> *>(gptr);
  GenericMsgP2PQueueHead<MsgType>* const mqs = gmht->mqs;
  unsigned int const nQs = gmht->nQs;

  //
  while (gmht->running) {
    // keep scanning all incoming queues round-robin, one message at a
    // time, until there are messages somewhere.
    unsigned int accProcessedMsgs;
    do {
      accProcessedMsgs = 0;
      // Spinning over empty queues is relatively inexpensive, as both
      // the queue heads and the message objects pointed by queue heads
      // should be in the cache from the last iteration.
      for (unsigned int i = 0; i < nQs; i++) {
	// prefetch the next queue's next element (if implemented, and
	// if the element is present, of course). Note if nQs==1 then
	// the next queue is this same queue.
	mqs[gmht->nextQueueIdx(i)].prefetchNext();

	MsgType* msg = mqs[i].dequeue();
	if (msg != (MsgType *) NULL) {
	  // "delayed" reclamation of messages (see
	  // GenericMsgP2PQueue.h for further details): since msg, as
	  // of this point in the message handler thread's execution,
	  // is still used by the queue implementation, it could not
	  // be reclaimed even when the message handler itself will no
	  // longer needed it.
	  if (lastMsg[i]->safeToReclaim())
	    // this means the message handler "released" the message
	    // object earlier, now the object is free and should be
	    // reclaimed:
	    msgHandler->reclaimMsgObj(lastMsg[i]);
	  lastMsg[i] = msg;

	  // handle the message itself;
	  msg->saveSenderIdx(i);
	  msgHandler->handleMsg(msg);

	  accProcessedMsgs++;
	}
      }
    } while (accProcessedMsgs > 0);

    // run out of messages to process: start dataflow synchronization
    // with listener(s), see DataflowController.h for details. 
    // First, tell the producer(s):
    syncObj->waitingForData(); 

    // check one more time - but do not process yet - the queues.
    bool lastMinuteData = false;
    for (unsigned int i = 0; i < nQs; i++) {
      if (!mqs[i].isEmpty()) {
	lastMinuteData = true;
	break;
      }
    }

    // block - and wait for a signal - if needed:
    if (!lastMinuteData)
      syncObj->waitForData();
    // .. either way, there should be some data now:
    syncObj->resumeWithData();
    // resume operation - in the outer loop;
  }
}
