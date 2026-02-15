# This file is part of the Ssstr string library.
# Copyright 2022-2023 Board of Regents of the University of Wisconsin System
# SPDX-License-Identifier: MIT

# Generate static HTML pages from man pages

import html
import os
import os.path
import re
import shutil
import subprocess
import sys


class CheckError(Exception):
    pass


def get_dest_for_src(manpage_path, destdir):
    path, name = os.path.split(manpage_path)
    _, dir = os.path.split(path)
    name, section = name.split(".")
    if dir not in (f"man{section}", f"link{section}"):
        raise CheckError(f"unexpected directory {dir!r} for section {section}")
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
                while True:
                    line = f.readline()
                    if not line.startswith(r".\""):
                        break
                words = line.split()
                if len(words) != 2 or words[0] != ".so":
                    raise CheckError(
                        f"expected .so directive with one argument, got: {line.rstrip()!r}"
                    )
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


def write_redirect(link, target):
    redirect = f"""
<!DOCTYPE html>
<html>
  <link rel="canonical" href="{target}">
  <meta http-equiv="refresh" content="0; url={target}">
</html>
"""
    with open(link, "w") as f:
        f.write(redirect.lstrip())


def generate_redirect(dest, src, link_targets):
    with open(src) as f:
        while True:
            line = f.readline()
            if not line.startswith('.\\"'):
                break
        words = line.split()
        if words[0] != ".so":
            raise CheckError(f"expected .so directive, got: {line.rstrip()!r}")
        realpage = words[1]

    dir, name = realpage.split("/")
    name, section = name.split(".")
    key = f"{name}({section})"
    target = link_targets[key]

    write_redirect(dest, target)


def sgr_to_html(text):
    known_codes = {0, 1, 4, 22, 24}
    parts = re.split(r"(\x1b\[[0-9;]*m)", text)
    bold = False
    italic = False
    result = ["<pre>\n"]
    for part in parts:
        m = re.fullmatch(r"\x1b\[([0-9;]*)m", part)
        if m:
            codes = (
                [int(c) for c in m.group(1).split(";") if c]
                if m.group(1)
                else [0]
            )
            for code in codes:
                if code not in known_codes:
                    raise CheckError(f"unknown SGR code: {code}")
                if code == 0:
                    bold = False
                    italic = False
                elif code == 1:
                    bold = True
                elif code == 4:
                    italic = True
                elif code == 22:
                    bold = False
                elif code == 24:
                    italic = False
        else:
            if "\x1b" in part:
                raise CheckError(
                    f"unexpected escape sequence in groff output: {part!r}"
                )
            escaped = html.escape(part, quote=False)
            if bold and italic:
                result.append(f"<b><i>{escaped}</i></b>")
            elif bold:
                result.append(f"<b>{escaped}</b>")
            elif italic:
                result.append(f"<i>{escaped}</i>")
            else:
                result.append(escaped)
    result.append("</pre>")
    return "".join(result)


def simplify_html_styling(text):
    text = re.sub(r"<b>( *)</b>", r"\1", text)
    text = re.sub(r"<i>( *)</i>", r"\1", text)
    text = re.sub(r"</b>( *)<b>", r"\1", text)
    text = re.sub(r"</i>( *)<i>", r"\1", text)
    text = re.sub(r"( *)</b>", r"</b>\1", text)
    text = re.sub(r"( *)</i>", r"</i>\1", text)
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
  <style>
    body {{ background: #fff; color: #000; }}
    pre {{ width: fit-content; margin: 0 auto; }}
    pre b {{ color: #1a1a8a; }}
    pre i {{ color: #8a1a1a; }}
    pre a b, pre a i {{ color: inherit; }}
    a {{ color: #0e6e6e; }}
    a:visited {{ color: #551a8a; }}
    @media (prefers-color-scheme: dark) {{
      body {{ background: #1a1a1a; color: #d4d4d4; }}
      pre b {{ color: #7a9aef; }}
      pre i {{ color: #ef7a7a; }}
      a {{ color: #5ac8c8; }}
      a:visited {{ color: #b07aef; }}
    }}
  </style>
</head>
<body>
{text}
</body>
</html>
"""
    return html


def generate_html(dest, src, groff, link_targets):
    with open(src) as f:
        while True:
            line = f.readline()
            if not line.startswith('.\\"'):
                break
        if line.startswith(".so "):
            return generate_redirect(dest, src, link_targets)

    nroff_result = subprocess.run(
        [groff, "-Tutf8", "-man", src], check=True, capture_output=True
    )
    text = nroff_result.stdout.decode()
    if "\x08" in text:
        raise CheckError(
            "groff produced overprint (backspace) output; expected SGR sequences"
        )
    text = sgr_to_html(text)
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
        except BaseException:
            print(
                f"While generating {destpath} from {srcpath}", file=sys.stderr
            )
            raise

    write_redirect(os.path.join(destdir, "index.html"), "man7/ssstr.7.html")


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
    try:
        generate(destdir, groff, manpage_paths)
    except CheckError as e:
        print(f"error: {e}", file=sys.stderr)
        sys.exit(1)
