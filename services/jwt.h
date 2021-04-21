/* license: MIT license
 * Copyright (C) 2021  Edward LEI <edward_lei72@hotmail.com> */

#ifndef _JWT_H_
#define _JWT_H_


#define JWT_PASSED 0
#define JWT_EXPIRED 1
#define JWT_FAILED 2


char *jwt_gen_token(const char *id,
                    const long tm_exp);

int jwt_verify(char *jwt);


#endif
