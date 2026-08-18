#ifndef INKEY_ENGINE_H
#define INKEY_ENGINE_H

#include <ibus.h>

G_BEGIN_DECLS

#define IBUS_TYPE_INKEY_ENGINE (ibus_inkey_engine_get_type())
#define IBUS_INKEY_ENGINE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), IBUS_TYPE_INKEY_ENGINE, IBusInkeyEngine))

typedef struct _IBusInkeyEngine      IBusInkeyEngine;
typedef struct _IBusInkeyEngineClass IBusInkeyEngineClass;

struct _IBusInkeyEngine {
    IBusEngine parent;

    /* Characters typed since the last word boundary (always ASCII: the
     * physical XKB layout is pinned to "us" while Inkey is active, so
     * every raw keystroke is Latin). Flushed and corrected on the next
     * boundary character; see ibus_inkey_is_word_boundary_char. */
    GString *word_buffer;
};

struct _IBusInkeyEngineClass {
    IBusEngineClass parent_class;
};

GType ibus_inkey_engine_get_type(void);

/*
 * Returns TRUE when the given modifier state means the key event must be
 * left untouched (Ctrl, Alt, or Super held) so shortcuts like Ctrl+C keep
 * working while Inkey is the active input source. Exposed for unit testing.
 */
gboolean ibus_inkey_should_pass_through(guint modifiers);

/*
 * Returns TRUE when `ch` is a word-boundary trigger character: space, one
 * of .,!?;: , or Enter (passed as '\n', since Enter has no unicode value
 * from ibus_keyval_to_unicode). Exposed for unit testing.
 */
gboolean ibus_inkey_is_word_boundary_char(gunichar ch);

G_END_DECLS

#endif /* INKEY_ENGINE_H */
