---
trigger: glob
globs: "*.h, *.cpp, *.cs, *.py"
description: Unreal Engine 5 C++ coding conventions and MCP Python automation rules for the Craps project.
---

# UE5 C++ & Editor Scripting Standards

1. **Unreal Reflection & Memory Safety:**
   - Prefix classes properly (`A` for Actors, `U` for UObjects/Components/Subsystems, `F` for Structs, `E` for Enums, `I` for Interfaces).
   - Always mark UObject pointers inside classes and structs with `UPROPERTY()` or `TObjectPtr<T>` to prevent garbage collection crashes.
   - Include `GENERATED_BODY()` and forward-declare types in `.h` files to keep compile times fast for Live Coding.

2. **Enhanced Input & Modular Hands:**
   - Use `UEnhancedInputComponent` and `UInputAction` for all POV hand interactions (On-Felt Tip, V-Set Slide, Throw Release, Left-Hand Rail Slam).
   - Implement throw styles via a modular component interface (`IThrowStyleInterface`) attached to `ACrapsPawn` so styles can be hot-swapped at runtime in the Testing Pit with `F1`–`F4`.

3. **Python MCP Execution:**
   - When executing Unreal Python scripts via MCP (`import unreal`), always wrap asset creation or actor modifications in an `unreal.ScopedEditorTransaction("Antigravity Action")` block so every AI change in the viewport can be undone with `Ctrl+Z`.