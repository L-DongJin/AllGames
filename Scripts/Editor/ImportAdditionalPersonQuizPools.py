import csv
import json
import re
from io import StringIO
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())
SOURCE_BASE = PROJECT_ROOT / "SourceAssets" / "IdolQuiz"
TABLE_PATH = "/Game/IdolQuiz/Data/DT_IdolQuizQuestionsExpanded"
IMPORT_BATCH_SIZE = 32
EXPECTED_EXISTING_ROWS = 892

DATASETS = (
    {
        "folder": "Naruto",
        "prefix": "NARUTO",
        "pool_tag": "Naruto",
        "group_name": "Naruto",
        "asset_folder": "/Game/IdolQuiz/Images/Naruto",
        "expected": 64,
    },
    {
        "folder": "MC",
        "prefix": "MC",
        "pool_tag": "MC",
        "group_name": "MC",
        "asset_folder": "/Game/IdolQuiz/Images/MC",
        "expected": 64,
        "active_expected": 40,
    },
    {
        "folder": "KBO2025",
        "prefix": "KBO",
        "pool_tag": "KBOPlayer",
        "group_name": "KBO2025",
        "asset_folder": "/Game/IdolQuiz/Images/KBO2025",
        "expected": 256,
    },
)


def load_records(dataset):
    source_root = SOURCE_BASE / dataset["folder"]
    manifest_path = source_root / "collection_manifest.json"
    with manifest_path.open(encoding="utf-8-sig") as handle:
        manifest = json.load(handle)
    records = manifest["records"]
    if len(records) != dataset["expected"]:
        raise RuntimeError(
            f"Expected {dataset['expected']} {dataset['folder']} records, found {len(records)}"
        )
    excluded_names = set()
    exclusion_path = source_root / "excluded_names.json"
    if exclusion_path.is_file():
        with exclusion_path.open(encoding="utf-8-sig") as handle:
            excluded_names = set(json.load(handle))
    manifest_names = {record["answer"] for record in records}
    unknown_exclusions = excluded_names - manifest_names
    if unknown_exclusions:
        raise RuntimeError(
            f"Unknown exclusions in {exclusion_path}: {sorted(unknown_exclusions)}"
        )

    for record in records:
        record["excluded"] = record["answer"] in excluded_names
        if record["excluded"]:
            continue
        if record.get("status") != "downloaded":
            raise RuntimeError(f"Incomplete source record: {dataset['folder']} {record}")
        image_path = source_root / "Images" / record["saved_filename"]
        if not image_path.is_file() or image_path.stat().st_size <= 0:
            raise RuntimeError(f"Missing source image: {image_path}")
        record["source_image_path"] = image_path
    active_expected = dataset.get("active_expected", dataset["expected"])
    active_count = sum(not record["excluded"] for record in records)
    if active_count != active_expected:
        raise RuntimeError(
            f"Expected {active_expected} active {dataset['folder']} records, "
            f"found {active_count}"
        )
    return records


def canonical_answer(dataset, record):
    answer = re.sub(r"\s+", " ", record["answer"]).strip()
    if dataset["pool_tag"] == "Naruto":
        answer = answer.split(" | ", 1)[0].strip()
    elif dataset["pool_tag"] == "KBOPlayer":
        answer = re.sub(r"\s*\([^()]*\)\s*$", "", answer).strip()
    if not answer:
        raise RuntimeError(f"Empty canonical answer for {record}")
    return answer


def accepted_aliases(dataset, record, answer):
    aliases = []
    original = re.sub(r"\s+", " ", record["answer"]).strip()
    if dataset["pool_tag"] == "KBOPlayer" and original != answer:
        aliases.append(original)
    return "|".join(dict.fromkeys(aliases))


def prepare_assets(dataset, records):
    source_root = SOURCE_BASE / dataset["folder"] / "Images"
    for record in records:
        index = int(record["index"])
        asset_name = f"T_{dataset['prefix']}_{index:03d}"
        record["asset_name"] = asset_name
        record["asset_path"] = f"{dataset['asset_folder']}/{asset_name}"

    active_records = [record for record in records if not record.get("excluded", False)]
    pending = [
        record for record in active_records
        if not unreal.EditorAssetLibrary.does_asset_exist(record["asset_path"])
    ]
    for batch_start in range(0, len(pending), IMPORT_BATCH_SIZE):
        batch = pending[batch_start:batch_start + IMPORT_BATCH_SIZE]
        tasks = []
        for record in batch:
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", str(source_root / record["saved_filename"]))
            task.set_editor_property("destination_path", dataset["asset_folder"])
            task.set_editor_property("destination_name", record["asset_name"])
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", False)
            task.set_editor_property("save", True)
            tasks.append(task)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        unreal.log(
            f"PERSON_QUIZ_IMPORT_CHECKPOINT pool={dataset['pool_tag']} "
            f"completed={min(batch_start + len(batch), len(pending))}/{len(pending)}"
        )

    missing = [
        record["asset_path"] for record in active_records
        if not unreal.EditorAssetLibrary.does_asset_exist(record["asset_path"])
    ]
    if missing:
        raise RuntimeError(f"Texture import failed for {dataset['pool_tag']}: {missing[:10]}")


def build_rows(dataset, records, fieldnames):
    rows = []
    for record in records:
        if record.get("excluded", False):
            continue
        index = int(record["index"])
        answer = canonical_answer(dataset, record)
        asset_name = record["asset_name"]
        rows.append({
            fieldnames[0]: f"{dataset['prefix']}_{index:03d}",
            "QuestionId": f"{dataset['prefix']}_{index:03d}",
            "Image": f"Texture2D'{record['asset_path']}.{asset_name}'",
            "StageName": answer,
            "RealName": "",
            "Aliases": accepted_aliases(dataset, record, answer),
            "GroupName": dataset["group_name"],
            "Generation": "0",
            "Category": dataset["pool_tag"],
            "PoolTags": dataset["pool_tag"],
            "bEnabled": "True",
        })
    return rows


def update_data_table(dataset_records):
    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        raise RuntimeError(f"Missing existing DataTable: {TABLE_PATH}")

    existing_csv = unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
    reader = csv.DictReader(StringIO(existing_csv))
    fieldnames = reader.fieldnames
    if not fieldnames or "PoolTags" not in fieldnames:
        raise RuntimeError("Expanded DataTable is missing required headers")

    replacing_tags = {dataset["pool_tag"].casefold() for dataset in DATASETS}
    existing_rows = []
    for row in reader:
        row_tags = {
            tag.strip().casefold()
            for tag in (row.get("PoolTags") or row.get("Category") or "").split("|")
            if tag.strip()
        }
        if row_tags & replacing_tags:
            continue
        if not row.get("PoolTags", "").strip():
            row["PoolTags"] = row.get("Category", "Idol") or "Idol"
        existing_rows.append(row)

    if len(existing_rows) != EXPECTED_EXISTING_ROWS:
        raise RuntimeError(
            f"Expected {EXPECTED_EXISTING_ROWS} existing Idol/Actor/OnePiece rows, "
            f"found {len(existing_rows)}"
        )

    added_rows = []
    for dataset, records in dataset_records:
        pool_rows = build_rows(dataset, records, fieldnames)
        added_rows.extend(pool_rows)
        csv_path = SOURCE_BASE / dataset["folder"] / f"{dataset['folder']}Questions.csv"
        with csv_path.open("w", newline="", encoding="utf-8-sig") as handle:
            writer = csv.DictWriter(handle, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(pool_rows)

    all_rows = existing_rows + added_rows
    expected_total = EXPECTED_EXISTING_ROWS + sum(
        dataset.get("active_expected", dataset["expected"])
        for dataset in DATASETS
    )
    if len(all_rows) != expected_total:
        raise RuntimeError(f"Expected {expected_total} total rows, built {len(all_rows)}")

    output = StringIO()
    writer = csv.DictWriter(output, fieldnames=fieldnames, lineterminator="\n")
    writer.writeheader()
    writer.writerows(all_rows)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, output.getvalue()):
        raise RuntimeError("Failed to update expanded Person Quiz DataTable")
    unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)

    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    if len(row_names) != expected_total:
        raise RuntimeError(f"Expected {expected_total} DataTable rows, found {len(row_names)}")
    return expected_total


dataset_records = []
for dataset in DATASETS:
    records = load_records(dataset)
    prepare_assets(dataset, records)
    dataset_records.append((dataset, records))

total_rows = update_data_table(dataset_records)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(
    "ADDITIONAL_PERSON_QUIZ_IMPORT_COMPLETE "
    f"naruto=64 mc=40 kbo=256 total_rows={total_rows}"
)
