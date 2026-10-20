---
name: craps-testing
description: Builds CrapsCrapsCraps and runs Unreal Engine 5.3 Automation Tests headless with parsed result reporting.
---

# CrapsCrapsCraps Headless Test Runner

This skill describes how to run and verify the automation test suite for **CrapsCrapsCraps** (UE 5.3).

## Test Commandlet Execution

Run the automation tests using `UnrealEditor-Cmd.exe` via PowerShell:

```powershell
& "C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Code\Unreal Projects\Craps\CrapsCrapsCraps\CrapsCrapsCraps.uproject" -ExecCmds="Automation RunTests Craps; Quit" -Run -stdout -nullrhi -unattended -nosplash -NoSound
```

## Parsing Test Results

- Search for `Automation Test Queue Empty` or `Test Results:` in the output.
- All tests matching filter `Craps` (including `Craps.Betting.*`, `Craps.Dice.*`, `Craps.Physics.*`, `Craps.Progression.*`) will be reported with `Success` or `Failed`.
- Total expected tests: 29.
