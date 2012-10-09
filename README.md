Arcon (Calamity Fuse)
=====================

Arcon was a Source mod that I worked on a long time ago. This code contains some interesting tidbits that some people may find useful, and I wanted a backup, so I've uploaded it here.

Valve's SDK source codes are absent, not included here. I made more than a few modifications to them while developing CF, which this code may rely on.

The code probably doesn't build anymore and I was in the middle of some things when I stopped working on it so anyone attempting to build it will likely encounter serious problems. I don't recommend it.

That said there's a few interesting things here, such as:

* The "primary/secondary" weapon system.
* The player's eye point is affixed to the eye of their player model, looking down provides the player a view of his legs, the weapon the player sees in first person is the weapon in the player model's hands, not a v model.
* The "choreo" directory is a movie making system which is now obsolete because of SFM.
* The instructor (cf_instructor.cpp/h) I am particularly proud of.
* The entire UI is a custom drawn UI (cfui_gui.cpp/h) on top of a single vgui panel. It formed the basis for the UI in my later projects such as SMAK and Digitanks. The latest version of it is available in my SMAK repo.
* All of the movement code for things like latching to walls.

And so on.

One last note: The majority of this code was written by me and me only, with a few tidbits of help from Tony Sergi here and there. It doesn't represent the work of anybody else on the team. I'm saying this because I don't want anybody else blamed for my previous incompetence. (Or as we used to say while working on this, I'll take all of the credit and or blame.)