#ifndef PASSWORD_H
#define PASSWORD_H

/**
 * Verifies the given password against the user's stored password.
 * 
 * @param input The password input to verify.
 * @return 1 if the password is correct, 0 if incorrect, -1 on error.
 */
int verify_password(const char *input);

#endif // PASSWORD_H
