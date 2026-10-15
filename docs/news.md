# The news

The launcher's News panel shows announcements from the OpenGP team. They are written by hand in one
file, `news.toml`, in a public GitHub gist; nothing is copied from anywhere else (post on Discord
yourself). The launcher downloads the file when it starts and every half hour while it is open, and
keeps the last copy in `user/news.toml` for when it is offline. Players can turn the news off
(Settings, Launcher).

The gist: _not created yet_. A build points at it with the CMake variable `LAUNCHER_NEWS_URL`, its
raw address without a revision (always the newest), for example
`https://gist.githubusercontent.com/<owner>/<id>/raw/news.toml`.

## The file

```toml
# Newest first is not needed: the launcher sorts by date (pinned items on top).

[[news]]
id = "2026-10-16-the-new-launcher"   # unique, and never reused: "unread" goes by it
date = 2026-10-16
title = "The new launcher"
tag = "update"                       # update, announcement (the default) or event
pinned = false                       # optional: keep it on top
link = "https://discord.gg/..."      # optional: a "Read more" link, https only
body = '''
Markdown: **bold**, *italic*, [links](https://...), lists.
'''
```

An item without an `id`, `date` or `title`, with another item's `id`, or with a link that is not
`https://` is left out (the launcher reports how many in its log). If the whole file is not TOML,
the launcher keeps showing the copy it had.

## Adding an item

Edit the gist on github.com (the pencil button), or use `tools/news.py`, which checks the file the
way the launcher reads it before publishing (it needs the GitHub CLI, `gh`, signed in as the gist's
owner):

```sh
python tools/news.py list
python tools/news.py add --title "A fix for linked races" --tag update --body-file post.md --link https://...
python tools/news.py pin 2026-10-16-a-fix-for-linked-races
python tools/news.py remove 2026-10-16-a-fix-for-linked-races
python tools/news.py check news.toml          # a local file
```

`--dry-run` prints the new file instead of publishing it; `--file news.toml` works on a local copy.
The id is made from the date and the title unless `--id` gives one.

## Trying a feed out

Point a launcher at a local file in `user/launcher.toml`:

```toml
[news]
url = "file:///C:/path/to/news.toml"
```
