# BUG-002: EOS P2P lobby join returns to room browser

## Status

- Identified from a second-PC packaged-build log on 2026-07-20.
- Fix implemented; cross-PC packaged verification pending.

## Symptoms

- The remote PC found the public EOS lobby.
- The first JoinLobby call succeeded, but the client returned to the room browser.
- Later attempts displayed `인터넷 방 입장에 실패했습니다.`.

## Evidence and root cause

- EOS authentication, lobby search, and the first lobby join all succeeded.
- `GetResolvedConnectString` returned `EOS:<ProductUserId>`.
- UE 5.7 `FURL` parsed this single-colon string as a protocol plus map name and emitted `InvalidURL` before `NetDriverEOS` could connect.
- Because travel failed after joining, EOS still considered the user a member. Repeated JoinLobby calls then returned `EOS_Lobby_LobbyAlreadyExists`, which produced the visible failure message.

## Fix

- Convert the EOSGS result to bracketed-host form `[EOS:<ProductUserId>]` before `ClientTravel`.
- UE's URL parser preserves the bracket contents as the host, while `NetDriverEOS` receives the expected `EOS:<ProductUserId>` address.
- Restarting the old failed client clears its stale local lobby membership; the fixed build should not enter that state.

## Verification

- Editor target build required.
- Repackage and run the same build on two PCs.
- Expected client log: successful JoinLobby, bracketed P2P travel, `NetDriverEOS` connection, and `IdolQuizLobbyMap` load without `InvalidURL` or `EOS_Lobby_LobbyAlreadyExists`.
