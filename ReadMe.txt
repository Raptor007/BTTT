-------------------------------------------------
|           BTTT: BattleTech TableTop           |
|            0.9.7 Beta (2022-11-11)            |
|          by Raptor007 (Blair Sherman)         |
|           http://raptor007.com/bttt/          |
-------------------------------------------------

BTTT is a fan project to make a computer game of tabletop BattleTech, using the latest
Total Warfare and BattleMech Manual rules from Catalyst games:
  https://www.catalystgamelabs.com/battletech/

I started this in 2020 with the goal of playing BattleTech with proper tabletop rules
without actually gathering around a table.  This project has been a fantastic learning
experience for me, as I had to really dig into the rulebooks to understand how some of
the equipment might interact.  I did my best to faithfully reproduce it.

This should not be confused with Harebrained Schemes BattleTech, which has singleplayer
missions and better graphics, but does not exactly clone the gameplay rules of tabletop:
  https://harebrained-schemes.com/battletech/

BTTT can be a learning tool for the tabletop rules, which is why the default Event Speed
is Slow to give you time to read what happens at every step.  Weapon hits, cluster rolls,
and step-by-step damage resolution are described.  When aiming, a straight line is drawn
for line of sight, highlighting intervening hexes that would impair or block the shot.

To play across the internet, the host must map TCP port 3050.

Game Settings / Optional Rules:
* Hotseat Mode: All players take their turns on the same computer.
* Limit to 1 Mech Per Player: One Mech per connected player, or one per team in Hotseat.
* Number of Teams: Allows more factions than the normal two.  Last team alive wins.
* AI Team: Select a team to be controlled by AI rather than human players.
* Engine Explosions: Destroyed engine explodes on roll of 10+. [BattleMech Manual p.47]
* One-Armed Prone Fire: Prone with 1 arm may still fire torso. [BattleMech Manual p.30]
* Enhanced Flamers: Flamers deal both heat and damage. [BattleMech Manual p.99]
* Enhanced Missile Defense: AMS-reduced cluster below 2 misses. [BattleMech Manual p.116]
* Floating Criticals: Roll new location after rolling 2 to crit. [BattleMech Manual p.45]
* Skip TAG Phase: Disable TAG; skip to Weapon Attack phase even when TAG could be used.

Getting Started:
* After hosting or joining a game, click the Team/Mech button.
* Select your team on the left, and your BattleMech variant on the right.
* Right-click on the map to drop your selected Mech.  You can turn and move with the
  arrow keys during this Setup phase, and press Enter to commit a new starting position.
* If you want to remove your Mech, press Del or Shift-Backspace to eject.
* If playing Hotseat or vs AI, press Tab to change teams and drop the other Mech(s).
* When everyone is ready, the host should press Enter or Esc and click Initiate Combat.

Gameplay Controls:
* Left-click selects a Mech.
* Arrow keys move your selected Mech.  You can try out moves before submitting.
* Backspace will undo your movement one step at a time.
* Enter submits your turn (on any phase).
* Right-click to aim during the TAG, Weapon Attack, or Physical Attack phases.
* Left and right arrow keys adjust torso twist during Weapon Attack phase.
* WASD moves the map, R/F zooms in/out, Q/Home resets view, and E zooms to active area.
* I displays record sheet for the selected mech, and O for the right-click aim target.

Walk, run, MASC, or jump will be automatically chosen based on how far you move and if
you use any reverse movement.  If you want to jump when the move could be walked or run,
simply spin in a circle until more movement points are spent than your run/sprint speed.

Maps are randomly generated in BattleMat size.  You can also try specific map seeds by
typing "map x" in the console.  Seeds are unsigned integers (positive whole numbers).

You can create custom variants using HeavyMetal Pro and add them to the Mechs directory:
  http://www.heavymetalpro.com/HMPro_Features.htm

Known Issues:
* Dropping to prone move is not implemented; Mechs can only go prone by falling.
* Cannot choose HE or ER ammo for ATM.
* Cannot choose cluster ammo for LB-X AC.
* Ammo should be consumed at attack declaration, not resolution. [BattleMech Manual p.31]
* Multiple attack target selection is not implemented.
* Aimed shots are not implemented. [BattleMech Manual p.30]
* Club, push, charge, and DFA physical attacks are not implemented.
* CASE II should make an additional roll of 8+ to check any critical hits.
* Reactive/Reflective/Hardened Armor are not implemented.
* Stealth Armor System cannot be turned off (but does disable when ECM is damaged).
* Streak passing through any Angel ECM (not just target's) should roll on cluster table.
* Bloodhound Active Probe should be immune to ECM, unless it is Angel ECM.
* Active Probe bonus through trees should be an optional rule. [BattleMech Manual p.110]
* Anti-Missile System firing arc should probably rotate with torso twist.
* Caseless AC rolling 2 should jam and crit itself; instead it explodes.
* Hyper-velocity AC rolling 2 should destroy all of its crit slots.
* Engine explosion check should only happen if the engine takes 4 critical hits.
* Engine explosion check should still occur after torso ammo explosion kills the pilot.
* MASC is always preferred over Supercharger with both equipped; it should be a choice.
* Arrow IV homing missile rules are not implemented.
* Quadrupedal Mech rules are only partially implemented.
* Coolant Pods are not implemented.
* Command Couch (backup pilot) is not implemented.
* Turret weapons are not implemented; they only fire forward.
* C3 networks are not implemented.
* When a team has fewer Mechs than 2+ other teams, other teams should interleave turns.
* Only teammates should be able to indirect fire at spotted targets (when 3+ teams).
* Not all included Mechs look correct (but you can edit MechTex.ini to improve them).

Troubleshooting:
* Windows shows blank textures (white boxes).
 - Move the zip file to a different directory like "C:\Program Files" before extracting.
* Windows SmartScreen "protected your PC" and will not run an unrecognized app.
 - Click "More info" then "Run anyway".
* Mac shows blank textures (white boxes).
 - Use the Terminal to remove the Quarantine flag from the directory:
    xattr -dr com.apple.quarantine "/Applications/BTTT"
* Mac will not allow a downloaded or unsigned application to run.
 - Use the Terminal to enable "allow apps downloaded from anywhere":
    sudo spctl --master-disable
* When starting the game, "Loading Mechs" is sometimes very slow.
 - Move BTTT to an SSD or remove some files from the Mechs folder.

Frequently Asked Questions (FAQ):
* How do I set up a game against the computer?
 - First select which team you would like the AI to control from the main menu.  Then
   press Tab to bring up the Team/Mech menu, switch to the AI team, and right-click to
   drop Mechs for them.  Then switch to your team, and drop your Mechs.  Finally press
   F10 to bring up the main menu, then press Return or click "Initiate Combat".
* How does BTTT compare to MegaMek?
 - I have not used MegaMek because it requires Java.  But I can see from the tutorial
   videos that it is an impressive effort to implement nearly every feature described
   in any BattleTech rulebook, such as hidden units, radar modes, non-Mech units, etc.
   By contrast, BTTT streamlines just the core gameplay rules of the BattleMech Manual
   and Total Warfare, attempting to clone how my friends and I play our tabletop games.
   Movements, attacks, and damage resolution are animated and described step by step,
   tracking damage on record sheets.  It is written in C++ and compiled to native code.
* Why do I need HeavyMetal Pro to make custom Mechs?
 - I chose HeavyMetal Pro to make custom record sheets for my tabletop games because it
   was the best tool I could find as a stand-alone executable; most others require Java.
   When I later decided to make BTTT, it made sense to try using the Mech variant files
   I already had instead of tediously copying these stats from rulebooks.
* Will there be a mobile/tablet port?
 - Maybe someday, but no promises.  I do have an Android phone now but prefer PC gaming.
   BTTT and RaptorEngine are both open source C++, so you are welcome to try porting it.
   You would probably need to ditch my SDL 1.2 bindings for SDL2, rework a few older GL
   calls that are not available in OpenGLES, and replace keyboard controls with touch.

Version 0.9.7 Beta (2022-11-11):
* Aiming exactly along hex edge now chooses one alternate path. [BattleMech Manual p.22]
* Mech variant search box now also searches equipment, era, and other properties.
* Scenario files can now be used to more quickly set up games.  Try making your own!
* Fixed punch attacks available for Mech after another player fired its arm weapons.
* Fixed AI-controlled Mechs using torso twist while prone.
* Fixed loud startup sound when joining a game that already has many Mechs.
* Fixed crash in dedicated servers when an AI-controlled Mech uses jump jets.
* Some improvements to AI.  They are less likely to get stuck and use NARC smarter.
* Increased probability of heavy woods in random map generation.
* Attack resolution now skips Mechs that declared 0 weapons to fire.
* Various minor UI improvements.  F9-F11 toggle menus.  Record sheets now show tonnage.
* Tweaked some animations to look better.  Mechs no longer step beneath friendlies.
* When the battle ends, Betty now announces mission successful or failed.
* Moved custom designs to "Mechs (Disabled)".  Move them to "Mechs" to display in menu.
* Defined texture data for every included stock Mech, though some could use improvement.
Version 0.9.6 Beta (2022-07-07):
* The new Biome setting changes map colors, and you can edit MapBiome.ini to customize.
* Clicking any Mech that has declared attacks now shows the attacks and to-hit rolls.
* Torso twist is now displayed with attack declaration instead of resolution.
* Spot for Indirect Fire no longer remains checked on subsequent turns.
* Fixed some lines-of-sight checks for Mechs two hexes apart (one hex between them).
* Fixed bug that allowed Mechs with jump jets to run up L3+ hills as Minimum Movement.
* Fixed AI using torso twist when unnecessary (no shots) or illegal (unconscous).
* Clarified setup prompts, especially when using AI teams.
Version 0.9.5 Beta (2022-06-22):
* Attack and defense modifiers are displayed while planning movement.
* Declared attacks now show what they are targeting.
* ECM ranges are now only displayed for the selected Mech or when aim passes through.
* Fixed cover for prone Mechs in some situations by reworking line-of-sight code.
* Fixed scroll behavior for Mech variant list.
* Fixed draw order problems, such as jumping Mechs sometimes drawn behind others.
* Fixed armor diagram sometimes not flashing damage when hit from side or rear arc.
* Fixed players without TAG being able to skip the turn when a teammate could use TAG.
* Fixed remote client desync/crash when too many events were transmitted in a batch.
* Fixed queued events continuing to play after ending the game early.
* Fixed AI-controlled Mechs standing up without spending the necessary MP.
* AI now avoids running/jumping when its Mech has damaged Gyro/Hip/Actuators.
* Ludicrous Speed now has better looking animations.
* Added and improved some Mech texture definitions.
Version 0.9.4 Beta (2022-03-14):
* Fixed to-hit previews in Movement phase not adding walk/run/jump attack modifier.
* Fixed non-explosive ammo (Gauss) still being usable after critical hit.
* Fixed several bugs related to Minimum Movement rule. [BattleMech Manual p.16]
* Fixed AI using torso twist with TAG; torso twist is set during Weapon Attack.
* Mechs controlled by AI move a little smarter and can now use arm flipping.
* Implemented weapons that deal both damage and heat to target (Plasma Rifles).
* Enabling hotseat mode mid-game switches your team to the current turn.
* Default game settings no longer limit to 1 Mech per player.
* Defined texture information for even more Mechs.
Version 0.9.3 Beta (2022-02-22):
* Added rudimentary AI code so you can play against the computer.
* Can preview to-hit rolls during Movement phase by right-clicking potential targets.
* Mech variant list in spawn menu is now searchable.
* The number of teams can now be changed in the setup menu.
* Defined texture information for more Mechs.
Version 0.9.2 Beta (2022-01-22):
* Added Record Sheet info windows.  Press I to view for selected Mech, O for target.
* Run/walk is now declared when standing.  Up arrow to stand running, down for walking.
* Fixed Mechs occasionally being unable to do some actions on the first turn of a phase.
* Fixed partial cover so it no longer applies downhill. [BattleMech Manual p.26]
* Fixed damage from Engine Explosion to use groups of 5 instead of one big hit.
* Fixed some issues when connecting to a game in progress, such as loud startup sound.
* Fixed wrong value shown in "O" of "GATOR" when target has movement defense bonus.
* Fixed Right Leg sometimes being called "Right Arm".
* Fixed pilot damage heat thresholds for torso cockpit when life support is damaged.
* Fixed run with failed MASC roll allowing reverse or jump moves and not adding 2 heat.
* Implemented Engine Supercharger. [Tactical Operations p.345]
* Mostly implemented CASE II, Laser/Compact Heat Sinks, and Mechanical Jump Boosters.
* Improved compatibility with some HeavyMetal Pro variant files.
* Other small improvements such as draggable windows and middle-click to drag the map.
Version 0.9.1 Beta (2021-09-19):
* Chat appears on screen, and you can press T to type a message.
* Improved map graphics to look like alpine/grasslands, and other minor visual tweaks.
* Fixed UAC/RAC rapid fire so it no longer combines all hits into one damage location.
* Fixed automatic fall not being triggered after rolling 12 on leg critical hit check.
* Fixed missing critical hit check when physical attack hits location 2 on prone target.
* Fixed incorrect placement of weapons when the same type is mounted front and rear.
* Cleanup and clarification of several damage resolution events.
* Like physical attack, TAG is automatically skipped when there are no valid targets.
* Spotting for indirect fire is announced when declared, not while resolving attacks.
* Remote players can change game options on a dedicated server.
Version 0.9 Beta (2021-09-01):
* First release.
