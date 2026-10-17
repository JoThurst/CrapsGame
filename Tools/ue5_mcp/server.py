#!/usr/bin/env python3
"""
Unreal Engine 5.3 MCP Server for CRT Synth-Noir Craps Roguelike (CrapsCrapsCraps).

Combines:
1. Official Unreal Remote Control HTTP API (localhost:30010) for real-time UObject property
   get/set, function calls, and asset queries.
2. PythonScriptPlugin remote execution via `/Script/PythonScriptPlugin.Default__PythonScriptLibrary:ExecutePythonCommandEx`
   with automatic `unreal.ScopedEditorTransaction` undo wrapping (per ue5-cpp-standards.md).
3. Blueprint & DataAsset batch generation helpers for `/Game/Data/DiceFaces/`, `/Game/Data/Trinkets/`,
   and `/Game/Maps/L_TestingPit` setup.
"""

import json
import os
import textwrap
import urllib.error
import urllib.request
from typing import Any, Dict, List, Optional

try:
    from mcp.server.mcpserver import MCPServer as FastMCP
except ImportError:
    from mcp.server.fastmcp import FastMCP

UE_REMOTE_URL = os.environ.get("UE_REMOTE_URL", "http://127.0.0.1:30010")
UE_WS_HOST = os.environ.get("UE_WS_HOST", "127.0.0.1")
UE_WS_PORT = int(os.environ.get("UE_WS_PORT", "30020"))
PYTHON_LIB_PATH = "/Script/PythonScriptPlugin.Default__PythonScriptLibrary"

mcp = FastMCP("unreal-craps-mcp")


def _rc_ws_request(endpoint: str, method: str = "PUT", payload: Optional[Dict[str, Any]] = None, timeout: float = 30.0) -> Dict[str, Any]:
    """Send a request over Unreal Engine's Remote Control WebSocket server (port 30020) when HTTP 30010 is unavailable."""
    import base64
    import socket
    import struct

    url_path = "/" + endpoint.lstrip("/")
    s = socket.create_connection((UE_WS_HOST, UE_WS_PORT), timeout=timeout)
    try:
        key = base64.b64encode(b"antigravity-ue5-mcp").decode()
        handshake = (
            "GET / HTTP/1.1\r\n"
            f"Host: {UE_WS_HOST}:{UE_WS_PORT}\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Key: {key}\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n"
        )
        s.sendall(handshake.encode("utf-8"))
        resp = b""
        while b"\r\n\r\n" not in resp:
            chunk = s.recv(4096)
            if not chunk:
                break
            resp += chunk

        params: Dict[str, Any] = {
            "RequestId": 1,
            "Url": url_path,
            "Verb": method.upper(),
        }
        if payload is not None:
            params["Body"] = payload

        msg_bytes = json.dumps({"MessageName": "http", "Parameters": params}).encode("utf-8")
        mask_key = b"\x12\x34\x56\x78"
        header = bytearray([0x81])
        length = len(msg_bytes)
        if length < 126:
            header.append(0x80 | length)
        elif length < 65536:
            header.append(0x80 | 126)
            header.extend(struct.pack("!H", length))
        else:
            header.append(0x80 | 127)
            header.extend(struct.pack("!Q", length))
        header.extend(mask_key)
        masked = bytearray(b ^ mask_key[i % 4] for i, b in enumerate(msg_bytes))
        s.sendall(header + masked)

        def _recv_exact(num_bytes: int) -> bytes:
            buf = bytearray()
            while len(buf) < num_bytes:
                c = s.recv(num_bytes - len(buf))
                if not c:
                    break
                buf.extend(c)
            return bytes(buf)

        hdr = _recv_exact(2)
        if len(hdr) < 2:
            return {"error": "Empty WebSocket frame from Unreal Editor"}
        frame_len = hdr[1] & 0x7F
        if frame_len == 126:
            frame_len = struct.unpack("!H", _recv_exact(2))[0]
        elif frame_len == 127:
            frame_len = struct.unpack("!Q", _recv_exact(8))[0]
        raw_body = _recv_exact(frame_len).decode("utf-8", errors="replace")
        ws_json = json.loads(raw_body)
        resp_code = ws_json.get("ResponseCode", 200)
        resp_body = ws_json.get("ResponseBody", {})
        if resp_code >= 400:
            return {"error": f"HTTP {resp_code}", "details": resp_body}
        return resp_body if isinstance(resp_body, dict) else {"result": resp_body}
    finally:
        try:
            s.close()
        except Exception:
            pass


def _rc_request(endpoint: str, method: str = "PUT", payload: Optional[Dict[str, Any]] = None, timeout: float = 15.0) -> Dict[str, Any]:
    """Send an HTTP request to Unreal Engine's Remote Control API, with automatic WebSocket (port 30020) fallback."""
    url = f"{UE_REMOTE_URL.rstrip('/')}/{endpoint.lstrip('/')}"
    data = json.dumps(payload).encode("utf-8") if payload is not None else None
    headers = {"Content-Type": "application/json"}
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=2.0) as resp:
            raw = resp.read().decode("utf-8")
            return json.loads(raw) if raw else {"status": "ok"}
    except urllib.error.HTTPError as e:
        err_body = e.read().decode("utf-8", errors="replace")
        return {
            "error": f"HTTP {e.code}: {e.reason}",
            "details": err_body,
            "url": url,
        }
    except urllib.error.URLError:
        try:
            return _rc_ws_request(endpoint=endpoint, method=method, payload=payload, timeout=timeout)
        except Exception as ws_e:
            return {
                "error": f"Could not connect to Unreal Engine Remote Control API at {UE_REMOTE_URL} or WebSocket {UE_WS_HOST}:{UE_WS_PORT}.",
                "reason": str(ws_e),
            }
    except Exception as e:
        return {"error": str(e)}


@mcp.tool()
def ue_ping() -> Dict[str, Any]:
    """
    Check if the Unreal Engine 5.3 Editor Remote Control HTTP server is active on localhost:30010
    and return basic engine/project info.
    """
    info = _rc_request("/remote/info", method="GET", payload=None, timeout=3.0)
    if "error" in info:
        return {"connected": False, **info}
    return {"connected": True, "remote_url": UE_REMOTE_URL, "routes_count": len(info.get("HttpRoutes", [])), "info": info}


@mcp.tool()
def ue_get_property(object_path: str, property_name: str) -> Dict[str, Any]:
    """
    Read a UPROPERTY value from any UObject in the Unreal Editor or PIE world via Remote Control API.

    Args:
        object_path: Full Unreal UObject path (e.g. '/Game/Maps/L_TestingPit.L_TestingPit:PersistentLevel.DieBase_0.DiceRiggingComp')
        property_name: Name of the property to read (e.g. 'DiceControlStat')
    """
    payload = {
        "objectPath": object_path,
        "access": "READ_ACCESS",
        "propertyName": property_name,
    }
    return _rc_request("/remote/object/property", method="PUT", payload=payload)


@mcp.tool()
def ue_set_property(
    object_path: str,
    property_name: str,
    property_value: Any,
    generate_transaction: bool = True,
) -> Dict[str, Any]:
    """
    Modify a UPROPERTY value on any UObject in real time via Remote Control API.
    Supports Ctrl+Z undo in the Unreal Editor when generate_transaction=True.

    Args:
        object_path: Full Unreal UObject path
        property_name: Name of the UPROPERTY to modify
        property_value: The new value (number, string, bool, dict/struct, or list)
        generate_transaction: If True, wraps the change in an Editor Undo transaction
    """
    payload = {
        "objectPath": object_path,
        "access": "WRITE_TRANSACTION_ACCESS" if generate_transaction else "WRITE_ACCESS",
        "propertyName": property_name,
        "propertyValue": {property_name: property_value},
    }
    return _rc_request("/remote/object/property", method="PUT", payload=payload)


@mcp.tool()
def ue_call_function(
    object_path: str,
    function_name: str,
    parameters: Optional[Dict[str, Any]] = None,
    generate_transaction: bool = True,
) -> Dict[str, Any]:
    """
    Call any BlueprintCallable UFUNCTION on a UObject or Default__Class CDO via Remote Control API.

    Args:
        object_path: Full Unreal UObject path or CDO (e.g., '/Script/Engine.Default__KismetSystemLibrary')
        function_name: Name of the UFUNCTION to invoke
        parameters: Dictionary of function arguments
        generate_transaction: If True, records an undoable editor transaction
    """
    payload = {
        "objectPath": object_path,
        "functionName": function_name,
        "parameters": parameters or {},
        "generateTransaction": generate_transaction,
    }
    return _rc_request("/remote/object/call", method="PUT", payload=payload)


@mcp.tool()
def ue_describe_object(object_path: str) -> Dict[str, Any]:
    """
    Inspect a UObject's exposed properties, functions, and metadata using Unreal Reflection.

    Args:
        object_path: Full Unreal UObject path or CDO path (e.g. '/Script/CrapsCrapsCraps.Default__ADieBase')
    """
    return _rc_request("/remote/object/describe", method="PUT", payload={"objectPath": object_path})


@mcp.tool()
def ue_execute_python(
    code: str,
    transaction_name: str = "Antigravity Action",
    use_scoped_transaction: bool = True,
) -> Dict[str, Any]:
    """
    Execute an arbitrary Editor Python script (`import unreal`) inside the open Unreal Engine 5.3 Editor.
    Per ue5-cpp-standards.md, automatically wraps execution in `unreal.ScopedEditorTransaction` so
    every viewport/asset change can be undone with Ctrl+Z.

    Inside your script, assign to `result` (dict/list/str/number) if you want structured JSON returned.

    Args:
        code: Python source code to execute inside Unreal's Python environment.
        transaction_name: Name shown in Unreal's Undo History.
        use_scoped_transaction: Whether to wrap in unreal.ScopedEditorTransaction (default True).
    """
    import tempfile
    result_file = os.path.join(tempfile.gettempdir(), "agy_ue5_mcp_result.json").replace("\\", "/")
    if os.path.exists(result_file):
        try:
            os.remove(result_file)
        except Exception:
            pass

    indented_user_code = textwrap.indent(code.strip(), "    ")
    if use_scoped_transaction:
        wrapped_code = (
            "import unreal, json\n"
            "result = None\n"
            f"with unreal.ScopedEditorTransaction({json.dumps(transaction_name)}):\n"
            f"{indented_user_code}\n"
            "if result is not None:\n"
            f"    with open({json.dumps(result_file)}, 'w', encoding='utf-8') as _f:\n"
            "        json.dump(result, _f, default=str)\n"
        )
    else:
        wrapped_code = (
            "import unreal, json\n"
            "result = None\n"
            f"{code.strip()}\n"
            "if result is not None:\n"
            f"    with open({json.dumps(result_file)}, 'w', encoding='utf-8') as _f:\n"
            "        json.dump(result, _f, default=str)\n"
        )

    script_file = os.path.join(tempfile.gettempdir(), "agy_ue5_mcp_script.py").replace("\\", "/")
    with open(script_file, "w", encoding="utf-8") as sf:
        sf.write(wrapped_code)

    payload = {
        "objectPath": PYTHON_LIB_PATH,
        "functionName": "ExecutePythonCommandEx",
        "parameters": {
            "PythonCommand": script_file,
            "ExecutionMode": "ExecuteFile",
            "FileExecutionScope": "Public",
        },
        "generateTransaction": False,
    }
    resp = _rc_request("/remote/object/call", method="PUT", payload=payload, timeout=60.0)
    if "error" in resp:
        return resp

    ret_vals = resp.get("ReturnValues", {})
    parsed_result = None
    if os.path.exists(result_file):
        try:
            with open(result_file, "r", encoding="utf-8") as f:
                parsed_result = json.load(f)
        except Exception as e:
            parsed_result = {"read_error": str(e)}

    return {
        "success": ret_vals.get("ReturnValue", True),
        "CommandResult": ret_vals.get("CommandResult", ""),
        "Result": parsed_result,
        "LogOutput": ret_vals.get("LogOutput", []),
    }


@mcp.tool()
def ue_list_level_actors() -> Dict[str, Any]:
    """
    List all actors in the currently open Unreal Editor level with their class, label, location, rotation, and scale.
    """
    script = """
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_sub.get_all_level_actors()
out = []
for a in actors:
    loc = a.get_actor_location()
    rot = a.get_actor_rotation()
    scale = a.get_actor_scale3d()
    out.append({
        "name": a.get_name(),
        "label": a.get_actor_label(),
        "path": a.get_path_name(),
        "class": a.get_class().get_name(),
        "location": [round(loc.x, 2), round(loc.y, 2), round(loc.z, 2)],
        "rotation": [round(rot.pitch, 2), round(rot.yaw, 2), round(rot.roll, 2)],
        "scale": [round(scale.x, 2), round(scale.y, 2), round(scale.z, 2)],
    })
result = {"count": len(out), "actors": out}
"""
    return ue_execute_python(script, transaction_name="List Level Actors", use_scoped_transaction=False)


@mcp.tool()
def ue_spawn_actor(
    actor_class_path: str,
    label: str,
    location: List[float] = [0.0, 0.0, 0.0],
    rotation: List[float] = [0.0, 0.0, 0.0],
    scale: List[float] = [1.0, 1.0, 1.0],
) -> Dict[str, Any]:
    """
    Spawn an actor in the current Unreal Editor level wrapped in an undoable ScopedEditorTransaction.

    Args:
        actor_class_path: Class path (e.g. '/Script/CrapsCrapsCraps.DieBase' or '/Script/Engine.StaticMeshActor')
        label: Actor label in the World Outliner
        location: [X, Y, Z] in Unreal units (remember 10x Craps scale!)
        rotation: [Pitch, Yaw, Roll] in degrees
        scale: [X, Y, Z] scale factor
    """
    script = f"""
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cls = unreal.load_class(None, {json.dumps(actor_class_path)})
if not cls:
    raise RuntimeError(f"Could not load class: {actor_class_path}")
loc = unreal.Vector({float(location[0])}, {float(location[1])}, {float(location[2])})
rot = unreal.Rotator({float(rotation[0])}, {float(rotation[1])}, {float(rotation[2])})
spawned = actor_sub.spawn_actor_from_class(cls, loc, rot)
if spawned:
    spawned.set_actor_label({json.dumps(label)})
    spawned.set_actor_scale3d(unreal.Vector({float(scale[0])}, {float(scale[1])}, {float(scale[2])}))
    result = {{"path": spawned.get_path_name(), "label": spawned.get_actor_label()}}
else:
    raise RuntimeError("Failed to spawn actor")
"""
    return ue_execute_python(script, transaction_name=f"Spawn {label}", use_scoped_transaction=True)


@mcp.tool()
def ue_create_data_asset(
    asset_name: str,
    package_path: str,
    parent_class_path: str,
    properties: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    """
    Create or update a UPrimaryDataAsset (e.g., UPDA_DiceFace, UPDA_Trinket, UPDA_BetUpgrade) in the Content Browser.

    Args:
        asset_name: Name of the asset (e.g. 'DA_Face_NeonWild')
        package_path: Target Content folder (e.g. '/Game/Data/DiceFaces')
        parent_class_path: Full C++ or Blueprint class path (e.g. '/Script/CrapsCrapsCraps.PDA_DiceFace')
        properties: Dictionary of editor properties to set on the created Data Asset
    """
    props_json = json.dumps(properties or {})
    script = f"""
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
full_path = f"{package_path.rstrip('/')}/{asset_name}"
if unreal.EditorAssetLibrary.does_asset_exist(full_path):
    asset = unreal.EditorAssetLibrary.load_asset(full_path)
else:
    factory = unreal.DataAssetFactory()
    cls = unreal.load_class(None, {json.dumps(parent_class_path)})
    if not cls:
        raise RuntimeError(f"Could not find DataAsset class: {parent_class_path}")
    factory.set_editor_property("data_asset_class", cls)
    asset = asset_tools.create_asset({json.dumps(asset_name)}, {json.dumps(package_path.rstrip('/'))}, cls, factory)

props = json.loads({json.dumps(props_json)})
for k, v in props.items():
    asset.set_editor_property(k, v)

unreal.EditorAssetLibrary.save_loaded_asset(asset)
result = {{"asset_path": asset.get_path_name(), "saved": True}}
"""
    return ue_execute_python(script, transaction_name=f"Create DataAsset {asset_name}", use_scoped_transaction=True)


@mcp.tool()
def ue_build_testing_pit_map(map_path: str = "/Game/Maps/L_TestingPit") -> Dict[str, Any]:
    """
    Create or update `/Game/Maps/L_TestingPit` with `ACrapsTestingGameMode`, `ACrapsTableGrayboxActor`,
    directional/Lumen lighting, and save the map.
    """
    script = f"""
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if level_sub.is_in_play_in_editor():
    level_sub.editor_request_end_play()

unreal.EditorAssetLibrary.make_directory("/Game/Maps")
unreal.EditorAssetLibrary.make_directory("/Game/Data/DiceFaces")
unreal.EditorAssetLibrary.make_directory("/Game/Data/Trinkets")
unreal.EditorAssetLibrary.make_directory("/Game/Data/Bets")

if not unreal.EditorAssetLibrary.does_asset_exist({json.dumps(map_path)}):
    level_sub.new_level({json.dumps(map_path)})
else:
    level_sub.load_level({json.dumps(map_path)})

# Spawn CrapsTableGrayboxActor if not already in level
table_cls = unreal.load_class(None, "/Script/CrapsCrapsCraps.CrapsTableGrayboxActor")
existing_tables = [a for a in actor_sub.get_all_level_actors() if a.get_class() == table_cls] if table_cls else []
if table_cls and not existing_tables:
    table = actor_sub.spawn_actor_from_class(table_cls, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if table:
        table.set_actor_label("CrapsTable_10xGraybox")

# Spawn a PlayerStart right at the Shooter Head (0, 0, 0)
ps_cls = unreal.PlayerStart
existing_ps = [a for a in actor_sub.get_all_level_actors() if a.get_class() == ps_cls]
if not existing_ps:
    ps = actor_sub.spawn_actor_from_class(ps_cls, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if ps:
        ps.set_actor_label("CrapsShooter_PlayerStart")

# Spawn a DirectionalLight + SkyLight so the Graybox Pit is well lit in Editor & PIE
dl_cls = unreal.DirectionalLight
if not [a for a in actor_sub.get_all_level_actors() if a.get_class() == dl_cls]:
    dl = actor_sub.spawn_actor_from_class(dl_cls, unreal.Vector(0, 0, 1200), unreal.Rotator(-55, -35, 0))
    if dl:
        dl.set_actor_label("PitDirectionalLight")

level_sub.save_current_level()
result = {{"map": {json.dumps(map_path)}, "status": "created_and_saved"}}
"""
    return ue_execute_python(script, transaction_name="Build L_TestingPit", use_scoped_transaction=True)


if __name__ == "__main__":
    mcp.run()
