# Renewing the screenshots of the README

The pictures of the README (`images/screenshots/`, listed in `docs/screenshots.json`) can be taken again on a GitHub runner, so a new version does not need a run on your own computer.

1. Start the workflow **Screenshots (manual)** from the Actions tab, or from a terminal: `gh workflow run screenshots.yml --ref main` (any branch works: `--ref <branch>`). It only starts by hand; it never runs on a push or on a pull request, it has no secrets and the token is read-only.
2. It fetches Studio (the script takes Playwright from `ARMOR-STUDIO/e2e`), starts the stand-in of the node (`tools/panel_mock.mjs`, made-up data) and runs `tools/capture_screenshots.py`, which takes the pages of the web panel that the README shows with the Chromium of Playwright.
3. When it ends, download the artifact **screenshots** (kept for 14 days): `gh run download <run id> --name screenshots`. The job summary lists every file and its size.
4. Look at the pictures and copy the ones you want into `images/screenshots/`. The workflow never changes the repository.

The runner has no Microsoft Edge, so fonts and edges can differ a little from the pictures taken on Windows with `py -3 tools/capture_screenshots.py` (the same script, which takes Edge by default; `ARMOR_SHOTS_CHANNEL=chromium` takes the Chromium of Playwright).
