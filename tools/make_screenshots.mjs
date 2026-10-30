#!/usr/bin/env node
/**
 * Takes the screenshots of the README: the web panel of the node in a real browser (headless Microsoft Edge) against the stand-in node of tools/panel_mock.mjs,
 * which answers with made-up sample data. Nothing real is behind them.
 * Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
 *
 *   node tools/make_screenshots.mjs              writes images/screenshots/<name>.png at 1920x1080
 *   node tools/make_screenshots.mjs --out DIR    writes them in DIR instead (to look at them before they replace the ones in the repository)
 *
 * It needs Edge (the one Windows has) and Playwright, which it takes from the Studio checkout next to this one (ARMOR-STUDIO/e2e/node_modules).
 * It takes no mouse or keyboard of the desktop: the browser has no window.
 */
import { spawn } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, "..");
const name = path.basename(repo);
const ROOT = path.resolve(repo, "..");
const require = createRequire(path.join(ROOT, "ARMOR-STUDIO", "e2e", "package.json"));
const { chromium } = require("@playwright/test");

// the pages of each node that the README shows, in the order of its Screenshots section: [file name, page of the panel]
const PAGES = {
  "ARMOR-NODE-RADAR": { board: "s3-eth", shots: [["overview", "overview"], ["radars", "radars"], ["pins", "pins"], ["network", "network"]] },
  "ARMOR-NODE-SOLAR": { board: "s3-wifi", shots: [["overview", "overview"], ["ports", "ports"], ["readings", "readings"], ["network", "network"]] },
  "ARMOR-NODE-ELECTRICAL": { board: "s3-wifi", shots: [["overview", "overview"], ["meters", "meters"], ["readings", "readings"], ["zigbee", "zigbee"]] },
  "ARMOR-NODE-ALARM": { board: "s3-wifi", shots: [["overview", "overview"], ["zones", "zones"], ["zigbee", "zigbee"], ["inputs-outputs", "io"]] },
  "ARMOR-NODE-HMI": { board: "s3-wifi", shots: [["overview", "overview"], ["screen", "screen"], ["voice", "voice"], ["server", "server"]] },
};
const plan = PAGES[name];
if (!plan) throw new Error(`${name} is not one of the nodes this script knows`);
const outIndex = process.argv.indexOf("--out");
const out = path.resolve(outIndex > 0 ? process.argv[outIndex + 1] : path.join(repo, "images", "screenshots"));
fs.mkdirSync(out, { recursive: true });

const port = 18200 + Math.floor(Math.random() * 500);
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
const mock = spawn(process.execPath, [path.join(here, "panel_mock.mjs"), String(port), "--user", "admin:adminpass123", "--board", plan.board], { cwd: repo, stdio: "ignore" });
const profile = fs.mkdtempSync(path.join(os.tmpdir(), "armor-shots-"));
let browser;
try {
  const base = `http://127.0.0.1:${port}`;
  for (let tries = 0; tries < 40; tries += 1) { try { await fetch(base + "/"); break; } catch { await sleep(250); } }
  const login = await fetch(base + "/api/v1/login", { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ user: "admin", password: "adminpass123" }) });
  const pair = login.headers.getSetCookie()[0].split(";")[0];
  browser = await chromium.launch({ channel: "msedge", headless: true });
  const context = await browser.newContext({ viewport: { width: 1920, height: 1080 }, deviceScaleFactor: 1 });
  await context.addCookies([{ name: pair.slice(0, pair.indexOf("=")), value: pair.slice(pair.indexOf("=") + 1), url: base + "/" }]);
  // the stand-in answers with the addresses of a real bench (the server and the node): the pictures show other, made-up ones
  const MADE_UP = [["192.168.0.180", "192.168.0.10"], ["192.168.0.181", "192.168.0.50"]];
  await context.route("**/api/**", async route => {
    const response = await route.fetch();
    let body = await response.text();
    for (const [real, made] of MADE_UP) body = body.split(real).join(made);
    await route.fulfill({ response, body });
  });
  const page = await context.newPage();
  await page.goto(base + "/");
  await page.waitForTimeout(1800);
  for (const [file, route] of plan.shots) {
    await page.evaluate(hash => { location.hash = "#/" + hash; }, route);
    await page.waitForTimeout(1200);
    await page.screenshot({ path: path.join(out, file + ".png") });
    console.log("wrote", path.join(out, file + ".png"));
  }
} finally {
  await browser?.close();
  mock.kill();
  fs.rmSync(profile, { recursive: true, force: true });
}
