#include "client.h"
#include "../capture/capture.h"
#include "../cyborg/control.h"
#include "../data/stack.h"
#include "../util/util.h"
#include "read_state.h"
#include "uv.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void on_connect(uv_connect_t *connection, int status);
void on_close_cb(uv_handle_t *handle);

struct NetClient *setup_client(struct DStack *frame_stack,
                               struct DStack *ctrl_stack) {
  int err;
  struct NetClient *client;
  client = malloc(sizeof(*client));
  if (!client)
    return NULL;
  memset(client, 0, sizeof(*client));
  client->loop = uv_default_loop();
  client->frame_stack = frame_stack;
  client->ctrl_stack = ctrl_stack;
  client->read_state = calloc(1, sizeof(*client->read_state));
  uv_cond_init(&client->cond);
  uv_mutex_init(&client->mutex);
  return client;
}

int destroy_client(struct NetClient *client) {
  if (client->tcp_stream)
    uv_close((uv_handle_t *)client->tcp_stream, on_close_cb);
  if (client->tcp_socket)
    free(client->tcp_socket);
  uv_loop_close(client->loop);
  free(client);
  return 0;
}

int connect_client(struct NetClient *c, const char *ip) {
  uv_connect_t *conn_req;
  c->tcp_socket = malloc(sizeof(*c->tcp_socket));
  uv_tcp_init(c->loop, c->tcp_socket);
  uv_tcp_keepalive(c->tcp_socket, 1, 60);
  struct sockaddr_in dest;
  uv_ip4_addr(ip, CALM_PORT, &dest);
  conn_req = malloc(sizeof(*conn_req));
  conn_req->data = c;
  uv_tcp_connect(conn_req, c->tcp_socket, (const struct sockaddr *)&dest,
                 on_connect);
  return uv_run(c->loop, UV_RUN_DEFAULT);
}

static void read_cframe_alloc_cb(uv_handle_t *handle, size_t size,
                                 uv_buf_t *buf) {
  struct NetClient *client = (struct NetClient *)handle->data;
  struct ReadState *read_state = client->read_state;
  switch (read_state->state) {
  case AllocateBufferLength:
    // Allocate 8 bytes to read the start of a cframe
    read_state->buf_len_buffer =
        malloc(sizeof(*read_state->buf_len_buffer) * 4);
    *buf = uv_buf_init((char *)read_state->buf_len_buffer, 4);
    break;
  case FillBufferLength:
    // Fill up the allocated frame buffer length
    read_state->current_offset = 0;
    *buf = uv_buf_init((char *)read_state->buf_len_buffer +
                           read_state->current_offset,
                       8 - read_state->current_offset);
    break;

  case AllocatePacketTypeLength:
    // Allocate 8 bytes to read the start of a cframe
    read_state->packet_type_buffer =
        malloc(sizeof(*read_state->packet_type_buffer) * 4);
    *buf = uv_buf_init((char *)read_state->packet_type_buffer, 4);
    break;
  case FillPacketTypeLength:
    // Fill up the allocated frame buffer length
    read_state->current_offset = 0;
    *buf = uv_buf_init((char *)read_state->packet_type_buffer +
                           read_state->current_offset,
                       8 - read_state->current_offset);
    break;

  case AllocateBuffer:
    // Allocate bytes required to read the frame
    read_state->buffer =
        malloc(sizeof(*read_state->buffer) * read_state->buf_len);
    *buf = uv_buf_init((char *)read_state->buffer, read_state->buf_len);
    break;
  case FillBuffer:
    // Fill up the allocated frame buffer
    read_state->current_offset = 0;
    *buf =
        uv_buf_init((char *)(read_state->buffer + read_state->current_offset),
                    read_state->buf_len - read_state->current_offset);
    break;

  default:
    /* UNREACHABLE */
    break;
  }
}

void on_close_cb(uv_handle_t *handle) {
#if 0
  printf("connection closed\n")
#endif
  free(handle);
}

void on_write(uv_write_t *req, int status) {
  if (status)
    uv_close((uv_handle_t *)req->handle, on_close_cb);
  free(req);
}

// NOTE: https://groups.google.com/g/libuv/c/fRNQV_QGgaA

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  struct CFrame *frame;
  struct Control *ctrl;
  struct NetClient *client = (struct NetClient *)stream->data;

  if (nread < 0) {
    if (nread != UV_EOF) {
      printf("Read error %d %s\n", client->read_state->state,
             uv_err_name(nread));
      // uv_close((uv_handle_t *)stream, on_close_cb);
    }
  }

  client->read_state->current_offset += nread;

  switch (client->read_state->state) {
  case AllocateBufferLength:
    if (client->read_state->current_offset != 4) {
      client->read_state->state = FillBufferLength;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case FillBufferLength:
    if (client->read_state->current_offset != 4) {
      break;
    }
    client->read_state->buf_len =
        read_uint64(client->read_state->buf_len_buffer);
    free(client->read_state->buf_len_buffer);
    client->read_state->state = AllocatePacketTypeLength;
    client->read_state->current_offset = 0;
    break;

  case AllocatePacketTypeLength:
    if (client->read_state->current_offset != 4) {
      client->read_state->state = FillPacketTypeLength;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case FillPacketTypeLength:
    if (client->read_state->current_offset != 4) {
      break;
    }
    client->read_state->buf_len =
        read_uint32(client->read_state->packet_type_buffer);
    free(client->read_state->packet_type_buffer);
    client->read_state->state = AllocateBuffer;
    client->read_state->current_offset = 0;
    break;

  case AllocateBuffer:
    if ((uint64_t)client->read_state->current_offset !=
        client->read_state->buf_len) {
      client->read_state->state = FillBuffer;
      break;
    }
    /* FALLTHROUGH IF BUFFER IS FULL */
  case FillBuffer:
    if (client->read_state->current_offset != client->read_state->buf_len) {
      break;
    }
    switch (client->read_state->packet_type) {
    case 1:
      frame = unmarshal_cframe(client->read_state->buffer,
                               client->read_state->buf_len);
      dstack_push(client->frame_stack, (void *)frame, 1);
      break;
    case 2:
      ctrl = ctrl_unmarshal_control(client->read_state->buffer,
                                    client->read_state->buf_len);
      dstack_push(client->frame_stack, (void *)frame, 1);
      break;
    default:
      printf("unknown packet type: %d\n", client->read_state->packet_type);
      break;
    }
    free(client->read_state->buffer);
    memset(client->read_state, 0, sizeof(*client->read_state));
    break;
  default:
    if (buf->base)
      free(buf->base);
    break;
  }
}

void write_stream(uv_stream_t *stream, char *data, int len2) {
  uv_buf_t buffer[] = {{.base = data, .len = len2}};
  uv_write_t *req = malloc(sizeof(uv_write_t));
  uv_write(req, stream, buffer, 1, on_write);
}

void on_connect(uv_connect_t *connection_req, int status) {
  struct NetClient *client = (struct NetClient *)connection_req->data;

  if (status < 0)
    printf("conn failed %d\n", status);
  else
    printf("connected.\n");

  client->tcp_stream = connection_req->handle;
  client->tcp_stream->data = client;
  free(connection_req);
  write_stream(client->tcp_stream, "echo  world!", 12);
  uv_read_start(client->tcp_stream, read_cframe_alloc_cb, on_read);
}
