#!/usr/bin/env python3
from __future__ import annotations

import html
import re
from pathlib import Path


DOCS_DIR = Path(__file__).resolve().parent
PROJECT_DIR = DOCS_DIR.parent
SITE_DIR = PROJECT_DIR / "site"


def site_source(name: str) -> Path:
    site_path = SITE_DIR / name
    if site_path.exists():
        return site_path
    return PROJECT_DIR / name

PAGES = [
    {
        "source": site_source("SPLASH.md"),
        "output": DOCS_DIR / "index.html",
        "label": "Home",
        "brand_title": "Major MIDI",
        "hero_title": "Major MIDI",
        "hero_tagline": "Midi File Player with Soundfount 2 Synth for Eurorack Modular",
    },
    {
        "source": site_source("USER.md"),
        "output": DOCS_DIR / "user.html",
        "label": "User Manual",
        "brand_title": "User Manual",
        "hero_title": "User Manual",
        "hero_tagline": "Current operating guide for the Major MIDI firmware.",
    },
    {
        "source": site_source("DEV.md"),
        "output": DOCS_DIR / "dev.html",
        "label": "Dev Resources",
        "brand_title": "Dev Resources",
        "hero_title": "Dev Resources",
        "hero_tagline": "Build notes, source map, and development entry points.",
    },
    {
        "source": site_source("ORDER.md"),
        "output": DOCS_DIR / "order.html",
        "label": "Order",
        "brand_title": "Order",
        "hero_title": "Order Major MIDI",
        "hero_tagline": "Ordering status, hardware context, and contact path.",
    },
]

NAV_LINKS = [
    ("index.html", "Home"),
    ("user.html", "User Manual"),
    ("dev.html", "Dev Resources"),
    ("order.html", "Order"),
    ("transfer.html", "Transfer MIDI"),
    ("remote.html", "Web Remote"),
]


def slugify(text: str) -> str:
    text = text.lower()
    text = re.sub(r"[^a-z0-9]+", "-", text)
    return text.strip("-") or "section"


def inline_format(text: str) -> str:
    text = html.escape(text)
    text = re.sub(
        r"\[([^\]]+)\]\(([^)]+)\)",
        lambda match: (
            f'<a href="{html.escape(match.group(2), quote=True)}">'
            f"{match.group(1)}</a>"
        ),
        text,
    )
    text = re.sub(r"`([^`]+)`", r"<code>\1</code>", text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", text)
    return text


def plain_text(text: str) -> str:
    text = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r"\1", text)
    text = text.replace("`", "")
    return text


def split_blocks(lines: list[str]) -> list[tuple[str, object]]:
    blocks: list[tuple[str, object]] = []
    i = 0
    while i < len(lines):
        line = lines[i].rstrip("\n")
        stripped = line.strip()
        if not stripped:
            i += 1
            continue

        if stripped.startswith("```"):
            lang = stripped[3:].strip()
            i += 1
            code_lines: list[str] = []
            while i < len(lines) and not lines[i].strip().startswith("```"):
                code_lines.append(lines[i].rstrip("\n"))
                i += 1
            if i < len(lines):
                i += 1
            blocks.append(("code", (lang, "\n".join(code_lines))))
            continue

        if stripped.startswith("<"):
            html_lines: list[str] = [line]
            i += 1
            while i < len(lines):
                candidate = lines[i].rstrip("\n")
                if not candidate.strip():
                    break
                html_lines.append(candidate)
                i += 1
            blocks.append(("html", "\n".join(html_lines)))
            continue

        if re.match(r"^#{1,6}\s", stripped):
            level = len(stripped) - len(stripped.lstrip("#"))
            text = stripped[level:].strip()
            blocks.append(("heading", (level, text)))
            i += 1
            continue

        if re.match(r"^\d+\.\s+", stripped):
            items: list[str] = []
            while i < len(lines):
                candidate = lines[i].strip()
                if not re.match(r"^\d+\.\s+", candidate):
                    break
                items.append(re.sub(r"^\d+\.\s+", "", candidate))
                i += 1
            blocks.append(("olist", items))
            continue

        if "|" in stripped and i + 1 < len(lines):
            separator = lines[i + 1].strip()
            if "|" in separator and re.fullmatch(r"[\|\-\:\s]+", separator):
                header_cells = [
                    cell.strip() for cell in stripped.strip("|").split("|")
                ]
                rows: list[list[str]] = []
                i += 2
                while i < len(lines):
                    candidate = lines[i].strip()
                    if not candidate or "|" not in candidate:
                        break
                    rows.append(
                        [cell.strip() for cell in candidate.strip("|").split("|")]
                    )
                    i += 1
                blocks.append(("table", (header_cells, rows)))
                continue

        if stripped.startswith("- "):
            items: list[str] = []
            while i < len(lines):
                candidate = lines[i].strip()
                if not candidate.startswith("- "):
                    break
                items.append(candidate[2:].strip())
                i += 1
            blocks.append(("ulist", items))
            continue

        paragraph: list[str] = [stripped]
        i += 1
        while i < len(lines):
            candidate = lines[i].strip()
            if not candidate:
                break
            if candidate.startswith("```") or candidate.startswith("- "):
                break
            if re.match(r"^\d+\.\s+", candidate):
                break
            if re.match(r"^#{1,6}\s", candidate):
                break
            paragraph.append(candidate)
            i += 1
        blocks.append(("paragraph", " ".join(paragraph)))
    return blocks


def build_sections(blocks: list[tuple[str, object]]) -> tuple[str, list[tuple[str, str]]]:
    nav: list[tuple[str, str]] = []
    sections: list[str] = []
    current: list[str] = []
    current_id = ""
    current_title = ""
    first_h1_seen = False
    in_section = False

    def flush_section() -> None:
        nonlocal current, current_id, current_title
        if not current or not current_id or not current_title:
            return
        sections.append(
            f'<section class="section" id="{current_id}">\n'
            f"  <h2>{html.escape(current_title)}</h2>\n"
            + "\n".join(current)
            + "\n</section>"
        )
        current = []

    for kind, data in blocks:
        if kind == "heading":
            level, text = data  # type: ignore[misc]
            if level == 1 and not first_h1_seen:
                first_h1_seen = True
                continue
            if level == 2:
                flush_section()
                current_title = text
                current_id = slugify(text)
                nav.append((current_id, text))
                in_section = True
                continue
            if level == 3:
                if not in_section:
                    continue
                current.append(f"<h3>{html.escape(text)}</h3>")
                continue
            if level == 4:
                if not in_section:
                    continue
                current.append(f"<h4>{html.escape(text)}</h4>")
                continue

        if not in_section:
            continue

        if kind == "paragraph":
            current.append(f"<p>{inline_format(data)}</p>")  # type: ignore[arg-type]
        elif kind == "ulist":
            items = "".join(
                f"<li>{inline_format(item)}</li>" for item in data  # type: ignore[arg-type]
            )
            current.append(f'<ul class="bullets">{items}</ul>')
        elif kind == "olist":
            items = "".join(
                f"<li>{inline_format(item)}</li>" for item in data  # type: ignore[arg-type]
            )
            current.append(f'<ol class="steps">{items}</ol>')
        elif kind == "table":
            headers, rows = data  # type: ignore[misc]
            thead = "".join(f"<th>{inline_format(cell)}</th>" for cell in headers)
            body_rows = []
            for row in rows:
                padded = list(row) + [""] * max(0, len(headers) - len(row))
                cells = "".join(
                    f"<td>{inline_format(cell)}</td>"
                    for cell in padded[: len(headers)]
                )
                body_rows.append(f"<tr>{cells}</tr>")
            tbody = "".join(body_rows)
            current.append(
                '<div class="table-wrap"><table><thead><tr>'
                + thead
                + "</tr></thead><tbody>"
                + tbody
                + "</tbody></table></div>"
            )
        elif kind == "code":
            lang, code = data  # type: ignore[misc]
            cls = f' class="language-{html.escape(lang)}"' if lang else ""
            current.append(f"<pre><code{cls}>{html.escape(code)}</code></pre>")
        elif kind == "html":
            current.append(str(data))

    flush_section()
    return "\n".join(sections), nav


def build_html(
    page: dict[str, object],
    title: str,
    intro: str,
    sections_html: str,
    nav: list[tuple[str, str]],
) -> str:
    del title
    page_nav = []
    current_output = Path(page["output"]).name  # type: ignore[arg-type]
    for href_raw, label_raw in NAV_LINKS:
        href = html.escape(href_raw)
        label = html.escape(label_raw)
        active = ' class="active"' if href == current_output else ""
        page_nav.append(f'          <a{active} href="{href}">{label}</a>')

    section_nav = "\n".join(
        f'          <a href="#{slug}">{html.escape(text)}</a>'
        for slug, text in nav
    )
    source_name = html.escape(Path(page["source"]).name)  # type: ignore[arg-type]
    brand_title = html.escape(str(page["brand_title"]))
    hero_title = html.escape(str(page["hero_title"]))
    hero_tagline = html.escape(str(page["hero_tagline"]))
    page_title = str(page["hero_title"])
    if current_output != "index.html":
        page_title = f"{page_title} | Major MIDI"

    return f"""<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>{html.escape(page_title)}</title>
    <meta
      name="description"
      content="{html.escape(intro)}"
    />
    <link rel="stylesheet" href="./styles.css" />
  </head>
  <body>
    <div class="shell">
      <aside class="sidebar">
        <div class="brand">
          <p class="kicker">Major MIDI</p>
          <h1>{brand_title}</h1>
        </div>

        <p class="nav-label">Pages</p>
        <nav class="nav page-nav">
{chr(10).join(page_nav)}
        </nav>

        <p class="nav-label">On This Page</p>
        <nav class="nav section-nav">
{section_nav}
        </nav>
      </aside>

      <main class="main">
        <section class="hero" id="top">
          <div class="hero-grid">
            <div>
              <h2>{hero_title}</h2>
              <p>{hero_tagline}</p>
            </div>
          </div>
        </section>

        {sections_html}

        <div class="footer">
          Generated from <code>{source_name}</code>.
        </div>
      </main>
    </div>
  </body>
</html>
"""


def render_page(page: dict[str, object]) -> None:
    source = Path(page["source"])  # type: ignore[arg-type]
    output = Path(page["output"])  # type: ignore[arg-type]
    text = source.read_text(encoding="utf-8")
    lines = text.splitlines()
    title = str(page["hero_title"])
    intro = "Major MIDI documentation."

    for line in lines:
        stripped = line.strip()
        if stripped.startswith("# "):
            title = stripped[2:].strip()
            break

    for line in lines:
        stripped = line.strip()
        if stripped and not stripped.startswith("#"):
            intro = plain_text(stripped)
            break

    blocks = split_blocks(lines)
    sections_html, nav = build_sections(blocks)
    output.write_text(
        build_html(page, title, intro, sections_html, nav),
        encoding="utf-8",
    )


def main() -> None:
    for page in PAGES:
        render_page(page)


if __name__ == "__main__":
    main()
