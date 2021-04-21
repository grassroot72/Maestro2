/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "xmalloc.h"
#include "util.h"
#include "json.h"
#include "base64.h"
#include "hmac_sha256.h"
#include "jwt.h"

//#define DEBUG
#include "debug.h"


/* id - user id to be authenticated
 * tm_exp - expiration time
 * len_token - token length returned
 *
 * Access token lifetime (from auth0.com)
 * By default, an access token for a custom API is valid
 * for 86400 seconds (24 hours). We recommend that you set
 * the validity period of your token based on the security
 * requirements of your API. For example, an access token
 * that accesses a banking API should expire more quickly
 * than one that accesses a to-do API.*/
char *jwt_gen_token(const char *id,
                    const long tm_exp)
{
  char *ret;
  /* jwt header - {"alg":"HS256","typ":"JWT"} in base64 format
   * the length of jwtheader is 36 + 1 */
  char *jwtheader = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.";
  /* jwt payload - {"exp":1616940701,"sub":"0001"}
   * exp - epoc time, can be obtained by time()
   * iat - epoc time, can be obtained by time() */
  char exp[16];
  itos((unsigned char *)exp, time(NULL) + tm_exp, 10, ' ');
  char payload[64];
  ret = strbld(payload, "{\"exp\":");
  ret = strbld(ret, exp);
  ret = strbld(ret, ",\"sub\":\"");
  ret = strbld(ret, id);
  ret = strbld(ret, "\"}");
  *ret++ = '\0';
  D_PRINT("[JWT] payload = %s\n", payload);
  /* the length of {"exp":1616940701,"sub":"0001"} is 31 */
  int len2;
  char *jwtpayload = base64url_enc((unsigned char *)payload, 31, 0, &len2);
  D_PRINT("[JWT] jwtpayload = %s\n", jwtpayload);
  D_PRINT("[JWT] jwtpayload length = %d\n", len2);

  int len12 = 37 + len2;
  char jwthp[len12 + 1];
  ret = strbld(jwthp, jwtheader);
  ret = strbld(ret, jwtpayload);
  *ret++ = '\0';
  xfree(jwtpayload);
  D_PRINT("[JWT] jwt header+payload = %s\n", jwthp);

  /* generate binary signature */
  unsigned char signature[HMAC_HASH_SIZE];
  /* if key, ex. 1Qaz@wSx3edc$rfv5Tgb6yHn
   * then base64 encoded: MVFhekB3U3gzZWRjJHJmdjVUZ2I2eUhuCiA=
   * and encoded length = 36 */
  const char *key = "MVFhekB3U3gzZWRjJHJmdjVUZ2I2eUhuCiA=";
  hmac_sha256(key, 36, jwthp, len12, signature, HMAC_HASH_SIZE);

  /* encode binary signature in base64url format */
  int len3;
  char *secret = base64url_enc((unsigned char *)signature,
                               HMAC_HASH_SIZE, 0, &len3);

  int len_token = len12 + 1 + len3;
  char *token = xmalloc(len_token + 1);
  ret = strbld(token, jwthp);
  *ret++ = '.';
  ret = strbld(ret, secret);
  *ret++ = '\0';
  D_PRINT("[JWT] jwt_token = %s\n", token);
  xfree(secret);

  return token;
}

int jwt_verify(char *token)
{
  /* we use the default hs256 aglo, so we don't extract the jwt header */
  char *payload = split_kv(token, '.');
  char *secret = split_kv(payload, '.');
  D_PRINT("[JWT] secret = %s\n", secret);
  /* extract the jwt payload */

  int len2 = strlen(payload);
  char *payload_json = (char *)base64url_dec(payload, len2);
  D_PRINT("[JWT] payload = %s\n", payload_json);

  struct json_value_s *root = json_parse(payload_json, strlen(payload_json));
  struct json_object_s *object = json_value_as_object(root);
  struct json_object_element_s *e0 = object->start;
  const char *jkey = ((struct json_string_s *)e0->name)->string;
  D_PRINT("[JWT] jkey = %s\n", jkey);
  /* check if jwt expired */
  if (strcmp(jkey, "exp") == 0) {
    struct json_number_s *e0_vn = json_value_as_number(e0->value);
    if (e0_vn) {
      time_t exp_time = atol(e0_vn->number);
      time_t cur_time = time(NULL);
      D_PRINT("[JWT] expire time = %ld\n", exp_time);
      D_PRINT("[JWT] current time = %ld\n", cur_time);
      if (cur_time > exp_time) {
        xfree(payload_json);
        xfree(root);
        return JWT_EXPIRED;
      }
    }
    else {
      xfree(payload_json);
      xfree(root);
      return JWT_FAILED;
    }
  }
  xfree(payload_json);
  xfree(root);

  /* verify the secret */
  token[36] = '.';
  char *jwthp = token;
  int len12 = 37 + len2;
  /* generate binary signature */
  unsigned char signature[HMAC_HASH_SIZE];
  /* if key, ex. 1Qaz@wSx3edc$rfv5Tgb6yHn
   * then base64 encoded: MVFhekB3U3gzZWRjJHJmdjVUZ2I2eUhuCiA=
   * and encoded length = 36 */
  const char *key = "MVFhekB3U3gzZWRjJHJmdjVUZ2I2eUhuCiA=";
  hmac_sha256(key, 36, jwthp, len12, signature, HMAC_HASH_SIZE);
  /* encode binary signature in base64url format */
  int len3;
  char *jwtsecret = base64url_enc((unsigned char *)signature,
                                  HMAC_HASH_SIZE, 0, &len3);
  D_PRINT("[JWT] jwtsecret = %s\n", jwtsecret);

  int rc = strcmp(jwtsecret, secret);
  xfree(jwtsecret);
  if (rc == 0) {
    D_PRINT("[JWT] authenticated!!\n");
    return JWT_PASSED;
  }
  else
    return JWT_FAILED;
}
