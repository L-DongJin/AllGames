import csv
import json
from collections import Counter
from io import StringIO
from pathlib import Path

import unreal

TABLE_PATH = "/Game/IdolQuiz/Data/DT_IdolQuizQuestionsExpanded"
EXPECTED = {
    "Idol": 258,
    "Actor": 331,
    "OnePiece": 303,
    "Naruto": 64,
    "MC": 40,
    "KBOPlayer": 256,
    "BJ": 112,
}

table = unreal.load_asset(TABLE_PATH)
if table is None:
    raise RuntimeError(f"Missing DataTable: {TABLE_PATH}")
reader = csv.DictReader(StringIO(unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)))
rows = list(reader)
counts = Counter(row.get("PoolTags", "") for row in rows)
if len(rows) != 1364:
    raise RuntimeError(f"Expected 1364 rows, found {len(rows)}")
for pool_tag, expected_count in EXPECTED.items():
    actual = counts.get(pool_tag, 0)
    if actual != expected_count:
        raise RuntimeError(f"Expected {expected_count} {pool_tag} rows, found {actual}")
if any(not row.get("StageName", "").strip() for row in rows):
    raise RuntimeError("Blank StageName found")
if any(not row.get("Image", "").strip() for row in rows):
    raise RuntimeError("Blank Image reference found")

kbo_rows = [row for row in rows if row.get("PoolTags") == "KBOPlayer"]
if any("(" in row["StageName"] or ")" in row["StageName"] for row in kbo_rows):
    raise RuntimeError("KBO canonical answers still contain team parentheses")
if any("(" not in row.get("Aliases", "") for row in kbo_rows):
    raise RuntimeError("KBO full display-name alias is missing")

result = {
    "total_rows": len(rows),
    "pool_counts": dict(sorted(counts.items())),
    "kbo_canonical_answers_without_parentheses": len(kbo_rows),
}
output_path = Path(unreal.Paths.project_saved_dir()) / "Validation" / "additional_person_quiz_validation.json"
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log(f"ADDITIONAL_PERSON_QUIZ_VALIDATION_OK {result}")
