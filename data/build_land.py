import json
from pathlib import Path

root = Path(__file__).resolve().parent
countries = json.loads(Path(r"C:\Users\User\AppData\Local\Temp\ne_110m_admin_0_countries.geojson").read_text(encoding="utf-8"))
provinces = json.loads(Path(r"C:\Users\User\AppData\Local\Temp\ne_50m_admin_1.geojson").read_text(encoding="utf-8"))

boxes = [
    ("ural", 44, 62, 52, 68),
    ("siberia", 60, 120, 50, 72),
    ("far-east", 120, 168, 48, 70),
    ("central-asia", 52, 80, 36, 50),
    ("east-asia", 100, 145, 20, 45),
    ("europe", -10, 40, 36, 70),
    ("middle-east", 32, 62, 14, 38),
    ("south-asia", 68, 95, 8, 32),
    ("southeast-asia", 95, 141, -11, 21),
    ("africa", -16, 50, -35, 34),
    ("north-america", -168, -52, 14, 78),
    ("south-america", -82, -34, -56, 13),
    ("oceania", 112, 180, -48, -10),
]
forced = {
    "AU": "oceania", "NZ": "oceania", "PG": "oceania", "FJ": "oceania", "SB": "oceania", "VU": "oceania",
    "NC": "oceania", "WS": "oceania", "TO": "oceania", "KI": "oceania", "FM": "oceania", "MH": "oceania",
    "PW": "oceania", "NR": "oceania", "TV": "oceania",
    "ID": "southeast-asia", "PH": "southeast-asia", "MY": "southeast-asia", "SG": "southeast-asia",
    "BN": "southeast-asia", "TH": "southeast-asia", "VN": "southeast-asia", "LA": "southeast-asia",
    "KH": "southeast-asia", "MM": "southeast-asia", "TL": "southeast-asia",
    "MN": "east-asia", "KP": "east-asia", "KR": "east-asia", "JP": "east-asia", "TW": "east-asia",
    "GL": "north-america", "IS": "europe",
}

def area(box):
    return (box[2] - box[1]) * (box[4] - box[3])

def assign_country(code, lon, lat):
    if code in forced:
        return forced[code]
    hits = [box for box in boxes if box[1] <= lon <= box[2] and box[3] <= lat <= box[4]]
    if hits:
        return min(hits, key=area)[0]
    best = None
    best_d = 1e9
    for box in boxes:
        cx = (box[1] + box[2]) * 0.5
        cy = (box[3] + box[4]) * 0.5
        distance = (lon - cx) ** 2 + (lat - cy) ** 2
        if distance < best_d:
            best_d = distance
            best = box[0]
    return best

def assign_russia(lon, lat):
    if lon >= 118:
        return "far-east"
    if lon >= 62:
        return "siberia"
    if lon >= 44 and lat >= 50:
        return "ural"
    if lat < 50 and lon >= 46:
        return "central-asia"
    if lat < 46:
        return "middle-east"
    return "european-russia"

def rings_of(geometry):
    kind = geometry["type"]
    coords = geometry["coordinates"]
    if kind == "Polygon":
        return [coords[0]]
    if kind == "MultiPolygon":
        return [poly[0] for poly in coords]
    return []

def simplify(ring):
    cleaned = []
    for lon, lat in ring:
        if lat < -62:
            continue
        point = (round(float(lon), 2), round(float(lat), 2))
        if not cleaned or abs(point[0] - cleaned[-1][0]) > 180:
            if len(cleaned) >= 3:
                yield cleaned
            cleaned = [point]
            continue
        if cleaned[-1] != point:
            cleaned.append(point)
    if len(cleaned) >= 3:
        yield cleaned

def thin(ring, limit=72):
    if len(ring) <= limit:
        return ring
    step = len(ring) / limit
    picked = [ring[int(i * step)] for i in range(limit)]
    if picked[0] != picked[-1]:
        picked.append(picked[0])
    return picked

out = []
counts = {}

def add_feature(feature, region):
    if region is None:
        return
    for ring in rings_of(feature["geometry"]):
        for part in simplify(ring):
            part = thin(part)
            if len(part) < 3:
                continue
            out.append({"region": region, "points": part})
            counts[region] = counts.get(region, 0) + 1

for feature in countries["features"]:
    props = feature["properties"]
    code = props.get("ADM0_A3") or ""
    iso = props.get("ISO_A2") or ""
    if code in ("ATA", "RUS") or iso == "RU":
        continue
    ring = rings_of(feature["geometry"])
    if not ring:
        continue
    lon = sum(p[0] for p in ring[0]) / len(ring[0])
    lat = sum(p[1] for p in ring[0]) / len(ring[0])
    if lat < -62:
        continue
    add_feature(feature, assign_country(iso if iso and iso != "-99" else code, lon, lat))

for feature in provinces["features"]:
    props = feature["properties"]
    if props.get("adm0_a3") != "RUS" and props.get("admin") != "Russia":
        continue
    ring = rings_of(feature["geometry"])
    if not ring:
        continue
    lon = sum(p[0] for p in ring[0]) / len(ring[0])
    lat = sum(p[1] for p in ring[0]) / len(ring[0])
    add_feature(feature, assign_russia(lon, lat))

(root / "land.json").write_text(json.dumps(out, separators=(",", ":")), encoding="utf-8")
print("polys", len(out), "bytes", (root / "land.json").stat().st_size)
for key in sorted(counts):
    print(f"{key}: {counts[key]}")
