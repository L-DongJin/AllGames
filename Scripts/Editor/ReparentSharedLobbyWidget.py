import unreal


blueprint = unreal.load_asset("/Game/UI/WBP_SharedLobby")
parent = unreal.load_class(None, "/Script/AllGames.IdolQuizLobbyWidget")
if blueprint is None or parent is None:
    raise RuntimeError("WBP_SharedLobby or IdolQuizLobbyWidget class is missing")

unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent)
unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
unreal.log("WBP_SHARED_LOBBY REPARENTED")
