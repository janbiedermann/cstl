/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_URL Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_URL_TEST___H)
#define H___FIO_URL_TEST___H
#ifndef H___FIO_URL___H
#define FIO_URL
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

/* Test for URI variations:
 *
 * * `/complete_path?query#target`
 *
 *   i.e.: /index.html?page=1#list
 *
 * * `host:port/complete_path?query#target`
 *
 *   i.e.:
 *      example.com
 *      example.com:8080
 *      example.com/index.html
 *      example.com:8080/index.html
 *      example.com:8080/index.html?key=val#target
 *
 * * `user:password@host:port/path?query#target`
 *
 *   i.e.: user:1234@example.com:8080/index.html
 *
 * * `username[:password]@host[:port][...]`
 *
 *   i.e.: john:1234@example.com
 *
 * * `schema://user:password@host:port/path?query#target`
 *
 *   i.e.: http://example.com/index.html?page=1#list
 */
FIO_SFUNC void FIO_NAME_TEST(stl, url)(void) {
  fprintf(stderr, "* Testing URL (URI) parser.\n");
  struct {
    char *url;
    size_t len;
    fio_url_s expected;
  } tests[] = {
      {
          .url = (char *)"file://go/home/",
          .len = 15,
          .expected =
              {
                  .scheme = {.buf = (char *)"file", .len = 4},
                  .path = {.buf = (char *)"go/home/", .len = 8},
              },
      },
      {
          .url = (char *)"unix:///go/home/",
          .len = 16,
          .expected =
              {
                  .scheme = {.buf = (char *)"unix", .len = 4},
                  .path = {.buf = (char *)"/go/home/", .len = 9},
              },
      },
      {
          .url = (char *)"unix:///go/home/?query#target",
          .len = 29,
          .expected =
              {
                  .scheme = {.buf = (char *)"unix", .len = 4},
                  .path = {.buf = (char *)"/go/home/", .len = 9},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"schema://user:password@host:port/path?query#target",
          .len = 50,
          .expected =
              {
                  .scheme = {.buf = (char *)"schema", .len = 6},
                  .user = {.buf = (char *)"user", .len = 4},
                  .password = {.buf = (char *)"password", .len = 8},
                  .host = {.buf = (char *)"host", .len = 4},
                  .port = {.buf = (char *)"port", .len = 4},
                  .path = {.buf = (char *)"/path", .len = 5},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"schema://user@host:port/path?query#target",
          .len = 41,
          .expected =
              {
                  .scheme = {.buf = (char *)"schema", .len = 6},
                  .user = {.buf = (char *)"user", .len = 4},
                  .host = {.buf = (char *)"host", .len = 4},
                  .port = {.buf = (char *)"port", .len = 4},
                  .path = {.buf = (char *)"/path", .len = 5},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"http://localhost.com:3000/home?is=1",
          .len = 35,
          .expected =
              {
                  .scheme = {.buf = (char *)"http", .len = 4},
                  .host = {.buf = (char *)"localhost.com", .len = 13},
                  .port = {.buf = (char *)"3000", .len = 4},
                  .path = {.buf = (char *)"/home", .len = 5},
                  .query = {.buf = (char *)"is=1", .len = 4},
              },
      },
      {
          .url = (char *)"/complete_path?query#target",
          .len = 27,
          .expected =
              {
                  .path = {.buf = (char *)"/complete_path", .len = 14},
                  .query = {.buf = (char *)"query", .len = 5},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"/index.html?page=1#list",
          .len = 23,
          .expected =
              {
                  .path = {.buf = (char *)"/index.html", .len = 11},
                  .query = {.buf = (char *)"page=1", .len = 6},
                  .target = {.buf = (char *)"list", .len = 4},
              },
      },
      {
          .url = (char *)"example.com",
          .len = 11,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
              },
      },

      {
          .url = (char *)"example.com:8080",
          .len = 16,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
              },
      },
      {
          .url = (char *)"example.com:8080?q=true",
          .len = 23,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .query = {.buf = (char *)"q=true", .len = 6},
              },
      },
      {
          .url = (char *)"example.com/index.html",
          .len = 22,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"example.com:8080/index.html",
          .len = 27,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"example.com:8080/index.html?key=val#target",
          .len = 42,
          .expected =
              {
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
                  .query = {.buf = (char *)"key=val", .len = 7},
                  .target = {.buf = (char *)"target", .len = 6},
              },
      },
      {
          .url = (char *)"user:1234@example.com:8080/index.html",
          .len = 37,
          .expected =
              {
                  .user = {.buf = (char *)"user", .len = 4},
                  .password = {.buf = (char *)"1234", .len = 4},
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {
          .url = (char *)"user@example.com:8080/index.html",
          .len = 32,
          .expected =
              {
                  .user = {.buf = (char *)"user", .len = 4},
                  .host = {.buf = (char *)"example.com", .len = 11},
                  .port = {.buf = (char *)"8080", .len = 4},
                  .path = {.buf = (char *)"/index.html", .len = 11},
              },
      },
      {.url = NULL},
  };
  for (size_t i = 0; tests[i].url; ++i) {
    fio_url_s result = fio_url_parse(tests[i].url, tests[i].len);
    FIO_LOG_DEBUG2("Result for: %s"
                   "\n\t     scheme   (%zu bytes):  %.*s"
                   "\n\t     user     (%zu bytes):  %.*s"
                   "\n\t     password (%zu bytes):  %.*s"
                   "\n\t     host     (%zu bytes):  %.*s"
                   "\n\t     port     (%zu bytes):  %.*s"
                   "\n\t     path     (%zu bytes):  %.*s"
                   "\n\t     query    (%zu bytes):  %.*s"
                   "\n\t     target   (%zu bytes):  %.*s\n",
                   tests[i].url,
                   result.scheme.len,
                   (int)result.scheme.len,
                   result.scheme.buf,
                   result.user.len,
                   (int)result.user.len,
                   result.user.buf,
                   result.password.len,
                   (int)result.password.len,
                   result.password.buf,
                   result.host.len,
                   (int)result.host.len,
                   result.host.buf,
                   result.port.len,
                   (int)result.port.len,
                   result.port.buf,
                   result.path.len,
                   (int)result.path.len,
                   result.path.buf,
                   result.query.len,
                   (int)result.query.len,
                   result.query.buf,
                   result.target.len,
                   (int)result.target.len,
                   result.target.buf);
    FIO_ASSERT(
        result.scheme.len == tests[i].expected.scheme.len &&
            (!result.scheme.len || !memcmp(result.scheme.buf,
                                           tests[i].expected.scheme.buf,
                                           tests[i].expected.scheme.len)),
        "scheme result failed for:\n\ttest[%zu]: %s\n\texpected: "
        "%s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.scheme.buf,
        (int)result.scheme.len,
        result.scheme.buf);
    FIO_ASSERT(
        result.user.len == tests[i].expected.user.len &&
            (!result.user.len || !memcmp(result.user.buf,
                                         tests[i].expected.user.buf,
                                         tests[i].expected.user.len)),
        "user result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.user.buf,
        (int)result.user.len,
        result.user.buf);
    FIO_ASSERT(
        result.password.len == tests[i].expected.password.len &&
            (!result.password.len || !memcmp(result.password.buf,
                                             tests[i].expected.password.buf,
                                             tests[i].expected.password.len)),
        "password result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: "
        "%.*s",
        i,
        tests[i].url,
        tests[i].expected.password.buf,
        (int)result.password.len,
        result.password.buf);
    FIO_ASSERT(
        result.host.len == tests[i].expected.host.len &&
            (!result.host.len || !memcmp(result.host.buf,
                                         tests[i].expected.host.buf,
                                         tests[i].expected.host.len)),
        "host result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.host.buf,
        (int)result.host.len,
        result.host.buf);
    FIO_ASSERT(
        result.port.len == tests[i].expected.port.len &&
            (!result.port.len || !memcmp(result.port.buf,
                                         tests[i].expected.port.buf,
                                         tests[i].expected.port.len)),
        "port result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.port.buf,
        (int)result.port.len,
        result.port.buf);
    FIO_ASSERT(
        result.path.len == tests[i].expected.path.len &&
            (!result.path.len || !memcmp(result.path.buf,
                                         tests[i].expected.path.buf,
                                         tests[i].expected.path.len)),
        "path result failed for:\n\ttest[%zu]: %s\n\texpected: %s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.path.buf,
        (int)result.path.len,
        result.path.buf);
    FIO_ASSERT(result.query.len == tests[i].expected.query.len &&
                   (!result.query.len || !memcmp(result.query.buf,
                                                 tests[i].expected.query.buf,
                                                 tests[i].expected.query.len)),
               "query result failed for:\n\ttest[%zu]: %s\n\texpected: "
               "%s\n\tgot: %.*s",
               i,
               tests[i].url,
               tests[i].expected.query.buf,
               (int)result.query.len,
               result.query.buf);
    FIO_ASSERT(
        result.target.len == tests[i].expected.target.len &&
            (!result.target.len || !memcmp(result.target.buf,
                                           tests[i].expected.target.buf,
                                           tests[i].expected.target.len)),
        "target result failed for:\n\ttest[%zu]: %s\n\texpected: "
        "%s\n\tgot: %.*s",
        i,
        tests[i].url,
        tests[i].expected.target.buf,
        (int)result.target.len,
        result.target.buf);
  }
}
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
