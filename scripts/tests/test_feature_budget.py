"""Unit tests for GitHub PR feature-budget enforcement."""
import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from feature_budget_check import validate_record, validate_pr


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
        "number": number, "title": title, "body": "## Scope\\nFeature-ID: " + feature_id + "\\n"}}


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
                "body": "Feature-ID: CLI-DRYRUN-01\\nFeature-ID: CLI-DRYRUN-01"}})

    def test_governance_exemption_requires_docs_title(self):
        data = fixture()
        validate_pr(data, event("GOVERNANCE-EXEMPT", number=999, title="docs(policy): update"))
        with self.assertRaisesRegex(ValueError, "docs"):
            validate_pr(data, event("GOVERNANCE-EXEMPT", number=999, title="feat: sneak around budget"))


if __name__ == "__main__":
    unittest.main()
