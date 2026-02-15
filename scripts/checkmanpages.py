# This file is part of the Ssstr string library.
# Copyright 2022-2023 Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

# Check man pages for consistency, match with prototypes in header, and match
# of examples with test sources.
# Run `python checkmanpages.py` to see usage.

from __future__ import annotations

import difflib
import hashlib
import os.path
import shlex
import sys
from dataclasses import dataclass, field


@dataclass
class SoPage:
    path: str
    primary: str
    section: str
    so: str


@dataclass
class ParsedPage:
    path: str
    primary: str
    section: str
    header_title: str
    header_section: str
    header_date: str
    header_source: str
    header_manual: str
    sections: list[tuple[str, list[str]]]


@dataclass
class CheckedPage:
    path: str
    primary: str
    section: str
    header_date: str
    name_items: list[str]
    name_brief: str
    see_also: list[tuple[str, str]]


@dataclass
class CheckedFuncPage(CheckedPage):
    prototypes: list[str] = field(default_factory=list)
    snippets: list[str] | None = None


@dataclass
class CheckedIntroPage(CheckedPage):
    functions: set[str] = field(default_factory=set)


def unescape_roff(s):
    s = s.replace("\\(rs", "\\")
    assert "\\(" not in s  # Not strictly a violation
    return s


def split_sections(lines, request=".SH"):
    # Each section is a pair (heading, [content_lines])
    sections = []
    while len(lines):
        sh = lines.pop(0)
        assert sh.startswith(request)
        heading = sh.split(None, 1)[1].rstrip()
        content = []
        while len(lines) and not lines[0].startswith(request):
            content.append(lines.pop(0))
        sections.append((heading, content))
    return sections


def read_page(path: str) -> SoPage | ParsedPage:
    filename = os.path.basename(path)
    primary, section = filename.split(".")
    with open(path) as f:
        lines = f.readlines()
        assert len(lines), f"{path} has no lines"
        assert lines[-1].endswith(
            "\n"
        ), f"{path} is missing newline at end of last line"

        i = 0
        while lines[i].startswith(r".\""):
            i += 1
        firstline = lines[i]
        if firstline.startswith(".so "):
            assert (
                len(firstline.split()) == 2
            ), ".so must be followed by one word"
            assert (
                len(lines) == i + 1
            ), "file with .so must not have additional lines"
            return SoPage(
                path=path,
                primary=primary,
                section=section,
                so=firstline.split()[1],
            )

        assert firstline.startswith(".TH "), "first line must be .so or .TH"
        items = shlex.split(firstline, posix=False)
        assert (
            len(items) == 6
        ), ".TH must be followed by 5 possibly-quoted words"
        return ParsedPage(
            path=path,
            primary=primary,
            section=section,
            header_title=items[1],
            header_section=items[2],
            header_date=items[3],
            header_source=items[4],
            header_manual=items[5],
            sections=split_sections(lines[i + 1 :]),
        )


def _validate_header(page: ParsedPage) -> None:
    assert page.section == page.header_section
    assert page.primary.upper() == page.header_title
    assert page.header_source == "SSSTR"
    assert page.header_manual == '"Ssstr Manual"'


def _extract_sections(
    sections: list[tuple[str, list[str]]],
    required_first: list[str],
    required_last: list[str],
    optional_order: list[str],
) -> dict[str, list[str]]:
    sections = sections[:]
    result: dict[str, list[str]] = {}
    for name in required_first:
        heading, lines = sections.pop(0)
        assert heading == name, f"expected section {name}, found {heading}"
        result[name] = lines
    for name in reversed(required_last):
        heading, lines = sections.pop()
        assert heading == name
        result[name] = lines
    for name in optional_order:
        if sections and sections[0][0] == name:
            result[name] = sections.pop(0)[1]
    section_names = [s[0] for s in sections]
    if section_names:
        print(f"Unrecognized sections: {section_names}")
    assert not section_names
    return result


def _check_name(
    lines: list[str], primary: str, section: str
) -> tuple[list[str], str]:
    oneline = " ".join(line.strip() for line in lines)
    assert len(oneline.split(" \\- ")) == 2
    items, brief = oneline.split(" \\- ")
    items = items.split(", ")
    for item in items:
        assert " " not in item
        assert "," not in item
        if section == "3":
            assert item.startswith("ss8_")
    assert len(items)
    assert items[0] == primary
    return items, brief


def _check_synopsis(lines: list[str], name_items: list[str]) -> list[str]:
    lines = lines[:]
    assert lines.pop(0).rstrip() == ".nf"
    assert lines.pop().rstrip() == ".fi"
    assert lines.pop(0).rstrip() == ".B #include <ss8str.h>"
    assert lines.pop(0).rstrip() == ".PP"
    proto_lines = []  # Prototype lines, .BI removed
    for line in lines:
        if line.rstrip() == ".PP":
            continue
        assert line.startswith(".BI ")
        proto_lines.append(line[len(".BI ") :].rstrip())
    prototypes = []
    for line in proto_lines:
        if "SS8_STATIC_INITIALIZER" in line:  # Special case
            continue
        tokens = shlex.split(line.rstrip(), posix=False)
        assert len(tokens) % 2 == 1
        for odd_tok in tokens[0::2]:
            assert odd_tok.startswith('"') and odd_tok.endswith('"'), f"{line}"
        for even_tok in tokens[1::2]:
            assert '"' not in even_tok, f"{line}"
        unquoted = "".join(tok.strip('"') for tok in tokens)
        if unquoted.startswith(" "):
            assert prototypes[-1].endswith(",")
            prototypes[-1] = " ".join((prototypes[-1], unquoted.strip()))
        else:
            prototypes.append(unquoted)
    prototypes = [" ".join(proto.split()) for proto in prototypes]
    proto_names = []
    for proto in prototypes:
        assert proto.endswith(";")
        for word in proto.split():
            if "(" in word:
                name = word.split("(", 1)[0].lstrip("*")
                proto_names.append(name)
                break
    assert proto_names == name_items
    return prototypes


def _check_description(lines: list[str]) -> None:
    assert len(lines)


def _check_return_value(
    lines: list[str] | None, prototypes: list[str]
) -> None:
    n_nonvoid_func = 0
    for proto in prototypes:
        if proto.startswith("void ") and not proto.startswith("void *"):
            pass
        else:
            n_nonvoid_func += 1
    assert (
        (lines is not None) == (n_nonvoid_func > 0)
    ), f"page has {n_nonvoid_func} non-void functions, mismatched with presence of RETURN VALUE section"
    if lines is not None:
        assert len(lines)


def _check_errors(lines: list[str] | None) -> None:
    if lines is not None:
        assert len(lines)


def _check_notes(lines: list[str] | None) -> None:
    if lines is not None:
        assert len(lines)


def _check_examples(lines: list[str] | None) -> list[str] | None:
    if lines is None:
        return None
    assert len(lines)
    lines = [line.rstrip() for line in lines]
    snippets = []
    while True:
        try:
            ex_lineno = lines.index(".EX")
        except ValueError:
            break
        assert lines[ex_lineno - 1] == ".nf"
        ee_lineno = lines.index(".EE")
        assert lines[ee_lineno + 1] == ".fi"
        snippet_lines = lines[ex_lineno + 1 : ee_lineno]
        snippets.append(
            unescape_roff("".join(line + "\n" for line in snippet_lines))
        )
        lines = lines[ee_lineno + 1 :]
    assert len(snippets)
    return snippets


def _check_see_also(lines: list[str], section: str) -> list[tuple[str, str]]:
    items = []
    for line in lines:
        assert line.startswith(".BR ")
        items.append(line[len(".BR ") :].rstrip())
    assert len(items)
    assert items[-1].endswith(")"), "last SEE ALSO item must end with ')'"
    for item in items[:-1]:
        assert item.endswith(","), "non-last SEE ALSO item must end with ','"
    items = [item.rstrip(",") for item in items]
    item_sect = []
    for item in items:
        assert len(item.split()) == 2, f"{item}"
        name, sect = item.split()
        assert sect.startswith("(") and sect.endswith(")")
        sect = sect[1:-1]
        assert sect in [str(i) for i in range(1, 9)]
        item_sect.append((name, sect))
    if section == "3":
        assert item_sect[-1] == (
            "ssstr",
            "7",
        ), "last item of SEE ALSO must be ssstr(7)"
        sorted_item_sect = sorted(
            item_sect[:-1], key=lambda i: (i[1], i[0])
        ) + [item_sect[-1]]
    else:
        sorted_item_sect = sorted(item_sect, key=lambda i: (i[1], i[0]))
    assert (
        item_sect == sorted_item_sect
    ), f"found SEE ALSO items {item_sect}; should be sorted {sorted_item_sect}"
    return item_sect


def parse_function_variant_suffixes(lines):
    # We require at least 2 suffixes to be listed; if only 1, list plainly.
    suffixes = []
    line = lines.pop(0)
    items = line.split()
    if items[0] == ".B":
        assert len(items) == 2
        suffixes.append(items[1])
        line = lines.pop(0)
        assert line.rstrip() == "or"
    else:
        assert len(items) == 3
        assert items[0] == ".BR"
        assert items[2] == ","
        suffixes.append(items[1])
        while True:
            line = lines.pop(0)
            items = line.split()
            if line.rstrip() == "or":
                break
            assert len(items) == 3
            assert items[0] == ".BR"
            assert items[2] == ","
            suffixes.append(items[1])
        assert len(suffixes) > 1
    line = lines.pop(0)
    items = line.split()
    if items[0] == ".B":
        assert len(items) == 2
        suffixes.append(items[1])
        return suffixes, False
    elif items[0] == ".BR":
        assert len(items) == 3
        assert items[2] == ";"
        suffixes.append(items[1])
        return suffixes, True
    assert False


def expand_function_variants(funcs, suffixes):
    ret = []
    for func in funcs:
        if "printf" in func:  # Grandfathered
            func = "_".join(func.split("_")[:-1])
            assert "printf" not in func
        for suffix in suffixes:
            assert suffix.startswith("_")
            ret.append(func + suffix)
    return ret


def parse_function_variants_specific(plain_funcs, lines):
    line = lines.pop(0)
    items = line.split()
    assert len(items) == 2
    assert items[0] == ".B"
    func = items[1]
    assert func in plain_funcs
    line = lines.pop(0)
    assert line.rstrip() == "with"
    suffixes, continued = parse_function_variant_suffixes(lines)
    return expand_function_variants([func], suffixes), continued


def parse_function_variants(plain_funcs, lines):
    suffixes, continued = parse_function_variant_suffixes(lines)
    return expand_function_variants(plain_funcs, suffixes), continued


def parse_function_subsect(subsection, lines):
    funcs = []
    assert lines, f"empty subsection {subsection} of FUNCTIONS"
    can_continue = True
    can_continue_plain = True
    plainly_listed_funcs = []
    while lines:
        assert can_continue
        line = lines.pop(0)
        items = line.split()
        assert items
        if items[0] == ".BR":
            assert can_continue_plain
            assert len(items) == 3
            assert items[1].startswith("ss8_")
            assert items[2] == "(3)" or items[2] == "(3),"
            plainly_listed_funcs.append(items[1])
            funcs.append(items[1])
            if items[2].endswith(","):
                assert (
                    lines
                ), f"trailing comma in FUNCTIONS subsection {subsection}"
            else:
                can_continue_plain = False
            continue
        if line.rstrip() == "and variants of":
            assert len(plainly_listed_funcs) > 1
            fs, cont = parse_function_variants_specific(
                plainly_listed_funcs, lines
            )
        elif line.rstrip() == "and its variants with":
            assert len(plainly_listed_funcs) == 1
            fs, cont = parse_function_variants(plainly_listed_funcs, lines)
        elif line.rstrip() == "and their variants with":
            assert len(plainly_listed_funcs) > 1
            fs, cont = parse_function_variants(plainly_listed_funcs, lines)
        else:
            line = line.rstrip()
            assert False, f"bad line in FUNCTIONS subsection {subsection}"
        funcs.extend(fs)
        can_continue = cont
        can_continue_plain = cont
        plainly_listed_funcs = []
    return funcs


def parse_function_lines(lines):
    subsections = split_sections(lines, ".SS")
    funcs = set()
    for subsection, lines in subsections:
        assert subsection
        funcs.update(parse_function_subsect(subsection, lines))
    return funcs


def _check_functions(lines: list[str]) -> set[str]:
    return parse_function_lines(lines)


def check_func_page(page: ParsedPage) -> CheckedFuncPage:
    _validate_header(page)
    assert page.primary.startswith("ss8_")
    sects = _extract_sections(
        page.sections,
        required_first=["NAME", "SYNOPSIS", "DESCRIPTION"],
        required_last=["SEE ALSO"],
        optional_order=["RETURN VALUE", "ERRORS", "NOTES", "BUGS", "EXAMPLES"],
    )
    name_items, name_brief = _check_name(
        sects["NAME"], page.primary, page.section
    )
    prototypes = _check_synopsis(sects["SYNOPSIS"], name_items)
    _check_description(sects["DESCRIPTION"])
    _check_return_value(sects.get("RETURN VALUE"), prototypes)
    _check_errors(sects.get("ERRORS"))
    _check_notes(sects.get("NOTES"))
    snippets = _check_examples(sects.get("EXAMPLES"))
    see_also = _check_see_also(sects["SEE ALSO"], page.section)
    return CheckedFuncPage(
        path=page.path,
        primary=page.primary,
        section=page.section,
        header_date=page.header_date,
        name_items=name_items,
        name_brief=name_brief,
        see_also=see_also,
        prototypes=prototypes,
        snippets=snippets,
    )


def check_intro_page(page: ParsedPage) -> CheckedIntroPage:
    _validate_header(page)
    assert page.primary == "ssstr"
    sects = _extract_sections(
        page.sections,
        required_first=["NAME", "SYNOPSIS", "DESCRIPTION"],
        required_last=["SEE ALSO"],
        optional_order=["FUNCTIONS"],
    )
    assert "FUNCTIONS" in sects
    name_items, name_brief = _check_name(
        sects["NAME"], page.primary, page.section
    )
    _check_description(sects["DESCRIPTION"])
    functions = _check_functions(sects["FUNCTIONS"])
    see_also = _check_see_also(sects["SEE ALSO"], page.section)
    return CheckedIntroPage(
        path=page.path,
        primary=page.primary,
        section=page.section,
        header_date=page.header_date,
        name_items=name_items,
        name_brief=name_brief,
        see_also=see_also,
        functions=functions,
    )


def check_so_target(
    so_page: SoPage, pages: dict[str, CheckedPage], section: str
) -> None:
    target = so_page.so
    dir, name = target.split("/")
    assert dir == f"man{section}"
    suffix = f".{section}"
    assert name.endswith(suffix)
    name = name[: -len(suffix)]
    assert name in pages
    target_page = pages[name]
    assert name in target_page.name_items


def check_nonprimaries_have_so(
    page: CheckedPage, so_pages: dict[str, SoPage], section: str
) -> None:
    primary = page.name_items[0]
    so_target = f"man{section}/{primary}.{section}"
    nonprimaries = page.name_items[1:]
    for nonp in nonprimaries:
        assert (
            nonp in so_pages
        ), f"need .so page for {nonp} with target {so_target}"
        assert so_pages[nonp].so == so_target


def check_see_also_targets(
    page: CheckedPage,
    pages: dict[str, CheckedFuncPage],
    so_pages: dict[str, SoPage],
) -> None:
    for item, section in page.see_also:
        if item.startswith("ss8_") and section == "3":
            path = page.path
            assert (
                item in pages or item in so_pages
            ), f"page not found for {item} listed in SEE ALSO of {path}"


def check_dates_equal(pages: list[CheckedPage]) -> None:
    date = None
    for page in pages:
        if date is None:
            date = page.header_date
        else:
            path = page.path
            assert (
                page.header_date == date
            ), f"date in {path} differs from other pages"


def check_duplicate_items(pages: dict[str, CheckedFuncPage]) -> None:
    all_items = set()
    for page in pages.values():
        for item in page.name_items:
            assert (
                item not in all_items
            ), f"{item} appears more than once in manual pages"
            all_items.add(item)


def check_manpages(paths):
    func_pages: dict[str, CheckedFuncPage] = {}
    func_so_pages: dict[str, SoPage] = {}
    intro_so_pages: dict[str, SoPage] = {}
    intro_page: CheckedIntroPage | None = None
    for path in paths:
        try:
            raw = read_page(path)
        except BaseException:
            print(f"While reading {path}:", file=sys.stderr)
            raise
        if isinstance(raw, SoPage):
            if raw.section == "3":
                func_so_pages[raw.primary] = raw
            elif raw.section == "7":
                intro_so_pages[raw.primary] = raw
            else:
                assert False, f"unrecognized manual section {raw.section}"
        else:
            try:
                if raw.section == "3":
                    func_pages[raw.primary] = check_func_page(raw)
                elif raw.section == "7":
                    assert intro_page is None
                    intro_page = check_intro_page(raw)
                else:
                    assert False, f"unrecognized manual section {raw.section}"
            except BaseException:
                print(f"While checking {path}:", file=sys.stderr)
                raise

    for so in func_so_pages:
        check_so_target(func_so_pages[so], func_pages, "3")
    assert intro_page is not None
    for so in intro_so_pages:
        check_so_target(intro_so_pages[so], {"ssstr": intro_page}, "7")

    check_duplicate_items(func_pages)

    for primary in func_pages:
        check_nonprimaries_have_so(func_pages[primary], func_so_pages, "3")
        check_see_also_targets(func_pages[primary], func_pages, func_so_pages)
    check_nonprimaries_have_so(intro_page, intro_so_pages, "7")

    all_pages: list[CheckedPage] = list(func_pages.values()) + [intro_page]
    check_dates_equal(all_pages)

    return func_pages, intro_page


def read_header(header_path):
    with open(header_path) as f:
        lines = [line.rstrip() for line in f.readlines() if line.strip()]
    begin = lines.index("///// BEGIN_DOCUMENTED_PROTOTYPES")
    end = lines.index("///// END_DOCUMENTED_PROTOTYPES")
    prototypes = []
    for line in lines[begin + 1 : end]:
        if line.startswith("#") or line.startswith("SSSTR_ATTRIBUTE"):
            pass
        elif line.startswith(" "):
            prototypes[-1] = " ".join((prototypes[-1], line.lstrip()))
        else:
            prototypes.append(line)
    prototypes = [
        proto.replace("SSSTR_INLINE ", "").replace(
            "SSSTR_RESTRICT", "restrict"
        )
        for proto in prototypes
    ]
    return set(prototypes)


def check_prototypes(
    pages: dict[str, CheckedFuncPage], header_prototypes: set[str]
) -> None:
    man_prototypes = {
        proto for page in pages.values() for proto in page.prototypes
    }

    not_in_man = header_prototypes - man_prototypes
    n_not_in_man = len(not_in_man)
    if n_not_in_man:
        print(f"{n_not_in_man} prototypes not in man pages:", file=sys.stderr)
        for proto in sorted(not_in_man):
            print(proto, file=sys.stderr)

    not_in_header = man_prototypes - header_prototypes
    n_not_in_header = len(not_in_header)
    if n_not_in_header:
        print(
            f"{n_not_in_header} prototypes in man pages but not in header:",
            file=sys.stderr,
        )
        for proto in sorted(not_in_header):
            print(proto, file=sys.stderr)

    assert not not_in_man
    assert not not_in_header


def get_prototype_func_name(prototype):
    # First word before open paren '('
    words = prototype.split()
    for word in words:
        if "(" in word:
            before_paren = word.split("(")[0]
            return before_paren.lstrip("*")
    assert False, "failed to parse prototype"


def check_intro_listing(
    intro_page: CheckedIntroPage, header_prototypes: set[str]
) -> None:
    intro_funcs = intro_page.functions
    header_funcs = {get_prototype_func_name(p) for p in header_prototypes}

    not_in_intro = header_funcs - intro_funcs
    n_not_in_intro = len(not_in_intro)
    if n_not_in_intro:
        print(f"{n_not_in_intro} functions not in ssstr.7:", file=sys.stderr)
        for func in sorted(not_in_intro):
            print(func, file=sys.stderr)

    not_in_header = intro_funcs - header_funcs
    n_not_in_header = len(not_in_header)
    if n_not_in_header:
        print(
            f"{n_not_in_header} functions in ssstr.7 but not in header:",
            file=sys.stderr,
        )
        for func in sorted(not_in_header):
            print(func, file=sys.stderr)

    assert not not_in_intro
    assert not not_in_header


def normalize_snippet(snippet):
    lines = snippet.split("\n")
    for i in range(len(lines)):
        line = lines[i]
        line = " ".join(line.split())
        if line == '#include "ss8str.h"':
            line = "#include <ss8str.h>"
        cmt = line.find("//")
        if cmt >= 0:
            line = line[:cmt]
        lines[i] = line.strip()
    return " ".join(line for line in lines if line)


def read_test_snippets(paths):
    snippets = []
    for path in paths:
        with open(path) as f:
            lines = f.readlines()
        snippet_lines = []
        while True:
            try:
                begin = lines.index("#define SNIPPET\n")
            except ValueError:
                break
            end = lines.index("#undef SNIPPET\n")
            snippet_lines.extend(lines[begin + 1 : end])
            lines = lines[end + 1 :]
        snippet = "".join(snippet_lines)
        assert snippet, f"{path} must contain snippet segments"
        snippets.append(snippet)
    return snippets


def snippet_checksum(snippet):
    hasher = hashlib.sha1()
    hasher.update(normalize_snippet(snippet).encode())
    return hasher.digest()


def check_examples(
    pages: dict[str, CheckedFuncPage], example_test_paths: list[str]
) -> None:
    test_cksums = {
        snippet_checksum(s): s for s in read_test_snippets(example_test_paths)
    }
    for page in pages.values():
        if page.snippets is None:
            continue
        for snippet in page.snippets:
            man_cksum = snippet_checksum(snippet)
            title = page.path
            if man_cksum not in test_cksums:
                normsnip = normalize_snippet(snippet)
                print(f"Snippet in {title} (normalized):", file=sys.stderr)
                print(normsnip, file=sys.stderr)
                print(
                    "Most similar snippet(s) in tests (normalized):",
                    file=sys.stderr,
                )
                candidates = [
                    normalize_snippet(s) for s in test_cksums.values()
                ]
                candidates = difflib.get_close_matches(normsnip, candidates)
                for snip in candidates:
                    print(normalize_snippet(snip), file=sys.stderr)
            assert (
                man_cksum in test_cksums
            ), f"snippet in {title} must match test"


def check(header_path, example_test_paths, manpage_paths):
    pages, intro_page = check_manpages(manpage_paths)
    header_prototypes = read_header(header_path)
    check_prototypes(pages, header_prototypes)
    check_intro_listing(intro_page, header_prototypes)
    check_examples(pages, example_test_paths)


def usage():
    print(
        "Usage: python checkmanpages.py header example_tests ... -- man_pages ...",
        file=sys.stderr,
    )
    sys.exit(1)


if __name__ == "__main__":
    args = sys.argv[1:]
    if not len(args):
        usage()
    header_path = args.pop(0)
    try:
        dashdash = args.index("--")
    except ValueError:
        usage()
    example_test_paths = args[:dashdash]
    manpage_paths = args[dashdash + 1 :]
    check(header_path, example_test_paths, manpage_paths)
