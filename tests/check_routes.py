#!/usr/bin/env python3
# ARMOR-NODE-RADAR - guard on the routes of the node's web server. Nothing is compiled: it reads main/web_server.cpp (the dispatch), panel/app.js and tools/panel_mock.mjs and fails when
#   1. a route that every node of the family serves (the session, the settings, the users, the log, the core dump, the updates, the clock...) is missing here: a copy of the shared code that
#      lost a route is how a node ended up without a page of its panel;
#   2. the panel calls a route the server does not answer (a button that would get a 404);
#   3. the stand-in node the panel is tried against (tools/panel_mock.mjs) does not know a route the panel calls, so a page could look right in the test and break on a node;
#   4. the panel asks for a file (a script, a style sheet, a picture named in panel/index.html or panel/app.js) that the server does not register: the browser gets a 404 and the page comes out
#      unstyled or dead, while a stand-in that serves the folder would never show it.
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# What every node of the family serves, by method. A trailing * is a prefix (users/<name>).
FAMILY = {
    "GET": ["ble-pin", "config", "config/export", "coredump", "coredump/info", "log", "ota/check", "ota/copy", "ota/progress", "session", "status", "users", "wifi/scan"],
    "POST": ["coredump/erase", "factory-reset", "login", "logout", "ota", "ota/check", "ota/copy", "ota/install", "ota/switch", "reboot", "setup", "time", "users"],
    "PUT": ["account", "config", "users/*"],
    "DELETE": ["users/*"],
}


def dispatch(text: str) -> dict[str, set[str]]:
    """The routes api_handler answers, by method: exact ones, and prefixes with a trailing *."""
    start = text.find("esp_err_t api_handler(")
    if start < 0:
        return {}
    end = text.find("\n}\n", start)
    found: dict[str, set[str]] = {}
    method = ""
    for line in text[start:end].splitlines():
        block = re.search(r"method == HTTP_(GET|POST|PUT|DELETE)\)\s*\{", line)
        if block and "route ==" not in line.split("method", 1)[0]:
            method = block.group(1)
            found.setdefault(method, set())
            continue
        if not method:
            continue
        for route in re.findall(r'route == "([^"]+)"', line):
            if f"HTTP_{method} && route" not in line:
                found[method].add(route)
        for prefix in re.findall(r'route\.compare\(0, \d+, "([^"]+)"\)', line):
            found[method].add(prefix + "*")
    return found


def calls(app: str) -> set[tuple[str, str]]:
    """The routes the panel calls with a literal path (a prefix when the path goes on with a variable)."""
    out: set[tuple[str, str]] = set()
    for method, route, more in re.findall(r'api\("(GET|POST|PUT|DELETE)",\s*"([^"]+)"(\s*\+)?', app):
        route = route.split("?", 1)[0]
        out.add((method, route + "*" if more and route.endswith("/") else route))
    return out


def served(routes: dict[str, set[str]], method: str, route: str) -> bool:
    have = routes.get(method, set())
    if route in have:
        return True
    return any(item.endswith("*") and route.startswith(item[:-1]) for item in have)


ASSET = r'[A-Za-z0-9_./-]+\.(?:js|css|png|svg|ico|woff2?)'


def asked_files(index: str, app: str) -> set[str]:
    """The files the browser asks the node for: the ones index.html links and the ones the script adds (href: "x.css", src: "x.js"). Addresses of the API are not files."""
    found = re.findall(r'(?:href|src)="(' + ASSET + r')"', index) + re.findall(r'(?:href|src)\s*[:=]\s*"(' + ASSET + r')"', app)
    return {"/" + name.lstrip("/") for name in found if not name.lstrip("/").startswith("api/")}


def registered_files(server: str) -> set[str]:
    """The paths the server registers for its pages: the list of register_handlers and any handler registered on a path of its own."""
    found = set(re.findall(r'\.uri = "([^"*]+)";', server))
    for listed in re.findall(r"for \(const char\* path : \{([^}]*)\}\)", server):
        found.update(re.findall(r'"([^"]+)"', listed))
    return found


def file_problems(server: str, index: str, app: str) -> list[str]:
    have = registered_files(server)
    problems = [] if "/" in have else ["the server does not register the page /"]
    for path in sorted(asked_files(index, app)):
        if path not in have:
            problems.append(f"the panel asks for {path} and the server does not register it (404)")
        elif path != "/app.js" and f'uri == "{path}"' not in server:
            problems.append(f"{path} is registered and the page handler has no branch for it (it would answer with index.html)")
    return problems


def problems_of(server: str, app: str, mock: str) -> list[str]:
    problems: list[str] = []
    routes = dispatch(server)
    if not routes:
        return ["main/web_server.cpp has no api_handler"]
    for method, wanted in FAMILY.items():
        for route in wanted:
            if not served(routes, method, route):
                problems.append(f"{method} {route}: every node of the family serves it and this server does not")
    for method, route in sorted(calls(app)):
        if not served(routes, method, route):
            problems.append(f"the panel calls {method} {route} and the server has no such route")
        elif mock and f'"{route.rstrip("*")}' not in mock:
            problems.append(f"the stand-in node (tools/panel_mock.mjs) does not know {method} {route}, which the panel calls")
    return problems


def main() -> int:
    def read(relative: str) -> str:
        path = ROOT / relative
        return path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""

    server = read("main/web_server.cpp")
    problems = problems_of(server, read("panel/app.js"), read("tools/panel_mock.mjs")) + file_problems(server, read("panel/index.html"), read("panel/app.js"))
    if problems:
        print("ROUTES=FAIL")
        for problem in problems:
            print("  " + problem)
        return 1
    print("ROUTES=PASS the family's routes are all served, the panel calls only routes that exist, the stand-in node knows them and every file the panel asks for is registered")
    return 0


if __name__ == "__main__":
    sys.exit(main())
