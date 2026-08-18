#include <glib.h>
#include <ibus.h>
#include "engine.h"

/*
 * Simulates a real client application: whatever gets committed lands in
 * `document` (mirroring ibus_engine_commit_text), and whatever gets shown
 * as preedit is tracked separately (mirroring the underlined, uncommitted
 * preedit region) rather than the document itself. This is what the
 * previous unit tests never modeled - they only checked internal function
 * return values - which is exactly how the original "corrected word
 * appears alongside the original" bug slipped through: raw characters
 * were being committed immediately (via passthrough) *and* the correction
 * was committed again on top.
 */
typedef struct {
    GString *document;
    GString *last_preedit;
    gboolean preedit_visible;
    int commit_count;
} FakeClient;

static void
fake_commit(gpointer user_data, const gchar *text)
{
    FakeClient *client = user_data;
    g_string_append(client->document, text);
    client->commit_count++;
}

static void
fake_update_preedit(gpointer user_data, const gchar *text)
{
    FakeClient *client = user_data;
    g_string_assign(client->last_preedit, text);
    client->preedit_visible = TRUE;
}

static void
fake_hide_preedit(gpointer user_data)
{
    FakeClient *client = user_data;
    g_string_assign(client->last_preedit, "");
    client->preedit_visible = FALSE;
}

/* Mirrors what happens to one key event outside the engine: if
 * ibus_inkey_handle_printable_char consumes it (TRUE), nothing else
 * happens; if it doesn't (FALSE - a boundary character, already flushed
 * by that call), a real client inserts the character itself, exactly
 * like an unconsumed key event reaching a real app's normal input
 * handling. */
static void
simulate_type_char(GString *word_buffer, const InkeySink *sink, FakeClient *client, gunichar ch)
{
    gboolean consumed = ibus_inkey_handle_printable_char(word_buffer, sink, ch);
    if (!consumed) {
        gchar utf8[7] = {0};
        gint len = g_unichar_to_utf8(ch, utf8);
        utf8[len] = '\0';
        g_string_append(client->document, utf8);
    }
}

static void
simulate_type_string(GString *word_buffer, const InkeySink *sink, FakeClient *client, const gchar *text)
{
    for (const gchar *p = text; *p != '\0'; p++)
        simulate_type_char(word_buffer, sink, client, (gunichar)(guchar) *p);
}

static void
test_wrong_layout_word_commits_once_with_no_duplicate(void)
{
    GString *word_buffer = g_string_new(NULL);
    FakeClient client = {
        .document = g_string_new(NULL),
        .last_preedit = g_string_new(NULL),
    };
    InkeySink sink = { fake_commit, fake_update_preedit, fake_hide_preedit, &client };

    simulate_type_string(word_buffer, &sink, &client, "ghbdtn ");

    g_assert_cmpstr(client.document->str, ==, "привет ");
    g_assert_cmpint(client.commit_count, ==, 1);

    g_string_free(word_buffer, TRUE);
    g_string_free(client.document, TRUE);
    g_string_free(client.last_preedit, TRUE);
}

static void
test_correctly_typed_word_commits_once_unmangled(void)
{
    GString *word_buffer = g_string_new(NULL);
    FakeClient client = {
        .document = g_string_new(NULL),
        .last_preedit = g_string_new(NULL),
    };
    InkeySink sink = { fake_commit, fake_update_preedit, fake_hide_preedit, &client };

    simulate_type_string(word_buffer, &sink, &client, "hello ");

    g_assert_cmpstr(client.document->str, ==, "hello ");
    g_assert_cmpint(client.commit_count, ==, 1);

    g_string_free(word_buffer, TRUE);
    g_string_free(client.document, TRUE);
    g_string_free(client.last_preedit, TRUE);
}

static void
test_partial_word_is_preedit_only_not_committed(void)
{
    GString *word_buffer = g_string_new(NULL);
    FakeClient client = {
        .document = g_string_new(NULL),
        .last_preedit = g_string_new(NULL),
    };
    InkeySink sink = { fake_commit, fake_update_preedit, fake_hide_preedit, &client };

    simulate_type_string(word_buffer, &sink, &client, "gh");

    g_assert_cmpstr(client.document->str, ==, "");
    g_assert_cmpint(client.commit_count, ==, 0);
    g_assert_true(client.preedit_visible);
    g_assert_cmpstr(client.last_preedit->str, ==, "gh");

    g_string_free(word_buffer, TRUE);
    g_string_free(client.document, TRUE);
    g_string_free(client.last_preedit, TRUE);
}

static void
test_backspace_edits_preedit_without_touching_document(void)
{
    GString *word_buffer = g_string_new(NULL);
    FakeClient client = {
        .document = g_string_new(NULL),
        .last_preedit = g_string_new(NULL),
    };
    InkeySink sink = { fake_commit, fake_update_preedit, fake_hide_preedit, &client };

    simulate_type_string(word_buffer, &sink, &client, "gh");
    gboolean consumed = ibus_inkey_handle_backspace(word_buffer, &sink);
    g_assert_true(consumed);
    simulate_type_string(word_buffer, &sink, &client, "bdtn ");

    /* "g" + "bdtn" -> gbdtn, not the ghbdtn->privet correction. */
    g_assert_cmpstr(client.document->str, ==, "gbdtn ");
    g_assert_cmpint(client.commit_count, ==, 1);

    g_string_free(word_buffer, TRUE);
    g_string_free(client.document, TRUE);
    g_string_free(client.last_preedit, TRUE);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/inkey/commit-e2e/wrong-layout-no-duplicate",
                     test_wrong_layout_word_commits_once_with_no_duplicate);
    g_test_add_func("/inkey/commit-e2e/correct-word-unmangled",
                     test_correctly_typed_word_commits_once_unmangled);
    g_test_add_func("/inkey/commit-e2e/partial-word-is-preedit-only",
                     test_partial_word_is_preedit_only_not_committed);
    g_test_add_func("/inkey/commit-e2e/backspace-edits-preedit",
                     test_backspace_edits_preedit_without_touching_document);

    return g_test_run();
}
