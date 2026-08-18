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

/*
 * Where the word-buffering/correction logic sends its output. Production
 * code points this at the real IBusEngine preedit/commit calls; tests
 * point it at a fake that records into a plain string so the final
 * "document" content can be asserted on without a live IBus session.
 */
typedef struct {
    void (*commit)(gpointer user_data, const gchar *text);
    void (*update_preedit)(gpointer user_data, const gchar *text);
    void (*hide_preedit)(gpointer user_data);
    gpointer user_data;
} InkeySink;

/*
 * Applies word-boundary correction to `word_buffer` (if non-empty) and
 * commits the result exactly once: the as-typed text if it doesn't need
 * correcting, or the positionally-remapped reinterpretation if it does.
 * Nothing is committed if the buffer is empty. Exposed for unit testing.
 */
void ibus_inkey_flush_word_buffer(GString *word_buffer, const InkeySink *sink);

/*
 * Handles one printable, non-boundary-or-boundary character: either
 * appends it to `word_buffer` and shows the buffer as preedit (returns
 * TRUE - the key event is consumed, nothing reaches the document until
 * the word is flushed), or, if `ch` is a word-boundary character, flushes
 * the buffer and returns FALSE so the caller lets the boundary character
 * itself pass through untouched. Exposed for unit testing.
 */
gboolean ibus_inkey_handle_printable_char(GString *word_buffer, const InkeySink *sink, gunichar ch);

/*
 * Handles Backspace: if `word_buffer` has buffered (not yet committed)
 * characters, pops the last one, updates/hides preedit accordingly, and
 * returns TRUE (consumed - there's nothing of ours in the real document
 * to delete yet). If the buffer is empty, returns FALSE so a real
 * Backspace reaches the document normally. Exposed for unit testing.
 */
gboolean ibus_inkey_handle_backspace(GString *word_buffer, const InkeySink *sink);

G_END_DECLS

#endif /* INKEY_ENGINE_H */
