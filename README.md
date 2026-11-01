# Cult of the Lamb dual-screen companion

A touch companion for the second screen of [Eden Duo](https://github.com/igawa6/eden-duo), made for
dual-screen Android handhelds such as the AYN Thor. It shows your cult and your current run live
while you play Cult of the Lamb on the main screen.

It only reads the game. It does not change your game or your save, and it gives no spoilers: the
dungeon map shows only what you have discovered.

## Tabs

| Tab | What it shows |
|---|---|
| **Cult** | Cult name and level, faith, hunger and other cult status |
| **Problems** | Followers who need attention, with a filter; tap a follower to see all their problems |
| **Quests** | Open quests with their exact text, steps, givers and deadlines |
| **Followers** | Every follower with details, traits and current thoughts |
| **Items** | Your inventory with the game's icons; tap an item for its description |
| **Dungeon** | Hearts as on the HUD, a map of discovered rooms and exits, and your tarot cards. Tap your weapon, curse or relic to see its stats and description, or a tarot card to see its effect |

## Requirements

- Eden Duo with runtime 22 or newer (tested with 1.3.2.2)
- Cult of the Lamb for Switch, title ID `01002E7016C46000`, version **1.5.27 or 1.5.28**. Both have
  the same build ID. Other versions will not load the companion.
- Your own legally dumped game. The companion contains no game files: all art, fonts and text are
  read at runtime from your copy.

## Install

1. Download `01002E7016C46000.dsmod.zip` from the [Releases](../../releases) page.
2. In Eden Duo, open the game's **Add-ons** menu, choose **Install**, then **Dual screen mods**.
3. Start the game. The second screen says "The Red Crown awaits" until a save is loaded.

## Known limitations

- English text only, and only player 1 in co-op.
- Weapon and curse damage follow the game's formula, but co-op tarot cards are not counted.
- Souls and soul fragments have no icon.
- [docs/final-review.md](docs/final-review.md) lists smaller known issues.

## Building from source

Build on Linux or WSL. Run `tools/setup_wsl.sh`, or provide checkouts of
[eden-duo](https://github.com/igawa6/eden-duo) and
[eden-duo-companions](https://github.com/igawa6/eden-duo-companions) under `~/cotl-deps`, plus
Android NDK r28c. Then run:

```bash
bash tools/build.sh --version 0.21.1
```

This runs the tests and writes the package to `out/`. Generated tables are in `native/*_1_5_27.inc`,
and the reverse-engineering notes are in [docs/](docs/) (in German). Never commit game files, keys
or saves; `.gitignore` excludes `game/` and `work/`.

## License

GNU General Public License v3.0, see [LICENSE](LICENSE). The module builds on the Unity reader from
eden-duo-companions, which is under the same licence.

This is a fan project, not affiliated with or endorsed by Massive Monster, Devolver Digital,
Nintendo, or the Eden projects. Cult of the Lamb and all related names and assets belong to their
owners.
