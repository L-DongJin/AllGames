import unreal


required = ["BG", "Title", "GameInfo", "Status", "Start", "Leave"]
required.extend([f"Player{index}" for index in range(1, 7)])

widget_class = unreal.load_class(None, "/Game/UI/WBP_SharedLobby.WBP_SharedLobby_C")
if widget_class is None:
    raise RuntimeError("Shared lobby classes could not be loaded")
if not isinstance(unreal.get_default_object(widget_class), unreal.IdolQuizLobbyWidget):
    raise RuntimeError("WBP_SharedLobby must inherit IdolQuizLobbyWidget")

unreal.log("WBP_SHARED_LOBBY CLASS READY; runtime bindings: " + ", ".join(required))
