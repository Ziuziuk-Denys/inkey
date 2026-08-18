#include <glib.h>
#include <ibus.h>
#include "engine.h"

static void
test_plain_character_is_not_passthrough(void)
{
    g_assert_false(ibus_inkey_should_pass_through(0));
}

static void
test_shift_alone_is_not_passthrough(void)
{
    g_assert_false(ibus_inkey_should_pass_through(IBUS_SHIFT_MASK));
}

static void
test_ctrl_is_passthrough(void)
{
    g_assert_true(ibus_inkey_should_pass_through(IBUS_CONTROL_MASK));
}

static void
test_ctrl_plus_shift_is_passthrough(void)
{
    g_assert_true(ibus_inkey_should_pass_through(IBUS_CONTROL_MASK | IBUS_SHIFT_MASK));
}

static void
test_alt_is_passthrough(void)
{
    g_assert_true(ibus_inkey_should_pass_through(IBUS_MOD1_MASK));
}

static void
test_super_via_mod4_is_passthrough(void)
{
    /* Super is commonly reported as Mod4 on X11/XWayland setups. */
    g_assert_true(ibus_inkey_should_pass_through(IBUS_MOD4_MASK));
}

static void
test_super_via_explicit_mask_is_passthrough(void)
{
    /* Newer ibus versions can report Super via the dedicated mask instead. */
    g_assert_true(ibus_inkey_should_pass_through(IBUS_SUPER_MASK));
}

static void
test_capslock_alone_is_not_passthrough(void)
{
    /* Caps Lock changes the keyval itself; it must not block correction. */
    g_assert_false(ibus_inkey_should_pass_through(IBUS_LOCK_MASK));
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/inkey/passthrough/plain-character", test_plain_character_is_not_passthrough);
    g_test_add_func("/inkey/passthrough/shift-alone", test_shift_alone_is_not_passthrough);
    g_test_add_func("/inkey/passthrough/ctrl", test_ctrl_is_passthrough);
    g_test_add_func("/inkey/passthrough/ctrl-plus-shift", test_ctrl_plus_shift_is_passthrough);
    g_test_add_func("/inkey/passthrough/alt", test_alt_is_passthrough);
    g_test_add_func("/inkey/passthrough/super-mod4", test_super_via_mod4_is_passthrough);
    g_test_add_func("/inkey/passthrough/super-explicit", test_super_via_explicit_mask_is_passthrough);
    g_test_add_func("/inkey/passthrough/capslock-alone", test_capslock_alone_is_not_passthrough);

    return g_test_run();
}
