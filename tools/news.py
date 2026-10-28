# SPDX-License-Identifier: GPL-3.0-or-later
"""Writes the launcher's news: the news.toml in the news gist (docs/news.md).

    python tools/news.py list
    python tools/news.py add --title "The new launcher" [--tag update] [--link https://...] [--pin]
                             [--body "Markdown" | --body-file text.md] [--id ID] [--date 2026-10-16]
    python tools/news.py remove ID
    python tools/news.py pin ID | unpin ID
    python tools/news.py check FILE

Changes go straight to the gist (through the GitHub CLI, gh, signed in as its
owner), unless --file works on a local copy instead or --dry-run only prints
the result. Every change is checked the way the launcher reads the feed first.
"""
from __future__ import annotations

import argparse
import datetime
import json
import re
import subprocess
import sys
import tomllib
from pathlib import Path

# The news gist (its owner can edit it), and the file in it.
GIST = ""
FILE = "news.toml"

HEADER = """# OpenGP news: shown in the launcher, newest first (pinned items on top).
# Each item: id (unique), date, title, and optionally tag (update,
# announcement or event), pinned, link (https) and body (Markdown).
# Edit by hand or with tools/news.py in the opengp-launcher repo.
"""

TAGS = ("update", "announcement", "event")


def check(text: str) -> list[dict]:
    """The feed's items, if the launcher would show them all; raises ValueError if not."""
    try:
        data = tomllib.loads(text)
    except tomllib.TOMLDecodeError as e:
        raise ValueError(f"not TOML: {e}") from e
    items = data.get("news")
    if not isinstance(items, list):
        raise ValueError("no [[news]] items")
    seen = set()
    for n, item in enumerate(items, 1):
        where = f"item {n} ({item.get('id', 'no id')})"
        for key in ("id", "date", "title"):
            if not item.get(key):
                raise ValueError(f"{where}: no {key}")
        date = item["date"]
        if isinstance(date, str) and not re.fullmatch(r"\d{4}-\d{2}-\d{2}", date):
            raise ValueError(f"{where}: the date must be YYYY-MM-DD")
        if not isinstance(date, (str, datetime.date)):
            raise ValueError(f"{where}: the date must be a date")
        if item["id"] in seen:
            raise ValueError(f"{where}: another item has this id")
        seen.add(item["id"])
        if item.get("tag", "announcement") not in TAGS:
            raise ValueError(f"{where}: the tag must be one of {', '.join(TAGS)}")
        link = item.get("link", "")
        if link and not link.startswith("https://"):
            raise ValueError(f"{where}: the link must start with https://")
    return items


def split(text: str) -> tuple[str, list[str]]:
    """The text before the first item, and each item's text."""
    parts = re.split(r"(?m)^(?=\[\[news\]\]\s*$)", text)
    return parts[0], [p.rstrip() + "\n" for p in parts[1:]]


def item_id(block: str) -> str:
    return tomllib.loads(block)["news"][0]["id"]


def string(s: str) -> str:
    return json.dumps(s, ensure_ascii=False)  # a JSON string is a TOML basic string


def multiline(s: str) -> str:
    s = s.strip("\n")
    if "'''" not in s:
        return "'''\n" + s + "\n'''"
    return '"""\n' + s.replace("\\", "\\\\").replace('"""', '\\"""') + '\n"""'


def block(args) -> str:
    lines = ["[[news]]", f"id = {string(args.id)}", f"date = {args.date}", f"title = {string(args.title)}",
             f"tag = {string(args.tag)}"]
    if args.pin:
        lines.append("pinned = true")
    if args.link:
        lines.append(f"link = {string(args.link)}")
    if args.body:
        lines.append(f"body = {multiline(args.body)}")
    return "\n".join(lines) + "\n"


def join(header: str, blocks: list[str]) -> str:
    return header.rstrip() + "\n\n" + "\n".join(blocks)


def set_pinned(blk: str, pinned: bool) -> str:
    lines = [l for l in blk.splitlines() if not re.match(r"pinned\s*=", l)]
    if pinned:
        # After the tag, or else the title, as `add` writes it.
        after = next((i for i, l in enumerate(lines) if re.match(r"tag\s*=", l)),
                     next(i for i, l in enumerate(lines) if re.match(r"title\s*=", l)))
        lines.insert(after + 1, "pinned = true")
    return "\n".join(lines) + "\n"


def slug(title: str, date: str) -> str:
    words, out = re.sub(r"[^a-z0-9]+", " ", title.lower()).split(), []
    for w in words:  # whole words, up to about 40 letters
        if out and len("-".join(out + [w])) > 40:
            break
        out.append(w)
    return "-".join([date] + out)


def gh(*args: str, input: str | None = None) -> str:
    try:
        r = subprocess.run(["gh", *args], input=input, capture_output=True, text=True, encoding="utf-8")
    except FileNotFoundError:
        sys.exit("needs the GitHub CLI (gh), signed in as the gist's owner")
    if r.returncode:
        sys.exit(f"gh {' '.join(args[:2])}: {r.stderr.strip()}")
    return r.stdout


def read(args) -> str:
    if args.file:
        path = Path(args.file)
        return path.read_text(encoding="utf-8") if path.exists() else HEADER
    gist = args.gist or GIST
    if not gist:
        sys.exit("no gist: pass --gist ID (or --file for a local copy)")
    return gh("gist", "view", gist, "--filename", FILE, "--raw")


def write(args, text: str) -> None:
    try:
        check(text)
    except ValueError as e:
        sys.exit(f"not saved: {e}")
    if args.dry_run:
        print(text)
        return
    if args.file:
        Path(args.file).write_text(text, encoding="utf-8", newline="\n")
        return
    tmp = Path(__file__).with_name(".news.toml.tmp")
    tmp.write_text(text, encoding="utf-8", newline="\n")
    try:
        gh("gist", "edit", args.gist or GIST, "--filename", FILE, str(tmp))
    finally:
        tmp.unlink(missing_ok=True)
    print("published")


def main(argv: list[str]) -> int:
    def common(default):  # the options, before or after the command
        c = argparse.ArgumentParser(add_help=False)
        c.add_argument("--gist", default=default, help="the gist's ID (default: the news gist)")
        c.add_argument("--file", default=default, help="work on this local news.toml instead of the gist")
        c.add_argument("--dry-run", action="store_true", default=default or False,
                       help="print the new feed instead of saving it")
        return c
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0], parents=[common(None)])
    sub = p.add_subparsers(dest="command", required=True)
    after = common(argparse.SUPPRESS)
    sub.add_parser("list", parents=[after])
    a = sub.add_parser("add", parents=[after])
    a.add_argument("--title", required=True)
    a.add_argument("--tag", choices=TAGS, default="announcement")
    a.add_argument("--link", default="")
    a.add_argument("--pin", action="store_true")
    a.add_argument("--body", default="")
    a.add_argument("--body-file")
    a.add_argument("--date", default=datetime.date.today().isoformat())
    a.add_argument("--id")
    for name in ("remove", "pin", "unpin"):
        sub.add_parser(name, parents=[after]).add_argument("id")
    sub.add_parser("check").add_argument("path")
    args = p.parse_args(argv)

    if args.command == "check":
        try:
            items = check(Path(args.path).read_text(encoding="utf-8"))
        except ValueError as e:
            sys.exit(f"{args.path}: {e}")
        print(f"{args.path}: {len(items)} items, all fine")
        return 0

    text = read(args)
    header, blocks = split(text)
    if args.command == "list":
        # As the launcher shows them: pinned first, then newest first.
        items = sorted(check(text), key=lambda i: (bool(i.get("pinned")), str(i["date"])), reverse=True)
        for item in items:
            flags = " (pinned)" if item.get("pinned") else ""
            print(f"{item['date']}  {item['id']}{flags}\n            {item['title']}")
        return 0
    if args.command == "add":
        if args.body_file:
            args.body = Path(args.body_file).read_text(encoding="utf-8")
        args.id = args.id or slug(args.title, args.date)
        if any(item_id(b) == args.id for b in blocks):
            sys.exit(f"there is already an item {args.id}: pass another --id")
        write(args, join(header, [block(args)] + blocks))
        return 0
    found = [i for i, b in enumerate(blocks) if item_id(b) == args.id]
    if not found:
        sys.exit(f"no item {args.id}")
    if args.command == "remove":
        del blocks[found[0]]
    else:
        blocks[found[0]] = set_pinned(blocks[found[0]], args.command == "pin")
    write(args, join(header, blocks))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
