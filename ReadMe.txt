-------------------------------------------------
|           BTTT: BattleTech TableTop           |
|             0.9 Beta (2021-09-01)             |
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
* 1 Mech Per Player: Limit to one Mech per connected player, or one per team in Hotseat.
* Engine Explosions: Destroyed engine explodes on roll of 10+. [BattleMech Manual p.47]
* One-Armed Prone Fire: Prone with 1 arm may still fire torso. [BattleMech Manual p.30]
* Enhanced Flamers: Flamers deal both heat and damage. [BattleMech Manual p.99]
* Enhanced Missile Defense: AMS-reduced cluster below 2 misses. [BattleMech Manual p.116]
* Floating Criticals: Roll new location after rolling 2 to crit. [BattleMech Manual p.45]
* Skip TAG Phase: Disable TAG; skip to Weapon Attack phase even when TAG could be used.

Getting Started:
* After hosting or joining a game, click the Team/Mech button.
* Select your team on the left, and your BattleMech variant on the right.
* Right-click on the map to spawn your selected Mech.  You can turn and move with the
  arrow keys during this Setup phase, and press Enter to commit a new starting position.
* If you want to remove your Mech, press Del or Shift-Backspace to eject.
* When everyone is ready, the host should press Esc and click Initiate Combat.

Gameplay Controls:
* Left-click selects a Mech.
* Arrow keys move your selected Mech.  You can try out moves before submitting.
* Backspace will undo your movement one step at a time.
* Enter submits your turn (on any phase).
* Right-click to aim during the TAG, Weapon Attack, or Physical Attack phases.
* WASD moves the map, R/F zooms in/out, Q resets view, and E zooms to active area.

Walk, run, MASC, or jump will be automatically chosen based on how far you move and if
you use any reverse movement.  If you want to jump when the move could be walked or run,
simply spin in a circle until more movement points are spent than your run/sprint speed.

Maps are randomly generated in BattleMat size.  You can also try specific map seeds by
typing "map x" in the console.  Seeds are unsigned integers (positive whole numbers).

You can create custom variants using HeavyMetal Pro and add them to the Mechs directory:
  http://www.heavymetalpro.com/HMPro_Features.htm

Known Issues:
* Run/walk speed should be declared before making stand attempts.
* Dropping to prone move is not implemented; Mechs can only go prone by falling.
* Cannot choose HE or ER ammo for ATM.
* Cannot choose cluster ammo for LB-X AC.
* Multiple attack target selection is not implemented.
* Called/aimed shots are not implemented.
* Club, push, charge, and DFA physical attacks are not implemented.
* CASE II is only implemented as CASE, without the additional features.
* Reactive/Reflective/Hardened Armor are not implemented.
* Stealth Armor System cannot be turned off (but does disable when ECM is damaged).
* Streak passing through any Angel ECM (not just target's) should roll on cluster table.
* Anti-Missile System firing arc should probably rotate with torso twist.
* Caseless AC rolling 2 should jam and crit itself; instead it explodes.
* Hyper-velocity AC rolling 2 should destroy all of its crit slots.
* Engine explosion check should only happen if the engine takes 4 critical hits.
* Arrow IV homing missile rules are not implemented.
* Quadrupedal Mech rules are only partially implemented.
* Coolant Pods are not implemented.
* Command Couch (backup pilot) is not implemented.
* C3 networks are not implemented.
* Not all included Mechs look correct (but you can add them to MechTex.ini).

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
