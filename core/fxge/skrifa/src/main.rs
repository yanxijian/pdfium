// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Based on https://github.com/googlefonts/fontations/pull/1820

#![allow(dead_code)]
use read_fonts::{
    model::pen::OutlinePen,
    ps::{
        cff::{
            charset::Charset as CffCharset, CffFontRef, Encoding as CffEncoding,
            Metadata as CffMetadata, Subfont as CffSubfont,
        },
        charmap::Charmap as PsCharmap,
        encoding::PredefinedEncoding,
        string::Sid,
        type1::Type1Font,
    },
    types::GlyphId,
    TableProvider,
};
use skrifa::MetadataProvider;

#[cxx::bridge(namespace = "skrifa")]
mod skrifa_ffi {
    #[derive(Copy, Clone, PartialEq, Eq, Debug)]
    pub enum PsEncodingKind {
        None = 0,
        Standard = 1,
        Expert = 2,
        IsoLatin1 = 3,
        Custom = 4,
    }

    #[derive(Copy, Clone, PartialEq, Eq, Debug)]
    pub enum FsType {
        RestrictedLicenseEmbedding = 0x0002,
        BitmapEmbeddingOnly = 0x0200,
    }

    // Should match SkPathVerb
    #[derive(Copy, Clone, PartialEq, Eq, Debug)]
    #[repr(u8)]
    pub enum PathVerb {
        MoveTo = 0,
        LineTo = 1,
        QuadTo = 2,
        CurveTo = 4,
        Close = 5,
    }

    #[derive(Copy, Clone, PartialEq, Debug)]
    pub struct Point {
        pub x: f32,
        pub y: f32,
    }

    #[derive(Copy, Clone, PartialEq, Debug)]
    pub struct BoundingBox {
        pub x_min: f32,
        pub y_min: f32,
        pub x_max: f32,
        pub y_max: f32,
    }

    #[derive(Copy, Clone, PartialEq, Eq, Debug)]
    pub struct CodePageRange {
        pub range1: u32,
        pub range2: u32,
    }

    #[derive(Copy, Clone, PartialEq, Eq, Debug)]
    pub struct Os2Panose {
        pub b0: u8,
        pub b1: u8,
    }

    #[derive(Copy, Clone, PartialEq, Eq, Debug)]
    pub struct UnicodeRange {
        pub range1: u32,
        pub range2: u32,
        pub range3: u32,
        pub range4: u32,
    }

    #[derive(Copy, Clone, PartialEq, Eq, Debug)]
    pub struct CharCodeAndIndex {
        pub char_code: u32,
        pub glyph_index: u32,
    }

    #[derive(Clone, Debug)]
    pub struct Outline {
        pub verbs: Vec<PathVerb>,
        pub points: Vec<Point>,
        pub advance_width: f32,
    }

    extern "Rust" {
        type PsFont<'a>;
        unsafe fn new_ps_font<'a>(data: &'a [u8]) -> Box<PsFont<'a>>;
        fn is_ok(&self) -> bool;
        unsafe fn name<'a>(&'a self) -> &'a str;
        unsafe fn family_name<'a>(&'a self) -> &'a str;
        fn units_per_em(&self) -> i32;
        fn ascent(&self) -> f32;
        fn descent(&self) -> f32;
        fn num_glyphs(&self) -> u32;
        fn is_cid(&self) -> bool;
        fn cid_to_gid(&self, cid: u16) -> u32;
        fn unicode_to_gid(&self, unicode: u32) -> u32;
        fn encoding(&self) -> PsEncodingKind;
        fn code_to_gid(&self, code: u8) -> u32;
        fn scaled_outline(&self, gid: u32, ppem: f32, outline: &mut Outline) -> bool;
        fn unscaled_outline(&self, gid: u32, outline: &mut Outline) -> bool;
        fn get_os2_code_page_range(data: &[u8], range: &mut CodePageRange) -> bool;
        fn get_os2_panose(data: &[u8], panose: &mut Os2Panose) -> bool;
        fn get_os2_fs_type(data: &[u8], fs_type: &mut u16) -> bool;
        fn get_os2_unicode_range(data: &[u8], range: &mut UnicodeRange) -> bool;
        fn get_style_name(data: &[u8]) -> String;
        fn get_glyph_name(data: &[u8], gid: u32) -> String;
        fn get_name_index(data: &[u8], name: &str) -> u32;

        fn agl_name_to_unicode(name: &str, unicode: &mut u32) -> bool;
        fn agl_unicode_to_name(unicode: u32, name: &mut [u8]) -> bool;

        fn get_char_codes_and_indices(data: &[u8], max_char: u32) -> Vec<CharCodeAndIndex>;
        fn has_glyph_names(data: &[u8]) -> bool;
        fn is_fixed_pitch(data: &[u8]) -> bool;
        fn is_scalable(data: &[u8]) -> bool;
        fn get_font_format(data: &[u8]) -> String;
        fn get_glyph_bounds(data: &[u8], glyph_index: u32) -> BoundingBox;
        fn is_tricky(data: &[u8], name: &str) -> bool;
    }

    unsafe extern "C++" {
        include!("outlines.h");

        fn run(font_path: &str);
    }
}

use skrifa_ffi::{Outline, PathVerb, Point, PsEncodingKind};

pub enum PsFont<'a> {
    Type1(Type1Font),
    Cff(CffFont<'a>),
    Error,
}

pub struct CffFont<'a> {
    font: CffFontRef<'a>,
    meta: Option<CffMetadata<'a>>,
    charset: Option<CffCharset<'a>>,
    encoding: Option<CffEncoding<'a>>,
    unicode_cmap: Option<PsCharmap>,
    subfonts: Vec<Option<CffSubfont>>,
}

pub fn new_ps_font(data: &[u8]) -> Box<PsFont<'_>> {
    let font = if let Ok(cff) = CffFontRef::new(data, 0, None) {
        let meta = cff.metadata();
        let charset = cff.charset();
        let encoding = cff.encoding();
        let subfonts = (0..cff.num_subfonts()).map(|i| cff.subfont(i, &[]).ok()).collect();
        let unicode_cmap = if let Some(charset) = charset.as_ref() {
            Some(PsCharmap::from_glyph_names(charset.iter().filter_map(|(gid, sid)| {
                Some((gid, core::str::from_utf8(cff.string(sid)?).ok()?))
            })))
        } else {
            None
        };
        PsFont::Cff(CffFont { font: cff, meta, charset, encoding, unicode_cmap, subfonts })
    } else if let Ok(type1) = Type1Font::new(data) {
        PsFont::Type1(type1)
    } else {
        PsFont::Error
    };
    Box::new(font)
}

impl PsFont<'_> {
    fn is_ok(&self) -> bool {
        !matches!(self, Self::Error)
    }

    fn name(&self) -> &str {
        match self {
            Self::Type1(type1) => type1.name().unwrap_or_default(),
            Self::Cff(cff) => cff.meta.as_ref().and_then(|meta| meta.name()).unwrap_or_default(),
            Self::Error => "",
        }
    }

    fn family_name(&self) -> &str {
        match self {
            Self::Type1(type1) => type1.family_name().unwrap_or_default(),
            Self::Cff(cff) => {
                cff.meta.as_ref().and_then(|meta| meta.family_name()).unwrap_or_default()
            }
            Self::Error => "",
        }
    }

    fn units_per_em(&self) -> i32 {
        match self {
            Self::Type1(type1) => type1.upem(),
            Self::Cff(cff) => cff.font.upem(),
            Self::Error => 0,
        }
    }

    fn ascent(&self) -> f32 {
        let bbox = match self {
            Self::Type1(type1) => type1.bbox(),
            Self::Cff(cff) => cff.meta.as_ref().map(|meta| meta.bbox()).unwrap_or_default(),
            Self::Error => return 0.0,
        };
        bbox.y_max.to_f32()
    }

    fn descent(&self) -> f32 {
        let bbox = match self {
            Self::Type1(type1) => type1.bbox(),
            Self::Cff(cff) => cff.meta.as_ref().map(|meta| meta.bbox()).unwrap_or_default(),
            Self::Error => return 0.0,
        };
        bbox.y_min.to_f32()
    }

    fn num_glyphs(&self) -> u32 {
        match self {
            Self::Type1(type1) => type1.num_glyphs(),
            Self::Cff(cff) => cff.font.num_glyphs(),
            Self::Error => 0,
        }
    }

    fn unicode_to_gid(&self, unicode: u32) -> u32 {
        let gid = match self {
            Self::Type1(type1) => type1.unicode_charmap().map(unicode),
            Self::Cff(cff) => cff.unicode_cmap.as_ref().and_then(|cmap| cmap.map(unicode)),
            Self::Error => return 0,
        };
        gid.unwrap_or_default().to_u32()
    }

    fn encoding(&self) -> PsEncodingKind {
        let maybe_predefined = match self {
            Self::Type1(type1) => type1.encoding().map(|encoding| encoding.predefined()),
            Self::Cff(cff) => cff.encoding.as_ref().map(|encoding| encoding.predefined()),
            Self::Error => return PsEncodingKind::None,
        };
        let Some(predefined) = maybe_predefined else {
            return PsEncodingKind::None;
        };
        match predefined {
            Some(PredefinedEncoding::Standard) => PsEncodingKind::Standard,
            Some(PredefinedEncoding::Expert) => PsEncodingKind::Custom,
            Some(PredefinedEncoding::IsoLatin1) => PsEncodingKind::IsoLatin1,
            None => PsEncodingKind::Custom,
        }
    }

    fn code_to_gid(&self, code: u8) -> u32 {
        let gid = match self {
            Self::Type1(type1) => type1.encoding().and_then(|encoding| encoding.map(code)),
            Self::Cff(cff) => cff.encoding.as_ref().and_then(|encoding| encoding.map(code)),
            Self::Error => return 0,
        };
        let gid = gid.unwrap_or_default().to_u32();
        if gid < self.num_glyphs() {
            gid
        } else {
            0
        }
    }

    fn is_cid(&self) -> bool {
        match self {
            Self::Cff(cff) => cff.font.is_cid(),
            _ => false,
        }
    }

    fn cid_to_gid(&self, cid: u16) -> u32 {
        match self {
            Self::Cff(cff) if cff.font.is_cid() => cff
                .charset
                .as_ref()
                .and_then(|charset| charset.glyph_id(Sid::new(cid)).ok())
                .unwrap_or_default()
                .to_u32(),
            _ => 0,
        }
    }

    fn scaled_outline(&self, gid: u32, ppem: f32, outline: &mut Outline) -> bool {
        self.outline_impl(gid, Some(ppem), outline).is_some()
    }

    fn unscaled_outline(&self, gid: u32, outline: &mut Outline) -> bool {
        self.outline_impl(gid, None, outline).is_some()
    }

    fn outline_impl(&self, gid: u32, ppem: Option<f32>, outline: &mut Outline) -> Option<()> {
        outline.verbs.clear();
        outline.points.clear();
        outline.advance_width = 0.0;
        let width = match self {
            Self::Type1(type1) => type1.draw(gid.into(), ppem, outline).ok()??,
            Self::Cff(cff) => {
                let gid = GlyphId::new(gid);
                let subfont = cff.subfonts.get(cff.font.subfont_index(gid)? as usize)?.as_ref()?;
                cff.font.draw(subfont, gid, &[], ppem, outline).ok()??
            }
            Self::Error => return None,
        };
        outline.advance_width = width;
        Some(())
    }
}

impl Point {
    fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
}

impl Outline {
    fn push<const N: usize>(&mut self, verb: PathVerb, points: [(f32, f32); N]) {
        self.verbs.push(verb);
        self.points.extend(points.into_iter().map(|(x, y)| Point::new(x, y)));
    }
}

impl OutlinePen for Outline {
    fn move_to(&mut self, x: f32, y: f32) {
        self.push(PathVerb::MoveTo, [(x, y)]);
    }

    fn line_to(&mut self, x: f32, y: f32) {
        self.push(PathVerb::LineTo, [(x, y)]);
    }

    fn quad_to(&mut self, cx0: f32, cy0: f32, x: f32, y: f32) {
        self.push(PathVerb::QuadTo, [(cx0, cy0), (x, y)]);
    }

    fn curve_to(&mut self, cx0: f32, cy0: f32, cx1: f32, cy1: f32, x: f32, y: f32) {
        self.push(PathVerb::CurveTo, [(cx0, cy0), (cx1, cy1), (x, y)]);
    }

    fn close(&mut self) {
        self.push(PathVerb::Close, []);
    }
}

fn agl_name_to_unicode(name: &str, unicode: &mut u32) -> bool {
    if let Some(uni) = read_fonts::ps::agl::name_to_char(name) {
        *unicode = uni as u32;
        true
    } else {
        false
    }
}

fn agl_unicode_to_name(unicode: u32, name: &mut [u8]) -> bool {
    read_fonts::ps::agl::char_to_name(unicode, name).is_some()
}

pub fn get_os2_code_page_range(data: &[u8], range: &mut skrifa_ffi::CodePageRange) -> bool {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        use read_fonts::TableProvider;
        if let Ok(os2) = font.os2() {
            range.range1 = os2.ul_code_page_range_1().unwrap_or(0);
            range.range2 = os2.ul_code_page_range_2().unwrap_or(0);
            return true;
        }
    }
    false
}

pub fn get_os2_panose(data: &[u8], panose: &mut skrifa_ffi::Os2Panose) -> bool {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        use read_fonts::TableProvider;
        if let Ok(os2) = font.os2() {
            let p = os2.panose_10();
            panose.b0 = p[0];
            panose.b1 = p[1];
            return true;
        }
    }
    false
}

pub fn get_os2_fs_type(data: &[u8], fs_type: &mut u16) -> bool {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        use read_fonts::TableProvider;
        if let Ok(os2) = font.os2() {
            *fs_type = os2.fs_type();
            return true;
        }
    }
    false
}

pub fn get_os2_unicode_range(data: &[u8], range: &mut skrifa_ffi::UnicodeRange) -> bool {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        use read_fonts::TableProvider;
        if let Ok(os2) = font.os2() {
            range.range1 = os2.ul_unicode_range_1();
            range.range2 = os2.ul_unicode_range_2();
            range.range3 = os2.ul_unicode_range_3();
            range.range4 = os2.ul_unicode_range_4();
            return true;
        }
    }
    false
}

pub fn get_style_name(data: &[u8]) -> String {
    if let Ok(font) = skrifa::FontRef::new(data) {
        use skrifa::string::StringId;
        use skrifa::MetadataProvider;
        if let Some(name) = font.localized_strings(StringId::SUBFAMILY_NAME).english_or_first() {
            return name.to_string();
        }
    }
    String::new()
}

pub fn get_glyph_name(data: &[u8], gid: u32) -> String {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        let glyph_names = skrifa::GlyphNames::new(&font);
        if let Some(name) = glyph_names.get(skrifa::GlyphId::new(gid)) {
            return name.to_string();
        }
    }
    String::new()
}

pub fn get_name_index(data: &[u8], name: &str) -> u32 {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        let glyph_names = skrifa::GlyphNames::new(&font);
        if let Some(gid) = glyph_names.iter().find(|(_id, n)| n.as_str() == name).map(|(id, _n)| id)
        {
            return gid.to_u32();
        }
    }
    0
}

pub fn get_char_codes_and_indices(data: &[u8], max_char: u32) -> Vec<skrifa_ffi::CharCodeAndIndex> {
    let mut results = Vec::new();
    if let Ok(font) = read_fonts::FontRef::new(data) {
        let charmap = font.charmap();
        if charmap.has_map() {
            for (char_code, glyph_id) in charmap.mappings() {
                if char_code > max_char {
                    break;
                }
                results.push(skrifa_ffi::CharCodeAndIndex {
                    char_code,
                    glyph_index: glyph_id.to_u32(),
                });
            }
            return results;
        }

        if let Ok(cmap) = font.cmap() {
            for record in cmap.encoding_records() {
                if let Ok(read_fonts::tables::cmap::CmapSubtable::Format0(format0)) =
                    record.subtable(cmap.offset_data())
                {
                    for (code, &gid) in format0.glyph_id_array().iter().enumerate() {
                        if gid != 0 {
                            let char_code = code as u32;
                            if char_code <= max_char {
                                results.push(skrifa_ffi::CharCodeAndIndex {
                                    char_code,
                                    glyph_index: gid as u32,
                                });
                            }
                        }
                    }
                    return results;
                }
            }

            if let Some((_, _, subtable)) = cmap.best_subtable() {
                for (char_code, glyph_id) in subtable.iter() {
                    if char_code > max_char {
                        continue;
                    }
                    results.push(skrifa_ffi::CharCodeAndIndex {
                        char_code,
                        glyph_index: glyph_id.to_u32(),
                    });
                }
                results.sort_by_key(|r| r.char_code);
                if let Some(pos) = results.iter().position(|r| r.char_code > max_char) {
                    results.truncate(pos);
                }
                return results;
            }
        }
    }
    results
}

pub fn has_glyph_names(data: &[u8]) -> bool {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        let glyph_names = skrifa::GlyphNames::new(&font);
        return glyph_names.source() != skrifa::GlyphNameSource::Synthesized;
    }
    false
}

pub fn is_fixed_pitch(data: &[u8]) -> bool {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        use read_fonts::TableProvider;
        if let Ok(post) = font.post() {
            return post.is_fixed_pitch() != 0;
        }
    }
    false
}

pub fn is_scalable(data: &[u8]) -> bool {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        use read_fonts::TableProvider;
        return font.glyf().is_ok() || font.cff().is_ok() || font.cff2().is_ok();
    }
    if read_fonts::ps::cff::CffFontRef::new(data, 0, None).is_ok() {
        return true;
    }
    if read_fonts::ps::type1::Type1Font::new(data).is_ok() {
        return true;
    }
    false
}

pub fn get_font_format(data: &[u8]) -> String {
    if read_fonts::ps::type1::Type1Font::new(data).is_ok() {
        return "Type 1".to_string();
    }
    if let Ok(font) = read_fonts::FontRef::new(data) {
        use read_fonts::TableProvider;
        if font.cff().is_ok() || font.cff2().is_ok() {
            return "CFF".to_string();
        }
        if font.glyf().is_ok() {
            return "TrueType".to_string();
        }
    }
    if read_fonts::ps::cff::CffFontRef::new(data, 0, None).is_ok() {
        return "CFF".to_string();
    }
    String::new()
}

pub fn get_glyph_bounds(data: &[u8], glyph_index: u32) -> skrifa_ffi::BoundingBox {
    if let Ok(font) = read_fonts::FontRef::new(data) {
        let metrics = skrifa::metrics::GlyphMetrics::new(
            &font,
            skrifa::instance::Size::unscaled(),
            skrifa::instance::LocationRef::default(),
        );
        if let Some(bbox) = metrics.bounds(skrifa::GlyphId::new(glyph_index)) {
            return skrifa_ffi::BoundingBox {
                x_min: bbox.x_min,
                y_min: bbox.y_min,
                x_max: bbox.x_max,
                y_max: bbox.y_max,
            };
        }
    }
    skrifa_ffi::BoundingBox { x_min: 0.0, y_min: 0.0, x_max: 0.0, y_max: 0.0 }
}

// The following data and logic are derived from FreeType's
// `src/truetype/ttobjs.c` and are subject to the FreeType License (FTL).
// Copyright (C) 2026 The FreeType Project (www.freetype.org). All rights
// reserved.

struct SfntIdRec {
    checksum: u32,
    length: u32,
}

const TRICK_NAMES_COUNT: usize = 20;
const TRICK_NAMES: [&str; TRICK_NAMES_COUNT] = [
    "cpop",
    "DFGirl-W6-WIN-BF",
    "DFGothic-EB",
    "DFGyoSho-Lt",
    "DFHei",
    "DFHSGothic-W5",
    "DFHSMincho-W3",
    "DFHSMincho-W7",
    "DFKaiSho-SB",
    "DFKaiShu",
    "DFKai-SB",
    "DFMing",
    "DLC",
    "HuaTianKaiTi?",
    "HuaTianSongTi?",
    "Ming(for ISO10646)",
    "MingLiU",
    "MingMedium",
    "PMingLiU",
    "MingLi43",
];

const TRICK_SFNT_IDS_NUM_FACES: usize = 31;
const TRICK_SFNT_IDS_PER_FACE: usize = 3;

const TRICK_SFNT_ID_CVT: usize = 0;
const TRICK_SFNT_ID_FPGM: usize = 1;
const TRICK_SFNT_ID_PREP: usize = 2;

const SFNT_IDS: [[SfntIdRec; TRICK_SFNT_IDS_PER_FACE]; TRICK_SFNT_IDS_NUM_FACES] = [
    [
        /* MingLiU 1995 */
        SfntIdRec { checksum: 0x05BCF058, length: 0x000002E4 },
        SfntIdRec { checksum: 0x28233BF1, length: 0x000087C4 },
        SfntIdRec { checksum: 0xA344A1EA, length: 0x000001E1 },
    ],
    [
        /* MingLiU 1996- */
        SfntIdRec { checksum: 0x05BCF058, length: 0x000002E4 },
        SfntIdRec { checksum: 0x28233BF1, length: 0x000087C4 },
        SfntIdRec { checksum: 0xA344A1EB, length: 0x000001E1 },
    ],
    [
        /* DFGothic-EB */
        SfntIdRec { checksum: 0x12C3EBB2, length: 0x00000350 },
        SfntIdRec { checksum: 0xB680EE64, length: 0x000087A7 },
        SfntIdRec { checksum: 0xCE939563, length: 0x00000758 },
    ],
    [
        /* DFGyoSho-Lt */
        SfntIdRec { checksum: 0x11E5EAD4, length: 0x00000350 },
        SfntIdRec { checksum: 0xCE5956E9, length: 0x0000BC85 },
        SfntIdRec { checksum: 0x8272F416, length: 0x00000045 },
    ],
    [
        /* DFHei-Md-HK-BF */
        SfntIdRec { checksum: 0x1257EB46, length: 0x00000350 },
        SfntIdRec { checksum: 0xF699D160, length: 0x0000715F },
        SfntIdRec { checksum: 0xD222F568, length: 0x000003BC },
    ],
    [
        /* DFHSGothic-W5 */
        SfntIdRec { checksum: 0x1262EB4E, length: 0x00000350 },
        SfntIdRec { checksum: 0xE86A5D64, length: 0x00007940 },
        SfntIdRec { checksum: 0x7850F729, length: 0x000005FF },
    ],
    [
        /* DFHSMincho-W3 */
        SfntIdRec { checksum: 0x122DEB0A, length: 0x00000350 },
        SfntIdRec { checksum: 0x3D16328A, length: 0x0000859B },
        SfntIdRec { checksum: 0xA93FC33B, length: 0x000002CB },
    ],
    [
        /* DFHSMincho-W7 */
        SfntIdRec { checksum: 0x125FEB26, length: 0x00000350 },
        SfntIdRec { checksum: 0xA5ACC982, length: 0x00007EE1 },
        SfntIdRec { checksum: 0x90999196, length: 0x0000041F },
    ],
    [
        /* DFKaiShu */
        SfntIdRec { checksum: 0x11E5EAD4, length: 0x00000350 },
        SfntIdRec { checksum: 0x5A30CA3B, length: 0x00009063 },
        SfntIdRec { checksum: 0x13A42602, length: 0x0000007E },
    ],
    [
        /* DFKaiShu, variant */
        SfntIdRec { checksum: 0x11E5EAD4, length: 0x00000350 },
        SfntIdRec { checksum: 0xA6E78C01, length: 0x00008998 },
        SfntIdRec { checksum: 0x13A42602, length: 0x0000007E },
    ],
    [
        /* DFKaiShu-Md-HK-BF */
        SfntIdRec { checksum: 0x11E5EAD4, length: 0x00000360 },
        SfntIdRec { checksum: 0x9DB282B2, length: 0x0000C06E },
        SfntIdRec { checksum: 0x53E6D7CA, length: 0x00000082 },
    ],
    [
        /* DFMing-Bd-HK-BF */
        SfntIdRec { checksum: 0x1243EB18, length: 0x00000350 },
        SfntIdRec { checksum: 0xBA0A8C30, length: 0x000074AD },
        SfntIdRec { checksum: 0xF3D83409, length: 0x0000037B },
    ],
    [
        /* DLCLiShu */
        SfntIdRec { checksum: 0x07DCF546, length: 0x00000308 },
        SfntIdRec { checksum: 0x40FE7C90, length: 0x00008E2A },
        SfntIdRec { checksum: 0x608174B5, length: 0x0000007A },
    ],
    [
        /* DLCHayBold */
        SfntIdRec { checksum: 0xEB891238, length: 0x00000308 },
        SfntIdRec { checksum: 0xD2E4DCD4, length: 0x0000676F },
        SfntIdRec { checksum: 0x8EA5F293, length: 0x000003B8 },
    ],
    [
        /* HuaTianKaiTi */
        SfntIdRec { checksum: 0xFFFBFFFC, length: 0x00000008 },
        SfntIdRec { checksum: 0x9C9E48B8, length: 0x0000BEA2 },
        SfntIdRec { checksum: 0x70020112, length: 0x00000008 },
    ],
    [
        /* HuaTianSongTi */
        SfntIdRec { checksum: 0xFFFBFFFC, length: 0x00000008 },
        SfntIdRec { checksum: 0x0A5A0483, length: 0x00017C39 },
        SfntIdRec { checksum: 0x70020112, length: 0x00000008 },
    ],
    [
        /* NEC fadpop7.ttf */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x40C92555, length: 0x000000E5 },
        SfntIdRec { checksum: 0xA39B58E3, length: 0x0000117C },
    ],
    [
        /* NEC fadrei5.ttf */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x33C41652, length: 0x000000E5 },
        SfntIdRec { checksum: 0x26D6C52A, length: 0x00000F6A },
    ],
    [
        /* NEC fangot7.ttf */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x6DB1651D, length: 0x0000019D },
        SfntIdRec { checksum: 0x6C6E4B03, length: 0x00002492 },
    ],
    [
        /* NEC fangyo5.ttf */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x40C92555, length: 0x000000E5 },
        SfntIdRec { checksum: 0xDE51FAD0, length: 0x0000117C },
    ],
    [
        /* NEC fankyo5.ttf */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x85E47664, length: 0x000000E5 },
        SfntIdRec { checksum: 0xA6C62831, length: 0x00001CAA },
    ],
    [
        /* NEC fanrgo5.ttf */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x2D891CFD, length: 0x0000019D },
        SfntIdRec { checksum: 0xA0604633, length: 0x00001DE8 },
    ],
    [
        /* NEC fangot5.ttc */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x40AA774C, length: 0x000001CB },
        SfntIdRec { checksum: 0x9B5CAA96, length: 0x00001F9A },
    ],
    [
        /* NEC fanmin3.ttc */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x0D3DE9CB, length: 0x00000141 },
        SfntIdRec { checksum: 0xD4127766, length: 0x00002280 },
    ],
    [
        /* NEC FA-Gothic, 1996 */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x4A692698, length: 0x000001F0 },
        SfntIdRec { checksum: 0x340D4346, length: 0x00001FCA },
    ],
    [
        /* NEC FA-Minchou, 1996 */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0xCD34C604, length: 0x00000166 },
        SfntIdRec { checksum: 0x6CF31046, length: 0x000022B0 },
    ],
    [
        /* NEC FA-RoundGothicB, 1996 */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0x5DA75315, length: 0x0000019D },
        SfntIdRec { checksum: 0x40745A5F, length: 0x000022E0 },
    ],
    [
        /* NEC FA-RoundGothicM, 1996 */
        SfntIdRec { checksum: 0x00000000, length: 0x00000000 },
        SfntIdRec { checksum: 0xF055FC48, length: 0x000001C2 },
        SfntIdRec { checksum: 0x3900DED3, length: 0x00001E18 },
    ],
    [
        /* MINGLI.TTF, 1992 */
        SfntIdRec { checksum: 0x00170003, length: 0x00000060 },
        SfntIdRec { checksum: 0xDBB4306E, length: 0x000058AA },
        SfntIdRec { checksum: 0xD643482A, length: 0x00000035 },
    ],
    [
        /* DFHei-Bd-WIN-HK-BF */
        SfntIdRec { checksum: 0x1269EB58, length: 0x00000350 },
        SfntIdRec { checksum: 0x5CD5957A, length: 0x00006A4E },
        SfntIdRec { checksum: 0xF758323A, length: 0x00000380 },
    ],
    [
        /* DFMing-Md-WIN-HK-BF */
        SfntIdRec { checksum: 0x122FEB0B, length: 0x00000350 },
        SfntIdRec { checksum: 0x7F10919A, length: 0x000070A9 },
        SfntIdRec { checksum: 0x7CD7E7B7, length: 0x0000025C },
    ],
];

fn compute_sfnt_checksum(data: &[u8]) -> u32 {
    let mut checksum: u32 = 0;
    let mut chunks = data.chunks_exact(4);
    for chunk in &mut chunks {
        checksum =
            checksum.wrapping_add(u32::from_be_bytes([chunk[0], chunk[1], chunk[2], chunk[3]]));
    }
    let remainder = chunks.remainder();
    if !remainder.is_empty() {
        let mut last_bytes = [0u8; 4];
        last_bytes[..remainder.len()].copy_from_slice(remainder);
        checksum = checksum.wrapping_add(u32::from_be_bytes(last_bytes));
    }
    checksum
}

fn skip_pdf_random_tag(name: &str) -> &str {
    if name.len() >= 7 && name.as_bytes()[6] == b'+' {
        return &name[7..];
    }
    name
}

pub fn is_tricky(data: &[u8], name: &str) -> bool {
    let name_without_tag = skip_pdf_random_tag(name);
    for trick_name in TRICK_NAMES.iter() {
        if name_without_tag.contains(trick_name) {
            return true;
        }
    }

    if let Ok(font) = read_fonts::FontRef::new(data) {
        let mut num_matched_ids = [0; TRICK_SFNT_IDS_NUM_FACES];
        let mut has_cvt = false;
        let mut has_fpgm = false;
        let mut has_prep = false;

        let tables = [
            (Tag::new(b"cvt "), TRICK_SFNT_ID_CVT),
            (Tag::new(b"fpgm"), TRICK_SFNT_ID_FPGM),
            (Tag::new(b"prep"), TRICK_SFNT_ID_PREP),
        ];

        for (tag, k) in tables.iter() {
            if let Some(table_data) = font.table_data(*tag) {
                match *k {
                    TRICK_SFNT_ID_CVT => has_cvt = true,
                    TRICK_SFNT_ID_FPGM => has_fpgm = true,
                    TRICK_SFNT_ID_PREP => has_prep = true,
                    _ => {}
                }

                let length = table_data.len() as u32;
                let checksum = compute_sfnt_checksum(table_data);

                for j in 0..TRICK_SFNT_IDS_NUM_FACES {
                    if length == SFNT_IDS[j][*k].length && checksum == SFNT_IDS[j][*k].checksum {
                        num_matched_ids[j] += 1;
                        if num_matched_ids[j] == TRICK_SFNT_IDS_PER_FACE {
                            return true;
                        }
                    }
                }
            }
        }

        for j in 0..TRICK_SFNT_IDS_NUM_FACES {
            if !has_cvt && SFNT_IDS[j][TRICK_SFNT_ID_CVT].length == 0 {
                num_matched_ids[j] += 1;
            }
            if !has_fpgm && SFNT_IDS[j][TRICK_SFNT_ID_FPGM].length == 0 {
                num_matched_ids[j] += 1;
            }
            if !has_prep && SFNT_IDS[j][TRICK_SFNT_ID_PREP].length == 0 {
                num_matched_ids[j] += 1;
            }
            if num_matched_ids[j] == TRICK_SFNT_IDS_PER_FACE {
                return true;
            }
        }
    }
    false
}

fn main() {
    skrifa_ffi::run("");
}
