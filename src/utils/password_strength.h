#ifndef SEAFILE_CLIENT_UTILS_PASSWORD_STRENGTH_H
#define SEAFILE_CLIENT_UTILS_PASSWORD_STRENGTH_H

using namespace std;

unsigned int getNISTNumBits(const char* passwd, unsigned int len, bool repeat_calc);

int isStrongPassword(	const char* passwd, unsigned int length,
						unsigned int minbits
						/* bool usedict=false, */
						/* unsigned int  minwordlen */);

#endif // SEAFILE_CLIENT_UTILS_PASSWORD_STRENGTH_H
