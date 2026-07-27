# BUG-006: Drawing Quiz fragmented remote strokes

## Symptom

- The drawer saw continuous lines locally, while another PC saw disconnected dots or short fragments.
- Eraser strokes appeared ineffective remotely.
- Full canvas clear could appear inconsistent during the same test.

## Cause

Every mouse segment used an unreliable client-to-server RPC followed by an unreliable multicast. Packet loss on either hop permanently removed intermediate segments. Eraser commands used the same lossy segment structure.

A later regression remained after switching those calls to reliable delivery:

- each two-point segment was still painted as an independent Slate line, so anti-aliased segment ends could look like regularly spaced gaps;
- the drawing client waited for the server round trip before displaying its own segment;
- every mouse event generated its own RPC/multicast instead of batching one frame of points;
- the feedback TextBlock overlapped the eraser and Clear controls at a higher Z-order and intercepted part of their mouse hit area;
- Clear existed only as a transient multicast event, with no replicated state revision to recover or confirm the clear.

## Fix

- The drawing client paints accepted points immediately and suppresses its echoed server copy.
- Points collected during one widget tick are sent as a bounded reliable batch instead of one RPC per point.
- Consecutive segments with the same brush are rendered as one Slate polyline, removing visible seams between two-point draw calls.
- The normalized sample spacing is 0.0025 for smoother curves while batching controls RPC count.
- Feedback is hit-test-invisible, drawing controls render above it, and the eraser/Clear hit areas are larger.
- Eraser strokes use the same predicted and batched path.
- Clear immediately resets the local canvas and increments a server-owned replicated `CanvasRevision`; its RepNotify clears every client and also survives timing/order differences better than a multicast-only event.

## Verification

- Test with two separate PCs rather than two local windows.
- Draw slow and fast curves and confirm both screens show continuous paths.
- Select Eraser as the active drawer and remove several portions on both screens.
- Use Clear and confirm both canvases become empty immediately.
- Confirm `Drawing Quiz canvas cleared by drawer.` appears once on the server and that no `clear rejected` warning appears for the active drawer.
