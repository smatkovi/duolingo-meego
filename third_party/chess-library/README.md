# Disservin/chess-library (vendored)

Unmodified copy of the single-header C++ chess library used as the board model
(`platform.md` §4.2, `docs/design.md` §2). Do not edit `chess.hpp`; to update,
replace it with a newer upstream file and adjust the version block below.

| | |
|---|---|
| Upstream | <https://github.com/Disservin/chess-library> |
| Version | **0.9.4** (the `VERSION:` line in the file header) |
| Commit | `53e6a841dcda7059a2af363d85f785ef1817304a` (2026-07-26) |
| Source path | `include/chess.hpp` |
| Fetched | 2026-09-12 |
| Licence | MIT — see `LICENSE` next to this file |

Fetched with:

```sh
curl -L -o chess.hpp \
  https://raw.githubusercontent.com/Disservin/chess-library/53e6a841dcda7059a2af363d85f785ef1817304a/include/chess.hpp
curl -L -o LICENSE \
  https://raw.githubusercontent.com/Disservin/chess-library/53e6a841dcda7059a2af363d85f785ef1817304a/LICENSE
```

The library is MIT, the application is GPL-3.0-or-later; MIT code may be
combined into a GPL work, so the header is included as-is and its licence text
ships with the package (`platform.md` §1.7).

Include it with `-isystem third_party/chess-library` so that its warnings do
not drown out our own `-Wall -Wextra` output.
