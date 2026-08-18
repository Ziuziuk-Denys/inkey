#include <glib.h>
#include <ibus.h>
#include "engine.h"

static void
test_space_is_boundary(void)
{
    g_assert_true(ibus_inkey_is_word_boundary_char(' '));
}

static void
test_period_comma_bang_question_semicolon_colon_are_boundaries(void)
{
    g_assert_true(ibus_inkey_is_word_boundary_char('.'));
    g_assert_true(ibus_inkey_is_word_boundary_char(','));
    g_assert_true(ibus_inkey_is_word_boundary_char('!'));
    g_assert_true(ibus_inkey_is_word_boundary_char('?'));
    g_assert_true(ibus_inkey_is_word_boundary_char(';'));
    g_assert_true(ibus_inkey_is_word_boundary_char(':'));
}

static void
test_newline_is_boundary(void)
{
    g_assert_true(ibus_inkey_is_word_boundary_char('\n'));
}

static void
test_letters_are_not_boundaries(void)
{
    g_assert_false(ibus_inkey_is_word_boundary_char('a'));
    g_assert_false(ibus_inkey_is_word_boundary_char('Z'));
}

static void
test_apostrophe_is_not_a_boundary(void)
{
    /* Contractions like "don't" must not split at the apostrophe. */
    g_assert_false(ibus_inkey_is_word_boundary_char('\''));
}

static void
test_digit_is_not_a_boundary(void)
{
    g_assert_false(ibus_inkey_is_word_boundary_char('5'));
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/inkey/boundary/space", test_space_is_boundary);
    g_test_add_func("/inkey/boundary/punctuation-set",
                     test_period_comma_bang_question_semicolon_colon_are_boundaries);
    g_test_add_func("/inkey/boundary/newline", test_newline_is_boundary);
    g_test_add_func("/inkey/boundary/letters-are-not", test_letters_are_not_boundaries);
    g_test_add_func("/inkey/boundary/apostrophe-is-not", test_apostrophe_is_not_a_boundary);
    g_test_add_func("/inkey/boundary/digit-is-not", test_digit_is_not_a_boundary);

    return g_test_run();
}
