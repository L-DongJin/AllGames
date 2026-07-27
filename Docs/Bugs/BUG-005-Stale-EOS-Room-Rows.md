# BUG-005: Stale EOS room rows

## Symptom

- A room could remain visible after its final player left.
- Selecting the row failed instead of entering the room.
- Restarting PIE did not necessarily remove historical rows.

## Cause

EOS discovery can briefly return an empty cached lobby after `LeaveLobby`. A forced PIE shutdown can also leave the local EOS account registered in a lobby while the new `GameInstance` no longer has an `ActiveEOSLobby`; attempting to join that row returns `EOS_Lobby_LobbyAlreadyExists`.

## Fix

- The final owner makes a one-member lobby non-advertised before leaving.
- Discovery does not expose zero-member lobby snapshots.
- Discovery detects an untracked lobby containing the local account, leaves it automatically, and suppresses the row.
- Search-result indexing is built only from visible, joinable rows.
- Before every new room creation, `RestoreLobbies` reconstructs backend membership after a previous crash, then all joined lobbies are left before `CreateLobby` runs. This prevents hidden residual membership from blocking creation even after Net Mode or the application has been restarted.

## Verification

- Enter the room browser once and refresh to clean historical orphan memberships.
- Create a room alone, leave through the shared lobby, and refresh from another client.
- Confirm the old row is absent and no `EOS_Lobby_LobbyAlreadyExists` error occurs.
