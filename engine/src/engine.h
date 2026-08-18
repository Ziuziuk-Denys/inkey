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

G_END_DECLS

#endif /* INKEY_ENGINE_H */
