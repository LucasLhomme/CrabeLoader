"""Developer script: regenerate src/assets/roster.json, the list of playable figures Creator can start from.

Reads two decompiled game scripts (unluac output) and joins them by actor name:
  virtualreaderpc_data.lua  -> VirtualReaderPC_Data.AvatarData rows (Name, sku_id, Icon, ProgressionTree, MetaData)
  toybox_actors.lua         -> the Toy Box ActorList (name -> Type, DNAFile)

Usage:
  java -jar unluac.jar <decrypted>/presentation/.../virtualreaderpc_data.lua > vr.lua
  java -jar unluac.jar <decrypted>/gamedb/core/.../toybox_actors.lua > actors.lua
  python tools/build_roster.py vr.lua actors.lua
Only metadata is written; the models themselves are read from the player's own game install at run time.
"""
import json
import re
import sys
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "src" / "assets" / "roster.json"
FIELDS = ("Name", "sku_id", "Icon", "ProgressionTree", "MetaData")
FRANCHISES = {
    "ALA": "Aladdin", "AVG": "Avengers", "BHS": "Big Hero 6", "BRV": "Brave", "DIS": "Disney", "DNO": "The Good Dinosaur",
    "EMP": "Star Wars", "FRO": "Frozen", "GOG": "Guardians of the Galaxy", "INC": "The Incredibles",
    "LAS": "Lilo & Stitch", "LRA": "The Lone Ranger", "MAL": "Maleficent", "MON": "Monsters University",
    "MUL": "Mulan", "NBC": "The Nightmare Before Christmas", "OUT": "Inside Out", "PNF": "Phineas and Ferb",
    "POC": "Pirates of the Caribbean", "PSX": "Star Wars", "REB": "Star Wars Rebels", "SPD": "Spider-Man",
    "TAN": "Tangled", "TCW": "Star Wars: The Clone Wars", "TOY": "Toy Story", "TRN": "Tron", "WIR": "Wreck-It Ralph",
    "ZOO": "Zootopia",
}
NAMES = {
    "SPD_Spiderman": "Spider-Man", "SPD_Spiderman_Black": "Spider-Man (Black Suit)", "AV_ElastiGirl": "Elastigirl",
    "TCW_ObiWan": "Obi-Wan Kenobi", "GOG_StarLord": "Star-Lord", "TB_Tinkerbell": "Tinker Bell",
    "AV_MrIncredible": "Mr. Incredible", "AV_MrIncredibleInfinite": "Mr. Incredible (Crystal)",
    "AV_BuzzInfinite": "Buzz Lightyear (Crystal)", "AV_Buzz_Glow": "Buzz Lightyear (Glow)", "AV_Buzz": "Buzz Lightyear",
    "MU_SullyInfinite": "Sulley (Crystal)", "MU_Sully": "Sulley", "PIR_JackSparrowInfinite": "Jack Sparrow (Crystal)",
    "PNF_PerryInfinite": "Perry (Crystal)", "LR_LoneRangerInfinite": "Lone Ranger (Crystal)",
    "TB_MickeyMouseInfinite": "Mickey Mouse (Crystal)", "EMP_Luke": "Luke Skywalker", "EMP_Leia": "Princess Leia",
    "AVG_HulkBuster": "Hulkbuster", "WR_Ralph": "Wreck-It Ralph", "TBX_ClassicMickey": "Mickey Mouse (Classic)",
    "TBX_Minnie": "Minnie Mouse", "DNO_Spot": "Spot", "PSX_Grimm": "Grimm", "LAS_Stitch": "Stitch",
}


def avatar_rows(source: str) -> list[dict]:
    body = source[source.index("VirtualReaderPC_Data.AvatarData = {"):]
    body = body[:body.index("\nVirtualReaderPC_Data.", 10)]
    rows = []
    for block in re.findall(r"\{\s*\n(.*?)\n\s*\}", body, re.S):
        row = dict(re.findall(r'(\w+) = "([^"]*)"', block))
        if "Name" in row and "sku_id" in row:
            rows.append({k: row.get(k, "") for k in FIELDS})
    return rows


def actor_list(source: str) -> dict[str, dict]:
    """name (lowercase) -> {type, dnax}, replaying the register assignments of the decompiled ActorList."""
    reg = r"(L\d+(?:_\d+)?)"
    strings: dict[str, str] = {}
    records: dict[str, dict] = {}
    actors: dict[str, dict] = {}
    for line in source.splitlines():
        line = line.strip()
        if m := re.fullmatch(reg + r' = "(.*)"', line):
            strings[m[1]] = m[2]
        elif m := re.fullmatch(reg + r" = \{\}", line):
            records[m[1]] = {}
        elif m := re.fullmatch(reg + r'\.(Parms|Type|Name) = (?:"(.*)"|' + reg + r")", line):
            rec = records.setdefault(m[1], {})
            rec[m[2]] = m[3] if m[3] is not None else strings.get(m[4], "")
            if {"Parms", "Type", "Name"} <= rec.keys():
                dna = re.search(r"DNAFile=([^;]+)", rec["Parms"])
                actors[rec["Name"].lower()] = {"type": rec["Type"], "dnax": dna[1].strip() if dna else ""}
                records.pop(m[1])
    return actors


def pretty(name: str) -> str:
    stem = name.split("_", 1)[1] if "_" in name else name
    stem = stem.replace("_", " ")
    return re.sub(r"(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", " ", stem).strip()


if __name__ == "__main__":
    vr, act = Path(sys.argv[1]).read_text(encoding="utf-8"), Path(sys.argv[2]).read_text(encoding="utf-8")
    actors = actor_list(act)
    out = []
    for row in avatar_rows(vr):
        a = actors.get(row["Name"].lower())
        if not a or a["type"] != "Avatar" or not a["dnax"]:
            print(f"skip {row['Name']}: {a['type'] if a else 'not in the ActorList'}")
            continue
        code = row["MetaData"].split("Franchise_")[-1] if "Franchise_" in row["MetaData"] else ""
        franchise = FRANCHISES.get(code, code)
        out.append({"actor": row["Name"], "display_name": NAMES.get(row["Name"], pretty(row["Name"])),
                    "sku": row["sku_id"],
                    "icon": row["Icon"], "progression_tree": row["ProgressionTree"], "franchise": franchise,
                    "dnax": a["dnax"]})
    OUT.write_text(json.dumps(out, indent=1) + "\n", encoding="utf-8")
    print(f"{len(out)} figures written to {OUT}")
