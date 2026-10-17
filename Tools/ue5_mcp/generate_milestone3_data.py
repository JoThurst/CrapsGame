#!/usr/bin/env python3
"""
generate_milestone3_data.py

Idempotent batch generator for Milestone 3.2 Data Assets:
- 8 UPDA_DiceFace assets in /Game/Data/DiceFaces/
- 15 UPDA_Trinket assets in /Game/Data/Trinkets/ (8 Finger Rings, 4 Active Wristwear, 3 Rail Props)

Can be executed inside Unreal Engine 5.3 via PythonScriptPlugin or ue5_mcp server.
"""

import unreal

def get_or_create_asset(asset_name: str, package_path: str, asset_class):
    """Load existing asset or create a new one idempotently."""
    full_path = f"{package_path.rstrip('/')}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        return unreal.EditorAssetLibrary.load_asset(full_path)
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    return asset_tools.create_asset(asset_name, package_path.rstrip('/'), asset_class, factory)

def set_gameplay_tag(asset, property_name: str, tag_str: str):
    """Set an FGameplayTag property on an asset using struct import/set."""
    try:
        tag = unreal.GameplayTag()
        tag.import_text(f'(TagName="{tag_str}")')
        asset.set_editor_property(property_name, tag)
    except Exception as e:
        print(f"Warning: could not set gameplay tag {tag_str} on {property_name}: {e}")

def add_tag_to_container(container, tag_str: str):
    """Add a tag to an FGameplayTagContainer."""
    try:
        tag = unreal.GameplayTag()
        tag.import_text(f'(TagName="{tag_str}")')
        unreal.GameplayTagLibrary.add_gameplay_tag(container, tag)
    except Exception as e:
        print(f"Warning: could not add tag {tag_str} to container: {e}")

def generate_dice_faces():
    """Generates the 8 core UPDA_DiceFace assets in /Game/Data/DiceFaces/."""
    package_path = "/Game/Data/DiceFaces"
    unreal.EditorAssetLibrary.make_directory(package_path)
    
    dice_face_class = unreal.load_class(None, "/Script/CrapsCrapsCraps.PDA_DiceFace")
    if not dice_face_class:
        raise RuntimeError("Failed to load UPDA_DiceFace class from /Script/CrapsCrapsCraps.PDA_DiceFace")

    faces_data = [
        {
            "name": "DA_Face_LoadedSix",
            "title": "Loaded Six",
            "desc": "Lead-weighted pip face tuned for heavy high-roller variance.",
            "value": 6,
            "sec_val": 0,
            "chips": 30,
            "throw_mult": 2.0,
            "cash": 0,
            "tags": ["Craps.Face.Weighted"],
            "color": unreal.LinearColor(0.0, 0.96, 0.83, 1.0) # Phosphor Cyan
        },
        {
            "name": "DA_Face_SnakeEyePip",
            "title": "Snake Eye Ace",
            "desc": "The deadly single pip. Catastrophic alone, lethal in pairs.",
            "value": 1,
            "sec_val": 0,
            "chips": 50,
            "throw_mult": 4.0,
            "cash": 0,
            "tags": ["Craps.Face.SnakeEyes"],
            "color": unreal.LinearColor(1.0, 0.12, 0.26, 1.0) # CRT Crimson
        },
        {
            "name": "DA_Face_GoldPip",
            "title": "Gold Bullion Pip",
            "desc": "Minted in pure aurum. Converts kinetic force into hard bankroll credit.",
            "value": 5,
            "sec_val": 0,
            "chips": 20,
            "throw_mult": 1.0,
            "cash": 25,
            "tags": ["Craps.Face.GoldBullion"],
            "color": unreal.LinearColor(1.0, 0.72, 0.01, 1.0) # Gold Amber
        },
        {
            "name": "DA_Face_MirrorPip",
            "title": "Chromeslide Mirror Face",
            "desc": "Reflective silver plating that duplicates the partner die's settled face value.",
            "value": 4,
            "sec_val": 0,
            "chips": 15,
            "throw_mult": 1.5,
            "cash": 0,
            "tags": ["Craps.Face.Mirror"],
            "color": unreal.LinearColor(0.85, 0.90, 1.0, 1.0) # Silver White
        },
        {
            "name": "DA_Face_EMPShockPip",
            "title": "EMP Shock Pip",
            "desc": "Discharges capacitor arcs on table impact, destabilizing magnetic rails.",
            "value": 3,
            "sec_val": 0,
            "chips": 25,
            "throw_mult": 3.0,
            "cash": 0,
            "tags": ["Craps.Face.EMP"],
            "color": unreal.LinearColor(0.1, 0.6, 1.0, 1.0) # Electric Blue
        },
        {
            "name": "DA_Face_VampirePip",
            "title": "Vampiric Drain Pip",
            "desc": "Siphons chip energy from lost proposition bets and feeds the throw multiplier.",
            "value": 2,
            "sec_val": 0,
            "chips": 10,
            "throw_mult": 2.5,
            "cash": 0,
            "tags": ["Craps.Face.Vampire"],
            "color": unreal.LinearColor(0.7, 0.0, 0.35, 1.0) # Blood Violet
        },
        {
            "name": "DA_Face_Hardwaypip",
            "title": "Tuned Hardway Pip",
            "desc": "Resonates with identical partner faces to boost Hardway bet multipliers.",
            "value": 4,
            "sec_val": 4,
            "chips": 40,
            "throw_mult": 2.0,
            "cash": 0,
            "tags": ["Craps.Face.Hardway"],
            "color": unreal.LinearColor(1.0, 0.0, 0.5, 1.0) # Synth Magenta
        },
        {
            "name": "DA_Face_LuckySevenPip",
            "title": "Overdrive Seven Pip",
            "desc": "Phosphor-hot pip that inverts Seven-Out penalties into jackpot multipliers on Come-Out rolls.",
            "value": 6,
            "sec_val": 0,
            "chips": 77,
            "throw_mult": 7.0,
            "cash": 7,
            "tags": ["Craps.Face.OverdriveSeven"],
            "color": unreal.LinearColor(1.0, 0.85, 0.1, 1.0) # High Voltage Amber
        }
    ]

    created = []
    for item in faces_data:
        asset = get_or_create_asset(item["name"], package_path, dice_face_class)
        if not asset:
            print(f"Error creating face asset: {item['name']}")
            continue

        asset.set_editor_property("face_name", unreal.Text(item["title"]))
        asset.set_editor_property("face_description", unreal.Text(item["desc"]))
        asset.set_editor_property("face_value", item["value"])
        asset.set_editor_property("secondary_face_value", item["sec_val"])
        asset.set_editor_property("base_chip_bonus", item["chips"])
        asset.set_editor_property("throw_mult_bonus", item["throw_mult"])
        asset.set_editor_property("direct_cash_bonus", item["cash"])
        asset.set_editor_property("pip_emissive_color", item["color"])

        container = unreal.GameplayTagContainer()
        for t in item["tags"]:
            add_tag_to_container(container, t)
        try:
            asset.set_editor_property("face_ability_tags", container)
        except Exception as e:
            print(f"Notice setting tags on {item['name']}: {e}")

        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        created.append(asset.get_path_name())
        print(f"Saved DiceFace Asset: {item['name']}")

    return created

def generate_trinkets():
    """Generates the 15 UPDA_Trinket assets in /Game/Data/Trinkets/."""
    package_path = "/Game/Data/Trinkets"
    unreal.EditorAssetLibrary.make_directory(package_path)

    trinket_class = unreal.load_class(None, "/Script/CrapsCrapsCraps.PDA_Trinket")
    if not trinket_class:
        raise RuntimeError("Failed to load UPDA_Trinket class from /Script/CrapsCrapsCraps.PDA_Trinket")

    # Slot types: FingerRing = 0, WristwearActive = 1, RailProp = 2
    # Layer types: BaseChips = 0, ThrowMult = 1, BetMult = 2
    trinkets_data = [
        # --- 8 Finger Rings ---
        {
            "name": "DA_Trinket_ObsidianSignet",
            "title": "Obsidian Signet",
            "desc": "+15 Base Chips awarded on any Hardway roll.",
            "slot": 0, "layer": 0, "chips": 15, "throw_mult": 0.0, "bet_mult": 1.0,
            "tag": "Craps.Trigger.OnHardway"
        },
        {
            "name": "DA_Trinket_NeonSignet",
            "title": "Neon Signet Ring",
            "desc": "+1.5x Throw Mult whenever both dice impact the back wall alligator padding.",
            "slot": 0, "layer": 1, "chips": 0, "throw_mult": 1.5, "bet_mult": 1.0,
            "tag": "Craps.Trigger.OnWallBounce"
        },
        {
            "name": "DA_Trinket_BrassKnuckleBand",
            "title": "Heavy Brass Band",
            "desc": "Heavy felt impact: grants +25 Base Chips when dice land in the Field.",
            "slot": 0, "layer": 0, "chips": 25, "throw_mult": 0.0, "bet_mult": 1.0,
            "tag": "Craps.Trigger.OnFieldSettle"
        },
        {
            "name": "DA_Trinket_PyriteRing",
            "title": "Fool's Gold Band",
            "desc": "1.25x Bet Mult applied to any Craps roll (2, 3, or 12).",
            "slot": 0, "layer": 2, "chips": 0, "throw_mult": 0.0, "bet_mult": 1.25,
            "tag": "Craps.Trigger.OnCraps"
        },
        {
            "name": "DA_Trinket_CopperLoop",
            "title": "Grounding Copper Loop",
            "desc": "Absorbs static energy, adding +10 Base Chips and +0.5x Mult per bounce.",
            "slot": 0, "layer": 0, "chips": 10, "throw_mult": 0.5, "bet_mult": 1.0,
            "tag": "Craps.Trigger.OnFeltBounce"
        },
        {
            "name": "DA_Trinket_SilverSextant",
            "title": "Shooter's Sextant",
            "desc": "+2.0x Throw Mult when settled within the target ring zone.",
            "slot": 0, "layer": 1, "chips": 0, "throw_mult": 2.0, "bet_mult": 1.0,
            "tag": "Craps.Trigger.OnTargetRingHit"
        },
        {
            "name": "DA_Trinket_CobaltBand",
            "title": "Cobalt Resonance Ring",
            "desc": "1.5x Bet Mult when successfully hitting the established Point.",
            "slot": 0, "layer": 2, "chips": 0, "throw_mult": 0.0, "bet_mult": 1.5,
            "tag": "Craps.Trigger.OnPointHit"
        },
        {
            "name": "DA_Trinket_OnyxClaw",
            "title": "Onyx Claw",
            "desc": "+50 Base Chips if the roll results in Any Seven on Come-Out.",
            "slot": 0, "layer": 0, "chips": 50, "throw_mult": 1.0, "bet_mult": 1.0,
            "tag": "Craps.Trigger.OnComeOutSeven"
        },

        # --- 4 Active Wristwear ---
        {
            "name": "DA_Trinket_ChronoChronometer",
            "title": "Sub-Second Chronometer",
            "desc": "Active Time-Dilation: Expands the precision torque control window during throw descent.",
            "slot": 1, "layer": 1, "chips": 0, "throw_mult": 3.0, "bet_mult": 1.0,
            "tag": "Craps.Ability.TimeDilation"
        },
        {
            "name": "DA_Trinket_MagneticGauntlet",
            "title": "Flux Magnetic Wristband",
            "desc": "Active Pull: Subtly guides rolling dice toward center felt track.",
            "slot": 1, "layer": 0, "chips": 40, "throw_mult": 1.0, "bet_mult": 1.0,
            "tag": "Craps.Ability.MagneticPull"
        },
        {
            "name": "DA_Trinket_PulseBracer",
            "title": "Shockwave Bracer",
            "desc": "Converts physical table impact into +2.5x Throw Multiplier.",
            "slot": 1, "layer": 1, "chips": 20, "throw_mult": 2.5, "bet_mult": 1.0,
            "tag": "Craps.Ability.KineticPulse"
        },
        {
            "name": "DA_Trinket_CyberneticCuff",
            "title": "Overdrive Wire Cuff",
            "desc": "Direct neural link to scoring subsystem: Multiplies all bet payouts by 1.35x.",
            "slot": 1, "layer": 2, "chips": 0, "throw_mult": 0.0, "bet_mult": 1.35,
            "tag": "Craps.Ability.NeuralLink"
        },

        # --- 3 Rail Props ---
        {
            "name": "DA_Trinket_AshtrayTotem",
            "title": "Vintage Chrome Ashtray",
            "desc": "Lucky smoke totem: Protects against one Seven-Out per run and awards +100 Base Chips.",
            "slot": 2, "layer": 0, "chips": 100, "throw_mult": 0.0, "bet_mult": 1.0,
            "tag": "Craps.Prop.SevenShield"
        },
        {
            "name": "DA_Trinket_ZippoLighter",
            "title": "Engraved Trench Lighter",
            "desc": "Flickering phosphor flame: Grants +4.0x Throw Mult on all Hardway rolls.",
            "slot": 2, "layer": 1, "chips": 0, "throw_mult": 4.0, "bet_mult": 1.0,
            "tag": "Craps.Prop.FlameResonance"
        },
        {
            "name": "DA_Trinket_WhiskeyTumbler",
            "title": "Heavy Crystal Tumbler",
            "desc": "Amber whiskey highball resting on the rail: 2.0x Bet Mult on High-Roller propositions.",
            "slot": 2, "layer": 2, "chips": 0, "throw_mult": 0.0, "bet_mult": 2.0,
            "tag": "Craps.Prop.LiquidConfidence"
        }
    ]

    # Inspect enum types if available
    slot_enum = getattr(unreal, "TrinketSlotType", None)
    layer_enum = getattr(unreal, "ScoringLayer", None)

    created = []
    for item in trinkets_data:
        asset = get_or_create_asset(item["name"], package_path, trinket_class)
        if not asset:
            print(f"Error creating trinket asset: {item['name']}")
            continue

        asset.set_editor_property("trinket_name", unreal.Text(item["title"]))
        asset.set_editor_property("description", unreal.Text(item["desc"]))
        
        # Set slot type (enum or int)
        if slot_enum:
            slot_names = ["FINGER_RING", "WRISTWEAR_ACTIVE", "RAIL_PROP"]
            enum_val = getattr(slot_enum, slot_names[item["slot"]], item["slot"])
            asset.set_editor_property("slot_type", enum_val)
        else:
            try:
                asset.set_editor_property("slot_type", item["slot"])
            except Exception:
                pass

        # Set scoring layer (enum or int)
        if layer_enum:
            layer_names = ["BASE_CHIPS", "THROW_MULT", "BET_MULT"]
            enum_val = getattr(layer_enum, layer_names[item["layer"]], item["layer"])
            asset.set_editor_property("target_scoring_layer", enum_val)
        else:
            try:
                asset.set_editor_property("target_scoring_layer", item["layer"])
            except Exception:
                pass

        asset.set_editor_property("bonus_base_chips", item["chips"])
        asset.set_editor_property("bonus_throw_mult", item["throw_mult"])
        asset.set_editor_property("bet_mult_factor", item["bet_mult"])

        set_gameplay_tag(asset, "trigger_condition_tag", item["tag"])

        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        created.append(asset.get_path_name())
        print(f"Saved Trinket Asset: {item['name']}")

    return created

def main():
    print("=== Starting Milestone 3.2 Asset Generation ===")
    faces = generate_dice_faces()
    print(f"Generated {len(faces)} UPDA_DiceFace assets.")
    trinkets = generate_trinkets()
    print(f"Generated {len(trinkets)} UPDA_Trinket assets.")
    print("=== Milestone 3.2 Asset Generation Complete ===")
    return {"dice_faces": len(faces), "trinkets": len(trinkets)}

if __name__ == "__main__" or __name__ == "builtins":
    result = main()
