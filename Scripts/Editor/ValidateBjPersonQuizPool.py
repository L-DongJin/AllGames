import csv
import json
from collections import Counter
from io import StringIO
from pathlib import Path

import unreal


TABLE_PATH = "/Game/IdolQuiz/Data/DT_IdolQuizQuestionsExpanded"
EXPECTED_TOTAL = 1364
EXPECTED_BJ = 112

table = unreal.load_asset(TABLE_PATH)
if table is None:
    raise RuntimeError(f"Missing DataTable: {TABLE_PATH}")

reader = csv.DictReader(StringIO(
    unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
))
rows = list(reader)
counts = Counter(row.get("PoolTags", "") for row in rows)
bj_rows = [row for row in rows if row.get("PoolTags") == "BJ"]
if len(rows) != EXPECTED_TOTAL:
    raise RuntimeError(f"Expected {EXPECTED_TOTAL} rows, found {len(rows)}")
if len(bj_rows) != EXPECTED_BJ:
    raise RuntimeError(f"Expected {EXPECTED_BJ} BJ rows, found {len(bj_rows)}")
if len({row["StageName"] for row in bj_rows}) != EXPECTED_BJ:
    raise RuntimeError("Duplicate BJ answers found")
if any(not row.get("StageName", "").strip() for row in bj_rows):
    raise RuntimeError("Blank BJ answer found")
if any(not row.get("Image", "").strip() for row in bj_rows):
    raise RuntimeError("Blank BJ image reference found")

missing_assets = []
for row in bj_rows:
    object_path = row["Image"].split("'", 1)[-1].rstrip("'")
    package_path = object_path.rsplit(".", 1)[0]
    if not unreal.EditorAssetLibrary.does_asset_exist(package_path):
        missing_assets.append(package_path)
if missing_assets:
    raise RuntimeError(f"Missing BJ texture assets: {missing_assets[:10]}")

result = {
    "total_rows": len(rows),
    "bj_rows": len(bj_rows),
    "unique_bj_answers": len({row["StageName"] for row in bj_rows}),
    "pool_counts": dict(sorted(counts.items())),
}
output_path = (
    Path(unreal.Paths.project_saved_dir())
    / "Validation"
    / "bj_person_quiz_validation.json"
)
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text(
    json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8"
)
unreal.log(f"BJ_PERSON_QUIZ_VALIDATION_OK {result}")
