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

/* Replaces the already-committed original word in the client app with
 * `corrected`: one synthetic Backspace per original character, then a
 * commit of the corrected text. Only called when a correction actually
 * changes the word, since this is disruptive (visible delete-and-retype)
 * compared to the normal passthrough echo. */
static void
inkey_replace_committed_word(IBusEngine *engine, const gchar *original, const gchar *corrected)
{
    glong char_count = g_utf8_strlen(original, -1);
    for (glong i = 0; i < char_count; i++) {
        ibus_engine_forward_key_event(engine, IBUS_KEY_BackSpace, 0, 0);
        ibus_engine_forward_key_event(engine, IBUS_KEY_BackSpace, 0, IBUS_RELEASE_MASK);
    }

    IBusText *text = ibus_text_new_from_string(corrected);
    ibus_engine_commit_text(engine, text);
}

/* Runs word-boundary correction on the buffered word (if any) and clears
 * the buffer. Called right before a boundary character (space, .,!?;:, or
 * Enter) is allowed to pass through, and on focus/reset so a buffer never
 * bleeds into a different field or app. */
static void
inkey_flush_word_buffer(IBusEngine *engine, IBusInkeyEngine *self)
{
    if (self->word_buffer->len == 0)
        return;

    char *corrected = inkey_transform(self->word_buffer->str);
    if (corrected != NULL) {
        if (g_strcmp0(corrected, self->word_buffer->str) != 0)
            inkey_replace_committed_word(engine, self->word_buffer->str, corrected);
        inkey_free_string(corrected);
    }

    g_string_set_size(self->word_buffer, 0);
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

    /* Enter has no unicode value from ibus_keyval_to_unicode, but it's
     * still a word boundary: flush, then let the app insert the newline
     * itself. */
    if (keyval == IBUS_KEY_Return || keyval == IBUS_KEY_KP_Enter) {
        inkey_flush_word_buffer(engine, self);
        return FALSE;
    }

    /* Keep the buffer in sync with ordinary edits so a stale prefix from
     * before a backspace doesn't get "corrected" later. Left/Right/Delete
     * and mouse-driven cursor moves are not tracked in this phase - a
     * known, accepted limitation of trigger-character word buffering. */
    if (keyval == IBUS_KEY_BackSpace) {
        if (self->word_buffer->len > 0)
            g_string_truncate(self->word_buffer, self->word_buffer->len - 1);
        return FALSE;
    }

    gunichar ch = ibus_keyval_to_unicode(keyval);

    /* Not a printable character (arrows, function keys, etc). */
    if (ch == 0 || !g_unichar_isprint(ch))
        return FALSE;

    if (ibus_inkey_is_word_boundary_char(ch)) {
        inkey_flush_word_buffer(engine, self);
        return FALSE;
    }

    gchar utf8[7] = {0};
    gint len = g_unichar_to_utf8(ch, utf8);
    utf8[len] = '\0';
    g_string_append(self->word_buffer, utf8);

    /* Let the app echo the character itself; we only ever intervene
     * (erase + recommit) once a completed word turns out to need
     * correcting. */
    return FALSE;
}

static void
ibus_inkey_engine_focus_out(IBusEngine *engine)
{
    inkey_flush_word_buffer(engine, IBUS_INKEY_ENGINE(engine));
    IBUS_ENGINE_CLASS(ibus_inkey_engine_parent_class)->focus_out(engine);
}

static void
ibus_inkey_engine_reset(IBusEngine *engine)
{
    IBusInkeyEngine *self = IBUS_INKEY_ENGINE(engine);
    g_string_set_size(self->word_buffer, 0);
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
