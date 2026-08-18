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

static gboolean
ibus_inkey_engine_process_key_event(IBusEngine *engine,
                                     guint       keyval,
                                     guint       keycode,
                                     guint       modifiers)
{
    (void) keycode;

    /* Let key releases pass through untouched. */
    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    /* Ctrl/Alt/Super-held key combos are shortcuts, not text input. */
    if (ibus_inkey_should_pass_through(modifiers))
        return FALSE;

    gunichar ch = ibus_keyval_to_unicode(keyval);

    /* Not a printable character (backspace, arrows, modifier-only, etc). */
    if (ch == 0 || !g_unichar_isprint(ch))
        return FALSE;

    gchar utf8[7] = {0};
    gint len = g_unichar_to_utf8(ch, utf8);
    utf8[len] = '\0';

    char *transformed = inkey_transform(utf8);
    if (transformed == NULL)
        return FALSE;

    IBusText *text = ibus_text_new_from_string(transformed);
    ibus_engine_commit_text(engine, text);

    inkey_free_string(transformed);

    return TRUE;
}

static void
ibus_inkey_engine_class_init(IBusInkeyEngineClass *klass)
{
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS(klass);
    engine_class->process_key_event = ibus_inkey_engine_process_key_event;
}

static void
ibus_inkey_engine_init(IBusInkeyEngine *self)
{
    (void) self;
}
