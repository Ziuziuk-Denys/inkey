#ifndef INKEY_CORE_H
#define INKEY_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Transforms input (UTF-8, NUL-terminated) and returns a newly allocated
 * UTF-8 string that the caller owns. Free it with inkey_free_string.
 * Never crashes on panic or invalid input; falls back to a copy of the
 * original text (or NULL if input itself was NULL).
 */
char *inkey_transform(const char *input);

/* Frees a string previously returned by inkey_transform. */
void inkey_free_string(char *s);

#ifdef __cplusplus
}
#endif

#endif /* INKEY_CORE_H */
