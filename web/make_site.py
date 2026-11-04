#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Make the site (GitHub Pages):

  index.html                  redirect to the main installer page (the first package)
  firmware/<name>.fwsc        every package (+ LICENSE, LICENSING.md, LICENSES/: the package holds
                              JieLi SDK files under Apache-2.0, see LICENSING.md)
  webapp/installer/index.html the first package's page, index_pkg.html self-contained (fm1pkg.js,
                              fm1ota.js, metadata inlined)
  webapp/installer-<model>/   one page per further package, with its own model name and package
  webapp/editor/index.html    editor.html (+ fukiai.ttf, FUKIAI-LICENSE.txt)
  src/                        not touched

  web/make_site.py OUT_DIR VERSION PKG [PKG ...]

The packages must be made by tools/fm1pkg_make.py (Felucca's own loader, no vendor files).
Each identity (FM-1_9xx, SMK-37 Pro/Elite_9xx, MKE-P37_9xx) is read from its package; the device must
report it after the install. The first package is the site's main installer page, the others get a page
each and every page links to the others, so one site serves every model.
"""
import json
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F
IDENTITY = re.compile(r"(FM-1|SMK-37 Pro|SMK-37 Elite|MKE-P37)_9\d\d")
# webapp/<page> per model (the first package always gets "installer", for the URLs already published)
PAGE = {"SMK-37 Pro": "installer-smk37-pro", "SMK-37 Elite": "installer-smk37-elite",
        "MKE-P37": "installer-mke-p37", "FM-1": "installer-fm-1"}
SUFFIX = {"SMK-37 Pro": "", "SMK-37 Elite": "-elite", "MKE-P37": "-mke", "FM-1": ""}


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def product_of(raw):
    """the package identity: one marker byte after each of the first 20 blocks (fm1pkg.js productOf)"""
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def main(out, version, pkgs):
    out = Path(out)
    packages = []
    for p in pkgs:
        pkg = Path(p)
        raw = pkg.read_bytes()
        product = product_of(raw)
        model = product.split("_")[0]
        if not IDENTITY.fullmatch(product) or model not in PAGE:
            raise SystemExit(f"{pkg}: identity {product!r} is not a Felucca package "
                             "(FM-1_9xx, SMK-37 Pro_9xx, SMK-37 Elite_9xx, MKE-P37_9xx)")
        if b"FELUCCA-LOADER-1" not in raw:      # Felucca loader marker: never publish a package with vendor files
            raise SystemExit(f"{pkg}: no Felucca loader in it; the site ships only fm1pkg_make.py packages "
                             "(a package patched from an official one carries vendor files)")
        packages.append({"pkg": pkg, "raw": raw, "product": product, "model": model,
                         "name": f"fmini-1-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}{SUFFIX[model]}.fwsc"})
    if len({p["name"] for p in packages}) != len(packages):
        raise SystemExit("two packages of the same model: give each model a package (one file per model)")
    html = (HERE / "index_pkg.html").read_text(encoding="utf-8")
    lib = strip_module((HERE / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1ota.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1backup.js").read_text(encoding="utf-8"))
    for mark in ("/*LIB*/", "/*META*/", "/*OTHERS*/"):
        if html.count(mark) != 1:
            raise SystemExit(f"index_pkg.html must contain {mark} once; update make_site.py")
    inst, ed, fw = out / "webapp" / "installer", out / "webapp" / "editor", out / "firmware"
    for d in (inst, ed, fw):
        d.mkdir(parents=True, exist_ok=True)
    for old in fw.glob("*.fwsc"):                 # the packages: those of this run
        old.unlink()
    url = lambda k: "installer" if k == 0 else PAGE[packages[k]["model"]]   # noqa: E731
    made = []
    for i, p in enumerate(packages):
        others = " · ".join(f'<a href="../{url(j)}/" style="color:inherit">{o["model"]}</a>'
                            for j, o in enumerate(packages) if j != i)
        meta = json.dumps({"version": version, "product": p["product"],
                           "pkg": "../../firmware/" + p["name"]})
        page = (html.replace("/*LIB*/", lib).replace("/*META*/", meta)
                .replace("SMK-37 PRO", p["model"].upper())     # the banner, then every mention of the model
                .replace("SMK-37 Pro", p["model"])
                .replace("/*OTHERS*/", others))                 # last: the links name the other models
        d = out / "webapp" / url(i)
        d.mkdir(parents=True, exist_ok=True)
        (d / "index.html").write_text(page, encoding="utf-8")
        shutil.copy(p["pkg"], fw / p["name"])
        made.append(f"webapp/{url(i)} ({p['model']}, {len(p['raw'])} B, {p['product']})")
    lic = HERE.parent / "LICENSES"                  # the package holds JieLi SDK files (Apache-2.0): their
    (fw / "LICENSES").mkdir(exist_ok=True)          # licence travels next to it, with Felucca's own
    names = sorted(f.name for f in lic.glob("*.txt"))
    for n in names:
        shutil.copy(lic / n, fw / "LICENSES" / n)
    (fw / "LICENSES" / "index.html").write_text(    # the installer links this folder: Pages lists no folders
        '<!doctype html><meta charset="utf-8"><title>Felucca licences</title><h1>Licence texts</h1><ul>'
        + "".join(f'<li><a href="{n}">{n}</a></li>' for n in names)
        + '</ul><p><a href="../LICENSING.md">LICENSING.md</a> · <a href="../LICENSE">LICENSE (GPL-3.0)</a></p>\n',
        encoding="utf-8")
    for doc in ("LICENSE", "LICENSING.md"):
        shutil.copy(HERE.parent / doc, fw / doc)
    shutil.copy(HERE / "editor.html", ed / "index.html")
    for f in ("fukiai.ttf", "FUKIAI-LICENSE.txt", "fm1backup.js"):
        if (HERE / f).exists():
            shutil.copy(HERE / f, ed / f)
    (out / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>Felucca</title>'
        f'<meta http-equiv="refresh" content="0; url=webapp/{url(0)}/">'
        f'<a href="webapp/{url(0)}/">Felucca installer</a>\n', encoding="utf-8")
    print(f"site: {out}: " + ", ".join(made) + ", webapp/editor")


if __name__ == "__main__":
    if len(sys.argv) < 5:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2], sys.argv[3:])
