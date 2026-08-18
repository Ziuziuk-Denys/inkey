// Positional keyboard remap tables for EN <-> RU and EN <-> UK wrong-layout
// correction.
//
// These tables are derived directly from this system's XKB symbol
// definitions, not hand-typed from memory:
//   - Latin (US) positions:    /usr/share/X11/xkb/symbols/us, "basic" group
//   - Russian positions:       /usr/share/X11/xkb/symbols/ru, default group
//                               ("winkeys", which includes "common" for the
//                               letter rows)
//   - Ukrainian positions:     /usr/share/X11/xkb/symbols/ua, default group
//                               ("unicode", which includes "legacy" for the
//                               letter rows it doesn't override)
// Cyrillic keysym -> Unicode code point values were read from
// /usr/include/X11/keysymdef.h, not assumed.
//
// Each pair is (lowercase Latin key, lowercase Cyrillic letter at that same
// physical key position). Uppercase forms are derived at lookup time via
// Unicode case conversion rather than hand-duplicated, since Cyrillic
// uppercase/lowercase is a straightforward 1:1 mapping and hand-duplicating
// invites transcription errors.
const RU_POSITIONAL_PAIRS: &[(char, char)] = &[
    ('`', 'ё'),
    ('q', 'й'), ('w', 'ц'), ('e', 'у'), ('r', 'к'), ('t', 'е'),
    ('y', 'н'), ('u', 'г'), ('i', 'ш'), ('o', 'щ'), ('p', 'з'),
    ('[', 'х'), (']', 'ъ'),
    ('a', 'ф'), ('s', 'ы'), ('d', 'в'), ('f', 'а'), ('g', 'п'),
    ('h', 'р'), ('j', 'о'), ('k', 'л'), ('l', 'д'), (';', 'ж'), ('\'', 'э'),
    ('z', 'я'), ('x', 'ч'), ('c', 'с'), ('v', 'м'), ('b', 'и'),
    ('n', 'т'), ('m', 'ь'), (',', 'б'), ('.', 'ю'),
];

// AB10 ('/') and BKSL ('\\') carry no Cyrillic letter in ru(winkeys) - all
// 33 Russian letters are already covered by the pairs above plus TLDE.
//
// Ukrainian differs from Russian at three letter-row positions: 's' is
// "i" (U+0456) here, not Russian's "yeru" (U+044B); '\'' is "ie" (U+0454,
// Ukrainian YE), not Russian's "e" (U+044D); ']' is "yi" (U+0457), not
// Russian's hard sign. BKSL carries a real letter here ("ghe with
// upturn", U+0491) where Russian has none.
const UK_POSITIONAL_PAIRS: &[(char, char)] = &[
    ('q', 'й'), ('w', 'ц'), ('e', 'у'), ('r', 'к'), ('t', 'е'),
    ('y', 'н'), ('u', 'г'), ('i', 'ш'), ('o', 'щ'), ('p', 'з'),
    ('[', 'х'), (']', 'ї'),
    ('a', 'ф'), ('s', 'і'), ('d', 'в'), ('f', 'а'), ('g', 'п'),
    ('h', 'р'), ('j', 'о'), ('k', 'л'), ('l', 'д'), (';', 'ж'), ('\'', 'є'),
    ('z', 'я'), ('x', 'ч'), ('c', 'с'), ('v', 'м'), ('b', 'и'),
    ('n', 'т'), ('m', 'ь'), (',', 'б'), ('.', 'ю'),
    ('\\', 'ґ'),
];

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CyrillicLayout {
    Ru,
    Uk,
}

fn pairs_for(layout: CyrillicLayout) -> &'static [(char, char)] {
    match layout {
        CyrillicLayout::Ru => RU_POSITIONAL_PAIRS,
        CyrillicLayout::Uk => UK_POSITIONAL_PAIRS,
    }
}

// Looks up the lowercase Cyrillic letter at the same physical key as the
// given lowercase Latin letter, or None if that key has no Cyrillic
// letter on this layout (punctuation-only positions, digits, etc).
fn latin_to_cyrillic_lower(layout: CyrillicLayout, latin_lower: char) -> Option<char> {
    pairs_for(layout)
        .iter()
        .find(|(l, _)| *l == latin_lower)
        .map(|(_, c)| *c)
}

// Looks up the lowercase Latin letter at the same physical key as the
// given lowercase Cyrillic letter, or None if no key on this layout
// produces that letter.
fn cyrillic_to_latin_lower(layout: CyrillicLayout, cyrillic_lower: char) -> Option<char> {
    pairs_for(layout)
        .iter()
        .find(|(_, c)| *c == cyrillic_lower)
        .map(|(l, _)| *l)
}

// Case-preserving single character remap: Latin -> Cyrillic.
pub fn latin_to_cyrillic(layout: CyrillicLayout, ch: char) -> Option<char> {
    let is_upper = ch.is_uppercase();
    let lower: char = ch.to_lowercase().next()?;
    let mapped = latin_to_cyrillic_lower(layout, lower)?;
    if is_upper {
        mapped.to_uppercase().next()
    } else {
        Some(mapped)
    }
}

// Case-preserving single character remap: Cyrillic -> Latin.
pub fn cyrillic_to_latin(layout: CyrillicLayout, ch: char) -> Option<char> {
    let is_upper = ch.is_uppercase();
    let lower: char = ch.to_lowercase().next()?;
    let mapped = cyrillic_to_latin_lower(layout, lower)?;
    if is_upper {
        mapped.to_uppercase().next()
    } else {
        Some(mapped)
    }
}

// Remaps every character of `word` from Latin to Cyrillic positionally.
// Returns None if any character has no mapping on this layout (the word
// can't be fully reinterpreted, so no correction should be offered).
pub fn remap_word_latin_to_cyrillic(word: &str, layout: CyrillicLayout) -> Option<String> {
    word.chars().map(|c| latin_to_cyrillic(layout, c)).collect()
}

// Remaps every character of `word` from Cyrillic to Latin positionally.
// Returns None if any character has no mapping on this layout.
pub fn remap_word_cyrillic_to_latin(word: &str, layout: CyrillicLayout) -> Option<String> {
    word.chars().map(|c| cyrillic_to_latin(layout, c)).collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ru_sanity_check_ghbdtn_maps_to_privet() {
        assert_eq!(
            remap_word_latin_to_cyrillic("ghbdtn", CyrillicLayout::Ru).as_deref(),
            Some("привет")
        );
    }

    #[test]
    fn ru_second_hand_checkable_word_spasibo() {
        assert_eq!(
            remap_word_latin_to_cyrillic("cgfcb,j", CyrillicLayout::Ru).as_deref(),
            Some("спасибо")
        );
    }

    #[test]
    fn ru_preserves_leading_capital() {
        assert_eq!(
            remap_word_latin_to_cyrillic("Ghbdtn", CyrillicLayout::Ru).as_deref(),
            Some("Привет")
        );
    }

    #[test]
    fn ru_reverse_direction_is_the_inverse() {
        assert_eq!(
            remap_word_cyrillic_to_latin("привет", CyrillicLayout::Ru).as_deref(),
            Some("ghbdtn")
        );
    }

    #[test]
    fn ru_word_with_unmapped_character_returns_none() {
        // '1' has no position on the letter rows in either table.
        assert_eq!(remap_word_latin_to_cyrillic("gh1", CyrillicLayout::Ru), None);
    }

    #[test]
    fn uk_hand_checkable_word_pryvit() {
        // "привіт" (Ukrainian for "hello"): p=g r=h i=b v=d i(dotted)=s t=n.
        assert_eq!(
            remap_word_latin_to_cyrillic("ghbdsn", CyrillicLayout::Uk).as_deref(),
            Some("привіт")
        );
    }

    #[test]
    fn uk_and_ru_diverge_at_the_s_key() {
        // Same physical key, different letter: Russian yeru vs Ukrainian i.
        assert_eq!(latin_to_cyrillic(CyrillicLayout::Ru, 's'), Some('ы'));
        assert_eq!(latin_to_cyrillic(CyrillicLayout::Uk, 's'), Some('і'));
    }

    #[test]
    fn uk_backslash_maps_to_ghe_with_upturn() {
        assert_eq!(latin_to_cyrillic(CyrillicLayout::Uk, '\\'), Some('ґ'));
    }
}
