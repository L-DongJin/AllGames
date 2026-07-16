import unreal


ASSET_PATH = "/Game/Data/DA_LongNote_Test"
MUSIC_PATH = "/Game/Audio/Music/Choom"


def make_note(lane, target, duration=0.0):
    note = unreal.RhythmNoteData()
    note.set_editor_property("lane_index", lane)
    note.set_editor_property("target_time_seconds", target)
    note.set_editor_property("duration_seconds", duration)
    return note


def main():
    asset = unreal.load_asset(ASSET_PATH) if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH) else None
    if asset is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.RhythmSongDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_LongNote_Test", "/Game/Data", unreal.RhythmSongDataAsset, factory
        )

    music = unreal.load_asset(MUSIC_PATH)
    if music is None:
        raise RuntimeError("Choom music is missing")

    asset.set_editor_property("song_title", "Long Note Test")
    asset.set_editor_property("music", music)
    asset.set_editor_property("bpm", 120.0)
    asset.set_editor_property("music_offset_seconds", 0.0)
    asset.set_editor_property("key_mode", unreal.RhythmChartKeyMode.FIVE_KEY)
    asset.set_editor_property("difficulty", unreal.RhythmDifficulty.NORMAL)
    asset.set_editor_property("note_travel_time_seconds", 2.0)
    asset.set_editor_property("notes", [
        make_note(0, 5.0, 2.0),
        make_note(1, 9.0),
        make_note(2, 11.0, 3.0),
        make_note(3, 16.0, 1.5),
        make_note(4, 20.0),
    ])
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

    widget_class = unreal.load_class(None, "/Game/UI/WBP_RhythmGameplay.WBP_RhythmGameplay_C")
    if widget_class is None:
        raise RuntimeError("WBP_RhythmGameplay class is missing")
    defaults = unreal.get_default_object(widget_class)
    texture_properties = [
        "long_note_head_image",
        "long_note_body_image",
        "long_note_tail_image",
        "long_note_hold_glow_image",
        "long_note_complete_effect_image",
        "long_note_break_effect_image",
    ]
    missing = [name for name in texture_properties if defaults.get_editor_property(name) is None]
    if missing:
        raise RuntimeError("Long-note textures are not assigned: {}".format(", ".join(missing)))

    unreal.log("LONG NOTE TEST READY: holds at 5-7s, 11-14s, 16-17.5s; all six textures assigned")


main()
