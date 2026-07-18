import os

import unreal


SOURCE_ROOT = r"C:\Users\user\Downloads\인물보고이름맞히기\아이돌\3세대"
IMAGE_DIR = "/Game/IdolQuiz/Images/Generation3"
DATA_DIR = "/Game/IdolQuiz/Data"
CATALOG_PATH = DATA_DIR + "/DA_IdolQuiz_3rdGeneration"
QUIZ_MAP = "/Game/Maps/IdolQuizMap"
REPAIRED_SOURCE_DIR = os.path.join(os.path.dirname(__file__), "ImportCache", "IdolQuiz")


def source_files():
    files = []
    for group_name in sorted(os.listdir(SOURCE_ROOT)):
        group_dir = os.path.join(SOURCE_ROOT, group_name)
        if not os.path.isdir(group_dir):
            continue
        for filename in sorted(os.listdir(group_dir)):
            path = os.path.join(group_dir, filename)
            answer, extension = os.path.splitext(filename)
            if os.path.isfile(path) and extension.lower() in (".jpg", ".jpeg", ".png"):
                files.append((group_name, answer, path))
    return files


def import_images(files):
    tasks = []
    for index, (_, _, source_path) in enumerate(files, 1):
        repaired_path = os.path.join(REPAIRED_SOURCE_DIR, "T_IDOL3_{:03d}.png".format(index))
        if os.path.isfile(repaired_path):
            source_path = repaired_path
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source_path)
        task.set_editor_property("destination_path", IMAGE_DIR)
        task.set_editor_property("destination_name", "T_IDOL3_{:03d}".format(index))
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)


def get_or_create_catalog():
    catalog = unreal.load_asset(CATALOG_PATH)
    if catalog is not None:
        return catalog
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.IdolQuizCatalogDataAsset)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_IdolQuiz_3rdGeneration", DATA_DIR, unreal.IdolQuizCatalogDataAsset, factory
    )


def main():
    files = source_files()
    if len(files) != 83:
        raise RuntimeError("Expected 83 third-generation images, found {}".format(len(files)))
    import_images(files)
    questions = []
    for index, (group_name, answer, _) in enumerate(files, 1):
        texture_path = IMAGE_DIR + "/T_IDOL3_{:03d}".format(index)
        texture = unreal.load_asset(texture_path)
        if texture is None:
            raise RuntimeError("Imported texture is missing: {}".format(texture_path))
        question = unreal.IdolQuizQuestion()
        question.set_editor_property("question_id", "IDOL3_{:03d}".format(index))
        question.set_editor_property("image", texture)
        question.set_editor_property("answer", answer)
        question.set_editor_property("accepted_answers", [answer])
        question.set_editor_property("group_name", group_name)
        question.set_editor_property("generation", 3)
        questions.append(question)
    catalog = get_or_create_catalog()
    catalog.set_editor_property("questions", questions)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(QUIZ_MAP):
        if not level_subsystem.new_level(QUIZ_MAP):
            raise RuntimeError("Failed to create IdolQuizMap")
    else:
        level_subsystem.load_level(QUIZ_MAP)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = unreal.load_class(None, "/Script/AllGames.IdolQuizGameModeBase")
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    level_subsystem.save_current_level()

    definition = unreal.load_asset("/Game/Common/Data/DA_Game_IdolQuiz")
    definition.set_editor_property("entry_map", unreal.load_asset(QUIZ_MAP))
    definition.set_editor_property("enabled", True)
    unreal.EditorAssetLibrary.save_loaded_asset(definition, only_if_is_dirty=False)
    unreal.log("IDOL QUIZ READY: 83 questions across {} groups".format(len(set(group for group, _, _ in files))))


main()
