#!/usr/bin/env python3
"""Validate the prospective five-opened-PR feature budget (stdlib CI tooling only).

GitHub live PR state is authoritative: this ledger is dated, and an accepted
pull_request event checks the new PR's feature ID and actual PR number.
"""
import argparse
import json
import re
import sys
from datetime import date
from pathlib import Path

REPO = "Rofikali/Messy-File-Cleaner"
ID = re.compile(r"^[A-Z][A-Z0-9-]{2,64}$")
SHA = re.compile(r"^[0-9a-f]{40}$")
STATES = {"ACTIVE", "PLANNED", "CLOSED", "BLOCKED", "CLOSED_LEGACY", "SAFETY_INTERRUPT"}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate_record(record):
    require(isinstance(record, dict), "Feature ledger must be an object")
    require(record.get("schema_version") == 1 and record.get("repository") == REPO,
            "Unsupported feature ledger schema/repository")
    require(record.get("policy_effective_date") == "2026-11-21", "Invalid policy date")
    require(isinstance(record.get("snapshot_main_sha"), str) and SHA.fullmatch(record["snapshot_main_sha"]),
            "Invalid main SHA")
    require(record.get("max_opened_prs_per_feature") == 5, "Five-PR ceiling cannot be altered")
    features = record.get("features")
    require(isinstance(features, list) and features, "Feature list required")
    seen_ids, seen_prs, active = set(), set(), []
    for feature in features:
        require(isinstance(feature, dict), "Feature entry must be an object")
        fid, state, prs = feature.get("id"), feature.get("status"), feature.get("prs")
        require(isinstance(fid, str) and ID.fullmatch(fid) and fid not in seen_ids,
                "Invalid or duplicate feature ID")
        seen_ids.add(fid)
        require(state in STATES, "Unknown feature state: " + fid)
        require(isinstance(feature.get("title"), str) and feature["title"].strip() and
                isinstance(feature.get("summary"), str) and feature["summary"].strip(),
                "Missing title/summary: " + fid)
        require(isinstance(prs, list), "Invalid feature PR list: " + fid)
        require(len(prs) <= 5, "Five-PR feature budget exceeded: " + fid)
        if state == "CLOSED_LEGACY":
            require(not prs, "Legacy PR numbers must not be recounted as post-policy attempts")
        for number in prs:
            require(type(number) is int and number > 0 and number not in seen_prs,
                    "Duplicate or invalid feature PR number: " + fid)
            seen_prs.add(number)
        if state == "ACTIVE":
            active.append(fid)
    require(len(active) == 1 and record.get("active_feature_id") == active[0],
            "Exactly one matching ACTIVE feature is required")
    return record


def validate_pr(record, event):
    pr = event.get("pull_request")
    if not isinstance(pr, dict):
        return
    number = pr.get("number", event.get("number"))
    require(type(number) is int and number > 0, "PR event lacks valid number")
    body = pr.get("body") or ""
    matches = re.findall(r"(?m)^Feature-ID:\s*([A-Z][A-Z0-9-]{2,64})\s*$", body)
    require(len(matches) == 1, "PR body must have exactly one Feature-ID: ID line")
    fid = matches[0]
    if fid == "GOVERNANCE-EXEMPT":
        require(str(pr.get("title", "")).startswith("docs("),
                "Governance exemption requires a docs(...) title and reviewer verification")
        return
    matches = [f for f in record["features"] if f["id"] == fid]
    require(len(matches) == 1, "Unregistered feature ID in PR: " + fid)
    feature = matches[0]
    require(feature["status"] in ("ACTIVE", "SAFETY_INTERRUPT"),
            "Feature is not eligible for a new PR: " + fid)
    require(fid == record["active_feature_id"] or feature["status"] == "SAFETY_INTERRUPT",
            "PR is not for the active feature or an approved independent safety interruption")
    require(number in feature["prs"],
            "PR number not recorded under feature " + fid + "; add actual PR number and push")
    return


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--event", help="GitHub Actions GITHUB_EVENT_PATH JSON")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    try:
        record = validate_record(json.loads((root / "docs/feature-budget.json").read_text(encoding="utf-8")))
        if args.event:
            validate_pr(record, json.loads(Path(args.event).read_text(encoding="utf-8")))
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        print("Feature budget validation failed: " + str(exc), file=sys.stderr)
        return 1
    print("Five-PR feature budget valid (dated ledger, not live GitHub reconciliation).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
