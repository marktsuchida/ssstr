# This file is part of the Ssstr string library.
# Copyright 2022, Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

# Generate static HTML pages from man pages

import html
import os, os.path
import re
import shutil
import subprocess
import sys


def get_dest_for_src(manpage_path, destdir):
    path, name = os.path.split(manpage_path)
    _, dir = os.path.split(path)
    name, section = name.split(".")
    assert dir in (f"man{section}", f"link{section}")
    return os.path.join(destdir, f"man{section}", f"{name}.{section}.html")


def get_link_targets(manpage_paths):
    # Return dict, keys like ss8_init(3), values like "../man3/ss8_init.3.html"
    # But resolve link files, so that ss8_back(3) maps to "../man3/ss8_at.html"
    ret = dict()
    for path in manpage_paths:
        dir, name = os.path.split(path)
        parent, dir = os.path.split(dir)
        name, section = name.split(".")
        if dir.startswith("link"):
            with open(path) as f:
                words = f.readline().split()
                assert len(words) == 2 and words[0] == ".so"
                target = words[1]
                path = os.path.join(parent, target)
        ret[f"{name}({section})"] = get_dest_for_src(path, "..")
    return ret


def get_external_link_targets():
    return {
        "string(3)": "https://www.man7.org/linux/man-pages/man3/string.3.html",
        "bstring(3)": "https://www.man7.org/linux/man-pages/man3/bstring.3.html",
        "sprintf(3)": "https://www.man7.org/linux/man-pages/man3/snprintf.3.html",
        "vsnprintf(3)": "https://www.man7.org/linux/man-pages/man3/vsnprintf.3.html",
    }


def generate_redirect(dest, src, link_targets):
    with open(src) as f:
        words = f.readline().split()
        assert words[0] == ".so"
        realpage = words[1]

    dir, name = realpage.split("/")
    name, section = name.split(".")
    key = f"{name}({section})"
    target = link_targets[key]

    html = f"""<!DOCTYPE html>
<html lang="en-US">
  <meta charset="utf-8">
  <title>Redirecting&hellip;</title>
  <link rel="canonical" href="{target}">
  <script>location="{target}"</script>
  <meta http-equiv="refresh" content="0; url={target}">
  <meta name="robots" content="noindex">
  <h1>Redirecting&hellip;</h1>
  <a href="{target}">Click here if you are not redirected.</a>
</html>
"""

    with open(dest, "w") as outfile:
        outfile.write(html)


def read_overprint_char(text, pos, end):
    assert pos < end
    assert text[pos] != "\b", f"unexpected backspace at {pos}"
    if pos + 1 == end or text[pos + 1] != "\b":
        return pos + 1, "R", text[pos]
    else:
        assert pos + 2 < end, "incomplete overprint sequence at end of text"
        if text[pos] == "_":
            if pos + 3 < end and text[pos + 3] == "\b":
                assert (
                    pos + 4 < end
                ), "incomplete overprint sequence at end of text"
                assert (
                    text[pos + 2] == text[pos + 4]
                ), f"incorrect overprint sequence at {pos}"
                return pos + 5, "BI", text[pos + 2]
            elif text[pos + 2] == "_":
                return pos + 3, "B/I", "_"
            else:
                return pos + 3, "I", text[pos + 2]
        else:
            assert text[pos] == text[pos + 2]
            return pos + 3, "B", text[pos]


def apply_html_style(style, text):
    text = html.escape(text, quote=False)
    ltag, rtag = {
        "R": ("", ""),
        "B": ("<b>", "</b>"),
        "I": ("<i>", "</i>"),
        "BI": ("<b><i>", "</b></i>"),
    }[style]
    return ltag + text + rtag


def overprint_to_html(text):
    # Replace streaks of '_\bx' and 'x\bx' with italic and bold, respectively
    # (also '_\bx\bx' for bold italic).
    # We also need to escape <, >, and &, but cannot do this first because they
    # may be bold or italic. So we first make a list of styled chuncks, escape
    # each chunk, and then combine.
    # Also, '_\b_' is ambiguous (could be bold or italic). So we put
    # underscores that are bold or italic (but not both) into their own chunk.
    chunks = []  # (style, text) where style in ("R", "B", "I", "BI", "B/I")
    end = len(text)
    pos = 0
    while pos < end:
        pos, style, ch = read_overprint_char(text, pos, end)
        if not chunks or chunks[-1][0] != style:
            chunks.append((style, ch))
        else:
            chunks[-1] = (style, chunks[-1][1] + ch)

    # Merge ambiguous chunks (bold or italic "_"s) of flanked by bold or italic
    i = 0
    while i < len(chunks):
        style, text = chunks[i]
        if style != "B/I":
            i += 1
            continue
        left_style = None if i == 0 else chunks[i - 1][0]
        right_style = None if i + 1 == len(chunks) else chunks[i + 1][0]
        assert right_style != "B/I"
        ambiguous = f"ambiguous underscoare style, flanked by {left_style} and {right_style}"
        if left_style in (None, "R"):
            if right_style in (None, "R"):
                assert False, ambiguous
            elif right_style in ("B", "I"):
                underscores = chunks.pop(i)[1]
                chunk_right = chunks[i]
                chunks[i] = (right_style, underscores + chunk_right[1])
            else:
                assert False, ambiguous
        elif right_style in (None, "R"):
            if left_style in ("B", "I"):
                underscores = chunks.pop(i)[1]
                i -= 1
                chunk_left = chunks[i]
                chunks[i] = (left_style, chunk_left[1] + underscores)
            else:
                assert False, ambiguous
        else:
            if (left_style, right_style) in (("B", "B"), ("I", "I")):
                underscores = chunks.pop(i)[1]
                next_chunk = chunks.pop(i)
                i -= 1
                chunk_left = chunks[i]
                chunks[i] = (
                    left_style,
                    chunk_left[1] + underscores + next_chunk[1],
                )
            else:
                assert False, ambiguous
        i += 1

    return (
        "<pre>\n"
        + "".join(apply_html_style(s, t) for s, t in chunks)
        + "</pre>"
    )


def simplify_html_styling(text):
    # This cannot undo all cases of excessive tags, but gets enough of them
    text = re.sub(r"</b>( *)<b>", r"\1", text)
    text = re.sub(r"</i>( *)<i>", r"\1", text)
    return text


def hyperlink(text, link_targets):
    # Some links may be hyphenated at a line break. Let's link those first.
    def link_hyphenated(match):
        name0, hyphen, indent, name1, section = (
            match.group(i) for i in range(1, 6)
        )
        key = f"{name0}{name1}({section})"
        try:
            url = link_targets[key]
            # Because we break the <a> but use a single <b>, we avoid matching
            # the second line again below as an unhyphenated link
            return f'<a href="{url}"><b>{name0}{hyphen}</a>\n{indent}<a href="{url}">{name1}</b>({section})</a>'
        except KeyError:
            print(
                f"broken link to {key} (hyphenated {name0}-{name1})",
                file=sys.stderr,
            )
            raise

    # 'groff -Tutf8' uses '\u2010' HYPHEN, but be safe ('\u2013' is EN DASH)
    text = re.sub(
        r"<b>([_A-Za-z][_A-Za-z0-9]*)([-\u2010\u2013])</b>\n( {4,8})<b>([_A-Za-z0-9]+)</b>\(([1-8])\)",
        link_hyphenated,
        text,
        flags=re.MULTILINE,
    )

    def link(match):
        name = match.group(1)
        section = match.group(2)
        key = f"{name}({section})"
        try:
            url = link_targets[key]
            return f'<a href="{url}"><b>{name}</b>({section})</a>'
        except KeyError:
            print(f"broken link to {key}", file=sys.stderr)
            raise

    text = re.sub(r"<b>([_A-Za-z][_A-Za-z0-9]*)</b>\(([1-8])\)", link, text)
    return text


def hyperlink_header(text, link_targets):
    def link(match):
        left, space0, center, space1, right = (
            match.group(i) for i in range(1, 6)
        )
        url = link_targets["ssstr(7)"]
        return f'{left}{space0}<a href="{url}">{center}</a>{space1}{right}'

    text = re.sub(
        r"^([_A-Z][_A-Z0-9]*\([1-8]\))( +)(Ssstr Manual)( +)([_A-Z][_A-Z0-9]*\([1-8]\))$",
        link,
        text,
        flags=re.MULTILINE,
    )
    return text


def add_html_header_footer(src, text):
    dir, name = os.path.split(src)
    name, section = name.split(".")
    title = f"{name}({section}) &mdash; Ssstr Manual"
    html = f"""<!DOCTYPE html>
<html lang="en-US">
<head>
  <meta charset="utf-8">
  <title>{title}</title>
</head>
<body>
{text}
</body>
</html>
"""
    return html


def generate_html(dest, src, groff, link_targets):
    with open(src) as f:
        firstline = f.readline()
        if firstline.startswith(".so "):
            return generate_redirect(dest, src, link_targets)

    # On macOS groff seems to use overprint whether we like it or not, so let's
    # always use '-c'.
    nroff_result = subprocess.run(
        [groff, "-Tutf8", "-c", "-man", src], check=True, capture_output=True
    )
    text = nroff_result.stdout.decode()
    text = overprint_to_html(text)
    text = hyperlink(text, link_targets)
    text = simplify_html_styling(text)
    text = hyperlink_header(text, link_targets)
    text = add_html_header_footer(src, text)
    with open(dest, "w") as outfile:
        outfile.write(text)


def generate(destdir, groff, manpage_paths):
    if os.path.exists(destdir):
        shutil.rmtree(destdir)
    os.mkdir(destdir)
    for section in (3, 7):
        os.mkdir(os.path.join(destdir, f"man{section}"))

    link_targets = get_link_targets(manpage_paths)
    link_targets.update(get_external_link_targets())

    for srcpath in manpage_paths:
        destpath = get_dest_for_src(srcpath, destdir)
        try:
            generate_html(destpath, srcpath, groff, link_targets)
        except:
            print(
                f"While generating {destpath} from {srcpath}", file=sys.stderr
            )
            raise


def usage():
    print(
        "Usage: python htmlman.py destdir /path/to/groff man_pages ...",
        file=sys.stderr,
    )
    sys.exit(1)


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) < 2:
        usage()
    destdir = args.pop(0)
    groff = args.pop(0)
    manpage_paths = args
    generate(destdir, groff, manpage_paths)
