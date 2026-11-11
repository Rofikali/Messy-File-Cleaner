//! Test-only support for future FCJ4 parser verification.
//!
//! This crate deliberately contains no FCJ4 grammar, parser implementation,
//! filesystem operations, production API, or FFI. It preserves arbitrary
//! bytes so later test harnesses do not accidentally treat hostile input as
//! UTF-8 text.

#![forbid(unsafe_code)]

/// An owned, arbitrary byte sequence for parser test inputs.
///
/// The type makes the test boundary explicit: input is bytes, not a string.
/// It is not a protocol record and performs no FCJ4 validation.
#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct ByteInput {
    bytes: Vec<u8>,
}

impl ByteInput {
    /// Copies an arbitrary byte slice into an owned test input.
    #[must_use]
    pub fn from_slice(bytes: &[u8]) -> Self {
        Self {
            bytes: bytes.to_vec(),
        }
    }

    /// Returns the exact input bytes without text conversion.
    #[must_use]
    pub fn as_bytes(&self) -> &[u8] {
        &self.bytes
    }

    /// Returns the number of input bytes.
    #[must_use]
    pub fn len(&self) -> usize {
        self.bytes.len()
    }

    /// Returns whether the input contains no bytes.
    #[must_use]
    pub fn is_empty(&self) -> bool {
        self.bytes.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::ByteInput;

    #[test]
    fn preserves_arbitrary_bytes_without_utf8_assumptions() {
        let source = [0x00, 0xFF, b'F', b'C', b'J', 0x80, b'\n'];
        let input = ByteInput::from_slice(&source);

        assert_eq!(input.as_bytes(), source);
        assert_eq!(input.len(), source.len());
        assert!(!input.is_empty());
    }

    #[test]
    fn owns_a_copy_of_the_original_input() {
        let mut source = [0x00, 0x41, 0xFF];
        let input = ByteInput::from_slice(&source);
        source.fill(0x7F);

        assert_eq!(input.as_bytes(), [0x00, 0x41, 0xFF]);
        assert_eq!(source, [0x7F; 3]);
    }

    #[test]
    fn accepts_empty_input() {
        let input = ByteInput::from_slice(&[]);

        assert!(input.is_empty());
        assert_eq!(input.len(), 0);
        assert!(input.as_bytes().is_empty());
    }
}
