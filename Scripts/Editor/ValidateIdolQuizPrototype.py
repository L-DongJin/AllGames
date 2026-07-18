import csv
import os

import unreal


TABLE_PATH = "/Game/IdolQuiz/Data/DT_IdolQuizQuestions"
QUIZ_MAP = "/Game/Maps/IdolQuizMap"
DEFINITION_PATH = "/Game/Common/Data/DA_Game_IdolQuiz"
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CSV_PATH = os.path.join(PROJECT_ROOT, "SourceAssets", "IdolQuiz", "IdolQuizQuestions.csv")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    table = unreal.load_asset(TABLE_PATH)
    require(table is not None, "Idol quiz DataTable is missing")
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    require(len(row_names) == 96, "Expected 96 questions, found {}".format(len(row_names)))

    question_ids = set()
    groups = set()
    generation_counts = {3: 0, 4: 0}
    with open(CSV_PATH, "r", encoding="utf-8-sig", newline="") as source:
        rows = list(csv.DictReader(source))
    require(len(rows) == 96, "Expected 96 CSV rows, found {}".format(len(rows)))
    table_row_names = {str(name) for name in row_names}
    for index, question in enumerate(rows, 1):
        question_id = question["QuestionId"].strip()
        row_name = question["Name"].strip()
        stage_name = question["StageName"].strip()
        real_name = question["RealName"].strip()
        group = question["GroupName"].strip()
        image_path = question["Image"].split("'")[1].split(".")[0]
        require(row_name in table_row_names, "CSV row is absent from DataTable: {}".format(row_name))
        require(question_id, "Question {} has no ID".format(index))
        require(question_id not in question_ids, "Duplicate question ID: {}".format(question_id))
        require(stage_name, "Question {} has no stage name".format(index))
        require(real_name, "Question {} has no real name".format(index))
        require(group, "Question {} has no group".format(index))
        require(unreal.EditorAssetLibrary.does_asset_exist(image_path), "Question {} image is missing".format(index))
        generation = int(question["Generation"])
        require(generation in generation_counts, "Question {} has unsupported generation".format(index))
        generation_counts[generation] += 1
        require(question["bEnabled"].lower() == "true", "Question {} is disabled".format(index))
        question_ids.add(question_id)
        groups.add(group)

    require(len(groups) == 15, "Expected 15 groups, found {}".format(len(groups)))
    require(generation_counts == {3: 83, 4: 13}, "Unexpected generation counts: {}".format(generation_counts))
    require(unreal.EditorAssetLibrary.does_asset_exist(QUIZ_MAP), "IdolQuizMap is missing")

    definition = unreal.load_asset(DEFINITION_PATH)
    require(definition is not None, "Idol quiz mini-game definition is missing")
    require(definition.get_editor_property("enabled"), "Idol quiz definition is disabled")
    entry_map = definition.get_editor_property("entry_map")
    require(entry_map is not None and entry_map.get_path_name().startswith(QUIZ_MAP), "Idol quiz entry map is incorrect")

    unreal.log("IDOL QUIZ VALIDATION PASSED: 96 DataTable questions with stage/real names, 15 groups, map and hub entry ready")


main()
