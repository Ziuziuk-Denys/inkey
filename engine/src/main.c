#include <ibus.h>
#include "engine.h"

#define INKEY_BUS_NAME "org.freedesktop.IBus.Inkey"

static IBusBus *bus = NULL;

static void
ibus_disconnected_cb(IBusBus *bus, gpointer user_data)
{
    (void) bus;
    (void) user_data;
    ibus_quit();
}

int
main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    ibus_init();

    bus = ibus_bus_new();
    g_signal_connect(bus, "disconnected", G_CALLBACK(ibus_disconnected_cb), NULL);

    IBusFactory *factory = ibus_factory_new(ibus_bus_get_connection(bus));
    ibus_factory_add_engine(factory, "inkey", IBUS_TYPE_INKEY_ENGINE);

    if (!ibus_bus_request_name(bus, INKEY_BUS_NAME, 0)) {
        g_error("Failed to request bus name " INKEY_BUS_NAME);
    }

    ibus_main();

    return 0;
}
