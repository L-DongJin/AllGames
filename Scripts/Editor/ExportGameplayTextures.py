import os

import unreal


OUTPUT_DIRECTORY = os.path.join(unreal.Paths.project_saved_dir(), "Diagnostics")
ASSET_PATHS = [
    "/Game/Textures/IMG_Background",
    "/Game/Textures/IMG_LaneBacground",
    "/Game/Textures/IMG_LaneBackground",
    "/Game/Textures/IMG_Note",
    "/Game/Textures/IMG_Note2",
    "/Game/Textures/T_LaneGlow",
]


os.makedirs(OUTPUT_DIRECTORY, exist_ok=True)
for asset_path in ASSET_PATHS:
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture is None:
        unreal.log_warning("Could not load {}".format(asset_path))
        continue

    task = unreal.AssetExportTask()
    task.set_editor_property("object", texture)
    task.set_editor_property(
        "filename",
        os.path.join(OUTPUT_DIRECTORY, asset_path.rsplit("/", 1)[-1] + ".png"),
    )
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("exporter", unreal.TextureExporterPNG())
    if not unreal.Exporter.run_asset_export_task(task):
        unreal.log_error("Failed to export {}".format(asset_path))
