"""Unit tests for GitHub PR feature-budget enforcement."""
import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from feature_budget_check import validate_record, validate_pr, validate_selector


def fixture():
    return {
        "schema_version": 1,
        "repository": "Rofikali/Messy-File-Cleaner",
        "policy_effective_date": "2026-11-21",
        "snapshot_main_sha": "a" * 40,
        "max_opened_prs_per_feature": 5,
        "active_feature_id": "CLI-DRYRUN-01",
        "features": [
            {"id": "PERF-01", "status": "CLOSED_LEGACY", "title": "Legacy", "summary": "Past work", "prs": []},
            {"id": "CLI-DRYRUN-01", "status": "ACTIVE", "title": "Preview", "summary": "Current", "prs": [130]},
        ],
    }


def event(feature_id="CLI-DRYRUN-01", number=130, title="feat(cli): preview evidence"):
    return {"number": number, "pull_request": {
        "number": number, "title": title, "body": "## Scope\nFeature-ID: " + feature_id + "\n"}}


class BudgetTests(unittest.TestCase):
    def test_five_is_ceiling(self):
        data = fixture()
        data["features"][1]["prs"] = [1, 2, 3, 4, 5]
        validate_record(data)
        data["features"][1]["prs"].append(6)
        with self.assertRaisesRegex(ValueError, "Five-PR feature budget exceeded"):
            validate_record(data)

    def test_duplicate_pr_is_rejected(self):
        data = fixture()
        data["features"][0]["status"] = "CLOSED"
        data["features"][0]["prs"] = [130]
        with self.assertRaisesRegex(ValueError, "Duplicate or invalid"):
            validate_record(data)

    def test_only_one_active_feature(self):
        data = fixture()
        data["features"][0]["status"] = "ACTIVE"
        with self.assertRaisesRegex(ValueError, "Exactly one matching"):
            validate_record(data)

    def test_terminal_feature_states_are_accepted_but_cannot_reopen(self):
        for state in ("DONE", "PARTIAL", "BLOCKED"):
            with self.subTest(state=state):
                data = fixture()
                data["features"].append({
                    "id": "SCAN-ROBUST-01", "status": "ACTIVE",
                    "title": "Scanner", "summary": "Current", "prs": [131],
                })
                data["features"][1]["status"] = state
                data["active_feature_id"] = "SCAN-ROBUST-01"
                validate_pr(validate_record(data), event("SCAN-ROBUST-01", 131))
                with self.assertRaisesRegex(ValueError, "not eligible"):
                    validate_pr(data, event("CLI-DRYRUN-01", 130))

    def test_pr_requires_real_ledger_number(self):
        data = fixture()
        validate_pr(validate_record(data), event())
        with self.assertRaisesRegex(ValueError, "PR number not recorded"):
            validate_pr(data, event(number=131))

    def test_closed_legacy_cannot_be_restarted(self):
        data = fixture()
        with self.assertRaisesRegex(ValueError, "not eligible"):
            validate_pr(validate_record(data), event(feature_id="PERF-01"))

    def test_missing_or_duplicate_metadata_rejected(self):
        data = fixture()
        with self.assertRaisesRegex(ValueError, "exactly one"):
            validate_pr(data, {"number": 130, "pull_request": {"number": 130, "body": ""}})
        with self.assertRaisesRegex(ValueError, "exactly one"):
            validate_pr(data, {"number": 130, "pull_request": {"number": 130,
                "body": "Feature-ID: CLI-DRYRUN-01\nFeature-ID: CLI-DRYRUN-01"}})

    def test_governance_exemption_requires_docs_title(self):
        data = fixture()
        validate_pr(data, event("GOVERNANCE-EXEMPT", number=999, title="docs(policy): update"))
        with self.assertRaisesRegex(ValueError, "docs"):
            validate_pr(data, event("GOVERNANCE-EXEMPT", number=999, title="feat: sneak around budget"))

    def test_live_now_selector_agrees_with_ledger(self):
        data = validate_record(fixture())
        board = """# Sample
## LIVE NOW / NEXT / LATER

| Lane | ID / budget / status | Description |
| --- | --- | --- |
| **NOW — IMPLEMENTATION PR** | **CLI-DRYRUN-01 (1/5)** | Safe preview |
| **DONE** | **PERF-01 (legacy)** | Historical |
## Historical
"""
        validate_selector(data, board)

    def test_live_now_selector_rejects_stale_or_duplicate_rows(self):
        data = validate_record(fixture())
        prefix = "## LIVE NOW / NEXT / LATER\n| Lane | ID |\n| --- | --- |\n"
        correct = "| **NOW — IMPLEMENTATION** | **CLI-DRYRUN-01 (1/5)** |\n"
        stale = "| **NOW — IMPLEMENTATION** | **SCAN-ROBUST-01 (1/5)** |\n"
        for board, diagnostic in (
            (prefix + stale, "differs"),
            (prefix + correct + stale, "Exactly one NOW row"),
            (prefix, "Exactly one NOW row"),
            ("## unrelated\n" + correct, "Exactly one LIVE"),
        ):
            with self.subTest(diagnostic=diagnostic):
                with self.assertRaisesRegex(ValueError, diagnostic):
                    validate_selector(data, board)


if __name__ == "__main__":
    unittest.main()
