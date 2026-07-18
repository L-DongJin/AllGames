import unreal


CATALOG_PATH = "/Game/IdolQuiz/Data/DA_IdolQuiz_3rdGeneration"
QUIZ_MAP = "/Game/Maps/IdolQuizMap"
DEFINITION_PATH = "/Game/Common/Data/DA_Game_IdolQuiz"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    catalog = unreal.load_asset(CATALOG_PATH)
    require(catalog is not None, "Idol quiz catalog is missing")
    questions = catalog.get_editor_property("questions")
    require(len(questions) == 83, "Expected 83 questions, found {}".format(len(questions)))

    question_ids = set()
    groups = set()
    for index, question in enumerate(questions, 1):
        question_id = str(question.get_editor_property("question_id"))
        answer = str(question.get_editor_property("answer"))
        group = str(question.get_editor_property("group_name"))
        image = question.get_editor_property("image")
        require(question_id and question_id != "None", "Question {} has no ID".format(index))
        require(question_id not in question_ids, "Duplicate question ID: {}".format(question_id))
        require(answer, "Question {} has no answer".format(index))
        require(group, "Question {} has no group".format(index))
        require(image is not None, "Question {} has no image".format(index))
        require(question.get_editor_property("generation") == 3, "Question {} has wrong generation".format(index))
        question_ids.add(question_id)
        groups.add(group)

    require(len(groups) == 13, "Expected 13 groups, found {}".format(len(groups)))
    require(unreal.EditorAssetLibrary.does_asset_exist(QUIZ_MAP), "IdolQuizMap is missing")

    definition = unreal.load_asset(DEFINITION_PATH)
    require(definition is not None, "Idol quiz mini-game definition is missing")
    require(definition.get_editor_property("enabled"), "Idol quiz definition is disabled")
    entry_map = definition.get_editor_property("entry_map")
    require(entry_map is not None and entry_map.get_path_name().startswith(QUIZ_MAP), "Idol quiz entry map is incorrect")

    unreal.log("IDOL QUIZ VALIDATION PASSED: 83 questions, 13 groups, map and hub entry ready")


main()
