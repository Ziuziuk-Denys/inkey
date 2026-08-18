#include "engine.h"
#include "inkey_core.h"

G_DEFINE_TYPE(IBusInkeyEngine, ibus_inkey_engine, IBUS_TYPE_ENGINE)

/* Ctrl, Alt, and Super. Super is reported as Mod4 on X11/XWayland and as
 * the dedicated Super mask on newer ibus/Wayland setups, so both are
 * checked. */
#define INKEY_MODIFIER_PASSTHROUGH_MASK \
    (IBUS_CONTROL_MASK | IBUS_MOD1_MASK | IBUS_MOD4_MASK | IBUS_SUPER_MASK)

gboolean
ibus_inkey_should_pass_through(guint modifiers)
{
    return (modifiers & INKEY_MODIFIER_PASSTHROUGH_MASK) != 0;
}

gboolean
ibus_inkey_is_word_boundary_char(gunichar ch)
{
    switch (ch) {
    case ' ':
    case '.':
    case ',':
    case '!':
    case '?':
    case ';':
    case ':':
    case '\n':
        return TRUE;
    default:
        return FALSE;
    }
}

void
ibus_inkey_flush_word_buffer(GString *word_buffer, const InkeySink *sink)
{
    if (word_buffer->len == 0)
        return;

    char *corrected = inkey_transform(word_buffer->str);
    const char *final_text = (corrected != NULL) ? corrected : word_buffer->str;

    /* Nothing has actually been committed yet - the word has only ever
     * been shown as preedit - so there's nothing to erase. Clear the
     * preedit display, then commit the winning text exactly once. */
    sink->hide_preedit(sink->user_data);
    sink->commit(sink->user_data, final_text);

    if (corrected != NULL)
        inkey_free_string(corrected);

    g_string_set_size(word_buffer, 0);
}

gboolean
ibus_inkey_handle_printable_char(GString *word_buffer, const InkeySink *sink, gunichar ch)
{
    if (ibus_inkey_is_word_boundary_char(ch)) {
        ibus_inkey_flush_word_buffer(word_buffer, sink);
        return FALSE; /* let the boundary character itself pass through */
    }

    gchar utf8[7] = {0};
    gint len = g_unichar_to_utf8(ch, utf8);
    utf8[len] = '\0';
    g_string_append(word_buffer, utf8);

    sink->update_preedit(sink->user_data, word_buffer->str);

    /* Consumed: the character is shown via preedit, not committed to the
     * document, until the word is flushed. */
    return TRUE;
}

gboolean
ibus_inkey_handle_backspace(GString *word_buffer, const InkeySink *sink)
{
    if (word_buffer->len == 0)
        return FALSE; /* nothing buffered - let a real Backspace do its job */

    g_string_truncate(word_buffer, word_buffer->len - 1);
    if (word_buffer->len > 0)
        sink->update_preedit(sink->user_data, word_buffer->str);
    else
        sink->hide_preedit(sink->user_data);

    /* Consumed: there's nothing of ours in the real document to delete -
     * the buffered word only ever existed in preedit. */
    return TRUE;
}

static void
real_sink_commit(gpointer user_data, const gchar *text)
{
    IBusEngine *engine = IBUS_ENGINE(user_data);
    IBusText *ibus_text = ibus_text_new_from_string(text);
    ibus_engine_commit_text(engine, ibus_text);
}

static void
real_sink_update_preedit(gpointer user_data, const gchar *text)
{
    IBusEngine *engine = IBUS_ENGINE(user_data);
    IBusText *ibus_text = ibus_text_new_from_string(text);
    guint cursor_pos = (guint) g_utf8_strlen(text, -1);
    /* IBUS_ENGINE_PREEDIT_COMMIT: if focus is lost mid-word, the client
     * commits whatever's currently buffered rather than silently
     * discarding it. */
    ibus_engine_update_preedit_text_with_mode(
        engine, ibus_text, cursor_pos, TRUE, IBUS_ENGINE_PREEDIT_COMMIT);
}

static void
real_sink_hide_preedit(gpointer user_data)
{
    ibus_engine_hide_preedit_text(IBUS_ENGINE(user_data));
}

static InkeySink
ibus_inkey_real_sink(IBusEngine *engine)
{
    InkeySink sink = {
        .commit = real_sink_commit,
        .update_preedit = real_sink_update_preedit,
        .hide_preedit = real_sink_hide_preedit,
        .user_data = engine,
    };
    return sink;
}

static gboolean
ibus_inkey_engine_process_key_event(IBusEngine *engine,
                                     guint       keyval,
                                     guint       keycode,
                                     guint       modifiers)
{
    (void) keycode;

    IBusInkeyEngine *self = IBUS_INKEY_ENGINE(engine);

    /* Let key releases pass through untouched. */
    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    /* Ctrl/Alt/Super-held key combos are shortcuts, not text input. */
    if (ibus_inkey_should_pass_through(modifiers))
        return FALSE;

    InkeySink sink = ibus_inkey_real_sink(engine);

    /* Enter has no unicode value from ibus_keyval_to_unicode, but it's
     * still a word boundary: flush, then let the app insert the newline
     * itself. */
    if (keyval == IBUS_KEY_Return || keyval == IBUS_KEY_KP_Enter) {
        ibus_inkey_flush_word_buffer(self->word_buffer, &sink);
        return FALSE;
    }

    if (keyval == IBUS_KEY_BackSpace)
        return ibus_inkey_handle_backspace(self->word_buffer, &sink);

    gunichar ch = ibus_keyval_to_unicode(keyval);

    /* Not part of a word (arrows, Tab, Escape, function keys, etc). Flush
     * any pending preedit first so it doesn't stay visibly stuck while
     * the user moves elsewhere; a no-op if nothing is buffered. */
    if (ch == 0 || !g_unichar_isprint(ch)) {
        ibus_inkey_flush_word_buffer(self->word_buffer, &sink);
        return FALSE;
    }

    return ibus_inkey_handle_printable_char(self->word_buffer, &sink, ch);
}

static void
ibus_inkey_engine_focus_out(IBusEngine *engine)
{
    IBusInkeyEngine *self = IBUS_INKEY_ENGINE(engine);
    InkeySink sink = ibus_inkey_real_sink(engine);
    ibus_inkey_flush_word_buffer(self->word_buffer, &sink);
    IBUS_ENGINE_CLASS(ibus_inkey_engine_parent_class)->focus_out(engine);
}

static void
ibus_inkey_engine_reset(IBusEngine *engine)
{
    IBusInkeyEngine *self = IBUS_INKEY_ENGINE(engine);
    InkeySink sink = ibus_inkey_real_sink(engine);
    ibus_inkey_flush_word_buffer(self->word_buffer, &sink);
    IBUS_ENGINE_CLASS(ibus_inkey_engine_parent_class)->reset(engine);
}

static void
ibus_inkey_engine_finalize(GObject *object)
{
    IBusInkeyEngine *self = IBUS_INKEY_ENGINE(object);
    if (self->word_buffer != NULL) {
        g_string_free(self->word_buffer, TRUE);
        self->word_buffer = NULL;
    }
    G_OBJECT_CLASS(ibus_inkey_engine_parent_class)->finalize(object);
}

static void
ibus_inkey_engine_class_init(IBusInkeyEngineClass *klass)
{
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS(klass);
    engine_class->process_key_event = ibus_inkey_engine_process_key_event;
    engine_class->focus_out = ibus_inkey_engine_focus_out;
    engine_class->reset = ibus_inkey_engine_reset;

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = ibus_inkey_engine_finalize;
}

static void
ibus_inkey_engine_init(IBusInkeyEngine *self)
{
    self->word_buffer = g_string_new(NULL);
}
