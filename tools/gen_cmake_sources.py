#!/usr/bin/env python3
# Copyright 2026 The PDFium Authors
# Helper: emit cmake/PdfiumSources.cmake from known MVP source sets.
# Regenerates lists for AGG / system-freetype builds (optional V8; no XFA).

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "cmake" / "PdfiumSources.cmake"


def collect_cmaps():
    files = []
    base = ROOT / "core" / "fpdfapi" / "cmaps"
    for p in sorted(base.rglob("*.cpp")):
        files.append(p.relative_to(ROOT).as_posix())
    return files


def emit(name, files, comment=""):
    lines = [f"# {comment}" if comment else f"# {name}", f"set({name}"]
    for f in files:
        lines.append(f"  {f}")
    lines.append(")")
    lines.append("")
    return "\n".join(lines)


def main():
    sections = []
    sections.append(
        "# Generated for CMake MVP (AGG; optional V8; no XFA). Do not hand-edit."
    )
    sections.append("# Regenerate: python tools/gen_cmake_sources.py")
    sections.append("")

    sections.append(
        emit(
            "PDFIUM_AGG_SOURCES",
            [
                "third_party/agg23/agg_curves.cpp",
                "third_party/agg23/agg_path_storage.cpp",
                "third_party/agg23/agg_rasterizer_scanline_aa.cpp",
                "third_party/agg23/agg_vcgen_dash.cpp",
                "third_party/agg23/agg_vcgen_stroke.cpp",
            ],
            "third_party fx_agg",
        )
    )

    lcms = sorted(
        (ROOT / "third_party" / "lcms" / "src").glob("cms*.c")
    )
    sections.append(
        emit(
            "PDFIUM_LCMS_SOURCES",
            [p.relative_to(ROOT).as_posix() for p in lcms],
            "third_party fx_lcms2",
        )
    )

    opj_names = [
        "bio.c",
        "cio.c",
        "dwt.c",
        "event.c",
        "function_list.c",
        "ht_dec.c",
        "image.c",
        "invert.c",
        "j2k.c",
        "jp2.c",
        "mct.c",
        "mqc.c",
        "openjpeg.c",
        "opj_malloc.cc",
        "pi.c",
        "sparse_array.c",
        "t1.c",
        "t2.c",
        "tcd.c",
        "tgt.c",
        "thread.c",
    ]
    sections.append(
        emit(
            "PDFIUM_OPENJPEG_SOURCES",
            [f"third_party/libopenjpeg/{n}" for n in opj_names],
            "third_party fx_libopenjpeg",
        )
    )

    fxcrt_common = [
        "core/fxcrt/binary_buffer.cpp",
        "core/fxcrt/bytestring.cpp",
        "core/fxcrt/bytestring_pool.cpp",
        "core/fxcrt/cfx_bitstream.cpp",
        "core/fxcrt/cfx_datetime.cpp",
        "core/fxcrt/cfx_fileaccess_stream.cpp",
        "core/fxcrt/cfx_read_only_container_stream.cpp",
        "core/fxcrt/cfx_read_only_span_stream.cpp",
        "core/fxcrt/cfx_seekablestreamproxy.cpp",
        "core/fxcrt/cfx_timer.cpp",
        "core/fxcrt/debug/alias.cc",
        "core/fxcrt/fx_bidi.cpp",
        "core/fxcrt/fx_codepage.cpp",
        "core/fxcrt/fx_coordinates.cpp",
        "core/fxcrt/fx_extension.cpp",
        "core/fxcrt/fx_memory.cpp",
        "core/fxcrt/fx_memory_malloc.cpp",
        "core/fxcrt/fx_number.cpp",
        "core/fxcrt/fx_random.cpp",
        "core/fxcrt/fx_stream.cpp",
        "core/fxcrt/fx_string.cpp",
        "core/fxcrt/fx_system.cpp",
        "core/fxcrt/fx_unicode.cpp",
        "core/fxcrt/observed_ptr.cpp",
        "core/fxcrt/string_data_template.cpp",
        "core/fxcrt/string_template.cpp",
        "core/fxcrt/widestring.cpp",
        "core/fxcrt/widetext_buffer.cpp",
        "core/fxcrt/xml/cfx_xmlchardata.cpp",
        "core/fxcrt/xml/cfx_xmldocument.cpp",
        "core/fxcrt/xml/cfx_xmlelement.cpp",
        "core/fxcrt/xml/cfx_xmlinstruction.cpp",
        "core/fxcrt/xml/cfx_xmlnode.cpp",
        "core/fxcrt/xml/cfx_xmlparser.cpp",
        "core/fxcrt/xml/cfx_xmltext.cpp",
    ]
    sections.append(emit("PDFIUM_FXCRT_SOURCES", fxcrt_common, "core/fxcrt common"))
    sections.append(
        emit(
            "PDFIUM_FXCRT_POSIX_SOURCES",
            [
                "core/fxcrt/cfx_fileaccess_posix.cpp",
                "core/fxcrt/fx_folder_posix.cpp",
                "core/fxcrt/mapped_data_bytes.cpp",
            ],
            "core/fxcrt posix",
        )
    )
    sections.append(
        emit(
            "PDFIUM_FXCRT_WIN_SOURCES",
            [
                "core/fxcrt/cfx_fileaccess_windows.cpp",
                "core/fxcrt/code_point_view.cpp",
                "core/fxcrt/fx_folder_windows.cpp",
                "core/fxcrt/win/win_util.cc",
            ],
            "core/fxcrt win",
        )
    )

    fxcodec = [
        "core/fxcodec/basic/basicmodule.cpp",
        "core/fxcodec/data_and_bytes_consumed.cpp",
        "core/fxcodec/fax/faxmodule.cpp",
        "core/fxcodec/flate/flatemodule.cpp",
        "core/fxcodec/fx_codec.cpp",
        "core/fxcodec/icc/icc_transform.cpp",
        "core/fxcodec/jbig2/JBig2_ArithDecoder.cpp",
        "core/fxcodec/jbig2/JBig2_ArithIntDecoder.cpp",
        "core/fxcodec/jbig2/JBig2_BitStream.cpp",
        "core/fxcodec/jbig2/JBig2_Context.cpp",
        "core/fxcodec/jbig2/JBig2_DocumentContext.cpp",
        "core/fxcodec/jbig2/JBig2_GrdProc.cpp",
        "core/fxcodec/jbig2/JBig2_GrrdProc.cpp",
        "core/fxcodec/jbig2/JBig2_HTRDProc.cpp",
        "core/fxcodec/jbig2/JBig2_HuffmanDecoder.cpp",
        "core/fxcodec/jbig2/JBig2_HuffmanTable.cpp",
        "core/fxcodec/jbig2/JBig2_Image.cpp",
        "core/fxcodec/jbig2/JBig2_PatternDict.cpp",
        "core/fxcodec/jbig2/JBig2_PddProc.cpp",
        "core/fxcodec/jbig2/JBig2_SddProc.cpp",
        "core/fxcodec/jbig2/JBig2_Segment.cpp",
        "core/fxcodec/jbig2/JBig2_SymbolDict.cpp",
        "core/fxcodec/jbig2/JBig2_TrdProc.cpp",
        "core/fxcodec/jbig2/jbig2_decoder.cpp",
        "core/fxcodec/jpeg/jpeg_common.c",
        "core/fxcodec/jpeg/jpegmodule.cpp",
        "core/fxcodec/jpx/cjpx_decoder.cpp",
        "core/fxcodec/jpx/jpx_decode_utils.cpp",
        "core/fxcodec/scanlinedecoder.cpp",
    ]
    # Verify jbig2 filenames on disk — casing may differ.
    jbig2_dir = ROOT / "core" / "fxcodec" / "jbig2"
    real_jbig2 = sorted(
        p.relative_to(ROOT).as_posix()
        for p in jbig2_dir.glob("*.cpp")
        if "unittest" not in p.name and "test" not in p.name
    )
    fxcodec_fixed = [
        f
        for f in fxcodec
        if not f.startswith("core/fxcodec/jbig2/")
    ] + real_jbig2
    sections.append(emit("PDFIUM_FXCODEC_SOURCES", fxcodec_fixed, "core/fxcodec"))

    fxge = [
        "core/fxge/calculate_pitch.cpp",
        "core/fxge/cfx_charmap_resolver.cpp",
        "core/fxge/cfx_color.cpp",
        "core/fxge/cfx_cttgsubtable.cpp",
        "core/fxge/cfx_drawutils.cpp",
        "core/fxge/cfx_face.cpp",
        "core/fxge/cfx_folderfontinfo.cpp",
        "core/fxge/cfx_font.cpp",
        "core/fxge/cfx_fontmapper.cpp",
        "core/fxge/cfx_fontmgr.cpp",
        "core/fxge/cfx_gemodule.cpp",
        "core/fxge/cfx_glyphbitmap.cpp",
        "core/fxge/cfx_glyphcache.cpp",
        "core/fxge/cfx_graphstate.cpp",
        "core/fxge/cfx_graphstatedata.cpp",
        "core/fxge/cfx_path.cpp",
        "core/fxge/cfx_renderdevice.cpp",
        "core/fxge/cfx_standardfont.cpp",
        "core/fxge/cfx_substfont.cpp",
        "core/fxge/dib/blend.cpp",
        "core/fxge/dib/cfx_bitmapstorer.cpp",
        "core/fxge/dib/cfx_cmyk_to_srgb.cpp",
        "core/fxge/dib/cfx_dibbase.cpp",
        "core/fxge/dib/cfx_dibitmap.cpp",
        "core/fxge/dib/cfx_imagestretcher.cpp",
        "core/fxge/dib/cfx_imagetransformer.cpp",
        "core/fxge/dib/cfx_scanlinecompositor.cpp",
        "core/fxge/dib/cstretchengine.cpp",
        "core/fxge/dib/fx_dib.cpp",
        "core/fxge/freetype/fx_freetype.cpp",
        "core/fxge/fx_font.cpp",
        "core/fxge/renderdevicedriver_iface.cpp",
        "core/fxge/text_char_pos.cpp",
        "core/fxge/text_glyph_pos.cpp",
        "core/fxge/agg/cfx_agg_bitmapcomposer.cpp",
        "core/fxge/agg/cfx_agg_cliprgn.cpp",
        "core/fxge/agg/cfx_agg_devicedriver.cpp",
        "core/fxge/agg/cfx_agg_imagerenderer.cpp",
    ]
    fontdata = sorted(
        (ROOT / "core" / "fxge" / "fontdata" / "chromefontdata").glob("*.cpp")
    )
    fxge += [p.relative_to(ROOT).as_posix() for p in fontdata]
    sections.append(emit("PDFIUM_FXGE_SOURCES", fxge, "core/fxge + AGG"))
    sections.append(
        emit(
            "PDFIUM_FXGE_LINUX_SOURCES",
            ["core/fxge/linux/fx_linux_impl.cpp"],
            "fxge linux",
        )
    )
    sections.append(
        emit(
            "PDFIUM_FXGE_MAC_SOURCES",
            [
                "core/fxge/apple/capple_platform.cpp",
                "core/fxge/apple/cquartz_2d.cpp",
                "core/fxge/apple/fx_apple_impl.cpp",
            ],
            "fxge mac",
        )
    )
    sections.append(
        emit(
            "PDFIUM_FXGE_WIN_SOURCES",
            [
                "core/fxge/win32/cfx_psfonttracker.cpp",
                "core/fxge/win32/cfx_psrenderer.cpp",
                "core/fxge/win32/cgdi_device_driver.cpp",
                "core/fxge/win32/cgdi_display_driver.cpp",
                "core/fxge/win32/cgdi_plus_ext.cpp",
                "core/fxge/win32/cgdi_printer_driver.cpp",
                "core/fxge/win32/cps_printer_driver.cpp",
                "core/fxge/win32/cpsoutput.cpp",
                "core/fxge/win32/ctext_only_printer_driver.cpp",
                "core/fxge/win32/cwin32_platform.cpp",
            ],
            "fxge win",
        )
    )

    sections.append(
        emit(
            "PDFIUM_FDRM_SOURCES",
            [
                "core/fdrm/fx_crypt.cpp",
                "core/fdrm/fx_crypt_aes.cpp",
                "core/fdrm/fx_crypt_sha.cpp",
            ],
            "fdrm",
        )
    )

    sections.append(emit("PDFIUM_CMAPS_SOURCES", collect_cmaps(), "fpdfapi/cmaps"))

    def sources_from_gnish(
        subdir,
        skip_substrings=(
            "unittest",
            "embeddertest",
            "test_support",
            "test_with_",
            "test_document",
            "_test.cpp",
            "_test.cc",
        ),
    ):
        files = []
        for p in sorted((ROOT / subdir).glob("*.cpp")):
            name = p.name
            if any(s in name for s in skip_substrings):
                continue
            files.append(p.relative_to(ROOT).as_posix())
        return files

    sections.append(
        emit(
            "PDFIUM_FONT_SOURCES",
            sources_from_gnish("core/fpdfapi/font"),
            "fpdfapi/font",
        )
    )
    sections.append(
        emit(
            "PDFIUM_PAGE_SOURCES",
            sources_from_gnish("core/fpdfapi/page"),
            "fpdfapi/page",
        )
    )
    parser = sources_from_gnish("core/fpdfapi/parser")
    parser = [f for f in parser if "seekablemultistream" not in f]
    sections.append(emit("PDFIUM_PARSER_SOURCES", parser, "fpdfapi/parser"))

    render = sources_from_gnish("core/fpdfapi/render")
    render_win = [f for f in render if "scaledrenderbuffer" in f]
    render = [f for f in render if "scaledrenderbuffer" not in f]
    sections.append(emit("PDFIUM_RENDER_SOURCES", render, "fpdfapi/render"))
    sections.append(
        emit("PDFIUM_RENDER_WIN_SOURCES", render_win, "fpdfapi/render win")
    )

    edit = sources_from_gnish("core/fpdfapi/edit")
    sections.append(emit("PDFIUM_EDIT_SOURCES", edit, "fpdfapi/edit"))

    sections.append(
        emit(
            "PDFIUM_FPDFDOC_SOURCES",
            sources_from_gnish("core/fpdfdoc"),
            "fpdfdoc",
        )
    )
    sections.append(
        emit(
            "PDFIUM_FPDFTEXT_SOURCES",
            sources_from_gnish("core/fpdftext"),
            "fpdftext",
        )
    )

    fpdfsdk = sources_from_gnish("fpdfsdk")
    sections.append(emit("PDFIUM_FPDFSDK_SOURCES", fpdfsdk, "fpdfsdk"))
    sections.append(
        emit(
            "PDFIUM_FORMFILLER_SOURCES",
            sources_from_gnish("fpdfsdk/formfiller"),
            "formfiller",
        )
    )
    sections.append(
        emit(
            "PDFIUM_PWL_SOURCES",
            sources_from_gnish("fpdfsdk/pwl"),
            "pwl",
        )
    )
    sections.append(
        emit(
            "PDFIUM_FXJS_STUB_SOURCES",
            [
                "fxjs/cjs_event_context_stub.cpp",
                "fxjs/cjs_runtimestub.cpp",
                "fxjs/ijs_runtime.cpp",
            ],
            "fxjs stubs (always built)",
        )
    )
    # Matches fxjs/BUILD.gn sources += when pdf_enable_v8 (no XFA / no gc).
    fxjs_v8 = [
        "fxjs/cfx_globaldata.cpp",
        "fxjs/cfx_isolate_wrapper.cpp",
        "fxjs/cfx_keyvalue.cpp",
        "fxjs/cfx_v8_array_buffer_allocator.cpp",
        "fxjs/cfxjs_engine.cpp",
        "fxjs/cjs_annot.cpp",
        "fxjs/cjs_app.cpp",
        "fxjs/cjs_border.cpp",
        "fxjs/cjs_color.cpp",
        "fxjs/cjs_console.cpp",
        "fxjs/cjs_delaydata.cpp",
        "fxjs/cjs_display.cpp",
        "fxjs/cjs_document.cpp",
        "fxjs/cjs_event.cpp",
        "fxjs/cjs_event_context.cpp",
        "fxjs/cjs_field.cpp",
        "fxjs/cjs_font.cpp",
        "fxjs/cjs_global.cpp",
        "fxjs/cjs_globalarrays.cpp",
        "fxjs/cjs_globalconsts.cpp",
        "fxjs/cjs_highlight.cpp",
        "fxjs/cjs_icon.cpp",
        "fxjs/cjs_object.cpp",
        "fxjs/cjs_position.cpp",
        "fxjs/cjs_publicmethods.cpp",
        "fxjs/cjs_result.cpp",
        "fxjs/cjs_runtime.cpp",
        "fxjs/cjs_scalehow.cpp",
        "fxjs/cjs_scalewhen.cpp",
        "fxjs/cjs_style.cpp",
        "fxjs/cjs_timerobj.cpp",
        "fxjs/cjs_util.cpp",
        "fxjs/cjs_zoomtype.cpp",
        "fxjs/fx_date_helpers.cpp",
        "fxjs/fxv8.cpp",
        "fxjs/global_timer.cpp",
        "fxjs/js_define.cpp",
        "fxjs/js_resources.cpp",
    ]
    sections.append(emit("PDFIUM_FXJS_V8_SOURCES", fxjs_v8, "fxjs V8 (no XFA)"))

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(sections) + "\n", encoding="utf-8")

    missing = []
    text = OUT.read_text(encoding="utf-8")
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("set(") or line == ")":
            continue
        p = ROOT / line
        if not p.exists():
            missing.append(line)
    if missing:
        print("MISSING FILES:")
        for m in missing:
            print(" ", m)
        raise SystemExit(1)
    print(f"Wrote {OUT} OK")


if __name__ == "__main__":
    main()
