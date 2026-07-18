import os

import unreal


TITLE_DIR = "/Game/Title"
REPAIRED_DIR = os.path.join(os.path.dirname(__file__), "ImportCache", "RhythmTitle")
SONGS = [
    ("Drip", r"C:\Users\user\Downloads\Music\썸네일\Drip.jpg", "Drip"),
    ("YenaCatch", r"C:\Users\user\Downloads\Music\썸네일\캐치캐치.jpg", "CatchCatch"),
    ("IveBangBang", os.path.join(REPAIRED_DIR, "BangBang.png"), "BangBang"),
    ("Sheesh", r"C:\Users\user\Downloads\Music\썸네일\Sheesh.jpg", "Sheesh"),
    ("KiiiKiii404", r"C:\Users\user\Downloads\Music\썸네일\Kiikii404.jpg", "KiiiKiii404"),
]
DIFFICULTIES = ("Easy", "Normal", "Hard", "Expert")


def main():
    tasks = []
    for _, source_path, texture_name in SONGS:
        if not os.path.isfile(source_path):
            raise RuntimeError("Title image source is missing: {}".format(source_path))
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source_path)
        task.set_editor_property("destination_path", TITLE_DIR)
        task.set_editor_property("destination_name", texture_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    assigned = 0
    for asset_prefix, _, texture_name in SONGS:
        texture = unreal.load_asset("{}/{}".format(TITLE_DIR, texture_name))
        if texture is None:
            raise RuntimeError("Imported title texture is missing: {}".format(texture_name))
        for difficulty in DIFFICULTIES:
            chart_path = "/Game/Data/DA_{}_{}_5Key".format(asset_prefix, difficulty)
            chart = unreal.load_asset(chart_path)
            if chart is None:
                raise RuntimeError("Rhythm chart is missing: {}".format(chart_path))
            chart.set_editor_property("title_image", texture)
            unreal.EditorAssetLibrary.save_loaded_asset(chart, only_if_is_dirty=False)
            assigned += 1
    unreal.log("FIVE NEW SONG TITLE IMAGES READY: 5 textures assigned to {} charts".format(assigned))


main()
