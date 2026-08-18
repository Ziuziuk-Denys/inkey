// FFI boundary between the C IBus engine and the Rust core. The engine
// buffers one word at a time and calls inkey_transform once per word
// boundary (see engine/src/engine.c) - not per keystroke, as Phase 0 did.

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::panic;

mod detect;
mod frequency;
mod layout_tables;

// Applies word-boundary EN<->RU/UK correction to the input UTF-8 word and
// returns a newly allocated C string (the corrected word, or a copy of the
// input if no correction applies). The caller owns the returned pointer
// and must free it with inkey_free_string. A panic inside the transform,
// or invalid input, falls back to returning a copy of the original text
// instead of crashing the caller (ibus-daemon must never go down because
// of this call).
#[no_mangle]
pub extern "C" fn inkey_transform(input: *const c_char) -> *mut c_char {
    if input.is_null() {
        return std::ptr::null_mut();
    }

    let input_str = match unsafe { CStr::from_ptr(input) }.to_str() {
        Ok(s) => s.to_owned(),
        Err(_) => return duplicate_c_string(input),
    };

    let result = panic::catch_unwind(|| detect::correct_word(&input_str));

    match result {
        Ok(transformed) => match CString::new(transformed) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => duplicate_c_string(input),
        },
        Err(_) => duplicate_c_string(input),
    }
}

// Frees a string previously returned by inkey_transform.
#[no_mangle]
pub extern "C" fn inkey_free_string(s: *mut c_char) {
    if s.is_null() {
        return;
    }
    unsafe {
        drop(CString::from_raw(s));
    }
}

fn duplicate_c_string(input: *const c_char) -> *mut c_char {
    unsafe { CStr::from_ptr(input) }.to_owned().into_raw()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn corrects_wrong_layout_word_through_the_ffi_boundary() {
        let input = CString::new("ghbdtn").unwrap();
        let output = inkey_transform(input.as_ptr());
        let output_str = unsafe { CStr::from_ptr(output) }.to_str().unwrap();
        assert_eq!(output_str, "привет");
        inkey_free_string(output);
    }

    #[test]
    fn leaves_correctly_typed_word_untouched_through_the_ffi_boundary() {
        let input = CString::new("hello").unwrap();
        let output = inkey_transform(input.as_ptr());
        let output_str = unsafe { CStr::from_ptr(output) }.to_str().unwrap();
        assert_eq!(output_str, "hello");
        inkey_free_string(output);
    }

    #[test]
    fn null_input_returns_null() {
        let output = inkey_transform(std::ptr::null());
        assert!(output.is_null());
    }
}
