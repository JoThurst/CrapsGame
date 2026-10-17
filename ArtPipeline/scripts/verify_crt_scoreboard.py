"""
Verification & Live Blender MCP Sync Utility for SM_CRT_Scoreboard_v1.

1. Syncs the procedural generation script into the live Blender session via Blender MCP socket (127.0.0.1:9876)
   and switches the 3D Viewport to Material Preview + frames the scoreboard.
2. Runs a clean-room FBX round-trip import verification in Blender and renders a preview image.
"""

from __future__ import annotations

import json
import os
import socket
import subprocess
import sys

PROJECT_ROOT = r"C:\Code\Unreal Projects\Craps\CrapsCrapsCraps"
GENERATOR_SCRIPT = os.path.join(PROJECT_ROOT, "ArtPipeline", "scripts", "generate_crt_scoreboard.py")
MANIFEST_PATH = os.path.join(PROJECT_ROOT, "ArtPipeline", "exports", "SM_CRT_Scoreboard_v1_manifest.json")
FBX_PATH = os.path.join(PROJECT_ROOT, "ArtPipeline", "exports", "SM_CRT_Scoreboard_v1.fbx")
BLEND_PATH = os.path.join(PROJECT_ROOT, "ArtPipeline", "blend", "CRT_Scoreboard_v1.blend")
PREVIEW_PNG = os.path.join(PROJECT_ROOT, "ArtPipeline", "exports", "SM_CRT_Scoreboard_v1_preview.png")
BLENDER_EXE = r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"


def run_in_live_blender_mcp() -> bool:
    """Executes the generator inside the open Blender instance on 127.0.0.1:9876 and renders a preview."""
    code = f"""
import bpy
import runpy

ns = runpy.run_path(r"{GENERATOR_SCRIPT}", run_name="__main__")

# Set viewport shading to RENDERED so the user sees warm casino lighting & materials immediately
for window in bpy.context.window_manager.windows:
    for area in window.screen.areas:
        if area.type == 'VIEW_3D':
            for space in area.spaces:
                if space.type == 'VIEW_3D':
                    space.shading.type = 'RENDERED'
                    space.region_3d.view_perspective = 'CAMERA'

# Render a close-up beauty preview under warm casino lighting for visual inspection
scene = bpy.context.scene
scene.render.resolution_x = 1920
scene.render.resolution_y = 1080
scene.render.filepath = r"{PREVIEW_PNG}"
bpy.ops.render.render(write_still=True)
print("LIVE_BLENDER_MCP_SUCCESS")
"""
    try:
        with socket.create_connection(("127.0.0.1", 9876), timeout=30) as sock:
            payload = json.dumps({"type": "execute_code", "params": {"code": code}}).encode("utf-8")
            sock.sendall(payload)
            resp_raw = sock.recv(65536).decode("utf-8")
            resp = json.loads(resp_raw)
            print("[BlenderMCP] Response:", resp)
            return resp.get("status") == "success"
    except Exception as exc:
        print(f"[BlenderMCP] Could not execute via socket ({exc}), falling back to CLI.")
        return False


def verify_fbx_roundtrip_cli() -> dict:
    """Re-imports the exported FBX in headless Blender to verify UE5 readiness."""
    verify_code = f"""
import bpy, json, sys

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=r"{FBX_PATH}")

imported_meshes = [o for o in bpy.data.objects if o.type == "MESH"]
screens = [o for o in imported_meshes if "Screen_" in o.name]
collisions = [o for o in imported_meshes if o.name.startswith("UCX_")]

screen_uv_checks = {{}}
for s in screens:
    uv_layer = s.data.uv_layers.active
    if uv_layer:
        uvs = [d.uv[:] for d in uv_layer.data]
        u_min = round(min(u[0] for u in uvs), 4)
        u_max = round(max(u[0] for u in uvs), 4)
        v_min = round(min(u[1] for u in uvs), 4)
        v_max = round(max(u[1] for u in uvs), 4)
        screen_uv_checks[s.name] = {{"u_range": [u_min, u_max], "v_range": [v_min, v_max]}}

report = {{
    "fbx_imported_mesh_count": len(imported_meshes),
    "fbx_screen_meshes": sorted([s.name for s in screens]),
    "fbx_collision_meshes": sorted([c.name for c in collisions]),
    "screen_uv_bounds": screen_uv_checks,
}}
print("FBX_VERIFY_JSON:" + json.dumps(report))
"""
    proc = subprocess.run(
        [BLENDER_EXE, "--background", "--python-expr", verify_code],
        capture_output=True,
        text=True,
        check=True,
    )
    for line in proc.stdout.splitlines():
        if line.startswith("FBX_VERIFY_JSON:"):
            return json.loads(line.split("FBX_VERIFY_JSON:", 1)[1])
    raise RuntimeError(f"FBX verification output not found:\n{proc.stdout}\n{proc.stderr}")


def main() -> None:
    mcp_ok = run_in_live_blender_mcp()
    if not mcp_ok:
        subprocess.run([BLENDER_EXE, "--background", "--python", GENERATOR_SCRIPT], check=True)

    assert os.path.exists(BLEND_PATH), f"Missing .blend file: {BLEND_PATH}"
    assert os.path.exists(FBX_PATH), f"Missing .fbx file: {FBX_PATH}"
    assert os.path.exists(MANIFEST_PATH), f"Missing manifest file: {MANIFEST_PATH}"

    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        manifest = json.load(f)

    fbx_report = verify_fbx_roundtrip_cli()

    # Assertions for all requirements
    expected_screens = [
        "SM_Scoreboard_Screen_Ante",
        "SM_Scoreboard_Screen_Bankroll",
        "SM_Scoreboard_Screen_Multiplier",
        "SM_Scoreboard_Screen_UnbankedPool",
    ]
    assert fbx_report["fbx_screen_meshes"] == expected_screens, (
        f"Expected 4 modular screens {expected_screens}, got {fbx_report['fbx_screen_meshes']}"
    )
    assert fbx_report["fbx_collision_meshes"] == ["UCX_SM_Scoreboard_Hull_Main_01"], (
        f"Missing UCX collision mesh: {fbx_report['fbx_collision_meshes']}"
    )
    for screen_name, uv_bounds in fbx_report["screen_uv_bounds"].items():
        assert uv_bounds["u_range"] == [0.0, 1.0] and uv_bounds["v_range"] == [0.0, 1.0], (
            f"Screen {screen_name} UVs not normalized [0..1]: {uv_bounds}"
        )

    non_manifold_total = sum(o["non_manifold_edges"] for o in manifest["objects"])
    degenerate_total = sum(o["degenerate_faces"] for o in manifest["objects"])
    assert non_manifold_total == 0, f"Found {non_manifold_total} non-manifold edges!"
    assert degenerate_total == 0, f"Found {degenerate_total} degenerate faces!"

    print("\n=== VERIFICATION PASSED ===")
    print(f"Blend File:   {BLEND_PATH} ({manifest['files']['blend_size_bytes']:,} bytes)")
    print(f"FBX File:     {FBX_PATH} ({manifest['files']['fbx_size_bytes']:,} bytes)")
    print(f"Preview PNG:  {PREVIEW_PNG} (exists={os.path.exists(PREVIEW_PNG)})")
    print(f"Render Meshes: {manifest['totals_excluding_collision']['mesh_object_count']}")
    print(f"Total Tris:    {manifest['totals_excluding_collision']['total_triangles']}")
    print("FBX Roundtrip:", json.dumps(fbx_report, indent=2))


if __name__ == "__main__":
    main()
