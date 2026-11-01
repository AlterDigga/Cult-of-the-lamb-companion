# Final review record

Reviewer verdict: ready to merge with fixes. All Critical and Important findings were fixed in ab57640.
Some older decisions below were later replaced by device feedback (for example hearts, icons and the map).

## Deferred minors

- dungeon tab tap does nothing after a run if Dungeon was the remembered tab
- page can stay on Waiting if the second screen first appears after the save loaded
- problems list counts followers on mission or in prison
- no hint when a problem filter yields zero rows
- DecoType 4 metadata naming; recruit plural and remove-structure quest text unverified
- silent permanent states after a font, EnsureData or I2 load failure
- Instance leak if std::thread constructor throws in Create
- misleading vis in quest step templates

## Rulings, in the order they were made

- Setup: Ruling: .gitignore ignored all of tools/ — narrowed to tools/hactool/ and tools/il2cppdumper/ so plan's own scripts are tracked — cost if wrong: none.
- Setup: Ruling: added .gitattributes eol=lf — shell scripts run in WSL — cost if wrong: none.
- Pre-flight: T4→T6/T7/T8 GameSnapshot fields offered_quest_follower_ids/items/run declared in T4, filled later; run changes from pointer to std::optional<Run> in T8 — Ruling: declare `std::optional<Run> run` already in T4 with a forward-declared-then-defined Run in snapshot.h — avoids a type change mid-plan — cost if wrong: none.
- Task 1: Ruling: Eden Duo v1.3.2.2 docs require Ubuntu 26.04+ (GCC/SDL3 too old in 24.04) — build in a new WSL distro Ubuntu-26.04 instead of the existing 24.04; Qt packages omitted (only eden-cli, ENABLE_QT=OFF); mesa-vulkan-drivers added for rendering in WSL — cost if wrong: one extra distro install (~2 GB).
- Task 1: Ruling: GCC 15 fails Eden Duo v1.3.2.2 on -Werror=extra (vk_dsmod_aux.cpp:135) — configure with official option -DENABLE_WERROR=OFF — cost if wrong: none (warnings only).
- Task 1: Ruling: no GPU Vulkan in WSL (no Dozen in Ubuntu mesa), lavapipe crashes, llvmpipe GL hangs — headless runs use --null-render; screen1 (game image) unavailable, so step 7 "screen1 shows base" replaced by memory proof (DataManager instance appears after loading save) and later checks via save/console/Thor — cost if wrong: in-game UI comparisons move to the Thor (user), slower validation.
- Task 1: Ruling: headless.sh waits fixed BOOT_WAIT (default 40 s) instead of "DSMod snapshot" (never logged without a package) — cost if wrong: none.
- Task 2: Ruling: step 6 (user installs on Thor) requested asynchronously; execution continues with Task 3 — waiting would stall the plan; Thor feedback is folded into the next user touchpoint — cost if wrong: a device-only problem surfaces a task later.
- Task 4: Ruling: follower details shown in a fixed panel at the bottom (tap row → panel, tap panel → close) instead of expanding the row in place — runtime has no variable-height rows; module publishes fol.sel_* for the tapped index (added PublishSelectedFollower + 2 tests) — cost if wrong: layout tweak only.
- Task 4: Ruling: step 9 (compare with in-game follower menu) done against memory/console only; in-game comparison folded into Thor check — no game image in WSL — cost if wrong: level/need mismatch found later on device.
- Thor feedback 1: Ruling: illness bar shown as "Health" (inverted, like the game HUD); time of day (phase + day progress) pulled forward from Task 10; life expectancy removed from details (user request) — cost if wrong: none.
- Thor feedback 1: Ruling: follower "Faith" bar replaced by Loyalty (Adoration 0–100) at user request; fol.faith no longer published — cost if wrong: none.
- Task 5: Ruling: I2 parser follows the measured layout (fields [NonSerialized] in the game are absent in the data although dump.cs lists them); verified against real resources.assets (11245 terms, English column 3) — cost if wrong: none, test i2_real_table_english_texts guards it.
- Task 5: Ruling: I2 table loads on a background thread started in create(); until ready, Terms(nullptr) shows identifiers — 12 MB parse would block a tick — cost if wrong: identifiers visible for the first second.
- Task 5: Ruling: lz4 v1.10.0 source cloned to ~/cotl-deps/lz4 (Eden uses system lz4, CPM cache has none); setup_wsl.sh updated — cost if wrong: none.
- Task 6: Ruling: ProblemKind order Starving, Ill, Injured, Hungry, Cursed, Dissent, Exhausted, QuestAccepted, QuestOffered, Old — spec puts life-threatening first (hunger included) and age last; plan had Hungry last — cost if wrong: ordering tweak only.
- Task 6: Ruling: "+N more" written without parentheses (built-in font lacks them) — cost if wrong: none.
- Task 6: Ruling: step 7 verified only for Starving/Exhausted/accepted quest with the user's save; Ill/Dissent/Old/offered quest need further saves or Thor check — cost if wrong: a state mapping bug surfaces later.
- Thor feedback 2: Ruling: quests moved out of Problems into a new Quests tab (6 tabs); ProblemKind renumbered (Old = 7) — user asked for separate tabs — cost if wrong: none.
- Thor feedback 2: Ruling: quest texts rebuilt for 8 of 36 objective types (incl. Custom, the most common); others show a readable type name — full coverage would need 36 RE passes — cost if wrong: some quests show a generic label.
- Thor feedback 2: Ruling: "Inventory" tab label shortened to "Items" so six tabs fit at the larger font — cost if wrong: label wording only.
- Thor feedback 3: Ruling: quests grouped by UniqueGroupID across Objectives + CompletedObjectives like the game's quest menu; groups with no open step hidden — cost if wrong: a finished quest the game still lists would be missing.
- Thor feedback 3: Ruling: list tabs (Problems, Quests, Followers) one font step larger (names 6, body 5, small 4); Cult tab unchanged — user asked for slightly larger, not Cult size — cost if wrong: one more size tweak.
- Task 7: Ruling: quest marker drawn as a blue "!" badge instead of the game's own quest sprite — locating the quest icon sprite would need another RE pass — cost if wrong: different look than in-game.
- Task 7: Ruling: 13 of 203 icons (sprites without atlas, own texture in resources.assets without TypeTree) stay empty instead of a generated placeholder; name and count are always shown — raw Texture2D parsing not worth it for 6 % — cost if wrong: a few blank icons.
- Task 7: Ruling: added MakeTiledAstcDecoder (host 2048 px and ce_unity 16 MiB limits) and atlas→bundle table generated by tools/gen_atlas_bundles.sh — cost if wrong: none, verified headless (all 16 inventory icons render).
- Thor-Feedback 4: Items in Menü-Gruppen Currency/Food/Items (Reihenfolge aus InventoryMenu..ctor), Beschreibungsfenster, gelber Streifen für angebotene Quests; min_runtime 22 (content_h_bind). Ruling: Gegenstände, die in keiner Menü-Liste stehen, werden wie im Spiel nicht angezeigt — weicht vom Spiel ab, falls dieses sie anderswo zeigt. Paket 0.9.0.
- Task 8: Ruling: run signal = PlayerFarming.Instance set and PlayerFarming.Location in GameManager.IsDungeon's set (7–10, 51, 53, 81, 86, 93, 96; None excluded), computed in the module instead of the points file — cost if wrong: a special location (e.g. a challenge) does not switch.
- Task 8: Ruling: tab memory via flag "tab" + derived g.view + page_binds per tab (no "return" page); outside a run tabs set the flag, during a run they navigate directly, after the run the pre-run tab is restored — runtime has no current-page value and @sel resets on page change — cost if wrong: none, verified headless.
- Task 8: Ruling: hearts read from Health base fields (_HP 0xAC/0xB0 …), not HealthPlayer.PLAYER_* caches; shown as one pip per half heart (integers) — cost if wrong: wrong counts, visible on Thor.
- Task 8: Ruling: weapon/curse/relic/tarot shown with name and level but without icons (sprite mapping for tarot/equipment needs its own RE pass); tarot name gets "+" per upgrade like TarotCards.LocalisedName — cost if wrong: less game-like look.
- Task 8: Ruling: Step 5 verified headless by poking PlayerFarming.Location (base → Dungeon1_1 → base) instead of a real run — no game image in WSL to walk into the portal; real run values (weapon, curse, tarot, hearts) still need the Thor — cost if wrong: an offset mismatch shows up on device.
- Task 9: Ruling: Room carries neighbour coordinates (from RoomConnection.Room) instead of n/e/s/w flags — links do not depend on which direction means +y — cost if wrong: none.
- Task 9: Ruling: north drawn up (larger y on top, Unity convention); not verified against the in-game map — cost if wrong: map mirrored vertically, one-line fix after Thor check.
- Task 9: Ruling: map drawn with fixed-size widget templates for 5 grid pitches (96/72/56/40/28 px), module picks the largest that fits 1160×380 — runtime has no bound widget size — cost if wrong: very large floors get small rooms.
- Task 9: Ruling: undiscovered rooms are not drawn at all (game shows them as "?"), per user wish "nur entdeckte Räume" — cost if wrong: none.
- Task 9: Ruling: Step 3/5 (compare with the game map on two floors incl. Woolhaven) not possible headless; layout verified with a temporary sample floor in a dev build (reverted, not committed) — cost if wrong: offset or orientation issue shows on Thor.
- Task 10: Ruling: "storage fill" = seed and fertiliser silos (the only storage with a code-defined capacity found: Structures_Silo*.get_Capacity 15/250); FoodStorage capacity is a constant 10 for reservations, not a player-visible store — cost if wrong: user expected a different storage.
- Task 10: Ruling: alive/ill/old counts reuse fol.count and prob.n1/prob.n7 instead of new module values — same numbers as the Followers/Problems tabs — cost if wrong: none.
- Task 10: Ruling: empty CultName (cult not yet named via follower quest) shown as "Unnamed cult" — cost if wrong: wording only.
- Task 10: Ruling: dirt (poop/vomit) left out, noted as open in re-notes — optional per spec — cost if wrong: one missing indicator.
- Thor 5: Ruling: Spielschrift statt eingebauter Blockschrift (Umlaute/Apostroph), 20 % größer gerendert — sonst wirken Kleinbuchstaben kleiner — Kosten falls falsch: Größe/Zeilenabstände nachjustieren.
- Thor 5: Ruling: Raumform 8:5 mit halber Höhe Abstand — Prefab-Werte nur zur Laufzeit lesbar — Kosten falls falsch: eine Konstante anpassen.
- Thor 5: Ruling: Geisterherzen = rotes Herz mit Geist-Umriss, leer = Umriss — HUD-Logik nicht nachgelesen — Kosten falls falsch: andere Region wählen.
- Thor 5: Ruling: Dreck-Kachel = Kot + Erbrochenes, Leichen eigene Kachel — Kosten falls falsch: Aufteilung ändern.
- Final: Ruling: font build stays on the first-draw path — game is still booting then; moving it risks atlas ordering — cost if wrong: a few hundred ms stall at boot
- Final: Ruling: followers selected row window shows blank if follower disappears — rare, tap closes — cost if wrong: one blank window
