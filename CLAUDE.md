---
description: Implement a feature, sync docs + website, and open a PR
argument-hint: <feature description>
---

You are shipping a feature end-to-end in the Major Midi monorepo. Feature
request: $ARGUMENTS

Follow these steps in order. Do not skip steps. Do not push to `main`.

## Non-negotiable output shape
This command produces **exactly one branch, one commit (default — only
split if truly necessary), and one PR** covering firmware + docs + site
together. Never open more than one PR. The docs-updater and site-updater
subagents only have Read/Edit/Write/Grep/Glob — they never run git or gh.
Committing and PR creation happen once, here, in the main agent, after
everything else is done. Don't commit firmware and docs separately "to keep
history clean" — stage and commit once at the end so the PR reflects one
atomic change.

## 1. Branch first
Create the branch before touching any files, so every edit below — firmware,
docs, site — happens on it, not on `main`:
`git checkout -b feature/<short-slug-from-arguments>`

## 2. Plan
Read enough of `firmware/` to understand where this feature belongs. State a
short plan (files to touch, any panel/CV/MIDI behavior changes) before
writing code.

## 3. Implement
Implement the feature in `firmware/`. Follow existing code style. Build the
firmware using the command in CLAUDE.md and fix any build errors before
moving on. Do not proceed to step 4 with a broken build.

## 4. Update docs
Use the `docs-updater` subagent (Task tool) to update everything under
`site/` that's affected by this change — new params, CV/MIDI mappings,
calibration notes, changelog entry, etc. Give it the diff/summary of what you
implemented so it doesn't have to rediscover it. It edits files only; it
does not commit.

## 5. Update website
Use the `site-updater` subagent (Task tool) to update the static site under
`docs/` — changelog/news entry, feature/manual page updates. Give it the same
summary. Build the site afterward using the command in CLAUDE.md and fix any
build errors. It edits files only; it does not commit.

## 6. Verify sync
Run `.claude/hooks/check-docs-sync.sh` (or equivalent) against the diff to
confirm firmware changes have matching docs/site changes. If it flags
anything, go back and fix it before continuing.

## 7. One commit, one push
- Confirm you're still on the `feature/<slug>` branch (`git branch
  --show-current`), not `main`.
- Stage everything in one go: `git add -A`
- Make a single commit with a conventional commit message summarizing the
  feature (`feat:` prefix). Include a body listing firmware, docs, and site
  changes so the commit itself documents the full scope.
- Push once: `git push -u origin HEAD`

## 8. One PR
Open exactly one PR with `gh pr create`:
- Title: concise feature summary
- Body with three sections: **Firmware changes**, **Docs updated** (list
  file paths), **Website updated** (list file paths). Note the build
  commands you ran and that they passed.
- Before calling `gh pr create`, check there isn't already an open PR from
  this branch (`gh pr view`) — if there is, stop and fix it rather than
  opening a second PR.

## 9. Report back
Summarize what shipped: branch name, PR URL, and the list of docs/site files
touched. For follow-up fixes to this same feature, push additional commits
to the same branch — do not open a second PR.