# WC3 Mod Manager
Especially with the latest patches, the most reliable way to make a mod in Warcraft III is using the "Allow Local Files" registry setting, which, as the name implies, allows files in the game's install folder to be read by the game.

This project aims to make it as easy as possible to switch between mods - which should also make it easy to use old mods which are released as MPQs.

The binary can be downloaded on [The Hive Workshop](https://www.hiveworkshop.com/threads/wc3-mod-manager.308948/).

## Core Features
* Toggle "Allow Local Files" - enable/disable the mod
* Toggle "Preferred Game Version" - switch between vanilla and expansion
* Mount/Unmount mod - automatically move the mod files to and from the game folder

# Contributing
WC3 Mod Manager is currently being developed in [Qt Creator 4.7.0](https://www.qt.io/download-qt-installer) and built with Qt 5.11.1/MinGW 32 bit. I'm not sure if other IDEs can open and build the project. MSVC should be able to compile as well, but I haven't tested.

I'm new to GitHub, so I'm not familiar with the usual procedures. Please feel free to fork, make pull requests, etc.
