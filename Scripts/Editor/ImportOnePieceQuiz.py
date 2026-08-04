import csv
import itertools
import json
from io import StringIO
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())
SOURCE_ROOT = PROJECT_ROOT / "SourceAssets" / "IdolQuiz" / "OnePiece"
MANIFEST_PATH = SOURCE_ROOT / "collection_manifest.json"
CSV_PATH = SOURCE_ROOT / "OnePieceQuestions.csv"
IMAGE_SOURCE_ROOT = SOURCE_ROOT / "Images"
IMAGE_DESTINATION = "/Game/IdolQuiz/Images/OnePiece"
TABLE_PATH = "/Game/IdolQuiz/Data/DT_IdolQuizQuestionsExpanded"


def build_aliases(answer):
    choices = []
    for token in answer.split():
        variants = [token]
        if token == "D":
            variants.append("디")
        elif token == "Dr":
            variants.append("닥터")
        elif token == "T":
            variants.append("티")
        elif token == "X":
            variants.append("엑스")
        elif token == "&":
            variants.extend(["엔", "앤"])
        elif token.startswith("T") and len(token) > 1:
            variants.append("티" + token[1:])
        choices.append(variants)

    aliases = {" ".join(parts) for parts in itertools.product(*choices)}
    if answer.startswith("S") and len(answer) > 1:
        aliases.add(answer[1:].lstrip())
    aliases.discard(answer)
    return "|".join(sorted(aliases))


def import_textures(records):
    tasks = []
    for index, record in enumerate(records, 1):
        asset_name = f"T_ONEPIECE_{index:03d}"
        record["asset_name"] = asset_name
        record["asset_path"] = f"{IMAGE_DESTINATION}/{asset_name}"
        if unreal.EditorAssetLibrary.does_asset_exist(record["asset_path"]):
            continue
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(IMAGE_SOURCE_ROOT / record["saved_filename"]))
        task.set_editor_property("destination_path", IMAGE_DESTINATION)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", True)
        tasks.append(task)
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    missing = [r["asset_path"] for r in records if not unreal.EditorAssetLibrary.does_asset_exist(r["asset_path"])]
    if missing:
        raise RuntimeError(f"One Piece texture import failed: {missing[:10]}")


def update_data_table(records):
    table = unreal.load_asset(TABLE_PATH)
    if table is None:
        raise RuntimeError(f"Missing existing DataTable: {TABLE_PATH}")

    existing_csv = unreal.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
    reader = csv.DictReader(StringIO(existing_csv))
    fieldnames = reader.fieldnames
    rows = [row for row in reader if row.get("Category", "").casefold() != "onepiece"]
    if not fieldnames:
        raise RuntimeError("Expanded DataTable CSV export has no header")
    if "PoolTags" not in fieldnames:
        raise RuntimeError("Expanded DataTable is missing the PoolTags column; rebuild C++ first")
    for row in rows:
        if not row.get("PoolTags", "").strip():
            row["PoolTags"] = row.get("Category", "Idol") or "Idol"

    one_piece_rows = []
    for index, record in enumerate(records, 1):
        asset_name = record["asset_name"]
        one_piece_rows.append({
            fieldnames[0]: f"ONEPIECE_{index:03d}",
            "QuestionId": f"ONEPIECE_{index:03d}",
            "Image": f"Texture2D'{record['asset_path']}.{asset_name}'",
            "StageName": record["answer"],
            "RealName": "",
            "Aliases": build_aliases(record["answer"]),
            "GroupName": "OnePiece",
            "Generation": "0",
            "Category": "OnePiece",
            "PoolTags": "OnePiece",
            "bEnabled": "True",
        })
    rows.extend(one_piece_rows)

    output = StringIO()
    writer = csv.DictWriter(output, fieldnames=fieldnames, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, output.getvalue()):
        raise RuntimeError("Failed to update expanded Idol Quiz DataTable")
    unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
    row_count = len(unreal.DataTableFunctionLibrary.get_data_table_row_names(table))
    if row_count != 892:
        raise RuntimeError(f"Expected 892 expanded rows after One Piece import, found {row_count}")

    with CSV_PATH.open("w", newline="", encoding="utf-8-sig") as handle:
        csv_writer = csv.DictWriter(handle, fieldnames=fieldnames)
        csv_writer.writeheader()
        csv_writer.writerows(one_piece_rows)


with MANIFEST_PATH.open(encoding="utf-8-sig") as handle:
    manifest = json.load(handle)
records = manifest["records"]
if len(records) != 303:
    raise RuntimeError(f"Expected 303 One Piece records, found {len(records)}")
import_textures(records)
update_data_table(records)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(f"ONE_PIECE_QUIZ_IMPORT_COMPLETE count={len(records)} csv={CSV_PATH}")
