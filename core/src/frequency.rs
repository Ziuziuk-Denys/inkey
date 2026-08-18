// Word-frequency lookup tables for EN/RU/UK, used to score whether a
// buffered word looks like a real word in a given language.
//
// Data source: wordfreq (MIT licensed), top 50k words per language with
// their zipf frequency (log-scaled, roughly 0-8; higher = more common),
// extracted offline and embedded as plain text at compile time.
//
// Ukrainian is intentionally best-effort only. dict_uk/VESUM - the
// higher-quality morphological Ukrainian dictionary that would catch
// inflected forms outside a raw frequency list - is CC BY-NC-SA
// (NonCommercial), which is incompatible with OSI distribution and
// SignPath eligibility, so it is not used here (already decided, not
// revisited in this phase). Plain frequency-list lookup has no
// morphological awareness: a valid Ukrainian word in an inflected form
// that isn't itself in the top 50k list will score as out-of-vocabulary
// even though it's correct. This is a known, accepted quality gap versus
// EN/RU for this phase, not a silent limitation.
use std::collections::HashMap;
use std::sync::OnceLock;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Lang {
    En,
    Ru,
    Uk,
}

const EN_DATA: &str = include_str!("../data/freq_en.txt");
const RU_DATA: &str = include_str!("../data/freq_ru.txt");
const UK_DATA: &str = include_str!("../data/freq_uk.txt");

fn parse(data: &str) -> HashMap<&str, f32> {
    data.lines()
        .filter_map(|line| {
            let mut parts = line.splitn(2, '\t');
            let word = parts.next()?;
            let freq: f32 = parts.next()?.parse().ok()?;
            Some((word, freq))
        })
        .collect()
}

fn table_for(lang: Lang) -> &'static HashMap<&'static str, f32> {
    static EN: OnceLock<HashMap<&str, f32>> = OnceLock::new();
    static RU: OnceLock<HashMap<&str, f32>> = OnceLock::new();
    static UK: OnceLock<HashMap<&str, f32>> = OnceLock::new();
    match lang {
        Lang::En => EN.get_or_init(|| parse(EN_DATA)),
        Lang::Ru => RU.get_or_init(|| parse(RU_DATA)),
        Lang::Uk => UK.get_or_init(|| parse(UK_DATA)),
    }
}

// Returns `word`'s frequency score for `lang` (0.0 if it isn't in the top
// 50k list for that language). Lookup is case-insensitive.
pub fn score(lang: Lang, word: &str) -> f32 {
    let lower = word.to_lowercase();
    table_for(lang).get(lower.as_str()).copied().unwrap_or(0.0)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn en_known_word_scores_above_zero() {
        assert!(score(Lang::En, "hello") > 0.0);
    }

    #[test]
    fn en_wrong_layout_gibberish_scores_zero() {
        assert_eq!(score(Lang::En, "ghbdtn"), 0.0);
    }

    #[test]
    fn ru_known_word_scores_above_zero() {
        assert!(score(Lang::Ru, "привет") > 0.0);
    }

    #[test]
    fn ru_latin_word_scores_zero() {
        assert_eq!(score(Lang::Ru, "ghbdtn"), 0.0);
    }

    #[test]
    fn uk_known_word_scores_above_zero() {
        assert!(score(Lang::Uk, "привіт") > 0.0);
    }

    #[test]
    fn lookup_is_case_insensitive() {
        assert_eq!(score(Lang::En, "Hello"), score(Lang::En, "hello"));
        assert!(score(Lang::En, "Hello") > 0.0);
    }

    #[test]
    fn unknown_garbage_word_scores_zero() {
        assert_eq!(score(Lang::En, "zxqvbkqq"), 0.0);
    }
}
