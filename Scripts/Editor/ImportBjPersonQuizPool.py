import csv
import json
from io import StringIO
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())
SOURCE_ROOT = PROJECT_ROOT / "SourceAssets" / "IdolQuiz"
SOURCE_FOLDERS = (
    SOURCE_ROOT / "AfreecaBJ" / "Images",
    SOURCE_ROOT / "AfreecaMaleBJ_6SVxLb" / "Images",
)
GENERATED_ROOT = SOURCE_ROOT / "BJ"
TABLE_PATH = "/Game/IdolQuiz/Data/DT_IdolQuizQuestionsExpanded"
ASSET_FOLDER = "/Game/IdolQuiz/Images/BJ"
IMPORT_BATCH_SIZE = 1
EXPECTED_EXISTING_ROWS = 1252


def collect_sources():
    sources = []
    for folder in SOURCE_FOLDERS:
        if not folder.is_dir():
            raise RuntimeError(f"Missing BJ source folder: {folder}")
        for image_path in folder.glob("*.png"):
            if image_path.stat().st_size <= 0:
                raise RuntimeError(f"Empty BJ source image: {image_path}")
            answer = image_path.stem.strip()
            if not answer:
                raise RuntimeError(f"Empty BJ answer for {image_path}")
            sources.append({"answer": answer, "source_path": image_path})

    sources.sort(key=lambda item: (item["answer"].casefold(), str(item["source_path"])))
    duplicate_answers = sorted(
        answer for answer in {item["answer"] for item in sources}
        if sum(item["answer"] == answer for item in sources) > 1
    )
    if duplicate_answers:
        raise RuntimeError(f"Duplicate BJ answers: {duplicate_answers}")
    if len(sources) != 112:
        raise RuntimeError(f"Expected 112 current BJ PNG files, found {len(sources)}")

    for index, item in enumerate(sources, start=1):
        item["index"] = index
        item["asset_name"] = f"T_BJ_{index:03d}"
        item["asset_path"] = f"{ASSET_FOLDER}/{item['asset_name']}"
        item["question_id"] = f"BJ_{index:03d}"
    return sources


def import_textures(sources):
    pending = [
        item for item in sources
        if not unreal.EditorAssetLibrary.does_asset_exist(item["asset_path"])
    ]
    for batch_start in range(0, len(pending), IMPORT_BATCH_SIZE):
        batch = pending[batch_start:batch_start + IMPORT_BATCH_SIZE]
        tasks = []
        for item in batch:
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", str(item["source_path"]))
            task.set_editor_property("destination_path", ASSET_FOLDER)
            task.set_editor_property("destination_name", item["asset_name"])
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", False)
            task.set_editor_property("save", True)
            tasks.append(task)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        unreal.log(
            "BJ_IMPORT_CHECKPOINT "
            f"completed={min(batch_start + len(batch), len(pending))}/{len(pending)}"
        )

    missing = [
        item["asset_path"] for item in sources
        if not unreal.EditorAssetLibrary.does_asset_exist(item["asset_path"])
    ]
    if missing:
        raise RuntimeError(f"BJ texture import failed: {missing[:10]}")


def update_table(sources):
    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        raise RuntimeError(f"Missing Person Quiz DataTable: {TABLE_PATH}")

    reader = csv.DictReader(StringIO(
        unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
    ))
    fieldnames = reader.fieldnames
    if not fieldnames or "PoolTags" not in fieldnames:
        raise RuntimeError("Expanded DataTable is missing PoolTags")

    existing_rows = []
    for row in reader:
        tags = {
            tag.strip().casefold()
            for tag in (row.get("PoolTags") or row.get("Category") or "").split("|")
            if tag.strip()
        }
        if "bj" not in tags:
            existing_rows.append(row)
    if len(existing_rows) != EXPECTED_EXISTING_ROWS:
        raise RuntimeError(
            f"Expected {EXPECTED_EXISTING_ROWS} non-BJ rows, found {len(existing_rows)}"
        )

    bj_rows = []
    for item in sources:
        bj_rows.append({
            fieldnames[0]: item["question_id"],
            "QuestionId": item["question_id"],
            "Image": f"Texture2D'{item['asset_path']}.{item['asset_name']}'",
            "StageName": item["answer"],
            "RealName": "",
            "Aliases": "",
            "GroupName": "BJ",
            "Generation": "0",
            "Category": "BJ",
            "PoolTags": "BJ",
            "bEnabled": "True",
        })

    GENERATED_ROOT.mkdir(parents=True, exist_ok=True)
    csv_path = GENERATED_ROOT / "BJQuestions.csv"
    with csv_path.open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(bj_rows)

    manifest = {
        "pool_tag": "BJ",
        "source_folders": [str(folder.relative_to(PROJECT_ROOT)) for folder in SOURCE_FOLDERS],
        "question_count": len(sources),
        "records": [
            {
                "question_id": item["question_id"],
                "answer": item["answer"],
                "source_path": str(item["source_path"].relative_to(PROJECT_ROOT)),
                "asset_path": item["asset_path"],
            }
            for item in sources
        ],
    }
    (GENERATED_ROOT / "import_manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    output = StringIO()
    writer = csv.DictWriter(output, fieldnames=fieldnames, lineterminator="\n")
    writer.writeheader()
    writer.writerows(existing_rows + bj_rows)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(
        table, output.getvalue()
    ):
        raise RuntimeError("Failed to add BJ rows to Person Quiz DataTable")
    unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)

    expected_total = EXPECTED_EXISTING_ROWS + len(bj_rows)
    actual_total = len(unreal.DataTableFunctionLibrary.get_data_table_row_names(table))
    if actual_total != expected_total:
        raise RuntimeError(f"Expected {expected_total} rows, found {actual_total}")
    return actual_total


sources = collect_sources()
import_textures(sources)
total = update_table(sources)
unreal.log(f"BJ_PERSON_QUIZ_IMPORT_COMPLETE bj={len(sources)} total_rows={total}")
