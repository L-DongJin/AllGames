import csv
import hashlib
from pathlib import Path

import unreal


SOURCE_ROOT = Path(r"C:\Users\user\Downloads\인물보고이름맞히기")
CONTENT_ROOT = "/Game/IdolQuiz/Images"
DATA_PATH = "/Game/IdolQuiz/Data"
TABLE_NAME = "DT_IdolQuizQuestionsExpanded"
MANIFEST_PATH = Path(unreal.Paths.project_saved_dir()) / "IdolQuizImportManifest.csv"
SUPPORTED_EXTENSIONS = {".jpg", ".jpeg", ".png", ".webp"}
START_INDICES = {"Generation2": 1, "Generation3": 84, "Generation4": 14, "Actors": 1}
PREFIXES = {"Generation2": "T_IDOL2", "Generation3": "T_IDOL3", "Generation4": "T_IDOL4", "Actors": "T_ACTOR"}
GROUP_PREFIX_ALIASES = {"TripleS": "트리플에스", "키오프": "키스오브라이프"}


def source_metadata(path: Path):
    relative = path.relative_to(SOURCE_ROOT)
    parts = relative.parts
    if parts[0] == "배우":
        return "Actors", "Actor", "", "", path.stem.strip()
    if parts[0] != "아이돌" or len(parts) < 4:
        return None
    generation_text = parts[1]
    generation_number = generation_text.replace("세대", "").strip()
    destination = f"Generation{generation_number}"
    if destination not in PREFIXES:
        return None
    group = parts[-2].strip()
    stage_name = path.stem.strip()
    displayed_group = GROUP_PREFIX_ALIASES.get(group, group)
    for separator in (" ", "_", "-"):
        prefix = displayed_group + separator
        if stage_name.casefold().startswith(prefix.casefold()):
            stage_name = stage_name[len(prefix):].strip()
            break
    return destination, "Idol", generation_number, group, stage_name


def collect_unique_sources():
    seen_hashes = set()
    seen_people = set()
    records = []
    for path in sorted(SOURCE_ROOT.rglob("*"), key=lambda item: str(item).casefold()):
        if not path.is_file() or path.suffix.lower() not in SUPPORTED_EXTENSIONS:
            continue
        metadata = source_metadata(path)
        if metadata is None:
            unreal.log_warning(f"Skipped unsupported quiz source path: {path}")
            continue
        digest = hashlib.sha1(path.read_bytes()).hexdigest()
        destination, category, generation, group, stage_name = metadata
        person_key = (category.casefold(), group.casefold(), stage_name.casefold())
        if digest in seen_hashes or person_key in seen_people:
            unreal.log(f"Skipped duplicate quiz image: {path}")
            continue
        seen_hashes.add(digest)
        seen_people.add(person_key)
        records.append({
            "source": path,
            "destination": destination,
            "category": category,
            "generation": generation,
            "group": group,
            "stage_name": stage_name,
            "sha1": digest,
        })
    return records


def import_textures(records):
    counters = dict(START_INDICES)
    tasks = []
    for record in records:
        destination = record["destination"]
        asset_name = f"{PREFIXES[destination]}_{counters[destination]:03d}"
        counters[destination] += 1
        record["asset_name"] = asset_name
        record["asset_path"] = f"{CONTENT_ROOT}/{destination}/{asset_name}"
        converted_source = Path(unreal.Paths.project_saved_dir()) / "QuizConvertedSources" / f"{asset_name}.png"
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(converted_source if converted_source.exists() else record["source"]))
        task.set_editor_property("destination_path", f"{CONTENT_ROOT}/{destination}")
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    failures = [record for record in records if not unreal.EditorAssetLibrary.does_asset_exist(record["asset_path"])]
    if failures:
        failed_path = MANIFEST_PATH.with_name("IdolQuizImportFailures.csv")
        with failed_path.open("w", newline="", encoding="utf-8-sig") as handle:
            writer = csv.writer(handle)
            writer.writerow(["source", "extension", "size_bytes", "expected_asset"])
            for record in failures:
                writer.writerow([record["source"], record["source"].suffix.lower(), record["source"].stat().st_size, record["asset_path"]])
        unreal.log_warning(f"Skipped {len(failures)} unreadable quiz images; report={failed_path}")
    failed_paths = {record["asset_path"] for record in failures}
    return [record for record in records if record["asset_path"] not in failed_paths]


def create_expanded_table(records):
    table_path = f"{DATA_PATH}/{TABLE_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(table_path):
        unreal.EditorAssetLibrary.delete_asset(table_path)
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.IdolQuizQuestion.static_struct())
    table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(TABLE_NAME, DATA_PATH, unreal.DataTable, factory)
    if table is None:
        raise RuntimeError("Could not create expanded Idol Quiz DataTable")

    csv_lines = ["Name,QuestionId,Image,StageName,RealName,Aliases,GroupName,Generation,Category,bEnabled"]
    for index, record in enumerate(records, 1):
        texture_reference = f"Texture2D'{record['asset_path']}.{record['asset_name']}'"
        values = [
            f"Expanded_{index:04d}",
            f"Expanded_{index:04d}",
            texture_reference,
            record["stage_name"],
            "",
            "",
            record["group"],
            record["generation"],
            record["category"],
            "True",
        ]
        csv_lines.append(",".join('"' + str(value).replace('"', '""') + '"' for value in values))
    csv_text = "\n".join(csv_lines)
    import_succeeded = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, csv_text)
    if not import_succeeded:
        raise RuntimeError("DataTable CSV import failed; see LogCSVImportFactory for details")
    unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)


def write_manifest(records):
    MANIFEST_PATH.parent.mkdir(parents=True, exist_ok=True)
    with MANIFEST_PATH.open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=["asset_name", "asset_path", "stage_name", "group", "generation", "category", "source", "sha1"])
        writer.writeheader()
        for record in records:
            writer.writerow({key: str(record[key]) for key in writer.fieldnames})


records = collect_unique_sources()
unreal.log(f"Idol Quiz import: {len(records)} unique source images")
records = import_textures(records)
create_expanded_table(records)
write_manifest(records)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(f"IDOL_QUIZ_IMPORT_COMPLETE count={len(records)} manifest={MANIFEST_PATH}")
