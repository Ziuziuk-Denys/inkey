// Word-boundary EN <-> RU/UK wrong-layout detection and correction.
//
// Inkey pins the physical XKB layout to "us" for as long as it's the
// active input source (see the `layout` field in
// engine/data/inkey.xml.in, and the Phase 1 design-question finding: IBus
// applies an engine's declared layout to the physical keyboard on every
// activation, per ibusenginedesc.h). That means every raw key event this
// engine ever receives is a Latin-ASCII keystroke - there is no "the user
// actually typed native Cyrillic" case to detect here, only "the user
// typed Latin characters that were probably meant as Cyrillic on a
// different physical layout". So the only direction handled is Latin
// as-typed vs. positionally-remapped-to-Cyrillic.
use crate::frequency::{score, Lang};
use crate::layout_tables::{remap_word_latin_to_cyrillic, CyrillicLayout};

// A remapped candidate must beat the as-typed score by at least this much
// (zipf scale, roughly log10 of frequency) to be committed instead. A
// wrong "correction" the user didn't ask for is worse than no correction,
// so ties and near-ties keep the text as typed.
const CONFIDENCE_MARGIN: f32 = 0.5;

// Decides what to commit for a buffered word: either the original
// as-typed text, or a positionally-remapped Cyrillic reinterpretation,
// whichever more clearly looks like a real word. Only pure Latin-alphabet
// words are considered; anything else (digits, punctuation, contractions,
// already-Cyrillic text) is returned unchanged.
pub fn correct_word(word: &str) -> String {
    if word.is_empty() || !word.chars().all(|c| c.is_ascii_alphabetic()) {
        return word.to_string();
    }

    let as_typed_score = score(Lang::En, word);

    let ru_candidate = remap_word_latin_to_cyrillic(word, CyrillicLayout::Ru);
    let ru_score = ru_candidate
        .as_deref()
        .map(|w| score(Lang::Ru, w))
        .unwrap_or(0.0);

    let uk_candidate = remap_word_latin_to_cyrillic(word, CyrillicLayout::Uk);
    let uk_score = uk_candidate
        .as_deref()
        .map(|w| score(Lang::Uk, w))
        .unwrap_or(0.0);

    if ru_score >= uk_score && ru_score > as_typed_score + CONFIDENCE_MARGIN {
        return ru_candidate.unwrap();
    }
    if uk_score > ru_score && uk_score > as_typed_score + CONFIDENCE_MARGIN {
        return uk_candidate.unwrap();
    }
    word.to_string()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn wrong_layout_russian_word_gets_corrected() {
        assert_eq!(correct_word("ghbdtn"), "привет");
    }

    #[test]
    fn correctly_typed_english_word_is_not_mangled() {
        assert_eq!(correct_word("hello"), "hello");
    }

    #[test]
    fn correctly_typed_common_english_word_is_not_mangled() {
        assert_eq!(correct_word("world"), "world");
    }

    #[test]
    fn wrong_layout_second_russian_word_gets_corrected() {
        // "vjkjrj" -> "молоко" (milk). Deliberately avoids the comma/
        // period keys (Cyrillic б/ю): those double as word-boundary
        // trigger characters in this phase's scope, so a word that needs
        // them mid-word (e.g. "spasibo", whose б sits on the comma key)
        // would get split at the comma before ever reaching correction -
        // a real, accepted limitation of trigger-character word-boundary
        // detection, not something this phase's scope covers.
        assert_eq!(correct_word("vjkjrj"), "молоко");
    }

    #[test]
    fn disambiguates_ukrainian_from_russian_by_score() {
        // ghbdsn remaps to "привыт" in RU (not a real word) and "привіт"
        // in UK (a real word) - the UK candidate should win.
        assert_eq!(correct_word("ghbdsn"), "привіт");
    }

    #[test]
    fn unrecognized_word_in_any_language_is_left_as_typed() {
        assert_eq!(correct_word("zzzqxvv"), "zzzqxvv");
    }

    #[test]
    fn word_with_apostrophe_is_left_untouched() {
        assert_eq!(correct_word("don't"), "don't");
    }

    #[test]
    fn empty_word_is_left_untouched() {
        assert_eq!(correct_word(""), "");
    }
}
