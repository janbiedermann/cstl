/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_SHA Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_SHA_TEST___H)
#define FIO___TEST_REINCLUDE
#define H___FIO_SHA_TEST___H
#ifndef H___FIO_SHA1___H
#define FIO_SHA1
#include FIO_INCLUDE_FILE
#endif
#ifndef H___FIO_SHA2___H
#define FIO_SHA2
#include FIO_INCLUDE_FILE
#endif
#undef FIO___TEST_REINCLUDE

/* *****************************************************************************
SHA1
***************************************************************************** */

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha1_wrapper)(char *data, size_t len) {
  fio_sha1_s h = fio_sha1((const void *)data, (uint64_t)len);
  return *(uintptr_t *)h.digest;
}

#if HAVE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper)(char *data,
                                                                size_t len) {
  fio_u256 result;
  SHA1((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}

#endif

FIO_SFUNC void FIO_NAME_TEST(stl, sha1)(void) {
  fprintf(stderr, "* Testing SHA-1\n");
  struct {
    const char *str;
    const char *sha1;
  } data[] = {
      {
          .str = "",
          .sha1 = "\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18"
                  "\x90\xaf\xd8\x07\x09",
      },
      {
          .str = "The quick brown fox jumps over the lazy dog",
          .sha1 = "\x2f\xd4\xe1\xc6\x7a\x2d\x28\xfc\xed\x84\x9e\xe1\xbb\x76\xe7"
                  "\x39\x1b\x93\xeb\x12",
      },
      {
          .str = "The quick brown fox jumps over the lazy cog",
          .sha1 = "\xde\x9f\x2c\x7f\xd2\x5e\x1b\x3a\xfa\xd3\xe8\x5a\x0b\xd1\x7d"
                  "\x9b\x10\x0d\xb4\xb3",
      },
  };
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    fio_sha1_s sha1 = fio_sha1(data[i].str, FIO_STRLEN(data[i].str));

    FIO_ASSERT(!memcmp(sha1.digest, data[i].sha1, fio_sha1_len()),
               "SHA1 mismatch for \"%s\"",
               data[i].str);
  }
#if !DEBUG
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_wrapper),
                         (char *)"fio_sha1",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_wrapper),
                         (char *)"fio_sha1",
                         13,
                         0,
                         1);
#if HAVE_OPENSSL
  fprintf(stderr, "* Comparing to " OPENSSL_VERSION_TEXT "\n");
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-1",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha1_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-1",
                         13,
                         0,
                         1);
#endif /* HAVE_OPENSSL */
#endif /* !DEBUG */
}

/* *****************************************************************************
SHA2
***************************************************************************** */

FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha256_wrapper)(char *data,
                                                         size_t len) {
  fio_u256 h = fio_sha256((const void *)data, (uint64_t)len);
  return (uintptr_t)(h.u64[0]);
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha512_wrapper)(char *data,
                                                         size_t len) {
  fio_u512 h = fio_sha512((const void *)data, (uint64_t)len);
  return (uintptr_t)(h.u64[0]);
}

#if HAVE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha256_open_ssl_wrapper)(char *data,
                                                                  size_t len) {
  fio_u256 result;
  SHA256((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}
FIO_SFUNC uintptr_t FIO_NAME_TEST(stl, __sha512_open_ssl_wrapper)(char *data,
                                                                  size_t len) {
  fio_u512 result;
  SHA512((const unsigned char *)data, len, result.u8);
  return result.u64[0];
}
#endif /* HAVE_OPENSSL */

FIO_SFUNC void FIO_NAME_TEST(stl, sha2)(void) {
  fprintf(stderr, "* Testing SHA-2\n");
  struct {
    const char *str;
    const char *sha256;
    const char *sha512;
  } data[] = {
      {
          .str = (char *)"abc",
          .sha256 = (char *)"\xBA\x78\x16\xBF\x8F\x01\xCF\xEA\x41\x41\x40\xDE"
                            "\x5D\xAE\x22\x23\xB0\x03\x61\xA3\x96\x17\x7A\x9C"
                            "\xB4\x10\xFF\x61\xF2\x00\x15\xAD",
      },
      {
          .str = (char *)"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnl"
                         "mnomnopnopq",
          .sha256 = (char *)"\x24\x8D\x6A\x61\xD2\x06\x38\xB8\xE5\xC0\x26"
                            "\x93\x0C\x3E\x60\x39\xA3\x3C\xE4\x59\x64\xFF"
                            "\x21\x67\xF6\xEC\xED\xD4\x19\xDB\x06\xC1",
      },
      {
          .str = (char *)"The quick brown fox jumps over the lazy dog",
          .sha256 = (char *)"\xD7\xA8\xFB\xB3\x07\xD7\x80\x94\x69\xCA\x9A\xBC"
                            "\xB0\x08\x2E\x4F\x8D\x56\x51\xE4\x6D\x3C\xDB\x76"
                            "\x2D\x02\xD0\xBF\x37\xC9\xE5\x92",
          .sha512 =
              (char *)"\x07\xE5\x47\xD9\x58\x6F\x6A\x73\xF7\x3F\xBA\xC0\x43\x5E"
                      "\xD7\x69\x51\x21\x8F\xB7\xD0\xC8\xD7\x88\xA3\x09\xD7\x85"
                      "\x43\x6B\xBB\x64\x2E\x93\xA2\x52\xA9\x54\xF2\x39\x12\x54"
                      "\x7D\x1E\x8A\x3B\x5E\xD6\xE1\xBF\xD7\x09\x78\x21\x23\x3F"
                      "\xA0\x53\x8F\x3D\xB8\x54\xFE\xE6",
      },
      {
          .str = (char *)"The quick brown fox jumps over the lazy cog",
          .sha256 = (char *)"\xE4\xC4\xD8\xF3\xBF\x76\xB6\x92\xDE\x79\x1A\x17"
                            "\x3E\x05\x32\x11\x50\xF7\xA3\x45\xB4\x64\x84\xFE"
                            "\x42\x7F\x6A\xCC\x7E\xCC\x81\xBE",
          .sha512 =
              (char *)"\x3E\xEE\xE1\xD0\xE1\x17\x33\xEF\x15\x2A\x6C\x29\x50\x3B"
                      "\x3A\xE2\x0C\x4F\x1F\x3C\xDA\x4C\xB2\x6F\x1B\xC1\xA4\x1F"
                      "\x91\xC7\xFE\x4A\xB3\xBD\x86\x49\x40\x49\xE2\x01\xC4\xBD"
                      "\x51\x55\xF3\x1E\xCB\x7A\x3C\x86\x06\x84\x3C\x4C\xC8\xDF"
                      "\xCA\xB7\xDA\x11\xC8\xAE\x50\x45",
      },
      {
          .str = (char *)"",
          .sha256 = (char *)"\xE3\xB0\xC4\x42\x98\xFC\x1C\x14\x9A\xFB\xF4\xC8"
                            "\x99\x6F\xB9\x24\x27\xAE\x41\xE4\x64\x9B\x93\x4C"
                            "\xA4\x95\x99\x1B\x78\x52\xB8\x55",
          .sha512 =
              (char *)"\xCF\x83\xE1\x35\x7E\xEF\xB8\xBD\xF1\x54\x28\x50\xD6\x6D"
                      "\x80\x07\xD6\x20\xE4\x05\x0B\x57\x15\xDC\x83\xF4\xA9\x21"
                      "\xD3\x6C\xE9\xCE\x47\xD0\xD1\x3C\x5D\x85\xF2\xB0\xFF\x83"
                      "\x18\xD2\x87\x7E\xEC\x2F\x63\xB9\x31\xBD\x47\x41\x7A\x81"
                      "\xA5\x38\x32\x7A\xF9\x27\xDA\x3E",
      },
  };
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    if (!data[i].str)
      continue;
    if (data[i].sha256) {
      fio_u256 sha256 = fio_sha256(data[i].str, FIO_STRLEN(data[i].str));
      FIO_ASSERT(!memcmp(sha256.u8, data[i].sha256, 32),
                 "SHA256 mismatch for \"%s\":\n\t %X%X%X%X...%X%X%X%X",
                 data[i].str,
                 sha256.u8[0],
                 sha256.u8[1],
                 sha256.u8[2],
                 sha256.u8[3],
                 sha256.u8[28],
                 sha256.u8[29],
                 sha256.u8[30],
                 sha256.u8[31]);
    }
    // if (data[i].sha512) {
    //   fio_u512 sha512 = fio_sha512(data[i].str, FIO_STRLEN(data[i].str));
    //   FIO_ASSERT(!memcmp(sha512.u8, data[i].sha512, 64),
    //              "SHA512 mismatch for \"%s\"",
    //              data[i].str);
    // }
  }
#if !DEBUG
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha256_wrapper),
                         (char *)"fio_sha256",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha256_wrapper),
                         (char *)"fio_sha256",
                         13,
                         0,
                         1);
  // fio_test_hash_function(FIO_NAME_TEST(stl, __sha512_wrapper),
  //                        (char *)"fio_sha512",
  //                        5,
  //                        0,
  //                        0);
  // fio_test_hash_function(FIO_NAME_TEST(stl, __sha512_wrapper),
  //                        (char *)"fio_sha512",
  //                        13,
  //                        0,
  //                        1);
#if HAVE_OPENSSL
  fprintf(stderr, "* Comparing to " OPENSSL_VERSION_TEXT "\n");
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha256_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-256",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha256_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-256",
                         13,
                         0,
                         1);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha512_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-512",
                         5,
                         0,
                         0);
  fio_test_hash_function(FIO_NAME_TEST(stl, __sha512_open_ssl_wrapper),
                         (char *)"OpenSSL SHA-512",
                         13,
                         0,
                         1);
#endif /* HAVE_OPENSSL */
#endif /* !DEBUG */
}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
