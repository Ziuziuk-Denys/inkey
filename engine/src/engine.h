#ifndef INKEY_ENGINE_H
#define INKEY_ENGINE_H

#include <ibus.h>

G_BEGIN_DECLS

#define IBUS_TYPE_INKEY_ENGINE (ibus_inkey_engine_get_type())

typedef struct _IBusInkeyEngine      IBusInkeyEngine;
typedef struct _IBusInkeyEngineClass IBusInkeyEngineClass;

struct _IBusInkeyEngine {
    IBusEngine parent;
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

G_END_DECLS

#endif /* INKEY_ENGINE_H */
