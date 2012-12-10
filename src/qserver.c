/*
 * See Copyright Notice in qnode.h
 */

#include <stdio.h>
#include "qassert.h"
#include "qconfig.h"
#include "qengine.h"
#include "qlog.h"
#include "qmalloc.h"
#include "qmailbox.h"
#include "qnet.h"
#include "qserver.h"
#include "qthread.h"

static void server_accept(int fd, int flags, void *data) {
  qinfo("add a connection....")
}

static int init_server_event(struct qserver_t *server) {
  int fd = qnet_tcp_server(22222, "127.0.0.1");
  if (fd < 0) {
    return -1;
  }

  qengine_t *engine = server->engine;
  if (qengine_add_event(engine, fd, QEVENT_READ, server_accept, server) < 0) {
    return -1;
  }
  return 0;
}

static void server_box(int fd, int flags, void *data) {
}

qserver_t *qserver_create(struct qconfig_t *config) {
  qassert(config);
  qassert(config->thread_num > 0);
  qserver_t *server = qalloc_type(qserver_t);
  server->thread_num = config->thread_num;
  server->engine = qengine_new();
  if (init_server_event(server) < 0) {
    qengine_destroy(server->engine);
    qfree(server);
    return NULL;
  }
  /* alloc threads and mailboxs */
  server->threads = (qthread_t**)qmalloc(server->thread_num * sizeof(qthread_t*));
  qalloc_assert(server->threads);
  server->threads[0] = NULL;
  server->boxs = (qmailbox_t**)qmalloc(server->thread_num * sizeof(qmailbox_t*));
  qalloc_assert(server->boxs);
  server->boxs[0] = NULL;
  server->thread_boxs = (qmailbox_t**)qmalloc(server->thread_num * sizeof(qmailbox_t*));
  qalloc_assert(server->thread_boxs);
  server->thread_boxs[0] = NULL;
  int i;
  for (i = 1; i <= server->thread_num; ++i) {
    server->boxs[i] = qmailbox_new(server_box, server);
    qassert(server->boxs[i]);
    server->threads[i] = qthread_new(server, i); 
    qassert(server->threads[i]);
    server->thread_boxs[i] = qthread_mailbox(server->threads[i]);
  }
  qinfo("qserver started...");
  return server;
}

qmailbox_t *qserver_get_box(qserver_t *server, int tid) {
  return server->boxs[tid];
}
