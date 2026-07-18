import os

import unreal


SOURCE_ROOT = r"C:\Users\user\Downloads\인물보고이름맞히기\아이돌\4세대"
IMAGE_DIR = "/Game/IdolQuiz/Images/Generation4"
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
            if os.path.isfile(path) and extension.lower() in (".jpg", ".jpeg", ".png", ".webp"):
                files.append((group_name, answer, path, extension.lower()))
    return files


def main():
    files = source_files()
    if len(files) != 13:
        raise RuntimeError("Expected 13 fourth-generation images, found {}".format(len(files)))

    tasks = []
    for index, (_, _, source_path, extension) in enumerate(files, 1):
        repaired_path = os.path.join(REPAIRED_SOURCE_DIR, "T_IDOL4_{:03d}.png".format(index))
        if extension == ".webp":
            if not os.path.isfile(repaired_path):
                raise RuntimeError("WEBP repair PNG is missing: {}".format(repaired_path))
            source_path = repaired_path
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source_path)
        task.set_editor_property("destination_path", IMAGE_DIR)
        task.set_editor_property("destination_name", "T_IDOL4_{:03d}".format(index))
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    for index in range(1, 14):
        path = IMAGE_DIR + "/T_IDOL4_{:03d}".format(index)
        if unreal.load_asset(path) is None:
            raise RuntimeError("Imported texture is missing: {}".format(path))
    unreal.log("IDOL QUIZ GEN4 IMAGES READY: 13 images across 2 groups")


main()
