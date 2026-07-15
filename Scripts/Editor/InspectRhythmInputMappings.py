import unreal


for context_path in ("/Game/Input/IMC_Rhythm_5Key", "/Game/Input/IMC_Rhythm_9Key"):
    context = unreal.load_asset(context_path)
    if context is None:
        unreal.log_error("Missing input context: {}".format(context_path))
        continue

    unreal.log("INPUT CONTEXT {}".format(context_path))
    for mapping in context.get_editor_property("mappings"):
        action = mapping.get_editor_property("action")
        key = mapping.get_editor_property("key")
        key_name = key.get_editor_property("key_name")
        unreal.log("  {} -> {}".format(key_name, action.get_name() if action else "None"))
