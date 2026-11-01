#!/usr/bin/env python3
"""Erzeugt package/CultOfTheLamb/dualscreen/manifest.json.

Das Manifest wird nie von Hand bearbeitet. Format: eden-duo-companions/docs/PACKAGE_FORMAT.md.
Schrift: die Spielschrift (Lapture) über das Modul; Versalhöhe ≈ 6 px je text_scale-Stufe.
"""
import json
import os
import sys

W, H = 1240, 1080
TAB_H = 100

# Schriftstufen (text_scale): Cult-Tab, Tab-Leiste, Fenstertitel
S_TITLE = 6
S_NAME = 5
S_BODY = 4
S_SMALL = 3
# Listen-Tabs (Problems, Quests, Followers): eine Stufe größer als zuvor (Thor-Test 3)
L_NAME = 6
L_BODY = 5
L_SMALL = 4

# Farben (0xAARRGGBB als Text)
BG = "#FF161214"
PANEL = "#FF231C1F"
POPUP = "#FF2E2328"
# Etwas hellere Ränder der Flächen
BORDER = "#FF54464D"
BORDER_POPUP = "#FF6E5C65"
BORDER_PX = 3
BAR_BG = "#FF3A3034"
TEXT = "#FFEDE3D6"
MUTED = "#FF9A8C84"
ACCENT = "#FFC8283C"
QUEST = "#FF4F8FD6"
OFFER = "#FFF0C43C"  # angebotene Quests, Quest-Symbol im Inventar
FAITH = "#FFE8C547"
HUNGER = "#FFD9873A"
ILLNESS = "#FF7FB24A"
HAPPY = "#FF6FA8DC"
DIRT = "#FFB07A45"

TABS = [
    ("cult", "Cult"),
    ("problems", "Problems"),
    ("quests", "Quests"),
    ("followers", "Followers"),
    ("inventory", "Items"),
    ("dungeon", "Dungeon"),
]

# Problemarten in der Reihenfolge von ProblemKind (native/problems.h): Name, Farbe
PROBLEM_KINDS = [
    ("Starving", "#FFC8283C"), ("Ill", "#FFC8283C"), ("Injured", "#FFC8283C"),
    ("Hungry", "#FFD9873A"), ("Cursed", "#FFC8283C"), ("Dissent", "#FFD9873A"),
    ("Exhausted", "#FFD9873A"), ("Old", "#FF6E625C"),
]

# Tagesphasen (DayPhase) mit je 240 Spielminuten: Name, Farbe
PHASES = [
    ("Dawn", "#FFE7A6A1"), ("Morning", "#FFF0D24A"), ("Afternoon", "#FFE8913A"),
    ("Dusk", "#FF8E4FA8"), ("Night", "#FF2B3A78"),
]

DUNGEON_TAB = [pid for pid, _ in TABS].index("dungeon")

ROW_H = 156
MAX_ROWS = 150  # Obergrenze der Listen-Templates


def tab_bar(active):
    """Tab-Leiste oben; die aktive Seite ist hervorgehoben."""
    widgets = [{"type": "rect", "rect": [0, 0, W, TAB_H], "bg": PANEL, "_plain": True},
               {"type": "rect", "rect": [0, TAB_H - BORDER_PX, W, BORDER_PX], "bg": BORDER}]
    tab_w = W // len(TABS)
    for i, (page_id, label) in enumerate(TABS):
        on = page_id == active
        # Außerhalb eines Runs setzt der Tab das Flag "tab"; die Seite folgt per page_binds
        # (g.view), so weiß das Paket nach dem Run, wohin es zurück muss. Im Run wechselt der Tab
        # direkt, ohne den gemerkten Tab zu ändern.
        for need, action in (("!g.in_run", f"tab.{i}"), ("g.in_run", f"go.{page_id}")):
            widgets.append({
                "type": "button", "id": f"tab.{page_id}",
                "rect": [i * tab_w + 4, 8, tab_w - 8, TAB_H - 16],
                "text": label, "align": "center", "text_scale": S_BODY,
                "color": TEXT if on else MUTED, "bg": ACCENT if on else PANEL, "border": 0,
                "on_tap": action, "need_bind": need,
            })
        # Zähler oben rechts am Tab, auf jeder Seite sichtbar (auch im Dungeon)
        count = {"problems": "prob.total", "quests": "quest.count"}.get(page_id)
        if count:
            widgets.append({"type": "value", "rect": [i * tab_w + tab_w - 14, 12, 0, 0],
                            "bind": count, "align": "right", "text_scale": S_SMALL,
                            "color": TEXT if on else "#FFF08A5D",
                            "hide_bind": count, "hide_eq": 0})
    return widgets


def page(page_id, widgets, scrolls=None):
    # Rechtecke ohne Umrandung: der Standardrahmen (hell) flimmert auf dem Thor am Rand.
    # DrawRect zeichnet einen Rahmen von max(1, frame) px, solange color != 0 – daher color 0.
    # Sichtbare Ränder daher als zwei gefüllte Flächen: heller Rand, darüber die Fläche.
    out = []
    for wd in widgets:
        plain = wd.pop("_plain", False)
        outline = wd.pop("_outline", False)
        if wd.get("type") == "rect":
            wd.setdefault("frame", 0)
            wd.setdefault("color", "#00000000")
            border = {PANEL: BORDER, POPUP: BORDER_POPUP}.get(wd.get("bg"))
            if border and not plain:
                x, y, rw, rh = wd["rect"]
                inner = {k: v for k, v in wd.items()
                         if k not in ("on_tap", "payload", "select_group", "input_block")}
                inner["rect"] = [x + BORDER_PX, y + BORDER_PX, rw - 2 * BORDER_PX,
                                 rh - 2 * BORDER_PX]
                wd["bg"] = border
                out += [wd, inner]
                continue
        if outline:
            # Rand um einen Balken: Fläche dahinter mit denselben Bedingungen und Wiederholungen
            x, y, rw, rh = wd["rect"]
            back = {k: v for k, v in wd.items()
                    if k in ("need_bind", "hide_bind", "hide_eq", "repeat", "repeat_bind",
                             "repeat_dy", "scroll")}
            out.append({"type": "rect", "rect": [x - 2, y - 2, rw + 4, rh + 4], "bg": BORDER,
                        "frame": 0, "color": "#00000000", **back})
        out.append(wd)
    widgets = out
    for wd in widgets:
        # Zentrierte Labels/Werte nehmen x als Mittelpunkt: hier geschrieben als Feld [x, y, w, h]
        # und auf die Mitte des Felds umgerechnet
        if wd.get("type") in ("label", "value") and wd.get("align") == "center" and \
                not wd.get("_centered"):
            x, y, rw, rh = wd["rect"]
            wd["rect"] = [x + rw // 2, y, rw, rh]
            wd["_centered"] = True
    p = {"id": page_id, "widgets": widgets}
    if scrolls:
        p["scrolls"] = scrolls
    return p


def background(page_id):
    return [{"type": "rect", "rect": [0, 0, W, H], "bg": BG}] + tab_bar(page_id)


def list_row(top, count_bind, scroll_id, row_h=ROW_H):
    """Template-Helfer: Widget wird pro Eintrag in der Scroll-Region wiederholt."""
    def row(widget):
        widget = dict(widget)
        x, y, rw, rh = widget["rect"]
        widget["rect"] = [x, top + y, rw, rh]
        widget.update({"repeat": MAX_ROWS, "repeat_bind": count_bind, "repeat_dy": row_h,
                       "scroll": scroll_id})
        return widget
    return row


def scroll(scroll_id, top, count_bind, row_h=ROW_H, bottom=H):
    return {"id": scroll_id, "rect": [0, top, W, bottom - top], "count_bind": count_bind,
            "row_h": row_h, "bar": "#80EDE3D6", "bar_track": "#22000000", "bar_w": 8}


def empty_label(text, need):
    return {"type": "label", "rect": [0, 540, W, 40], "text": text, "align": "center",
            "text_scale": L_BODY, "color": MUTED, "need_bind": need}


def popup(flag, close_action, top=TAB_H + 40, bottom=H - 40, need=None):
    """Schwebendes Fenster; sichtbar solange @flag:<flag> != -1 (need: solange dieser Wert gesetzt
    ist). Fängt alle Tipps ab."""
    vis = {"need_bind": need} if need else {"hide_bind": f"@flag:{flag}", "hide_eq": -1}
    return vis, [
        {"type": "rect", "rect": [0, TAB_H, W, H - TAB_H], "bg": "#B0000000", "input_block": True,
         "on_tap": close_action, **vis},
        {"type": "rect", "rect": [40, top, W - 80, bottom - top], "bg": POPUP,
         "on_tap": close_action, **vis},
        {"type": "label", "rect": [40, bottom - 56, W - 80, 30], "text": "Tap to close",
         "align": "center", "text_scale": S_SMALL, "color": MUTED, **vis},
    ]


# ---- Kult -------------------------------------------------------------------------------------

def labeled_bar(y, label, bind, max_bind, color, pct_bind):
    return [
        {"type": "rect", "rect": [30, y, W - 60, 132], "bg": PANEL},
        {"type": "label", "rect": [54, y + 22, 700, 50], "text": label, "text_scale": S_NAME,
         "color": TEXT},
        {"type": "value", "rect": [W - 54, y + 22, 0, 0], "bind": pct_bind, "suffix": "%",
         "align": "right", "text_scale": S_NAME, "color": TEXT},
        {"type": "bar", "rect": [54, y + 66, W - 108, 50], "bind": bind, "max_bind": max_bind,
         "color": color, "bg": BAR_BG, "frame": 0, "_outline": True},
    ]


def stat_tile(x, y, w_, label, bind, color):
    return [
        {"type": "rect", "rect": [x, y, w_, 110], "bg": PANEL},
        {"type": "label", "rect": [x + 24, y + 20, w_ - 48, 34], "text": label,
         "text_scale": S_SMALL, "color": MUTED},
        {"type": "value", "rect": [x + 24, y + 58, 0, 0], "bind": bind, "text_scale": S_NAME,
         "color": color},
    ]


def storage_tile(x, y, w_, label, key, color):
    """Silo-Füllstand: Name, "n / max" und Balken; nur sichtbar, wenn es ein Silo gibt."""
    gate = {"need_bind": f"cult.has_{key}"}
    return [
        {"type": "rect", "rect": [x, y, w_, 110], "bg": PANEL, **gate},
        {"type": "label", "rect": [x + 24, y + 25, 300, 34], "text": label,
         "text_scale": S_SMALL, "color": MUTED, **gate},
        {"type": "value", "rect": [x + w_ - 24, y + 20, 0, 0], "bind": f"cult.{key}",
         "max_bind": f"cult.{key}_cap", "max_sep": " / ", "align": "right",
         "text_scale": S_BODY, "color": TEXT, **gate},
        {"type": "bar", "rect": [x + 24, y + 64, w_ - 48, 28], "bind": f"cult.{key}",
         "max_bind": f"cult.{key}_cap", "color": color, "bg": BAR_BG, "frame": 0,
         "_outline": True, **gate},
    ]


def cult_page():
    w = background("cult")
    # Kopfzeile: Kultname und Kult-Level
    w += [
        {"type": "rect", "rect": [30, TAB_H + 8, W - 60, 74], "bg": PANEL},
        {"type": "label", "rect": [54, TAB_H + 35, 800, 50], "bind_text": "cult.name",
         "text_scale": S_NAME, "color": TEXT},
        {"type": "value", "rect": [W - 54, TAB_H + 35, 0, 0], "bind": "cult.level",
         "text": "Cult Level ", "align": "right", "text_scale": S_NAME, "color": FAITH},
    ]
    w += labeled_bar(190, "Faith", "cult.faith", "cult.faith_max", FAITH, "cult.faith_pct")
    w += labeled_bar(330, "Hunger", "cult.hunger", "cult.hunger_max", HUNGER, "cult.hunger_pct")
    # Wie im Spiel (IllnessBar.get_IllnessNormalized = 1 - Count/Max): voll = gesund
    w += labeled_bar(470, "Health", "cult.health", "cult.illness_max", ILLNESS, "cult.health_pct")
    # Tag: gemeinsame Fläche; Rand um den Tagesbalken (die Phasen sind halbtransparent, deshalb
    # liegt zwischen Rand und Phasen noch einmal die Flächenfarbe)
    w += [
        {"type": "rect", "rect": [30, 610, W - 60, 168], "bg": PANEL},
        {"type": "rect", "rect": [52, 674, W - 104, 48], "bg": BORDER},
        {"type": "rect", "rect": [54, 676, W - 108, 44], "bg": PANEL, "_plain": True},
        {"type": "value", "rect": [54, 632, 0, 0], "bind": "g.day", "text": "Day ",
         "text_scale": S_NAME, "color": TEXT},
        {"type": "label", "rect": [W - 54, 632, 0, 40], "text_bind": "g.phase", "align": "right",
         "text_map": {str(k): name for k, (name, _) in enumerate(PHASES)},
         "text_scale": S_NAME, "color": TEXT},
    ]
    # Tagesbalken: fünf Phasen à 240 Spielminuten (TimeManager.GAME_MINUTES_PER_PHASE),
    # vergangener Teil voll gefärbt, Rest gedämpft; weiße Marke bei der aktuellen Zeit
    bar_x, bar_y, bar_w, bar_h = 54, 676, W - 108, 44
    seg_w = bar_w / 5
    for k, (name, color) in enumerate(PHASES):
        x = round(bar_x + k * seg_w)
        sw = round(bar_x + (k + 1) * seg_w) - x
        dim = "#55" + color[3:]
        w.append({"type": "bar", "rect": [x, bar_y, sw, bar_h], "bind": f"g.seg{k}", "max": 240,
                  "color": color, "bg": dim, "frame": 0})
        w.append({"type": "label", "rect": [x, bar_y + bar_h + 10, sw, 30], "text": name,
                  "align": "center", "text_scale": S_SMALL, "color": MUTED})
    w.append({"type": "rect", "rect": [bar_x - 3, bar_y - 8, 6, bar_h + 16], "bg": TEXT,
              "x_bind": "g.time", "x_scale": bar_w / 1200})
    # Follower-Zahlen (krank/alt wie im Problems-Tab gezählt), Dreck (Kot + Erbrochenes) und
    # Leichen in der Basis
    tiles = [("Followers", "fol.count", TEXT), ("Ill", "prob.n1", "#FFC8283C"),
             ("Old age", "prob.n7", MUTED), ("Dirt", "cult.dirt", DIRT),
             ("Bodies", "cult.corpses", "#FFC8283C")]
    tw = (W - 60 - (len(tiles) - 1) * 20) // len(tiles)
    for k, (label, bind, color) in enumerate(tiles):
        w += stat_tile(30 + k * (tw + 20), 790, tw, label, bind, color)
    # Silo-Füllstände
    sw2 = (W - 60 - 20) // 2
    w += storage_tile(30, 916, sw2, "Seed silos", "seed", ILLNESS)
    w += storage_tile(30 + sw2 + 20, 916, sw2, "Fertiliser silos", "fert", HUNGER)
    return page("cult", w)


# ---- Probleme ---------------------------------------------------------------------------------

def problems_page():
    w = background("problems")
    # Zähler je Problemart (zwei Reihen à 4), nur sichtbar bei Anzahl > 0; Antippen filtert
    bw, bh, gap = 296, 64, 8
    for k, (label, color) in enumerate(PROBLEM_KINDS):
        # Lage aus prob.cx/cy: nur vorhandene Arten belegen Plätze, ohne Lücken
        x, y = 20, TAB_H + 14
        hide = {"hide_bind": f"prob.n{k}", "hide_eq": 0, "x_bind": f"prob.cx{k}",
                "y_bind": f"prob.cy{k}"}
        w.append({"type": "rect", "rect": [x, y, bw, bh], "bg": color,
                  "on_tap": f"prob.f{k}", **hide})
        w.append({"type": "label", "rect": [x + 24, y + 22, bw - 90, 34], "text": label,
                  "text_scale": L_SMALL, "color": TEXT, **hide})
        w.append({"type": "value", "rect": [x + bw - 24, y + 22, 0, 0], "bind": f"prob.n{k}",
                  "align": "right", "text_scale": L_SMALL, "color": TEXT, **hide})
        # Aktiver Filter: weißer Rahmen; erneutes Antippen hebt den Filter auf
        only_k = {"hide_bind": "@flag:prob.filter", "keep_min": k, "keep_max": k,
                  "x_bind": f"prob.cx{k}", "y_bind": f"prob.cy{k}"}
        w.append({"type": "rect", "rect": [x - 4, y - 4, bw + 8, bh + 8], "bg": "#00000000",
                  "color": TEXT, "frame": 4, "on_tap": "prob.fclear", **only_k})
    top = TAB_H + 14 + 2 * (bh + gap) + 6
    w.append(empty_label("Everyone is fine", "!prob.total"))
    row = list_row(top, "prob.count", "prob.list")
    w += [
        # Antippen zeigt alle Probleme des Followers
        row({"type": "rect", "rect": [20, 6, W - 40, ROW_H - 12], "bg": PANEL,
             "on_tap": "prob.select", "payload": "{i}", "select_group": "prob"}),
        row({"type": "label", "rect": [44, 30, 600, 48], "bind_text": "prob.name{i}",
             "text_scale": L_NAME, "color": TEXT}),
        row({"type": "value", "rect": [44, 98, 0, 0], "bind": "prob.level{i}", "text": "Lv ",
             "text_scale": L_BODY, "color": MUTED}),
        row({"type": "label", "rect": [200, 98, 600, 36], "bind_text": "prob.reason{i}",
             "text_scale": L_BODY, "color": TEXT}),
    ]
    # Farbiges Badge der gezeigten Art: ein Template je Art, sichtbar nur bei passender Art
    for k, (label, color) in enumerate(PROBLEM_KINDS):
        only_k = {"hide_bind": "prob.kind{i}", "keep_min": k, "keep_max": k}
        w.append(row({"type": "rect", "rect": [W - 340, 42, 300, 70], "bg": color, **only_k}))
        w.append(row({"type": "label", "rect": [W - 340, 67, 300, 36], "text": label,
                      "align": "center", "text_scale": L_BODY, "color": TEXT, **only_k}))
    # Fenster: alle Probleme des angetippten Followers, dringendste zuerst
    vis, win = popup("prob.open", "prob.close", top=TAB_H + 40, bottom=H - 30)
    w += win
    w += [
        {"type": "label", "rect": [80, TAB_H + 70, 800, 50], "bind_text": "prob.sel_name",
         "text_scale": L_NAME, "color": TEXT, **vis},
        {"type": "value", "rect": [80, TAB_H + 140, 0, 0], "bind": "prob.sel_level", "text": "Lv ",
         "text_scale": L_BODY, "color": MUTED, **vis},
        {"type": "label", "rect": [0, 560, W, 40], "text": "No problems right now",
         "align": "center", "text_scale": L_BODY, "color": MUTED, "need_bind": "prob.sel_none",
         **vis},
    ]
    stop, srh = TAB_H + 210, 100
    srow = list_row(stop, "prob.sel_count", "prob.sel_list", srh)
    w.append(srow({"type": "rect", "rect": [70, 6, W - 140, srh - 12], "bg": PANEL}))
    for k, (_, color) in enumerate(PROBLEM_KINDS):
        only_k = {"hide_bind": "prob.sel_kind{i}", "keep_min": k, "keep_max": k}
        w.append(srow({"type": "rect", "rect": [70, 6, 14, srh - 12], "bg": color, **only_k}))
    w.append(srow({"type": "label", "rect": [108, 40, W - 220, 40],
                   "bind_text": "prob.sel_reason{i}", "text_scale": L_BODY, "color": TEXT}))
    scrolls = [scroll("prob.list", top, "prob.count"),
               {**scroll("prob.sel_list", stop, "prob.sel_count", srh, bottom=H - 90),
                "show_bind": "prob.sel_open"}]
    return page("problems", w, scrolls)


# ---- Quests -----------------------------------------------------------------------------------

def quests_page():
    w = background("quests")
    top = TAB_H + 10
    rh = 190
    w.append(empty_label("No open quests", "!quest.count"))
    row = list_row(top, "quest.count", "quest.list", rh)
    w += [
        row({"type": "rect", "rect": [20, 6, W - 40, rh - 12], "bg": PANEL,
             "on_tap": "quest.select", "payload": "{i}", "select_group": "quest"}),
        # Angebotene Quests: gelber Streifen links
        row({"type": "rect", "rect": [20, 6, 12, rh - 12], "bg": OFFER,
             "hide_bind": "quest.offered{i}", "hide_eq": 0}),
        row({"type": "label", "rect": [50, 30, 1100, 36], "bind_text": "quest.title{i}",
             "text_scale": L_SMALL, "color": MUTED}),
        row({"type": "label", "rect": [50, 76, 1140, 48], "bind_text": "quest.text{i}",
             "text_scale": L_NAME, "color": TEXT, "wrap_width": 1140, "max_lines": 1}),
        row({"type": "label", "rect": [50, 136, 480, 36], "bind_text": "quest.giver{i}",
             "text_scale": L_BODY, "color": QUEST}),
        row({"type": "label", "rect": [560, 136, 340, 36], "bind_text": "quest.more{i}",
             "text_scale": L_BODY, "color": MUTED}),
        row({"type": "label", "rect": [W - 50, 136, 0, 36], "bind_text": "quest.due{i}",
             "align": "right", "text_scale": L_BODY, "color": HUNGER}),
    ]
    # Fenster mit allen Unterpunkten des angetippten Quests
    vis, win = popup("quest.open", "quest.close", top=TAB_H + 40, bottom=H - 30)
    w += win
    w += [
        {"type": "label", "rect": [80, TAB_H + 70, W - 160, 50], "bind_text": "quest.sel_title",
         "text_scale": L_NAME, "color": TEXT, **vis},
        {"type": "label", "rect": [80, TAB_H + 140, 600, 40], "bind_text": "quest.sel_giver",
         "text_scale": L_BODY, "color": QUEST, **vis},
        {"type": "label", "rect": [W - 80, TAB_H + 140, 0, 40], "bind_text": "quest.sel_due",
         "align": "right", "text_scale": L_BODY, "color": HUNGER, **vis},
    ]
    stop, srh = TAB_H + 210, 120
    srow = list_row(stop, "quest.step_count", "quest.steps", srh)
    open_step = {"hide_bind": "quest.step_done{i}", "hide_eq": 1}
    done_step = {"hide_bind": "quest.step_done{i}", "hide_eq": 0}
    w += [
        srow({"type": "rect", "rect": [70, 6, W - 140, srh - 12], "bg": PANEL, **vis}),
        srow({"type": "label", "rect": [100, 30, W - 200, 80], "bind_text": "quest.step{i}",
              "wrap_width": W - 200, "max_lines": 2, "line_gap": 18, "text_scale": L_BODY,
              "color": TEXT, **vis, **open_step}),
        srow({"type": "label", "rect": [100, 30, W - 320, 80], "bind_text": "quest.step{i}",
              "wrap_width": W - 320, "max_lines": 2, "line_gap": 18, "text_scale": L_BODY,
              "color": MUTED, **vis, **done_step}),
        srow({"type": "label", "rect": [W - 100, 30, 0, 36], "text": "done", "align": "right",
              "text_scale": L_BODY, "color": ILLNESS, **vis, **done_step}),
    ]
    scrolls = [scroll("quest.list", top, "quest.count", rh),
               {**scroll("quest.steps", stop, "quest.step_count", srh, bottom=H - 90),
                "show_bind": "quest.steps_open"}]
    return page("quests", w, scrolls)


# ---- Follower ---------------------------------------------------------------------------------

def followers_page():
    w = background("followers")
    head = TAB_H + 16
    top = TAB_H + 64
    bars = [(560, "Fed", HUNGER, "sat"), (720, "Loyalty", FAITH, "loyal"),
            (880, "Ill", ILLNESS, "ill"), (1040, "Happy", HAPPY, "happy")]
    for x, label, _, _ in bars:
        w.append({"type": "label", "rect": [x, head, 150, 34], "text": label,
                  "text_scale": L_SMALL, "color": MUTED})
    w.append(empty_label("No followers yet", "!fol.count"))
    row = list_row(top, "fol.count", "fol.list")
    w += [
        row({"type": "rect", "rect": [20, 6, W - 40, ROW_H - 12], "bg": PANEL,
             "on_tap": "fol.select", "payload": "{i}", "select_group": "fol"}),
        row({"type": "label", "rect": [44, 30, 500, 48], "bind_text": "fol.name{i}",
             "text_scale": L_NAME, "color": TEXT}),
        row({"type": "value", "rect": [44, 98, 0, 0], "bind": "fol.level{i}", "text": "Lv ",
             "text_scale": L_BODY, "color": MUTED}),
        row({"type": "label", "rect": [200, 98, 340, 36], "bind_text": "fol.role{i}",
             "text_scale": L_BODY, "color": MUTED}),
        row({"type": "label", "rect": [560, 100, 600, 34], "text_bind": "fol.group{i}",
             "text_map": {"1": "On mission", "2": "Imprisoned"}, "text_scale": L_SMALL,
             "color": ACCENT}),
    ]
    for x, _, color, key in bars:
        w.append(row({"type": "bar", "rect": [x, 50, 140, 34], "bind": f"fol.{key}{{i}}",
                      "max": 100, "color": color, "bg": BAR_BG, "frame": 0,
                      "_outline": True}))

    # Detailfeld des angetippten Followers unten; Antippen schließt es
    dvis = {"hide_bind": "@flag:fol.open", "hide_eq": -1}
    dtop = H - 340
    w += [
        {"type": "rect", "rect": [20, dtop, W - 40, 320], "bg": POPUP, "on_tap": "fol.close",
         **dvis},
        {"type": "label", "rect": [50, dtop + 24, 800, 50], "bind_text": "fol.sel_name",
         "text_scale": S_TITLE, "color": TEXT, **dvis},
        {"type": "label", "rect": [50, dtop + 100, W - 100, 200], "bind_text": "fol.sel_detail",
         "wrap_width": W - 100, "max_lines": 4, "line_gap": 18, "text_scale": L_BODY,
         "color": TEXT, **dvis},
        # Knopf für das Thoughts-Fenster
        {"type": "rect", "rect": [W - 330, dtop + 20, 290, 70], "bg": QUEST,
         "on_tap": "fol.thoughts", **dvis},
        {"type": "label", "rect": [W - 330, dtop + 45, 290, 36], "text": "Thoughts",
         "align": "center", "text_scale": L_BODY, "color": TEXT, **dvis},
    ]
    # Thoughts-Fenster: Gedanken des angetippten Followers mit Glaubens-Modifikator
    tvis, win = popup("fol.thoughts", "fol.thoughts_close", top=TAB_H + 40, bottom=H - 30)
    w += win
    w += [
        {"type": "label", "rect": [80, TAB_H + 70, 700, 50], "bind_text": "fol.sel_name",
         "text_scale": S_TITLE, "color": TEXT, **tvis},
        {"type": "label", "rect": [80, TAB_H + 140, 400, 34], "text": "Thoughts",
         "text_scale": L_SMALL, "color": MUTED, **tvis},
        {"type": "label", "rect": [0, 560, W, 40], "text": "No thoughts right now",
         "align": "center", "text_scale": L_BODY, "color": MUTED, "need_bind": "!fol.th_count",
         **tvis},
    ]
    ttop, trh = TAB_H + 190, 76
    trow = list_row(ttop, "fol.th_count", "fol.th_list", trh)
    w += [
        trow({"type": "label", "rect": [80, 18, 900, 40], "bind_text": "fol.th_text{i}",
              "text_scale": L_BODY, "color": TEXT, **tvis}),
        trow({"type": "label", "rect": [W - 80, 18, 0, 40], "bind_text": "fol.th_mod{i}",
              "align": "right", "text_scale": L_BODY, "color": FAITH, **tvis}),
    ]
    scrolls = [scroll("fol.list", top, "fol.count"),
               {**scroll("fol.th_list", ttop, "fol.th_count", trh, bottom=H - 100),
                "show_bind": "fol.th_open"}]
    return page("followers", w, scrolls)


# ---- Inventar ---------------------------------------------------------------------------------

# Abschnitte wie im Spiel-Menü (InventoryMenu, native/inventory_menu_1_5_27.inc)
INV_GROUPS = ["Currency", "Food", "Items"]
# Kleinere Kacheln bei gleicher Schriftgröße (Thor-Test 5): 6 Spalten
INV_COLS, INV_TILE_W, INV_TILE_H, INV_GAP = 6, 188, 222, 10
INV_HEAD = 76  # Höhe der Abschnittsüberschrift


def inv_section_h(g):
    """Höhe von Abschnitt g im Scroll-Inhalt (0 wenn leer), als Ausdruck."""
    n = f"inv.n{g}"
    return f"min({n}, 1) * ({INV_HEAD} + ceil({n} / {INV_COLS}) * {INV_TILE_H + INV_GAP})"


def inventory_page():
    w = background("inventory")
    top = TAB_H + 14
    tile_w, tile_h, gap = INV_TILE_W, INV_TILE_H, INV_GAP
    w.append(empty_label("Inventory is empty", "!inv.count"))

    def placed(widget, g, repeat):
        widget = dict(widget)
        x, y, rw, rh = widget["rect"]
        widget["rect"] = [20 + x, top + y, rw, rh]
        # Auch die Überschrift ist ein Repeat: der Host verschiebt nur Repeat-Vorlagen mit der
        # Scroll-Region
        widget.update({"scroll": "inv.grid", "y_bind": f"inv.off{g}", "repeat": 1})
        if repeat:
            widget.update({"repeat": MAX_ROWS * 2, "repeat_bind": f"inv.n{g}",
                           "repeat_cols": INV_COLS, "repeat_dx": tile_w + gap,
                           "repeat_row_dy": tile_h + gap})
        return widget

    for g, title in enumerate(INV_GROUPS):
        key = f"inv.g{g}."
        def tile(widget, g=g):
            widget = dict(widget)
            widget["rect"] = [widget["rect"][0], INV_HEAD + widget["rect"][1],
                              *widget["rect"][2:]]
            return placed(widget, g, True)
        quest = {"hide_bind": key + "quest{i}", "hide_eq": 0}
        w += [
            placed({"type": "label", "rect": [4, 18, 800, 44], "text": title,
                    "text_scale": L_NAME, "color": FAITH,
                    "hide_bind": f"inv.n{g}", "hide_eq": 0}, g, False),
            tile({"type": "rect", "rect": [0, 0, tile_w, tile_h], "bg": PANEL,
                  "on_tap": "inv.select", "payload": f"{{i+{g * 1000}}}",
                  "select_group": "inv"}),
            # Icon aus den Spieldateien (module:cotl:item/<ITEM_TYPE>, native/icons.cpp)
            tile({"type": "image", "rect": [(tile_w - 90) // 2, 48, 90, 90],
                  "src_bind": key + "icon{i}"}),
            tile({"type": "value", "rect": [16, 19, 0, 0], "bind": key + "qty{i}", "text": "x",
                  "text_scale": L_BODY, "color": TEXT}),
            tile({"type": "label", "rect": [6, 146, tile_w - 12, 68],
                  "bind_text": key + "name{i}", "align": "center", "wrap_width": tile_w - 12,
                  "max_lines": 2, "line_gap": 14, "text_scale": L_SMALL, "color": TEXT}),
            # Quest-Symbol oben rechts: eine offene Sammel-/Abgabe-Quest braucht den Gegenstand
            tile({"type": "rect", "rect": [tile_w - 50, 14, 36, 36], "bg": OFFER, **quest}),
            tile({"type": "label", "rect": [tile_w - 50, 21, 36, 26], "text": "!",
                  "align": "center", "text_scale": L_BODY, "color": BG, **quest}),
        ]

    # Fenster mit Beschreibung des angetippten Gegenstands
    vis, win = popup("inv.open", "inv.close", top=TAB_H + 120, bottom=H - 120)
    ptop = TAB_H + 120
    w += win
    w += [
        {"type": "rect", "rect": [80, ptop + 40, 200, 200], "bg": PANEL, **vis},
        {"type": "image", "rect": [95, ptop + 55, 170, 170], "src_bind": "inv.sel_icon", **vis},
        {"type": "label", "rect": [320, ptop + 60, W - 400, 50], "bind_text": "inv.sel_name",
         "text_scale": S_TITLE, "color": TEXT, **vis},
        {"type": "value", "rect": [320, ptop + 150, 0, 0], "bind": "inv.sel_qty",
         "text": "Owned: ", "text_scale": L_BODY, "color": MUTED, **vis},
        {"type": "label", "rect": [80, ptop + 290, W - 160, 300], "bind_text": "inv.sel_desc", "color_markup": True,
         "wrap_width": W - 160, "max_lines": 6, "line_gap": 18, "text_scale": L_BODY,
         "color": TEXT, **vis},
    ]
    scrolls = [{"id": "inv.grid", "rect": [0, top, W, H - top],
                "content_h_bind": "inv.content_h", "bar": "#80EDE3D6",
                "bar_track": "#22000000", "bar_w": 8}]
    return page("inventory", w, scrolls)


# ---- Dungeon ----------------------------------------------------------------------------------

# Herzbilder: Kantenlänge und Anzahl je Reihe (580 px breite Spalte links)
HEART_PX, HEART_COLS = 50, 11


# Kartenfläche; muss zu kMapAreaW/kMapAreaH in native/publish.h passen
MAP_X, MAP_Y, MAP_W, MAP_H = 40, 392, 1160, 380
# Raumhöhen wie kMapSizes in native/dungeon_map.h; Breite 8/5, Abstand halbe Höhe
MAP_SIZES = [80, 72, 64]
ROOM_CUR = "#FFEDE3D6"   # aktueller Raum: hell, mit gelbem Rahmen
ROOM_SEEN = "#FF8C7F76"  # besucht
ROOM_DIM = "#FF3C3337"   # nur entdeckt (Spielkarte: halbe Deckkraft)
ROOM_UNKNOWN = "#FF4A4448"  # unentdeckter Nachbar mit Fragezeichen
# Raumkacheln der Spielkarte: "MapPlayer" (74 × 72, grau mit rauem schwarzem Rand, als 9-Slice
# gestreckt) und "Map_You" (205 × 123, hell, aktueller Raum; Seitenverhältnis wie die Räume)
TILE = {"type": "image", "src": "module:cotl:mapicon/MapPlayer", "fill": "slice",
        "slice": [14, 14, 14, 14]}
TILE_CUR = {"type": "image", "src": "module:cotl:mapicon/Map_You"}
# Spielerfigur auf dem aktuellen Raum: Lammkopf (Sprite in resources.assets, Atlas Coop_Indicator).
# Das Kartensymbol des Spiels sitzt im MiniMap-Prefab, das nicht lesbar ist (docs/re-notes.md).
MAP_PLAYER = "Coop_Indicator_Lamb"


def map_widgets():
    """Räume und Türstummel je Größe; nur die Vorlagen der gewählten Größe sind sichtbar."""
    w = [{"type": "rect", "rect": [MAP_X - 10, MAP_Y - 10, MAP_W + 20, MAP_H + 20], "bg": PANEL,
          "need_bind": "g.in_run"},
         {"type": "label", "rect": [0, MAP_Y + MAP_H // 2 - 20, W, 40], "text": "No map yet",
          "align": "center", "text_scale": L_BODY, "color": MUTED, "need_bind": "map.empty"}]
    for k, u in enumerate(MAP_SIZES):
        rw, rh, gap = u * 8 // 5, u, u // 2
        slen, t = (gap + 1) // 2, max(4, u // 6)
        gate = f"map.is{k}"

        def each(widget, count, xb, yb, gate=gate):
            widget = dict(widget)
            x, y, ww, wh = widget["rect"]
            widget["rect"] = [MAP_X + x, MAP_Y + y, ww, wh]
            widget.update({"repeat": 200, "repeat_bind": count, "x_bind": xb, "y_bind": yb,
                           "need_bind": gate, "scroll": "map.clip"})
            return widget

        def room(widget):
            return each(widget, f"map.count{k}", "map.x{i}", "map.y{i}")

        def stubs(d, size):
            # Stummel besuchter Räume hell, nur entdeckter Räume blass
            return [each({"type": "rect", "rect": [0, 0, *size], "bg": color,
                          "hide_bind": f"map.{d}dim{{i}}", "hide_eq": hide},
                         f"map.{d}count{k}", f"map.{d}x{{i}}", f"map.{d}y{{i}}")
                    for color, hide in ((ROOM_SEEN, 1), (ROOM_DIM, 0))]

        icon = rh * 7 // 10   # Raumsymbol
        me = rh * 17 // 20    # Spielerfigur
        qi = max(2, rh // 8)  # Fragezeichen-Räume: um qi je Seite kleiner
        qs = max(2, u // 13)  # Schriftstufe des Fragezeichens
        ext = gap - slen + qi  # Tür bis an einen Fragezeichen-Raum (MapExtLen)
        w += stubs("h", (slen, t)) + stubs("v", (t, slen)) + stubs("he", (ext, t)) +             stubs("ve", (t, ext)) + [
            # Raumkacheln der Spielkarte (MapIcons-Atlas): besucht grau, nur entdeckt halb
            # durchsichtig, aktueller Raum hell
            room({**TILE, "rect": [0, 0, rw, rh], "hide_bind": "map.vis{i}", "hide_eq": 0}),
            room({**TILE, "rect": [0, 0, rw, rh], "tint": "#80FFFFFF",
                  "hide_bind": "map.dim{i}", "hide_eq": 0}),
            room({**TILE_CUR, "rect": [0, 0, rw, rh], "hide_bind": "map.cur{i}", "hide_eq": 0}),
            # Raumsymbol wie die Spielkarte (GenerateRoom.MapIcon), nur entdeckt: halb durchsichtig
            room({"type": "image", "rect": [(rw - icon) // 2, (rh - icon) // 2, icon, icon],
                  "src_bind": "map.icon{i}", "hide_bind": "map.has_icon{i}", "hide_eq": 0,
                  "tint_bind": "map.dim{i}", "tint_colors": ["#FFFFFFFF", "#80FFFFFF"]}),
            # Unentdeckter Nachbar eines besuchten Raums: grauer, etwas kleinerer Raum mit "?"
            room({**TILE, "rect": [qi, qi, rw - 2 * qi, rh - 2 * qi], "tint": "#FF8A8A8A",
                  "hide_bind": "map.q{i}", "hide_eq": 0}),
            room({"type": "label", "rect": [0, (rh - 5 * qs) // 2, rw, 5 * qs], "text": "?",
                  "align": "center", "text_scale": qs, "color": TEXT,
                  "hide_bind": "map.q{i}", "hide_eq": 0}),
            # Spielerfigur auf dem aktuellen Raum
            room({"type": "image", "rect": [(rw - me) // 2, (rh - me) // 2, me, me],
                  "src": "module:cotl:mapicon/" + MAP_PLAYER,
                  "hide_bind": "map.cur{i}", "hide_eq": 0}),
        ]
    return w


def dungeon_page():
    w = background("dungeon")
    w.append({"type": "label", "rect": [0, 540, W, 40], "text": "Not in a dungeon run",
              "align": "center", "text_scale": L_BODY, "color": MUTED, "need_bind": "!g.in_run"})
    run = {"need_bind": "g.in_run"}
    # Herzen links wie im HUD (Bilder aus heart.atlas): rote mit leeren Behältern, Geist-, blaue,
    # schwarze, Feuer- und Eisherzen; Reihen zu HEART_COLS
    y = TAB_H + 14
    w.append({"type": "rect", "rect": [30, y - 6, 618, 270], "bg": PANEL, **run})
    w.append({"type": "label", "rect": [54, y + 14, 500, 36], "text": "Hearts", "text_scale": L_SMALL,
              "color": MUTED, **run})
    w.append({"type": "image", "rect": [54, y + 48, HEART_PX, HEART_PX], "src_bind": "run.heart{i}",
              "repeat": 44, "repeat_bind": "run.heart_count", "repeat_cols": HEART_COLS,
              "repeat_dx": HEART_PX + 2, "repeat_row_dy": HEART_PX + 2, **run})
    # Ausrüstung rechts, eine Zeile je Teil
    x = 660
    w.append({"type": "label", "rect": [x + 4, y + 14, 500, 36], "text": "Equipment", "text_scale": L_SMALL,
              "color": MUTED, **run})
    gear = [("Weapon", "run.weapon", "run.weapon_lvl", "run.has_weapon", "run.weapon_icon"),
            ("Curse", "run.curse", "run.curse_lvl", "run.has_curse", "run.curse_icon"),
            ("Relic", "run.relic", None, "run.has_relic", "run.relic_icon")]
    for k, (label, name, lvl, need, icon) in enumerate(gear):
        gy = y + 44 + k * 76
        w += [
            # Antippen zeigt Werte und Text wie die Karte im Spiel
            {"type": "rect", "rect": [x, gy, W - x - 40, 68], "bg": PANEL, "on_tap": "run.select",
             "payload": str(100 + k), **run},
            {"type": "label", "rect": [x + 24, gy + 28, 110, 30], "text": label,
             "text_scale": S_SMALL, "color": MUTED, **run},
            # Icon aus den Spieldateien (WeaponsIcons- bzw. Reliquien-Atlas)
            {"type": "image", "rect": [x + 124, gy + 4, 60, 60], "src_bind": icon,
             "need_bind": need},
            {"type": "label", "rect": [x + 196, gy + 26, 270, 36], "bind_text": name,
             "text_scale": L_SMALL, "color": TEXT, "need_bind": need},
            {"type": "label", "rect": [x + 196, gy + 26, 270, 36], "text": "None",
             "text_scale": L_SMALL, "color": MUTED, "need_bind": need.replace("has_", "none_")},
        ]
        if lvl:
            w.append({"type": "value", "rect": [W - 64, gy + 26, 0, 0], "bind": lvl,
                      "text": "Lv ", "align": "right", "text_scale": L_SMALL, "color": FAITH,
                      "need_bind": need})
    w += map_widgets()
    # Tarotkarten als Raster unter der Karte; Antippen zeigt die Wirkung
    ty = MAP_Y + MAP_H + 18
    w.append({"type": "label", "rect": [40, ty, 500, 36], "text": "Tarot cards",
              "text_scale": L_SMALL, "color": MUTED, **run})
    w.append({"type": "label", "rect": [40, ty + 50, 600, 40], "text": "No tarot cards yet",
              "text_scale": L_BODY, "color": MUTED, "need_bind": "run.no_tarot"})
    top = ty + 44
    cols, cw, ch, gap = 3, 380, 100, 10

    def card(widget):
        widget = dict(widget)
        x0, y0, rw, rh = widget["rect"]
        widget["rect"] = [40 + x0, top + y0, rw, rh]
        widget.update({"repeat": 60, "repeat_bind": "run.tarot_count", "repeat_cols": cols,
                       "repeat_dx": cw + gap, "repeat_row_dy": ch + gap, "scroll": "run.tarot"})
        return widget

    w += [
        card({"type": "rect", "rect": [0, 0, cw, ch], "bg": PANEL, "on_tap": "run.select",
              "payload": "{i}", "select_group": "tarot"}),
        # Kartenvorderseite (WeaponCard.atlas, 312 × 497)
        card({"type": "image", "rect": [14, 8, 53, 84], "src_bind": "run.tarot_icon{i}"}),
        card({"type": "label", "rect": [82, 22, cw - 96, ch - 26], "bind_text": "run.tarot_name{i}",
              "wrap_width": cw - 96, "max_lines": 2, "line_gap": 14,
              "text_scale": L_SMALL, "color": TEXT}),
    ]
    vis, win = popup("run.open", "run.close", top=TAB_H + 200, bottom=H - 200, need="run.sel_open")
    w += win
    w += [
        {"type": "label", "rect": [80, TAB_H + 240, W - 160, 50], "bind_text": "run.sel_name",
         "text_scale": S_TITLE, "color": TEXT, **vis},
        {"type": "label", "rect": [80, TAB_H + 296, W - 160, 36], "bind_text": "run.sel_stats",
         "text_scale": L_SMALL, "color": FAITH, **vis},
        {"type": "label", "rect": [80, TAB_H + 350, W - 160, 200], "bind_text": "run.sel_text", "color_markup": True,
         "wrap_width": W - 160, "max_lines": 5, "line_gap": 18, "text_scale": L_BODY,
         "color": TEXT, **vis},
        {"type": "label", "rect": [80, TAB_H + 590, W - 160, 90], "bind_text": "run.sel_lore",
         "wrap_width": W - 160, "max_lines": 2, "line_gap": 14, "text_scale": L_SMALL,
         "color": MUTED, **vis},
    ]
    # Kartenausschnitt: schneidet Räume am Rand ab; Inhalt so groß wie die Fläche, also kein
    # Scrollen (große Etagen folgen dem aktuellen Raum, native/dungeon_map.h)
    scrolls = [{"id": "map.clip", "rect": [MAP_X, MAP_Y, MAP_W, MAP_H], "content_h": MAP_H,
                "fling": False, "show_bind": "g.in_run"},
               {"id": "run.tarot", "rect": [0, top, W, H - top], "count_bind": "run.tarot_count",
                "row_h": ch + gap, "cols": cols, "bar": "#80EDE3D6", "bar_track": "#22000000",
                "bar_w": 8, "show_bind": "g.in_run"}]
    return page("dungeon", w, scrolls)


# ---- Platzhalter und Warten -------------------------------------------------------------------

def placeholder_page(page_id, text):
    w = background(page_id)
    w.append({"type": "label", "rect": [0, 540, W, 40], "text": text, "align": "center",
              "text_scale": L_BODY, "color": MUTED})
    return page(page_id, w)


def waiting_page():
    return page("waiting", [
        {"type": "rect", "rect": [0, 0, W, H], "bg": BG},
        {"type": "label", "rect": [0, 500, W, 60], "text": "The Red Crown awaits", "align": "center",
         "text_scale": S_TITLE, "color": MUTED},
    ])


# ---- Werte, Aktionen, Gesamtdokument ----------------------------------------------------------

def derived():
    d = [
        # Am Titelbild gibt es schon ein DataManager-Objekt: bereit erst mit geladenem Spielstand
        {"name": "g.ready", "expr": "dm != 0 && g.loaded != 0"},
        # Maxima aus dem Spielcode: CultFaithManager.MAX_FAITH = 85, HungerBar.get_MAX_HUNGER = 100
        {"name": "cult.faith_max", "terms": [], "add": 85},
        {"name": "cult.hunger_max", "terms": [], "add": 100},
        {"name": "cult.faith_pct", "expr": "clamp(cult.faith * 100 / 85, 0, 100)", "floor": True},
        {"name": "cult.hunger_pct", "expr": "clamp(cult.hunger, 0, 100)", "floor": True},
        {"name": "cult.health", "expr": "max(cult.illness_max - cult.illness, 0)"},
        {"name": "cult.health_pct",
         "expr": "clamp(100 - cult.illness * 100 / max(cult.illness_max, 1), 0, 100)", "floor": True},
        # Angezeigte Seite: -1 Warten, im Run der Dungeon-Tab, sonst der zuletzt gewählte Tab
        {"name": "g.view",
         "expr": f"g.ready ? (g.in_run ? {DUNGEON_TAB} : @flag:tab) : -1"},
        {"name": "view.wait", "expr": "g.view == -1"},
        # Dungeon-Karte: gewählte Rastergröße, leere Karte im Run
        *[{"name": f"map.is{k}", "expr": f"map.sz == {k} && map.count > 0"}
          for k in range(len(MAP_SIZES))],
        {"name": "map.empty", "expr": "g.in_run && map.count == 0"},
        # Im Run: Platzhalter "None" je Ausrüstungsteil, "No tarot cards yet"
        *[{"name": f"run.none_{k}", "expr": f"g.in_run && run.has_{k} == 0"}
          for k in ("weapon", "curse", "relic")],
        {"name": "run.no_tarot", "expr": "g.in_run && run.tarot_count == 0"},
        *[{"name": f"view.{k}", "expr": f"g.view == {k}"} for k in range(len(TABS))],
        # Scroll-Regionen der Fenster nur bei offenem Fenster
        {"name": "fol.th_open", "expr": "@flag:fol.thoughts != -1"},
        {"name": "quest.steps_open", "expr": "@flag:quest.open != -1"},
        {"name": "prob.sel_open", "expr": "@flag:prob.open != -1"},
        {"name": "prob.sel_none", "expr": "@flag:prob.open != -1 && prob.sel_count == 0"},
        # Inventar: Beginn der Abschnitte im Scroll-Inhalt und dessen Gesamthöhe
        {"name": "inv.off0", "terms": [], "add": 0},
        {"name": "inv.off1", "expr": inv_section_h(0)},
        {"name": "inv.off2", "expr": f"{inv_section_h(0)} + {inv_section_h(1)}"},
        {"name": "inv.content_h",
         "expr": f"{inv_section_h(0)} + {inv_section_h(1)} + {inv_section_h(2)} + 20"},
    ]
    # Problem-Filter: Platz = Anzahl vorhandener Arten davor; 4 je Reihe (296 + 8 breit, 64 + 8 hoch)
    for k in range(len(PROBLEM_KINDS)):
        pos = " + ".join([f"min(prob.n{j}, 1)" for j in range(k)]) or "0"
        d.append({"name": f"prob.pos{k}", "expr": pos})
        d.append({"name": f"prob.cx{k}",
                  "expr": f"(prob.pos{k} - 4 * floor(prob.pos{k} / 4)) * 304"})
        d.append({"name": f"prob.cy{k}", "expr": f"floor(prob.pos{k} / 4) * 72"})
    # Vergangene Minuten je Tagesphase (0–240) für den Tagesbalken
    for k in range(len(PHASES)):
        d.append({"name": f"g.seg{k}", "expr": f"clamp(g.time - {k * 240}, 0, 240)"})
    return d


def actions():
    acts = {f"go.{pid}": {"kind": "page", "page": pid, "transition": "fade", "duration_ms": 120}
            for pid, _ in TABS}
    acts["fol.select"] = {"kind": "flag", "flag": "fol.open", "value": "$payload"}
    acts["fol.close"] = {"kind": "flag", "flag": "fol.open", "value": -1}
    acts["fol.thoughts"] = {"kind": "flag", "flag": "fol.thoughts", "value": 1}
    acts["fol.thoughts_close"] = {"kind": "flag", "flag": "fol.thoughts", "value": -1}
    for k in range(len(TABS)):
        acts[f"tab.{k}"] = {"kind": "flag", "flag": "tab", "value": k}
    # Das Tarot-Fenster hält das Modul: es schließt mit dem Run (native/module.cpp)
    acts["run.select"] = {"kind": "module", "action": "run.select", "argument": "$payload"}
    acts["run.close"] = {"kind": "module", "action": "run.close", "argument": 0}
    acts["quest.select"] = {"kind": "flag", "flag": "quest.open", "value": "$payload"}
    acts["quest.close"] = {"kind": "flag", "flag": "quest.open", "value": -1}
    acts["prob.select"] = {"kind": "flag", "flag": "prob.open", "value": "$payload"}
    acts["prob.close"] = {"kind": "flag", "flag": "prob.open", "value": -1}
    acts["inv.select"] = {"kind": "flag", "flag": "inv.open", "value": "$payload"}
    acts["inv.close"] = {"kind": "flag", "flag": "inv.open", "value": -1}
    for k in range(len(PROBLEM_KINDS)):
        acts[f"prob.f{k}"] = {"kind": "flag", "flag": "prob.filter", "value": k}
    acts["prob.fclear"] = {"kind": "flag", "flag": "prob.filter", "value": -1}
    return acts


def manifest():
    return {
        "format": 1,
        "name": "Cult of the Lamb dual screen",
        "title_id": "01002E7016C46000",
        "min_runtime": 22,  # content_h_bind der Inventar-Scroll-Region
        "canvas_w": W,
        "canvas_h": H,
        "background": BG,
        # Spielschrift (Umlaute, Apostroph, Kleinbuchstaben): Metriken und Atlas vom Modul
        "font": "file:cotl_font.txt",
        "font_atlas": "module:cotl:font",
        "flags": {"fol.open": -1, "fol.thoughts": -1, "quest.open": -1, "prob.filter": -1,
                  "inv.open": -1, "prob.open": -1, "tab": 0},
        "derived": derived(),
        "actions": actions(),
        "page_binds": [{"point": "view.wait", "equals": 1, "when_equal": {"page": "waiting"}}] + [
            {"point": f"view.{k}", "equals": 1,
             "when_equal": {"page": pid, "transition": "fade", "duration_ms": 120}}
            for k, (pid, _) in enumerate(TABS)],
        "pages": [
            waiting_page(),
            cult_page(),
            problems_page(),
            quests_page(),
            followers_page(),
            inventory_page(),
            dungeon_page(),
        ],
    }


def write_manifest(path):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(manifest(), f, indent=1, ensure_ascii=False)
        f.write("\n")


if __name__ == "__main__":
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        root, "package", "CultOfTheLamb", "dualscreen", "manifest.json")
    write_manifest(out)
    print(out)
