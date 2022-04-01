/* *****************************************************************************
Copyright: Boaz Segev, 2019-2020
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */

/* *****************************************************************************
This is a simple HTTP "Hello World" / echo server example using `poll`.

Benchmark with keep-alive:

    ab -c 200 -t 4 -n 1000000 -k http://127.0.0.1:3000/
    wrk -c200 -d4 -t2 http://localhost:3000/

Note: This is a **TOY** example, with only minimal security features.
***************************************************************************** */

/* include some of the modules we use... */
#define FIO_LOG
#define FIO_CLI
#define FIO_SERVER
// #define FIO_MEMORY_DISABLE 1
// #define FIO_USE_THREAD_MUTEX 1
#include "fio-stl.h"

/* we use local global variables to make the code easier. */

#define HTTP_CLIENT_BUFFER 32768
#define HTTP_MAX_BODY_SIZE (1 << 27)
#define HTTP_MAX_HEADERS   16
#define HTTP_TIMEOUTS      5000

/* an echo response (vs. Hello World). */
#define HTTP_RESPONSE_ECHO 1

/* *****************************************************************************
Callbacks and object used by main()
***************************************************************************** */
#include "http/http-handle.h"

/** Called when a new connection is created. */
FIO_SFUNC void on_open(int fd, void *udata);
/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io);
/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *udata);

fio_protocol_s HTTP_PROTOCOL_1 = {
    .on_data = on_data,
    .on_close = on_close,
};

/* *****************************************************************************
The HTTP Response Function
***************************************************************************** */
#define FIO_STR_NAME str
#define FIO_MEM_NAME str_allocator
#include "fio-stl.h"

FIO_SFUNC int http_write_headers_to_string(http_s *h,
                                           fio_str_info_s name,
                                           fio_str_info_s value,
                                           void *out_) {
  str_s *out = (str_s *)out_;
  (void)h;
  str_write(out, name.buf, name.len);
  str_write(out, ": ", 2);
  str_write(out, value.buf, value.len);
  str_write(out, "\r\n", 2);
  return 0;
}

FIO_SFUNC void http_respond(http_s *h) {
#if HTTP_RESPONSE_ECHO
  str_s out = FIO_STR_INIT;
  str_write(&out, http_method_get(h).buf, http_method_get(h).len);
  str_write(&out, " ", 1);
  str_write(&out, http_path_get(h).buf, http_path_get(h).len);
  if (http_query_get(h).len) {
    str_write(&out, "?", 1);
    str_write(&out, http_query_get(h).buf, http_query_get(h).len);
  }
  str_write(&out, " ", 1);
  str_write(&out, http_version_get(h).buf, http_version_get(h).len);
  str_write(&out, "\r\n", 2);
  http_request_header_each(h, http_write_headers_to_string, &out);
  if (http_body_length(h)) {
    fio_str_info_s body = http_body_read(h, -1);
    str_write(&out, "\r\n", 2);
    str_write(&out, body.buf, body.len);
  }
  size_t len = str_len((&out));
  http_write(h,
             .data = str_detach(&out),
             .len = len,
             .dealloc = str_dealloc,
             .copy = 0,
             .finish = 1);

#else
  http_response_header_set(h,
                           FIO_STR_INFO1("server"),
                           FIO_STR_INFO1("fio-stl"));
  http_write(h, .data = "Hello World!", .len = 12, .finish = 1);
#endif
}

/* *****************************************************************************
Starting the program - main()
***************************************************************************** */

int main(int argc, char const *argv[]) {
#ifdef TEST
  FIO_LOG_INFO("Testing HTTP Handle.");
  http_test();
#endif
  /* initialize the CLI options */
  fio_cli_start(argc,
                argv,
                0, /* allow 1 unnamed argument - the address to connect to */
                1,
                "A simple HTTP \"hello world\" example, listening on the "
                "specified URL. i.e.\n"
                "\tNAME <url>\n\n"
                "Unix socket examples:\n"
                "\tNAME unix://./my.sock\n"
                "\tNAME /full/path/to/my.sock\n"
                "\nTCP/IP socket examples:\n"
                "\tNAME tcp://localhost:3000/\n"
                "\tNAME localhost://3000\n",
                FIO_CLI_BOOL("--verbose -V -d print out debugging messages."),
                FIO_CLI_PRINT_LINE(
                    "NOTE: requests are limited to 32Kb and 16 headers each."));

  /* review CLI for logging */
  if (fio_cli_get_bool("-V")) {
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  }
  /* review CLI connection address (in URL format) */
  FIO_ASSERT(!fio_listen(.url = fio_cli_unnamed(0), .on_open = on_open),
             "Could not open listening socket as requested.");
  FIO_LOG_INFO("Starting HTTP echo server. Press ^C to exit.");
  fio_srv_run();
  FIO_LOG_INFO("Shutdown complete.");
  fio_cli_end();
  return 0;
}

/* *****************************************************************************
IO "Objects"and helpers
***************************************************************************** */
#include "../parsers/http1_parser.h"

#include "http/http-handle.c"

typedef struct {
  http1_parser_s parser;
  fio_s *io;
  http_s *h;
  int buf_pos;
  int buf_con;
  char buf[]; /* header and data buffer */
} client_s;

#define FIO_REF_NAME client
#define FIO_REF_CONSTRUCTOR_ONLY
#define FIO_REF_DESTROY(c)                                                     \
  do {                                                                         \
    http_free((c).h);                                                          \
    (c).h = NULL;                                                              \
  } while (0)
#define FIO_REF_FLEX_TYPE char
#include "fio-stl.h"

/** Called when a new connection is created. */
FIO_SFUNC void on_open(int fd, void *udata) {
  client_s *c = client_new(HTTP_CLIENT_BUFFER);
  FIO_ASSERT_ALLOC(c);
  c->io = fio_attach_fd(fd, &HTTP_PROTOCOL_1, c, udata);
}

/* *****************************************************************************
IO callback(s)
***************************************************************************** */

/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data__internal(client_s *c) {
  size_t tmp;
  if ((tmp = http1_parse(&c->parser,
                         c->buf + c->buf_con,
                         (size_t)(c->buf_pos - c->buf_con)))) {
    c->buf_con += tmp;
    if (!http1_complete(&c->parser))
      return;
    if (c->buf_con == c->buf_pos)
      c->buf_pos = c->buf_con = 0;
    else {
      c->buf_pos = c->buf_pos - c->buf_con;
      memmove(c->buf, c->buf + c->buf_con, c->buf_pos);
      c->buf_con = 0;
    }
  }
  return;
}

/** Called there's incoming data (from STDIN / the client socket. */
FIO_SFUNC void on_data(fio_s *io) {
  client_s *c = fio_udata_get(io);
  ssize_t r =
      fio_read(io, c->buf + c->buf_pos, HTTP_CLIENT_BUFFER - c->buf_pos);
  if (r > 0)
    c->buf_pos += r;
  on_data__internal(fio_udata_get(io));
}

/** Called when the monitored IO is closed or has a fatal error. */
FIO_SFUNC void on_close(void *c_) {
  client_s *c = (client_s *)c_;
  c->io = NULL;
  client_free(c);
}

/* *****************************************************************************
HTTP/1.1 Protocol Controller
***************************************************************************** */
/* Use fio_malloc. */
#undef FIO_MEM_REALLOC
#undef FIO_MEM_FREE
#undef FIO_MEM_REALLOC_IS_SAFE
#define FIO_MALLOC
#include "fio-stl.h"

/** Informs the controller that a request is starting. */
int http1_start_request(http_s *h, int reserved, int streaming) {
  (void)reserved;
  (void)streaming;
  client_s *c = http_controller_data(h);
  if (!c->io)
    return -1;
  return -1;
}
/** Called before an HTTP handler link to an HTTP Controller is revoked. */
void http1_on_unlinked(http_s *h, void *c_) {
  client_s *c = c_; // client_s *c = http_controller_data(h);
  if (c->h == h)
    c->h = NULL;
  client_free(c);
  (void)h;
}

/** Informs the controller that a response is starting. */
int http1_start_response(http_s *h, int status, int streaming) {
  (void)status;
  client_s *c = http_controller_data(h);
  if (!c->io)
    return -1;
  if (streaming) {
    /* TODO: add streaming headers */
  }
  return 0;
}

static void http_1___str_free(void *ptr) { FIO_MEM_FREE(ptr, 0); }
static fio_str_info_s http_1___str_reserve(fio_str_info_s dest,
                                           size_t new_capa) {
  void *tmp = FIO_MEM_REALLOC(dest.buf, dest.capa, new_capa, dest.len);
  if (!tmp)
    return dest;
  dest.capa = new_capa;
  dest.buf = (char *)tmp;
  return dest;
}

/** called by the HTTP handle for each header. */
int http1___write_header_callback(http_s *h,
                                  fio_str_info_s name,
                                  fio_str_info_s value,
                                  void *out_) {
  (void)h;
  fio_str_info_s *out = (fio_str_info_s *)out_;
  size_t new_len = out->len + name.len + 1 + value.len + 2;
  if (out->capa < new_len + 1) {
    *out = http_1___str_reserve(
        *out,
        ((new_len + 15LL + (!(new_len & 15ULL))) & (~((size_t)15ULL))));
  }
  if (out->capa < new_len + 1)
    return -1;
  memcpy(out->buf + out->len, name.buf, name.len);
  out->buf[out->len + name.len] = ':';
  memcpy(out->buf + out->len + name.len + 1, value.buf, value.len);
  out->buf[out->len + name.len + value.len + 1] = '\r';
  out->buf[out->len + name.len + value.len + 2] = '\n';
  out->len = new_len;
  return 0;
}

/** Informs the controller that all headers were provided. */
void http1_send_headers(http_s *h) {
  client_s *c = http_controller_data(h);
  if (!c->io)
    return;
  fio_str_info_s buf = FIO_STR_INFO2(NULL, 0);
  /* write status string */
  fio_str_info_s tmp = http_version_get(h);
  if (tmp.len > 15) {
    FIO_LOG_ERROR("HTTP/1.1 client version string too long!");
    tmp = FIO_STR_INFO1("HTTP/1.1");
  }
  buf = fio_str_info_write(buf, http_1___str_reserve, tmp.buf, tmp.len);
  {
    char num_buf[32];
    num_buf[0] = ' ';
    size_t n_len = fio_ltoa(num_buf + 1, http_status_get(h), 10) + 1;
    num_buf[n_len++] = ' ';
    buf = fio_str_info_write(buf, http_1___str_reserve, num_buf, n_len);
  }
  tmp = http_status2str(http_status_get(h));
  buf = fio_str_info_write(buf, http_1___str_reserve, tmp.buf, tmp.len);
  buf = fio_str_info_write(buf, http_1___str_reserve, "\r\n", 2);
  /* write headers */
  http_response_header_each(h, http1___write_header_callback, &buf);
  /* write cookies */
  http_set_cookie_each(h, http1___write_header_callback, &buf);
  buf = fio_str_info_write(buf, http_1___str_reserve, "\r\n", 2);

  fio_write2(c->io,
             .buf = buf.buf,
             .len = buf.len,
             .copy = 0,
             .dealloc = http_1___str_free);
}
/** called by the HTTP handle for each body chunk (or to finish a response. */
void http1_write_body(http_s *h, http_write_args_s args) {
  client_s *c = http_controller_data(h);
  if (!c->io)
    return;
  if (http_is_streaming(h)) {
    /* TODO: add streaming wrapper for chunked data */
  }
  fio_write2(c->io,
             .buf = (void *)args.data,
             .len = args.len,
             .fd = args.fd,
             .dealloc = args.dealloc,
             .copy = args.copy);
  if (args.finish) {
    http_free(c->h);
  }
}

http_controller_s HTTP1_CONTROLLER = {
    .on_unlinked = http1_on_unlinked,
    .start_response = http1_start_response,
    .start_request = http1_start_request,
    .send_headers = http1_send_headers,
    .write_body = http1_write_body,
};

/* *****************************************************************************
HTTP/1.1 callback(s)
***************************************************************************** */

FIO_SFUNC void http_deferred_response(void *h_, void *ignr_) {
  http_s *h = (http_s *)h_;
  (void)ignr_;
  http_respond(h);
  if (!http_is_streaming(h) || !http_is_finished(h))
    http_finish(h);
}

/** called when a request was received. */
static int http1_on_request(http1_parser_s *parser) {
  client_s *c = (client_s *)parser;
  http_status_set(c->h, 200);
  http_respond(c->h);
  return 0;
}

/** called when a response was received. */
static int http1_on_response(http1_parser_s *parser) {
  (void)parser;
  FIO_LOG_ERROR("response received instead of a request. Silently ignored.");
  return -1;
}
/** called when a request method is parsed. */
static int http1_on_method(http1_parser_s *parser,
                           char *method,
                           size_t method_len) {
  client_s *c = (client_s *)parser;
  if (!c->h) {
    c->h = http_new();
    http_controller_set(c->h, &HTTP1_CONTROLLER, client_dup(c));
  }
  http_method_set(c->h, FIO_STR_INFO2(method, method_len));
  return 0;
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int http1_on_status(http1_parser_s *parser,
                           size_t status,
                           char *status_str,
                           size_t len) {
  return -1;
  client_s *c = (client_s *)parser;
  http_status_set(c->h, status);
  (void)parser;
  (void)status;
  (void)status_str;
  (void)len;
}
/** called when a request path (excluding query) is parsed. */
static int http1_on_path(http1_parser_s *parser, char *path, size_t path_len) {
  client_s *c = (client_s *)parser;
  http_path_set(c->h, FIO_STR_INFO2(path, path_len));
  return 0;
}
/** called when a request path (excluding query) is parsed. */
static int http1_on_query(http1_parser_s *parser,
                          char *query,
                          size_t query_len) {
  client_s *c = (client_s *)parser;
  http_query_set(c->h, FIO_STR_INFO2(query, query_len));
  return 0;
}
/** called when a the HTTP/1.x version is parsed. */
static int http1_on_version(http1_parser_s *parser, char *version, size_t len) {
  client_s *c = (client_s *)parser;
  http_version_set(c->h, FIO_STR_INFO2(version, len));
  return 0;
}
/** called when a header is parsed. */
static int http1_on_header(http1_parser_s *parser,
                           char *name,
                           size_t name_len,
                           char *value,
                           size_t value_len) {
  client_s *c = (client_s *)parser;
  http_request_header_add(c->h,
                          FIO_STR_INFO2(name, name_len),
                          FIO_STR_INFO2(value, value_len));
  return 0;
}
/** called when a body chunk is parsed. */
static int http1_on_body_chunk(http1_parser_s *parser,
                               char *data,
                               size_t data_len) {
  client_s *c = (client_s *)parser;
  if (data_len + http_body_length(c->h) > HTTP_MAX_BODY_SIZE)
    return -1;
  http_body_write(c->h, data, data_len);
  return 0;
}
/** called when a protocol error occurred. */
static int http1_on_error(http1_parser_s *parser) {
  client_s *c = (client_s *)parser;
  if (c->h) {
    http_status_set(c->h, 400);
    http_write(c->h,
               .data = "Bad Request... be nicer next time!",
               .len = 34,
               .finish = 1);
  }
  fio_close(c->io);
  return -1;
}
