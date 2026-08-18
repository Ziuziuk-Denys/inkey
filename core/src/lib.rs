// Phase 0 FFI boundary: proves the engine (C) can call into core (Rust)
// and get a real transformed answer back. The transform itself
// (uppercasing) is a placeholder to make the call observable end to end -
// correction/prediction/punctuation logic replaces it in later phases.

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::panic;

mod layout_tables;

// Uppercases the input UTF-8 string and returns a newly allocated C string.
// The caller owns the returned pointer and must free it with
// inkey_free_string. A panic inside the transform, or invalid input, falls
// back to returning a copy of the original text instead of crashing the
// caller (ibus-daemon must never go down because of this call).
#[no_mangle]
pub extern "C" fn inkey_transform(input: *const c_char) -> *mut c_char {
    if input.is_null() {
        return std::ptr::null_mut();
    }

    let input_str = match unsafe { CStr::from_ptr(input) }.to_str() {
        Ok(s) => s.to_owned(),
        Err(_) => return duplicate_c_string(input),
    };

    let result = panic::catch_unwind(|| input_str.to_uppercase());

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
    fn uppercases_ascii() {
        let input = CString::new("hello").unwrap();
        let output = inkey_transform(input.as_ptr());
        let output_str = unsafe { CStr::from_ptr(output) }.to_str().unwrap();
        assert_eq!(output_str, "HELLO");
        inkey_free_string(output);
    }

    #[test]
    fn null_input_returns_null() {
        let output = inkey_transform(std::ptr::null());
        assert!(output.is_null());
    }
}
